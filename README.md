# AI-Chat-SDK

一个使用 C++ 实现的多模型聊天 SDK，统一封装了云端大模型与本地 Ollama 模型的初始化、会话管理、消息发送、流式输出和聊天记录持久化能力。

目前项目提供静态库构建方式，核心能力集中在 `ChatSDK`，并通过 `SessionManager` + `DataManager(SQLite)` 管理会话与历史消息。

## 功能特性

- 统一的聊天接口，屏蔽不同模型提供方差异
- 支持云端模型与本地 Ollama 模型混合接入
- 支持普通问答与流式输出两种调用方式
- 支持多会话管理：创建、查询、列出、删除会话
- 基于 SQLite 持久化保存会话与消息记录
- 基于 CMake 构建，生成静态库便于集成到其他 C++ 项目中

## 当前已接入模型

根据当前代码实现，SDK 内置支持以下模型提供者：

### 云端模型

- `deepseek-chat`
- `gpt-5.4`
- `gemini-3-flash-preview`

### 本地模型

- 任意可通过 Ollama HTTP 接口访问的模型
- 例如测试代码中使用过的：`deepseek-r1:1.5b`

## 项目结构

```text
.
├── README.md
├── README.en.md
└── src
    ├── sdk
    │   ├── CMakeLists.txt
    │   ├── include
    │   │   ├── ChatSDK.h
    │   │   ├── LLMManager.h
    │   │   ├── SessionManager.h
    │   │   ├── DataManager.h
    │   │   ├── common.h
    │   │   └── ...
    │   └── src
    │       └── ...
    └── test
        ├── CMakeLists.txt
        └── testLLM.cpp
```

## 核心模块说明

### ChatSDK

对外统一入口，主要提供：

- 初始化模型：`initModels(...)`
- 创建会话：`createSession(...)`
- 获取会话：`getSession(...)`
- 获取会话列表：`getSessionList()`
- 删除会话：`deleteSession(...)`
- 查询可用模型：`getModelAvailableInfo()`
- 发送消息：`sendMessage(...)`
- 流式发送消息：`sendMessageStream(...)`

参考：`src/sdk/include/ChatSDK.h:12`

### LLMManager

负责注册模型提供者、初始化模型、转发消息请求。

参考：`src/sdk/include/LLMManager.h:9`

### SessionManager

负责会话生命周期与历史消息维护，并调用数据层进行持久化。

参考：`src/sdk/include/SessionManager.h:11`

### DataManager

基于 SQLite 管理会话和消息数据，默认数据库文件名为 `chatdb.db`。

参考：`src/sdk/include/DataManager.h:8`

### 通用数据结构

项目内定义了以下核心结构：

- `Message`
- `Config`
- `ApiConfig`
- `OllamaConfig`
- `ModelInfo`
- `SessionInfo`

参考：`src/sdk/include/common.h:7`

## 依赖项

从当前 CMake 配置可知，项目依赖以下库：

- CMake >= 3.10
- C++17
- OpenSSL
- jsoncpp
- spdlog
- fmt
- sqlite3
- gtest（仅测试使用）

参考：

- `src/sdk/CMakeLists.txt:1`
- `src/test/CMakeLists.txt:1`

## 构建 SDK

项目当前没有仓库根目录的总 CMakeLists，需分别进入 `src/sdk` 或 `src/test` 构建。

### 1. 构建静态库

```bash
cd src/sdk
mkdir -p build
cd build
cmake ..
make
```

构建完成后会生成静态库：

- `libai_chat_sdk.a`（名称取决于生成器与平台）

### 2. 安装 SDK

`src/sdk/CMakeLists.txt` 中已配置安装规则：

- 静态库安装到：`/usr/local/lib`
- 头文件安装到：`/usr/local/include/ai_chat_sdk`

安装命令：

```bash
cd src/sdk/build
sudo make install
```

## 构建测试程序

```bash
cd src/test
mkdir -p build
cd build
cmake ..
make
```

生成的测试程序为：

- `testLLM`

参考：`src/test/CMakeLists.txt:14`

## 快速上手

下面展示一个最基本的使用流程：

1. 创建 `ChatSDK`
2. 配置模型
3. 初始化模型
4. 创建会话
5. 发送消息
6. 获取回复

### 示例：接入云端模型

```cpp
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include "ChatSDK.h"

int main() {
    auto sdk = std::make_shared<ai_chat_sdk::ChatSDK>();

    auto config = std::make_shared<ai_chat_sdk::ApiConfig>();
    config->_modelName = "deepseek-chat";
    config->_apiKey = std::getenv("DeepSeek_api_key");
    config->_temperature = 0.7;
    config->_maxTokens = 2048;

    std::vector<std::shared_ptr<ai_chat_sdk::Config>> configs = {config};
    if (!sdk->initModels(configs)) {
        std::cerr << "initModels failed" << std::endl;
        return 1;
    }

    std::string sessionId = sdk->createSession("deepseek-chat");
    if (sessionId.empty()) {
        std::cerr << "createSession failed" << std::endl;
        return 1;
    }

    std::string reply = sdk->sendMessage(sessionId, "你好，请介绍一下你自己");
    std::cout << "assistant: " << reply << std::endl;
    return 0;
}
```

### 示例：接入 Ollama 本地模型

