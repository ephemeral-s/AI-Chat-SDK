#include "../include/DataManager.h"
#include "../include/util/myLog.h"
#include <bits/types/time_t.h>
#include <cstdint>

namespace ai_chat_sdk{
    DataManager::DataManager(const std::string& dbName)
        :_dbName(dbName) 
    {
        //创建并打开数据库
        int rc = sqlite3_open(_dbName.c_str(), &_db);
        if(rc != SQLITE_OK){
            ERR("打开数据库失败：{}", sqlite3_errmsg(_db));
        }
        INFO("数据库打开成功");

        //初始化表
        if(!initDataBase()){
            sqlite3_close(_db);
            _db = nullptr;
            ERR("初始化数据库表失败");
        }
        else INFO("数据库表初始化成功");
    }

    DataManager::~DataManager(){
        if(_db != nullptr){
            sqlite3_close(_db);
            _db = nullptr;
        }
    }

    //初始化数据库表
    bool DataManager::initDataBase(){
        //创建会话表
        std::string createSessionTable = R"(
            CREATE TABLE IF NOT EXISTS sessions (
                session_id TEXT PRIMARY KEY, 
                model_name TEXT NOT NULL, 
                create_time INTEGER NOT NULL,
                update_time INTEGER NOT NULL
            );
        )";

        //执行创建会话表的SQL语句
        if(!execSQL(createSessionTable)){
            return false;
        }

