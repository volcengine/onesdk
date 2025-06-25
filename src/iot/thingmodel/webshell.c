/*
 * Copyright 2022-2024 Beijing Volcano Engine Technology Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "onesdk_config.h"
#ifdef ONESDK_ENABLE_IOT

#include "thing_model/webshell.h"
#include "iot/iot_utils.h"
#include "iot_log.h"
#include "util/aws_json.h"
#include "thing_model/iot_tm_api.h"
#include "thing_model/iot_tm_header.h"
#include "util/util.h"
#include "error_code.h"
#include "iot/iot_popen.h"

void __send_webshell_command_reply(void* handle, const char* id) {
    iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handle;
    iot_tm_msg_t dm_msg = {0};
    dm_msg.type = IOT_TM_MSG_WEBSHELL_COMMAND_REPLY;
    iot_tm_recv_webshell_command_reply_t* webshell_command_reply;
    iot_webshell_command_reply_init(&webshell_command_reply, id);
    dm_msg.data.webshell_command_reply = webshell_command_reply;
    iot_tm_send(dm_handle, &dm_msg);
    iot_webshell_command_reply_free(webshell_command_reply);
}


// recv server webshell cmd  's msg
void _tm_recv_webshell_command_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
    struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
    struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

    LOGD(TAG_IOT_MQTT, "_tm_recv_weshell_command_handler call topic = %.*s,  payload = %.*s", 
        AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
        AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
    iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) pUserData;
    if (dm_handle == NULL) {
        return;
    }
    if (NULL == dm_handle->recv_handler) {
        return;
    }
    iot_tm_recv_t recv;
    AWS_ZERO_STRUCT(recv);
    recv.type = IOT_TM_RECV_WEBSHELL_COMMAND;

    //parse topic for uid
    struct aws_array_list topic_split_data_list;
    aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 8,sizeof(struct aws_byte_cursor));
    aws_byte_cursor_split_on_char(&topic_byte_cursor, '/',&topic_split_data_list);
    struct aws_byte_cursor uid_cur = {0};
    aws_array_list_get_at(&topic_split_data_list, &uid_cur, 5);

    // reply service
    struct aws_string* uid_aws_string = aws_string_new_from_cursor(dm_handle->allocator, &uid_cur);
    __send_webshell_command_reply(dm_handle, aws_string_c_str(uid_aws_string));
    aws_mem_release(dm_handle->allocator, uid_aws_string);

    // exec this command (in payload)
    const char* command = aws_cur_to_char_str(dm_handle->allocator, &payload_byte_cursor);
    char type = command[0];
    if (type == '1') {
        // write
        iot_popen((const char *) (command + 1), "w", NULL, 0);

    } else if (type == '2') {
        // ping, try send pong
        __send_webshell_command_pong(dm_handle, aws_string_c_str(uid_aws_string), "2");

    } else if (type == '3') {
        // resize terminal
        struct aws_json_value* resize_terminal_json = aws_json_value_new_from_string(dm_handle->allocator,
                                                                                     aws_byte_cursor_from_c_str((const char*)(command + 1)));
        // parse params
        int rows = aws_json_get_num_val(resize_terminal_json, "Rows");
        int columns = aws_json_get_num_val(resize_terminal_json, "Columns");
        char resize_cmd[64] = {0};
        sprintf(resize_cmd, "resize -s %d %d", rows, columns);
        iot_popen(resize_cmd, "w", NULL, 0);

    } else {
        LOGE(TAG_IOT_MQTT, "webshell type is unknown !!");
        return;
    }



}

void iot_webshell_command_reply_init(iot_tm_recv_webshell_command_reply_t** pty, const char* uid) {
    if (uid == NULL) {
        return;
    }
    iot_tm_recv_webshell_command_reply_t* webshellP = malloc( sizeof(iot_tm_recv_webshell_command_reply_t));
    webshellP->uid = uid;
    *pty = webshellP;

}

void* iot_webshell_command_reply_payload(iot_tm_recv_webshell_command_reply_t* pty) {
    return NULL;
}

void iot_webshell_command_reply_free(iot_tm_recv_webshell_command_reply_t* pty) {
    if (pty == NULL) {
        return;
    }
    aws_mem_release(aws_alloc(), pty);
}

int32_t _tm_send_webshell_command_reply(void* handler, const char* topic, const void* msg_p) {
    iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
    iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
    int32_t ret = VOLC_OK;
    struct aws_byte_buf payload_buf = aws_byte_buf_from_c_str(iot_webshell_command_pong_payload(msg->data.webshell_command_pong));
    struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
    // struct aws_byte_cursor public_topic = aws_byte_cursor_from_c_str(topic);
    // uint16_t  packet_id = aws_mqtt_client_connection_publish(dm_handle->mqtt_handle->mqtt_connection, &public_topic,
    //                                                          AWS_MQTT_QOS_AT_MOST_ONCE, false, &payload_cur,
    //                                                          _tm_mqtt_post_on_complete_fn,
    //                                                          NULL);

    iot_mqtt_publish(dm_handle->mqtt_handle, topic, (uint8_t *)payload_cur.ptr, payload_cur.len, IOT_MQTT_QOS1);
    LOGD(TAG_IOT_MQTT, "_tm_send_gateway_add_topo_notify_reply call topic = %.*s,  payload = %.*s", topic,
         AWS_BYTE_CURSOR_PRI(payload_cur));
    aws_byte_buf_clean_up(&payload_buf);
    return ret;
}

void __send_webshell_command_pong(void* handle, const char* uid, const char* pong) {
    iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handle;
    iot_tm_msg_t dm_msg = {0};
    dm_msg.type = IOT_TM_MSG_WEBSHELL_COMMAND_PONG;
    iot_tm_msg_webshell_command_pong_t* webshell_command_pong;
    iot_webshell_command_pong_init(&webshell_command_pong,uid, pong);
    iot_tm_send(dm_handle, &dm_msg);
    iot_webshell_command_pong_free(webshell_command_pong);


}

void iot_webshell_command_pong_init(iot_tm_msg_webshell_command_pong_t** pty, const char* uid, const char* pong_byte) {
    iot_tm_msg_webshell_command_pong_t* webshellP = malloc( sizeof(iot_tm_msg_webshell_command_pong_t));
    webshellP->pong = pong_byte;
    webshellP->uid = uid;
    *pty = webshellP;
}

const char* iot_webshell_command_pong_payload(iot_tm_msg_webshell_command_pong_t* pty) {
    return pty->pong;
}

void iot_webshell_command_pong_free(iot_tm_msg_webshell_command_pong_t* pty) {
    if (pty == NULL) {
        return;
    }
    aws_mem_release(aws_alloc(), pty);
}

int32_t _tm_send_webshell_command_pong(void* handler, const char* topic, const void* msg_p) {
    iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
    iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
    int32_t ret = VOLC_OK;
    struct aws_byte_buf payload_buf = aws_byte_buf_from_c_str(iot_webshell_command_pong_payload(msg->data.webshell_command_pong));
    struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
    
    iot_mqtt_publish(dm_handle->mqtt_handle, topic, (uint8_t *)payload_cur.ptr, payload_cur.len, IOT_MQTT_QOS1);
    LOGD(TAG_IOT_MQTT, "_tm_send_webshell_command call topic = %.*s,  payload = %.*s", topic,
         AWS_BYTE_CURSOR_PRI(payload_cur));

    aws_byte_buf_clean_up(&payload_buf);
    return ret;
}
#endif //ONESDK_ENABLE_IOT