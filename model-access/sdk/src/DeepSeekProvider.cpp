#include "../include/DeepSeekProvider.h"
#include "../include/util/myLog.h"
#include <jsoncpp/json/json.h>
#include <httplib.h>

namespace ai_chat_sdk{

    // 初始化模型
    bool DeepSeekProvider::initModel(const std::map<std::string, std::string>& modelConfig){
        // 初始化 API 密钥
        auto it = modelConfig.find("api_key");
        if(it == modelConfig.end()){
            ERR("DeepSeekProvider::initModel: api_key not found in modelConfig");
            return false;
        }else{
            _apiKey = it->second;
        }

        // 初始化调用地址
        it = modelConfig.find("endpoint");
        if(it == modelConfig.end()){
            ERR("DeepSeekProvider::initModel: endpoint not found in modelConfig");
            return false;
        }else{
            _endpoint = it->second;
        }
        
        _isAvailable = true;
        INFO("DeepSeekProvider::initModel: init success");
        return true;
    }
        
    // 是否可用
    bool DeepSeekProvider::isAvailable() const{
        return _isAvailable;
    }

    // 获取模型名称
    std::string DeepSeekProvider::getModelName() const{
        return "deepseek-chat";
    }

    // 获取模型描述
    std::string DeepSeekProvider::getModelDesc() const{
        return "一款实用性强，中文优化的聊天模型，适用于日常问答与创作";
    }

    // 发送消息 - 全量返回
    std::string DeepSeekProvider::sendMessage(const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParams){
        //1. 检查模型是否可用
        if(!_isAvailable){
            ERR("DeepSeekProvider::sendMessage: model not available");
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

        Json::Value requestBody(Json::objectValue);
        requestBody["model"] = getModelName();
        requestBody["messages"] = messageArray;
        requestBody["temperature"] = temperature;
        requestBody["max_tokens"] = maxTokens;
        requestBody["stream"] = false;

        //3. 序列化请求体
        Json::StreamWriterBuilder writerBuilder;
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);
        INFO("DeepSeekProvider::sendMessage: requestBodyStr = {}", requestBodyStr); // 打印请求体

        //4. 创建http客户端并设置请求头
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(60, 0);

        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
            {"Content-Type", "application/json"}
        };

        //5. 发送POST请求
        auto res = client.Post("/v1/chat/completions", headers, requestBodyStr, "application/json");
        if(!res){
            ERR("DeepSeekProvider::sendMessage: http request failed");
            return "";
        }
        INFO("DeepSeekProvider::sendMessage: http response status = {}", res->status); // 打印响应状态码
        INFO("DeepSeekProvider::sendMessage: http response body = {}", res->body); // 打印响应体
        //检查响应状态码
        if(res->status != 200){
            ERR("DeepSeekProvider::sendMessage: http request failed, status = {}", res->status);
            return "";
        }

        //6. 解析响应体并返回
        Json::Value responseBody;
        Json::CharReaderBuilder readerBuilder;
        std::string parseError;
        std::istringstream responseStream(res->body);
        if(Json::parseFromStream(readerBuilder, responseStream, &responseBody, &parseError)){
            //获取message
            if(responseBody.isMember("choices") && responseBody["choices"].isArray() && !responseBody["choices"].empty()){
                auto choice = responseBody["choices"][0];
                if(choice.isMember("message") && choice["message"].isMember("content")){
                    auto replyContent = choice["message"]["content"].asString();
                    INFO("DeepSeekProvider::sendMessage: replyContent = {}", replyContent); // 打印回复内容
                    return replyContent;
                }
            }
        }

        //解析失败
        ERR("DeepSeekProvider::sendMessage: parse response body failed, error = {}", parseError);
        return "deepseek parse response body failed";
    }

    // 发送消息 - 流式返回
    std::string DeepSeekProvider::sendMessageStream(const std::vector<Message>& messages, 
                                                    const std::map<std::string, std::string>& requestParams, 
                                                    std::function<void(const std::string&, bool)> callback){
        return "";
    }
}
