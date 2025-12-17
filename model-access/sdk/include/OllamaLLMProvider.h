#pragma once
#include "LLMProvider.h"
#include "common.h"
#include <string>
#include <map>
#include <vector>
#include <functional>

namespace ai_chat_sdk{
    
    class OllamaLLMProvider : public LLMProvider{
    public:
        // 初始化模型
        virtual bool initModel(const std::map<std::string, std::string>& modelConfig) override;
        // 是否可用
        virtual bool isAvailable() const override;
        // 获取模型名称
        virtual std::string getModelName() const override;
        // 获取模型描述
        virtual std::string getModelDesc() const override;
        // 发送消息 - 全量返回
        virtual std::string sendMessage(const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParams) override;
        // 发送消息 - 流式返回
        virtual std::string sendMessageStream(const std::vector<Message>& messages, 
                                            const std::map<std::string, std::string>& requestParams, 
                                            std::function<void(const std::string&, bool)> callback) override; // callback：回调函数处理返回的文本，参数：(返回的文本，是否是最后一个)
    private:
        std::string _modelName; // 模型名称
        std::string _modelDesc; // 模型描述
    };
}