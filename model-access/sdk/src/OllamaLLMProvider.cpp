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
        //1. 检查模型是否可用
        if(!isAvailable()){
            ERR("OllamaLLMProvider::sendMessageStream: model not available");
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
        requestBody["stream"] = true;

        //3. 序列化请求体
        Json::StreamWriterBuilder writerBuilder;
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);
        INFO("OllamaLLMProvider::sendMessageStream: requestBodyStr = {}", requestBodyStr); // 打印请求体

        //4. 创建http客户端并设置请求头
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(300, 0);

        httplib::Headers headers = {
            {"Content-Type", "application/json"}
        };

        //流式响应处理变量
        std::string buffer;          // 用于累计响应体的缓冲区
        bool gotError = false;       // 标记响应是否成功
        std::string errorMsg;        // 用于存储错误信息
        int statusCode = 0;          // 用于存储http响应状态码
        bool streamFinished = false; // 标记是否完成流式响应
        std::string fullResponse;    // 用于存储完整的响应体

        //5. 构建请求对象
        httplib::Request req;
        req.method = "POST";
        req.path = "/api/chat";
        req.headers = headers;
        req.body = requestBodyStr;

        //设置响应处理器
        req.response_handler = [&](const httplib::Response& res) -> bool {
            statusCode = res.status;
            if(statusCode != 200){
                gotError = true;
                errorMsg = "http request failed, status = " + std::to_string(statusCode);
                return false;
            }
            return true;
        };

        //设置数据接收处理器 -- 接收流式响应每一部分的数据
        req.content_receiver = [&](const char* data, size_t len, size_t offset, size_t totalLength) -> bool {
            //验证响应是否出错
            if(gotError){
                return false;
            }

            buffer.append(data, len);
            INFO("OllamaLLMProvider sendMessageStream buffer = {}", buffer); // 打印接收到的数据

            //处理所有流式数据块(数据块之间以\n分隔)
            size_t pos = 0; // 记录数据末尾
            while((pos = buffer.find("\n")) != std::string::npos){
                //截取当前找到的数据块
                std::string modelDataStr = buffer.substr(0, pos);
                buffer.erase(0, pos + 1); // 移除已处理的数据块

                if(modelDataStr.empty()) continue;

                //反序列化解析有效数据
                Json::Value modelDataJson;
                Json::CharReaderBuilder reader;
                std::string errs;
                std::istringstream modelDataStream(modelDataStr);
                if(Json::parseFromStream(reader, modelDataStream, &modelDataJson, &errs)){
                    //先判断是否为结尾数据
                    if(modelDataJson.get("done", false).asBool()){
                        streamFinished = true;
                        callback("", true);
                        return true;
                    }

                    //解析有效数据
                    if(modelDataJson.isMember("message") && modelDataJson["message"].isObject() && modelDataJson["message"].isMember("content")){
                        std::string deltaContent = modelDataJson["message"]["content"].asString();
                        fullResponse += deltaContent;

                        //将本次提取到的有效数据返回给调用者使用
                        callback(deltaContent, false);
                    }else{
                        WRN("OllamaLLMProvider::sendMessageStream: parse modelDataJson failed, err = {}", errs);
                        return false;
                    }
                }
            }
            return true;
        };

        //6. 给模型发送请求
        auto res = client.send(req);
        if(!res){
            ERR("NetWork error: {}", to_string(res.error()));
            callback("", true);
        }
        if(!streamFinished){
            WRN("OllamaLLMProvider::sendMessageStream: stream not finished");
            callback("", true);
        }
        return fullResponse;
    }
}