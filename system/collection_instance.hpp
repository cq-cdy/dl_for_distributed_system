#pragma once
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <list>
#include <thread>

namespace fs = std::filesystem;

template <class T>
struct CollectionInstance {
    CollectionInstance(
        std::string path, int max_size = 1 << 16,
        int max_io_thread_count_ = std::thread::hardware_concurrency() << 1,
        std::string data_file_name = "system_runtime.data")
        : max_size_(max_size),
          max_io_thread_count_(max_io_thread_count_),
          data_file_name_(data_file_name) {
        //
        namespace fs = std::filesystem;
        fs::path baseDir = path;
        fs::path fullPath = baseDir / data_file_name;
        file_ = std::ofstream(fullPath, std::ios::out | std::ios::app);
        if (!file_.is_open()) {
            std::runtime_error("Failed to open file.");
        }
        is_runing_ = true;
    }

    ~CollectionInstance() {
        std::lock(this->io_mtx_, this->push_mtx_);
        std::lock_guard<std::mutex> lk1(io_mtx_, std::adopt_lock);
        std::lock_guard<std::mutex> lk2(push_mtx_, std::adopt_lock);
        is_runing_ = false;
        if (mem_data_ != nullptr) {
            delete mem_data_;
        }
        file_.flush();
        file_.close();
    }
    void push(T data) noexcept {
        std::unique_lock lock(this->push_mtx_);
        if (!is_runing_) {
            if (mem_data_ != nullptr) {
                delete mem_data_;
            }
            return;
        }
        auto N = mem_data_->size();
        mem_data_->push_back(data);
        printf("[%d / %d]\n", N, max_size_);
        if (N > max_size_) {
            std::list<T>* disk_io_data = mem_data_;
            mem_data_ = new std::list<T>();
            cv_.wait(lock,
                     [this]() -> bool {  // if to much io_trhead,then wait here;
                         return this->io_thread_count_.load() <
                                this->max_io_thread_count_;
                     });
            std::thread([this, disk_io_data]() {
                printf("cur io count %d \n", this->io_thread_count_.load());
                this->io_thread_count_.fetch_add(1);
                this->diskio(disk_io_data);
                //auto cur_count = io_thread_count_.load();
                this->cv_.notify_one();
                this->io_thread_count_.fetch_sub(1);
            }).detach();
        }

        // bool expected = false;

        // while (!is_adding_.compare_exchange_strong(expected, true));
        // auto N = mem_data_->size();
        // // to here is_adding_ must be true;
        // if (not is_runing_) {
        //     is_adding_.store(false);
        //     return;
        // }
        // (*mem_data_).push_back(data);
        // printf("[%d / %d]\n", N, max_size);
        // if (N >= max_size) {
        //     std::list<T>* disk_io_data = mem_data_;
        //     mem_data_ = new std::list<T>();
        //     if (is_runing_ == false) {
        //         delete mem_data_;
        //         delete disk_io_data;
        //         is_adding_.store(false);
        //         return;
        //     }
        //     std::thread([this, disk_io_data]() {
        //         this->diskio(disk_io_data);
        //     }).detach();
        // }
        // is_adding_.store(false);
    }

   private:
    void diskio(std::list<T>* diskio_data) {
        std::lock_guard lock(this->io_mtx_);
        sleep(1);
        if (!is_runing_) {
            if (diskio_data != nullptr) {
                delete diskio_data;
            }
            return;
        }
        if (diskio_data == nullptr) {
            return;
        }

        for (const auto& data : *diskio_data) {
            if (not is_runing_) {
                break;
            }

            file_ << data << "\n";
        }
        if (diskio_data != nullptr) {
            delete diskio_data;
        }
    }

   private:
    std::ofstream file_;
    std::list<T>* mem_data_ = new std::list<T>();

    std::mutex push_mtx_{};
    std::mutex io_mtx_{};
    bool is_runing_{};
    std::atomic<int> io_thread_count_{0};
    std::condition_variable cv_;

    int max_size_{};
    int max_io_thread_count_{};
    std::string data_file_name_{};

    // std::atomic<bool> is_adding_{false};
};