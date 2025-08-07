# OneSDK API Documentation

## Overview

OneSDK is a comprehensive C library that provides unified access to Volcengine Edge AI Gateway and IoT Platform services, including AI capabilities, real-time communication, and IoT device management. This document provides detailed API reference for all OneSDK functions and data structures.

## Table of Contents

1. [Core API](#core-api)
2. [AI Chat API](#ai-chat-api)
3. [Real-time AI API](#real-time-ai-api)
4. [IoT API](#iot-api)
5. [Configuration](#configuration)
6. [Error Handling](#error-handling)
7. [Examples](#examples)

## Core API

### Initialization and Lifecycle

#### `onesdk_init`
```c
int onesdk_init(onesdk_ctx_t *ctx, const onesdk_config_t *config);
```
Initializes the OneSDK context with the provided configuration.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `config`: Pointer to the configuration structure

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_connect`
```c
int onesdk_connect(onesdk_ctx_t *ctx);
```
Establishes connection to the Volcengine IoT Platform.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_disconnect`
```c
int onesdk_disconnect(onesdk_ctx_t *ctx);
```
Disconnects from the Volcengine IoT Platform.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_deinit`
```c
int onesdk_deinit(const onesdk_ctx_t *ctx);
```
Deinitializes the OneSDK context and frees allocated resources.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_fetch_config`
```c
int onesdk_fetch_config(onesdk_ctx_t *ctx);
```
Fetches configuration from the IoT platform.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure

**Returns:**
- `0` on success
- Negative value on error

## AI Chat API

*Available when `ONESDK_ENABLE_AI` is defined*

### Chat Functions

#### `onesdk_chat`
```c
int onesdk_chat(onesdk_ctx_t *ctx, onesdk_chat_request_t *request, 
                char *output, size_t *output_len);
```
Sends a chat request and receives the response.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `request`: Pointer to the chat request structure
- `output`: Buffer to store the response
- `output_len`: Pointer to the output buffer length

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_chat_stream`
```c
int onesdk_chat_stream(onesdk_ctx_t *ctx, onesdk_chat_request_t *request, 
                      onesdk_chat_callbacks_t *cbs);
```
Sends a streaming chat request with callbacks.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `request`: Pointer to the chat request structure
- `cbs`: Pointer to the callback structure

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_chat_set_callbacks`
```c
void onesdk_chat_set_callbacks(onesdk_ctx_t *ctx, onesdk_chat_callbacks_t *cbs, 
                              void *user_data);
```
Sets the chat callbacks for streaming responses.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `cbs`: Pointer to the callback structure
- `user_data`: User-defined data passed to callbacks

#### `onesdk_chat_set_functioncall_data`
```c
void onesdk_chat_set_functioncall_data(onesdk_ctx_t *ctx, 
                                      onesdk_chat_response_t **response_out);
```
Sets function call data for the chat response.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `response_out`: Pointer to receive the response structure

#### `onesdk_chat_wait`
```c
int onesdk_chat_wait(onesdk_ctx_t *ctx);
```
Waits for an asynchronous chat request to complete.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_chat_release_context`
```c
void onesdk_chat_release_context(onesdk_ctx_t *ctx);
```
Releases the chat context and frees associated resources.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure

## Real-time AI API

*Available when `ONESDK_ENABLE_AI_REALTIME` is defined*

### Real-time Session Management

#### `onesdk_rt_set_event_cb`
```c
int onesdk_rt_set_event_cb(onesdk_ctx_t *ctx, onesdk_rt_event_cb_t *cb);
```
Sets the real-time event callbacks.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `cb`: Pointer to the event callback structure

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_rt_session_keepalive`
```c
int onesdk_rt_session_keepalive(onesdk_ctx_t *ctx);
```
Sends keepalive messages to maintain the real-time session.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure

**Returns:**
- `0` on success
- Negative value on error

### Chat Agent Functions

#### `onesdk_rt_session_update`
```c
int onesdk_rt_session_update(onesdk_ctx_t *ctx, aigw_ws_session_t *session);
```
Updates the real-time chat session configuration.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `session`: Pointer to the session configuration

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_rt_audio_send`
```c
int onesdk_rt_audio_send(onesdk_ctx_t *ctx, const char *audio_data, 
                        size_t len, bool commit);
```
Sends audio data to the real-time chat agent.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `audio_data`: Pointer to audio data buffer
- `len`: Length of audio data
- `commit`: Whether to commit the audio data

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_rt_audio_response_cancel`
```c
int onesdk_rt_audio_response_cancel(onesdk_ctx_t *ctx);
```
Cancels the current audio response.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure

**Returns:**
- `0` on success
- Negative value on error

### Translation Agent Functions

#### `onesdk_rt_translation_session_update`
```c
int onesdk_rt_translation_session_update(onesdk_ctx_t *ctx, 
                                        aigw_ws_translation_session_t *session);
```
Updates the real-time translation session configuration.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `session`: Pointer to the translation session configuration

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_rt_translation_audio_send`
```c
int onesdk_rt_translation_audio_send(onesdk_ctx_t *ctx, const char *audio_data, 
                                    size_t len, bool commit);
```
Sends audio data to the real-time translation agent.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `audio_data`: Pointer to audio data buffer
- `len`: Length of audio data
- `commit`: Whether to commit the audio data

**Returns:**
- `0` on success
- Negative value on error

## IoT API

*Available when `ONESDK_ENABLE_IOT` is defined*

### IoT Keepalive

#### `onesdk_iot_keepalive`
```c
int onesdk_iot_keepalive(onesdk_ctx_t *ctx);
```
Sends keepalive messages to maintain the IoT connection.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure

**Returns:**
- `0` on success
- Negative value on error

### Log Upload

#### `onesdk_iot_enable_log_upload`
```c
int onesdk_iot_enable_log_upload(onesdk_ctx_t *ctx);
```
Enables log upload functionality.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure

**Returns:**
- `0` on success
- Negative value on error

### OTA (Over-the-Air) Update

#### `onesdk_iot_enable_ota`
```c
int onesdk_iot_enable_ota(onesdk_ctx_t *ctx, const char *download_dir);
```
Enables OTA update functionality.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `download_dir`: Directory to store downloaded firmware

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_iot_ota_set_device_module_info`
```c
int onesdk_iot_ota_set_device_module_info(onesdk_ctx_t *ctx, 
                                         const char *module_name, 
                                         const char *module_version);
```
Sets device module information for OTA updates.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `module_name`: Name of the module
- `module_version`: Version of the module

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_iot_ota_set_download_complete_cb`
```c
void onesdk_iot_ota_set_download_complete_cb(onesdk_ctx_t *ctx, 
                                            iot_ota_download_complete_callback *cb);
```
Sets the callback function for OTA download completion.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `cb`: Pointer to the callback function

#### `onesdk_iot_ota_report_install_status`
```c
int onesdk_iot_ota_report_install_status(onesdk_ctx_t *ctx, char *jobID, 
                                        ota_upgrade_device_status_enum_t status);
```
Reports the OTA installation status.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `jobID`: OTA job ID
- `status`: Installation status

**Returns:**
- `0` on success
- Negative value on error

### Thing Model

#### `onesdk_iot_tm_init`
```c
int onesdk_iot_tm_init(onesdk_ctx_t *ctx);
```
Initializes the thing model functionality.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_iot_tm_deinit`
```c
void onesdk_iot_tm_deinit(onesdk_ctx_t *ctx);
```
Deinitializes the thing model functionality.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure

#### `onesdk_iot_tm_set_recv_cb`
```c
int onesdk_iot_tm_set_recv_cb(onesdk_ctx_t *ctx, iot_tm_recv_handler_t *cb, 
                             void *user_data);
```
Sets the thing model receive callback.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `cb`: Pointer to the receive handler
- `user_data`: User-defined data

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_iot_get_tm_handler`
```c
iot_tm_handler_t* onesdk_iot_get_tm_handler(onesdk_ctx_t *ctx);
```
Gets the thing model handler.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure

**Returns:**
- Pointer to the thing model handler

#### `onesdk_iot_tm_custom_topic_sub`
```c
int onesdk_iot_tm_custom_topic_sub(onesdk_ctx_t *ctx, const char *topic_suffix);
```
Subscribes to a custom topic.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `topic_suffix`: Topic suffix to subscribe to

**Returns:**
- `0` on success
- Negative value on error

#### `onesdk_iot_tm_custom_topic_pub`
```c
int onesdk_iot_tm_custom_topic_pub(onesdk_ctx_t *ctx, const char* topic_suffix, 
                                  const char *payload);
```
Publishes to a custom topic.

**Parameters:**
- `ctx`: Pointer to the OneSDK context structure
- `topic_suffix`: Topic suffix to publish to
- `payload`: Message payload

**Returns:**
- `0` on success
- Negative value on error

## Configuration

### Data Structures

#### `onesdk_config_t`
```c
typedef struct onesdk_config {
    iot_basic_config_t *device_config;
#ifdef ONESDK_ENABLE_IOT
    iot_mqtt_config_t* mqtt_config;
#endif
#if defined(ONESDK_ENABLE_AI) || defined(ONESDK_ENABLE_AI_REALTIME)
    aigw_llm_config_t *aigw_llm_config;
#endif
#ifdef ONESDK_ENABLE_AI_REALTIME
    const char* aigw_path;
    bool send_ping;
    int ping_interval_s;
#endif
} onesdk_config_t;
```

#### `onesdk_ctx_t`
```c
typedef struct {
    onesdk_config_t *config;
    iot_basic_ctx_t *iot_basic_ctx;
    void* user_data;
#ifdef ONESDK_ENABLE_AI
    onesdk_chat_context_t *chat_ctx;
#endif
#ifdef ONESDK_ENABLE_AI_REALTIME
    aigw_ws_ctx_t *aigw_ws_ctx;
    onesdk_rt_event_cb_t *rt_event_cb;
    char *rt_json_buf;
    size_t rt_json_buf_size;
    size_t rt_json_buf_capacity;
    char *rt_json_pos;
#endif
#ifdef ONESDK_ENABLE_IOT
    iot_mqtt_ctx_t *iot_mqtt_ctx;
    bool iot_log_upload_switch;
    log_handler_t *iot_log_handler;
    bool iot_ota_switch;
    iot_ota_handler_t *iot_ota_handler;
    bool iot_tm_switch;
    iot_tm_handler_t *iot_tm_handler;
#endif
} onesdk_ctx_t;
```

#### `onesdk_rt_event_cb_t`
```c
typedef struct {
    void (*on_audio)(const void *audio_data, size_t audio_len, void *user_data);
    void (*on_text)(const void *text_data, size_t text_len, void *user_data);
    void (*on_error)(const char* error_code, const char *error_msg, void *user_data);
    void (*on_transcript_text)(const void *transcript_data, size_t transcript_len, void *user_data);
    void (*on_translation_text)(const void *translation_data, size_t translation_len, void *user_data);
    void (*on_response_done)(void *user_data);
} onesdk_rt_event_cb_t;
```

### OTA Status Enumeration

```c
typedef enum ota_upgrade_device_status_enum {
    OTA_UPGRADE_DEVICE_STATUS_SUCCESS = 0,
    OTA_UPGRADE_DEVICE_STATUS_FAILED = 1,
    OTA_UPGRADE_DEVICE_STATUS_DOWNLOADING = 2,
} ota_upgrade_device_status_enum_t;
```

## Error Handling

OneSDK uses integer return codes to indicate success or failure:

- `0`: Success
- Negative values: Error codes (see `error_code.h` for specific error definitions)

## Examples

### Basic Chat Example
```c
#include "onesdk.h"

int main() {
    // Initialize device configuration
    iot_basic_config_t device_config = {
        .http_host = "https://iot-cn-shanghai.iot.volces.com",
        .instance_id = "your_instance_id",
        .auth_type = ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED,
        .product_key = "your_product_key",
        .product_secret = "your_product_secret",
        .device_name = "your_device_name",
        .verify_ssl = true,
        .ssl_ca_cert = "your_ssl_ca_cert",
    };
    
    // Initialize OneSDK configuration
    onesdk_config_t *config = malloc(sizeof(onesdk_config_t));
    if (NULL == config) {
        return -1;
    }
    memset(config, 0, sizeof(onesdk_config_t));
    config->device_config = &device_config;
    
    // Initialize OneSDK context
    onesdk_ctx_t *ctx = malloc(sizeof(onesdk_ctx_t));
    if (NULL == ctx) {
        free(config);
        return -1;
    }
    memset(ctx, 0, sizeof(onesdk_ctx_t));
    
    // Initialize OneSDK
    if (onesdk_init(ctx, config) != 0) {
        printf("Failed to initialize OneSDK\n");
        free(ctx);
        free(config);
        return -1;
    }
    
    // Connect to platform
    if (onesdk_connect(ctx) != 0) {
        printf("Failed to connect\n");
        onesdk_deinit(ctx);
        free(ctx);
        free(config);
        return -1;
    }
    
    // Send chat request
    onesdk_chat_request_t request = {
        .model = "Doubao-1.5-pro-256k",
        .messages = NULL,
        .messages_count = 0,
        .stream = false,
        .temperature = 0.7,
    };
    char output[4096] = {0};
    size_t output_len = sizeof(output);
    
    if (onesdk_chat(ctx, &request, output, &output_len) == 0) {
        printf("Response: %s\n", output);
    }
    
    // Cleanup
    onesdk_disconnect(ctx);
    onesdk_deinit(ctx);
    free(ctx);
    free(config);
    
    return 0;
}
```

### IoT Thing Model Example
```c
#include "onesdk.h"

void tm_recv_handler(void *handler, const iot_tm_recv_t *recv, void *userdata) {
    onesdk_ctx_t *ctx = (onesdk_ctx_t *)userdata;
    
    switch (recv->type) {
        case IOT_TM_RECV_PROPERTY_SET_POST_REPLY:
            printf("Property set reply: msg_id=%s, code=%d\n",
                   recv->data.property_set_post_reply.msg_id,
                   recv->data.property_set_post_reply.code);
            break;
            
        case IOT_TM_RECV_PROPERTY_SET:
            printf("Property set: msg_id=%s, params=%.*s\n",
                   recv->data.property_set.msg_id,
                   recv->data.property_set.params_len,
                   recv->data.property_set.params);
            break;
            
        default:
            printf("Unknown message type: %d\n", recv->type);
            break;
    }
}

int main() {
    // Initialize device configuration
    iot_basic_config_t device_config = {
        .http_host = "https://iot-cn-shanghai.iot.volces.com",
        .instance_id = "your_instance_id",
        .auth_type = ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED,
        .product_key = "your_product_key",
        .product_secret = "your_product_secret",
        .device_name = "your_device_name",
        .device_secret = "your_device_secret",
        .verify_ssl = true,
        .ssl_ca_cert = "your_ssl_ca_cert",
    };
    
    // Initialize MQTT configuration
    iot_mqtt_config_t mqtt_config = {
        .mqtt_host = "your_mqtt_host",
        .basic_config = &device_config,
        .keep_alive = 60,
        .auto_reconnect = true,
    };
    
    // Initialize OneSDK configuration
    onesdk_config_t config = {
        .device_config = &device_config,
        .mqtt_config = &mqtt_config,
    };
    
    // Initialize OneSDK context
    onesdk_ctx_t *ctx = malloc(sizeof(onesdk_ctx_t));
    if (NULL == ctx) {
        return -1;
    }
    memset(ctx, 0, sizeof(onesdk_ctx_t));
    
    // Initialize OneSDK
    if (onesdk_init(ctx, &config) != 0) {
        printf("Failed to initialize OneSDK\n");
        free(ctx);
        return -1;
    }
    
    // Connect to platform
    if (onesdk_connect(ctx) != 0) {
        printf("Failed to connect\n");
        onesdk_deinit(ctx);
        free(ctx);
        return -1;
    }
    
    // Initialize thing model
    if (onesdk_iot_tm_init(ctx) != 0) {
        printf("Failed to initialize thing model\n");
        onesdk_disconnect(ctx);
        onesdk_deinit(ctx);
        free(ctx);
        return -1;
    }
    
    // Set thing model receive callback
    onesdk_iot_tm_set_recv_cb(ctx, tm_recv_handler, ctx);
    
    // Subscribe to custom topic
    onesdk_iot_tm_custom_topic_sub(ctx, "get");
    
    // Publish property report
    iot_tm_msg_t property_post_msg = {0};
    property_post_msg.type = IOT_TM_MSG_PROPERTY_POST;
    
    iot_tm_msg_property_post_t *property_post;
    iot_property_post_init(&property_post);
    
    // Add property parameters
    iot_property_post_add_param_num(property_post, "default:Temperature", 25.5);
    iot_property_post_add_param_num(property_post, "default:Humidity", 60.0);
    iot_property_post_add_param_str(property_post, "default:Status", "normal");
    
    // Publish to custom topic
    onesdk_iot_tm_custom_topic_pub(ctx, "report", "{\"data\":\"test\"}");
    
    // Keep IoT connection alive
    while (1) {
        onesdk_iot_keepalive(ctx);
        sleep(1);
    }
    
    onesdk_iot_tm_deinit(ctx);
    onesdk_disconnect(ctx);
    onesdk_deinit(ctx);
    free(ctx);
    
    return 0;
}
```

### Real-time Audio Example
```c
#include "onesdk.h"

void audio_callback(const void *audio_data, size_t audio_len, void *user_data) {
    printf("Received audio data: %zu bytes\n", audio_len);
}

void text_callback(const void *text_data, size_t text_len, void *user_data) {
    printf("Received text: %.*s\n", (int)text_len, (char*)text_data);
}

void error_callback(const char* code, const char *message, void *user_data) {
    printf("Error: %s - %s\n", code, message);
}

int main() {
    // Initialize device configuration
    iot_basic_config_t device_config = {
        .http_host = "https://iot-cn-shanghai.iot.volces.com",
        .instance_id = "your_instance_id",
        .auth_type = ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED,
        .product_key = "your_product_key",
        .product_secret = "your_product_secret",
        .device_name = "your_device_name",
        .device_secret = "your_device_secret",
        .verify_ssl = true,
        .ssl_ca_cert = "your_ssl_ca_cert",
    };
    
    // Initialize OneSDK configuration
    onesdk_config_t *config = malloc(sizeof(onesdk_config_t));
    if (NULL == config) {
        return -1;
    }
    memset(config, 0, sizeof(onesdk_config_t));
    config->device_config = &device_config;
    config->aigw_path = "/v1/realtime?model=AG-voice-chat-agent";
    config->send_ping = true;
    
    // Initialize OneSDK context
    onesdk_ctx_t *ctx = malloc(sizeof(onesdk_ctx_t));
    if (NULL == ctx) {
        free(config);
        return -1;
    }
    memset(ctx, 0, sizeof(onesdk_ctx_t));
    
    // Initialize OneSDK
    if (onesdk_init(ctx, config) != 0) {
        printf("Failed to initialize OneSDK\n");
        free(ctx);
        free(config);
        return -1;
    }
    
    // Set up real-time callbacks
    onesdk_rt_event_cb_t callbacks = {
        .on_audio = audio_callback,
        .on_text = text_callback,
        .on_error = error_callback,
    };
    onesdk_rt_set_event_cb(ctx, &callbacks);
    
    // Connect to platform
    if (onesdk_connect(ctx) != 0) {
        printf("Failed to connect\n");
        onesdk_deinit(ctx);
        free(ctx);
        free(config);
        return -1;
    }
    
    // Send audio data
    const char audio_data[] = "audio_data_here";
    onesdk_rt_audio_send(ctx, audio_data, sizeof(audio_data), true);
    
    // Keep session alive
    while (1) {
        onesdk_rt_session_keepalive(ctx);
        sleep(1);
    }
    
    onesdk_disconnect(ctx);
    onesdk_deinit(ctx);
    free(ctx);
    free(config);
    
    return 0;
}
```

### IoT OTA Example

```c
#include "onesdk.h"

void ota_download_complete(void *handler, int error_code, 
                          iot_ota_job_info_t *job_info,
                          const char *ota_file_path, void *user_data) {
    onesdk_ctx_t *ctx = (onesdk_ctx_t *)user_data;
    
    if (error_code == 0) {
        // Report successful installation
        onesdk_iot_ota_set_device_module_info(ctx, job_info->module, job_info->dest_version);
        onesdk_iot_ota_report_install_status(ctx, job_info->ota_job_id, 
                                           OTA_UPGRADE_DEVICE_STATUS_SUCCESS);
    } else {
        // Report failed installation
        onesdk_iot_ota_report_install_status(ctx, job_info->ota_job_id, 
                                           OTA_UPGRADE_DEVICE_STATUS_FAILED);
    }
}

int main() {
    // Initialize device configuration
    iot_basic_config_t device_config = {
        .http_host = "https://iot-cn-shanghai.iot.volces.com",
        .instance_id = "your_instance_id",
        .auth_type = ONESDK_AUTH_DYNAMIC_NO_PRE_REGISTERED,
        .product_key = "your_product_key",
        .product_secret = "your_product_secret",
        .device_name = "your_device_name",
        .device_secret = "your_device_secret",
        .verify_ssl = true,
        .ssl_ca_cert = "your_ssl_ca_cert",
    };
    
    // Initialize MQTT configuration
    iot_mqtt_config_t mqtt_config = {
        .mqtt_host = "your_mqtt_host",
        .basic_config = &device_config,
        .keep_alive = 60,
        .auto_reconnect = true,
    };
    
    // Initialize OneSDK configuration
    onesdk_config_t config = {
        .device_config = &device_config,
        .mqtt_config = &mqtt_config,
    };
    
    // Initialize OneSDK context
    onesdk_ctx_t *ctx = malloc(sizeof(onesdk_ctx_t));
    if (NULL == ctx) {
        return -1;
    }
    memset(ctx, 0, sizeof(onesdk_ctx_t));
    
    // Initialize OneSDK
    if (onesdk_init(ctx, &config) != 0) {
        printf("Failed to initialize OneSDK\n");
        free(ctx);
        return -1;
    }
    
    // Enable log upload
    if (onesdk_iot_enable_log_upload(ctx) != 0) {
        printf("Failed to enable log upload\n");
        onesdk_deinit(ctx);
        free(ctx);
        return -1;
    }
    
    // Enable OTA
    if (onesdk_iot_enable_ota(ctx, "./") != 0) {
        printf("Failed to enable OTA\n");
        onesdk_deinit(ctx);
        free(ctx);
        return -1;
    }
    
    // Set device module info and OTA callback
    onesdk_iot_ota_set_device_module_info(ctx, "default", "1.0.0");
    onesdk_iot_ota_set_download_complete_cb(ctx, ota_download_complete);
    
    // Connect to platform
    if (onesdk_connect(ctx) != 0) {
        printf("Failed to connect\n");
        onesdk_deinit(ctx);
        free(ctx);
        return -1;
    }
    
    // Keep IoT connection alive
    while (1) {
        onesdk_iot_keepalive(ctx);
        sleep(1);
    }
    
    onesdk_disconnect(ctx);
    onesdk_deinit(ctx);
    free(ctx);
    
    return 0;
}
```

## Build Configuration

OneSDK supports several build-time configuration options:

- `ONESDK_ENABLE_AI`: Enable AI chat functionality
- `ONESDK_ENABLE_AI_REALTIME`: Enable real-time AI functionality
- `ONESDK_ENABLE_IOT`: Enable IoT functionality
- `ONESDK_WITH_EXAMPLE`: Build example applications
- `ONESDK_WITH_TEST`: Build unit tests
- `ONESDK_WITH_SHARED`: Build shared libraries
- `ONESDK_WITH_STATIC`: Build static libraries
- `ONESDK_WITH_STRICT_MODE`: Enable compiler address sanitizer

## Platform Support

OneSDK supports multiple platforms:

- **Windows**: Visual Studio 2019/2022, MinGW-w64
- **Linux**: GCC, Clang
- **macOS**: Xcode, GCC
- **ESP32**: ESP-IDF v5.5

## Dependencies

- **CMake**: 3.10+ (but not higher than 4.x)
- **OpenSSL**: For TLS support
- **libwebsockets**: For WebSocket communication
- **cJSON**: For JSON parsing

## License

OneSDK is licensed under the Apache License, Version 2.0. 