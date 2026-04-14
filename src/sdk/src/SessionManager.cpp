#include "../include/SessionManager.h"
#include "../include/util/myLog.h"
#include <memory>
#include <sstream>
#include <iomanip>

namespace ai_chat_sdk{
    std::atomic<size_t> SessionManager::_sessionCounter(0);

    SessionManager::SessionManager(const std::string& dbName)
        :_dataManager(dbName)
    {
        //从数据库获取所有会话
        auto sessions = _dataManager.getAllSessions();
        for(auto& session : sessions){
            _sessions[session->_sessionId] = session;
        }
    }

    //会话id格式：session_时间戳_会话计数
    std::string SessionManager::generateSessionId(){
        _sessionCounter++;
        std::ostringstream os;
        os << "session" << "_" << std::to_string(time(nullptr)) << "_" << std::setw(8) << std::setfill('0') << _sessionCounter;
        return os.str();
    }

    //消息id格式：message_时间戳_消息计数
    std::string SessionManager::generateMessageId(size_t messageCounter){
        messageCounter++;
        std::ostringstream os;
        os << "_message" << std::to_string(time(nullptr)) << "_" << std::setw(8) << std::setfill('0') << messageCounter;
        return os.str();
    }

    //新建会话（提供模型名称）
    std::string SessionManager::createSession(const std::string& modelName){
        _mutex.lock();
        std::string sessionId = generateSessionId();
        
        auto sessionInfo = std::make_shared<SessionInfo>(modelName);
        sessionInfo->_sessionId = sessionId;
        sessionInfo->_createTime = time(nullptr);
        sessionInfo->_lastActiveTime = sessionInfo->_createTime;
        _sessions[sessionId] = sessionInfo;
        _mutex.unlock(); // 注意防止死锁

        //将会话保存到数据库
        _dataManager.insertSession(*sessionInfo);
        return sessionId;
    }

    //通过会话ID获取会话信息
    std::shared_ptr<SessionInfo> SessionManager::getSession(const std::string& sessionId){
        std::unique_lock<std::mutex> lock(_mutex);
        //先在内存中查找
        auto it = _sessions.find(sessionId);
        if(it != _sessions.end()){
            it->second->_messages = _dataManager.getHistroyMessages(sessionId);
            return it->second;
        }

        //没找到，则从数据库中查找
        auto session = _dataManager.getSession(sessionId);
        if(session){
            //在内存中查找，如果没有则同步到内存
            it = _sessions.find(sessionId);
            if(it == _sessions.end()){
                _sessions[sessionId] = session;
                it = _sessions.find(sessionId); // 更新it，防止迭代器失效问题
            }
            it->second->_messages = _dataManager.getHistroyMessages(sessionId); // 赋值操作必须在临界区
            return it->second;
        }
        WRN("Session {} not found", sessionId);
        return nullptr;
    }

    //向会话添加消息
    bool SessionManager::addMessage(const std::string& sessionId, const Message& message){
        _mutex.lock();
        
        // 在内存中查找会话
        auto it = _sessions.find(sessionId);
        if(it == _sessions.end()){
            WRN("Session {} not found", sessionId);
            _mutex.unlock(); // 注意防止死锁
            return false;
        }

        // 添加消息
        Message msg(message._role, message._content);
        msg._id = generateMessageId(it->second->_messages.size());
        it->second->_messages.push_back(msg);
        it->second->_lastActiveTime = time(nullptr);
        _mutex.unlock(); // 注意防止死锁

        //将消息保存到数据库
        _dataManager.insertMessage(sessionId, msg);
        INFO("Add message to session {}: {}", sessionId, msg._content);
        return true;
    }

    //获取会话的历史消息
    std::vector<Message> SessionManager::getHistroyMessages(const std::string& sessionId){
        _mutex.lock();
        
        // 在内存中查找会话
        auto it = _sessions.find(sessionId);
        if(it != _sessions.end()){
            _mutex.unlock(); // 注意防止死锁
            return it->second->_messages;
        }

        //在数据库中查找会话
        _mutex.unlock(); // 注意防止死锁
        return _dataManager.getHistroyMessages(sessionId);
    }

    //更新会话时间戳
    void SessionManager::updateSessionTimestamp(const std::string& sessionId){
        _mutex.lock();
         
        // 在内存中查找会话
        auto it = _sessions.find(sessionId);
        if(it != _sessions.end()){
            it->second->_lastActiveTime = time(nullptr);
        }
        _mutex.unlock(); // 注意防止死锁

        //更新数据库中的会话时间戳
        _dataManager.updateSessionTimestamp(sessionId, it->second->_lastActiveTime);
    }

    //获取所有会话列表
    //返回的会话列表中，应按照会话时间戳进行降序排列
    std::vector<std::string> SessionManager::getSessionList(){
        auto sessions = _dataManager.getAllSessions();

        std::unique_lock<std::mutex> lock(_mutex);
        std::vector<std::pair<time_t, std::shared_ptr<SessionInfo>>> tmp;
        tmp.reserve(sessions.size());
        for(const auto& pair : _sessions){
            tmp.emplace_back(pair.second->_lastActiveTime, pair.second);
        }

        //将缺失的会话从数据库添加到临时列表
        for(const auto& session : sessions){
            auto it = _sessions.find(session->_sessionId);
            if(it == _sessions.end()){
                tmp.emplace_back(session->_lastActiveTime, session);
            }
        }

        std::sort(tmp.begin(), tmp.end(), [](const auto& a, const auto& b){
            return a.first > b.first;
        });

        std::vector<std::string> sessionList;
        sessionList.reserve(tmp.size());
        for(auto& pair : tmp){
            sessionList.push_back(pair.second->_sessionId);
        }
        return sessionList;
    }

    //删除会话
    bool SessionManager::deleteSession(const std::string& sessionId){
        _mutex.lock();
        auto it = _sessions.find(sessionId);
        if(it == _sessions.end()){
            _mutex.unlock(); // 注意防止死锁
            return false;
        }
        _sessions.erase(it);
        _sessionCounter--;

        //对数据库中的会话进行删除
        _mutex.unlock(); // 注意防止死锁
        _dataManager.deleteSession(sessionId);
        return true;
    }

    //清空所有会话
    void SessionManager::clearSessions(){
        _mutex.lock();
        _sessions.clear();
        _sessionCounter.store(0);
        _mutex.unlock(); // 注意防止死锁
        _dataManager.clearSessions();
    }

    //获取会话总数
    size_t SessionManager::getSessionCount() const{
        std::unique_lock<std::mutex> lock(_mutex);
        return _sessionCounter.load();
    }
}