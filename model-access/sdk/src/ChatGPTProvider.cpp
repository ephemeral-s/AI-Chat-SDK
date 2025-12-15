#include "../include/ChatGPTProvider.h"
#include "../include/util/myLog.h"
#include <cstdint>
#include <jsoncpp/json/json.h>
#include <httplib.h>
#include <jsoncpp/json/reader.h>
#include <sstream>

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
        if(!isAvailable()){
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

        //6. 解析响应体并返回
        Json::Value responseBody;
        Json::CharReaderBuilder readerBuilder;
        std::string parseError;
        std::istringstream responseStream(res->body);
        if(!Json::parseFromStream(readerBuilder, responseStream, &responseBody, &parseError)){
            ERR("ChatGPTProvider::sendMessage: parse response body failed, error: {}", parseError);
            return "";
        }

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
        //1. 检测模型是否可用
        if(!isAvailable()){
            ERR("ChatGPTProvider::sendMessageStream: model is not available");
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
        requestBody["stream"] = true;

        //3. 序列化请求体
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);

        //4. 创建http客户端并设置请求头
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(300, 0);
        client.set_proxy("127.0.0.1", 7890); // 设置代理

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
        req.path = "/v1/responses";
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
            INFO("ChatGPTProvider sendMessageStream buffer = {}", buffer); // 打印接收到的数据

            //处理所有流式数据块(数据块之间以\n\n分隔)
            size_t pos = 0; // 记录数据末尾
            while((pos = buffer.find("\n\n")) != std::string::npos){
                //截取当前找到的数据块
                std::string event = buffer.substr(0, pos);
                buffer.erase(0, pos + 2); // 移除已处理的数据块

                //解析事件类型和具体数据的位置
                std::istringstream eventStream(event);
                std::string eventType;
                std::string eventData;
                std::string line;
                while(std::getline(eventStream, line)){
                    if(line.empty()){
                        continue;
                    }
                    if(line.compare(0, 6, "event:") == 0){
                        eventType = line.substr(7);
                    }
                    else if(line.compare(0, 5, "data:") == 0){
                        eventData = line.substr(6);
                    }
                }

                //对模型返回结果进行序列化
                Json::Value chunk;
                Json::CharReaderBuilder readerBuilder;
                std::string errs;
                std::istringstream eventDataStream(eventData);
                if(!Json::parseFromStream(readerBuilder, eventDataStream, &chunk, &errs)){
                    ERR("ChatGPTProvider::sendMessageStream: parse json error, error = {}", errs);
                    continue;
                }

                //按照事件类型进行数据分析
                if(eventType == "response.output_text.delta"){ // 模型返回的文本增量
                    if(chunk.isMember("delta") && chunk["delta"].isString()){
                        std::string delta = chunk["delta"].asString();
                        callback(delta, false);
                    }
                }
                else if(eventType == "response.output_item.done"){ // 模型返回的文本增量结束(包含整个消息)
                    if(chunk.isMember("item") && chunk["item"].isObject()){
                        Json::Value item = chunk["item"];
                        if(item.isMember("content") && item["content"].isArray() && !item["content"].empty() && item["content"][0].isMember("text") && 
                        item["content"][0]["text"].isString()){
                            fullResponse += item["content"][0]["text"].asString();
                        }
                    }
                }
                else if(eventType == "response.completed"){ // 模型响应结束
                    streamFinished = true;
                    callback("", true);
                    return true;
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
