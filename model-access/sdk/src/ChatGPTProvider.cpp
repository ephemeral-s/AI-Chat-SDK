#include "../include/ChatGPTProvider.h"
#include "../include/util/myLog.h"
#include <jsoncpp/json/json.h>
#include <httplib.h>
#include <jsoncpp/json/reader.h>

namespace ai_chat_sdk{

    // 初始化模型
    bool ChatGPTProvider::initModel(const std::map<std::string, std::string>& modelConfig){
        // 初始化 API 密钥
        auto it = modelConfig.find("api_key");
        if(it == modelConfig.end()){
            ERR("ChatGPTProvider::initModel: api_key not found in modelConfig");
            return false;
        }else{
            _apiKey = it->second;
        }

        // 初始化调用地址
        it = modelConfig.find("endpoint");
        if(it == modelConfig.end()){
            ERR("ChatGPTProvider::initModel: endpoint not found in modelConfig");
            return false;
        }else{
            _endpoint = it->second;
        }
        
        _isAvailable = true;
        INFO("ChatGPTProvider::initModel: init success");
        return true;
    }

    // 是否可用
    bool ChatGPTProvider::isAvailable() const{
        return _isAvailable;
    }

    // 获取模型名称
    std::string ChatGPTProvider::getModelName() const{
        return "gpt-4o-mini";
    }

    //获取模型描述
    std::string ChatGPTProvider::getModelDesc() const{
        return "gpt-4o-mini 是一个基于 GPT-4 架构的模型，由 OpenAI 开发。它是一个轻量级模型，适用于多种自然语言处理任务，如文本生成、问答、翻译等。";
    }

    // 发送消息 - 全量返回
    std::string ChatGPTProvider::sendMessage(const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParams)
    {
        return "";
    }

    // 发送消息 - 流式返回
    std::string ChatGPTProvider::sendMessageStream(const std::vector<Message>& messages, 
                                                const std::map<std::string, std::string>& requestParams, 
                                                std::function<void(const std::string&, bool)> callback)
    {
        return "";
    }
}
