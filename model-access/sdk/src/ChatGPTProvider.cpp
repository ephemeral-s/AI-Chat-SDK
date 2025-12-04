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
        //1. 检测模型是否可用
        if(!_isAvailable){
            ERR("ChatGPTProvider::sendMessage: model is not available");
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
        int maxOutputTokens = 2048;
        if(requestParams.find("temperature") != requestParams.end()){
            temperature = std::stod(requestParams.at("temperature"));
        }
        if(requestParams.find("max_output_tokens") != requestParams.end()){
            maxOutputTokens = std::stoi(requestParams.at("max_output_tokens"));
        }

        Json::Value requestBody(Json::objectValue);
        requestBody["model"] = getModelName();
        requestBody["input"] = messageArray;
        requestBody["temperature"] = temperature;
        requestBody["max_output_tokens"] = maxOutputTokens;

        //3. 序列化请求体
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);

        //4. 创建http客户端并设置请求头
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(60, 0);
        client.set_proxy("127.0.0.1", 7890); // 设置代理

        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey}
        };

        //5. 发送POST请求
        auto res = client.Post("/v1/responses", headers, requestBodyStr, "application/json");
        if(!res){
            ERR("ChatGPTProvider::sendMessage: http request failed: {}", to_string(res.error()));
            return "";
        }
        //检查响应状态码
        if(res->status != 200){
            ERR("ChatGPTProvider::sendMessage: http request failed, status code: {}", res->status);
            return "";
        }
        INFO("ChatGPTProvider::sendMessage: http response status = {}", res->status); // 打印响应状态码
        INFO("ChatGPTProvider::sendMessage: http response body = {}", res->body); // 打印响应体

        //6. 对响应体进行反序列化
        Json::Value responseBody;
        Json::CharReaderBuilder readerBuilder;
        std::string parseError;
        std::istringstream responseStream(res->body);
        if(!Json::parseFromStream(readerBuilder, responseStream, &responseBody, &parseError)){
            ERR("ChatGPTProvider::sendMessage: parse response body failed, error: {}", parseError);
            return "";
        }
        
        //7. 解析响应体并返回
        if(responseBody.isMember("output") && responseBody["output"].isArray() && !responseBody["output"].empty()){
            auto output = responseBody["output"][0];
            if(output.isMember("content") && output["content"].isArray() && !output["content"].empty() && output["content"][0].isMember("text")){
                auto replyContent = output["content"][0]["text"].asString();
                INFO("ChatGPTProvider::sendMessage: reply content = {}", replyContent); // 打印回复内容
                return replyContent;
            }
        }

        ERR("ChatGPTProvider::sendMessage: parse response body failed, error: {}", parseError);
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
