// API Configuration
const API_BASE = '';

// App State
const state = {
    currentSessionId: null,
    currentModel: '',
    models: [],
    sessions: [],
    isStreaming: false,
    abortController: null,
    connectionStatus: 'disconnected',
    selectedModelForNewSession: null
};

// DOM Elements
const elements = {
    // Sidebar
    sessionList: document.getElementById('sessionList'),
    newSessionBtn: document.getElementById('newSessionBtn'),
    currentModelDisplay: document.getElementById('currentModelDisplay'),
    connectionStatus: document.getElementById('connectionStatus'),
    connectionText: document.getElementById('connectionText'),

    // Main
    currentSessionTitle: document.getElementById('currentSessionTitle'),
    refreshModelsBtn: document.getElementById('refreshModelsBtn'),
    messageContainer: document.getElementById('messageContainer'),
    emptyState: document.getElementById('emptyState'),
    messageInput: document.getElementById('messageInput'),
    sendBtn: document.getElementById('sendBtn'),
    stopBtn: document.getElementById('stopBtn'),

    // Modal
    modelModal: document.getElementById('modelModal'),
    modelList: document.getElementById('modelList'),
    closeModalBtn: document.getElementById('closeModalBtn'),

    // Toast
    toast: document.getElementById('toast'),
    toastIcon: document.getElementById('toastIcon'),
    toastMessage: document.getElementById('toastMessage'),

    // Example questions
    exampleQuestions: document.querySelectorAll('.example-question')
};

// Utility Functions
function formatTimestamp(timestamp) {
    const date = new Date(timestamp * 1000);
    const now = new Date();
    const diff = now - date;

    if (diff < 60000) return '刚刚';
    if (diff < 3600000) return `${Math.floor(diff / 60000)} 分钟前`;
    if (diff < 86400000) return date.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' });
    if (diff < 604800000) return `${Math.floor(diff / 86400000)} 天前`;
    return date.toLocaleDateString('zh-CN');
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

function showToast(message, type = 'info') {
    const icons = {
        success: 'check_circle',
        error: 'error',
        info: 'info'
    };

    elements.toastIcon.textContent = icons[type];
    elements.toastMessage.textContent = message;
    elements.toast.className = `toast ${type}`;

    requestAnimationFrame(() => {
        elements.toast.classList.add('show');
    });

    setTimeout(() => {
        elements.toast.classList.remove('show');
    }, 3000);
}

function updateConnectionStatus(connected) {
    state.connectionStatus = connected ? 'connected' : 'disconnected';
    elements.connectionStatus.className = `status-dot ${connected ? 'connected' : ''}`;
    elements.connectionText.textContent = `连接状态：${connected ? '后端已连接' : '未连接'}`;
}

function autoResizeTextarea() {
    elements.messageInput.style.height = 'auto';
    const newHeight = Math.min(elements.messageInput.scrollHeight, 192);
    elements.messageInput.style.height = newHeight + 'px';
}

// Modal Functions
function openModelModal() {
    console.log('openModelModal called, models:', state.models.length);
    if (state.models.length === 0) {
        showToast('暂无可用模型，请先刷新模型列表', 'error');
        return;
    }
    renderModelList();
    elements.modelModal.classList.add('show');
    state.selectedModelForNewSession = null;
    console.log('Modal should be visible now');
}

function closeModelModal() {
    elements.modelModal.classList.remove('show');
    state.selectedModelForNewSession = null;
}

function renderModelList() {
    if (state.models.length === 0) {
        elements.modelList.innerHTML = '<div class="model-list-empty">暂无可用模型</div>';
        return;
    }

    elements.modelList.innerHTML = state.models.map(model => `
        <div class="model-option" data-model-name="${escapeHtml(model.name)}">
            <div class="model-option-header">
                <span class="model-option-name">${escapeHtml(model.name)}</span>
                <div class="model-option-icon">
                    <span class="material-symbols-outlined">check</span>
                </div>
            </div>
            <div class="model-option-desc">${escapeHtml(model.desc || '点击选择此模型开始对话')}</div>
        </div>
    `).join('');

    // 绑定点击事件
    document.querySelectorAll('.model-option').forEach(option => {
        option.addEventListener('click', () => {
            const modelName = option.dataset.modelName;
            selectModelAndCreateSession(modelName);
        });
    });
}

function selectModelAndCreateSession(modelName) {
    console.log('selectModelAndCreateSession called with:', modelName);
    state.selectedModelForNewSession = modelName;
    closeModelModal();

    // 确保 sessions 是数组
    const sessions = Array.isArray(state.sessions) ? state.sessions : [];

    // 检查是否已有该模型的空会话（没有消息）
    const emptySession = sessions.find(s => s.model === modelName && s.message_count === 0);
    if (emptySession) {
        // 直接跳转到空会话
        console.log('Found empty session, selecting it:', emptySession.id);
        selectSession(emptySession.id);
        showToast('已跳转到已有会话', 'success');
    } else {
        // 创建新会话
        console.log('No empty session found, creating new one');
        createNewSession(modelName);
    }
}

// API Functions
async function fetchModels() {
    try {
        const response = await fetch(`${API_BASE}/api/models`);
        const data = await response.json();

        if (data.success) {
            state.models = data.data;
            return true;
        }
        return false;
    } catch (error) {
        console.error('Failed to fetch models:', error);
        return false;
    }
}

async function createSession(modelName) {
    try {
        const response = await fetch(`${API_BASE}/api/session`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ model: modelName })
        });
        const data = await response.json();

        if (data.success) {
            return data.data;
        }
        throw new Error(data.message || '创建会话失败');
    } catch (error) {
        console.error('Failed to create session:', error);
        throw error;
    }
}

