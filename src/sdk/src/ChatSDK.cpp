#include "../include/ChatSDK.h"
#include "../include/DeepSeekProvider.h"
#include "../include/ChatGPTProvider.h"
#include "../include/GeminiProvider.h"
#include "../include/OllamaLLMProvider.h"
#include "../include/util/myLog.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>


namespace ai_chat_sdk{
    //注册所支持的模型
    bool ChatSDK::registerAllProviders(const std::vector<std::shared_ptr<Config>>& configs){
        //注册云端模型
        if(!_llmManager.IsModelAvailable("deepseek-chat")){
            auto deepseekProvider = std::make_unique<DeepSeekProvider>();
            _llmManager.RegisterProvider("deepseek-chat", std::move(deepseekProvider)); /* 这里使用move的原因：unique_ptr不能被复制，只能被移动 */
            INFO("Registered DeepSeekProvider Successfully");
        }
        if(!_llmManager.IsModelAvailable("gpt-5.4")){
            auto chatgptProvider = std::make_unique<ChatGPTProvider>();
            _llmManager.RegisterProvider("gpt-5.4", std::move(chatgptProvider)); /* 这里使用move的原因：unique_ptr不能被复制，只能被移动 */
            INFO("Registered ChatGPTProvider Successfully");
        }
        if(!_llmManager.IsModelAvailable("gemini-3-flash-preview")){
            auto geminiProvider = std::make_unique<GeminiProvider>();
            _llmManager.RegisterProvider("gemini-3-flash-preview", std::move(geminiProvider)); /* 这里使用move的原因：unique_ptr不能被复制，只能被移动 */
            INFO("Registered GeminiProvider Successfully");
        }

        // 注册ollama模型（要确保config是OllamaConfig类型的对象）
        std::unordered_set<std::string> modelNames;
        for(const auto& config : configs){
            auto ollamaConfig = std::dynamic_pointer_cast<OllamaConfig>(config); /* 安全转换，返回空指针表示不是OllamaConfig类型 */
            if(ollamaConfig){
                auto modelName = ollamaConfig->_modelName;
                if(modelNames.find(modelName) == modelNames.end()){
                    modelNames.insert(modelName);

                    if(!_llmManager.IsModelAvailable(modelName)){
                        _llmManager.RegisterProvider(modelName, std::make_unique<OllamaLLMProvider>());
                        INFO("Registered OllamaProvider Successfully: {}", modelName);
                    }
                }
            }
        }
        return true;
    }

    //初始化所支持的模型
    bool ChatSDK::initAllModels(const std::vector<std::shared_ptr<Config>>& configs){
        for(const auto& config : configs){
            auto apiConfig = std::dynamic_pointer_cast<ApiConfig>(config);
            if(apiConfig){ // 云端模型初始化
                if(apiConfig->_modelName == "deepseek-chat" || 
                    apiConfig->_modelName == "gpt-5.4" || 
                    apiConfig->_modelName == "gemini-3-flash-preview"){
                    initApiModelProviders(apiConfig->_modelName, apiConfig);
                }else{
                    ERR("Model {} is not supported by this SDK", apiConfig->_modelName);
                }
            }
            else if(auto ollamaConfig = std::dynamic_pointer_cast<OllamaConfig>(config)){ // ollama模型初始化
                initOllamaModelProviders(ollamaConfig->_modelName, ollamaConfig);
            }else{
                ERR("Unsupported config type for model initialization");
            }
        }
        return true;
    }

    //初始化API模型提供者
    bool ChatSDK::initApiModelProviders(const std::string& modelName, const std::shared_ptr<ApiConfig>& apiConfig){
        //参数检查
        if(modelName.empty()){
            ERR("Model name is empty");
            return false;
        }
        if(apiConfig->_apiKey.empty()){
            ERR("API key is empty for model {}", modelName);
            return false;
        }

        //检查模型是否已初始化
        if(_llmManager.IsModelAvailable(modelName)){
            INFO("Model {} is already initialized", modelName);
            return true;
        }

        //初始化模型
        std::map<std::string, std::string> modelParams;
        modelParams["api_key"] = apiConfig->_apiKey;
        if(!_llmManager.InitModel(modelName, modelParams)){
            ERR("Failed to initialize model {} with API key", modelName);
            return false;
        }

        //保存模型配置信息
        _modelConfigs[modelName] = apiConfig;
        INFO("Initialized model {} successfully", modelName);
        return true;
    }

    //初始化Ollama模型提供者
    bool ChatSDK::initOllamaModelProviders(const std::string& modelName, const std::shared_ptr<OllamaConfig>& ollamaConfig){
        //参数检查
        if(modelName.empty()){
            ERR("Model name is empty");
            return false;
        }
        if(ollamaConfig->_endpoint.empty()){
            ERR("Endpoint is empty for model {}", modelName);
            return false;
        }

        //检查模型是否已初始化
        if(_llmManager.IsModelAvailable(modelName)){
            INFO("Model {} is already initialized", modelName);
            return true;
        }

        //初始化模型
        std::map<std::string, std::string> modelParams;
        modelParams["model_name"] = modelName;
        modelParams["model_desc"] = ollamaConfig->_modelDesc;
        modelParams["endpoint"] = ollamaConfig->_endpoint;
        if(!_llmManager.InitModel(modelName, modelParams)){
            ERR("Failed to initialize model {} with endpoint", modelName);
            return false;
        }

        //保存模型配置信息
        _modelConfigs[modelName] = ollamaConfig;
        INFO("Initialized Ollama model {} successfully", modelName);
        return true;
    }