```cpp
#include <iostream>
#include <memory>
#include <vector>
#include "ChatSDK.h"

int main() {
    auto sdk = std::make_shared<ai_chat_sdk::ChatSDK>();

    auto config = std::make_shared<ai_chat_sdk::OllamaConfig>();
    config->_modelName = "deepseek-r1:1.5b";
    config->_modelDesc = "本地部署的 deepseek-r1 1.5b";
    config->_endpoint = "http://127.0.0.1:11434";
    config->_temperature = 0.7;
    config->_maxTokens = 2048;

    std::vector<std::shared_ptr<ai_chat_sdk::Config>> configs = {config};
    sdk->initModels(configs);

    std::string sessionId = sdk->createSession(config->_modelName);
    std::string reply = sdk->sendMessage(sessionId, "请简要介绍一下 C++17 的特性");

    std::cout << reply << std::endl;
    return 0;
}
```

## 流式输出示例

```cpp
auto onChunk = [](const std::string& chunk, bool last) {
    std::cout << chunk;
    if (last) {
        std::cout << std::endl;
    }
};

std::string fullReply = sdk->sendMessageStream(
    sessionId,
    "请用分点方式介绍一下 SQLite",
    onChunk
);
```

## 典型使用流程

### 1. 初始化多个模型

SDK 支持一次传入多个模型配置：

```cpp
std::vector<std::shared_ptr<ai_chat_sdk::Config>> modelConfigs = {
    deepseekConfig,
    chatgptConfig,
    geminiConfig,
    ollamaConfig
};

sdk->initModels(modelConfigs);
```

### 2. 创建指定模型的会话

```cpp
std::string sessionId = sdk->createSession("gpt-5.4");
```

### 3. 连续多轮对话

同一个 `sessionId` 下发送多条消息时，历史消息会自动参与上下文。

```cpp
sdk->sendMessage(sessionId, "你好");
sdk->sendMessage(sessionId, "继续展开刚才的话题");
```

### 4. 查看会话列表

```cpp
auto sessions = sdk->getSessionList();
for (const auto& id : sessions) {
    std::cout << id << std::endl;
}
```

### 5. 删除会话

```cpp
sdk->deleteSession(sessionId);
```

## 配置说明

### ApiConfig

用于配置通过云端 API 访问的模型：

```cpp
auto config = std::make_shared<ai_chat_sdk::ApiConfig>();
config->_modelName = "deepseek-chat";
config->_apiKey = "your-api-key";
config->_temperature = 0.7;
config->_maxTokens = 2048;
```

字段说明：

- `_modelName`：模型名称
- `_apiKey`：服务提供方 API Key
- `_temperature`：采样温度
- `_maxTokens`：最大输出长度

### OllamaConfig

用于配置本地 Ollama 模型：

```cpp
auto config = std::make_shared<ai_chat_sdk::OllamaConfig>();
config->_modelName = "deepseek-r1:1.5b";
config->_modelDesc = "本地模型";
config->_endpoint = "http://127.0.0.1:11434";
config->_temperature = 0.7;
config->_maxTokens = 2048;
```

字段说明：

- `_modelName`：模型名称
- `_modelDesc`：模型描述
- `_endpoint`：Ollama HTTP 服务地址
- `_temperature`：采样温度
- `_maxTokens`：最大输出长度

## 会话与数据持久化

SDK 当前已经集成会话与消息持久化能力：

- 创建会话时会写入数据库
- 发送消息时会保存用户消息和模型回复
- 支持重新查询历史消息
- 删除会话时会连同关联消息一起删除

默认数据库文件名：

```text
chatdb.db
```

相关实现参考：

- `src/sdk/include/SessionManager.h:11`
- `src/sdk/include/DataManager.h:8`

## 测试说明

测试代码位于：

- `src/test/testLLM.cpp:183`

当前测试示例展示了：

- 初始化 DeepSeek / ChatGPT / Gemini / Ollama 模型
- 创建会话
- 发送两轮消息
- 读取历史消息并输出

运行测试前请确保：

- 对应 API Key 已通过环境变量提供
- 本地 Ollama 服务可正常访问
- 依赖库已正确安装

测试代码中使用过的环境变量名包括：

- `DeepSeek_api_key`
- `ChatGPT_api_key`
- `Gemini_api_key`

## 常见问题

### 1. `initModels` 成功后仍然无法聊天？

请优先检查：

- 模型名称是否与当前 SDK 内置支持名称一致
- API Key 是否为空
- Ollama endpoint 是否可访问
- OpenSSL / sqlite3 / jsoncpp 等依赖是否正确链接

### 2. 历史消息保存在哪里？

默认保存在 SQLite 数据库文件 `chatdb.db` 中。

### 3. 如何新增模型提供者？

可以参考现有 Provider 的实现方式：

- `DeepSeekProvider`
- `ChatGPTProvider`
- `GeminiProvider`
- `OllamaLLMProvider`

新增 Provider 后，在 `ChatSDK` 中注册并初始化即可。

## 后续可完善方向

- 增加仓库根目录总 CMakeLists，简化整体构建流程
- 增加更完整的示例工程
- 完善单元测试与自动化测试
- 补充安装依赖说明与跨平台说明
- 提供更清晰的错误码与异常处理策略

## 参与贡献

欢迎提交 Issue 或 Pull Request 来改进这个项目。

建议贡献流程：

1. Fork 本仓库
2. 新建功能分支
3. 提交修改
4. 发起 Pull Request

## 许可证

当前仓库 README 中尚未声明许可证。如需开源分发，建议补充 LICENSE 文件并在此处说明。
