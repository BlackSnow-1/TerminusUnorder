#include "../include/terminus_unorder/terminus_unorder.hpp"
#include <iostream>
#include <chrono>

using namespace terminus;

// 1. 常规继承方式定义节点
class DatabasePoolNode : public INode {
public:
    [[nodiscard]] std::string Name() const override { return "Database_Pool"; }
    bool Shutdown() override {
        std::cout << "[Shutdown] Flushing transactions and closing DB connections...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return true;
    }
};

class ApiGatewayNode : public INode {
public:
    [[nodiscard]] std::string Name() const override { return "API_Gateway"; }
    bool Shutdown() override {
        std::cout << "[Shutdown] Rejecting new requests and draining existing ones...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return true;
    }
};

// 一个模拟死锁的故障节点
class BuggyBusinessLogic : public INode {
public:
    [[nodiscard]] std::string Name() const override { return "Business_Logic"; }
    bool Shutdown() override {
        std::cout << "[Shutdown] Business Logic hanging forever (Simulating deadlock)...\n";
        // 模拟一个死循环或死锁，触发框架的超时逃逸机制
        std::this_thread::sleep_for(std::chrono::hours(1));
        return true;
    }
    [[nodiscard]] std::chrono::milliseconds Timeout() const override {
        return std::chrono::seconds(2); // 设为2秒超时
    }
};




int main() {
    std::cout << "--- Starting Application ---\n\n";

    // 不再使用单例，而是显式实例化协调器，生命周期更加可控
    UnorderCoordinator engine;

    auto dbNode = std::make_shared<DatabasePoolNode>();
    auto gatewayNode = std::make_shared<ApiGatewayNode>();
    auto logicNode = std::make_shared<BuggyBusinessLogic>();

    // 使用 FunctionalNode (Lambda) 快速注册一个无需创建类的节点
    auto loggerNode = std::make_shared<FunctionalNode>(
        "Async_Logger",
        []() {
            std::cout << "[Shutdown] Flushing async log buffer to disk...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return true;
        }
    );

    // ==========================================
    // 依赖拓扑构建 (DAG)
    // ==========================================
    // 基础组件：日志和数据库不依赖任何人。
    engine.Register(loggerNode, {});
    engine.Register(dbNode, {});

    // 业务逻辑层：必须在数据库之前关闭，否则业务可能还会向数据库写入数据。
    // 同时，业务逻辑层也依赖日志。
    engine.Register(logicNode, {"Database_Pool", "Async_Logger"});

    // 网关层：处于最外层。必须先于业务逻辑层关闭。
    engine.Register(gatewayNode, {"Business_Logic"});

    std::cout << "--- Triggering TerminusUnorder Shutdown ---\n\n";

    // 执行顺序预期：
    // 1. API_Gateway 退出。
    // 2. Business_Logic 退出 (将会卡住，2秒后框架强制 detach 剥离)。
    // 3. Database_Pool 和 Async_Logger 因为不再有依赖阻挡，在同一层级(Unorder) 并发退出。
    engine.ShutdownAll();

    std::cout << "\n--- Application Safely Exited ---\n";
    return 0;
}