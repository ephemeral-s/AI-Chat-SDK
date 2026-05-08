# ChatServer

基于 cpp-httplib 和 ai_chat_sdk 的多模型 HTTP 聊天服务，支持同步/流式对话、会话管理和多模型切换（DeepSeek、ChatGPT、Gemini、Ollama）。

## 依赖

- cpp-httplib — HTTP 服务器/客户端
- jsoncpp — JSON 序列化
- spdlog + fmt — 日志
- OpenSSL — HTTPS 支持
- gflags — 命令行参数解析
- sqlite3 — 数据存储
- ai_chat_sdk — AI 聊天 SDK

## 快速开始

### 构建

```bash
cd build
cmake ..
make -j$(nproc)
```

### 运行

```bash
# 设置 API Key（至少一个）
export DeepSeek_api_key="your-key"
# export ChatGPT_api_key="your-key"
# export Gemini_api_key="your-key"

# 启动服务
./ChatServer
```

## 配置

配置优先级：**默认值 < 配置文件 < 命令行参数**

### 命令行参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--port` | 8080 | 监听端口 |
| `--listen_addr` | 0.0.0.0 | 监听地址 |
| `--log_level` | INFO | 日志级别 |
| `--temperature` | 0.7 | 模型温度 [0, 2] |
| `--max_tokens` | 2048 | 最大生成长度 |
| `--ollama_model_name` | deepseek-r1:1.5b | Ollama 模型名称 |
| `--ollama_endpoint` | http://192.168.58.1:11434 | Ollama 访问地址 |
| `--config` | ../ChatServer.conf | 配置文件路径 |

### 配置文件

位于 `../ChatServer.conf`，使用 `key=value` 格式：

```ini
port=8080
log_level=INFO
temperature=0.7
max_tokens=2048
ollama_model_name=deepseek-r1:1.5b
ollama_endpoint=http://192.168.58.1:11434
```

### 环境变量

API Key 通过环境变量注入，不写入配置文件：

- `DeepSeek_api_key` — DeepSeek API Key
- `ChatGPT_api_key` — OpenAI API Key
- `Gemini_api_key` — Gemini API Key

## API 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| `POST` | `/api/session` | 创建会话 |
| `GET` | `/api/sessions` | 获取会话列表 |
| `GET` | `/api/models` | 获取可用模型列表 |
| `DELETE` | `/api/session/{session_id}` | 删除会话 |
| `GET` | `/api/session/{session_id}/history` | 获取会话历史 |
| `POST` | `/api/message` | 发送消息（同步返回） |
| `POST` | `/api/message/async` | 发送消息（SSE 流式返回） |

### 接口示例

**创建会话**

```bash
curl -X POST http://localhost:8080/api/session \
  -H "Content-Type: application/json" \
  -d '{"model": "deepseek-chat"}'
```

**发送消息**

```bash
curl -X POST http://localhost:8080/api/message \
  -H "Content-Type: application/json" \
  -d '{"session_id": "<session-id>", "message": "Hello"}'
```

**流式对话**

```bash
curl -N -X POST http://localhost:8080/api/message/async \
  -H "Content-Type: application/json" \
  -d '{"session_id": "<session-id>", "message": "Hello"}'
```

## 项目结构

```
ChatServer/
├── CMakeLists.txt          # CMake 构建配置
├── ChatServer.conf         # 服务配置文件
├── ChatServer.h            # 服务头文件
├── ChatServer.cpp          # 服务实现（路由、请求处理）
├── main.cpp                # 入口（CLI 解析、信号处理）
├── build/                  # 构建输出目录
├── www/                    # 静态前端资源
└── README.md
```

## 架构

```
main.cpp → ChatServer → httplib::Server ← HTTP 请求
                            │
                     ai_chat_sdk::ChatSDK
                            │
            ┌───────────────┼───────────────┐
         DeepSeek      ChatGPT/OpenAI    Gemini/Ollama
```

- `ChatServer` 封装 HTTP 路由和请求处理，通过 `ai_chat_sdk` 统一调用不同模型
- `main.cpp` 负责配置加载（配置文件 + 命令行 + 环境变量）和生命周期管理
- 支持同步（`/api/message`）和 SSE 流式（`/api/message/async`）两种响应模式
- 会话数据和消息历史由 SDK 内部管理
