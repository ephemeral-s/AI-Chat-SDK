#pragma once
#include "common.h"
#include <string>
#include <map>
#include <vector>
#include <functional>

namespace ai_chat_sdk{
    
    class LLMProvider{
    public:
        // 初始化模型
        virtual void initModel(const std::map<std::string, std::string>& modelConfig) = 0;
        // 是否可用
        virtual bool isAvailable() const = 0;
        // 获取模型名称
        virtual std::string getModelName() const = 0;
        // 获取模型描述 
        virtual std::string getModelDesc() const = 0;
        //发送消息 - 全量返回
        virtual std::string sendMessage(const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParams) = 0;
        // 发送消息 - 流式返回
        virtual std::string sendMessageStream(const std::vector<Message>& messages, 
                                            const std::map<std::string, std::string>& requestParams, 
                                            std::function<void(const std::string&, bool)> callback) = 0; // callback：回调函数处理返回的文本，参数：(返回的文本，是否是最后一个)
    private:
        bool _isAvailable = false; // 是否可用
        std::string _apiKey;       // API 密钥
        std::string _endpoint;     // API 调用地址
    };
}