// /*
//  * Copyright 2022-2024 Beijing Volcano Engine Technology Ltd.
//  *
//  * Licensed under the Apache License, Version 2.0 (the "License");
//  * you may not use this file except in compliance with the License.
//  * You may obtain a copy of the License at
//  *
//  *    http://www.apache.org/licenses/LICENSE-2.0
//  *
//  * Unless required by applicable law or agreed to in writing, software
//  * distributed under the License is distributed on an "AS IS" BASIS,
//  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  * See the License for the specific language governing permissions and
//  * limitations under the License.
//  */

// #include "thing_model/gateway.h"
// #include <ctype.h>

// #include "iot/iot_utils.h"
// #include "iot_log.h"
// #include "util/util.h"
// #include "aws/common/string.h"
// #include "aws/common/json.h"
// #include "util/aws_json.h"

// #include "thing_model/iot_tm_api.h"
// #include "thing_model/iot_tm_header.h"

// void iot_gateway_add_topo_init(iot_tm_msg_gateway_add_topo_t** pty) {
//     iot_tm_msg_gateway_add_topo_t* gatewayP = (iot_tm_msg_gateway_add_topo_t*)malloc(sizeof(iot_tm_msg_gateway_add_topo_t));
//     const char* id = get_random_string_with_time_suffix(aws_alloc());
//     gatewayP->id = id;
//     gatewayP->version = SDK_VERSION;
//     gatewayP->gateway_topo_list = (void*) aws_json_value_new_array(aws_alloc());
//     *pty = gatewayP;
// }

// bool _check_device_name_legality(const char* device_name) {
//     return device_name != NULL && strlen(device_name) <= 32 && strspn(device_name, "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-.:@") == strlen(device_name);
// }

// void to_lowercase(const char* cstr, char* output){
//     if (cstr == NULL) {
//         return;
//     }
//     int len = sizeof(cstr)/ sizeof (char);
//     for (int i=0; i < len; ++i ) {
//         output[i] = tolower(*cstr++);
//     }
// }

// int32_t iot_gateway_add_topo_item(iot_tm_msg_gateway_add_topo_t* pty, const char* device_name,const char* product_key, const char* product_secret) {
//     if (!_check_device_name_legality(device_name)) {
//         return -1;
//     }
//     struct aws_json_value* gateway_topo_list = (struct aws_json_value* )pty->gateway_topo_list;
//     struct aws_json_value* gateway_topo_item = aws_json_value_new_object(aws_alloc());
//     aws_json_add_str_val(gateway_topo_item, "device_name", device_name);
//     aws_json_add_str_val(gateway_topo_item, "product_key", product_key);
//     aws_json_add_str_val(gateway_topo_item, "product_secret", product_secret);
//     aws_json_add_array_element(gateway_topo_list, gateway_topo_item);
//     return 0;
// }

// void* iot_gateway_add_topo_payload(iot_tm_msg_gateway_add_topo_t* pty) {
//     if (pty->payload_root == NULL) {
//         pty->payload_root = (struct aws_json_value*) aws_json_value_new_object(aws_alloc());
//         aws_json_add_str_val((struct aws_json_value*) pty->payload_root, "ID", pty->id);
//         aws_json_add_str_val((struct aws_json_value*) pty->payload_root, "Version", pty->version);
//         struct aws_json_value* params =  aws_json_value_new_array(aws_alloc());

//         // add sub params
//         for (int i = 0; i < aws_json_get_array_size(((struct aws_json_value*) pty->gateway_topo_list)); ++i) {
//             struct aws_json_value* gate_topo_item = aws_json_get_array_element(((struct aws_json_value*) pty->gateway_topo_list), i);

//             uint32_t random_num = random_num();
// //            get_random_num_uint32(&random_num);
//             uint64_t time_stamp = get_current_time_sec();
//             struct aws_string* device_name =  aws_json_get_string1_val(aws_alloc(),gate_topo_item ,"device_name");
//             struct aws_string* product_key =  aws_json_get_string1_val(aws_alloc(),gate_topo_item ,"product_key");
//             struct aws_string* product_secret =  aws_json_get_string1_val(aws_alloc(),gate_topo_item ,"product_secret");

//             struct aws_string* signature = hmac_sha256_encrypt(aws_alloc(), (int32_t)random_num, time_stamp,
//                                                                aws_string_c_str(device_name),
//                                                                aws_string_c_str(product_key),
//                                                                aws_string_c_str(product_secret));
//             struct aws_json_value* sub_params = aws_json_value_new_object(aws_alloc());
//             aws_json_add_str_val(sub_params, "ProductKey", aws_string_c_str(product_key));
//             aws_json_add_str_val(sub_params, "DeviceName", aws_string_c_str(device_name));
//             aws_json_add_num_val(sub_params, "random_num", random_num);
//             aws_json_add_num_val(sub_params, "timestamp", (double)time_stamp);
//             aws_json_add_str_val(sub_params, "signature", aws_string_c_str(signature));
//             // aws_json_add_str_val method will alloc new mem for value field，so this mem can release
//             aws_mem_release(aws_alloc(), device_name);
//             aws_mem_release(aws_alloc(), product_key);
//             aws_mem_release(aws_alloc(), product_secret);
//             aws_mem_release(aws_alloc(), signature);
//             aws_json_add_array_element(params, sub_params);

//         }
//         aws_json_add_json_obj((struct aws_json_value*) pty->payload_root, "params", params);

//     }
//     return (void*)pty->payload_root;
// }

// void iot_gateway_add_topo_item_free(struct aws_array_list gateway_topo_list) {
//     if (gateway_topo_list.length <= 0) {
//         return;
//     }
//     for (int i = 0; i < gateway_topo_list.length; ++i) {
//         iot_tm_msg_gateway_add_topo_item_t* gateway_topo_item = malloc(sizeof(iot_tm_msg_gateway_add_topo_item_t));
//         aws_array_list_get_at(&gateway_topo_list, gateway_topo_item ,i);
//         if (gateway_topo_item == NULL) {
//             continue;
//         }
//         if (gateway_topo_item->product_key != NULL) {
//             aws_mem_release(aws_alloc(), (void*)gateway_topo_item->product_key);
//         }
//         if (gateway_topo_item->product_secret != NULL) {
//             aws_mem_release(aws_alloc(), (void*)gateway_topo_item->product_secret);
//         }
//         if (gateway_topo_item->device_name != NULL) {
//             aws_mem_release(aws_alloc(), (void*)gateway_topo_item->device_name);
//         }
//         free(gateway_topo_item);
//     }
// }

// void iot_gateway_add_topo_free(iot_tm_msg_gateway_add_topo_t* pty) {
//     if (pty == NULL) {
//         return;
//     }
//     if (pty->payload_root != NULL) {
//         aws_mem_release(aws_alloc(),(void*)pty->payload_root);
//     }
//     if (pty->gateway_topo_list) {
//         aws_mem_release(aws_alloc(), (void*)pty->gateway_topo_list);
//     }
//     // release item
// //    iot_gateway_add_topo_item_free(pty->gateway_topo_list);
//     aws_mem_release(aws_alloc(), (void*)pty->id);
//     aws_mem_release(aws_alloc(), pty);
// }

// int32_t _tm_send_gateway_add_topo(void* handler, const char* topic, const void* msg_p) {
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
//     iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
//     int32_t ret = VOLC_OK;
//     struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(aws_alloc(), (struct aws_json_value*)iot_gateway_add_topo_payload(msg->data.gateway_add_topo));
//     struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
//     // struct aws_byte_cursor public_topic = aws_byte_cursor_from_c_str(topic);
//     // uint16_t  packet_id = aws_mqtt_client_connection_publish(dm_handle->mqtt_handle->mqtt_connection, &public_topic,
//     //                                                          AWS_MQTT_QOS_AT_MOST_ONCE, false, &payload_cur,
//     //                                                          _tm_mqtt_post_on_complete_fn,
//     //                                                          NULL);
//     PublishParams pubParams = DEFAULT_PUB_PARAMS;
//     pubParams.qos           = QOS1;
//     pubParams.payload_len   = payload_cur.len;
//     pubParams.payload       = (void *)payload_cur.ptr;
//     int packet_id = IOT_MQTT_Publish(dm_handle->mqtt_client, topic, &pubParams);
//     LOGD(TAG_IOT_MQTT, "_tm_send_gateway_add_topo call packet_id = %d, topic = %.*s,  payload = %.*s", packet_id, topic,
//          AWS_BYTE_CURSOR_PRI(payload_cur));
//     aws_byte_buf_clean_up(&payload_buf);

//     return ret;
// }

// void _tm_recv_gateway_add_topo_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
//     struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
//     struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

//     LOGD(TAG_IOT_MQTT, "_tm_recv_gateway_add_topo_handler call topic = %.*s,  payload = %.*s", 
//         AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
//         AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) pUserData;
//     if (dm_handle == NULL) {
//         return;
//     }
//     if (NULL == dm_handle->recv_handler) {
//         return;
//     }
//     iot_tm_recv_t recv;
//     AWS_ZERO_STRUCT(recv);
//     recv.type = IOT_TM_RECV_GATEWAY_ADD_TOPO_REPLY;

//     // parse product_key, device_name, module_key by topic value
//     struct aws_array_list topic_split_data_list;
//     aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 8,sizeof(struct aws_byte_cursor));
//     aws_byte_cursor_split_on_char(&topic_byte_cursor, '/',&topic_split_data_list);

//     struct aws_byte_cursor product_key_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &product_key_cur, 1);
//     struct aws_byte_cursor device_name_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &device_name_cur, 2);
//     recv.product_key = aws_cur_to_char_str(dm_handle->allocator, &product_key_cur);
//     recv.device_name = aws_cur_to_char_str(dm_handle->allocator, &device_name_cur);

//     // package business data
//     iot_tm_msg_gateway_add_topo_reply_t gateway_add_reply;
//     struct aws_json_value* payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload_byte_cursor);
//     //    struct aws_byte_cursor id_cur = aws_json_get_str_byte_cur_val(payload_json, "ID");
//     double error_code = aws_json_get_num_val(payload_json, "Code");
//     if (error_code != 0) {
//         goto end;
//     }
//     struct aws_json_value* data_json = aws_json_get_json_obj(dm_handle->allocator, payload_json, "data");
//     size_t data_size = aws_json_get_array_size(data_json);
//     for (int i = 0; i < data_size; ++i) {
//         struct aws_json_value* data_item = aws_json_get_array_element(data_json, i);
//         struct aws_string* product_key = aws_json_get_string1_val(dm_handle->allocator, data_item, "ProductKey");
//         struct aws_string* device_name = aws_json_get_string1_val(dm_handle->allocator, data_item, "DeviceName");
//         struct aws_json_value* device_secret = aws_json_get_json_obj(dm_handle->allocator, data_item, "device_secret");
//         struct aws_string* device_secret_payload = aws_json_get_string1_val(dm_handle->allocator, device_secret, "payload");
//         struct aws_string* aws_string_secret = aes_decode(dm_handle->allocator, aws_string_c_str(dm_handle->mqtt_handle->device_secret),
//                                                aws_string_c_str(device_secret_payload));
//         const char* secret = aws_string_c_str(aws_string_secret);
//         aws_hash_table_put(&get_iot_core_context()->device_secret_map, aws_string_c_str(get_user_name(dm_handle->allocator, product_key, device_name)),
//                             secret, NULL);
// //        aws_hash_table_foreach(&get_iot_core_context()->device_secret_map, mapForeach, NULL);

