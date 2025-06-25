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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "audio_pipeline.h"
#include "audio_element.h"
#include "audio_thread.h"
#include "audio_recorder.h"
#include "audio_mem.h"

#include "recorder_sr.h"
#include "esp_delegate.h"
#include "esp_dispatcher.h"
#include "i2s_stream.h"
#include "raw_stream.h"
#include "esp_task_wdt.h"
#include "periph_button.h"
#include "board.h"
#include "mbedtls/base64.h"
#include "audio_processor.h"

#include "onesdk.h"
#include "sntp_example_main.h"
#include "audio_utils.h"
bool allow_stop_playing_audio = false;
// OneSDK 配置参数
#define SAMPLE_HTTP_HOST "https://iot-cn-shanghai.iot.volces.com"
#define SAMPLE_INSTANCE_ID "***"
// #define SAMPLE_MQTT_HOST "***"
#define SAMPLE_MQTT_HOST "***"
#define SAMPLE_DEVICE_NAME "***"
#define SAMPLE_DEVICE_SECRET "***"
#define SAMPLE_PRODUCT_KEY "***"
#define SAMPLE_PRODUCT_SECRET "***"


// 日志标签
static const char *TAG = "REALTIME_AUDIO";
// 全局变量
onesdk_ctx_t *onesdk_ctx;

#define STATS_TASK_PRIO        (5)
#define JOIN_EVENT_BIT         (1 << 0)
#define WAIT_DESTORY_EVENT_BIT (1 << 0)
#define WAKEUP_REC_READING     (1 << 0)

#define volc_onesdk_safe_free(x, fn) do {   \
    if (x) {                             \
        fn(x);                           \
        x = NULL;                        \
    }                                    \
} while (0)

// audio pipeline 相关初始化

typedef struct onesdk_demo_engine {
    onesdk_ctx_t *onesdk_ctx;
    recorder_pipeline_handle_t record_pipeline;
    player_pipeline_handle_t   player_pipeline;
    void                      *recorder_engine;
    EventGroupHandle_t         join_event;
    EventGroupHandle_t         wait_destory_event;
    EventGroupHandle_t         wakeup_event;
    QueueHandle_t              frame_q;
    esp_dispatcher_handle_t    esp_dispatcher;
    bool                       byte_rtc_running;
    bool                       data_proc_running;
} onesdk_demo_engine_t;

typedef struct {
    char *frame_ptr;
    int frame_len;
} frame_package_t;

static onesdk_demo_engine_t engine = {0};
static esp_err_t dispatcher_audio_play(void *instance, action_arg_t *arg, action_result_t *result)
{
    audio_tone_play((char *)arg->data);
    return ESP_OK;
};
static esp_err_t rec_engine_cb(audio_rec_evt_t *event, void *user_data)
{
    if (AUDIO_REC_WAKEUP_START == event->type) {
#if defined (CONFIG_LANGUAGE_WAKEUP_MODE)
        ESP_LOGI(TAG, "rec_engine_cb - AUDIO_REC_WAKEUP_START");
        player_pipeline_stop(engine.player_pipeline);
        action_arg_t action_arg = {0};
        action_arg.data = (void *)"spiffs://spiffs/dingding.wav";

        action_result_t result = {0};
        esp_dispatcher_execute_with_func(engine.esp_dispatcher, dispatcher_audio_play, NULL, &action_arg, &result);
        // // 用户唤醒后要等一小会在开始，否则有一段没有声音的数据被写入
        xEventGroupSetBits(engine.wakeup_event, WAKEUP_REC_READING);
#endif // CONFIG_LANGUAGE_WAKEUP_MODE
    } else if (AUDIO_REC_VAD_START == event->type) {
#if defined (CONFIG_LANGUAGE_WAKEUP_MODE)
        //TODO 大模型网关现在严格依赖顺序，必须先播放完成或者停止音频播放后，才能开始语音识别
        // 否则会导致音频数据被覆盖，导致语音识别失败
        ESP_LOGI(TAG, "rec_engine_cb - AUDIO_REC_VAD_START");
        if (allow_stop_playing_audio) {
            ESP_LOGI(TAG, "rec_engine_cb - AUDIO_REC_VAD_START, allow_stop_playing_audio is true, stop player pipeline");
            player_pipeline_stop(engine.player_pipeline);
            xEventGroupSetBits(engine.wakeup_event, WAKEUP_REC_READING);

        } else {
            // 未来支持自动停止和录音切换
            ESP_LOGI(TAG, "rec_engine_cb - AUDIO_REC_VAD_START, but allow_stop_playing_audio is false, skip stop player pipeline");
        }
        // 用户唤醒后要等一小会在开始，否则有一段没有声音的数据被写入
#endif // CONFIG_LANGUAGE_WAKEUP_MODE
    } else if (AUDIO_REC_VAD_END == event->type) {
#if defined (CONFIG_LANGUAGE_WAKEUP_MODE)
        ESP_LOGI(TAG, "rec_engine_cb - AUDIO_REC_VAD_END");
        // 重新启动播放器管道
        player_pipeline_run(engine.player_pipeline);
        // 设置commit = true，表示音频数据已经处理完毕
        ESP_LOGI(TAG, "onesdk_rt_audio_send - commit = true");
        onesdk_rt_audio_send(engine.onesdk_ctx, NULL, 0, true);
        // 防止录音，否则会出现AFE: Ringbuffer of AFE is empty, Please use feed() to write data
        if (!allow_stop_playing_audio) {// 音频下载过程中，不允许录音
            xEventGroupClearBits(engine.wakeup_event, WAKEUP_REC_READING);
        }

#endif // CONFIG_LANGUAGE_WAKEUP_MODE
    } else if (AUDIO_REC_WAKEUP_END == event->type) {
#if defined (CONFIG_LANGUAGE_WAKEUP_MODE)
        xEventGroupClearBits(engine.wakeup_event, WAKEUP_REC_READING);
        ESP_LOGI(TAG, "rec_engine_cb - AUDIO_REC_WAKEUP_END");

#endif // CONFIG_LANGUAGE_WAKEUP_MODE
    } else {
    }

    return ESP_OK;
}

