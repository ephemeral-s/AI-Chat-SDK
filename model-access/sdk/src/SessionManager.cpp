#include "../include/SessionManager.h"
#include "../include/util/myLog.h"
#include <memory>
#include <sstream>
#include <iomanip>

namespace ai_chat_sdk{
    std::atomic<size_t> SessionManager::_sessionCounter(0);

    //会话id格式：session_时间戳_会话计数
    std::string SessionManager::generateSessionId(){
        _sessionCounter++;
        std::ostringstream os;
        os << "_session" << std::to_string(time(nullptr)) << "_" << std::setw(8) << std::setfill('0') << _sessionCounter;
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
        std::unique_lock<std::mutex> lock(_mutex);
        std::string sessionId = generateSessionId();
        
        auto sessionInfo = std::make_shared<SessionInfo>(modelName);
        sessionInfo->_sessionId = sessionId;
        sessionInfo->_createTime = time(nullptr);
        sessionInfo->_lastActiveTime = sessionInfo->_createTime;
        _sessions[sessionId] = sessionInfo;
        return sessionId;
    }

    //通过会话ID获取会话信息
    std::shared_ptr<SessionInfo> SessionManager::getSession(const std::string& sessionId){
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if(it != _sessions.end()){
            return it->second;
        }
        return nullptr;
    }

    //向会话添加消息
    bool SessionManager::addMessage(const std::string& sessionId, const Message& message){
        std::unique_lock<std::mutex> lock(_mutex);
        
        // 查找会话
        auto it = getSession(sessionId);
        if(!it) return false;

        // 添加消息
        Message msg(message._role, message._content);
        msg._id = generateMessageId(it->_messages.size());
        it->_messages.push_back(msg);
        it->_lastActiveTime = time(nullptr);
        INFO("Add message to session {}: {}", sessionId, msg._content);
        return true;
    }

    //获取会话的历史消息
    std::vector<Message> SessionManager::getHistroyMessages(const std::string& sessionId){
        std::unique_lock<std::mutex> lock(_mutex);
        
        // 查找会话
        auto it = getSession(sessionId);
        if(!it) return {};

        return it->_messages;
    }

    //更新会话时间戳
    void SessionManager::updateSessionTimestamp(const std::string& sessionId){
        std::unique_lock<std::mutex> lock(_mutex);
         
        // 查找会话
        auto it = getSession(sessionId);
        if(!it) return;

        it->_lastActiveTime = time(nullptr);
        INFO("Update session {} timestamp to {}", sessionId, it->_lastActiveTime);
    }

    //获取所有会话列表
    //返回的会话列表中，应按照会话时间戳进行降序排列
    std::vector<std::string> SessionManager::getSessionList(){
        std::unique_lock<std::mutex> lock(_mutex);
        std::vector<std::pair<time_t, std::shared_ptr<SessionInfo>>> tmp;
        tmp.reserve(_sessions.size());
        for(const auto& pair : _sessions){
            tmp.emplace_back(pair.second->_lastActiveTime, pair.second);
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
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if(it != _sessions.end()){
            _sessions.erase(it);
            _sessionCounter--;
            return true;
        }
        return false;
    }

    //清空所有会话
    void SessionManager::clearSessions(){
        std::unique_lock<std::mutex> lock(_mutex);
        _sessions.clear();
        _sessionCounter.store(0);
    }

    //获取会话总数
    size_t SessionManager::getSessionCount() const{
        return _sessionCounter.load();
    }
}