// #include <gtest/gtest.h>
// #include <spdlog/common.h>
// #include "../sdk/include/OllamaLLMProvider.h"
// #include "../sdk/include/util/myLog.h"

// #if 0
// #include "../sdk/include/DeepSeekProvider.h"
// TEST(DeepSeekProviderTest, sendMessage)
// {
//     std::map<std::string, std::string> modelConfig = {
//         {"api_key", std::getenv("DeepSeek_api_key")},
//         {"endpoint", "https://api.deepseek.com"}
//     };

//     auto Provider = std::make_shared<ai_chat_sdk::DeepSeekProvider>();
//     ASSERT_TRUE(Provider != nullptr);
//     Provider->initModel(modelConfig);
//     ASSERT_TRUE(Provider->isAvailable());
    
//     std::vector<ai_chat_sdk::Message> messages = {
//         {"user", "你是谁"}
//     };

//     std::map<std::string, std::string> requestParams = {
//         {"temperature", "0.7"},
//         {"max_tokens", "2048"}
//     };

//     //std::string response = Provider->sendMessage(messages, requestParams);

//     auto writeChunk = [&](const std::string& chunk, bool last){
//         INFO("writeChunk: {}", chunk);
//         if(last){
//             INFO("[DONE]");
//         }
//     };
//     std::string response = Provider->sendMessageStream(messages, requestParams, writeChunk);
//     ASSERT_TRUE(!response.empty());
//     INFO("response: {}", response);
// }

// TEST2(ChatGPTProviderTest, sendMessage)
// {
//     std::map<std::string, std::string> modelConfig = {
//         {"api_key", std::getenv("ChatGPT_api_key")},
//         {"endpoint", "https://api.openai.com"}
//     };

//     auto Provider = std::make_shared<ai_chat_sdk::ChatGPTProvider>();
//     ASSERT_TRUE(Provider != nullptr);
//     Provider->initModel(modelConfig);
//     ASSERT_TRUE(Provider->isAvailable());
    
//     std::vector<ai_chat_sdk::Message> messages = {
//         {"user", "你是谁"}
//     };

//     std::map<std::string, std::string> requestParams = {
//         {"temperature", "0.7"},
//         {"max_output_tokens", "2048"}
//     };

//     std::string response = Provider->sendMessage(messages, requestParams);

//     ASSERT_TRUE(!response.empty());
//     INFO("response: {}", response);
// }

// TEST(DeepSeekProviderTest, sendMessage)
// {
//     std::map<std::string, std::string> modelConfig = {
//         {"api_key", std::getenv("ChatGPT_api_key")},
//         {"endpoint", "https://api.openai.com"}
//     };

//     auto Provider = std::make_shared<ai_chat_sdk::ChatGPTProvider>();
//     ASSERT_TRUE(Provider != nullptr);
//     Provider->initModel(modelConfig);
//     ASSERT_TRUE(Provider->isAvailable());
    
//     std::vector<ai_chat_sdk::Message> messages = {
//         {"user", "你是谁"}
//     };

//     std::map<std::string, std::string> requestParams = {
//         {"temperature", "0.7"},
//         {"max_tokens", "2048"}
//     };

//     //std::string response = Provider->sendMessage(messages, requestParams);

//     auto writeChunk = [&](const std::string& chunk, bool last){
//         INFO("writeChunk: {}", chunk);
//         if(last){
//             INFO("[DONE]");
//         }
//     };
//     std::string response = Provider->sendMessageStream(messages, requestParams, writeChunk);
//     ASSERT_TRUE(!response.empty());
//     INFO("response: {}", response);
// }
// #endif

// // TEST(OllamaLLMProviderTest, sendMessage)
// // {
// //     std::map<std::string, std::string> modelConfig = {
// //         {"model_name", "deepseek-r1:1.5b"},
// //         {"model_desc", "本地部署开源模型deepseek-r1 1.5b"},
// //         {"endpoint", "http://192.168.58.1:11434"}
// //     };

// //     auto Provider = std::make_shared<ai_chat_sdk::OllamaLLMProvider>();
// //     ASSERT_TRUE(Provider != nullptr);
// //     Provider->initModel(modelConfig);
// //     ASSERT_TRUE(Provider->isAvailable());
    
// //     std::vector<ai_chat_sdk::Message> messages = {
// //         {"user", "你是谁"}
// //     };

// //     std::map<std::string, std::string> requestParams = {
// //         {"temperature", "0.7"},
// //         {"max_tokens", "2048"}
// //     };

// //     std::string response = Provider->sendMessage(messages, requestParams);

// //     ASSERT_TRUE(!response.empty());
// //     INFO("response: {}", response);
// // }

// TEST(OllamaLLMProviderTest, sendMessage)
// {
//     std::map<std::string, std::string> modelConfig = {
//         {"model_name", "deepseek-r1-original:1.5b"},
//         {"model_desc", "本地部署开源模型deepseek-r1 1.5b"},
//         {"endpoint", "http://192.168.58.1:11434"}
//     };

