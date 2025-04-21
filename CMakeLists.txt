cmake_minimum_required(VERSION 3.10)
project(net_device LANGUAGES C)

# 添加选项控制是否编译测试可执行文件
option(BUILD_TEST "Build test executable" ON)

# 检查 mempool 子模块是否存在
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rd/mempool/CMakeLists.txt")
    message(FATAL_ERROR
        "mempool submodule not found! Please run:\n"
        "  git submodule update --init --recursive\n"
        "or clone the repo with:\n"
        "  git clone --recurse-submodules <repo-url>"
    )
endif()

# 添加 mempool 子模块（显式指定路径）
add_subdirectory(3rd/mempool)

# 创建 net_device 库
add_library(net_device
    src/net_device.c
    src/net_log.c
)

# 设置头文件目录（现代 CMake 风格）
target_include_directories(net_device PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 链接依赖库（传递性依赖自动处理）
target_link_libraries(net_device PUBLIC
    mempool
)

# 如果启用测试，创建测试可执行文件
if(BUILD_TEST)
    add_executable(net_device_test
        test/test.c
    )
    
    target_include_directories(net_device_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    target_link_libraries(net_device_test
        PRIVATE
        net_device
    )
    
    message(STATUS "Test executable enabled: net_device_test")
endif()

# 安装规则（可选）
install(TARGETS net_device
    ARCHIVE  DESTINATION lib
    LIBRARY  DESTINATION lib
    RUNTIME  DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)