#pragma once
#include "monitor_pub.hpp"
struct MonitorTask {
    virtual std::string describe() const noexcept = 0;

    virtual nlohmann::json get_host_data() = 0;
    virtual nlohmann::json get_pid_data() { return {}; }
    void set_pid(pid_t t_pid) noexcept{
        this->pid_ = t_pid;
    }
    pid_t pid_ = getpid();
};