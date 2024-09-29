#include <chrono>

#include "./collection_instance.hpp"
#include "./monitor/context_switches_task.hpp"
#include "./monitor/cpu_task.hpp"
#include "./monitor/disk_io_task.hpp"
#include "./monitor/memory_task.hpp"
#include "./monitor/network_task.hpp"
#include "./monitor/threads_fd_task.hpp"

template <class DATA_T>
class MonitorInstance {
   public:
    explicit MonitorInstance(
        std::string path, int max_size = 1 << 4,
        int max_io_thread_count_ = std::thread::hardware_concurrency() << 1,
        std::string data_file_name = "system_runtime.data") {
        collection_instance_ptr_ = new CollectionInstance<DATA_T>(
            path, max_size, max_io_thread_count_, data_file_name);
    }
    void record_ctx_swtich() { this->system_push(&(this->ctx_swtich_task_)); }
    void record_network() { this->system_push(&(this->network_task_)); }
    void record_thread_fd() { this->system_push(&(this->thread_fd_task_)); }
    void record_cpu() { this->system_push(&(this->cpu_task_)); }
    void record_disk() { this->system_push(&(this->disk_task_)); }
    void record_mem() { this->system_push(&(this->mem_task_)); }

    // for user special task
    template <class Handler>
        requires std::is_base_of<MonitorTask, Handler>::value
    void record_custom(Handler handler) {
        this->system_push(&handler);
    }

    ~MonitorInstance() { delete collection_instance_ptr_; }

   private:
    void system_push(MonitorTask* sys_task) {
        auto data = sys_task->get_pid_data();
        data["type"] = sys_task->describe();
        if (data.size() > 0) {
            this->push(std::move(data));
        }
        data = sys_task->get_host_data();
        if (data.size() > 0) {
            this->push(std::move(data));
        }
    }
    void push(nlohmann::json data) {
        data["timestamp"] = timestamp();
        this->collection_instance_ptr_->push(data.dump(4));
    }
    inline static std::string timestamp() noexcept {
        auto now = std::chrono::high_resolution_clock::now();

        auto timestamp_microseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(
                now.time_since_epoch())
                .count();
        return std::to_string(timestamp_microseconds);
    }

   private:
    DiskIoTask disk_task_;
    CPUTask cpu_task_;
    MemoryTask mem_task_;
    ThreadsAndFdTask thread_fd_task_;
    NetWorkTask network_task_;
    ContextSwitchTask ctx_swtich_task_;
    CollectionInstance<std::string>* collection_instance_ptr_;
    //
};
