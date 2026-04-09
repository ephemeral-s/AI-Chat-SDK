#include "ChatServer.h"
#include "jsoncpp/json/json.h"
#include <cstdint>
#include <cstdlib>
#include <httplib.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <memory>

namespace ai_chat_server{
    namespace {
        // 安全读取环境变量，避免 getenv 返回空指针时直接构造 std::string。
        std::string getEnvOrEmpty(const char* key){
            const char* value = std::getenv(key);
            return value == nullptr ? std::string() : std::string(value);
        }
    }

    ChatServer::ChatServer(const ServerConfig& config)
        : _chatSDK(std::make_unique<ai_chat_sdk::ChatSDK>()), _config(config) {
        // 按统一配置初始化各模型 provider，供后续会话按 modelName 选择。
        auto deepseekConfig = std::make_shared<ai_chat_sdk::ApiConfig>();
        deepseekConfig->_modelName = "deepseek-chat";
        deepseekConfig->_apiKey = getEnvOrEmpty("DeepSeek_api_key");
        deepseekConfig->_maxTokens = config.max_tokens;
        deepseekConfig->_temperature = config.temperature;

        auto ChatGPTConfig = std::make_shared<ai_chat_sdk::ApiConfig>();
        ChatGPTConfig->_modelName = "gpt-5.4";
        ChatGPTConfig->_apiKey = getEnvOrEmpty("ChatGPT_api_key");
        ChatGPTConfig->_maxTokens = config.max_tokens;
        ChatGPTConfig->_temperature = config.temperature;

        auto GeminiConfig = std::make_shared<ai_chat_sdk::ApiConfig>();
        GeminiConfig->_modelName = "gemini-3-flash-preview";
        GeminiConfig->_apiKey = getEnvOrEmpty("Gemini_api_key");
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
        _server = std::make_unique<httplib::Server>();
        if(!_server){
            ERR("createServer failed");
            return;
        }
    }

    ChatServer::~ChatServer() = default;