        //创建消息表
        std::string createMessageTable = R"(
            CREATE TABLE IF NOT EXISTS messages (
                message_id TEXT PRIMARY KEY,
                session_id TEXT NOT NULL,
                role TEXT NOT NULL,
                content TEXT NOT NULL,
                create_time INTEGER NOT NULL,
                FOREIGN KEY (session_id) REFERENCES sessions (session_id) ON DELETE CASCADE
                )
        )";
        if(!execSQL(createMessageTable)){
            return false;
        }
        
        return true;
    }

    //执行SQL语句
    bool DataManager::execSQL(const std::string& sql){
       if(_db == nullptr){
           ERR("数据库连接为空");
           return false;
       }

       char* errMsg = nullptr;
       int rc = sqlite3_exec(_db, sql.c_str(), nullptr, nullptr, &errMsg);
       if(rc != SQLITE_OK){
           ERR("执行SQL语句失败：{}", errMsg);
           sqlite3_free(errMsg);
           return false;
       }
       sqlite3_free(errMsg);
       return true;
    }

    //插入新会话
    bool DataManager::insertSession(const SessionInfo& sessionInfo){
        std::unique_lock<std::mutex> lock(_mutex);
        std::string insertSession = R"(
            INSERT INTO sessions (session_id, model_name, create_time, update_time)
            VALUES (?, ?, ?, ?)
        )";

        //准备SQL语句
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(_db, insertSession.c_str(), -1, &stmt, nullptr);
        if(rc != SQLITE_OK){
            ERR("insertSession 准备SQL语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }
        
        //绑定参数
        rc = sqlite3_bind_text(stmt, 1, sessionInfo._sessionId.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_bind_text(stmt, 2, sessionInfo._modelName.c_str(), -1, SQLITE_STATIC);
        rc = sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(sessionInfo._createTime));
        rc = sqlite3_bind_int64(stmt, 4, static_cast<int64_t>(sessionInfo._lastActiveTime));

        //执行SQL语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE){
            ERR("insertSession 执行SQL语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        INFO("insertSession 会话插入成功：{}", sessionInfo._sessionId);
        return true;
    }

    //获取指定会话
    std::shared_ptr<SessionInfo> DataManager::getSession(const std::string& sessionId) const {
        std::unique_lock<std::mutex> lock(_mutex);
        std::string selectSession = R"(
            SELECT model_name, create_time, update_time FROM sessions WHERE session_id = ?
        )";

        //准备SQL语句
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(_db, selectSession.c_str(), -1, &stmt, nullptr);
        if(rc != SQLITE_OK){
            ERR("getSession 准备SQL语句失败：{}", sqlite3_errmsg(_db));
            return nullptr;
        }
        
        //绑定参数
        rc = sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_STATIC);
        
        //执行SQL语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_ROW){
            ERR("getSession 执行SQL语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return nullptr;
        }
        
        //提取结果
        std::string modelName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int64_t createTime = sqlite3_column_int64(stmt, 1);
        int64_t lastActiveTime = sqlite3_column_int64(stmt, 2);

        //创建会话对象
        std::shared_ptr<SessionInfo> sessionInfo = std::make_shared<SessionInfo>(modelName);
        sessionInfo->_sessionId = sessionId;
        sessionInfo->_createTime = static_cast<time_t>(createTime);
        sessionInfo->_lastActiveTime = static_cast<time_t>(lastActiveTime);

        //获取该会话中的所有消息
        sessionInfo->_messages = getHistroyMessages(sessionId);

        //释放SQL语句
        sqlite3_finalize(stmt);
        INFO("getSession 会话获取成功：{}", sessionId);
        return sessionInfo;
    }

    //更新指定会话的时间戳
    bool DataManager::updateSessionTimestamp(const std::string& sessionId, std::time_t timestamp){
        std::unique_lock<std::mutex> lock(_mutex);
        std::string updateSession = R"(
            UPDATE sessions SET update_time = ? WHERE session_id = ?
        )";

        //准备SQL语句
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(_db, updateSession.c_str(), -1, &stmt, nullptr);
        if(rc != SQLITE_OK){
            ERR("updateSessionTimestamp 准备SQL语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }
        
        //绑定参数
        rc = sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(timestamp));
        rc = sqlite3_bind_text(stmt, 2, sessionId.c_str(), -1, SQLITE_STATIC);
        
        //执行SQL语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE){
            ERR("updateSessionTimestamp 执行SQL语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        INFO("updateSessionTimestamp 会话时间戳更新成功：{}", sessionId);
        return true;
    }

    //删除指定会话（删除会话时也需要删除会话管理的所有消息）
    bool DataManager::deleteSession(const std::string& sessionId){
        std::unique_lock<std::mutex> lock(_mutex);
        std::string deleteSession = R"(
            DELETE FROM sessions WHERE session_id = ?
        )";

        //准备SQL语句
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(_db, deleteSession.c_str(), -1, &stmt, nullptr);
        if(rc != SQLITE_OK){
            ERR("deleteSession 准备SQL语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }
        
        //绑定参数
        rc = sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_STATIC);
        
        //执行SQL语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE){
            ERR("deleteSession 执行SQL语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        INFO("deleteSession 会话删除成功：{}", sessionId);
        return true;
    }

    //获取所有会话ID
    std::vector<std::string> DataManager::getSessionList() const{
        std::unique_lock<std::mutex> lock(_mutex);
        std::string getSession = R"(
            SELECT session_id FROM sessions ORDER BY update_time DESC
        )";

        //准备SQL语句
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(_db, getSession.c_str(), -1, &stmt, nullptr);
        if(rc != SQLITE_OK){
            ERR("getSessionList 准备SQL语句失败：{}", sqlite3_errmsg(_db));
            return {};
        }
        
        //执行SQL语句
        std::vector<std::string> sessionIds;
        while(sqlite3_step(stmt) == SQLITE_ROW){
            std::string sessionId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            sessionIds.push_back(sessionId);
        }
        sqlite3_finalize(stmt);
        INFO("getSessionList 会话列表获取成功，会话总数：{}", sessionIds.size());
        return sessionIds;
    }

    //获取所有会话，并按照更新时间降序排列
    std::vector<std::shared_ptr<SessionInfo>> DataManager::getAllSessions() const{
        std::unique_lock<std::mutex> lock(_mutex);
        std::string getSession = R"(
            SELECT session_id, model_name, create_time, update_time FROM sessions ORDER BY update_time DESC
        )";

        //准备SQL语句
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(_db, getSession.c_str(), -1, &stmt, nullptr);
        if(rc != SQLITE_OK){
            ERR("getAllSessions 准备SQL语句失败：{}", sqlite3_errmsg(_db));
            return {};
        }
        
        //执行SQL语句
        std::vector<std::shared_ptr<SessionInfo>> sessionsInfos;
        while(sqlite3_step(stmt) == SQLITE_ROW){
            std::string sessionId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string modelName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            int64_t createTime = sqlite3_column_int64(stmt, 2);
            int64_t lastActiveTime = sqlite3_column_int64(stmt, 3);

            std::shared_ptr<SessionInfo> sessionInfo = std::make_shared<SessionInfo>(modelName);
            sessionInfo->_sessionId = sessionId;
            sessionInfo->_createTime = static_cast<time_t>(createTime);
            sessionInfo->_lastActiveTime = static_cast<time_t>(lastActiveTime);
            sessionsInfos.push_back(sessionInfo);

            //历史消息先不获取，点击指定会话时再获取
        }
        sqlite3_finalize(stmt);
        INFO("getAllSessions 会话列表获取成功，会话总数：{}", sessionsInfos.size());
        return sessionsInfos;
    }

    //获取会话总数
    size_t DataManager::getSessionCount() const{
        std::unique_lock<std::mutex> lock(_mutex);
        std::string getSession = R"(
            SELECT COUNT(*) FROM sessions
        )";

        //准备SQL语句
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(_db, getSession.c_str(), -1, &stmt, nullptr);
        if(rc != SQLITE_OK){
            ERR("getSessionCount 准备SQL语句失败：{}", sqlite3_errmsg(_db));
            return 0;
        }
        
        //执行SQL语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_ROW){
            ERR("getSessionCount 执行SQL语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return 0;
        }

        size_t count = sqlite3_column_int64(stmt, 0);
        INFO("getSessionCount 会话总数获取成功：{}", count);
        return count;
    }

    //清空所有会话
    void DataManager::clearSessions(){
        std::unique_lock<std::mutex> lock(_mutex);
        std::string clearSessions = R"(
            DELETE FROM sessions
        )";

        //准备SQL语句
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(_db, clearSessions.c_str(), -1, &stmt, nullptr);
        if(rc != SQLITE_OK){
            ERR("clearSessions 准备SQL语句失败：{}", sqlite3_errmsg(_db));
            return;
        }
        
        //执行SQL语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE){
            ERR("clearSessions 执行SQL语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return;
        }
        sqlite3_finalize(stmt);
        INFO("clearSessions 会话列表清空成功");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //插入消息（需要更新会话时间戳）
    bool DataManager::insertMessage(const std::string& sessionId, const Message& message){
        {
            std::unique_lock<std::mutex> lock(_mutex);
            std::string insertMessage = R"(
                INSERT INTO messages (message_id, session_id, role, content, create_time)
                VALUES (?, ?, ?, ?, ?)
            )";

            //准备SQL语句
            sqlite3_stmt* stmt = nullptr;
            int rc = sqlite3_prepare_v2(_db, insertMessage.c_str(), -1, &stmt, nullptr);
            if(rc != SQLITE_OK){
                ERR("insertMessage 准备SQL语句失败：{}", sqlite3_errmsg(_db));
                return false;
            }

            //绑定参数
            rc = sqlite3_bind_text(stmt, 1, message._id.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_bind_text(stmt, 2, sessionId.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_bind_text(stmt, 3, message._role.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_bind_text(stmt, 4, message._content.c_str(), -1, SQLITE_STATIC);
            rc = sqlite3_bind_int64(stmt, 5, static_cast<int64_t>(message._timestamp));

            //执行SQL语句
            rc = sqlite3_step(stmt);
            if(rc != SQLITE_DONE){
                ERR("insertMessage 执行SQL语句失败：{}", sqlite3_errmsg(_db));
                sqlite3_finalize(stmt);
                return false;
            }
            sqlite3_finalize(stmt);
            INFO("insertMessage 消息插入成功");
        }

        //更新时间
        return updateSessionTimestamp(sessionId, message._timestamp);
    }

    //获取指定会话的历史消息
    std::vector<Message> DataManager::getHistroyMessages(const std::string& sessionId) const{
        std::unique_lock<std::mutex> lock(_mutex);
        std::string getMessages = R"(
            SELECT message_id, role, content, create_time FROM messages WHERE session_id = ?;
        )";

        //准备SQL语句
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(_db, getMessages.c_str(), -1, &stmt, nullptr);
        if(rc != SQLITE_OK){
            ERR("getHistroyMessages 准备SQL语句失败：{}", sqlite3_errmsg(_db));
            return {};
        }
        
        //绑定参数
        sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_STATIC);

        //执行SQL语句
        std::vector<Message> messages;
        while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
            Message message;
            message._id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            message._role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            message._content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            message._timestamp = static_cast<time_t>(sqlite3_column_int64(stmt, 3));
            messages.push_back(message);
        }
        
        if(rc != SQLITE_DONE){
            ERR("getHistroyMessages 执行SQL语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return {};
        }

        sqlite3_finalize(stmt);
        INFO("getHistroyMessages 历史消息获取成功，消息总数：{}", messages.size());
        return messages;
    }

    //删除指定会话的所有消息
    bool DataManager::deleteMessages(const std::string& sessionId){
        std::unique_lock<std::mutex> lock(_mutex);
        std::string deleteMessages = R"(
            DELETE FROM messages WHERE session_id = ?
        )";

        //准备SQL语句
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(_db, deleteMessages.c_str(), -1, &stmt, nullptr);
        if(rc != SQLITE_OK){
            ERR("deleteMessages 准备SQL语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }
        
        //绑定参数
        rc = sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_STATIC);
        
        //执行SQL语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE){
            ERR("deleteMessages 执行SQL语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        INFO("deleteMessages 消息删除成功：{}", sessionId);
        return true;
    }
}