cmake_minimum_required(VERSION 3.10)
project(ChatServer)

set(CHATSERVER_NAME ChatServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${CHATSERVER_NAME}
    main.cpp
    ChatServer.cpp
)

find_package(OpenSSL REQUIRED)
find_package(gflags REQUIRED)
target_compile_definitions(${CHATSERVER_NAME} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

link_directories(/usr/local/lib)

target_link_libraries(${CHATSERVER_NAME} PRIVATE
    jsoncpp
    spdlog
    fmt
    OpenSSL::SSL
    OpenSSL::Crypto
    gflags
    ai_chat_sdk
    sqlite3
)

set_target_properties(${CHATSERVER_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