async function createNewSession(modelName) {
    try {
        const sessionData = await createSession(modelName);
        state.currentSessionId = sessionData.sessionId;
        state.currentModel = modelName;
        elements.currentSessionTitle.textContent = '新对话';

        const model = state.models.find(m => m.name === modelName);
        if (model) {
            elements.currentModelDisplay.textContent = model.desc || model.name;
        }

        // 清空消息容器，确保隐藏空状态
        elements.messageContainer.innerHTML = '';
        elements.emptyState.classList.add('hidden');

        // 刷新会话列表以高亮新会话
        await fetchSessions();

        // 确保显示聊天界面
        showChatSession();
        showToast('会话已创建', 'success');
    } catch (error) {
        console.error('createNewSession error:', error);
        showToast('创建会话失败: ' + error.message, 'error');
    }
}

async function fetchSessions() {
    try {
        const response = await fetch(`${API_BASE}/api/sessions`);
        const data = await response.json();

        if (data.success) {
            // 确保 sessions 始终是数组
            state.sessions = Array.isArray(data.data) ? data.data : [];
            renderSessionList();
            return true;
        }
        // 请求失败时也设置为空数组
        state.sessions = [];
        return false;
    } catch (error) {
        console.error('Failed to fetch sessions:', error);
        // 发生错误时设置为空数组
        state.sessions = [];
        return false;
    }
}

async function deleteSession(sessionId) {
    try {
        const response = await fetch(`${API_BASE}/api/session/${sessionId}`, {
            method: 'DELETE'
        });
        const data = await response.json();

        if (data.success) {
            // 如果删除的是当前会话，清空当前会话ID
            const wasCurrentSession = state.currentSessionId === sessionId;
            if (wasCurrentSession) {
                state.currentSessionId = null;
            }

            // 刷新会话列表
            await fetchSessions();

            // 如果删除的是当前会话，或者会话列表为空，显示空状态
            if (wasCurrentSession || state.sessions.length === 0) {
                showEmptyState();
            }

            showToast('会话已删除', 'success');
            return true;
        }
        showToast(data.message || '删除会话失败', 'error');
        return false;
    } catch (error) {
        console.error('Failed to delete session:', error);
        showToast('删除会话失败', 'error');
        return false;
    }
}

async function fetchHistory(sessionId) {
    try {
        const response = await fetch(`${API_BASE}/api/session/${sessionId}/history`);
        const data = await response.json();

        if (data.success) {
            return data.data;
        }
        throw new Error(data.message || '获取历史记录失败');
    } catch (error) {
        console.error('Failed to fetch history:', error);
        throw error;
    }
}

