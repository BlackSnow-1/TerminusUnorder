//
// Created by Administrator on 2026/6/2.
//

#include <gtest/gtest.h>
#include "terminus_unorder/terminus_unorder.hpp"
#include <mutex>


TEST(CoordinatorTest, DependencyOrderIsStrict) {
    terminus::UnorderCoordinator engine;

    // 用于记录实际退出顺序
    std::vector<std::string> exit_sequence;
    std::mutex seq_mutex;

    // 辅助闭包：模拟节点退出并记录
    auto make_node = [&](const std::string &name) {
        return std::make_shared<terminus::FunctionalNode>(name, [&, name]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::lock_guard<std::mutex> lock(seq_mutex);
            exit_sequence.push_back(name);
            return true;
        });
    };

    // 构造拓扑：Logic 依赖 DB 和 Cache。Gateway 依赖 Logic。
    // 预期退出顺序：Gateway -> Logic -> (DB/Cache 无序)
    engine.Register(make_node("DB"), {});
    engine.Register(make_node("Cache"), {});
    engine.Register(make_node("Logic"), {"DB", "Cache"});
    engine.Register(make_node("Gateway"), {"Logic"});

    engine.ShutdownAll();

    // 断言验证顺序
    ASSERT_EQ(exit_sequence.size(), 4);
    EXPECT_EQ(exit_sequence[0], "Gateway");
    EXPECT_EQ(exit_sequence[1], "Logic");
    // DB 和 Cache 都在 Logic 之后，但它们俩互为 Unorder
    EXPECT_TRUE(exit_sequence[2] == "DB" || exit_sequence[2] == "Cache");
    EXPECT_TRUE(exit_sequence[3] == "DB" || exit_sequence[3] == "Cache");
}