//         // release
//         aws_mem_release(dm_handle->allocator, product_key);
//         aws_mem_release(dm_handle->allocator, device_name);
//         aws_mem_release(dm_handle->allocator, device_secret);
//         aws_mem_release(dm_handle->allocator, device_secret_payload);
//     }
//     if (data_json!= NULL) {
//         aws_mem_release(dm_handle->allocator, data_json);
//     }
// end:
//     // release
//     aws_mem_release(dm_handle->allocator, payload_json);
// }


// void iot_gateway_delete_topo_init(iot_tm_msg_gateway_delete_topo_t** pty) {
//     if (pty == NULL) {
//         return;
//     }
//     iot_tm_msg_gateway_delete_topo_t* gatewayP = malloc( sizeof(iot_tm_msg_gateway_delete_topo_t));
//     const char* id = get_random_string_with_time_suffix(aws_alloc());
//     gatewayP->id = id;
//     gatewayP->version = SDK_VERSION;
//     gatewayP->gateway_topo_list = (void*) aws_json_value_new_array(aws_alloc());
//     *pty = gatewayP;
// }

// void iot_gateway_delete_topo_item(iot_tm_msg_gateway_delete_topo_t* pty, const char* device_name, const char* product_key) {
//     struct aws_json_value* gateway_detele_item = aws_json_value_new_object(aws_alloc());
//     aws_json_add_str_val(gateway_detele_item, "product_key", product_key);
//     aws_json_add_str_val(gateway_detele_item, "device_name", device_name);
//     aws_json_add_array_element((struct aws_json_value*) pty->gateway_topo_list, gateway_detele_item);
// }

// void* iot_gateway_delete_topo_payload(iot_tm_msg_gateway_delete_topo_t* pty) {
//     if (pty->payload_root == NULL) {
//         pty->payload_root = aws_json_value_new_object(aws_alloc());
//         aws_json_add_str_val(pty->payload_root, "ID", pty->id);
//         aws_json_add_str_val(pty->payload_root, "Version", pty->version);
//         struct aws_json_value* params = aws_json_value_new_array(aws_alloc());
//         for (int i = 0; i < aws_json_get_array_size((struct aws_json_value*)pty->gateway_topo_list); ++i) {
//             struct aws_json_value* gateway_delete_item = aws_json_get_array_element((struct aws_json_value*) pty->gateway_topo_list, i);

//             struct aws_json_value* sub_params = aws_json_value_new_object(aws_alloc());
//             struct aws_string* product_key = aws_json_get_string1_val(aws_alloc(), gateway_delete_item, "product_key");
//             struct aws_string* device_name = aws_json_get_string1_val(aws_alloc(), gateway_delete_item, "device_name");
//             aws_json_add_str_val(sub_params, "ProductKey", aws_string_c_str(product_key));
//             aws_json_add_str_val(sub_params, "DeviceName", aws_string_c_str(device_name));
//             aws_json_add_array_element(params, sub_params);

//             // release
//             aws_mem_release(aws_alloc(), device_name);
//             aws_mem_release(aws_alloc(), product_key);
//         }
//         aws_json_add_json_obj(pty->payload_root, "params", params);
//     }
//     return pty->payload_root;
// }

// void iot_gateway_delete_topo_item_free(struct aws_array_list gateway_topo_list) {
//     if (gateway_topo_list.length <= 0) {
//         return;
//     }
//     for (int i = 0; i < gateway_topo_list.length; ++i) {
//         iot_tm_msg_gateway_add_topo_item_t* gateway_topo_item = HAL_Malloc(sizeof(iot_tm_msg_gateway_add_topo_item_t));
//         aws_array_list_get_at(&gateway_topo_list,gateway_topo_item,i);
//         if (gateway_topo_item == NULL) {
//             continue;
//         }
//         if (gateway_topo_item->product_key != NULL) {
//             aws_mem_release(aws_alloc(), gateway_topo_item->product_key);
//         }
//         if (gateway_topo_item->device_name != NULL) {
//             aws_mem_release(aws_alloc(), gateway_topo_item->device_name);
//         }
//         HAL_Free(gateway_topo_item);
//     }
// }

// void iot_gateway_delete_topo_free(iot_tm_msg_gateway_delete_topo_t * pty) {
//     if (pty == NULL) {
//         return;
//     }
//     if (pty->payload_root != NULL) {
//         aws_mem_release(aws_alloc(), pty->payload_root);
//     }
//     if (pty->gateway_topo_list != NULL) {
//         aws_mem_release(aws_alloc(), pty->gateway_topo_list);
//     }
// //    iot_gateway_delete_topo_item_free(pty->gateway_topo_list);
//     aws_mem_release(aws_alloc(), pty->id);
//     aws_mem_release(aws_alloc(), pty);
// }

// int32_t _tm_send_gateway_delete_topo(void* handler, const char* topic, const void* msg_p) {
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
//     iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
//     int32_t ret = VOLC_OK;
//     struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, ((struct aws_json_value*) iot_gateway_delete_topo_payload(msg->data.gateway_delete_topo)));
//     struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
//     // struct aws_byte_cursor public_topic = aws_byte_cursor_from_c_str(topic);
//     // uint16_t  packet_id = aws_mqtt_client_connection_publish(dm_handle->mqtt_handle->mqtt_connection, &public_topic,
//     //                                                          AWS_MQTT_QOS_AT_MOST_ONCE, false, &payload_cur,
//     //                                                          _tm_mqtt_post_on_complete_fn,
//     //                                                          NULL);
//     PublishParams pubParams = DEFAULT_PUB_PARAMS;
//     pubParams.qos           = QOS1;
//     pubParams.payload_len   = payload_cur.len;
//     pubParams.payload       = (void *)payload_cur.ptr;
//     int packet_id = IOT_MQTT_Publish(dm_handle->mqtt_client, topic, &pubParams);

//     LOGD(TAG_IOT_MQTT, "_tm_send_gateway_delete_topo call packet_id = %d, topic = %.*s,  payload = %.*s", packet_id, topic,
//          AWS_BYTE_CURSOR_PRI(payload_cur));
//     aws_byte_buf_clean_up(&payload_buf);
//     if (packet_id == 0) {
//         ret = STATE_DM_MQTT_PUBLISH_ERROR;
//     }
//     return ret;
// }

// void _tm_recv_gateway_delete_topo_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
//     struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
//     struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

//     LOGD(TAG_IOT_MQTT, "_tm_recv_gateway_delete_topo_handler call topic = %.*s,  payload = %.*s", 
//         AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
//         AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) pUserData;
//     if (dm_handle == NULL) {
//         return;
//     }
//     if (NULL == dm_handle->recv_handler) {
//         return;
//     }
//     iot_tm_recv_t recv;
//     AWS_ZERO_STRUCT(recv);
//     recv.type = IOT_TM_RECV_GATEWAY_DELETE_TOPO_REPLY;

//     // parse product_key, device_name, module_key by topic value
//     struct aws_array_list topic_split_data_list;
//     aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 8,sizeof(struct aws_byte_cursor));
//     aws_byte_cursor_split_on_char(&topic, '/',&topic_split_data_list);

//     struct aws_byte_cursor product_key_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &product_key_cur, 1);
//     struct aws_byte_cursor device_name_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &device_name_cur, 2);
//     recv.product_key = aws_cur_to_char_str(dm_handle->allocator, &product_key_cur);
//     recv.device_name = aws_cur_to_char_str(dm_handle->allocator, &device_name_cur);

//     // package business data
//     iot_tm_msg_gateway_delete_topo_reply_t gateway_delete_reply;
//     struct aws_json_value* payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload);
//     struct aws_byte_cursor id_cur = aws_json_get_str_byte_cur_val(payload_json, "ID");
//     const char* id = aws_cur_to_char_str(dm_handle->allocator, &id_cur);
//     double error_code = aws_json_get_num_val(payload_json, "Code");
//     if (error_code != 0) {
//         goto end;
//     }
//     struct aws_json_value* data_json = aws_json_get_json_obj(dm_handle->allocator, payload_json, "data");
//     if (data_json == NULL) {
//         aws_mem_release(dm_handle->allocator, data_json);
//         goto end;
//     }
//     size_t data_size = aws_json_get_array_size(data_json);
//     for (int i = 0; i < data_size; ++i) {
//         struct aws_json_value* data_item = aws_json_get_array_element(data_json, i);
//         struct aws_string* product_key = aws_json_get_string1_val(dm_handle->allocator, data_item, "ProductKey");
//         struct aws_string* device_name = aws_json_get_string1_val(dm_handle->allocator, data_item, "DeviceName");

//         // unsubscript
//         char topic[256] = { 0 };
//         sprintf(topic, "sys/%s/%s/#", aws_string_c_str(product_key), aws_string_c_str(device_name));
//         // iot_mqtt_unsub(dm_handle->mqtt_handle, topic, 1,
//         //                _tm_recv_gateway_unsubscribe_topic_handler,(void*) dm_handle);
//         IOT_MQTT_Unsubscribe(dm_handle->mqtt_client, topic);
//         // release
//         aws_mem_release(dm_handle->allocator, product_key);
//         aws_mem_release(dm_handle->allocator, device_name);
//     }

//     // release
//     aws_mem_release(dm_handle->allocator, data_json);
//     end:
//     aws_mem_release(dm_handle->allocator, payload_json);
//     aws_mem_release(dm_handle->allocator, id);

// }

// void iot_gateway_get_topo_init(iot_tm_msg_gateway_get_topo_t** pty) {
//     iot_gateway_get_topo_init_with_id(pty, NULL);
// }

// void iot_gateway_get_topo_init_with_id(iot_tm_msg_gateway_get_topo_t** pty, const char* id) {
//     iot_tm_msg_gateway_get_topo_t* gatewayP = malloc( sizeof(iot_tm_msg_gateway_get_topo_t));
//     char* real_id = id;
//     if (id == NULL) {
//         real_id = get_random_string_with_time_suffix(aws_alloc());
//     }
//     gatewayP->id = real_id;
//     gatewayP->version = SDK_VERSION;
//     *pty = gatewayP;
// }

// void* iot_gateway_get_topo_payload(iot_tm_msg_gateway_get_topo_t* pty) {
//     if (pty->payload_root == NULL) {
//         pty->payload_root = (void*) aws_json_value_new_object(aws_alloc());
//         aws_json_add_str_val_1(aws_alloc(), (struct aws_json_value*) pty->payload_root, "ID", pty->id);
//         aws_json_add_str_val_1(aws_alloc(), (struct aws_json_value*) pty->payload_root, "Version", pty->version);
//     }
//     return pty->payload_root;
// }

// void iot_gateway_get_topo_free(iot_tm_msg_gateway_get_topo_t* pty) {
//     if (pty == NULL) {
//         return;
//     }
//     if (pty->payload_root != NULL) {
//         aws_mem_release(aws_alloc(), pty->payload_root);
//     }

//     aws_mem_release(aws_alloc(), pty->id);
//     aws_mem_release(aws_alloc(), pty);
// }

