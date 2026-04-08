#include "ChatServer.h"
#include "jsoncpp/json/json.h"
#include <cstdint>
#include <httplib.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
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
        //反序列化请求体
        Json::Value requestBody;
        Json::Reader reader;
        if(!reader.parse(request.body, requestBody)){
            std::string errorResponseString = buildResponse("parse request body failed", false);
            response.status = 400;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        //获取请求参数
        std::string modelName = requestBody.get("model", "deepseek-chat").asString();

        //创建会话
        std::string sessionId = _chatSDK->createSession(modelName);
        if(sessionId.empty()){
            std::string errorResponseString = buildResponse("create session failed", false);
            response.status = 500;
            response.set_content(errorResponseString, "application/json");
            return;
        } 

        //构建响应体
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

    //处理获取会话列表
    void ChatServer::handleGetSessionList(const httplib::Request& request, httplib::Response& response){
        //获取会话列表
        std::vector<std::string> sessionList = _chatSDK->getSessionList();

        //构建响应体
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
                if(!session->_messages.empty()) sessionJson["first_user_message"] = session->_messages[0]._content;
                
                dataArray.append(sessionJson);
            } 
        }

        //构建响应体
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

    //处理获取模型列表
    void ChatServer::handleGetModelList(const httplib::Request& request, httplib::Response& response){
        //获取支持的模型列表
        auto modelList = _chatSDK->getModelAvailableInfo();

        //构建响应体
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
        //获取请求中的会话ID（会话ID是路径参数）
        std::string sessionId = request.matches[1];

        //删除会话
        bool success = _chatSDK->deleteSession(sessionId);
        if(!success){
            std::string errorResponseString = buildResponse("delete session failed", false);
            response.status = 404;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        //构建响应体
        std::string responseString = buildResponse("deleteSession success", true);
        response.body = responseString;
        response.status = 200;
        response.set_content(responseString, "application/json");
    }

    //处理获取历史消息请求
    void ChatServer::handleGetHistory(const httplib::Request& request, httplib::Response& response){
        //获取会话ID
        std::string sessionId = request.matches[1];
        //获取会话
        auto session = _chatSDK->getSession(sessionId);
        if(!session){
            std::string errorResponseString = buildResponse("session not found", false);
            response.status = 404;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        //构建历史消息列表
        Json::Value dataArray;
        for(const auto& message : session->_messages){
            Json::Value messageJson;
            messageJson["id"] = message._id;
            messageJson["role"] = message._role;
            messageJson["content"] = message._content;
            messageJson["timestamp"] = static_cast<int64_t>(message._timestamp);
            dataArray.append(messageJson);
        }

        //构建响应体
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

    //处理发送消息请求 - 全量返回
    void ChatServer::handleSendMessage(const httplib::Request& request, httplib::Response& response){
        //获取请求参数
        Json::Value requestBody;
        Json::Reader reader;
        if(!reader.parse(request.body, requestBody)){
            std::string errorResponseString = buildResponse("parse request body failed", false);
            response.status = 400;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        //解析请求参数
        std::string sessionId = requestBody["session_id"].asString();
        std::string message = requestBody["message"].asString();
        if(sessionId.empty() || message.empty()){
            std::string errorResponseString = buildResponse("session_id or message is empty", false);
            response.status = 400;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        //发送消息
        std::string assistantResponse = _chatSDK->sendMessage(sessionId, message);
        if(assistantResponse.empty()){
            std::string errorResponseString = buildResponse("send message failed", false);
            response.status = 500;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        //构造响应参数
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

    //处理发送消息请求 - 流式返回
    void ChatServer::handleSendMessageStream(const httplib::Request& request, httplib::Response& response){
        //获取请求参数
        Json::Value requestBody;
        Json::Reader reader;
        if(!reader.parse(request.body, requestBody)){
            std::string errorResponseString = buildResponse("parse request body failed", false);
            response.status = 400;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        //解析请求参数
        std::string sessionId = requestBody["session_id"].asString();
        std::string message = requestBody["message"].asString();
        if(sessionId.empty() || message.empty()){
            std::string errorResponseString = buildResponse("session_id or message is empty", false);
            response.status = 400;
            response.set_content(errorResponseString, "application/json");
            return;
        }

        //准备流式响应
        response.status = 200;
        response.set_header("Cache-Control", "no-cache"); // 禁用缓存
        response.set_header("Connection", "keep-alive"); // 保持连接
        response.set_chunked_content_provider("application/json", 
            [this, sessionId, message](size_t offset, httplib::DataSink& dataSink)->bool
        {
            auto writeChunk = [&](const std::string& chunk, bool last)
            {
                //将chunk转化为SSE数据格式
                //并对chunk进行转义，防止特殊字符导致解析错误
                std::string SSEData = "data: " + Json::valueToQuotedString(chunk.c_str()) + "\n\n";
                dataSink.write(SSEData.c_str(), SSEData.size()); // 将收到的响应流立即发给客户端

                //处理流式响应结束标志
                if(last){
                    //流式响应结束
                    std::string doneData = "[DONE]";
                    dataSink.write(doneData.c_str(), doneData.size()); // 将流式响应结束标志立即发给客户端
                    dataSink.done(); // 标记流式响应结束
                }     
            };

            //先给客户端发一个空的数据块
            writeChunk("", true);
            _chatSDK->sendMessageStream(sessionId, message, writeChunk);
            return true; // 表明后续还有数据，因为消息是异步发送的
        });
    }

    //设置HTTP路由规则
    void ChatServer::setHttpRoutes(){
        //处理创建会话请求
        _server->Post("api/session", [this](const httplib::Request& request, httplib::Response& response){
            this->handleCreateSession(request, response);
        });

        //处理获取会话列表
        _server->Get("api/sessions", [this](const httplib::Request& request, httplib::Response& response){
            this->handleGetSessionList(request, response);
        });

        //处理获取模型列表
        _server->Get("api/models", [this](const httplib::Request& request, httplib::Response& response){
            this->handleGetModelList(request, response);
        });

        //处理删除会话请求
        _server->Delete("api/session/${session_id}", [this](const httplib::Request& request, httplib::Response& response){
            this->handleDeleteSession(request, response);
        });

        //处理获取历史消息请求
        _server->Get("api/session/${session_id}/history", [this](const httplib::Request& request, httplib::Response& response){
            this->handleGetHistory(request, response);
        });

        //处理发送消息请求 - 全量返回
        _server->Post("api/message", [this](const httplib::Request& request, httplib::Response& response){
            this->handleSendMessage(request, response);
        });

        //处理发送消息请求 - 流式返回
        _server->Post("api/message/async", [this](const httplib::Request& request, httplib::Response& response){
            this->handleSendMessageStream(request, response);
        });
    }
}