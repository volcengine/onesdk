// Copyright (2025) Beijing Volcano Engine Technology Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef AIGW_WS_H
#define AIGW_WS_H
#include "onesdk_config.h"

#include <libwebsockets.h>
#include "platform_thread.h"
#include "platform_compat.h"
#include <stdbool.h>
#include "iot_basic.h"
#include "aigw/llm.h"

#ifdef __cplusplus
extern "C" {
#endif

// 响应回调类型
typedef void (*aigw_ws_message_cb)(const char* message, size_t len, void* userdata);

/**
 * @brief 协议连接配置（可扩展）
 */
typedef struct {
    const char* url;              // 目标地址（ws://）
    const char* path;             // 目标路径
    const char* api_key;          // 认证密钥
    int reconnect_interval_ms;    // 断线重连间隔
    bool verify_ssl;              // 是否验证SSL证书
    const char* ca;               // CA内容
    bool send_ping;              // 是否发送ping
    int ping_interval_s;          // ping间隔
} aigw_ws_config_t;

void aigw_ws_config_deinit(aigw_ws_config_t *config);

/**
 * @brief 上下文对象（线程安全设计）
 */
typedef struct {
    struct lws_context* lws_ctx;            // LWS主上下文
    struct lws* active_conn;                // 当前活跃连接
    aigw_ws_config_t* config;               // 连接配置
    iot_basic_config_t *iot_device_config;  // 设备配置
    aigw_ws_message_cb callback;            // 消息回调
    void* userdata;                         // 用户透传数据
    struct lws_ring* send_ring;             // 环形发送缓冲区（线程安全）
    platform_mutex_t lock;                   // 连接状态锁
    volatile bool connected;                // 连接活跃标志
    volatile bool try_connect;              // 断连后尝试重连标志，在发送请求的时候判断
    uint32_t tail;
    int32_t ping_count;
} aigw_ws_ctx_t;

/**
 * @brief 可配置的上下文参数
 */
typedef enum aigw_ws_option {
    AIGW_WS_URL,
    AIGW_WS_PATH,
    AIGW_WS_API_KEY,
    AIGW_WS_RECONNECT_INTERVAL,
    AIGW_WS_VERIFY_SSL,
    AIGW_WS_SSL_CA_PATH,
    AIGW_WS_IOT_CONFIG,
} aigw_ws_option_t;

/**
 * @brief 配置上下文的参数
 * @param ctx 上下文对象
 * @param key 参数键
 * @param value 参数值
 */
void aigw_ws_set_option(aigw_ws_ctx_t *ctx, aigw_ws_option_t key, void *value);

/**
 * @brief 初始化全局LWS上下文
 * @param ctx 上下文对象指针（需预先分配内存）
 * @param config 连接配置
 * @return VOLC_OK 成功，其他为错误码
 */
int aigw_ws_init(aigw_ws_ctx_t* ctx, const aigw_ws_config_t* config);

/**
 * @brief 销毁上下文并释放资源
 */
void aigw_ws_deinit(aigw_ws_ctx_t* ctx);

/**
 * @brief 启动连接（非阻塞）
 * @note 实际连接状态通过回调通知
 */
int aigw_ws_connect(aigw_ws_ctx_t* ctx);

/**
 * @brief 主动断开连接
 */
void aigw_ws_disconnect(aigw_ws_ctx_t* ctx);

/**
 * @brief 获取大模型网关连接配置信息
 * @param aigw_ws_config 大模型网关连接配置
 * @param ctx iot上下文
 * @return 0 成功，其他失败
 */
int onesdk_iot_get_binding_aigw_info(aigw_llm_config_t *llm_config, aigw_ws_config_t * aigw_ws_config, iot_basic_ctx_t *ctx);

/**
 * @brief 发送JSON请求（线程安全）
 * @param ctx 上下文对象
 * @param json_str 符合网关协议的JSON字符串
 * @return 成功返回VOLC_OK，队列满返回VOLC_ERR_SEND_QUEUE_FULL
 */
int aigw_ws_send_request(aigw_ws_ctx_t* ctx, const char* json_str);

typedef struct {
    char* type;
    char* name;
    char* description;
    char* parameters;
} aigw_ws_session_tools_t;

typedef struct {
    const char* instructions;
    const char* voice;
    const char* input_audio_format;
    const char* output_audio_format;
    bool input_audio_transcription;
    bool modality_audio_enabled;
    bool modality_text_enabled;
    aigw_ws_session_tools_t* tools;
} aigw_ws_session_t;

#define DEFAULT_SESSION \
    { \
        .instructions = "你的名字叫豆包，你是一个智能助手，你的回答要尽量简短。", \
        .voice = "zh_female_tianmeiyueyue_moon_bigtts", \
        .input_audio_format = "pcm16", \
        .output_audio_format = "pcm16", \
        .input_audio_transcription = true, \
        .modality_audio_enabled = true, \
        .modality_text_enabled = false, \
        .tools = NULL \
    }

typedef struct {
    char* input_audio_transcription; // 例如："火山引擎智能创作平台"
    char* input_audio_translation;   // 例如："volcengine creative cloud"
} aigw_ws_glossary_item_t;

typedef struct {
    char** hot_word_list;                 // 热词列表 (字符串数组)
    size_t num_hot_words;                 // hot_word_list中的元素数量
    aigw_ws_glossary_item_t* glossary_list; // 词汇表示例列表 (结构体数组)
    size_t num_glossary_items;            // glossary_list中的元素数量
} aigw_ws_add_vocab_t;

typedef struct {
    char* source_language;              // 源语言，例如："zh"
    char* target_language;              // 目标语言，例如："en"
    aigw_ws_add_vocab_t add_vocab;      // 词汇表增强配置
} aigw_ws_input_audio_translation_config_t;

typedef struct {
    char* input_audio_format;                               // 输入音频格式，例如："pcm16"
    char** modalities;                                      // 模式列表 (字符串数组)，例如：["text"]
    size_t num_modalities;                                  // modalities数组中的元素数量
    aigw_ws_input_audio_translation_config_t input_audio_translation; // 音频翻译特定配置
} aigw_ws_translation_session_t;

/**
 * @brief 更新会话的默认配置
 * @param ctx 上下文对象
 * @param session 会话配置
 * @return VOLC_OK 成功，其他为错误码
 */
int aigw_ws_session_update(aigw_ws_ctx_t* ctx, const aigw_ws_session_t* session);

/**
 * @brief 传输音频字节到缓冲区
 * @param ctx 上下文对象
 * @param buffer 音频数据
 * @param len 数据长度
 * @return VOLC_OK 成功，其他为错误码
 */
int aigw_ws_input_audio_buffer_append(aigw_ws_ctx_t* ctx, const char* buffer, size_t len);

/**
 * @brief 提交音频缓冲区
 * @param ctx 上下文对象
 * @return VOLC_OK 成功，其他为错误码
 */
int aigw_ws_input_audio_buffer_commit(aigw_ws_ctx_t* ctx);

/**
 * @brief 更新翻译会话的默认配置
 * @param ctx 上下文对象
 * @return VOLC_OK 成功，其他为错误码
*/
int aigw_ws_translation_session_update(aigw_ws_ctx_t* ctx, const aigw_ws_translation_session_t* session);

/**
 * @brief 标记同声传译音频输入完成
 * @param ctx 上下文对象
 * @return VOLC_OK 成功，其他为错误码
 */
int aigw_ws_input_audio_done(aigw_ws_ctx_t* ctx);

/**
 * @brief 向对话的上下文中添加一个新项目，包括消息、函数调用响应。目前只支持函数调用应答。
 * @param ctx 上下文对象
 * @param func_call_id 函数调用ID
 * @return VOLC_OK 成功，其他为错误码
 */
int aigw_ws_conversation_item_create(aigw_ws_ctx_t* ctx, char* func_call_id);

/**
 * @brief 创建响应
 * @param ctx 上下文对象
 * @return VOLC_OK 成功，其他为错误码
 */
int aigw_ws_response_create(aigw_ws_ctx_t* ctx);

/**
 * @brief 取消正在回复的应答
 * @param ctx 上下文对象
 * @return VOLC_OK 成功，其他为错误码
 */
int aigw_ws_response_cancel(aigw_ws_ctx_t* ctx);

/**
 * @brief 注册响应回调
 */
void aigw_ws_register_callback(aigw_ws_ctx_t* ctx, aigw_ws_message_cb cb, void* userdata);

/**
 * @brief 运行事件循环（应在独立线程中调用）
 * @param ctx 上下文对象
 * @param timeout_ms 事件循环超时时间
 */
int aigw_ws_run_event_loop(aigw_ws_ctx_t* ctx, int timeout_ms);


typedef struct my_item {
    void *value;
    size_t len;
} my_item_t;


#ifdef __cplusplus
}
#endif

#endif // AIGW_WS_H