// int32_t _tm_send_gateway_get_topo(void* handler, const char* topic, const void* msg_p) {
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
//     iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
//     int32_t ret = VOLC_OK;
//     struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, ((struct aws_json_value*)iot_gateway_get_topo_payload(msg->data.gateway_get_topo)));
//     struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
//     // struct aws_byte_cursor public_topic = aws_byte_cursor_from_c_str(topic);
//     // uint16_t  packet_id = aws_mqtt_client_connection_publish(dm_handle->mqtt_handle->mqtt_connection, &public_topic,
//     //                                                          AWS_MQTT_QOS_AT_MOST_ONCE, false, &payload_cur,
//     //                                                          _tm_mqtt_post_on_complete_fn,
//     //                                                          NULL);
//     PublishParams pubParams = DEFAULT_PUB_PARAMS;
//     pubParams.qos           = QOS1;
//     pubParams.payload_len   = payload_cur.len;
//     pubParams.payload       = (void *)payload_cur.ptr;
//     int packet_id = IOT_MQTT_Publish(dm_handle->mqtt_client, topic, &pubParams);

//     LOGD(TAG_IOT_MQTT, "_tm_send_gateway_get_topo call packet_id = %d, topic = %.*s,  payload = %.*s", packet_id, topic,
//          AWS_BYTE_CURSOR_PRI(payload_cur));
//     aws_byte_buf_clean_up(&payload_buf);
//     if (packet_id == 0) {
//         ret = STATE_DM_MQTT_PUBLISH_ERROR;
//     }
//     return ret;
// }

// void _tm_recv_gateway_get_topo_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
//     struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
//     struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

//     LOGD(TAG_IOT_MQTT, "_tm_recv_gateway_get_topo_handler call topic = %.*s,  payload = %.*s", 
//         AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
//         AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) pUserData;
//     if (dm_handle == NULL) {
//         return;
//     }
//     if (NULL == dm_handle->recv_handler) {
//         return;
//     }
//     iot_tm_recv_t recv;
//     AWS_ZERO_STRUCT(recv);
//     recv.type = IOT_TM_RECV_GATEWAY_GET_TOPO_REPLY;

//     // parse product_key, device_name, module_key by topic value
//     struct aws_array_list topic_split_data_list;
//     aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 8,sizeof(struct aws_byte_cursor));
//     aws_byte_cursor_split_on_char(&topic, '/',&topic_split_data_list);

//     struct aws_byte_cursor product_key_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &product_key_cur, 1);
//     struct aws_byte_cursor device_name_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &device_name_cur, 2);
//     recv.product_key = aws_cur_to_char_str(dm_handle->allocator, &product_key_cur);
//     recv.device_name = aws_cur_to_char_str(dm_handle->allocator, &device_name_cur);

//     // package business data
//     iot_tm_msg_gateway_get_topo_reply_t gateway_get_reply;
//     struct aws_json_value* payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload);
//     struct aws_byte_cursor id_cur = aws_json_get_str_byte_cur_val(payload_json, "ID");
//     const char* id = aws_cur_to_char_str(dm_handle->allocator, &id_cur);
//     double error_code = aws_json_get_num_val(payload_json, "Code");
//     if (error_code != 0) {
//         goto end;
//     }
//     struct aws_json_value* data_json = aws_json_get_json_obj(dm_handle->allocator, payload_json, "data");
//     if (data_json == NULL) {
//         aws_mem_release(dm_handle->allocator, data_json);
//         goto end;
//     }
//     const size_t data_size = aws_json_get_array_size(data_json);
//     iot_tm_msg_gateway_get_topo_item_t* gateway_get_items[64] = {};
//     for (int i = 0; i < data_size; ++i) {
//         struct aws_json_value* data_item = aws_json_get_array_element(data_json, i);
//         struct aws_string* product_key = aws_json_get_string1_val(dm_handle->allocator, data_item, "ProductKey");
//         struct aws_string* device_name = aws_json_get_string1_val(dm_handle->allocator, data_item, "DeviceName");

//         gateway_get_items[i] = (iot_tm_msg_gateway_get_topo_item_t*) aws_mem_calloc(dm_handle->allocator, 1, sizeof(iot_tm_msg_gateway_get_topo_item_t));
//         gateway_get_items[i]->product_key = aws_string_c_str(product_key);
//         gateway_get_items[i]->device_name = aws_string_c_str(device_name);
//         // release
//         aws_mem_release(dm_handle->allocator, product_key);
//         aws_mem_release(dm_handle->allocator, device_name);
//     }
//     gateway_get_reply.gateway_topo_list = gateway_get_items;
//     recv.data.gateway_get = gateway_get_reply;

//     // callback business
//     dm_handle->recv_handler(dm_handle, &recv, dm_handle->userdata);

//     // release
//     for (int i = 0; i < data_size; ++i) {
//         aws_mem_release(dm_handle->allocator, gateway_get_items[i]);
//     }

//     aws_mem_release(dm_handle->allocator, data_json);
//     end:
//     aws_mem_release(dm_handle->allocator, payload_json);
//     aws_mem_release(dm_handle->allocator, id);

// }


// void iot_gateway_get_device_secret_init(iot_tm_msg_gateway_get_device_secret_t** pty) {
//     iot_tm_msg_gateway_get_device_secret_t* gatewayP = malloc( sizeof(iot_tm_msg_gateway_get_device_secret_t));
//     const char* id = get_random_string_with_time_suffix(aws_alloc());
//     gatewayP->id = id;
//     gatewayP->version = SDK_VERSION;
//     gatewayP->uuid = get_uuid_c_str(aws_alloc());
//     gatewayP->gateway_topo_list = (void*) aws_json_value_new_array(aws_alloc());
//     *pty = gatewayP;
// }

// int32_t iot_gateway_get_device_secret_item(iot_tm_msg_gateway_get_device_secret_t* pty, const char* device_name, const char* product_key) {
//     if (!_check_device_name_legality(device_name)) {
//         return -1;
//     }
//     struct aws_json_value* get_device_secret_item = aws_json_value_new_object(aws_alloc());
//     aws_json_add_str_val(get_device_secret_item, "device_name", device_name);
//     aws_json_add_str_val(get_device_secret_item, "product_key", product_key);
//     aws_json_add_array_element((struct aws_json_value*) pty->gateway_topo_list, get_device_secret_item);
//     return 0;
// }

// void* iot_gateway_get_device_secret_payload(iot_tm_msg_gateway_get_device_secret_t* pty) {
//     if (pty->payload_root == NULL) {
//         pty->payload_root = aws_json_value_new_object(aws_alloc());
//         aws_json_add_str_val(pty->payload_root, "ID", pty->id);
//         aws_json_add_str_val(pty->payload_root, "Version", pty->version);
//         struct aws_json_value* params = aws_json_value_new_array(aws_alloc());
//         for (int i = 0; i < aws_json_get_array_size(((struct aws_json_value*) pty->gateway_topo_list)); ++i) {
//             struct aws_json_value* gateway_get_device_secret_item = aws_json_get_array_element((struct aws_json_value*) pty->gateway_topo_list, i);

//             struct aws_string* product_key = aws_json_get_string1_val(aws_alloc(), gateway_get_device_secret_item, "product_key");
//             struct aws_string* device_name = aws_json_get_string1_val(aws_alloc(), gateway_get_device_secret_item, "device_name");
//             struct aws_json_value* sub_params = aws_json_value_new_object(aws_alloc());
//             aws_json_add_str_val(sub_params, "ProductKey", aws_string_c_str(product_key));
//             aws_json_add_str_val(sub_params, "DeviceName", aws_string_c_str(device_name));
//             aws_json_add_array_element(params, sub_params);

//             aws_mem_release(aws_alloc(), product_key);
//             aws_mem_release(aws_alloc(), device_name);
//         }
//         aws_json_add_json_obj((struct aws_json_value*) pty->payload_root, "params", params);
//     }
//     return pty->payload_root;
// }

// void iot_gateway_get_device_secret_free(iot_tm_msg_gateway_get_device_secret_t* pty) {
//     if (pty == NULL) {
//         return;
//     }
//     if (pty->payload_root != NULL) {
//         aws_mem_release(aws_alloc(), pty->payload_root);
//     }
//     if (pty->gateway_topo_list != NULL) {
//         aws_mem_release(aws_alloc(), pty->gateway_topo_list);
//     }
//     aws_mem_release(aws_alloc(), pty->id);
// //    aws_mem_release(aws_alloc(), pty->uuid);
//     aws_mem_release(aws_alloc(), pty);
// }

// int32_t _tm_send_gateway_get_device_secret(void* handler, const char* topic, const void* msg_p) {
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
//     iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
//     int32_t ret = VOLC_OK;
//     struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, ((struct aws_json_value*)iot_gateway_get_device_secret_payload(msg->data.gateway_get_device_secret)));
//     struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
//     // struct aws_byte_cursor public_topic = aws_byte_cursor_from_c_str(topic);
//     // uint16_t  packet_id = aws_mqtt_client_connection_publish(dm_handle->mqtt_handle->mqtt_connection, &public_topic,
//     //                                                          AWS_MQTT_QOS_AT_MOST_ONCE, false, &payload_cur,
//     //                                                          _tm_mqtt_post_on_complete_fn,
//     //                                                          NULL);
//     PublishParams pubParams = DEFAULT_PUB_PARAMS;
//     pubParams.qos           = QOS1;
//     pubParams.payload_len   = payload_cur.len;
//     pubParams.payload       = (void *)payload_cur.ptr;
//     int packet_id = IOT_MQTT_Publish(dm_handle->mqtt_client, topic, &pubParams);

//     LOGD(TAG_IOT_MQTT, "_tm_send_gateway_get_device_secret call packet_id = %d, topic = %.*s,  payload = %.*s", packet_id, topic,
//          AWS_BYTE_CURSOR_PRI(payload_cur));
//     aws_byte_buf_clean_up(&payload_buf);
//     if (packet_id == 0) {
//         ret = STATE_DM_MQTT_PUBLISH_ERROR;
//     }
//     return ret;
// }

// void _tm_recv_gateway_get_device_secret_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
//     struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
//     struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);


//     LOGD(TAG_IOT_MQTT, "_tm_recv_gateway_get_device_secret_handler call topic = %.*s,  payload = %.*s", 
//         AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
//         AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) pUserData;
//     if (dm_handle == NULL) {
//         return;
//     }
//     if (NULL == dm_handle->recv_handler) {
//         return;
//     }
//     iot_tm_recv_t recv;
//     AWS_ZERO_STRUCT(recv);
//     recv.type = IOT_TM_RECV_GATEWAY_GET_DEVICE_SECRET;

//     // parse product_key, device_name, module_key by topic value
//     struct aws_array_list topic_split_data_list;
//     aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 8,sizeof(struct aws_byte_cursor));
//     aws_byte_cursor_split_on_char(&topic, '/',&topic_split_data_list);

//     struct aws_byte_cursor product_key_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &product_key_cur, 1);
//     struct aws_byte_cursor device_name_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &device_name_cur, 2);
//     recv.product_key = aws_cur_to_char_str(dm_handle->allocator, &product_key_cur);
//     recv.device_name = aws_cur_to_char_str(dm_handle->allocator, &device_name_cur);