static esp_err_t open_audio_pipeline()
{
    engine.record_pipeline = recorder_pipeline_open();
    engine.player_pipeline = player_pipeline_open();
    recorder_pipeline_run(engine.record_pipeline);
    player_pipeline_run(engine.player_pipeline);
    ESP_LOGI(TAG, "open_audio_pipeline success\n");
    return ESP_OK;
}

void keepalive_thread(void *arg) {
    printf("running keepalive_thread\n");
    onesdk_ctx_t *ctx = (onesdk_ctx_t *)arg;
    if (NULL == ctx) {
        printf("keepalive_thread: ctx is NULL\n");
        return;
    }
    onesdk_rt_session_keepalive(ctx);
}

void text_callback(const void *text_data, size_t text_len, void *user_data) {
    printf("Received text data: %s\n", (char *)text_data);
}

// 音频回调函数，参考byte_rtc_on_audio_data
void audio_callback(const void *audio_data, size_t audio_len, void *user_data) {
    ESP_LOGI(TAG, "audio_callback Received audio data, len: %zu", audio_len);
    // ESP_LOGD(TAG, "remote audio data %s %s %d %d %p %zu", channel, uid, sent_ts, codec, data_ptr, data_len);

    pipe_player_state_e state;
    player_pipeline_get_state(engine.player_pipeline, &state);
    if (state == PIPE_STATE_IDLE) {
        return;
    }
    frame_package_t frame = { 0 };
    frame.frame_ptr = audio_calloc(1, audio_len);
    memcpy(frame.frame_ptr, audio_data, audio_len);
    frame.frame_len = audio_len;
    // printf("audio_callback: frame_ptr: %p, frame_len: %d\n", frame.frame_ptr, frame.frame_len);
    if (xQueueSend(engine.frame_q, &frame, pdMS_TO_TICKS(10)) != pdPASS) {
        ESP_LOGW(TAG, "audio frame queue full");
    }
    // 设置 allow_stop_playing_audio = false；反之播放被后台录音中断
    ESP_LOGI(TAG, "audio_callback: allow_stop_playing_audio = false");
    allow_stop_playing_audio = false;
}


void error_callback(const char* code, const char *message, void *user_data) {
    ESP_LOGE(TAG, "Error: %s - %s", code, message);
}

void on_response_done(void *user_data) {
    ESP_LOGI(TAG, "on_response_done, allow_stop_playing_audio is %d, now set to true", allow_stop_playing_audio);
    // 可以在这里处理响应完成后的逻辑，然后才能开始录音
    // xEventGroupSetBits(engine.join_event, JOIN_EVENT_BIT);
    allow_stop_playing_audio = true;
    xEventGroupSetBits(engine.wakeup_event, WAKEUP_REC_READING);
}

