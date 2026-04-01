#pragma once
#include <memory>
#include <sqlite3.h>
#include <string>
#include <mutex>
#include "common.h"

namespace ai_chat_sdk{
    class DataManager{
    public:
        DataManager(const std::string& dbName = "chatdb.db");
        ~DataManager();

        //Session相关操作
        //插入新会话
        bool insertSession(const SessionInfo& sessionInfo);
        //获取指定会话
        std::shared_ptr<SessionInfo> getSession(const std::string& sessionId) const;
        //更新指定会话的时间戳
        bool updateSessionTimestamp(const std::string& sessionId, std::time_t timestamp);
        //删除指定会话（删除会话时也需要删除会话管理的所有消息）
        bool deleteSession(const std::string& sessionId);
        //获取所有会话ID
        std::vector<std::string> getSessionList() const;
        //获取所有会话
        std::vector<std::shared_ptr<SessionInfo>> getAllSessions() const;
        //获取会话总数
        size_t getSessionCount() const;
        //清空所有会话
        void clearSessions();

        //Message相关操作
        //插入消息（需要更新会话时间戳）
        bool insertMessage(const std::string& sessionId, const Message& message);
        //获取指定会话的历史消息
        std::vector<Message> getHistroyMessages(const std::string& sessionId) const;
        //删除指定会话的所有消息
        bool deleteMessages(const std::string& sessionId);
       private:
        //初始化数据库
        bool initDataBase();
        //执行SQL语句
        bool execSQL(const std::string& sql);
    private:
        sqlite3* _db = nullptr; // SQLite数据库连接对象
        std::string _dbName; // 数据库文件名
        mutable std::mutex _mutex; // 保护数据库连接的互斥锁
    };
}
