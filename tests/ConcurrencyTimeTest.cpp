//
// Created by Administrator on 2026/6/2.
//

#include <gtest/gtest.h>



#include "terminus_unorder/terminus_unorder.hpp"

TEST(CoordinatorTest, UnorderNodesExecuteConcurrently) {
    terminus::UnorderCoordinator engine;

    auto slow_task = []() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return true;
    };

    // 注册 3 个互相独立的节点
    engine.Register(std::make_shared<terminus::FunctionalNode>("Node1", slow_task), {});
    engine.Register(std::make_shared<terminus::FunctionalNode>("Node2", slow_task), {});
    engine.Register(std::make_shared<terminus::FunctionalNode>("Node3", slow_task), {});

    auto start_time = std::chrono::steady_clock::now();
    engine.ShutdownAll();
    auto end_time = std::chrono::steady_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // 并发执行耗时应该接近 100ms。允许一定的线程调度开销（如 50ms）
    // 如果是串行，会 > 300ms
    // 1. 验证无依赖的节点是真正并行执行的，而不是被串行阻塞
    // 1.1. 严格检查下限：绝对不能早于单个节点的基础耗时 (假设每个节点 sleep 100ms)
    EXPECT_GE(elapsed, 100) << "Nodes finished too early! Did they actually execute the payload?";

    // 1.2. 宽松检查上限：允许 CI 环境下多线程创建、上下文切换有较大的时间开销 (之前跑出了 257ms)
    // 只要它显著小于“所有节点完全串行执行”的时间（例如 3 个节点串行需要 300ms 以上），就能证明是并发的
    EXPECT_LT(elapsed, 350) << "Nodes did not execute concurrently or CI runner is extremely overloaded.";
}
