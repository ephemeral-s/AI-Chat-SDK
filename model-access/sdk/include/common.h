#pragma once
#include <string>
#include <vector>

namespace ai_chat_sdk{
    //消息结构
    struct Message{
        std::string _id;          //消息ID
        std::string _role;        //角色，user或assistant
        std::string _content;     //消息内容
        std::time_t _timestamp;   //发送时间戳

        //构造函数
        Message(const std::string& role, const std::string& content)
            :_role(role), _content(content) 
        {}
    };

    //模型公共配置信息
    struct Config{
        std::string _modelName;        //模型名称
        double _temperature = 0.7;     //温度参数，控制文本生成的随机性
        int _maxTokens = 2048;         //最大令牌数，控制生成文本的长度
    };

    //通过api调用模型的配置信息
    struct ApiConfig : public Config{
        std::string _apiKey;           //api密钥
    };

    //模型相关信息
    struct ModelInfo{
        std::string _modelName;        //模型名称
        std::string _modelDesc;        //模型描述
        std::string _provider;         //模型提供方
        std::string _endpoint;         //模型api调用地址
        bool _isAvailable = false;      //模型是否可用

        //构造函数
        ModelInfo(const std::string& modelName = "", const std::string& modelDesc = "",
            const std::string& provider = "", const std::string& endpoint = "")
            :_modelName(modelName), _modelDesc(modelDesc), 
            _provider(provider), _endpoint(endpoint)
        {}
    };

    //会话信息
    struct SessionInfo{
        std::string _sessionId;         //会话ID
        std::string _modelName;         //模型名称
        std::vector<Message> _messages; //会话消息记录
        std::time_t _createTime;             //会话创建时间
        std::time_t _lastActiveTime;         //会话最后活跃时间

        //构造函数
        SessionInfo(const std::string& modelName = "")
            :_modelName(modelName)
        {}
    };
} 