async function sendMessage(sessionId, message) {
    state.isStreaming = true;
    updateStreamingUI(true);

    addMessageToUI('user', message);
    const assistantMessageId = addMessageToUI('assistant', '', true);

    state.abortController = new AbortController();
    let fullResponse = '';
    let buffer = '';

    try {
        const response = await fetch(`${API_BASE}/api/message/async`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                session_id: sessionId,
                message: message
            }),
            signal: state.abortController.signal
        });

        if (!response.ok) {
            throw new Error('发送消息失败');
        }

        const reader = response.body.getReader();
        const decoder = new TextDecoder();

        while (true) {
            const { done, value } = await reader.read();

            if (done) {
                // 处理缓冲区中剩余的数据
                if (buffer.trim()) {
                    processSSELine(buffer, fullResponse, assistantMessageId);
                }
                break;
            }

            buffer += decoder.decode(value, { stream: true });
            const lines = buffer.split('\n');

            // 保留最后一个不完整的行
            buffer = lines.pop() || '';

            for (const line of lines) {
                fullResponse = processSSELine(line, fullResponse, assistantMessageId);
            }
        }

        // 流结束，移除加载状态
        finalizeAssistantMessage(assistantMessageId, fullResponse);

    } catch (error) {
        if (error.name === 'AbortError') {
            finalizeAssistantMessage(assistantMessageId, fullResponse + '\n\n[已停止生成]');
        } else {
            console.error('Failed to send message:', error);
            finalizeAssistantMessage(assistantMessageId, fullResponse + '\n\n[发送失败: ' + error.message + ']');
            showToast('发送消息失败', 'error');
        }
    } finally {
        state.isStreaming = false;
        updateStreamingUI(false);
        state.abortController = null;
        fetchSessions();
    }
}

function processSSELine(line, fullResponse, messageId) {
    const trimmedLine = line.trim();

    if (!trimmedLine) return fullResponse;

    if (trimmedLine.startsWith('data: ')) {
        const data = trimmedLine.slice(6).trim();

        if (data === '[DONE]') {
            return fullResponse;
        }

        try {
            const parsed = JSON.parse(data);
            if (typeof parsed === 'string') {
                fullResponse += parsed;
            } else if (parsed.response) {
                fullResponse += parsed.response;
            } else {
                fullResponse += data;
            }
            updateAssistantMessage(messageId, fullResponse);
        } catch (e) {
            // 不是 JSON，直接添加
            fullResponse += data;
            updateAssistantMessage(messageId, fullResponse);
        }
    }

    return fullResponse;
}

// Render Functions
function renderSessionList() {
    if (state.sessions.length === 0) {
        elements.sessionList.innerHTML = '<div class="session-list-empty">暂无会话</div>';
        return;
    }

    elements.sessionList.innerHTML = state.sessions.map(session => {
        const isActive = session.id === state.currentSessionId;
        const title = session.first_user_message || '新对话';
        const time = formatTimestamp(session.updated_at);

        return `
            <div class="session-item ${isActive ? 'active' : ''}" data-session-id="${escapeHtml(session.id)}">
                <div class="session-title">${escapeHtml(title)}</div>
                <div class="session-meta">
                    <span>${escapeHtml(session.model)}</span>
                    <span>•</span>
                    <span>${time}</span>
                    <span>•</span>
                    <span>${session.message_count} 消息</span>
                </div>
                <button class="delete-btn" data-session-id="${escapeHtml(session.id)}" title="删除会话">
                    <span class="material-symbols-outlined">delete</span>
                </button>
            </div>
        `;
    }).join('');

    // 绑定会话点击事件
    document.querySelectorAll('.session-item').forEach(item => {
        item.addEventListener('click', (e) => {
            if (!e.target.closest('.delete-btn')) {
                selectSession(item.dataset.sessionId);
            }
        });
    });

    // 绑定删除按钮事件
    document.querySelectorAll('.delete-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            e.stopPropagation();
            const sessionId = btn.dataset.sessionId;
            if (confirm('确定要删除这个会话吗？')) {
                deleteSession(sessionId);
            }
        });
    });
}

function addMessageToUI(role, content, isStreaming = false) {
    const messageId = `msg-${Date.now()}`;
    const timestamp = new Date();
    const timeStr = timestamp.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' });

    const renderedContent = role === 'assistant' ? formatMessageContent(content) : formatMessageContent(escapeHtml(content));

    const messageHtml = `
        <div class="message-block ${role}" id="${messageId}">
            <div class="message-meta">
                <span>${role === 'user' ? 'JUST NOW' : timeStr}</span>
                <span style="font-weight: 700; color: var(--accent-primary);">${role === 'user' ? 'YOU' : 'ASSISTANT'}</span>
            </div>
            <div class="message-bubble">
                ${isStreaming ? `
                    <div class="streaming-indicator">
                        <span class="material-symbols-outlined" style="animation: spin 1s linear infinite;">progress_activity</span>
                        正在生成中...
                    </div>
                ` : ''}
                <div class="message-content">${renderedContent}${isStreaming ? '<span class="streaming-cursor"></span>' : ''}</div>
            </div>
        </div>
    `;

    elements.messageContainer.insertAdjacentHTML('beforeend', messageHtml);
    scrollToBottom();

    return messageId;
}

