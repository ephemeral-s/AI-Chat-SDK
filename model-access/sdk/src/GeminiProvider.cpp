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
        return "gemini-2.5-flash";
    }

    // 获取模型描述
    std::string GeminiProvider::getModelDesc() const{
        return "Google开发的急速响应模型，支持文本、图像、视频等多模态输入，适用于实时对话场景。";
    }

    // 发送消息 - 全量返回
    std::string GeminiProvider::sendMessage(const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParams)
    {
        //1. 检查模型是否可用
        if(!isAvailable()){
            ERR("GeminiProvider::sendMessage: model not available");
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
        INFO("GeminiProvider::sendMessage: requestBodyStr = {}", requestBodyStr);

        //4. 创建http客户端并设置请求头
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(60, 0);
        client.set_proxy("127.0.0.1", 7890);

        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey}
        };

        //5. 发送POST请求
        auto res = client.Post("/v1beta/openai/chat/completions", headers, requestBodyStr, "application/json");
        if(!res){
            ERR("GeminiProvider::sendMessage: http request failed, error = {}", to_string(res.error()));
            return "";
        }
        //检查响应状态码
        if(res->status != 200){
            ERR("GeminiProvider::sendMessage: http request failed, status = {}", res->status);
            return "";
        }
        INFO("GeminiProvider::sendMessage: http response status = {}", res->status); // 打印响应状态码
        INFO("GeminiProvider::sendMessage: http response body = {}", res->body); // 打印响应体

        //6. 解析响应体并返回
        Json::Value responseBody;
        Json::CharReaderBuilder readerBuilder;
        std::string parseError;
        std::istringstream responseStream(res->body);
        if(Json::parseFromStream(readerBuilder, responseStream, &responseBody, &parseError)){
            if(responseBody.isMember("choices") && responseBody["choices"].isArray() && !responseBody["choices"].empty()){
                auto choice = responseBody["choices"][0];
                if(choice.isMember("message") && choice["message"].isMember("content")){
                    auto replyContent = choice["message"]["content"].asString();
                    INFO("GeminiProvider::sendMessage: replyContent = {}", replyContent);
                    return replyContent;
                }
            }
        }
       
        //解析失败
        ERR("GeminiProvider::sendMessage: parse response body failed, error = {}", parseError);
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