//     // package business data
//     iot_tm_msg_gateway_get_device_secret_reply_t gateway_get_device_secret_reply;
//     struct aws_json_value* payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload);
//     struct aws_byte_cursor id_cur = aws_json_get_str_byte_cur_val(payload_json, "ID");
//     const char* id = aws_cur_to_char_str(dm_handle->allocator, &id_cur);
//     double error_code = aws_json_get_num_val(payload_json, "Code");
//     if (error_code != 0) {
//         goto end;
//     }
//     struct aws_json_value* data_json = aws_json_get_json_obj(dm_handle->allocator, payload_json, "data");
//     if (data_json == NULL) {
//         aws_mem_release(dm_handle->allocator, data_json);
//         goto end;
//     }
//     size_t data_size = aws_json_get_array_size(data_json);
//     for (int i = 0; i < data_size; ++i) {
//         struct aws_json_value* data_item = aws_json_get_array_element(data_json, i);
//         struct aws_string* product_key = aws_json_get_string1_val(dm_handle->allocator, data_item, "ProductKey");
//         struct aws_string* device_name = aws_json_get_string1_val(dm_handle->allocator, data_item, "DeviceName");
//         struct aws_json_value* device_secret_json = aws_json_get_json_obj(dm_handle->allocator, data_item, "device_secret");
//         struct aws_string* device_secret_payload = aws_json_get_string1_val(dm_handle->allocator, device_secret_json, "payload");

//         // save
//         struct aws_string* secret = aes_decode(dm_handle->allocator, aws_string_c_str(dm_handle->mqtt_handle->device_secret),
//                                                aws_string_c_str(device_secret_payload));
//         aws_hash_table_put(&get_iot_core_context()->device_secret_map, aws_string_c_str(get_user_name(dm_handle->allocator, product_key, device_name)),
//                             aws_string_c_str(secret), NULL);

//         // release
//         aws_mem_release(dm_handle->allocator, product_key);
//         aws_mem_release(dm_handle->allocator, device_name);
//         aws_mem_release(dm_handle->allocator, device_secret_json);
//         aws_mem_release(dm_handle->allocator, device_secret_payload);
//     }

//     // release
//     aws_mem_release(dm_handle->allocator, data_json);
//     end:
//     aws_mem_release(dm_handle->allocator, payload_json);
//     aws_mem_release(dm_handle->allocator, id);

// }


// void iot_gateway_sub_device_login_init(iot_tm_msg_gateway_sub_device_login_t** pty) {
//     iot_tm_msg_gateway_sub_device_login_t* gatewayP = malloc( sizeof (iot_tm_msg_gateway_sub_device_login_t));
//     const char* id = get_random_string_with_time_suffix(aws_alloc());
//     gatewayP->id = aws_string_new_from_c_str(aws_alloc(), id);
//     gatewayP->version = SDK_VERSION;
//     gatewayP->gateway_topo_list = (void*) aws_json_value_new_array(aws_alloc());
//     *pty = gatewayP;
// }

// int32_t iot_gateway_sub_device_login_item(iot_tm_msg_gateway_sub_device_login_t* pty, const char* device_name, const char* product_key) {
//     if (!_check_device_name_legality(device_name)) {
//         return -1;
//     }
//     struct aws_json_value* gateway_sub_device_login_item = aws_json_value_new_object(aws_alloc());
//     aws_json_add_str_val(gateway_sub_device_login_item, "device_name",device_name);
//     aws_json_add_str_val(gateway_sub_device_login_item, "product_key", product_key);
//     aws_json_add_array_element((struct aws_json_value*) pty->gateway_topo_list, gateway_sub_device_login_item);
//     return 0;
// }

// // int mapForeach(void *context, struct aws_hash_element *p_element) {
// //     const char* key = p_element->key;
// //     const char* value = p_element->value;
// //     LOGI("skin", "key = %s, value = %s", key, value);
// //
// // }

// void* iot_gateway_sub_device_login_payload(iot_tm_msg_gateway_sub_device_login_t* pty) {
//     if (pty->payload_root == NULL) {
//         pty->payload_root = aws_json_value_new_object(aws_alloc());
//         aws_json_add_str_val(pty->payload_root, "ID", aws_string_c_str(pty->id));
//         aws_json_add_str_val(pty->payload_root, "Version", pty->version);
//         struct aws_json_value* params =  aws_json_value_new_array(aws_alloc());
//         for (int i = 0; i < aws_json_get_array_size(((struct aws_json_value*) pty->gateway_topo_list)); ++i) {
//             struct aws_json_value* gateway_sub_device_login_item = aws_json_get_array_element((struct aws_json_value*)pty->gateway_topo_list,  i);

//             struct aws_string* product_key = aws_json_get_string1_val(aws_alloc(), gateway_sub_device_login_item, "product_key");
//             struct aws_string* device_name = aws_json_get_string1_val(aws_alloc(), gateway_sub_device_login_item, "device_name");

//             uint32_t random_num = arenal_get_random_num();
// //            get_random_num_uint32(&random_num);
//             uint64_t time_stamp = get_current_time_sec();
//             // get device secret
//             struct aws_hash_element* device_secret = NULL;
//             const char* device_secret_key = aws_string_c_str(get_user_name(aws_alloc(), product_key, device_name));
//             // aws_hash_table_foreach(&get_iot_core_context()->device_secret_map, mapForeach, NULL);
//             aws_hash_table_find(&get_iot_core_context()->device_secret_map, device_secret_key,&device_secret);
//             if (device_secret == NULL) {
//                 LOGE(TAG_IOT_MQTT, "sub device[%s] no secret", device_secret_key);
//                 goto end;
//             }
//             const char* secret = device_secret->value;

//             LOGD(TAG_IOT_MQTT, "sub device[%s] secret = %s", device_secret_key, secret);
//             struct aws_string* signature = hmac_sha256_encrypt(aws_alloc(), random_num, time_stamp,
//                                                                aws_string_c_str(device_name),
//                                                                aws_string_c_str(product_key),
//                                                                secret);

//             struct aws_json_value* sub_params = aws_json_value_new_object(aws_alloc());
//             aws_json_add_str_val(sub_params, "ProductKey", aws_string_c_str(product_key));
//             aws_json_add_str_val(sub_params, "DeviceName", aws_string_c_str(device_name));
//             aws_json_add_num_val(sub_params, "random_num", random_num);
//             aws_json_add_num_val(sub_params, "timestamp", time_stamp);
//             aws_json_add_str_val(sub_params, "signature", aws_string_c_str(signature));
//             // aws_json_add_str_val method will alloc new mem for value field，so this mem can release
// //            aws_mem_release(aws_alloc(), signature);
//             aws_json_add_array_element(params, sub_params);

//             // release
//             aws_mem_release(aws_alloc(), product_key);
//             aws_mem_release(aws_alloc(), device_name);
//         }
//         aws_json_add_json_obj(pty->payload_root, "params", params);


//     }
//     end:
//     return pty->payload_root;
// }

// void iot_gateway_sub_device_login_free(iot_tm_msg_gateway_sub_device_login_t* pty) {
//     if(pty == NULL) {
//         return;
//     }
//     if (pty->payload_root != NULL) {
//         aws_mem_release(aws_alloc(), pty->payload_root);
//     }
//     if (pty->gateway_topo_list != NULL){
//         aws_mem_release(aws_alloc(), pty->gateway_topo_list);
//     }
//     aws_mem_release(aws_alloc(), pty->id);
// }

// int32_t _tm_send_gateway_sub_device_login(void* handler, const char* topic, const void* msg_p) {
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
//     iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
//     int32_t ret = VOLC_OK;
//     struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, ((struct aws_json_value*)iot_gateway_sub_device_login_payload(msg->data.gateway_sub_device_login)));
//     struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
//     // struct aws_byte_cursor public_topic = aws_byte_cursor_from_c_str(topic);
//     // uint16_t  packet_id = aws_mqtt_client_connection_publish(dm_handle->mqtt_handle->mqtt_connection, &public_topic,
//     //                                                          AWS_MQTT_QOS_AT_MOST_ONCE, false, &payload_cur,
//     //                                                          _tm_mqtt_post_on_complete_fn,
//     //                                                          NULL);
//     PublishParams pubParams = DEFAULT_PUB_PARAMS;
//     pubParams.qos           = QOS1;
//     pubParams.payload_len   = payload_cur.len;
//     pubParams.payload       = (void *)payload_cur.ptr;
//     int packet_id = IOT_MQTT_Publish(dm_handle->mqtt_client, topic, &pubParams);

//     LOGD(TAG_IOT_MQTT, "_tm_send_gateway_sub_device_login call packet_id = %d, topic = %.*s,  payload = %.*s", packet_id, topic,
//          AWS_BYTE_CURSOR_PRI(payload_cur));
//     aws_byte_buf_clean_up(&payload_buf);
//     if (packet_id == 0) {
//         ret = STATE_DM_MQTT_PUBLISH_ERROR;
//     }
//     return ret;
// }

// void _tm_recv_gateway_sub_device_login_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
//     struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
//     struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

//     LOGD(TAG_IOT_MQTT, "_tm_recv_gateway_sub_device_login_handler call topic = %.*s,  payload = %.*s", 
//         AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
//         AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) pUserData;
//     if (dm_handle == NULL) {
//         return;
//     }
//     if (NULL == dm_handle->recv_handler) {
//         return;
//     }
//     iot_tm_recv_t recv;
//     AWS_ZERO_STRUCT(recv);
//     recv.type = IOT_TM_RECV_GATEWAY_GET_TOPO_REPLY;

//     // parse product_key, device_name, module_key by topic value
//     struct aws_array_list topic_split_data_list;
//     aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 8,sizeof(struct aws_byte_cursor));
//     aws_byte_cursor_split_on_char(&topic, '/',&topic_split_data_list);

//     struct aws_byte_cursor product_key_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &product_key_cur, 1);
//     struct aws_byte_cursor device_name_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &device_name_cur, 2);
//     recv.product_key = aws_cur_to_char_str(dm_handle->allocator, &product_key_cur);
//     recv.device_name = aws_cur_to_char_str(dm_handle->allocator, &device_name_cur);

//     // package business data
//     iot_tm_msg_gateway_get_topo_reply_t gateway_get_reply;
//     struct aws_json_value* payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload);
//     struct aws_byte_cursor id_cur = aws_json_get_str_byte_cur_val(payload_json, "ID");
//     const char* id = aws_cur_to_char_str(dm_handle->allocator, &id_cur);
//     double error_code = aws_json_get_num_val(payload_json, "Code");
//     if (error_code != 0) {
//         goto end;
//     }
//     struct aws_json_value* data_json = aws_json_get_json_obj(dm_handle->allocator, payload_json, "data");
//     if (data_json == NULL) {
//         aws_mem_release(dm_handle->allocator, data_json);
//         goto end;
//     }
//     const size_t data_size = aws_json_get_array_size(data_json);
//     iot_tm_msg_gateway_get_topo_item_t* gateway_get_items[64] = {};
//     for (int i = 0; i < data_size; ++i) {
//         struct aws_json_value* data_item = aws_json_get_array_element(data_json, i);
//         struct aws_string* product_key = aws_json_get_string1_val(dm_handle->allocator, data_item, "ProductKey");
//         struct aws_string* device_name = aws_json_get_string1_val(dm_handle->allocator, data_item, "DeviceName");

