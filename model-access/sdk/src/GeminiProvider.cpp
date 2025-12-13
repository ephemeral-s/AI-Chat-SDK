#include "../include/GeminiProvider.h"
#include "../include/util/myLog.h"
#include <jsoncpp/json/json.h>
#include <httplib.h>
#include <jsoncpp/json/reader.h>

namespace ai_chat_sdk{
    // 初始化模型
    bool GeminiProvider::initModel(const std::map<std::string, std::string>& modelConfig){
        //初始化API密钥
        auto it = modelConfig.find("api_key");
        if(it == modelConfig.end()){
            ERR("GeminiProvider::initModel: api_key not found in modelConfig");
            return false;
        }else{
            _apiKey = it->second;
        }

        //初始化调用地址
        it = modelConfig.find("endpoint");
        if(it == modelConfig.end()){
            ERR("GeminiProvider::initModel: endpoint not found in modelConfig");
            return false;
        }else{
            _endpoint = it->second;
        }

        _isAvailable = true;
        INFO("GeminiProvider::initModel: init success");
        return true;
    }

    // 是否可用
    bool GeminiProvider::isAvailable() const{
        return _isAvailable;
    }

    // 获取模型名称
    std::string GeminiProvider::getModelName() const{
        return "gemini-2.0-flash";
    }

    // 获取模型描述
    std::string GeminiProvider::getModelDesc() const{
        return "Google开发的急速响应模型，支持文本、图像、视频等多模态输入，适用于实时对话场景。";
    }

    // 发送消息 - 全量返回
    std::string GeminiProvider::sendMessage(const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParams)
    {
        return "";
    }

    // 发送消息 - 流式返回
    std::string GeminiProvider::sendMessageStream(const std::vector<Message>& messages, 
                                                const std::map<std::string, std::string>& requestParams, 
                                                std::function<void(const std::string&, bool)> callback)
    {
        return "";
    }
}