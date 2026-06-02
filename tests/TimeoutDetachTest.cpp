//
// Created by Administrator on 2026/6/2.
//

#include <gtest/gtest.h>
#include "terminus_unorder/terminus_unorder.hpp"

TEST(CoordinatorTest, DeadlockRecoveryByTimeout) {
    terminus::UnorderCoordinator engine;

    bool downstream_executed = false;

    // 恶意死锁节点，设置超时时间为 200 毫秒
    auto buggy_node = std::make_shared<terminus::FunctionalNode>(
        "BuggyNode",
        []() {
            // 故意挂起很久，模拟死锁
            std::this_thread::sleep_for(std::chrono::seconds(10));
            return true;
        },
        std::chrono::milliseconds(200) // Timeout = 200ms
    );

    // 下游节点，依赖 BuggyNode
    auto safe_node = std::make_shared<terminus::FunctionalNode>("SafeNode", [&]() {
        downstream_executed = true;
        return true;
    });

    engine.Register(buggy_node, {});
    engine.Register(safe_node, {"BuggyNode"});

    auto start = std::chrono::steady_clock::now();
    engine.ShutdownAll();
    auto end = std::chrono::steady_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // 1. 验证框架在约 200ms 超时后立刻返回了，没有被 10 秒的死锁拖住
    EXPECT_NEAR(elapsed, 200, 50); // 允许 50ms 的误差

    // 2. 验证尽管 BuggyNode 超时了，它所阻挡的下游节点依然得到了执行的机会
    EXPECT_TRUE(downstream_executed) << "Downstream node was skipped!";
}