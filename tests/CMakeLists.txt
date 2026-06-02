# ==========================================
# 1. 使用本地的 GoogleTest 框架
# ==========================================
find_package(GTest REQUIRED)

# ==========================================
# 2. 自动扫描并编译测试源码 (替换了原来的写法)
# ==========================================
# 扫描当前目录下所有的 .cpp 文件
file(GLOB TEST_SOURCES "*.cpp")

# 将扫描到的所有源文件编译成一个测试程序
add_executable(terminus_test ${TEST_SOURCES})

# ==========================================
# 3. 链接与编译选项
# ==========================================
# 链接我们的主库和 gtest
target_link_libraries(terminus_test PRIVATE
        terminus_unorder
        GTest::gtest_main
)

## 【新增】：配置测试模块专属的头文件搜索路径
## 假设你的 include 目录在项目根目录下，使用 ${PROJECT_SOURCE_DIR}/include 定位
#target_include_directories(terminus_test PRIVATE
#        ${PROJECT_SOURCE_DIR}/include
#)

# 开启 TSan 数据竞争检测 (推荐在 Linux/macOS 下开启)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(terminus_test PRIVATE -fsanitize=thread -g)
    target_link_options(terminus_test PRIVATE -fsanitize=thread)
endif()

# ==========================================
# 4. 注册测试供 CTest 发现
# ==========================================
include(GoogleTest)
gtest_discover_tests(terminus_test)