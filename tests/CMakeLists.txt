# ==========================================
# 0. 修复 Windows MSVC 运行时库冲突
# ==========================================
# 强制 GoogleTest 使用动态 C++ 运行时库 (/MD)，与 CMake 默认行为保持一致
if(MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

# ==========================================
# 1. 智能依赖查找 (Local Fallback to Fetch)
# ==========================================
# 第一步：尝试在本地静默查找 GTest (不加 REQUIRED，找不到不报错)
find_package(GTest QUIET)

if(GTest_FOUND)
    message(STATUS "[TerminusUnorder] Found local GoogleTest. Using GTest::gtest_main.")
    # 如果找到了，记录本地目标名称
    set(MY_GTEST_TARGET GTest::gtest_main)
else()
    message(STATUS "[TerminusUnorder] Local GoogleTest not found. Fetching from GitHub...")
    # 第二步：如果本地没有，则通过网络拉取
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    # 记录 FetchContent 生成的目标名称
    set(MY_GTEST_TARGET gtest_main)
endif()

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
        ${MY_GTEST_TARGET}
)

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