//         // subscribe sub device topic
//         __s_tm_set_up_mqtt_topic(dm_handle, product_key, device_name);

//         // release
//         aws_mem_release(dm_handle->allocator, product_key);
//         aws_mem_release(dm_handle->allocator, device_name);
//         gateway_get_items[i] = (iot_tm_msg_gateway_get_topo_item_t*) aws_mem_calloc(dm_handle->allocator, 1, sizeof(iot_tm_msg_gateway_get_topo_item_t));
//         gateway_get_items[i]->product_key = aws_string_c_str(product_key);
//         gateway_get_items[i]->device_name = aws_string_c_str(device_name);
//     }
//     gateway_get_reply.gateway_topo_list = gateway_get_items;
//     recv.data.gateway_get = gateway_get_reply;

//     // callback business
//     dm_handle->recv_handler(dm_handle, &recv, dm_handle->userdata);

//     // release
//     for (int i = 0; i < data_size; ++i) {
//         aws_mem_release(dm_handle->allocator, gateway_get_items[i]);
//     }

//     aws_mem_release(dm_handle->allocator, data_json);
//     end:
//     aws_mem_release(dm_handle->allocator, payload_json);
//     aws_mem_release(dm_handle->allocator, id);

// }

// void iot_gateway_sub_device_logout_init(iot_tm_msg_gateway_sub_device_logout_t** pty) {
//     iot_tm_msg_gateway_sub_device_logout_t* gatewayP = malloc( sizeof(iot_tm_msg_gateway_sub_device_logout_t));
//     const char* id = get_random_string_with_time_suffix(aws_alloc());
//     gatewayP->id = id;
//     gatewayP->version = SDK_VERSION;
//     gatewayP->gateway_topo_list = (void*) aws_json_value_new_array(aws_alloc());
//     *pty = gatewayP;
// }

// int32_t iot_gateway_sub_device_logout_item(iot_tm_msg_gateway_sub_device_logout_t* pty, const char* device_name, const char* product_key) {
//     if (pty == NULL || !_check_device_name_legality(device_name)) {
//         return -1;
//     }
//     struct aws_json_value* gateway_sub_device_logout_item = aws_json_value_new_object(aws_alloc());
//     aws_json_add_str_val(gateway_sub_device_logout_item, "device_name", device_name);
//     aws_json_add_str_val(gateway_sub_device_logout_item, "product_key", product_key);
//     aws_json_add_array_element((struct aws_json_value*) pty->gateway_topo_list, gateway_sub_device_logout_item);
//     return 0;
// }

// void* iot_gateway_sub_device_logout_payload(iot_tm_msg_gateway_sub_device_logout_t* pty) {
//     if (pty->payload_root == NULL) {
//         pty->payload_root = aws_json_value_new_object(aws_alloc());
//         aws_json_add_str_val((struct aws_json_value*) pty->payload_root, "ID", pty->id);
//         aws_json_add_str_val((struct aws_json_value*) pty->payload_root, "Version", pty->version);
//         struct aws_json_value* params = aws_json_value_new_array(aws_alloc());
//         for (int i = 0; i < aws_json_get_array_size((struct aws_json_value*)pty->gateway_topo_list); ++i) {
//             struct aws_json_value* gateway_sub_device_logout_item = aws_json_get_array_element((struct aws_json_value*)pty->gateway_topo_list, i);
//             struct aws_string* product_key = aws_json_get_string1_val(aws_alloc(), gateway_sub_device_logout_item, "product_key");
//             struct aws_string* device_name = aws_json_get_string1_val(aws_alloc(), gateway_sub_device_logout_item, "device_name");

//             struct aws_json_value* sub_params = aws_json_value_new_object(aws_alloc());
//             aws_json_add_str_val(sub_params, "ProductKey", aws_string_c_str(product_key));
//             aws_json_add_str_val(sub_params, "DeviceName", aws_string_c_str(device_name));
//             aws_json_add_array_element(params, sub_params);

//             // release
//             aws_mem_release(aws_alloc(), product_key);
//             aws_mem_release(aws_alloc(), device_name);
//         }
//         aws_json_add_json_obj((struct aws_json_value*) pty->payload_root, "params", params);
//     }
//     return pty->payload_root;
// }

// void iot_gateway_sub_device_logout_free(iot_tm_msg_gateway_sub_device_logout_t* pty) {
//     if (pty == NULL) {
//         return;
//     }
//     if (pty->payload_root != NULL) {
//         aws_mem_release(aws_alloc(), pty->payload_root);
//     }
//     if (pty->gateway_topo_list != NULL) {
//         aws_mem_release(aws_alloc(), pty->gateway_topo_list);
//     }
//     aws_mem_release(aws_alloc(), pty->id);
// }

// int32_t _tm_send_gateway_sub_device_logout(void* handler, const char* topic, const void* msg_p) {
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
//     iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
//     int32_t ret = VOLC_OK;
//     struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, ((struct aws_json_value*)iot_gateway_sub_device_logout_payload(msg->data.gateway_sub_device_logout)));
//     struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
//     // struct aws_byte_cursor public_topic = aws_byte_cursor_from_c_str(topic);
//     // uint16_t  packet_id = aws_mqtt_client_connection_publish(dm_handle->mqtt_handle->mqtt_connection, &public_topic,
//     //                                                          AWS_MQTT_QOS_AT_MOST_ONCE, false, &payload_cur,
//     //                                                          _tm_mqtt_post_on_complete_fn,
//     //                                                          NULL);
//     PublishParams pubParams = DEFAULT_PUB_PARAMS;
//     pubParams.qos           = QOS1;
//     pubParams.payload_len   = payload_cur.len;
//     pubParams.payload       = (void *)payload_cur.ptr;
//     int packet_id = IOT_MQTT_Publish(dm_handle->mqtt_client, topic, &pubParams);

//     LOGD(TAG_IOT_MQTT, "_tm_send_gateway_sub_device_logout call packet_id = %d, topic = %.*s,  payload = %.*s", packet_id, topic,
//          AWS_BYTE_CURSOR_PRI(payload_cur));
//     aws_byte_buf_clean_up(&payload_buf);
//     if (packet_id == 0) {
//         ret = STATE_DM_MQTT_PUBLISH_ERROR;
//     }
//     return ret;
// }

// void _tm_recv_gateway_sub_device_logout_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
//     struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
//     struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

//     LOGD(TAG_IOT_MQTT, "_tm_recv_gateway_sub_device_logout_handler call topic = %.*s,  payload = %.*s", 
//         AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
//         AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) pUserData;
//     if (dm_handle == NULL) {
//         return;
//     }
//     if (NULL == dm_handle->recv_handler) {
//         return;
//     }
//     iot_tm_recv_t recv;
//     AWS_ZERO_STRUCT(recv);
//     recv.type = IOT_TM_RECV_GATEWAY_SUB_DEVICE_LOGOUT;

//     // parse product_key, device_name, module_key by topic value
//     struct aws_array_list topic_split_data_list;
//     aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 8,sizeof(struct aws_byte_cursor));
//     aws_byte_cursor_split_on_char(&topic, '/',&topic_split_data_list);

//     struct aws_byte_cursor product_key_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &product_key_cur, 1);
//     struct aws_byte_cursor device_name_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &device_name_cur, 2);
//     recv.product_key = aws_cur_to_char_str(dm_handle->allocator, &product_key_cur);
//     recv.device_name = aws_cur_to_char_str(dm_handle->allocator, &device_name_cur);

//     // package business data
//     iot_tm_msg_gateway_sub_device_logout_reply_t gateway_sub_device_logout_reply;
//     struct aws_json_value* payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload);
//     struct aws_byte_cursor id_cur = aws_json_get_str_byte_cur_val(payload_json, "ID");
//     const char* id = aws_cur_to_char_str(dm_handle->allocator, &id_cur);
//     double error_code = aws_json_get_num_val(payload_json, "Code");
//     if (error_code != 0) {
//         goto end;
//     }
//     struct aws_json_value* data_json = aws_json_get_json_obj(dm_handle->allocator, payload_json, "data");
//     if (data_json == NULL) {
//         aws_mem_release(dm_handle->allocator, data_json);
//         goto end;
//     }
//     size_t data_size = aws_json_get_array_size(data_json);
//     for (int i = 0; i < data_size; ++i) {
//         struct aws_json_value* data_item = aws_json_get_array_element(data_json, i);
//         struct aws_string* product_key = aws_json_get_string1_val(dm_handle->allocator, data_item, "ProductKey");
//         struct aws_string* device_name = aws_json_get_string1_val(dm_handle->allocator, data_item, "DeviceName");

//         // unsubscript
//         char topic[256] = { 0 };
//         sprintf(topic, "sys/%s/%s/#", aws_string_c_str(product_key), aws_string_c_str(device_name));
//         // iot_mqtt_unsub(dm_handle->mqtt_handle, topic, 1,
//         //                _tm_recv_gateway_unsubscribe_topic_handler,(void*) dm_handle);
//         IOT_MQTT_Unsubscribe(dm_handle->mqtt_client, topic);
//         // release
//         aws_mem_release(dm_handle->allocator, product_key);
//         aws_mem_release(dm_handle->allocator, device_name);
//     }

//     // release
//     aws_mem_release(dm_handle->allocator, data_json);
//     end:
//     aws_mem_release(dm_handle->allocator, payload_json);
//     aws_mem_release(dm_handle->allocator, id);

// }


// void iot_gateway_sub_device_discovery_init(iot_tm_msg_gateway_sub_device_discovery_t** pty) {
//     iot_tm_msg_gateway_sub_device_discovery_t* gatewayP = malloc( sizeof(iot_tm_msg_gateway_sub_device_discovery_t));
//     const char* id = get_random_string_with_time_suffix(aws_alloc());
//     gatewayP->id = id;
//     gatewayP->version = SDK_VERSION;
//     gatewayP->gateway_topo_list = (void*) aws_json_value_new_array(aws_alloc());
//     *pty = gatewayP;
// }

// int32_t iot_gateway_sub_device_discovery_item(iot_tm_msg_gateway_sub_device_discovery_t* pty, const char* device_name, const char* product_key) {
//     if (pty == NULL || !_check_device_name_legality(device_name)) {
//         return -1;
//     }
//     struct aws_json_value* gateway_sub_device_discovery_item = aws_json_value_new_object(aws_alloc());
//     aws_json_add_str_val(gateway_sub_device_discovery_item, "device_name", device_name);
//     aws_json_add_str_val(gateway_sub_device_discovery_item, "product_key", product_key);
//     aws_json_add_array_element((struct aws_json_value*)pty->gateway_topo_list, gateway_sub_device_discovery_item);
//     return 0;
// }

// void* iot_gateway_sub_device_discovery_payload(iot_tm_msg_gateway_sub_device_discovery_t* pty) {
//     if (pty->payload_root == NULL) {
//         pty->payload_root = aws_json_value_new_object(aws_alloc());
//         aws_json_add_str_val((struct aws_json_value*)pty->payload_root, "ID", aws_string_c_str(pty->id));
//         aws_json_add_str_val((struct aws_json_value*)pty->payload_root, "Version", pty->version);
//         struct aws_json_value* params = aws_json_value_new_array(aws_alloc());
//         for (int i = 0; i < aws_json_get_array_size((struct aws_json_value*) pty->gateway_topo_list); ++i) {
//             struct aws_json_value* gateway_sub_device_discovery_item = aws_json_get_array_element((struct aws_json_value*) pty->gateway_topo_list, i);

