# 设置CMake的最小版本
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(ChatServer)

# 设置目标名称
set(CHATSERVER_NAME ChatServer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 收集源文件并生成可执行文件
add_executable(${CHATSERVER_NAME}
    main.cpp
    ChatServer.cpp
)

# 查找OpenSSL和gflags库
find_package(OpenSSL REQUIRED)
find_package(gflags REQUIRED)

# 添加CPPHTTPLIB_OPENSSL_SUPPORT 定义
target_compile_definitions(${CHATSERVER_NAME} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# 设置链接库目录
link_directories(/usr/local/lib)

# 链接库
target_link_libraries(${CHATSERVER_NAME} PRIVATE
    jsoncpp
    spdlog
    fmt
    OpenSSL::SSL
    OpenSSL::Crypto
    gflags
    ai_chat_sdk
    sqlite3
    cpp-httplib
)

# 设置可执行文件输出目录为构建目录
set_target_properties(${CHATSERVER_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