function updateAssistantMessage(messageId, content) {
    const messageElement = document.getElementById(messageId);
    if (messageElement) {
        const contentElement = messageElement.querySelector('.message-content');
        if (contentElement) {
            contentElement.innerHTML = formatMessageContent(content) + '<span class="streaming-cursor"></span>';
        }
        scrollToBottom();
    }
}


function finalizeAssistantMessage(messageId, content) {
    const messageElement = document.getElementById(messageId);
    if (messageElement) {
        // 移除加载指示器
        const loadingIndicator = messageElement.querySelector('.streaming-indicator');
        if (loadingIndicator) {
            loadingIndicator.remove();
        }

        // 更新最终内容，使用 Markdown 解析（不转义 HTML，让 marked 处理）
        const contentElement = messageElement.querySelector('.message-content');
        if (contentElement) {
            contentElement.innerHTML = formatMessageContent(content);
        }
    }
}

function normalizeMarkdownTables(content) {
    const lines = content.split('\n');
    const normalized = [];

    const isTableSeparator = line => /^\s*\|?(\s*:?-{3,}:?\s*\|)+\s*:?-{3,}:?\s*\|?\s*$/.test(line);
    const isTableRow = line => /\|/.test(line) && /^\s*\|?.+\|.+\|?\s*$/.test(line);

    let i = 0;
    while (i < lines.length) {
        const line = lines[i];
        const nextLine = i + 1 < lines.length ? lines[i + 1] : '';
        const prevLine = normalized.length > 0 ? normalized[normalized.length - 1] : '';

        if (isTableRow(line) && isTableSeparator(nextLine)) {
            if (prevLine.trim()) {
                normalized.push('');
            }

            normalized.push(line);
            normalized.push(nextLine);
            i += 2;

            while (i < lines.length && isTableRow(lines[i])) {
                normalized.push(lines[i]);
                i += 1;
            }

            if (i < lines.length && lines[i].trim()) {
                normalized.push('');
            }
            continue;
        }

        normalized.push(line);
        i += 1;
    }

    return normalized.join('\n');
}

function formatMessageContent(content) {
    // 检查 marked.js 是否已加载
    if (typeof marked === 'undefined') {
        console.warn('marked.js not loaded, falling back to plain text');
        return escapeHtml(content).replace(/\n/g, '<br>');
    }

    try {
        const normalizedContent = normalizeMarkdownTables(content);

        // 使用 marked.js 解析 Markdown
        let html;
        if (typeof marked.parse === 'function') {
            html = marked.parse(normalizedContent);
        } else if (typeof marked === 'function') {
            html = marked(normalizedContent);
        } else {
            throw new Error('marked API not recognized');
        }
        console.log('Markdown parsed successfully');

        // 处理代码块，添加语言标签和复制按钮
        const tempDiv = document.createElement('div');
        tempDiv.innerHTML = html;

        tempDiv.querySelectorAll('table').forEach(table => {
            const wrapper = document.createElement('div');
            wrapper.className = 'table-wrapper';
            table.parentNode.insertBefore(wrapper, table);
            wrapper.appendChild(table);
        });

        tempDiv.querySelectorAll('pre').forEach((pre, index) => {
            const code = pre.querySelector('code');
            if (!code) return;

            // 获取语言类型
            let language = 'plaintext';
            const classList = code.className.split(' ');
            for (const cls of classList) {
                if (cls.startsWith('language-')) {
                    language = cls.replace('language-', '');
                    break;
                }
            }

            // 生成唯一ID
            const codeBlockId = `code-block-${Date.now()}-${index}`;

            // 设置代码块ID
            pre.id = codeBlockId;

            // 创建代码块头部
            const header = document.createElement('div');
            header.className = 'code-block-header';

            const languageSpan = document.createElement('span');
            languageSpan.className = 'code-block-language';
            languageSpan.textContent = language;

            const copyButton = document.createElement('button');
            copyButton.className = 'code-block-copy';
            copyButton.type = 'button';
            copyButton.title = '复制代码';
            copyButton.dataset.codeBlockId = codeBlockId;
            copyButton.innerHTML = '<span class="material-symbols-outlined">content_copy</span>';

            header.appendChild(languageSpan);
            header.appendChild(copyButton);

            // 将头部插入到 code 元素之前
            pre.insertBefore(header, code);

            // 应用语法高亮
            if (window.hljs) {
                hljs.highlightElement(code);
            }
        });

        return tempDiv.innerHTML;
    } catch (error) {
        console.error('Markdown parsing error:', error);
        // 如果 Markdown 解析失败，返回转义后的纯文本
        return escapeHtml(content).replace(/\n/g, '<br>');
    }
}