//             struct aws_string* product_key = aws_json_get_string1_val(aws_alloc(), gateway_sub_device_discovery_item, "product_key");
//             struct aws_string* device_name = aws_json_get_string1_val(aws_alloc(), gateway_sub_device_discovery_item, "device_name");

//             struct aws_json_value* sub_params = aws_json_value_new_object(aws_alloc());
//             aws_json_add_str_val(sub_params, "ProductKey", aws_string_c_str(product_key));
//             aws_json_add_str_val(sub_params, "DeviceName", aws_string_c_str(device_name));
//             aws_json_add_array_element(params, sub_params);
//         }
//         aws_json_add_json_obj((struct aws_json_value*)pty->payload_root, "params", params);
//     }
//     return pty->payload_root;
// }

// void iot_gateway_sub_device_discovery_free(iot_tm_msg_gateway_sub_device_discovery_t* pty) {
//     if (pty == NULL) {
//         return;
//     }
//     if (pty->payload_root != NULL) {
//         aws_mem_release(aws_alloc(), pty->payload_root);
//     }
//     if (pty->gateway_topo_list != NULL) {
//         aws_mem_release(aws_alloc(), pty->gateway_topo_list);
//     }
//     aws_mem_release(aws_alloc(), pty->id);
// }

// int32_t _tm_send_gateway_sub_device_discovery(void* handler, const char* topic, const void* msg_p) {
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
//     iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
//     int32_t ret = VOLC_OK;
//     struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, ((struct aws_json_value*)iot_gateway_sub_device_discovery_payload(msg->data.gateway_sub_device_discovery)));
//     struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
//     // struct aws_byte_cursor public_topic = aws_byte_cursor_from_c_str(topic);
//     // uint16_t  packet_id = aws_mqtt_client_connection_publish(dm_handle->mqtt_handle->mqtt_connection, &public_topic,
//     //                                                          AWS_MQTT_QOS_AT_MOST_ONCE, false, &payload_cur,
//     //                                                          _tm_mqtt_post_on_complete_fn,
//     //                                                          NULL);

//     PublishParams pubParams = DEFAULT_PUB_PARAMS;
//     pubParams.qos           = QOS1;
//     pubParams.payload_len   = payload_cur.len;
//     pubParams.payload       = (void *)payload_cur.ptr;
//     int packet_id = IOT_MQTT_Publish(dm_handle->mqtt_client, topic, &pubParams);

//     LOGD(TAG_IOT_MQTT, "_tm_send_gateway_sub_device_discovery call packet_id = %d, topic = %.*s,  payload = %.*s", packet_id, topic,
//          AWS_BYTE_CURSOR_PRI(payload_cur));
//     aws_byte_buf_clean_up(&payload_buf);
//     if (packet_id == 0) {
//         ret = STATE_DM_MQTT_PUBLISH_ERROR;
//     }
//     return ret;
// }

// void _tm_recv_gateway_sub_device_discovery_reply_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
//     struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
//     struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

//     LOGD(TAG_IOT_MQTT, "_tm_recv_gateway_sub_device_discovery_handler call topic = %.*s,  payload = %.*s", 
//         AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
//         AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) pUserData;
//     if (dm_handle == NULL) {
//         return;
//     }
//     if (NULL == dm_handle->recv_handler) {
//         return;
//     }
//     iot_tm_recv_t recv;
//     AWS_ZERO_STRUCT(recv);
//     recv.type = IOT_TM_RECV_GATEWAY_SUB_DEVICE_LOGOUT;

//     // parse product_key, device_name, module_key by topic value
//     struct aws_array_list topic_split_data_list;
//     aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 8,sizeof(struct aws_byte_cursor));
//     aws_byte_cursor_split_on_char(&topic, '/',&topic_split_data_list);

//     struct aws_byte_cursor product_key_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &product_key_cur, 1);
//     struct aws_byte_cursor device_name_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &device_name_cur, 2);
//     recv.product_key = aws_cur_to_char_str(dm_handle->allocator, &product_key_cur);
//     recv.device_name = aws_cur_to_char_str(dm_handle->allocator, &device_name_cur);

//     // package business data
//     iot_tm_msg_gateway_sub_device_logout_reply_t gateway_sub_device_logout_reply;
//     struct aws_json_value* payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload);
//     struct aws_byte_cursor id_cur = aws_json_get_str_byte_cur_val(payload_json, "ID");
//     const char* id = aws_cur_to_char_str(dm_handle->allocator, &id_cur);
//     double error_code = aws_json_get_num_val(payload_json, "Code");
//     if (error_code != 0) {
//         goto end;
//     }
//     struct aws_json_value* data_json = aws_json_get_json_obj(dm_handle->allocator, payload_json, "data");
//     if (data_json == NULL) {
//         aws_mem_release(dm_handle->allocator, data_json);
//         goto end;
//     }
//     size_t data_size = aws_json_get_array_size(data_json);
//     for (int i = 0; i < data_size; ++i) {
//         struct aws_json_value* data_item = aws_json_get_array_element(data_json, i);
//         struct aws_string* product_key = aws_json_get_string1_val(dm_handle->allocator, data_item, "ProductKey");
//         struct aws_string* device_name = aws_json_get_string1_val(dm_handle->allocator, data_item, "DeviceName");

//         // release
//         aws_mem_release(dm_handle->allocator, product_key);
//         aws_mem_release(dm_handle->allocator, device_name);
//     }

//     // release
//     aws_mem_release(dm_handle->allocator, data_json);
//     end:
//     aws_mem_release(dm_handle->allocator, payload_json);
//     aws_mem_release(dm_handle->allocator, id);
// }

// void _tm_recv_gateway_add_topo_notify_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
//     struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
//     struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

//     LOGD(TAG_IOT_MQTT, "_tm_recv_gateway_add_topo_notify_handler call topic = %.*s,  payload = %.*s", 
//         AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
//         AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) pUserData;
//     if (dm_handle == NULL) {
//         return;
//     }
//     if (NULL == dm_handle->recv_handler) {
//         return;
//     }
//     iot_tm_recv_t recv;
//     AWS_ZERO_STRUCT(recv);
//     recv.type = IOT_TM_RECV_GATEWAY_ADD_TOPO_NOTIFY;

//     // parse product_key, device_name, module_key by topic value
//     struct aws_array_list topic_split_data_list;
//     aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 8,sizeof(struct aws_byte_cursor));
//     aws_byte_cursor_split_on_char(&topic, '/',&topic_split_data_list);

//     struct aws_byte_cursor product_key_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &product_key_cur, 1);
//     struct aws_byte_cursor device_name_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &device_name_cur, 2);
//     size_t topic_split_data_list_size = aws_array_list_length(&topic_split_data_list);
//     struct aws_byte_cursor trace_id_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &trace_id_cur, topic_split_data_list_size -1);
//     recv.product_key = aws_cur_to_char_str(dm_handle->allocator, &product_key_cur);
//     recv.device_name = aws_cur_to_char_str(dm_handle->allocator, &device_name_cur);

//     // package business data
//     iot_tm_recv_gateway_add_topo_notify_t gateway_add_topo_notify;
//     struct aws_json_value* payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload);

// //    double error_code = aws_json_get_num_val(payload_json, "Code");
// //    if (error_code != 0) {
// //        goto end;
// //    }
//     struct aws_json_value* data_json = aws_json_get_json_obj(dm_handle->allocator, payload_json, "params");
//     if (data_json == NULL) {
//         aws_mem_release(dm_handle->allocator, data_json);
//         goto end;
//     }
//     struct aws_byte_cursor id_cur = aws_json_get_str_byte_cur_val(payload_json, "ID");
//     const char* id = aws_cur_to_char_str(dm_handle->allocator, &id_cur);
//     double time_stamp = aws_json_get_num_val(payload_json, "timestamp");
//     const size_t data_size = aws_json_get_array_size(data_json);
//     iot_tm_recv_gateway_add_topo_notify_item_t* gateway_add_topo_items[64] = {};
//     for (int i = 0; i < data_size; ++i) {
//         struct aws_json_value* data_item = aws_json_get_array_element(data_json, i);
//         struct aws_string* product_key = aws_json_get_string1_val(dm_handle->allocator, data_item, "ProductKey");
//         struct aws_string* device_name = aws_json_get_string1_val(dm_handle->allocator, data_item, "DeviceName");

//         gateway_add_topo_items[i] = (iot_tm_msg_gateway_get_topo_item_t*) aws_mem_calloc(dm_handle->allocator, 1, sizeof(iot_tm_recv_gateway_add_topo_notify_item_t));
//         gateway_add_topo_items[i]->product_key = aws_string_c_str(product_key);
//         gateway_add_topo_items[i]->device_name = aws_string_c_str(device_name);

//     }
//     gateway_add_topo_notify.gateway_topo_list = gateway_add_topo_items;
//     recv.data.gateway_add_topo_notify = gateway_add_topo_notify;

//     // callback business
//     dm_handle->recv_handler(dm_handle, &recv, dm_handle->userdata);

//     // reply server
//     struct aws_string* trace_id_string = aws_string_new_from_cursor(dm_handle->allocator, &trace_id_cur);
//     __send_gateway_add_topo_notify_reply(dm_handle, id, (int64_t)time_stamp, aws_string_c_str(trace_id_string));
//     aws_mem_release(dm_handle->allocator,trace_id_string);

//     // release, don't release


//     aws_mem_release(dm_handle->allocator, data_json);
//     aws_mem_release(dm_handle->allocator, id);

//     end:
//     aws_mem_release(dm_handle->allocator, payload_json);

// }

// void __send_gateway_add_topo_notify_reply(void* handle, const char* id, int64_t time_stamp, const char* trace_id) {
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handle;
//     iot_tm_msg_t dm_mg = {0};
//     dm_mg.type = IOT_TM_MSG_GATEWAY_ADD_TOPO_NOTIFY_REPLY;
//     iot_tm_msg_gateway_add_topo_notify_reply_t* gateway_add_topo_notify_reply;
//     iot_gateway_add_topo_notify_reply_init(&gateway_add_topo_notify_reply, id, time_stamp, trace_id);
//     dm_mg.data.gateway_add_topo_notify_reply = gateway_add_topo_notify_reply;
//     iot_tm_send(dm_handle, &dm_mg);
//     iot_gateway_add_topo_notify_reply_free(gateway_add_topo_notify_reply);
// }

// void iot_gateway_add_topo_notify_reply_init(iot_tm_msg_gateway_add_topo_notify_reply_t** pty, const char* id, int64_t time_stamp, const char* trace_id) {
//     iot_tm_msg_gateway_add_topo_notify_reply_t* gatewayP = malloc( sizeof(iot_tm_msg_gateway_add_topo_notify_reply_t));
//     gatewayP->id = id;
//     gatewayP->code = 0;
//     gatewayP->time_stamp = time_stamp;
//     gatewayP->trace_id = trace_id;
//     *pty = gatewayP;
// }

