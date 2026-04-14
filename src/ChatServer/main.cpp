#include "ChatServer.h"

#include <gflags/gflags.h>
#include <spdlog/common.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <csignal>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {
constexpr const char* kVersion = "1.0.0";
constexpr const char* kDefaultConfigPath = "../ChatServer.conf";

// CLI 参数定义：支持默认值、配置文件覆盖、命令行覆盖三层合并。
DEFINE_string(config, kDefaultConfigPath, "Path to key=value config file.");
DEFINE_int32(port, 8080, "Server listen port.");
DEFINE_string(log_level, "INFO", "Log level: TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL, OFF.");
DEFINE_string(listen_addr, "0.0.0.0", "Server listen address.");
DEFINE_double(temperature, 0.7, "Model temperature, valid range [0, 2].");
DEFINE_int32(max_tokens, 2048, "Maximum generated token count, must be > 0.");
DEFINE_string(ollama_model_name, "deepseek-r1:1.5b", "Ollama model name.");
DEFINE_string(ollama_model_desc, "DeepSeek R1 1.5B via Ollama", "Ollama model description.");
DEFINE_string(ollama_endpoint, "http://192.168.58.1:11434", "Ollama endpoint URL.");
DEFINE_bool(chatserver_help, false, "Show ChatServer help.");
DEFINE_bool(chatserver_version, false, "Show ChatServer version.");

std::atomic<bool> g_signalRequested{false};
std::condition_variable g_signalCv;
std::mutex g_signalMutex;

std::string trim(const std::string& input){
    const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if(begin >= end){
        return "";
    }
    return std::string(begin, end);
}