static void audio_pull_data_process(char *ptr, int len)
{
    char *data_ptr = ptr;
    int data_len = len;
    /* Since OPUS is in VBR mode, it needs to be packaged into a length + data format first then to decoder*/
#if defined (CONFIG_AUDIO_SUPPORT_OPUS_DECODER)

#define frame_length_prefix (2)
    if (engine.opus_data_cache_len + frame_length_prefix < len) {
        engine.opus_data_cache = audio_realloc(engine.opus_data_cache, len + frame_length_prefix);
        engine.opus_data_cache_len = len;
    }
    engine.opus_data_cache[0] = (len >> 8) & 0xFF;
    engine.opus_data_cache[1] = len & 0xFF;
    memcpy(engine.opus_data_cache + frame_length_prefix, ptr, len);
    data_ptr = engine.opus_data_cache;
    data_len += frame_length_prefix;
#else
    data_ptr = ptr;
    data_len = len;
#endif // CONFIG_AUDIO_SUPPORT_OPUS_DECODER
    // decode 后发送给播放器
    printf("audio_pull_data_process: data_ptr: %p, data_len: %d\n", data_ptr, data_len);
    // ESP_LOGI(TAG, "remote audio data %p %d", data_ptr, data_len);
    size_t output_length = 0;
    char *output_buffer = NULL;
    int ret = onesdk_base64_decode(data_ptr, data_len, &output_buffer, &output_length);
    if (ret != 0) {
        ESP_LOGE(TAG, "onesdk_base64_decode failed");
        return;
    }
    raw_stream_write(player_pipeline_get_raw_write(engine.player_pipeline), output_buffer, output_length);
    free(output_buffer);
}



static void audio_data_process_task(void *args)
{
    frame_package_t frame = { 0 };
    engine.data_proc_running = true;
    while (engine.data_proc_running) {
        xQueueReceive(engine.frame_q, &frame, portMAX_DELAY);
        printf("audio_data_process_task: frame_ptr: %p, frame_len: %d\n", frame.frame_ptr, frame.frame_len);
        if (frame.frame_ptr) {
            audio_pull_data_process(frame.frame_ptr, frame.frame_len);
            audio_free(frame.frame_ptr);
        }
    }
    vTaskDelete(NULL);
}

static void voice_read_task(void *args)
{
    const int voice_data_read_sz = recorder_pipeline_get_default_read_size(engine.record_pipeline);
    uint8_t *voice_data = audio_calloc(1, voice_data_read_sz);
    bool runing = true;

// #if defined (CONFIG_AUDIO_SUPPORT_OPUS_DECODER)
//     audio_frame_info_t audio_frame_info = {.data_type = AUDIO_DATA_TYPE_OPUS};
// #elif defined (CONFIG_AUDIO_SUPPORT_G711A_DECODER)
//     audio_frame_info_t audio_frame_info = {.data_type = AUDIO_DATA_TYPE_PCMA};
// #else
//     audio_frame_info_t audio_frame_info = {.data_type = AUDIO_DATA_TYPE_AACLC};
// #endif // CONFIG_AUDIO_SUPPORT_OPUS_DECODER

#if defined (CONFIG_LANGUAGE_WAKEUP_MODE)
    TickType_t wait_tm = portMAX_DELAY;
#else
    TickType_t wait_tm = 0;
#endif // CONFIG_LANGUAGE_WAKEUP_MODE
    while (runing) {
        EventBits_t bits = xEventGroupWaitBits(engine.wakeup_event, WAKEUP_REC_READING , false, true, wait_tm);
    #if defined (CONFIG_LANGUAGE_WAKEUP_MODE)
        if (bits & WAKEUP_REC_READING) {
            int ret = audio_recorder_data_read(engine.recorder_engine, voice_data, voice_data_read_sz, portMAX_DELAY);
            if (ret == 0 || ret == -1) {
                xEventGroupClearBits(engine.wakeup_event,  WAKEUP_REC_READING);
            } else {
                // byte_rtc_send_audio_data(engine.engine, DEFAULT_ROOMID, voice_data, voice_data_read_sz, &audio_frame_info);
                // 音频数据线base64编码
                char *base64_buffer = NULL;
                size_t output_length = 0;
                int r = onesdk_base64_encode((const char *)voice_data, voice_data_read_sz, &base64_buffer, &output_length);
                if (r != 0) {
                    printf("onesdk_base64_encode failed: %d\n", r);
                    continue;
                }
                printf("sending audio data length2: %d, %d\n", strlen(base64_buffer), output_length);
                onesdk_rt_audio_send(engine.onesdk_ctx, (const char*)base64_buffer, output_length, false);
                free(base64_buffer);
                vTaskDelay(150 / portTICK_PERIOD_MS);
            }
        }
    #else
        audio_recorder_data_read(engine.recorder_engine, voice_data, voice_data_read_sz, portMAX_DELAY);
        // byte_rtc_send_audio_data(engine.engine, DEFAULT_ROOMID, voice_data, voice_data_read_sz, &audio_frame_info);
        char *base64_buffer = NULL;
        size_t output_length = 0;
        int r = onesdk_base64_encode((const char *)voice_data, voice_data_read_sz, &base64_buffer, &output_length);
        if (r!= 0) {
            ESP_LOGE(TAG, "onesdk_base64_encode failed: %d\n", r);
            continue;
        }
        onesdk_rt_audio_send(engine.onesdk_ctx, (const char*)base64_buffer, output_length, false);
        free(base64_buffer);
        vTaskDelay(50 / portTICK_PERIOD_MS);
    #endif
    }
    xEventGroupClearBits(engine.wakeup_event, WAKEUP_REC_READING);
    audio_free(voice_data);
    vTaskDelete(NULL);
}