// void* iot_gateway_add_topo_notify_reply_payload(iot_tm_msg_gateway_add_topo_notify_reply_t* pty) {
//     if (pty->payload_root == NULL) {
//         pty->payload_root = aws_json_value_new_object(aws_alloc());
//         aws_json_add_str_val((struct aws_json_value*) pty->payload_root, "ID", pty->id);
//         aws_json_add_num_val((struct aws_json_value*) pty->payload_root, "Code", pty->code);
//         aws_json_add_num_val((struct aws_json_value*) pty->payload_root, "timestamp", pty->time_stamp);
//     }
//     return pty->payload_root;
// }

// void iot_gateway_add_topo_notify_reply_free(iot_tm_msg_gateway_add_topo_notify_reply_t* pty) {
//     if (pty == NULL) {
//         return;
//     }
//     if (pty->payload_root !=  NULL) {
//         aws_mem_release(aws_alloc(), pty->payload_root);
//     }
// //    aws_mem_release(aws_alloc(), pty->id);
//     aws_mem_release(aws_alloc(), pty);
// }

// int32_t _tm_send_gateway_add_topo_notify_reply(void* handler, const char* topic, const void* msg_p) {
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
//     iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
//     int32_t ret = VOLC_OK;
//     struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, ((struct aws_json_value*)iot_gateway_add_topo_notify_reply_payload(msg->data.gateway_add_topo_notify_reply)));
//     struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
//     // struct aws_byte_cursor public_topic = aws_byte_cursor_from_c_str(topic);
//     // uint16_t  packet_id = aws_mqtt_client_connection_publish(dm_handle->mqtt_handle->mqtt_connection, &public_topic,
//     //                                                          AWS_MQTT_QOS_AT_MOST_ONCE, false, &payload_cur,
//     //                                                          _tm_mqtt_post_on_complete_fn,
//     //                                                          NULL);

//     PublishParams pubParams = DEFAULT_PUB_PARAMS;
//     pubParams.qos           = QOS1;
//     pubParams.payload_len   = payload_cur.len;
//     pubParams.payload       = (void *)payload_cur.ptr;
//     int packet_id = IOT_MQTT_Publish(dm_handle->mqtt_client, topic, &pubParams);

//     LOGD(TAG_IOT_MQTT, "_tm_send_gateway_add_topo_notify_reply call packet_id = %d, topic = %.*s,  payload = %.*s", packet_id, topic,
//          AWS_BYTE_CURSOR_PRI(payload_cur));
//     aws_byte_buf_clean_up(&payload_buf);
//     if (packet_id == 0) {
//         ret = STATE_DM_MQTT_PUBLISH_ERROR;
//     }
//     return ret;
// }

// void __send_gateway_topo_change_notify_reply(void* handle, const char* id) {
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*)handle;
//     iot_tm_msg_t dm_mdg = {};
//     dm_mdg.type = IOT_TM_MSG_GATEWAY_TOPO_CHANGE_NOTIFY_REPLY;
//     iot_tm_msg_gateway_topo_change_notify_reply_t* gateway_topo_change_notify_reply;
//     iot_gateway_topo_change_notify_reply_init(&gateway_topo_change_notify_reply, id);
//     dm_mdg.data.gateway_topo_change_notify_reply = gateway_topo_change_notify_reply;
//     iot_tm_send(dm_handle, &dm_mdg);
//     iot_gateway_topo_change_notify_reply_free(gateway_topo_change_notify_reply);
// }

// void _tm_recv_gateway_topo_change_notify_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
//     struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
//     struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

//     LOGD(TAG_IOT_MQTT, "_tm_recv_gateway_topo_change_notify_handler call topic = %.*s,  payload = %.*s", 
//         AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
//         AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) pUserData;
//     if (dm_handle == NULL) {
//         return;
//     }
//     if (NULL == dm_handle->recv_handler) {
//         return;
//     }
//     iot_tm_recv_t recv;
//     AWS_ZERO_STRUCT(recv);
//     recv.type = IOT_TM_RECV_GATEWAY_TOPO_CHANGE_NOTIFY;

//     // parse product_key, device_name, module_key by topic value
//     struct aws_array_list topic_split_data_list;
//     aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 8,sizeof(struct aws_byte_cursor));
//     aws_byte_cursor_split_on_char(&topic, '/',&topic_split_data_list);

//     struct aws_byte_cursor product_key_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &product_key_cur, 1);
//     struct aws_byte_cursor device_name_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &device_name_cur, 2);
//     size_t topic_split_data_list_size = aws_json_get_array_size(&topic_split_data_list);
//     struct aws_byte_cursor trace_id_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &trace_id_cur, topic_split_data_list_size -1);
//     recv.product_key = aws_cur_to_char_str(dm_handle->allocator, &product_key_cur);
//     recv.device_name = aws_cur_to_char_str(dm_handle->allocator, &device_name_cur);

//     iot_tm_recv_gateway_topo_change_notify_t gateway_topo_change_notify;
//     struct aws_json_value* payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload);
//     struct aws_string* id = aws_json_get_string1_val(dm_handle->allocator, payload_json, "ID");
//     struct aws_json_value* params = aws_json_get_json_obj(dm_handle->allocator, payload_json, "params");

//     struct aws_string* operate_type_string = aws_json_get_string1_val(dm_handle->allocator, params, "operate_type");
//     struct aws_json_value* sub_devices = aws_json_get_json_obj(dm_handle->allocator, params, "sub_devices");

//     char operate_type_c_str[16] = {0};
//     to_lowercase(aws_string_c_str(operate_type_string), operate_type_c_str);

//     if (strcmp(operate_type_c_str, "create") == 0) {
//         gateway_topo_change_notify.change_type = IOT_GATEWAY_TOPO_CHANGE_TYPE_CREATE;
//     } else if (strcmp(operate_type_c_str, "delete") == 0) {
//         gateway_topo_change_notify.change_type = IOT_GATEWAY_TOPO_CHANGE_TYPE_DELETE;
//     } else if (strcmp(operate_type_c_str, "enable") == 0) {
//         gateway_topo_change_notify.change_type = IOT_GATEWAY_TOPO_CHANGE_TYPE_ENABLE;
//     } else if (strcmp(operate_type_c_str, "disable") == 0) {
//         gateway_topo_change_notify.change_type = IOT_GATEWAY_TOPO_CHANGE_TYPE_DISABLE;
//     } else {
//         LOGE(TAG_IOT_MQTT, "recv %s operate type,  unknown type", operate_type_c_str);
//         return;
//     }
//     // try release
//     aws_mem_release(dm_handle->allocator, operate_type_string);

//     if (sub_devices == NULL) {
//         goto end;
//     }

//     size_t sub_device_size = aws_json_get_array_size(sub_devices);
//     iot_tm_recv_gateway_topo_change_notify_item_t* gateway_topo_change_notify_item_list[64] = {0};
//     for (int i = 0; i < sub_device_size; ++ i) {
//         struct aws_json_value* sub_device_json = aws_json_get_array_element(sub_devices, i);
//         struct aws_string* product_key = aws_json_get_string1_val(dm_handle->allocator, sub_device_json, "ProductKey");
//         struct aws_string* device_name = aws_json_get_string1_val(dm_handle->allocator, sub_device_json, "DeviceName");
//         struct aws_json_value* device_secret_json = aws_json_get_string1_val(dm_handle->allocator, sub_device_json, "device_secret");
//         struct aws_string* device_secret_payload = aws_json_get_string1_val(dm_handle->allocator, device_secret_json, "payload");
//         struct aws_string* aws_string_secret = aes_decode(dm_handle->allocator, aws_string_c_str(dm_handle->mqtt_handle->device_secret),
//                                                           aws_string_c_str(device_secret_payload));
//         gateway_topo_change_notify_item_list[i] = (struct iot_tm_recv_gateway_topo_change_notify_item_t*) aws_mem_calloc(dm_handle->allocator, 1, sizeof(iot_tm_recv_gateway_topo_change_notify_item_t));
//         gateway_topo_change_notify_item_list[i]->product_key = aws_string_c_str(product_key);
//         gateway_topo_change_notify_item_list[i]->device_name = aws_string_c_str(device_name);
//         gateway_topo_change_notify_item_list[i]->device_secrt = aws_string_c_str(aws_string_secret);

//         // release
//         aws_mem_release(dm_handle->allocator, sub_device_json);
//         aws_mem_release(dm_handle->allocator, device_secret_payload);
//     }
//     gateway_topo_change_notify.gateway_topo_list = gateway_topo_change_notify_item_list;
//     recv.data.gateway_topo_change_notify = gateway_topo_change_notify;

//     // callback business
//     dm_handle->recv_handler(dm_handle, &recv, dm_handle->userdata);

//     // reply server
//     __send_gateway_topo_change_notify_reply(dm_handle, aws_string_c_str(id));

//     // save device info when op_type is create, remove topic when op_type is  delete and disable
//     for (int i = 0; i < sub_device_size; ++i) {
//         struct aws_string* product_key = aws_string_new_from_c_str(dm_handle->allocator, gateway_topo_change_notify_item_list[i]->product_key);
//         struct aws_string* device_name = aws_string_new_from_c_str(dm_handle->allocator, gateway_topo_change_notify_item_list[i]->device_name);
// //        struct aws_string* device_secret = aws_string_new_from_c_str(dm_handle->allocator, gateway_topo_change_notify_item_list[i]->device_secrt);
//         if (strcmp(operate_type_c_str, "create") == 0) {
//             aws_hash_table_put(&get_iot_core_context()->device_secret_map, aws_string_c_str(get_user_name(dm_handle->allocator, product_key, device_name)),
//                                gateway_topo_change_notify_item_list[i]->device_secrt, NULL);

//         } else if (strcmp(operate_type_c_str, "delete") == 0 || strcmp(operate_type_c_str, "disable") == 0) {
//             // remove device info from device secret map and un sub topic
//             aws_hash_table_remove(&get_iot_core_context()->device_secret_map, aws_string_c_str(get_user_name(dm_handle->allocator, product_key, device_name)), NULL, NULL);

//             // hashmap_delete(get_iot_core_context()->device_secret_map, get_user_name(dm_handle->allocator, product_key, device_name));

//             char topic[256] = { 0 };
//             sprintf(topic, "sys/%s/%s/#", gateway_topo_change_notify_item_list[i]->product_key, gateway_topo_change_notify_item_list[i]->device_name);
//             // iot_mqtt_unsub(dm_handle->mqtt_handle, topic, 1,
//             //                _tm_recv_gateway_unsubscribe_topic_handler,(void*) dm_handle);
//             IOT_MQTT_Unsubscribe(dm_handle->mqtt_client, topic);
//         }
//         aws_mem_release(dm_handle->allocator, product_key);
//         aws_mem_release(dm_handle->allocator, device_name);
//     }

//     // release, no need release gateway_topo_change_notify_item_list item's params, due to it's char*

//     end:
//     aws_mem_release(dm_handle->allocator, payload_json);
//     aws_mem_release(dm_handle->allocator, params);
//     aws_mem_release(dm_handle->allocator, id);
// }

// void iot_gateway_topo_change_notify_reply_init(iot_tm_msg_gateway_topo_change_notify_reply_t** pty, const char* id) {
//     iot_tm_msg_gateway_topo_change_notify_reply_t* gatewayP = malloc( sizeof(iot_tm_msg_gateway_topo_change_notify_reply_t));
//     gatewayP->id = id;
//     gatewayP->code = 0;
//     *pty = gatewayP;
// }

