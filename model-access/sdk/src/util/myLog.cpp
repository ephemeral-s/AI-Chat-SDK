#include "../../include/util/myLog.h"

namespace ephemerals{
    std::shared_ptr<spdlog::logger> Logger::_logger = nullptr;
    std::mutex Logger::_mutex;

    // 初始化日志记录器
    void Logger::initLogger(const std::string& name, const std::string& logFilePath, spdlog::level::level_enum logLevel){
        if(_logger == nullptr){
            std::unique_lock<std::mutex> lock(_mutex);
            if(_logger == nullptr){
                // 设置全局自动刷新级别，当日志级别大于等于 logLevel 时自动刷新
                spdlog::flush_on(logLevel);
                // 启用异步日志，由后台线程负责写入日志文件
                spdlog::init_thread_pool(32768, 1); // 队列大小 32768，线程数 1

                // 根据路径创建日志记录器
                if(logFilePath == "stdout"){
                    _logger = spdlog::stdout_color_mt(name);
                }else{
                    _logger = spdlog::basic_logger_mt<spdlog::async_factory>(name, logFilePath);
                }
            }

            // 设置格式 -- 时间(时分秒) 日志器名称 日志级别 日志消息
            _logger->set_pattern("[%H:%M:%S][%n][%-7l]%v");
            // 设置日志级别
            _logger->set_level(logLevel);
        }
    }

    // 获取日志记录器
    std::shared_ptr<spdlog::logger> Logger::getLogger(){
        return _logger;
    }
}