static void log_clear(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("AUDIO_THREAD", ESP_LOG_ERROR);
    esp_log_level_set("i2c_bus_v2", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_HAL", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_PIPELINE", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_ELEMENT", ESP_LOG_ERROR);
    esp_log_level_set("I2S_STREAM_IDF5.x", ESP_LOG_ERROR);
    esp_log_level_set("RSP_FILTER", ESP_LOG_ERROR);
    esp_log_level_set("AUDIO_EVT", ESP_LOG_ERROR);
}


esp_err_t volc_onesdk_init(void)
{
    log_clear();
    audio_tone_init();

    engine.join_event = xEventGroupCreate();
    engine.wait_destory_event = xEventGroupCreate();
    engine.wakeup_event = xEventGroupCreate();
    engine.frame_q = xQueueCreate(30, sizeof(frame_package_t));
    engine.esp_dispatcher = esp_dispatcher_get_delegate_handle();

    // byte_onesdk_engine_create();
    open_audio_pipeline();
    engine.recorder_engine = audio_record_engine_init(engine.record_pipeline, rec_engine_cb);
    audio_thread_create(NULL, "voice_read_task", voice_read_task, (void *)NULL, 5 * 1024, 5, true, 1);
    audio_thread_create(NULL, "audio_data_process_task", audio_data_process_task, (void *)NULL, 5 * 1024, 10, true, 1);

    return ESP_OK;
}


esp_err_t volc_onesdk_deinit(void)
{
    engine.byte_rtc_running = false;
    engine.data_proc_running = false;
    xEventGroupWaitBits(engine.wait_destory_event, WAIT_DESTORY_EVENT_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

    volc_onesdk_safe_free(engine.join_event, vEventGroupDelete);
    volc_onesdk_safe_free(engine.wait_destory_event, vEventGroupDelete);
    return ESP_OK;
}




void onesdk_main() {

    // ntp 初始化
    setup_ntp_time();
    // 配置 OneSDK
    onesdk_config_t config = {0};
    iot_basic_config_t device_config = {
        .http_host = SAMPLE_HTTP_HOST,
        .instance_id = SAMPLE_INSTANCE_ID,
        .auth_type = ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED,
        .product_key = SAMPLE_PRODUCT_KEY,
        .product_secret = SAMPLE_PRODUCT_SECRET,
        .device_name = SAMPLE_DEVICE_NAME,
        .device_secret = "",
        .verify_ssl = false,
        // .ssl_ca_cert = digicert_global_root_g2,
    };
    config.device_config = &device_config;
    config.aigw_path = "/v1/realtime?model=AG-voice-chat-agent";
    config.send_ping = true;

    // 初始化 OneSDK
    onesdk_ctx = malloc(sizeof(onesdk_ctx_t));
    if (onesdk_ctx == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for onesdk_ctx");
        goto rt_exit;
    }
    onesdk_init(onesdk_ctx, &config);

    // 设置回调
    onesdk_rt_event_cb_t event_cb = {
        .on_audio = audio_callback,
        .on_text = text_callback,
        .on_error = error_callback,
        .on_response_done = on_response_done,
    };
    onesdk_rt_set_event_cb(onesdk_ctx, &event_cb);

    // 连接 OneSDK 服务
    onesdk_connect(onesdk_ctx);
    // 更新设置
    aigw_ws_session_t session = {
        .instructions = "你的名字叫火山引擎豆包，你是一个智能助手,时长尽量控制在30秒"
    };
    onesdk_rt_session_update(onesdk_ctx, &session);
    // create a keep alive thread
    // create_keep_alive_task(onesdk_ctx);
    audio_thread_t xHandle = NULL;
    audio_thread_create(&xHandle, "keepalive_thread", keepalive_thread, onesdk_ctx, 10 * 1024, 10, true, 0);
    esp_task_wdt_add((TaskHandle_t)xHandle);

    // wait 10 seconds
    // ESP_LOGI(TAG, "Waiting 10 seconds...");
    // vTaskDelay(10000 / portTICK_PERIOD_MS);
    // 初始化音频编码
    // onesdk_demo_engine_t *engine = malloc(sizeof(onesdk_demo_engine_t));
    engine.onesdk_ctx = onesdk_ctx;
    volc_onesdk_init();
    while (1)
    {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    rt_exit:
    // 释放 OneSDK 资源
    onesdk_deinit(onesdk_ctx);
    free(onesdk_ctx);
    ESP_LOGI(TAG, "OneSDK deinitialized and memory freed.");
    volc_onesdk_deinit();
    ESP_LOGI(TAG, "Volc OneSDK deinitialized and memory freed.");
}

