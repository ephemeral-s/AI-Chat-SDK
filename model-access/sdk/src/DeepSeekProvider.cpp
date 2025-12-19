#include "../include/DeepSeekProvider.h"
#include "../include/util/myLog.h"
#include <jsoncpp/json/json.h>
#include <httplib.h>
#include <jsoncpp/json/reader.h>

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
        return "由 DeepSeek 开发，是一个轻量级模型，适用于多种自然语言处理任务，如文本生成、问答、翻译等。";
    }

    // 发送消息 - 全量返回
    std::string DeepSeekProvider::sendMessage(const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParams)
    {
        //1. 检查模型是否可用
        if(!isAvailable()){
            ERR("DeepSeekProvider::sendMessage: model is not available");
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
            {"Authorization", "Bearer " + _apiKey}
        };

        //5. 发送POST请求
        auto res = client.Post("/v1/chat/completions", headers, requestBodyStr, "application/json");
        if(!res){
            ERR("DeepSeekProvider::sendMessage: http request failed: {}", to_string(res.error()));
            return "";
        }
        //检查响应状态码
        if(res->status != 200){
            ERR("DeepSeekProvider::sendMessage: http request failed, status = {}", res->status);
            return "";
        }
        INFO("DeepSeekProvider::sendMessage: http response status = {}", res->status); // 打印响应状态码
        INFO("DeepSeekProvider::sendMessage: http response body = {}", res->body); // 打印响应体

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
        return "";
    }

    // 发送消息 - 流式返回
    std::string DeepSeekProvider::sendMessageStream(const std::vector<Message>& messages, 
                                                    const std::map<std::string, std::string>& requestParams, 
                                                    std::function<void(const std::string&, bool)> callback)
    {
        //1. 检查模型是否可用
        if(!isAvailable()){
            ERR("DeepSeekProvider::sendMessageStream: model not available");
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
        requestBody["stream"] = true;

        //3. 序列化请求体
        Json::StreamWriterBuilder writerBuilder;
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);
        INFO("DeepSeekProvider::sendMessageStream: requestBodyStr = {}", requestBodyStr); // 打印请求体

        //4. 创建http客户端并设置请求头
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(300, 0);

        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
            {"Content-Type", "application/json"},
            {"Accept", "text/event-stream"}
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
        req.path = "/v1/chat/completions";
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
            INFO("DeepSeekProvider sendMessageStream buffer = {}", buffer); // 打印接收到的数据

            //处理所有流式数据块(数据块之间以\n\n分隔)
            size_t pos = 0; // 记录数据末尾
            while((pos = buffer.find("\n\n")) != std::string::npos){
                //截取当前找到的数据块
                std::string chunk = buffer.substr(0, pos);
                buffer.erase(0, pos + 2); // 移除已处理的数据块

                if(chunk.empty()) continue;

                //解析有效数据
                //获取返回的有效数据
                if(chunk.compare(0, 6, "data: ") == 0){
                    std::string modelData = chunk.substr(6);

                    //先检查是否为结束标记[DONE]
                    if(modelData == "[DONE]"){
                        streamFinished = true;
                        callback("", true);
                        return true;
                    }

                    //反序列化
                    Json::Value modelDataJson;
                    Json::CharReaderBuilder reader;
                    std::string errs;
                    std::istringstream modelDataStream(modelData);
                    if(Json::parseFromStream(reader, modelDataStream, &modelDataJson, &errs)){
                        if(modelDataJson.isMember("choices") && 
                            modelDataJson["choices"].isArray() &&   
                            !modelDataJson["choices"].empty() && 
                            modelDataJson["choices"][0].isMember("delta") &&
                            modelDataJson["choices"][0]["delta"].isMember("content")){
                            std::string deltaContent = modelDataJson["choices"][0]["delta"]["content"].asString();
                            fullResponse += deltaContent;

                            //将本次提取到的有效数据返回给调用者使用
                            callback(deltaContent, false);
                        }
                        else{
                            WRN("DeepSeekProvider::sendMessageStream: parse modelDataJson failed, err = {}", errs);
                            return false;
                        }
                    } 
                }
            }
            return true;
        };

        //6. 给模型发送请求
        //该状态(非阻塞)下，send返回值表示是否成功发送请求；而是否成功响应需要通过响应处理器response_handler判断
        auto res = client.send(req);
        if(!res){
            ERR("NetWork error: {}", to_string(res.error()));
            callback("", true);
        }
        if(!streamFinished){
            WRN("DeepSeekProvider::sendMessageStream: stream not finished");
            callback("", true);
        }
        return fullResponse;
    }
}