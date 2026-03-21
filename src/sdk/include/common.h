#pragma once
#include <string>
#include <vector>

namespace ai_chat_sdk{
    // 信息结构
    struct Message{
        std::string _id;          // 信息ID
        std::string _role;        // 角色：user 或 assistant
        std::string _content;     // 信息内容
        time_t _timestamp;        // 时间戳

        // 构造函数
        Message(const std::string& role, const std::string& content)
            :_role(role), _content(content) 
        {}
    };

    // 模型配置相关信息
    struct Config{
        std::string _modelName;        // 模型名称
        double _temperature = 0.7;     // 温度参数，决定生成文本的多样性
        int _maxTokens = 2048;         // 最大 tokens，限制生成文本的长度
    };

    // 通过 API 访问模型的配置信息
    struct ApiConfig : public Config{
        std::string _apiKey;           // API 密钥
    };

    // 模型信息
    struct ModelInfo{
        std::string _modelName;        // 模型名称
        std::string _modelDesc;        // 模型描述
        std::string _provider;         // 提供方
        std::string _endpoint;         // API 调用地址
        bool _isAvailable = false;     // 是否可用

        // 构造函数
        ModelInfo(const std::string& modelName = "", const std::string& modelDesc = "",
            const std::string& provider = "", const std::string& endpoint = "")
            :_modelName(modelName), _modelDesc(modelDesc), 
            _provider(provider), _endpoint(endpoint)
        {}
    };

    // 会话信息
    struct SessionInfo{
        std::string _sessionId;         // 会话ID
        std::string _modelName;         // 模型名称
        std::vector<Message> _messages; // 会话消息记录
        time_t _createTime;             // 创建时间
        time_t _lastActiveTime;         // 最后活跃时间

        // 构造函数
        SessionInfo(const std::string& modelName = "")
            :_modelName(modelName)
        {}
    };
}
