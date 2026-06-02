cmake_minimum_required(VERSION 4.2)
project(TerminusUnorder VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件目录 (Header-only)
add_library(terminus_unorder INTERFACE)
target_include_directories(terminus_unorder INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 启用多线程支持 (底层系统必备)
find_package(Threads REQUIRED)
target_link_libraries(terminus_unorder INTERFACE Threads::Threads)

# 2. 提供构建选项 (默认关闭，作为主项目构建时才开启)
option(TERMINUS_BUILD_EXAMPLES "Build example applications" OFF)
option(TERMINUS_BUILD_TESTS "Build unit tests" OFF)

# 如果当前项目是顶层项目，自动开启测试和示例
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(TERMINUS_BUILD_EXAMPLES ON)
    set(TERMINUS_BUILD_TESTS ON)
endif()

# 3. 按需挂载子目录
if(TERMINUS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(TERMINUS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()


# --- 顶层 CMakeLists.txt 追加内容 ---

# 引入 GNU 标准安装目录变量 (如 CMAKE_INSTALL_INCLUDEDIR)
include(GNUInstallDirs)

# 告诉 CMake，当用户执行 install 时，把 include 目录下的所有东西拷贝到系统的 include 目录去
install(DIRECTORY include/terminus_unorder
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)