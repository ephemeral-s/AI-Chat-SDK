# 设置CMake的最小版本
cmake_minimum_required(VERSION 3.10)

# 设置SDK名称
set(SDK_NAME ai_chat_sdk)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型 release
set(CMAKE_BUILD_TYPE Release)

# 收集原文件
file(GLOB_RECURSE SDK_SOURCES "src/*.cpp")
file(GLOB_RECURSE SDK_HEADERS "include/*.h")

# 生成静态库
add_library(${SDK_NAME} STATIC ${SDK_SOURCES} ${SDK_HEADERS})

# 设置输出目录
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build)

# 查找OpenSSL库
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# 添加CPPPHTTPLIB_OPENSSL_SUPPORT 定义
target_compile_definitions(${SDK_NAME} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# 添加头文件搜索路径
target_include_directories(${SDK_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 设置链接库目录
link_directories(/usr/local/lib)

# 链接库
target_link_libraries(${SDK_NAME} jsoncpp spdlog sqlite3 fmt OpenSSL::SSL OpenSSL::Crypto)

# 定义安装规则（拷贝静态库到系统目录/usr/local/lib下）
install(TARGETS ${SDK_NAME} ARCHIVE DESTINATION lib)

# 定义安装规则（拷贝头文件到系统目录/usr/local/include/ai_chat_sdk下）
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include/ai_chat_sdk)