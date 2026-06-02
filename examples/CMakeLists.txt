# examples/CMakeLists.txt

# 1. 扫描当前目录下所有的 .cpp 文件
file(GLOB EXAMPLE_SOURCES "*.cpp")

# 2. 遍历每个文件并生成对应的目标
foreach(SOURCE_FILE IN LISTS EXAMPLE_SOURCES)
    # 提取纯文件名（不带 .cpp 后缀），例如将 "gateway_example.cpp" 提取为 "gateway_example"
    get_filename_component(TARGET_NAME ${SOURCE_FILE} NAME_WE)

    # 将其添加为可执行文件
    add_executable(${TARGET_NAME} ${SOURCE_FILE})

    # 链接核心框架
    target_link_libraries(${TARGET_NAME} PRIVATE terminus_unorder)

    # (可选) 将所有编译出的 examples 统一放到 build/bin 目录下，方便集中运行
    set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endforeach()