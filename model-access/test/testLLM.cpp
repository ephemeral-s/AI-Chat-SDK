#include <gtest/gtest.h>
#include <spdlog/common.h>
#include "../sdk/include/DeepSeekProvider.h"
#include "../sdk/include/util/myLog.h"

TEST(DeepSeekProviderTest, sendMessage)
{
    std::map<std::string, std::string> modelConfig = {
        {"api_key", std::getenv("DeepSeek_api_key")},
        {"endpoint", "https://api.deepseek.com"}
    };

    auto Provider = std::make_shared<ai_chat_sdk::DeepSeekProvider>();
    ASSERT_TRUE(Provider != nullptr);
    Provider->initModel(modelConfig);
    ASSERT_TRUE(Provider->isAvailable());
    
    std::vector<ai_chat_sdk::Message> messages = {
        {"user", "你是谁"}
    };

    std::map<std::string, std::string> requestParams = {
        {"temperature", "0.7"},
        {"max_tokens", "2048"}
    };

    //std::string response = Provider->sendMessage(messages, requestParams);

    auto writeChunk = [&](const std::string& chunk, bool last){
        INFO("writeChunk: {}", chunk);
        if(last){
            INFO("[DONE]");
        }
    };
    std::string response = Provider->sendMessageStream(messages, requestParams, writeChunk);
    ASSERT_TRUE(!response.empty());
    INFO("response: {}", response);
}

int main()
{
    testing::InitGoogleTest(); // 初始化 Google Test 框架
    ephemerals::Logger::initLogger("DeepSeekProviderTest", "stdout", spdlog::level::debug); // 初始化日志记录器
    return RUN_ALL_TESTS(); // 运行所有测试
}