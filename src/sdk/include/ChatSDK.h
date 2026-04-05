#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include "common.h"
#include "SessionManager.h"
#include "LLMManager.h"

namespace ai_chat_sdk{
    class ChatSDK{
    public:
        //初始化模型
        bool initModels(const std::vector<std::shared_ptr<Config>>& configs);
        //创建会话
        std::string createSession(const std::string& modelName);
        //获取指定会话
        std::shared_ptr<SessionInfo> getSession(const std::string& sessionId);
        //获取会话列表
        std::vector<std::string> getSessionList();
        //删除指定会话
        bool deleteSession(const std::string& sessionId);
        //获取可用模型信息
        std::vector<ModelInfo> getModelAvailableInfo();

        //发送消息给指定模型 -- 全量返回
        std::string sendMessage(const std::string& sessionId, const std::string& message);
        //发送消息给指定模型 -- 流式返回
        std::string sendMessageStream(const std::string& sessionId, const std::string& message, std::function<void(const std::string&, bool)> callback);
    private:
        //注册所支持的模型
        bool registerAllProviders(const std::vector<std::shared_ptr<Config>>& configs);
        //初始化所支持的模型
        bool initAllModels(const std::vector<std::shared_ptr<Config>>& configs);
        //初始化API模型提供者
        bool initApiModelProviders(const std::string& modelName, const std::shared_ptr<ApiConfig>& apiConfig);
        //初始化Ollama模型提供者
        bool initOllamaModelProviders(const std::string& modelName, const std::shared_ptr<OllamaConfig>& ollamaConfig);
    private:
        bool _initialized; // 是否初始化
        LLMManager _llmManager;
        std::unordered_map<std::string, std::shared_ptr<Config>> _modelConfigs; // 模型配置映射 <模型名称, 配置信息>
    public:
        SessionManager _sessionManager;
    };
}