function setCopyButtonState(button, icon, title, copied = false) {
    button.classList.toggle('copied', copied);
    button.innerHTML = `<span class="material-symbols-outlined">${icon}</span>`;
    button.title = title;
}

function resetCopyButton(button) {
    setCopyButtonState(button, 'content_copy', '复制代码');
}

function showCopySuccess(button) {
    setCopyButtonState(button, 'check', '已复制', true);

    setTimeout(() => {
        resetCopyButton(button);
    }, 2000);
}

function fallbackCopyText(text) {
    const textarea = document.createElement('textarea');
    textarea.value = text;
    textarea.setAttribute('readonly', '');
    textarea.style.position = 'fixed';
    textarea.style.top = '0';
    textarea.style.left = '0';
    textarea.style.opacity = '0';
    textarea.style.pointerEvents = 'none';

    document.body.appendChild(textarea);

    const selection = document.getSelection();
    const originalRange = selection && selection.rangeCount > 0 ? selection.getRangeAt(0) : null;

    textarea.focus();
    textarea.select();
    textarea.setSelectionRange(0, textarea.value.length);

    let copied = false;
    try {
        copied = document.execCommand('copy');
    } finally {
        document.body.removeChild(textarea);

        if (selection) {
            selection.removeAllRanges();
            if (originalRange) {
                selection.addRange(originalRange);
            }
        }
    }

    if (!copied) {
        throw new Error('document.execCommand("copy") failed');
    }
}

async function writeTextToClipboard(text) {
    const canUseClipboardApi = window.isSecureContext && navigator.clipboard && typeof navigator.clipboard.writeText === 'function';

    if (canUseClipboardApi) {
        try {
            await navigator.clipboard.writeText(text);
            return;
        } catch (error) {
            console.warn('navigator.clipboard.writeText failed, falling back to execCommand', error);
        }
    }

    fallbackCopyText(text);
}

// 复制代码块函数
function copyCodeBlock(codeBlockId, button) {
    const pre = document.getElementById(codeBlockId);
    if (!pre) {
        console.error('Code block not found:', codeBlockId);
        showToast('复制失败', 'error');
        return;
    }

    const code = pre.querySelector('code');
    if (!code) {
        console.error('Code element not found in block:', codeBlockId);
        showToast('复制失败', 'error');
        return;
    }

    const text = code.textContent || code.innerText || '';

    writeTextToClipboard(text).then(() => {
        showCopySuccess(button);
        showToast('复制成功', 'success');
    }).catch(err => {
        console.error('Failed to copy:', err);
        resetCopyButton(button);
        showToast('复制失败', 'error');
    });
}

function scrollToBottom() {
    elements.messageContainer.scrollTop = elements.messageContainer.scrollHeight;
}

function updateStreamingUI(isStreaming) {
    if (isStreaming) {
        elements.sendBtn.disabled = true;
        elements.stopBtn.classList.add('visible');
    } else {
        elements.sendBtn.disabled = false;
        elements.stopBtn.classList.remove('visible');
    }
}

function showEmptyState() {
    elements.emptyState.classList.remove('hidden');
    elements.messageContainer.innerHTML = '';
    elements.currentSessionTitle.textContent = '请选择或创建会话';
    elements.messageInput.disabled = true;
}

function showChatSession() {
    elements.emptyState.classList.add('hidden');
    elements.messageInput.disabled = false;
    elements.messageInput.focus();
}