std::string toLower(std::string value){
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string toUpper(std::string value){
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

bool isFlagPresent(int argc, char** argv, const std::string& name){
    const std::string prefix = "--" + name + "=";
    for(int index = 1; index < argc; ++index){
        const std::string arg = argv[index];
        if(arg == "--" + name || arg.rfind(prefix, 0) == 0){
            return true;
        }
    }
    return false;
}

std::string detectConfigPath(int argc, char** argv){
    for(int index = 1; index < argc; ++index){
        const std::string arg = argv[index];
        if(arg.rfind("--config=", 0) == 0){
            return arg.substr(std::string("--config=").size());
        }
        if(arg == "--config" && index + 1 < argc){
            return argv[index + 1];
        }
    }
    return kDefaultConfigPath;
}

bool normalizeSpecialFlags(int argc, char** argv){
    // 将常见的 -h/-v 形式提前转换成自定义 flag，避免落入 gflags 默认帮助输出。
    for(int index = 1; index < argc; ++index){
        std::string arg = argv[index];
        if(arg == "-h" || arg == "--help"){
            argv[index] = const_cast<char*>("--chatserver_help");
        }else if(arg == "-v" || arg == "--version"){
            argv[index] = const_cast<char*>("--chatserver_version");
        }else if(arg == "--help=true" || arg == "--help=1"){
            argv[index] = const_cast<char*>("--chatserver_help=true");
        }else if(arg == "--version=true" || arg == "--version=1"){
            argv[index] = const_cast<char*>("--chatserver_version=true");
        }
    }
    return true;
}

std::map<std::string, std::string> readConfigFile(const std::string& path, std::string& error){
    // 配置文件采用简单 key=value 格式，方便直接解析且不引入额外依赖。
    std::map<std::string, std::string> configValues;
    std::ifstream input(path);
    if(!input.is_open()){
        error = "failed to open config file: " + path;
        return configValues;
    }

    std::string line;
    int lineNumber = 0;
    while(std::getline(input, line)){
        ++lineNumber;
        std::string content = trim(line);
        if(content.empty() || content[0] == '#'){
            continue;
        }

        const auto separator = content.find('=');
        if(separator == std::string::npos){
            error = "invalid config line " + std::to_string(lineNumber) + ": " + content;
            configValues.clear();
            return configValues;
        }

        std::string key = trim(content.substr(0, separator));
        std::string value = trim(content.substr(separator + 1));
        if(key.empty()){
            error = "empty config key at line " + std::to_string(lineNumber);
            configValues.clear();
            return configValues;
        }

        configValues[key] = value;
    }

    return configValues;
}

bool applyConfigDefaults(const std::map<std::string, std::string>& configValues, std::string& error){
    // 只把配置文件内容作为默认值注入，命令行显式传参仍然拥有最高优先级。
    static const std::map<std::string, std::string> flagMap = {
        {"port", "port"},
        {"log_level", "log_level"},
        {"listen_addr", "listen_addr"},
        {"temperature", "temperature"},
        {"max_tokens", "max_tokens"},
        {"ollama_model_name", "ollama_model_name"},
        {"ollama_model_desc", "ollama_model_desc"},
        {"ollama_endpoint", "ollama_endpoint"}
    };

    for(const auto& entry : configValues){
        auto it = flagMap.find(entry.first);
        if(it == flagMap.end()){
            error = "unsupported config key: " + entry.first;
            return false;
        }
        if(gflags::SetCommandLineOptionWithMode(it->second.c_str(), entry.second.c_str(), gflags::SET_FLAGS_DEFAULT).empty()){
            error = "invalid value for config key " + entry.first + ": " + entry.second;
            return false;
        }
    }
    return true;
}

bool parseLogLevel(const std::string& input, spdlog::level::level_enum& level){
    static const std::map<std::string, spdlog::level::level_enum> levels = {
        {"trace", spdlog::level::trace},
        {"debug", spdlog::level::debug},
        {"info", spdlog::level::info},
        {"warn", spdlog::level::warn},
        {"warning", spdlog::level::warn},
        {"error", spdlog::level::err},
        {"err", spdlog::level::err},
        {"critical", spdlog::level::critical},
        {"off", spdlog::level::off}
    };

    auto it = levels.find(toLower(trim(input)));
    if(it == levels.end()){
        return false;
    }
    level = it->second;
    return true;
}

bool validateFlags(std::string& error, spdlog::level::level_enum& logLevel){
    // 在构造最终 ServerConfig 前集中校验，避免带着非法配置进入运行期。
    if(FLAGS_temperature < 0.0 || FLAGS_temperature > 2.0){
        error = "temperature must be in [0, 2]";
        return false;
    }
    if(FLAGS_max_tokens <= 0){
        error = "max_tokens must be greater than 0";
        return false;
    }
    if(FLAGS_port <= 0 || FLAGS_port > 65535){
        error = "port must be in range 1-65535";
        return false;
    }
    if(trim(FLAGS_listen_addr).empty()){
        error = "listen_addr must not be empty";
        return false;
    }
    if(trim(FLAGS_ollama_model_name).empty()){
        error = "ollama_model_name must not be empty";
        return false;
    }
    if(trim(FLAGS_ollama_model_desc).empty()){
        error = "ollama_model_desc must not be empty";
        return false;
    }
    if(trim(FLAGS_ollama_endpoint).empty()){
        error = "ollama_endpoint must not be empty";
        return false;
    }
    if(!parseLogLevel(FLAGS_log_level, logLevel)){
        error = "log_level must be one of TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL, OFF";
        return false;
    }
    return true;
}

std::string buildHelpText(const char* programName){
    // 帮助文本独立构造，确保 parse 前后的帮助分支都能复用同一份输出。
    std::ostringstream out;
    out << "ChatServer " << kVersion << "\n\n";
    out << "功能简介\n";
    out << "  基于现有 ChatServer + ai_chat_sdk 启动 HTTP 聊天服务，提供模型列表、会话管理、历史消息与消息发送接口。\n\n";
    out << "使用方式\n";
    out << "  " << programName << " [options]\n\n";
    out << "参数说明\n";
    out << "  -h, --help                 显示帮助信息\n";
    out << "  -v, --version              显示版本信息\n";
    out << "      --config               配置文件路径，默认 ../../ChatServer.conf\n";
    out << "      --port                 监听端口，默认 8080\n";
    out << "      --log_level            日志级别：TRACE/DEBUG/INFO/WARN/ERROR/CRITICAL/OFF\n";
    out << "      --listen_addr          监听地址，默认 0.0.0.0\n";
    out << "      --temperature          温度参数，默认 0.7，范围 [0, 2]\n";
    out << "      --max_tokens           最大生成长度，默认 2048\n";
    out << "      --ollama_model_name    Ollama 模型名称，默认 deepseek-r1:1.5b\n";
    out << "      --ollama_model_desc    Ollama 模型描述\n";
    out << "      --ollama_endpoint      Ollama 访问地址，默认 http://192.168.58.1:11434\n\n";
    out << "配置优先级\n";
    out << "  默认值 -> 配置文件 -> 命令行参数\n\n";
    out << "环境变量\n";
    out << "  DeepSeek_api_key / ChatGPT_api_key / Gemini_api_key\n";
    out << "  API key 不写入配置文件，由运行环境提供。\n\n";
    out << "接口说明\n";
    out << "  GET    /api/models\n";
    out << "  POST   /api/session\n";
    out << "  GET    /api/sessions\n";
    out << "  DELETE /api/session/{session_id}\n";
    out << "  GET    /api/session/{session_id}/history\n";
    out << "  POST   /api/message\n";
    out << "  POST   /api/message/async\n";
    return out.str();
}

void handleSignal(int){
    // 信号处理函数只做最小动作：设置退出标记并唤醒等待中的主线程。
    g_signalRequested.store(true);
    g_signalCv.notify_all();
}
}

int main(int argc, char** argv){
    // 先处理帮助/版本短参数，保证后续统一按自定义 flag 流程执行。
    normalizeSpecialFlags(argc, argv);

    if(isFlagPresent(argc, argv, "chatserver_help")){
        std::cout << buildHelpText(argv[0]) << std::endl;
        return 0;
    }

    if(isFlagPresent(argc, argv, "chatserver_version")){
        std::cout << kVersion << std::endl;
        return 0;
    }

    const std::string configPath = detectConfigPath(argc, argv);
    if(!configPath.empty()){
        // 在正式 parse 之前先加载配置文件默认值，形成“默认值 -> 配置文件 -> 命令行”的覆盖顺序。
        std::string configError;
        const auto configValues = readConfigFile(configPath, configError);
        if(!configError.empty()){
            std::cerr << "Config error: " << configError << std::endl;
            return 1;
        }
        if(!applyConfigDefaults(configValues, configError)){
            std::cerr << "Config error: " << configError << std::endl;
            return 1;
        }
        if(!isFlagPresent(argc, argv, "config")){
            gflags::SetCommandLineOptionWithMode("config", configPath.c_str(), gflags::SET_FLAGS_DEFAULT);
        }
    }

    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if(FLAGS_chatserver_help){
        std::cout << buildHelpText(argv[0]) << std::endl;
        return 0;
    }

    if(FLAGS_chatserver_version){
        std::cout << kVersion << std::endl;
        return 0;
    }

    spdlog::level::level_enum logLevel;
    std::string validationError;
    if(!validateFlags(validationError, logLevel)){
        std::cerr << "Invalid argument: " << validationError << std::endl;
        std::cerr << "Use --help to see available options." << std::endl;
        return 1;
    }

    ephemerals::Logger::initLogger("ChatServer", "stdout", logLevel);

    // 经过校验和归一化后的 flag 在这里汇总成运行期配置对象。
    ai_chat_server::ServerConfig config;
    config.host = trim(FLAGS_listen_addr);
    config.port = FLAGS_port;
    config.loglevel = toUpper(trim(FLAGS_log_level));
    config.temperature = FLAGS_temperature;
    config.max_tokens = FLAGS_max_tokens;
    config.Ollama_Model_Name = trim(FLAGS_ollama_model_name);
    config.Ollama_Model_Desc = trim(FLAGS_ollama_model_desc);
    config.Ollama_Endpoint = trim(FLAGS_ollama_endpoint);

    ai_chat_server::ChatServer server(config);

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    std::mutex stateMutex;
    bool serverExited = false;
    bool serverStartResult = false;

    std::thread serverThread([&]() {
        // start() 会阻塞到服务退出，因此放到独立线程执行。
        serverStartResult = server.start();
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            serverExited = true;
        }
        g_signalCv.notify_all();
    });

    {
        // 主线程在这里等待两类事件：收到退出信号，或 server.start() 提前返回。
        std::unique_lock<std::mutex> lock(g_signalMutex);
        g_signalCv.wait(lock, [&]() {
            std::lock_guard<std::mutex> stateLock(stateMutex);
            return g_signalRequested.load() || serverExited;
        });
    }

    {
        // 只有在服务仍在运行时才主动 stop()，避免和已退出的 serverThread 重复收尾。
        std::lock_guard<std::mutex> lock(stateMutex);
        if(g_signalRequested.load() && !serverExited){
            INFO("received shutdown signal");
            server.stop();
        }
    }

    serverThread.join();
    return serverStartResult ? 0 : 1;
}
