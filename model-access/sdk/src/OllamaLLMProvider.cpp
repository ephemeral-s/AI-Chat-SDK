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
        //1. 检查模型是否可用
        if(!isAvailable()){
            ERR("OllamaLLMProvider::sendMessage: model not available");
            return "";
        }

        //2. 构建请求体
        //构建历史消息
        Json::Value messageArray(Json::arrayValue);
        for(const auto& msg : messages){
            Json::Value message(Json::objectValue);
            message["role"] = msg._role;
            message["content"] = msg._content;
            messageArray.append(message);
        }

        //构建请求参数
        double temperature = 0.7;
        int maxTokens = 2048;
        if(requestParams.find("temperature") != requestParams.end()){
            temperature = std::stod(requestParams.at("temperature"));
        }
        if(requestParams.find("max_tokens") != requestParams.end()){
            maxTokens = std::stoi(requestParams.at("max_tokens"));
        }

        Json::Value options(Json::objectValue);
        options["temperature"] = temperature;
        options["num_ctx"] = maxTokens;

        Json::Value requestBody(Json::objectValue);
        requestBody["model"] = getModelName();
        requestBody["messages"] = messageArray;
        requestBody["options"] = options;
        requestBody["stream"] = false;

        //3. 序列化请求体
        Json::StreamWriterBuilder writerBuilder;
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);
        INFO("OllamaLLMProvider::sendMessage: requestBodyStr = {}", requestBodyStr); // 打印请求体

        //4. 创建http客户端并设置请求头
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(60, 0);

        httplib::Headers headers = {
            {"Content-Type", "application/json"}
        };

        //5. 发送POST请求
        auto res = client.Post("/api/chat", headers, requestBodyStr, "application/json");
        if(!res){
            ERR("OllamaLLMProvider::sendMessage: failed to send request, error = {}",  to_string(res.error()));
            return "";
        }
        //检查响应状态码
        if(res->status != 200){
            ERR("OllamaLLMProvider::sendMessage: http request failed, status = {}", res->status);
            return "";
        }
        INFO("OllamaLLMProvider::sendMessage: http response status = {}", res->status); // 打印响应状态码
        INFO("OllamaLLMProvider::sendMessage: http response body = {}", res->body); // 打印响应体

        //6. 解析响应体并返回
        Json::Value responseBody;
        Json::CharReaderBuilder readerBuilder;
        std::string parseError;
        std::istringstream responseStream(res->body);
        if(Json::parseFromStream(readerBuilder, responseStream, &responseBody, &parseError)){
            if(responseBody.isMember("message") && responseBody["message"].isObject() && responseBody["message"].isMember("content")){
                auto replyContent = responseBody["message"]["content"].asString();
                INFO("OllamaLLMProvider::sendMessage: replyContent = {}", replyContent); // 打印回复内容
                return replyContent;
            }
        }
        
        //解析失败
        ERR("OllamaLLMProvider::sendMessage: parse response body failed, error = {}", parseError);
        return "";
    }

    // 发送消息 - 流式返回
    std::string OllamaLLMProvider::sendMessageStream(const std::vector<Message>& messages, 
                                                const std::map<std::string, std::string>& requestParams, 
                                                std::function<void(const std::string&, bool)> callback){
        return "";
    }
}
