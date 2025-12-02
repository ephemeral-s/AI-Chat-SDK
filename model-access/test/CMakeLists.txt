# 设置CMake的最小版本
cmake_minimum_required(VERSION 3.10)

# 项目名称
project(DeepSeekProviderTest)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型 debug
set(CMAKE_BUILD_TYPE Debug)

# 添加测试可执行文件
add_executable(testLLM testLLM.cpp ../sdk/src/util/myLog.cpp ../sdk/src/DeepSeekProvider.cpp)

# 设置输出目录
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# 添加头文件搜索路径
include_directories(${CMAKE_PROJECT_INCLUDE_DIR}/../sdk/include)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# 添加CPPPHTTPLIB_OPENSSL_SUPPORT 定义
target_compile_definitions(testLLM PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# 链接库
target_link_libraries(testLLM jsoncpp fmt spdlog gtest OpenSSL::SSL OpenSSL::Crypto)