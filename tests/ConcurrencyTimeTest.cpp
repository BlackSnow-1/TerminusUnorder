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
    EXPECT_LT(elapsed, 200) << "Nodes did not execute concurrently!";
    EXPECT_GE(elapsed, 100);
}