    bool ChatServer::start(){
        if(_isRunning.load()){
            ERR("server is already running");
            return false;
        }

        if(!_server || !_chatSDK){
            ERR("server dependencies are not initialized");
            return false;
        }

        // listen 为阻塞调用，因此 start() 的返回时机天然代表服务已退出。
        // main 中通过独立线程调用它，主线程只负责等待信号并触发 stop()。
        //设置路由规则
        setHttpRoutes();

        //设置静态资源路径 没有具体路径时，默认指向index.html
        _server->set_mount_point("/", "./www");

        _isRunning.store(true);
        INFO("starting server on {}:{}", _config.host, _config.port);
        const bool listenResult = _server->listen(_config.host.c_str(), _config.port);
        _isRunning.store(false);

        if(!listenResult){
            ERR("server failed to listen on {}:{}", _config.host, _config.port);
            return false;
        }

        INFO("server stopped on {}:{}", _config.host, _config.port);
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

    //构建响应
    std::string ChatServer::buildResponse(const std::string& message, bool success){
        Json::Value responseJson;
        responseJson["success"] = success;
        responseJson["message"] = message;

        Json::StreamWriterBuilder writerBuilder;
        std::string responseString = Json::writeString(writerBuilder, responseJson);

        return responseString;
    }

    //处理创建会话请求
    void ChatServer::handleCreateSession(const httplib::Request& request, httplib::Response& response){
        Json::Value requestBody;
        Json::Reader reader;
        if(!reader.parse(request.body, requestBody)){
            std::string errorResponseString = buildResponse("parse request body failed", false);
            response.status = 400;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        std::string modelName = requestBody.get("model", "deepseek-chat").asString();
        std::string sessionId = _chatSDK->createSession(modelName);
        if(sessionId.empty()){
            std::string errorResponseString = buildResponse("create session failed", false);
            response.status = 500;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        Json::Value responseJson;
        Json::Value dataJson;
        responseJson["success"] = true;
        responseJson["message"] = "createSession success";
        dataJson["sessionId"] = sessionId;
        dataJson["model"] = modelName;
        responseJson["data"] = dataJson;

        Json::StreamWriterBuilder writerBuilder;
        std::string responseString = Json::writeString(writerBuilder, responseJson);

        response.body = responseString;
        response.status = 200;
        response.set_content(responseString, "application/json");
    }

    //处理获取会话列表请求
    void ChatServer::handleGetSessionList(const httplib::Request& request, httplib::Response& response){
        (void)request;
        std::vector<std::string> sessionList = _chatSDK->getSessionList();

        Json::Value dataArray;
        for(const auto& sessionId : sessionList){
            auto session = _chatSDK->getSession(sessionId);
            if(session){
                Json::Value sessionJson;
                sessionJson["id"] = session->_sessionId;
                sessionJson["model"] = session->_modelName;
                sessionJson["created_at"] = static_cast<int64_t>(session->_createTime);
                sessionJson["updated_at"] = static_cast<int64_t>(session->_lastActiveTime);
                sessionJson["message_count"] = session->_messages.size();
                if(!session->_messages.empty()) {
                    sessionJson["first_user_message"] = session->_messages[0]._content;
                }
                dataArray.append(sessionJson);
            }
        }

        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "getSessionList success";
        responseJson["data"] = dataArray;

        Json::StreamWriterBuilder writerBuilder;
        std::string responseString = Json::writeString(writerBuilder, responseJson);

        response.body = responseString;
        response.status = 200;
        response.set_content(responseString, "application/json");
    }

    //处理获取模型列表请求
    void ChatServer::handleGetModelList(const httplib::Request& request, httplib::Response& response){
        (void)request;
        auto modelList = _chatSDK->getModelAvailableInfo();

        Json::Value dataArray;
        for(const auto& model : modelList){
            Json::Value modelJson;
            modelJson["name"] = model._modelName;
            modelJson["desc"] = model._modelDesc;
            dataArray.append(modelJson);
        }

        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "getModelList success";
        responseJson["data"] = dataArray;

        Json::StreamWriterBuilder writerBuilder;
        std::string responseString = Json::writeString(writerBuilder, responseJson);

        response.body = responseString;
        response.status = 200;
        response.set_content(responseString, "application/json");
    }

    //处理删除会话请求
    void ChatServer::handleDeleteSession(const httplib::Request& request, httplib::Response& response){
        std::string sessionId = request.matches[1];

        bool success = _chatSDK->deleteSession(sessionId);
        if(!success){
            std::string errorResponseString = buildResponse("delete session failed", false);
            response.status = 404;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        std::string responseString = buildResponse("deleteSession success", true);
        response.body = responseString;
        response.status = 200;
        response.set_content(responseString, "application/json");
    }

    //处理获取会话历史请求
    void ChatServer::handleGetHistory(const httplib::Request& request, httplib::Response& response){
        std::string sessionId = request.matches[1];
        auto session = _chatSDK->getSession(sessionId);
        if(!session){
            std::string errorResponseString = buildResponse("session not found", false);
            response.status = 404;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        Json::Value dataArray;
        for(const auto& message : session->_messages){
            Json::Value messageJson;
            messageJson["id"] = message._id;
            messageJson["role"] = message._role;
            messageJson["content"] = message._content;
            messageJson["timestamp"] = static_cast<int64_t>(message._timestamp);
            dataArray.append(messageJson);
        }

        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "getHistory success";
        responseJson["data"] = dataArray;

        Json::StreamWriterBuilder writerBuilder;
        std::string responseString = Json::writeString(writerBuilder, responseJson);

        response.body = responseString;
        response.status = 200;
        response.set_content(responseString, "application/json");
    }

    //处理发送消息请求
    void ChatServer::handleSendMessage(const httplib::Request& request, httplib::Response& response){
        Json::Value requestBody;
        Json::Reader reader;
        if(!reader.parse(request.body, requestBody)){
            std::string errorResponseString = buildResponse("parse request body failed", false);
            response.status = 400;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        std::string sessionId = requestBody["session_id"].asString();
        std::string message = requestBody["message"].asString();
        if(sessionId.empty() || message.empty()){
            std::string errorResponseString = buildResponse("session_id or message is empty", false);
            response.status = 400;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        std::string assistantResponse = _chatSDK->sendMessage(sessionId, message);
        if(assistantResponse.empty()){
            std::string errorResponseString = buildResponse("send message failed", false);
            response.status = 500;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        Json::Value responseJson;
        Json::Value dataJson;
        dataJson["session_id"] = sessionId;
        dataJson["response"] = assistantResponse;
        responseJson["success"] = true;
        responseJson["message"] = "sendMessage success";
        responseJson["data"] = dataJson;

        Json::StreamWriterBuilder writerBuilder;
        std::string responseString = Json::writeString(writerBuilder, responseJson);
        response.body = responseString;
        response.status = 200;
        response.set_content(responseString, "application/json");
    }

    //处理发送消息流请求
    void ChatServer::handleSendMessageStream(const httplib::Request& request, httplib::Response& response){
        Json::Value requestBody;
        Json::Reader reader;
        if(!reader.parse(request.body, requestBody)){
            std::string errorResponseString = buildResponse("parse request body failed", false);
            response.status = 400;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        std::string sessionId = requestBody["session_id"].asString();
        std::string message = requestBody["message"].asString();
        if(sessionId.empty() || message.empty()){
            std::string errorResponseString = buildResponse("session_id or message is empty", false);
            response.status = 400;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        response.status = 200;
        response.set_header("Cache-Control", "no-cache");
        response.set_header("Connection", "keep-alive");
        response.set_chunked_content_provider("text/event-stream",
            [this, sessionId, message](size_t offset, httplib::DataSink& dataSink)->bool {
                (void)offset;
                auto writeChunk = [&](const std::string& chunk, bool last) {
                    // 每个分片都按 SSE 的 data: 行输出，结束时额外补一个 [DONE] 标记。
                    std::string SSEData = "data: " + Json::valueToQuotedString(chunk.c_str()) + "\n\n";
                    dataSink.write(SSEData.c_str(), SSEData.size());
                    if(last){
                        static const std::string doneData = "data: [DONE]\n\n";
                        dataSink.write(doneData.c_str(), doneData.size());
                        dataSink.done();
                    }
                };

                // sendMessageStream 内部同步驱动回调，这里只负责把回调结果转发给 HTTP 客户端。
                _chatSDK->sendMessageStream(sessionId, message, writeChunk);
                return false;
            });
    }

    void ChatServer::setHttpRoutes(){
        // 带 session_id 的接口使用正则路由，保证 request.matches[1] 可稳定取到路径参数。
        _server->Post("/api/session", [this](const httplib::Request& request, httplib::Response& response){
            this->handleCreateSession(request, response);
        });

        _server->Get("/api/sessions", [this](const httplib::Request& request, httplib::Response& response){
            this->handleGetSessionList(request, response);
        });

        _server->Get("/api/models", [this](const httplib::Request& request, httplib::Response& response){
            this->handleGetModelList(request, response);
        });

        _server->Delete(R"(/api/session/([^/]+))", [this](const httplib::Request& request, httplib::Response& response){
            this->handleDeleteSession(request, response);
        });

        _server->Get(R"(/api/session/([^/]+)/history)", [this](const httplib::Request& request, httplib::Response& response){
            this->handleGetHistory(request, response);
        });

        _server->Post("/api/message", [this](const httplib::Request& request, httplib::Response& response){
            this->handleSendMessage(request, response);
        });

        _server->Post("/api/message/async", [this](const httplib::Request& request, httplib::Response& response){
            this->handleSendMessageStream(request, response);
        });
    }
}
