#include "../include/LLMManager.h"
#include "../include/util/myLog.h"
#include "../include/common.h"

namespace ai_chat_sdk{
    //注册模型提供者
    bool LLMManager::RegisterProvider(const std::string& modelName, std::unique_ptr<LLMProvider> provider){
        if(provider == nullptr){
            ERR("provider is null");
            return false;
        }
        _providers[modelName] = std::move(provider);
        _modelInfos[modelName] = ModelInfo(modelName);
        INFO("Register provider for model {}", modelName);
        return true;
    }

    //初始化指定模型
    bool LLMManager::InitModel(const std::string& modelName, const std::map<std::string, std::string>& modelConfig){
        //检查是否注册
        auto it = _providers.find(modelName);
        if(it == _providers.end()){
            ERR("Provider for model {} not registered", modelName);
        }

        //初始化
        bool isSuccess = it->second->initModel(modelConfig);
        if(!isSuccess){
            ERR("Init model {} failed", modelName);
        }
        else{
            INFO("Init model {} success", modelName);
            _modelInfos[modelName]._isAvailable = true;
            _modelInfos[modelName]._modelDesc = it->second->getModelDesc();
        }
        return isSuccess;
    }

    //获取可用模型
    std::vector<ModelInfo> LLMManager::GetAvailableModels() const{
        std::vector<ModelInfo> availableModels;
        for(const auto& pair : _modelInfos){
            if(pair.second._isAvailable){
                availableModels.push_back(pair.second);
            }
        }
        return availableModels;
    }

    //检测指定模型是否可用
    bool LLMManager::IsModelAvailable(const std::string& modelName) const{
        auto it = _modelInfos.find(modelName);
        return it != _modelInfos.end() && it->second._isAvailable;
    }

    //发送消息给指定模型 -- 全量返回
    std::string LLMManager::SendMessage(const std::string& modelName, const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParams){
        //检查模型是否注册
        auto it = _providers.find(modelName);
        if(it == _providers.end()){
            ERR("Provider for model {} not registered", modelName);
            return "";
        }
        
        //检查模型是否可用
        if(!IsModelAvailable(modelName)){
            ERR("Model {} not available", modelName);
            return "";
        }
        return _providers[modelName]->sendMessage(messages, requestParams);
    }

    //发送消息给指定模型 -- 流式返回
    std::string LLMManager::SendMessageStream(const std::string& modelName, const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParams, std::function<void(const std::string&, bool)> callback){
        //检查模型是否注册
        auto it = _providers.find(modelName);
        if(it == _providers.end()){
            ERR("Provider for model {} not registered", modelName);
            return "";
        }
        
        //检查模型是否可用
        if(!IsModelAvailable(modelName)){
            ERR("Model {} not available", modelName);
            return "";
        }
        return _providers[modelName]->sendMessageStream(messages, requestParams, callback);
    }
}