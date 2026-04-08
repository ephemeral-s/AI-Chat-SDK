#include "ChatServer.h"
#include <memory>

namespace ai_chat_server{
    ChatServer::ChatServer(const ServerConfig& config){
        //chatSDK初始化
        auto deepseekConfig = std::make_shared<ai_chat_sdk::ApiConfig>();
        deepseekConfig->_modelName = "deepseek-chat";
        deepseekConfig->_apiKey = std::getenv("DeepSeek_api_key");
        deepseekConfig->_maxTokens = config.max_tokens;
        deepseekConfig->_temperature = config.temperature;

        auto ChatGPTConfig = std::make_shared<ai_chat_sdk::ApiConfig>();
        ChatGPTConfig->_modelName = "gpt-5.4";
        ChatGPTConfig->_apiKey = std::getenv("ChatGPT_api_key");
        ChatGPTConfig->_maxTokens = config.max_tokens;
        ChatGPTConfig->_temperature = config.temperature;

        auto GeminiConfig = std::make_shared<ai_chat_sdk::ApiConfig>();
        GeminiConfig->_modelName = "gemini-3-flash-preview";
        GeminiConfig->_apiKey = std::getenv("Gemini_api_key");
        GeminiConfig->_maxTokens = config.max_tokens;
        GeminiConfig->_temperature = config.temperature;

        auto OllamaConfig = std::make_shared<ai_chat_sdk::OllamaConfig>();
        OllamaConfig->_modelName = config.Ollama_Model_Name;
        OllamaConfig->_modelDesc = config.Ollama_Model_Desc;
        OllamaConfig->_endpoint = config.Ollama_Endpoint;
        OllamaConfig->_maxTokens = config.max_tokens;
        OllamaConfig->_temperature = config.temperature;

        std::vector<std::shared_ptr<ai_chat_sdk::Config>> modelConfigs = {
            deepseekConfig,
            ChatGPTConfig,
            GeminiConfig,
            OllamaConfig
        };

        if(!_chatSDK->initModels(modelConfigs)){
            ERR("initModels failed");
            return;
        }

        //创建服务器
        _server = std::unique_ptr<httplib::Server>(new httplib::Server());
        if(!_server){
            ERR("createServer failed");
            return;
        }
    }

    bool ChatServer::start(){
        if(_isRunning.load()){
            ERR("server is already running");
            return false;
        }
        
        //为了让服务器不卡主程，使用异步线程来运行服务器
        std::thread serverThread([this](){
            _server->listen(_config.host, _config.port);
            INFO("server is running on %s:%d", _config.host.c_str(), _config.port);
        });
        serverThread.detach();
        _isRunning.store(true);
        return true;
    }

    void ChatServer::stop(){
        if(!_isRunning.load()){
            ERR("server is not running");
            return;
        }
        
        if(_server){
            _server->stop();
        }
        _isRunning.store(false);
    }

    bool ChatServer::isRunning(){
        return _isRunning.load();
    }
}
