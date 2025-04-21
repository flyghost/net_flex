cmake_minimum_required(VERSION 3.10)
project(net_device LANGUAGES C)

# 添加选项控制是否编译测试可执行文件
option(BUILD_TEST "Build test executable" ON)

# 添加mempool子模块
add_subdirectory(3rd/mempool)

# 创建net_device库
add_library(net_device
    src/net_device.c
    src/net_log.c
)

# 设置头文件目录
target_include_directories(net_device
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/mempool/include
)

# 链接依赖库
target_link_libraries(net_device
    PUBLIC 
    mempool
)

# 如果启用测试，创建测试可执行文件
if(BUILD_TEST)
    add_executable(net_device_test
        test/test.c  # 假设你的测试文件在这里
    )
    
    target_include_directories(net_device_test
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    target_link_libraries(net_device_test
        PRIVATE
        net_device
    )
    
    message(STATUS "Test executable will be built: net_device_test")
endif()