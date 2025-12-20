#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "LLMProvider.h"
#include "common.h"

namespace ai_chat_sdk{
    class LLMManager{
    public:
        //注册模型提供者
        bool RegisterProvider(const std::string& modelName, std::unique_ptr<LLMProvider> provider);

        //初始化指定模型
        bool InitModel(const std::string& modelName, const std::map<std::string, std::string>& modelConfig);

        //获取可用模型
        std::vector<ModelInfo> GetAvailableModels() const;

        //检测指定模型是否可用
        bool IsModelAvailable(const std::string& modelName) const;
        
        //发送消息给指定模型 -- 全量返回
        std::string SendMessage(const std::string& modelName, const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParams);
        
        //发送消息给指定模型 -- 流式返回
        std::string SendMessageStream(const std::string& modelName, const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParams, std::function<void(const std::string&, bool)> callback);
    private:
        std::map<std::string, std::unique_ptr<LLMProvider>> _providers; // <模型名称, 模型提供者>
        std::map<std::string, ModelInfo> _modelInfos; // <模型名称, 模型信息>
    };
}