//     auto Provider = std::make_shared<ai_chat_sdk::OllamaLLMProvider>();
//     ASSERT_TRUE(Provider != nullptr);
//     Provider->initModel(modelConfig);
//     ASSERT_TRUE(Provider->isAvailable());
    
//     std::vector<ai_chat_sdk::Message> messages = {
//         {"user", "介绍一下C++语言"}
//     };

//     std::map<std::string, std::string> requestParams = {
//         {"temperature", "0.7"},
//         {"max_tokens", "2048"}
//     };

//     //std::string response = Provider->sendMessage(messages, requestParams);

//     auto writeChunk = [&](const std::string& chunk, bool last){
//         INFO("writeChunk: {}", chunk);
//         if(last){
//             INFO("[DONE]");
//         }
//     };
//     std::string response = Provider->sendMessageStream(messages, requestParams, writeChunk);
//     ASSERT_TRUE(!response.empty());
//     INFO("response: {}", response);
// }

// int main()
// {
//     testing::InitGoogleTest(); // 初始化 Google Test 框架
//     ephemerals::Logger::initLogger("TestLLM", "stdout", spdlog::level::debug); // 初始化日志记录器
//     return RUN_ALL_TESTS(); // 运行所有测试
// }

//测试ChatSDK

#include <gtest/gtest.h>
#include <spdlog/common.h>
#include "../sdk/include/ChatSDK.h"
#include <cstdlib>
#include <gtest/gtest.h>
#include <iostream>
#include "../sdk/include/util/myLog.h"

TEST(ChatSDKTest, sendMessage){
    auto sdk = std::make_shared<ai_chat_sdk::ChatSDK>();
    ASSERT_TRUE(sdk != nullptr);

    auto deepseekConfig = std::make_shared<ai_chat_sdk::ApiConfig>();
    ASSERT_TRUE(deepseekConfig != nullptr);
    deepseekConfig->_modelName = "deepseek-chat";
    deepseekConfig->_apiKey = std::getenv("DeepSeek_api_key");
    ASSERT_FALSE(deepseekConfig->_apiKey.empty());
    deepseekConfig->_maxTokens = 2048;
    deepseekConfig->_temperature = 0.7;

    auto ChatGPTConfig = std::make_shared<ai_chat_sdk::ApiConfig>();
    ASSERT_TRUE(ChatGPTConfig != nullptr);
    ChatGPTConfig->_modelName = "gpt-5.4";
    ChatGPTConfig->_apiKey = std::getenv("ChatGPT_api_key");
    ASSERT_FALSE(ChatGPTConfig->_apiKey.empty());
    ChatGPTConfig->_maxTokens = 2048;
    ChatGPTConfig->_temperature = 0.7;

    auto GeminiConfig = std::make_shared<ai_chat_sdk::ApiConfig>();
    ASSERT_TRUE(GeminiConfig != nullptr);
    GeminiConfig->_modelName = "gemini-3-flash-preview";
    GeminiConfig->_apiKey = std::getenv("Gemini_api_key");
    ASSERT_FALSE(GeminiConfig->_apiKey.empty());
    GeminiConfig->_maxTokens = 2048;
    GeminiConfig->_temperature = 0.7;

    auto OllamaConfig = std::make_shared<ai_chat_sdk::OllamaConfig>();
    ASSERT_TRUE(OllamaConfig != nullptr);
    OllamaConfig->_modelName = "deepseek-r1:1.5b";
    OllamaConfig->_modelDesc = "本地部署开源模型deepseek-r1 1.5b";
    OllamaConfig->_endpoint = "http://192.168.58.1:11434";
    OllamaConfig->_maxTokens = 2048;
    OllamaConfig->_temperature = 0.7;

    std::vector<std::shared_ptr<ai_chat_sdk::Config>> modelConfigs = {
        deepseekConfig,
        ChatGPTConfig,
        GeminiConfig,
        OllamaConfig
    };

    sdk->initModels(modelConfigs);

    //创建会话
    std::string sessionId = sdk->createSession(GeminiConfig->_modelName);
    ASSERT_TRUE(!sessionId.empty());

    std::string message;
    std::cout << "请输入消息: ";
    std::getline(std::cin, message);

    auto response = sdk->sendMessage(sessionId, message);
    ASSERT_TRUE(!response.empty());
    
    std::cout << "模型回复: " << response << std::endl;

    std::cout << "请输入第二条消息: ";
    std::getline(std::cin, message);

    response = sdk->sendMessage(sessionId, message);
    ASSERT_TRUE(!response.empty());
    
    std::cout << "模型回复: " << response << std::endl;

    //用sessionManager获取会话历史消息
    std::vector<ai_chat_sdk::Message> messages = sdk->_sessionManager.getHistroyMessages(sessionId);
    ASSERT_TRUE(!messages.empty());
    for(const auto& message : messages){
        std::cout << message._role << ": " << message._content << std::endl;
    }
}

int main()
{
    testing::InitGoogleTest(); // 初始化 Google Test 框架
    ephemerals::Logger::initLogger("TestLLM", "stdout", spdlog::level::debug); // 初始化日志记录器
    return RUN_ALL_TESTS(); // 运行所有测试
}