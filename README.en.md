# AI-Chat-SDK

A C++ multi-model chat SDK that provides a unified interface for initializing cloud LLMs and local Ollama models, managing chat sessions, sending messages, streaming responses, and persisting chat history.

The current project builds as a static library. Its core entry point is `ChatSDK`, while `SessionManager` and `DataManager (SQLite)` handle session lifecycle and message persistence.

## Features

- Unified chat interface across different model providers
- Supports both cloud-hosted models and local Ollama models
- Supports both standard request/response and streaming output
- Multi-session management: create, query, list, and delete sessions
- SQLite-based persistence for sessions and message history
- CMake-based build system that produces a static library for easy C++ integration

## Supported Models

Based on the current implementation, the SDK has built-in support for the following providers.

### Cloud models

- `deepseek-chat`
- `gpt-5.4`
- `gemini-3-flash-preview`

### Local models

- Any model accessible through the Ollama HTTP API
- Example used in the test code: `deepseek-r1:1.5b`

## Project Structure

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

## Core Modules

### ChatSDK

The main public entry point. It provides:

- model initialization: `initModels(...)`
- session creation: `createSession(...)`
- session lookup: `getSession(...)`
- session listing: `getSessionList()`
- session deletion: `deleteSession(...)`
- available model query: `getModelAvailableInfo()`
- non-streaming messaging: `sendMessage(...)`
- streaming messaging: `sendMessageStream(...)`

Reference: `src/sdk/include/ChatSDK.h:12`

### LLMManager

Responsible for registering providers, initializing models, and forwarding chat requests.

Reference: `src/sdk/include/LLMManager.h:9`

### SessionManager

Manages session lifecycle, in-memory history access, and persistence through the data layer.

Reference: `src/sdk/include/SessionManager.h:11`

### DataManager

Handles session and message storage using SQLite. The default database filename is `chatdb.db`.

Reference: `src/sdk/include/DataManager.h:8`

### Common data structures

The project defines the following core types:

- `Message`
- `Config`
- `ApiConfig`
- `OllamaConfig`
- `ModelInfo`
- `SessionInfo`

Reference: `src/sdk/include/common.h:7`

## Dependencies

From the current CMake configuration, the project depends on:

- CMake >= 3.10
- C++17
- OpenSSL
- jsoncpp
- spdlog
- fmt
- sqlite3
- gtest (tests only)

Reference:

- `src/sdk/CMakeLists.txt:1`
- `src/test/CMakeLists.txt:1`

## Build the SDK

The repository currently does not include a top-level `CMakeLists.txt`, so `src/sdk` and `src/test` are built separately.

### 1. Build the static library

```bash
cd src/sdk
mkdir -p build
cd build
cmake ..
make
```

After building, the static library will be generated:

- `libai_chat_sdk.a` (actual name may vary by generator/platform)

### 2. Install the SDK

`src/sdk/CMakeLists.txt` already defines install rules:

- static library to: `/usr/local/lib`
- header files to: `/usr/local/include/ai_chat_sdk`

Install with:

```bash
cd src/sdk/build
sudo make install
```

## Build the Test Program

```bash
cd src/test
mkdir -p build
cd build
cmake ..
make
```

The generated test executable is:

- `testLLM`

Reference: `src/test/CMakeLists.txt:14`

## Quick Start

A minimal usage flow looks like this:

1. Create a `ChatSDK` instance
2. Configure one or more models
3. Initialize models
4. Create a session
5. Send a message
6. Read the reply

### Example: use a cloud model

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

    std::string reply = sdk->sendMessage(sessionId, "Hello, please introduce yourself.");
    std::cout << "assistant: " << reply << std::endl;
    return 0;
}
```

### Example: use a local Ollama model

```cpp
#include <iostream>
#include <memory>
#include <vector>
#include "ChatSDK.h"

int main() {
    auto sdk = std::make_shared<ai_chat_sdk::ChatSDK>();

    auto config = std::make_shared<ai_chat_sdk::OllamaConfig>();
    config->_modelName = "deepseek-r1:1.5b";
    config->_modelDesc = "Local deepseek-r1 1.5b model";
    config->_endpoint = "http://127.0.0.1:11434";
    config->_temperature = 0.7;
    config->_maxTokens = 2048;

    std::vector<std::shared_ptr<ai_chat_sdk::Config>> configs = {config};
    sdk->initModels(configs);

    std::string sessionId = sdk->createSession(config->_modelName);
    std::string reply = sdk->sendMessage(sessionId, "Please briefly introduce the key features of C++17.");

    std::cout << reply << std::endl;
    return 0;
}
```

## Streaming Example

```cpp
auto onChunk = [](const std::string& chunk, bool last) {
    std::cout << chunk;
    if (last) {
        std::cout << std::endl;
    }
};

