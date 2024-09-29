#include "monitor_task.hpp"
struct ContextSwitchTask : public MonitorTask {
    ContextSwitchTask() {
        pid_status_file_ =
            std::ifstream("/proc/" + std::to_string(pid_) + "/status");
        if (!pid_status_file_.is_open()) {
            std::cerr << "Failed to open /proc/" << pid_ << "/status"
                      << std::endl;
        }
    }
    std::string describe() const noexcept override { return "context_switches"; }

    virtual nlohmann::json get_host_data() override { return {}; }
    virtual nlohmann::json get_pid_data() override {
        this->pid_status_file_.clear();
        this->pid_status_file_.seekg(0, std::ios::beg);
        nlohmann::json ret;
        std::string line;
        std::getline(pid_status_file_, line);
        std::istringstream iss(line);
        std::vector<std::string> tokens(std::istream_iterator<std::string>{iss},
                                        std::istream_iterator<std::string>());
        if (tokens.size() > 17) {
            ret["minflt"] =
                std::stoi(tokens[9]);  // minor faults 不需要从硬盘加载页面
            ret["majflt"] =
                std::stoi(tokens[11]);  // major faults 需要从硬盘加载页面
            ret["nvcsw"] = std::stoi(tokens[19]);  // voluntary context switches
            ret["nivcsw"] =
                std::stoi(tokens[20]);  // involuntary context switches
        }
        return ret;
    }

    ~ContextSwitchTask() { pid_status_file_.close(); }

   private:
    std::ifstream pid_status_file_;
};