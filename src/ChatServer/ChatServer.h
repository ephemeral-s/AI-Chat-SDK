#pragma once

#include <httplib.h>
#include <ai_chat_sdk/ChatSDK.h>
#include <ai_chat_sdk/util/myLog.h>
#include <memory>
#include <string>
#include <iostream>
#include <vector>

namespace ai_chat_server{
    //服务器配置信息
    struct ServerConfig{
        std::string host = "0.0.0.0";
        int port = 8080;
        std::string loglevel = "INFO";

        //模型需要的配置信息
        double temperature = 0.7;
        int max_tokens = 2048;

        //API_key
        std::string DeepSeek_API_Key;
        std::string ChatGPT_API_Key;
        std::string Gemini_API_Key;

        //Ollama
        std::string Ollama_Model_Name;
        std::string Ollama_Model_Desc;
        std::string Ollama_Endpoint;
    };

    class ChatServer{
    public:
        ChatServer(const ServerConfig& config);
        ~ChatServer();

        bool start();
        void stop();
        bool isRunning();
    private:
        //构建响应
        std::string buildResponse(const std::string& message, bool success = false);
        //处理创建会话请求
        void handleCreateSession(const httplib::Request& request, httplib::Response& response);
        //处理获取会话列表
        void handleGetSessionList(const httplib::Request& request, httplib::Response& response);
        //处理获取模型列表
        void handleGetModelList(const httplib::Request& request, httplib::Response& response);
        //处理删除会话请求
        void handleDeleteSession(const httplib::Request& request, httplib::Response& response);
        //处理获取历史消息请求
        void handleGetHistory(const httplib::Request& request, httplib::Response& response);
        //处理发送消息请求 - 全量返回
        void handleSendMessage(const httplib::Request& request, httplib::Response& response);
        //处理发送消息请求 - 流式返回
        void handleSendMessageStream(const httplib::Request& request, httplib::Response& response);
    private:
        std::unique_ptr<httplib::Server> _server = nullptr; // 服务器
        std::unique_ptr<ai_chat_sdk::ChatSDK> _chatSDK = nullptr; // 聊天SDK
        std::atomic<bool> _isRunning = {false}; // 是否运行中
        ServerConfig _config; // 服务器配置信息 
    };
}