std::string fullReply = sdk->sendMessageStream(
    sessionId,
    "Please explain SQLite in bullet points.",
    onChunk
);
```

## Typical Workflow

### 1. Initialize multiple models

The SDK supports passing multiple model configs at once:

```cpp
std::vector<std::shared_ptr<ai_chat_sdk::Config>> modelConfigs = {
    deepseekConfig,
    chatgptConfig,
    geminiConfig,
    ollamaConfig
};

sdk->initModels(modelConfigs);
```

### 2. Create a session for a specific model

```cpp
std::string sessionId = sdk->createSession("gpt-5.4");
```

### 3. Continue a multi-turn conversation

When multiple messages are sent under the same `sessionId`, the previous messages are automatically included in context.

```cpp
sdk->sendMessage(sessionId, "Hello");
sdk->sendMessage(sessionId, "Please continue the previous topic.");
```

### 4. List sessions

```cpp
auto sessions = sdk->getSessionList();
for (const auto& id : sessions) {
    std::cout << id << std::endl;
}
```

### 5. Delete a session

```cpp
sdk->deleteSession(sessionId);
```

## Configuration

### ApiConfig

Used for models accessed through cloud APIs:

```cpp
auto config = std::make_shared<ai_chat_sdk::ApiConfig>();
config->_modelName = "deepseek-chat";
config->_apiKey = "your-api-key";
config->_temperature = 0.7;
config->_maxTokens = 2048;
```

Fields:

- `_modelName`: model name
- `_apiKey`: provider API key
- `_temperature`: sampling temperature
- `_maxTokens`: max output length

### OllamaConfig

Used for local Ollama-based models:

```cpp
auto config = std::make_shared<ai_chat_sdk::OllamaConfig>();
config->_modelName = "deepseek-r1:1.5b";
config->_modelDesc = "local model";
config->_endpoint = "http://127.0.0.1:11434";
config->_temperature = 0.7;
config->_maxTokens = 2048;
```

Fields:

- `_modelName`: model name
- `_modelDesc`: model description
- `_endpoint`: Ollama HTTP service address
- `_temperature`: sampling temperature
- `_maxTokens`: max output length

## Sessions and Persistence

The SDK already includes session and message persistence:

- sessions are written to the database when created
- user messages and model replies are saved automatically
- chat history can be queried again later
- deleting a session also removes its associated messages

Default database filename:

```text
chatdb.db
```

Related implementation:

- `src/sdk/include/SessionManager.h:11`
- `src/sdk/include/DataManager.h:8`

## Testing

Test code is located at:

- `src/test/testLLM.cpp:183`

The current test demonstrates:

- initializing DeepSeek / ChatGPT / Gemini / Ollama models
- creating a session
- sending two rounds of messages
- reading and printing chat history

Before running the test, make sure:

- the required API keys are provided through environment variables
- the local Ollama service is reachable
- all dependencies are installed correctly

Environment variable names used in the test include:

- `DeepSeek_api_key`
- `ChatGPT_api_key`
- `Gemini_api_key`

## FAQ

### 1. `initModels` succeeds, but chatting still does not work

Check the following first:

- whether the model name matches one of the names supported by the SDK
- whether the API key is empty
- whether the Ollama endpoint is reachable
- whether dependencies such as OpenSSL / sqlite3 / jsoncpp are linked correctly

### 2. Where is chat history stored?

By default, it is stored in the SQLite database file `chatdb.db`.

### 3. How do I add a new model provider?

You can follow the existing provider implementations:

- `DeepSeekProvider`
- `ChatGPTProvider`
- `GeminiProvider`
- `OllamaLLMProvider`

After adding a new provider, register and initialize it in `ChatSDK`.

## Possible Next Improvements

- add a top-level `CMakeLists.txt` to simplify building the whole repository
- add a more complete example project
- improve unit and automated test coverage
- document dependency installation and cross-platform usage
- provide clearer error codes and exception strategies

## Contribution

Issues and pull requests are welcome.

Suggested workflow:

1. Fork this repository
2. Create a feature branch
3. Commit your changes
4. Open a pull request

## License

The repository does not currently declare a license in the README. If you plan to distribute it as open source, consider adding a LICENSE file and documenting it here.
