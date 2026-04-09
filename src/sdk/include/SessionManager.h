#pragma once
#include "common.h"
#include "DataManager.h"
#include <cstddef>
#include <unordered_map>
#include <string>
#include <mutex>
#include <atomic>
#include <memory>

namespace ai_chat_sdk{
    class SessionManager{
    public:
        SessionManager() = default;
        ~SessionManager() = default;
        SessionManager(const std::string& dbName = "chatdb.db");

        //新建会话（提供模型名称）
        std::string createSession(const std::string& modelName);
        //通过会话ID获取会话信息
        std::shared_ptr<SessionInfo> getSession(const std::string& sessionId);
        //向会话添加消息
        bool addMessage(const std::string& sessionId, const Message& message);
        //获取会话的历史消息
        std::vector<Message> getHistroyMessages(const std::string& sessionId);
        //更新会话时间戳
        void updateSessionTimestamp(const std::string& sessionId);
        //获取所有会话列表
        std::vector<std::string> getSessionList();
        //删除会话
        bool deleteSession(const std::string& sessionId);
        //清空所有会话
        void clearSessions();
        //获取会话总数
        size_t getSessionCount() const;
    private:
        std::unordered_map<std::string, std::shared_ptr<SessionInfo>> _sessions; // 管理所有会话信息 <会话ID, 会话信息>
        mutable std::mutex _mutex; // 保护会话信息的互斥锁
        static std::atomic<size_t> _sessionCounter; // 记录当前会话总数

        std::string generateSessionId(); // 生成唯一会话ID
        std::string generateMessageId(size_t messageCounter); // 生成唯一消息ID
        DataManager _dataManager; // 数据管理器实例    
    };
}