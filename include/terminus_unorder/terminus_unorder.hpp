#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <future>
#include <chrono>
#include <thread>
#include <queue>
#include <mutex>
#include <functional>

namespace terminus {

// ==========================================
// 1. 通用节点接口抽象
// ==========================================
class INode {
public:
    virtual ~INode() = default;

    // 节点唯一名称
    [[nodiscard]] virtual std::string Name() const = 0;

    // 执行退出清理。返回 true 表示成功
    virtual bool Shutdown() = 0;

    // 该节点允许的最大清理时间
    [[nodiscard]] virtual std::chrono::milliseconds Timeout() const {
        return std::chrono::seconds(3);
    }
};

// ==========================================
// 2. Lambda 快速封装器 (便捷工具)
// 允许用户直接用函数闭包创建 Node，而无需显式继承
// ==========================================
class FunctionalNode : public INode {
public:
    FunctionalNode(std::string name, std::function<bool()> shutdown_func,
                   const std::chrono::milliseconds timeout = std::chrono::seconds(3))
        : name_(std::move(name)), func_(std::move(shutdown_func)), timeout_(timeout) {}

    [[nodiscard]] std::string Name() const override { return name_; }
    bool Shutdown() override { return func_ ? func_() : false; }
    [[nodiscard]] std::chrono::milliseconds Timeout() const override { return timeout_; }

private:
    std::string name_;
    std::function<bool()> func_;
    std::chrono::milliseconds timeout_;
};

// ==========================================
// 3. 通用无序退出协调器引擎
// ==========================================
class UnorderCoordinator {
public:
    UnorderCoordinator() = default;
    ~UnorderCoordinator() = default;

    // 禁用拷贝和赋值
    UnorderCoordinator(const UnorderCoordinator&) = delete;
    UnorderCoordinator& operator=(const UnorderCoordinator&) = delete;

    // 注册节点及依赖关系 (依赖的语义：当前节点 必须在 dependsOn 节点【之前】退出)
    void Register(const std::shared_ptr<INode>& node, const std::vector<std::string>& dependsOn = {}) {
        std::lock_guard<std::mutex> lock(mutex_);
        const std::string& name = node->Name();
        nodes_[name] = node;

        if (in_degree_.find(name) == in_degree_.end()) {
            in_degree_[name] = 0;
        }

        for (const auto& dep : dependsOn) {
            adj_list_[name].push_back(dep);
            in_degree_[dep]++;
        }
    }

    // 执行通用退出流程
    void ShutdownAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::queue<std::string> ready_queue;

        // 1. 寻找入度为 0 的首批可退出节点
        for (const auto& pair : nodes_) {
            if (in_degree_[pair.first] == 0) {
                ready_queue.push(pair.first);
            }
        }

        // 2. 逐层并发收割
        while (!ready_queue.empty()) {
            size_t current_layer_size = ready_queue.size();

            // 用于存储当前拓扑层的所有异步任务结果
            struct TaskContext {
                std::string name;
                std::shared_ptr<INode> node;
                std::shared_future<bool> future;
                std::thread worker;
            };
            std::vector<TaskContext> layer_tasks;

            // 派发当前层级 (Unorder 层) 的并发任务
            for (size_t i = 0; i < current_layer_size; ++i) {
                std::string name = ready_queue.front();
                ready_queue.pop();

                auto node = nodes_[name];
                auto promise = std::make_shared<std::promise<bool>>();
                const auto future = promise->get_future().share();

                std::thread t([node, promise]() {
                    try {
                        const bool result = node->Shutdown();
                        promise->set_value(result);
                    } catch (...) {
                        promise->set_value(false);
                    }
                });

                layer_tasks.push_back({name, node, future, std::move(t)});
            }

            // 同步等待当前层级任务，并处理超时 (死锁逃逸)
            for (auto& task : layer_tasks) {
                auto status = task.future.wait_for(task.node->Timeout());

                if (status == std::future_status::ready) {
                    // 正常结束，安全回收线程
                    if (task.worker.joinable()) task.worker.join();
                    bool success = task.future.get();
                    if (!success) {
                        std::cerr << "[Warning] Node shutdown returned false: " << task.name << "\n";
                    }
                } else {
                    // 超时发生，强行分离线程以避免死锁阻塞整个退出流程
                    std::cerr << "[FATAL] Node timeout, detaching: " << task.name << "\n";
                    if (task.worker.joinable()) task.worker.detach();
                }

                // 推进 DAG 依赖图，释放下游节点
                for (const auto& neighbor : adj_list_[task.name]) {
                    in_degree_[neighbor]--;
                    if (in_degree_[neighbor] == 0) {
                        ready_queue.push(neighbor);
                    }
                }
            }
        }
    }

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<INode>> nodes_;
    std::unordered_map<std::string, std::vector<std::string>> adj_list_;
    std::unordered_map<std::string, int> in_degree_;
};

} // namespace terminus