// void* iot_gateway_topo_change_notify_reply_payload(iot_tm_msg_gateway_topo_change_notify_reply_t* pty) {
//     if (pty->payload_root == NULL) {
//         pty->payload_root = aws_json_value_new_object(aws_alloc());
//         aws_json_add_str_val((struct aws_json_value*) pty->payload_root, "ID", aws_string_c_str(pty->id));
//         aws_json_add_num_val((struct aws_json_value*) pty->payload_root, "Code", pty->code);
//     }
//     return pty->payload_root;
// //    return aws_json_obj_to_byte_buf(aws_alloc(), pty->payload_root);
// }

// void iot_gateway_topo_change_notify_reply_free(iot_tm_msg_gateway_topo_change_notify_reply_t* pty) {
//     if (pty == NULL) {
//         return;
//     }

//     if (pty->payload_root != NULL) {
//         aws_mem_release(aws_alloc(), pty->payload_root);
//     }
// //    aws_mem_release(aws_alloc(), pty->id);
// }

// int32_t _tm_send_gateway_topo_change_notify_reply(void* handler, const char* topic, const void* msg_p) {
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
//     iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
//     int32_t ret = VOLC_OK;
//     struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, ((struct aws_json_value*)iot_gateway_topo_change_notify_reply_payload(msg->data.gateway_topo_change_notify_reply)));
//     struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
//     // struct aws_byte_cursor public_topic = aws_byte_cursor_from_c_str(topic);
//     // uint16_t  packet_id = aws_mqtt_client_connection_publish(dm_handle->mqtt_handle->mqtt_connection, &public_topic,
//     //                                                          AWS_MQTT_QOS_AT_MOST_ONCE, false, &payload_cur,
//     //                                                          _tm_mqtt_post_on_complete_fn,
//     //                                                          NULL);

//     PublishParams pubParams = DEFAULT_PUB_PARAMS;
//     pubParams.qos           = QOS1;
//     pubParams.payload_len   = payload_cur.len;
//     pubParams.payload       = (void *)payload_cur.ptr;
//     int packet_id = IOT_MQTT_Publish(dm_handle->mqtt_client, topic, &pubParams);

//     LOGD(TAG_IOT_MQTT, "_tm_send_gateway_add_topo_notify_reply call packet_id = %d, topic = %.*s,  payload = %.*s", packet_id, topic,
//          AWS_BYTE_CURSOR_PRI(payload_cur));
//     aws_byte_buf_clean_up(&payload_buf);
//     if (packet_id == 0) {
//         ret = STATE_DM_MQTT_PUBLISH_ERROR;
//     }
//     return ret;
// }

// void __send_gateway_sub_device_change_notify_reply(void* handle, const char* id) {
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handle;
//     iot_tm_msg_t dm_msg = {0};
//     dm_msg.type = IOT_TM_MSG_GATEWAY_SUB_DEVICE_CHANGE_NOTIFY_REPLY;
//     iot_tm_msg_gateway_sub_device_change_notify_reply_t* gateway_sub_device_change;
//     iot_gateway_sub_device_change_notify_reply_init(&gateway_sub_device_change, id);
//     dm_msg.data.gateway_sub_device_notify_reply = gateway_sub_device_change;
//     iot_tm_send(dm_handle, &gateway_sub_device_change);
//     iot_gateway_sub_device_change_notify_reply_free(gateway_sub_device_change);

// }

// void _tm_recv_gateway_sub_device_change_notify_handler(const char* topic, const uint8_t *payload, size_t len, void *pUserData) {
//     struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
//     struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

//     LOGD(TAG_IOT_MQTT, "_tm_recv_gateway_sub_device_change_notify_handler call topic = %.*s,  payload = %.*s", 
//         AWS_BYTE_CURSOR_PRI(topic_byte_cursor), 
//         AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) pUserData;
//     if (dm_handle == NULL) {
//         return;
//     }
//     if (NULL == dm_handle->recv_handler) {
//         return;
//     }
//     iot_tm_recv_t recv;
//     AWS_ZERO_STRUCT(recv);
//     recv.type = IOT_TM_RECV_GATEWAY_SUB_DEVICE_CHANGE_NOTIFY;

//     // parse product_key, device_name, module_key by topic value
//     struct aws_array_list topic_split_data_list;
//     aws_array_list_init_dynamic(&topic_split_data_list, dm_handle->allocator, 8,sizeof(struct aws_byte_cursor));
//     aws_byte_cursor_split_on_char(&topic, '/',&topic_split_data_list);

//     struct aws_byte_cursor product_key_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &product_key_cur, 1);
//     struct aws_byte_cursor device_name_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &device_name_cur, 2);
//     size_t topic_split_data_list_size = aws_json_get_array_size(&topic_split_data_list);
//     struct aws_byte_cursor trace_id_cur = {0};
//     aws_array_list_get_at(&topic_split_data_list, &trace_id_cur, topic_split_data_list_size -1);
//     recv.product_key = aws_cur_to_char_str(dm_handle->allocator, &product_key_cur);
//     recv.device_name = aws_cur_to_char_str(dm_handle->allocator, &device_name_cur);

//     iot_tm_recv_gateway_sub_device_change_notify_t gateway_sub_device_change_notify;
//     struct aws_json_value* payload_json = aws_json_value_new_from_string(dm_handle->allocator, payload);
//     struct aws_string* id = aws_json_get_string1_val(dm_handle->allocator, payload_json, "ID");
//     struct aws_json_value* params = aws_json_get_json_obj(dm_handle->allocator, payload_json, "params");

//     struct aws_string* operate_type_string = aws_json_get_string1_val(dm_handle->allocator, params, "operate_type");
//     struct aws_json_value* sub_devices = aws_json_get_json_obj(dm_handle->allocator, params, "sub_devices");

//     const char* operate_type_c_str = aws_string_c_str(operate_type_string);
//     // todo operate type

//     // try release
//     aws_mem_release(dm_handle->allocator, operate_type_c_str);

//     if (sub_devices == NULL) {
//         goto end;
//     }

//     size_t sub_device_size = aws_json_get_array_size(sub_devices);
//     iot_tm_recv_gateway_topo_change_notify_item_t* gateway_topo_change_notify_item_list[64] = {0};
//     for (int i = 0; i < sub_device_size; ++ i) {
//         struct aws_json_value* sub_device_json = aws_json_get_array_element(sub_devices, i);
//         struct aws_string* product_key = aws_json_get_string1_val(dm_handle->allocator, sub_device_json, "ProductKey");
//         struct aws_string* device_name = aws_json_get_string1_val(dm_handle->allocator, sub_device_json, "DeviceName");

//         gateway_topo_change_notify_item_list[i] = (struct iot_tm_recv_gateway_topo_change_notify_item_t*) aws_mem_calloc(dm_handle->allocator, 1, sizeof(iot_tm_recv_gateway_topo_change_notify_item_t));
//         gateway_topo_change_notify_item_list[i]->product_key = aws_string_c_str(product_key);
//         gateway_topo_change_notify_item_list[i]->device_name = aws_string_c_str(device_name);

//         // release
//         aws_mem_release(dm_handle->allocator, sub_device_json);
//     }
//     gateway_sub_device_change_notify.gateway_topo_list = gateway_topo_change_notify_item_list;
//     recv.data.gateway_sub_device_change_notify = gateway_sub_device_change_notify;

//     // callback business
//     dm_handle->recv_handler(dm_handle, &recv, dm_handle->userdata);

//     // reply server
//     __send_gateway_sub_device_change_notify_reply(dm_handle, aws_string_c_str(id));

//     // release
//     for (int i = 0; i < sub_device_size; ++i ) {
//         aws_mem_release(dm_handle->allocator, gateway_topo_change_notify_item_list[i]->product_key);
//         aws_mem_release(dm_handle->allocator, gateway_topo_change_notify_item_list[i]->device_name);
//         aws_mem_release(dm_handle->allocator, gateway_topo_change_notify_item_list[i]->device_secrt);
//     }

//     end:
//     aws_mem_release(dm_handle->allocator, payload_json);
//     aws_mem_release(dm_handle->allocator, params);
//     aws_mem_release(dm_handle->allocator, id);

// }

// void iot_gateway_sub_device_change_notify_reply_init(iot_tm_msg_gateway_sub_device_change_notify_reply_t** pty, const char* id) {
//     iot_tm_msg_gateway_sub_device_change_notify_reply_t* gatewayP = malloc( sizeof(iot_tm_msg_gateway_sub_device_change_notify_reply_t));
//     gatewayP->id = id;
//     gatewayP->code = 0;
//     *pty = gatewayP;
// }

// void* iot_gateway_sub_device_change_notify_reply_payload(iot_tm_msg_gateway_sub_device_change_notify_reply_t* pty) {
//     if (pty->payload_root == NULL) {
//         pty->payload_root = aws_json_value_new_object(aws_alloc());
//         aws_json_add_str_val((struct aws_json_value*) pty->payload_root, "ID", pty->id);
//         aws_json_add_num_val((struct aws_json_value*) pty->payload_root, "Code", pty->code);
//     }
//     return pty->payload_root;
// }

// void iot_gateway_sub_device_change_notify_reply_free(iot_tm_msg_gateway_sub_device_change_notify_reply_t* pty) {
//     if (pty == NULL) {
//         return;
//     }
//     if (pty->payload_root != NULL) {
//         aws_mem_release(aws_alloc(), pty->payload_root);
//     }
//     aws_mem_release(aws_alloc(), pty->id);
//     aws_mem_release(aws_alloc(), pty);
// }

// int32_t _tm_send_gateway_sub_device_change_notify_reply(void* handler, const char* topic, const void* msg_p) {
//     iot_tm_handler_t* dm_handle = (iot_tm_handler_t*) handler;
//     iot_tm_msg_t* msg = (iot_tm_msg_t*) msg_p;
//     int32_t ret = VOLC_OK;
//     struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(dm_handle->allocator, ((struct aws_json_value*)iot_gateway_sub_device_change_notify_reply_payload(msg->data.gateway_sub_device_notify_reply)));
//     struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
//     // struct aws_byte_cursor public_topic = aws_byte_cursor_from_c_str(topic);
//     // uint16_t  packet_id = aws_mqtt_client_connection_publish(dm_handle->mqtt_handle->mqtt_connection, &public_topic,
//     //                                                          AWS_MQTT_QOS_AT_MOST_ONCE, false, &payload_cur,
//     //                                                          _tm_mqtt_post_on_complete_fn,
//     //                                                          NULL);

//     PublishParams pubParams = DEFAULT_PUB_PARAMS;
//     pubParams.qos           = QOS1;
//     pubParams.payload_len   = payload_cur.len;
//     pubParams.payload       = (void *)payload_cur.ptr;
//     int packet_id = IOT_MQTT_Publish(dm_handle->mqtt_client, topic, &pubParams);

//     LOGD(TAG_IOT_MQTT, "_tm_send_gateway_add_topo_notify_reply call packet_id = %d, topic = %.*s,  payload = %.*s", packet_id, topic,
//          AWS_BYTE_CURSOR_PRI(payload_cur));
//     aws_byte_buf_clean_up(&payload_buf);
//     if (packet_id == 0) {
//         ret = STATE_DM_MQTT_PUBLISH_ERROR;
//     }
//     return ret;
// }