#include "../include/OllamaLLMProvider.h"
#include "../include/util/myLog.h"
#include <jsoncpp/json/json.h>
#include <httplib.h>
#include <jsoncpp/json/reader.h>

namespace ai_chat_sdk{
    // 初始化模型
    bool OllamaLLMProvider::initModel(const std::map<std::string, std::string>& modelConfig){
        //初始化模型名称
        if(modelConfig.find("model_name") == modelConfig.end()){
            ERR("OllamaLLMProvider::initModel: model_name not found in modelConfig");
            return false;
        }
        _modelName = modelConfig.at("model_name");

        //初始化模型描述
        if(modelConfig.find("model_desc") == modelConfig.end()){
            ERR("OllamaLLMProvider::initModel: model_desc not found in modelConfig");
            return false;
        }
        _modelDesc = modelConfig.at("model_desc");

        //初始化调用地址
        if(modelConfig.find("endpoint") == modelConfig.end()){
            ERR("OllamaLLMProvider::initModel: endpoint not found in modelConfig");
            return false;
        }
        _endpoint = modelConfig.at("endpoint");

        _isAvailable = true;
        return true;
    }

    // 是否可用
    bool OllamaLLMProvider::isAvailable() const{
        return _isAvailable;
    }

    // 获取模型名称
    std::string OllamaLLMProvider::getModelName() const{
        return _modelName;
    }

    // 获取模型描述
    std::string OllamaLLMProvider::getModelDesc() const{
        return _modelDesc;
    }

    // 发送消息 - 全量返回
    std::string OllamaLLMProvider::sendMessage(const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParams){
        return "";
    }

    // 发送消息 - 流式返回
    std::string OllamaLLMProvider::sendMessageStream(const std::vector<Message>& messages, 
                                                const std::map<std::string, std::string>& requestParams, 
                                                std::function<void(const std::string&, bool)> callback){
        return "";
    }
}