async function selectSession(sessionId) {
    state.currentSessionId = sessionId;
    const session = state.sessions.find(s => s.id === sessionId);

    if (session) {
        elements.currentSessionTitle.textContent = session.first_user_message || '新对话';
        state.currentModel = session.model;

        const model = state.models.find(m => m.name === session.model);
        if (model) {
            elements.currentModelDisplay.textContent = model.desc || model.name;
        }
    }

    document.querySelectorAll('.session-item').forEach(item => {
        item.classList.toggle('active', item.dataset.sessionId === sessionId);
    });

    // 清空消息容器
    elements.messageContainer.innerHTML = '';

    try {
        const history = await fetchHistory(sessionId);

        // 如果会话有消息，显示历史记录
        if (history && history.length > 0) {
            history.forEach(msg => {
                const messageId = `msg-${msg.id}`;
                const timeStr = formatTimestamp(msg.timestamp);

                const messageHtml = `
                    <div class="message-block ${msg.role}" id="${messageId}">
                        <div class="message-meta">
                            <span>${timeStr}</span>
                            <span style="font-weight: 700; color: var(--accent-primary);">${msg.role === 'user' ? 'YOU' : 'ASSISTANT'}</span>
                        </div>
                        <div class="message-bubble">
                            <div class="message-content">${formatMessageContent(msg.content)}</div>
                        </div>
                    </div>
                `;

                elements.messageContainer.insertAdjacentHTML('beforeend', messageHtml);
            });
            scrollToBottom();
        }

        showChatSession();
    } catch (error) {
        // 如果会话为空（没有历史记录），直接显示空白聊天界面
        console.log('Session is empty or history load failed:', error);
        showChatSession();
    }
}

async function handleSendMessage() {
    const message = elements.messageInput.value.trim();

    if (!message || state.isStreaming) return;

    if (!state.currentSessionId) {
        // 如果没有当前会话，打开模型选择弹窗
        openModelModal();
        return;
    }

    elements.messageInput.value = '';
    autoResizeTextarea();

    await sendMessage(state.currentSessionId, message);
}

function stopGeneration() {
    if (state.abortController) {
        state.abortController.abort();
    }
}

// Event delegation for dynamic code block copy buttons
document.addEventListener('click', (event) => {
    const copyButton = event.target.closest('.code-block-copy');
    if (!copyButton) {
        return;
    }

    const { codeBlockId } = copyButton.dataset;
    if (!codeBlockId) {
        console.error('Missing codeBlockId for copy button');
        showToast('复制失败', 'error');
        return;
    }

    copyCodeBlock(codeBlockId, copyButton);
});

elements.newSessionBtn.addEventListener('click', openModelModal);
elements.closeModalBtn.addEventListener('click', closeModelModal);

elements.modelModal.addEventListener('click', (e) => {
    if (e.target === elements.modelModal || e.target.classList.contains('modal-overlay')) {
        closeModelModal();
    }
});

elements.refreshModelsBtn.addEventListener('click', async () => {
    const success = await fetchModels();
    if (success) {
        showToast('模型列表已刷新', 'success');
        updateConnectionStatus(true);
    } else {
        showToast('刷新模型列表失败', 'error');
    }
});

elements.sendBtn.addEventListener('click', handleSendMessage);
elements.stopBtn.addEventListener('click', stopGeneration);

elements.messageInput.addEventListener('input', autoResizeTextarea);

elements.messageInput.addEventListener('keydown', (e) => {
    if (e.key === 'Enter' && !e.shiftKey) {
        e.preventDefault();
        handleSendMessage();
    }
});

// 示例问题点击
elements.exampleQuestions.forEach(question => {
    question.addEventListener('click', () => {
        const text = question.dataset.question || question.querySelector('p').textContent;
        elements.messageInput.value = text;
        autoResizeTextarea();
    });
});

// ESC 键关闭弹窗
document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && elements.modelModal.classList.contains('show')) {
        closeModelModal();
    }
});

// Initialize App
async function init() {
    try {
        await fetchModels();
        updateConnectionStatus(true);
    } catch (error) {
        updateConnectionStatus(false);
        showToast('无法连接到后端服务', 'error');
        return;
    }

    await fetchSessions();

    if (state.sessions.length > 0) {
        selectSession(state.sessions[0].id);
    }

    showToast('欢迎使用 AI Chat', 'success');
}

// 添加旋转动画
const style = document.createElement('style');
style.textContent = `
    @keyframes spin {
        from { transform: rotate(0deg); }
        to { transform: rotate(360deg); }
    }
`;
document.head.appendChild(style);

// 配置 marked.js 和 highlight.js
if (typeof marked !== 'undefined') {
    marked.setOptions({
        highlight: function(code, lang) {
            if (lang && window.hljs && hljs.getLanguage(lang)) {
                try {
                    return hljs.highlight(code, { language: lang }).value;
                } catch (e) {
                    console.error('Highlight error:', e);
                }
            }
            return code;
        },
        breaks: true,
        gfm: true
    });
    console.log('marked.js configured successfully');
} else {
    console.warn('marked.js not available');
}

// 等待 DOM 加载完成后启动应用
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
        console.log('DOM loaded, starting app');
        init();
    });
} else {
    console.log('DOM already loaded, starting app');
    init();
}
