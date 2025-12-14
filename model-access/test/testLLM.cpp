#include <gtest/gtest.h>
#include <spdlog/common.h>
#include "../sdk/include/GeminiProvider.h"
#include "../sdk/include/util/myLog.h"

#if 0
#include "../sdk/include/DeepSeekProvider.h"
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

TEST2(ChatGPTProviderTest, sendMessage)
{
    std::map<std::string, std::string> modelConfig = {
        {"api_key", std::getenv("ChatGPT_api_key")},
        {"endpoint", "https://api.openai.com"}
    };

    auto Provider = std::make_shared<ai_chat_sdk::ChatGPTProvider>();
    ASSERT_TRUE(Provider != nullptr);
    Provider->initModel(modelConfig);
    ASSERT_TRUE(Provider->isAvailable());
    
    std::vector<ai_chat_sdk::Message> messages = {
        {"user", "你是谁"}
    };

    std::map<std::string, std::string> requestParams = {
        {"temperature", "0.7"},
        {"max_output_tokens", "2048"}
    };

    std::string response = Provider->sendMessage(messages, requestParams);

    ASSERT_TRUE(!response.empty());
    INFO("response: {}", response);
}

TEST(DeepSeekProviderTest, sendMessage)
{
    std::map<std::string, std::string> modelConfig = {
        {"api_key", std::getenv("ChatGPT_api_key")},
        {"endpoint", "https://api.openai.com"}
    };

    auto Provider = std::make_shared<ai_chat_sdk::ChatGPTProvider>();
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
#endif

TEST(GeminiProviderTest, sendMessage)
{
    std::map<std::string, std::string> modelConfig = {
        {"api_key", std::getenv("Gemini_api_key")},
        {"endpoint", "https://generativelanguage.googleapis.com"}
    };

    auto Provider = std::make_shared<ai_chat_sdk::GeminiProvider>();
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

    std::string response = Provider->sendMessage(messages, requestParams);

    ASSERT_TRUE(!response.empty());
    INFO("response: {}", response);
}

int main()
{
    testing::InitGoogleTest(); // 初始化 Google Test 框架
    ephemerals::Logger::initLogger("TestLLM", "stdout", spdlog::level::debug); // 初始化日志记录器
    return RUN_ALL_TESTS(); // 运行所有测试
}