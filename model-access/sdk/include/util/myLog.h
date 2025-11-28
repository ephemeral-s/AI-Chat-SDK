#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>
#include <mutex>
#include <memory>

namespace ephemerals{
    class Logger{
    public:
        // 初始化日志记录器
        static void initLogger(const std::string& name, const std::string& logFilePath, 
            spdlog::level::level_enum logLevel = spdlog::level::info);
        
        // 获取日志记录器
        static std::shared_ptr<spdlog::logger> getLogger();
    private:
        static std::shared_ptr<spdlog::logger> _logger;
        static std::mutex _mutex;
    private:
        Logger() = default;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
    };

    #define TRACE(format, ...) Logger::getLogger()->trace(std::string("[{:>10s}:{:<4d}]") + format, __FILE__, __LINE__, ##__VA_ARGS__)
    #define DBG(format, ...) Logger::getLogger()->debug(std::string("[{:>10s}:{:<4d}]") + format, __FILE__, __LINE__, ##__VA_ARGS__)
    #define INF(format, ...) Logger::getLogger()->info(std::string("[{:>10s}:{:<4d}]") + format, __FILE__, __LINE__, ##__VA_ARGS__)
    #define WRN(format, ...) Logger::getLogger()->warn(std::string("[{:>10s}:{:<4d}]") + format, __FILE__, __LINE__, ##__VA_ARGS__)
    #define ERR(format, ...) Logger::getLogger()->error(std::string("[{:>10s}:{:<4d}]") + format, __FILE__, __LINE__, ##__VA_ARGS__)
    #define CRIT(format, ...) Logger::getLogger()->critical(std::string("[{:>10s}:{:<4d}]") + format, __FILE__, __LINE__, ##__VA_ARGS__)
}
