cmake_minimum_required(VERSION 3.16)
project(Bookstore_2025)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# 添加可执行文件，命名为code
add_executable(code
        src/main.cpp
        src/book.cpp
        src/user.cpp
        src/log.cpp
)

# 包含头文件目录
target_include_directories(code PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 设置可执行文件的输出名称为code（确保在根目录生成）
set_target_properties(code PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_NAME "code"
)

# 在Linux/macOS上，确保code文件有可执行权限
if(UNIX)
    add_custom_command(TARGET code POST_BUILD
            COMMAND chmod +x ${CMAKE_SOURCE_DIR}/code
            COMMENT "Setting executable permissions for code"
    )
endif()

# 可选：添加调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(code PRIVATE -g -O0)
else()
    target_compile_options(code PRIVATE -O2)
endif()

# 可选：显示更多编译信息
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Executable will be created at: ${CMAKE_SOURCE_DIR}/code")