    //初始化模型
    bool ChatSDK::initModels(const std::vector<std::shared_ptr<Config>>& configs){
        registerAllProviders(configs);
        initAllModels(configs);
        _initialized = true;
        return true;
    }

    //创建会话
    std::string ChatSDK::createSession(const std::string& modelName){
        if(!_initialized){
            ERR("SDK is not initialized");
            return "";
        }

        //通过SessionManager创建会话
        auto sessionId = _sessionManager.createSession(modelName);
        if(sessionId.empty()){
            ERR("Failed to create session for model {}", modelName);
            return "";
        }
        INFO("Created session {} for model {}", sessionId, modelName);
        return sessionId;
    }

    //获取指定会话
    std::shared_ptr<SessionInfo> ChatSDK::getSession(const std::string& sessionId){
        if(!_initialized){
            ERR("SDK is not initialized");
            return nullptr;
        }

        //通过SessionManager获取会话
        return _sessionManager.getSession(sessionId);
    }

    //获取会话列表
    std::vector<std::string> ChatSDK::getSessionList(){
        if(!_initialized){
            ERR("SDK is not initialized");
            return {};
        }

        //通过SessionManager获取会话列表
        return _sessionManager.getSessionList();
    }

    //删除指定会话
    bool ChatSDK::deleteSession(const std::string& sessionId){
        if(!_initialized){
            ERR("SDK is not initialized");
            return false;
        }

        //通过SessionManager删除会话
        return _sessionManager.deleteSession(sessionId);
    }

    //获取可用模型信息
    std::vector<ModelInfo> ChatSDK::getModelAvailableInfo(){
        if(!_initialized){
            ERR("SDK is not initialized");
            return {};
        }

        //通过可用模型信息
        return _llmManager.GetAvailableModels();
    }

    //发送消息给指定模型 -- 全量返回
    std::string ChatSDK::sendMessage(const std::string& sessionId, const std::string& message){
        if(!_initialized){
            ERR("SDK is not initialized");
            return "";
        }

        //根据SessionId构造消息，并添加到历史消息中
        Message newMessage("user", message);
        _sessionManager.addMessage(sessionId, newMessage);
        auto historyMessages = _sessionManager.getHistroyMessages(sessionId);

        //获取会话对象
        auto session = _sessionManager.getSession(sessionId);
        if(!session){
            ERR("Session {} not found", sessionId);
            return "";
        }

        //构建请求参数
        auto it = _modelConfigs.find(session->_modelName);
        if(it == _modelConfigs.end()){
            ERR("Model {} not found in model configs", session->_modelName);
            return "";
        }
        std::map<std::string, std::string> requestParams;
        requestParams["temperature"] = std::to_string(it->second->_temperature);
        requestParams["max_tokens"] = std::to_string(it->second->_maxTokens);

        //通过LLMManager发送消息
        auto response = _llmManager.SendMessage(session->_modelName, historyMessages, requestParams);
        if(response.empty()){
            ERR("Failed to send message to model {}", session->_modelName);
            return "";
        }

        //添加回复消息到历史消息中，并更新会话时间戳
        Message responseMessage("assistant", response);
        _sessionManager.addMessage(sessionId, responseMessage);
        _sessionManager.updateSessionTimestamp(sessionId);
        INFO("Received response from model {} for session {}", session->_modelName, sessionId);
        return response;
    }

    //发送消息给指定模型 -- 流式返回
    std::string ChatSDK::sendMessageStream(const std::string& sessionId, const std::string& message, std::function<void(const std::string&, bool)> callback){
        if(!_initialized){
            ERR("SDK is not initialized");
            return "";
        }
        
        //根据SessionId构造消息，并添加到历史消息中
        Message newMessage("user", message);
        _sessionManager.addMessage(sessionId, newMessage);
        auto historyMessages = _sessionManager.getHistroyMessages(sessionId);

        //获取会话对象
        auto session = _sessionManager.getSession(sessionId);
        if(!session){
            ERR("Session {} not found", sessionId);
            return "";
        }

        //构建请求参数
        auto it = _modelConfigs.find(session->_modelName);
        if(it == _modelConfigs.end()){
            ERR("Model {} not found in model configs", session->_modelName);
            return "";
        }
        std::map<std::string, std::string> requestParams;
        requestParams["temperature"] = std::to_string(it->second->_temperature);
        requestParams["max_tokens"] = std::to_string(it->second->_maxTokens);

        //通过LLMManager发送消息
        auto response = _llmManager.SendMessageStream(session->_modelName, historyMessages, requestParams, callback);
        if(response.empty()){
            ERR("Failed to send message to model {}", session->_modelName);
            return "";
        }

        //添加回复消息到历史消息中，并更新会话时间戳
        Message responseMessage("assistant", response);
        _sessionManager.addMessage(sessionId, responseMessage);
        _sessionManager.updateSessionTimestamp(sessionId);
        INFO("Received response from model {} for session {}", session->_modelName, sessionId);
        return response;
    }
}
