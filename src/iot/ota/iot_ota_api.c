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


#include "onesdk_config.h"
#ifdef ONESDK_ENABLE_IOT

#include "iot/iot_ota_api.h"
#include "iot/iot_ota_utils.h"
#include "iot/iot_ota_header.h"
#include "iot/iot_kv.h"
#include "iot_log.h"
#include "aws/common/utils.h"
#include "aws/common/array_list.h"
#include "util/aws_json.h"
#include "util/util.h"
#include "util/aes_decode.h"
#include "iot/iot_utils.h"
#include "protocols/http.h"
#include <libwebsockets.h>

#define OTA_UPGRADE_NOTIFY_TOPIC "sys/%s/%s/ota/notify/+"
#define OTA_REQUEST_UPGRADE_REPLY_TOPIC "sys/%s/%s/ota/upgrade/post_reply"
#define OTA_UPGRADE_PROGRESS_REPORT_TOPIC "sys/%s/%s/ota/progress/%s"
#define OTA_REQUEST_UPGRADE_TOPIC "sys/%s/%s/ota/upgrade/post"
#define OTA_VERSION_REPORT_TOPIC "sys/%s/%s/ota/version"

#define KEY_SAVE_JOB_INFO "key_save_job_info_%s"
#define KEY_SAVE_TASK_INFO "key_save_task_info_%s"

int hash_c_str_eq(const void *a, const void *b, void *udata) {
    iot_ota_job_task_info_t *a_job = (iot_ota_job_task_info_t *)a;
    iot_ota_job_task_info_t *b_job = (iot_ota_job_task_info_t *)b;
    return strcmp(a_job->server_job_info->ota_job_id, b_job->server_job_info->ota_job_id);
}

iot_ota_handler_t *iot_ota_init() {
    iot_ota_handler_t *ota_handler = (iot_ota_handler_t*)malloc(sizeof(iot_ota_handler_t));
    memset(ota_handler, 0, sizeof(iot_ota_handler_t));
    ota_handler->allocator = aws_alloc();
    lws_pthread_mutex_init(ota_handler->lock);
    ota_handler->kv_ctx = iot_kv_init("", "./ota.kv"); // TODO： 后续优化
    aws_hash_table_init(&ota_handler->task_hash_map, ota_handler->allocator, 2, aws_hash_c_string,
         aws_hash_callback_c_str_eq, NULL, NULL);

    // 任务Task Map jobid -> jobTask
    ota_handler->auto_request_ota_info_interval_sec = OTA_AUTO_REQUEST_INFO_INTERVAL_SEC;

    return ota_handler;
}

void iot_ota_deinit(iot_ota_handler_t *handler) {
    lws_pthread_mutex_destroy(handler->lock);
    iot_kv_deinit(handler->kv_ctx);
    free(handler->device_info_array);
    free(handler->download_dir);
    aws_hash_table_clean_up(&handler->task_hash_map);
    // hashmap_free(handler->task_hash_map);
}

int32_t iot_ota_set_mqtt_handler(iot_ota_handler_t *handle, iot_mqtt_ctx_t *mqtt_handle)
{
    CHECK_NULL_POINTER(handle);
    CHECK_NULL_POINTER(mqtt_handle);

    handle->mqtt_handle = mqtt_handle;
    _mqtt_sub_ota_info(handle);
    _mqtt_sub_ota_upgrade_post_reply(handle);
    return VOLC_OK;
}

int32_t iot_ota_set_get_job_info_callback(iot_ota_handler_t *handle, iot_ota_get_job_info_callback *callback, void *user_data) {
    CHECK_NULL_POINTER(handle);

    handle->get_job_info_callback = callback;
    handle->get_job_info_callback_user_data = user_data;
    return VOLC_OK;
}

int32_t iot_ota_set_download_complete_callback(iot_ota_handler_t *handle, iot_ota_download_complete_callback *callback, void *user_data) {
    CHECK_NULL_POINTER(handle);

    handle->ota_download_complete_callback = callback;
    handle->ota_download_complete_callback_user_data = user_data;
    return VOLC_OK;
}

int32_t iot_ota_set_rev_data_progress_callback(iot_ota_handler_t *handle, iot_ota_rev_data_progress_callback *callback, void *user_data) {
    CHECK_NULL_POINTER(handle);

    handle->iot_ota_rev_data_progress_callback = callback;
    handle->iot_ota_rev_data_progress_callback_user_data = user_data;
    return VOLC_OK;
}

int32_t iot_ota_set_auto_request_ota_info_interval_sec(iot_ota_handler_t *handle, int32_t interval) {
    CHECK_NULL_POINTER(handle);

    handle->auto_request_ota_info_interval_sec = interval;
    return VOLC_OK;
}

int32_t iot_ota_set_download_dir(iot_ota_handler_t *handle, const char *download_dir) {
    CHECK_NULL_POINTER(handle);
    if (handle->download_dir != NULL) {
        free(handle->download_dir);
    }
    char* dir = malloc(strlen(download_dir)+1);
    snprintf(dir, strlen(download_dir)+1, "%s", download_dir);
    // strcpy(dir, download_dir);
    handle->download_dir = dir;
    return VOLC_OK;
}

int32_t iot_ota_set_device_module_info(iot_ota_handler_t *handler, iot_ota_device_info_t *device_info_array, int device_info_array_size) {
    handler->device_info_array_size = device_info_array_size;
    if (handler->device_info_array != NULL) {
        free(handler->device_info_array);
    }
    handler->device_info_array = malloc(device_info_array_size * sizeof(iot_ota_device_info_t));
    memcpy(handler->device_info_array, device_info_array, device_info_array_size * sizeof(iot_ota_device_info_t));


    for (int i = 0; i < handler->device_info_array_size; i++) {
        iot_ota_device_info_t deviceInfo = handler->device_info_array[i];

        char kv_key_job_info[100];
        snprintf(kv_key_job_info, sizeof(kv_key_job_info), KEY_SAVE_JOB_INFO, deviceInfo.module);

        struct aws_byte_cursor key_cur = aws_byte_cursor_from_c_str(kv_key_job_info);
        struct aws_byte_cursor value = {0};
        iot_get_kv_string(handler->kv_ctx, key_cur, &value);  // 从kv中取出对应module的job info字节流（cursor）
        if (value.len > 0) {

            struct aws_json_value *data = aws_json_value_new_from_string(handler->allocator, value); // 将value转换成json
            iot_ota_job_info_t *jobInfo = _ota_data_json_to_ota_job_info(handler->allocator, data);  // 从json组装jobinfo
            aws_json_value_destroy(data);
            char kv_key_task_info[100];
            sprintf(kv_key_task_info, KEY_SAVE_TASK_INFO, jobInfo->ota_job_id); // key_save_task_info_ota-job-id
            struct aws_byte_cursor key_task_info_cur = aws_byte_cursor_from_c_str(kv_key_task_info);
            struct aws_byte_cursor task_info_value = {0};
            iot_get_kv_string(handler->kv_ctx, key_task_info_cur, &task_info_value); // 从kv中取出ota job id对应的字节流

            if (strcmp(jobInfo->dest_version, deviceInfo.version) == 0) {
                // 上报安装完成了
                iot_ota_report_progress_success(handler, jobInfo->ota_job_id, UpgradeDeviceStatusSuccess);
                iot_remove_key_str(handler->kv_ctx, kv_key_job_info);  // 从kv移除 job 和 task 信息
                iot_remove_key_str(handler->kv_ctx, kv_key_task_info);
                aws_mem_release(handler->allocator, jobInfo);
            } else {
                if (task_info_value.len > 0) {
                    // 缓存task info
                    struct aws_json_value *task_json = aws_json_value_new_from_string(handler->allocator, task_info_value);  // task_info_value变成json
                    iot_ota_job_task_info_t *task_info = malloc(sizeof(iot_ota_job_task_info_t)); // hash中释放
                    task_info->ota_handler = handler;
                    task_info->server_job_info = jobInfo;
                    task_info->decode_url = aws_json_get_string1_val(handler->allocator, task_json, "decode_url");
                    task_info->retry_time = (int32_t) aws_json_get_num_val(task_json, "retry_time");
                    task_info->is_pending_to_retry = true;
                    task_info->upgrade_device_status = (int) aws_json_get_num_val(task_json, "upgrade_device_status");
                    task_info->ota_file_path = aws_json_get_str(handler->allocator, task_json, "ota_file_path");
                    // 写入map
                    aws_hash_table_put(&handler->task_hash_map, jobInfo->ota_job_id, task_info, NULL);
                    // hashmap_set(handler->task_hash_map, task_info);
                }
                aws_mem_release(handler->allocator, jobInfo);
            }
        }
    }

    // 读取历史 task 写入缓存.
    // 上报 版本号
    iot_ota_report_version(handler, device_info_array, device_info_array_size);
    return 0;
}

void job_info_release(iot_ota_handler_t *handler, iot_ota_job_info_t *job_info) {
    if (job_info->ota_job_id != NULL) {
        aws_mem_release(handler->allocator, job_info->ota_job_id);
        job_info->ota_job_id = NULL;
    }
    if (job_info->dest_version != NULL) {
        aws_mem_release(handler->allocator, job_info->dest_version);
        job_info->dest_version = NULL;
    }
    if (job_info->url != NULL) {
        aws_mem_release(handler->allocator, job_info->url);
        job_info->url = NULL;
    }
    if (job_info->module != NULL) {
        aws_mem_release(handler->allocator, job_info->module);
        job_info->module = NULL;
    }
    if (job_info->sign != NULL) {
        aws_mem_release(handler->allocator, job_info->sign);
        job_info->sign = NULL;
    }
    aws_mem_release(handler->allocator, job_info);
}

void ota_task_release(iot_ota_job_task_info_t *task_info) {
    if (task_info->decode_url != NULL) {
        // aws_string_destroy_secure(task_info->decode_url);
        aws_string_destroy(task_info->decode_url);
        task_info->decode_url = NULL;
    }
    if (task_info->ota_file_path!= NULL) {
        free(task_info->ota_file_path);
        task_info->ota_file_path = NULL;
    }
    // download_handler 在下载完成后会自动回收
    aws_mem_release(task_info->ota_handler->allocator, task_info);
}


// static void s_ota_retry_task(struct aws_task *task, void *arg, enum aws_task_status status) {
//     iot_ota_job_task_info_t *ota_task = arg;
//     struct aws_allocator *allocator = ota_task->ota_handler->allocator;
//     iot_ota_start_download(ota_task->ota_handler, ota_task->server_job_info);
//     aws_mem_release(allocator, task);
// }

static void s_download_call_back(void *download_handler, struct http_download_file_response *response, void *user_data) {
    iot_http_download_handler_t *handler = download_handler;
    iot_ota_job_task_info_t *ota_task = user_data;
    LOGD(TAG_OTA, "s_download_call_back download_file_path = %s", handler->download_file_path);

    int ret = 0;
    // if (response->error_code == VOLC_OK) {
        if (handler->download_file_size != ota_task->server_job_info->size) {
            ret = VOLC_ERR_OTA_DOWNLOAD_FILE_SIZE;
            goto result_handler;
        }

        if (ota_task->server_job_info->sign != NULL && strlen(ota_task->server_job_info->sign) > 0) {
            FILE *fp = fopen(handler->download_file_path, "rb");
            struct aws_string *md5_str = md5File(fp);
            bool is_sign_eq = aws_string_eq_c_str(md5_str, ota_task->server_job_info->sign);
            LOGD(TAG_OTA, "md5_str = %s", aws_string_c_str(md5_str));
            LOGD(TAG_OTA, "ota_task->server_job_info->sign = %s", ota_task->server_job_info->sign);
            LOGD(TAG_OTA, "is_sign_eq = %d", is_sign_eq);
            fclose(fp);
            aws_string_destroy_secure(md5_str);
            if (md5_str == NULL || !is_sign_eq) {
                ret = VOLC_ERR_OTA_DOWNLOAD_FILE_SIGN_CHECK;
                goto result_handler;
            }
        }
    // }

result_handler:
    LOGD(TAG_OTA, "s_download_call_back result_handler ret = %d", ret);
    if (ret != VOLC_OK) {
        if (ota_task->ota_handler->ota_download_complete_callback != NULL) {
            ota_task->ota_handler->ota_download_complete_callback(ota_task->ota_handler,
                                                                  ret,
                                                                  ota_task->server_job_info,
                                                                  NULL,
                                                                  ota_task->ota_handler->ota_download_complete_callback_user_data);

        }
        // 上报下载失败
        iot_ota_report_progress_failed(ota_task->ota_handler, ota_task->server_job_info->ota_job_id, UpgradeDeviceStatusFailed, ret, "download failed");
        LOGD(TAG_OTA, "s_download_call_back 彻底失败");
    } else {
        // 上报下载成功
        iot_ota_report_progress_success(ota_task->ota_handler, ota_task->server_job_info->ota_job_id, UpgradeDeviceStatusDownloaded);
        iot_ota_save_job_task_info(ota_task);
        if (ota_task->ota_handler->ota_download_complete_callback != NULL) {
            ota_task->ota_handler->ota_download_complete_callback(ota_task->ota_handler,
                                                                  ret,
                                                                  ota_task->server_job_info,
                                                                  handler->download_file_path,
                                                                  ota_task->ota_handler->ota_download_complete_callback_user_data);
        }
    }
    iot_ota_delete_job_task_info(ota_task);
    aws_hash_table_remove(&ota_task->ota_handler->task_hash_map, ota_task->server_job_info->ota_job_id, NULL, NULL);
    job_info_release(ota_task->ota_handler, ota_task->server_job_info);
    ota_task_release(ota_task);
}

static void on_data_received(const char *body, size_t len, bool last_chunk, void *user_data) {
    LOGD(TAG_OTA, "on_data_received len = %d", len);
    iot_ota_job_task_info_t *ota_task = user_data;
	ota_task->downloaded_size += len;
	uint32_t percent = 100 * ota_task->downloaded_size / ota_task->download_file_size;
	if (ota_task->ota_handler->iot_ota_rev_data_progress_callback != NULL) {
        ota_task->ota_handler->iot_ota_rev_data_progress_callback(ota_task->ota_handler,
                                                                  ota_task->server_job_info,
                                                                  (uint8_t*)body, len, percent,
                                                                  ota_task->ota_handler->iot_ota_rev_data_progress_callback_user_data);
    }
}

static void on_download_error(int error_code,char *msg, void *user_data) {
	iot_ota_job_task_info_t *ota_task = user_data;
	// 删除文件
	delete_fw_info_file(ota_task->ota_file_path);
	// 上报下载失败
	iot_ota_report_progress_failed(ota_task->ota_handler, ota_task->server_job_info->ota_job_id, UpgradeDeviceStatusFailed, error_code, "download failed");
	LOGD(TAG_OTA, "on_download_error 彻底失败: %s", msg);
	// 删除任务
	iot_ota_delete_job_task_info(ota_task);
    aws_hash_table_remove(&ota_task->ota_handler->task_hash_map, ota_task->server_job_info->ota_job_id, NULL, NULL);
    job_info_release(ota_task->ota_handler, ota_task->server_job_info);
    ota_task_release(ota_task);
}

static void on_download_complete(void *cb_user_data) {
	LOGD(TAG_OTA, "on_download_complete");
	iot_ota_job_task_info_t *ota_task = cb_user_data;
	// 判断文件md5码
	if (ota_task->server_job_info->sign != NULL && strlen(ota_task->server_job_info->sign) > 0) {
		FILE *fp = fopen(ota_task->ota_file_path, "rb");
		struct aws_string *md5_str = md5File(fp);
		bool is_sign_eq = aws_string_eq_c_str(md5_str, ota_task->server_job_info->sign);
		LOGD(TAG_OTA, "md5_str = %s", aws_string_c_str(md5_str));
		LOGD(TAG_OTA, "ota_task->server_job_info->sign = %s", ota_task->server_job_info->sign);
		LOGD(TAG_OTA, "is_sign_eq = %d", is_sign_eq);
		fclose(fp);
		aws_string_destroy_secure(md5_str);
		if (md5_str == NULL || !is_sign_eq) {
			// 删除文件
			delete_fw_info_file(ota_task->ota_file_path);
			// 上报下载失败
			iot_ota_report_progress_failed(ota_task->ota_handler, ota_task->server_job_info->ota_job_id, 
				UpgradeDeviceStatusFailed, VOLC_ERR_OTA_DOWNLOAD_FILE_SIGN_CHECK, "download failed");
			LOGD(TAG_OTA, "on_download_error 彻底失败");
		} else {
            // 上报下载成功
            iot_ota_report_progress_success(ota_task->ota_handler, ota_task->server_job_info->ota_job_id, UpgradeDeviceStatusDownloaded);
            iot_ota_save_job_task_info(ota_task);
            if (ota_task->ota_handler->ota_download_complete_callback != NULL) {
                ota_task->ota_handler->ota_download_complete_callback(ota_task->ota_handler,
                                                                    VOLC_OK,
                                                                    ota_task->server_job_info,
                                                                    ota_task->ota_file_path,
                                                                    ota_task->ota_handler->ota_download_complete_callback_user_data);
            }
            LOGD(TAG_OTA, "on_download_complete 下载完成");
        }
	}
    // 删除任务
    iot_ota_delete_job_task_info(ota_task);
    aws_hash_table_remove(&ota_task->ota_handler->task_hash_map, ota_task->server_job_info->ota_job_id, NULL, NULL);
    job_info_release(ota_task->ota_handler, ota_task->server_job_info);
    ota_task_release(ota_task);
    return;

}

void iot_ota_start_download(iot_ota_handler_t *handle, iot_ota_job_info_t *job_info) {
    // 判断是否有正在进行的任务?
    struct aws_hash_element *value_elem = NULL;
    aws_hash_table_find(&handle->task_hash_map, job_info->ota_job_id, &value_elem);
    iot_ota_job_task_info_t *last_task = NULL;
    if (value_elem != NULL) {
        last_task = (iot_ota_job_task_info_t *) value_elem->value;
        LOGD(TAG_OTA, "exist last_task:%p in hashmap", last_task);
    }
    if (last_task != NULL) {
        if (last_task->ota_file_path != NULL) {
            if (last_task->server_job_info->sign != NULL && strlen(last_task->server_job_info->sign) > 0 && access((const char*)last_task->ota_file_path, F_OK)==0 ) {
                FILE *fp = fopen(last_task->ota_file_path, "rb");
                struct aws_string *md5_str = md5File(fp);
                fclose(fp);
                bool is_sign_eq = aws_string_eq_c_str(md5_str, last_task->server_job_info->sign);
                aws_string_destroy_secure(md5_str);
                if (md5_str != NULL && is_sign_eq) {
                    // 之前的任务下载完了, 且文件校验成功 直接回调下载完成
                    if (handle->ota_download_complete_callback != NULL) {
                        handle->ota_download_complete_callback(handle, 0, job_info, last_task->ota_file_path, handle->ota_download_complete_callback_user_data);
                        return;
                    }
                }
            }

            if (!last_task->is_pending_to_retry) {
                // 任务正在进行
                return;
            }
        }
    }

    if (job_info->url == NULL || job_info->size <= 0) {
        return;
    }

    // 读取是否存在历史 task 是否下载完成, 文件签名校验是否通过, 如果通过的话, 无需再次下载.

    //  创建新的task
    iot_ota_job_task_info_t *ota_task = aws_mem_calloc(handle->allocator, 1, sizeof(iot_ota_job_task_info_t));
    ota_task->ota_handler = handle;
    ota_task->server_job_info = job_info;
    ota_task->retry_time = 0;
    ota_task->is_pending_to_retry = false;
    if (last_task != NULL) {
        ota_task->retry_time = last_task->retry_time + 1;
    } else {
        ota_task->retry_time = 0;
    }

    // url 解密
    ota_task->decode_url = aws_string_new_from_c_str(handle->allocator, aes_decode(handle->allocator, 
        handle->mqtt_handle->config->basic_config->device_secret, job_info->url, true));
	char *file_name = extract_filename(aws_string_c_str(ota_task->decode_url));
    ota_task->ota_file_path = concat_const_strings(handle->download_dir, file_name);
    LOGD(TAG_OTA, "ota_task->ota_file_path = %s", ota_task->ota_file_path);
    LOGD(TAG_OTA, "file_name = %s", file_name);
    LOGD(TAG_OTA, "dir = %s", handle->download_dir);
    
	http_request_context_t *http_ctx = new_http_ctx();
    http_ctx_set_ca_crt(http_ctx, handle->mqtt_handle->config->basic_config->ssl_ca_cert);
    http_ctx_set_url(http_ctx, (char*)aws_string_c_str(ota_task->decode_url));
    http_ctx_set_method(http_ctx, HTTP_GET);
    http_ctx_set_timeout_mil(http_ctx, 10000); // TODO：10 seconds for transferring data
    http_ctx_set_on_get_body_cb(http_ctx, on_data_received, ota_task); // 保存到文件，上报下载中状态
    http_ctx_set_on_error_cb(http_ctx, on_download_error, ota_task); // 删除文件，上报下载失败状态
    http_ctx_set_on_complete_cb(http_ctx, on_download_complete, ota_task); // 上报下载完成状态
	
	http_ctx_download_init(http_ctx, ota_task->ota_file_path, 0);

	int ret = http_request_async(http_ctx);
	if (ret != 0) {
		LOGD(TAG_OTA, "http_request_async failed");
		http_ctx_release(http_ctx);
		return;
	} 
	http_wait_complete(http_ctx);
    http_ctx_release(http_ctx);
    free(file_name);
    
    // // 开始下载
    // iot_http_download_handler_t *download_handler = new_http_download_handler();
    // ota_task->download_handler = download_handler;
    // http_download_handler_set_url(download_handler, aws_string_c_str(ota_task->decode_url));
    // if (handle->download_dir != NULL) {
    //     http_download_handler_set_file_download_dir(download_handler, handle->download_dir);
    // } else {
    //     http_download_handler_set_file_download_dir(download_handler, "download");
    // }

    // http_download_handler_set_file_download_path(download_handler, ota_task->ota_file_path);
    // http_download_handler_set_file_size(download_handler, job_info->size);

    // http_download_handler_set_download_callback(download_handler, s_download_call_back, ota_task);
    // http_download_handler_set_rev_data_callback(download_handler, s_download_rev_data_callback, ota_task);
    // http_download_start(download_handler);
    // ota_task->upgrade_device_status = UpgradeDeviceStatusDownloading;
    //
    // // 上报下载中
    // iot_ota_report_progress_success(handle, job_info->ota_job_id, UpgradeDeviceStatusDownloading);

    // aws_hash_table_put(&handle->task_hash_map, job_info->ota_job_id, ota_task, NULL);
    // // hashmap_set(handle->task_hash_map, ota_task);
    // if (last_task != NULL) {
    //     // 销毁上一个task
    //     ota_task_release(last_task);
    // }
}

// static void _ota_mqtt_event_callback(void *pclient, MQTTEventType event_type, void *user_data)
// {
//     // OTA_MQTT_Struct_t *h_osc = (OTA_MQTT_Struct_t *)user_data;

//     switch (event_type) {
//         case MQTT_EVENT_SUBCRIBE_SUCCESS:
//             LOGD(TAG_OTA, "OTA topic subscribe success");
//         // h_osc->topic_ready = true;
//         break;

//         case MQTT_EVENT_SUBCRIBE_TIMEOUT:
//             LOGI(TAG_OTA, "OTA topic subscribe timeout");
//         // h_osc->topic_ready = false;
//         break;

//         case MQTT_EVENT_SUBCRIBE_NACK:
//             LOGI(TAG_OTA, "OTA topic subscribe NACK");
//         // h_osc->topic_ready = false;
//         break;
//         case MQTT_EVENT_UNSUBSCRIBE:
//             LOGI(TAG_OTA, "OTA topic has been unsubscribed");
//         // h_osc->topic_ready = false;
//         ;
//         break;
//         case MQTT_EVENT_CLIENT_DESTROY:
//             LOGI(TAG_OTA, "mqtt client has been destroyed");
//         // h_osc->topic_ready = false;
//         ;
//         break;
//         default:
//             return;
//     }
// }

void _mqtt_sub_ota_info(iot_ota_handler_t *ota_handler) {
    if (ota_handler->mqtt_handle == NULL) {
        return;
    }

    struct aws_string *product_key = aws_string_new_from_c_str(ota_handler->allocator, 
        ota_handler->mqtt_handle->config->basic_config->product_key);
    struct aws_string *device_name = aws_string_new_from_c_str(ota_handler->allocator, 
        ota_handler->mqtt_handle->config->basic_config->device_name);
    char *topic = iot_get_common_topic(ota_handler->allocator, OTA_UPGRADE_NOTIFY_TOPIC, 
        product_key, device_name);

    iot_mqtt_subscribe(ota_handler->mqtt_handle, &(iot_mqtt_topic_map_t){
        .topic = topic,
        .message_callback = _ota_rev_ota_notify_info,
        // .event_callback = _ota_mqtt_event_callback;
        .user_data = (void*)ota_handler,
        .qos = IOT_MQTT_QOS1,
    });

    aws_mem_release(ota_handler->allocator, topic);
    aws_string_destroy_secure(product_key);
    aws_string_destroy_secure(device_name);
}

void _mqtt_sub_ota_upgrade_post_reply(iot_ota_handler_t *ota_handler) {
    if (ota_handler->mqtt_handle == NULL) {
        return;
    }

    struct aws_string *product_key = aws_string_new_from_c_str(ota_handler->allocator, 
        ota_handler->mqtt_handle->config->basic_config->product_key);
    struct aws_string *device_name = aws_string_new_from_c_str(ota_handler->allocator, 
        ota_handler->mqtt_handle->config->basic_config->device_name);
    char *topic = iot_get_common_topic(ota_handler->allocator, OTA_REQUEST_UPGRADE_REPLY_TOPIC, 
        product_key, device_name);    
    
    iot_mqtt_subscribe(ota_handler->mqtt_handle, &(iot_mqtt_topic_map_t){
        .topic = topic,
        .message_callback = _ota_rev_ota_upgrade_post_reply,
        // .event_callback = _ota_mqtt_event_callback;
        .user_data = (void*)ota_handler,
        .qos = IOT_MQTT_QOS1,
    });

    aws_mem_release(ota_handler->allocator, topic);
    aws_string_destroy_secure(product_key);
    aws_string_destroy_secure(device_name);
}

void _ota_rev_ota_notify_info(const char* topic, const uint8_t *payload, size_t len, void *user_data) {
    struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
    struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

    iot_ota_handler_t *ota_handler = user_data;

    LOGI(TAG_OTA, "_ota_rev_ota_notify_info topic = %.*s  payload = %.*s",
         AWS_BYTE_CURSOR_PRI(topic_byte_cursor),
         AWS_BYTE_CURSOR_PRI(payload_byte_cursor));

    //  sys/6476ee400c910eb5b69001a6/skin_left/ota/notify/639b2507886c5549e2afcb08
    //  payload = {"id":"d72ed64a221215220312","code":0,"data":{"type":"Upgrade","module":"default","dest_version":"1.1.0"}}

//    注意这里 ota_jobId 是来自于 topic
    struct aws_array_list topic_split_data_list;
    aws_array_list_init_dynamic(&topic_split_data_list, ota_handler->allocator, 8, sizeof(struct aws_byte_cursor));
    aws_byte_cursor_split_on_char(&topic_byte_cursor, '/', &topic_split_data_list);

    struct aws_byte_cursor ota_job_id_cur = {0};
    aws_array_list_get_at(&topic_split_data_list, &ota_job_id_cur, 5);

    struct aws_json_value *payloadJson = aws_json_value_new_from_string(ota_handler->allocator, payload_byte_cursor);
    // 这里的id
    struct aws_string *ota_job_id = aws_string_new_from_cursor(ota_handler->allocator, &ota_job_id_cur);
    struct aws_json_value *data_json = aws_json_value_get_from_object(payloadJson, aws_byte_cursor_from_c_str("data"));
    if (data_json != NULL) {
        struct aws_string *module = aws_json_get_string1_val(ota_handler->allocator, data_json, "module");
        struct aws_string *type = aws_json_get_string1_val(ota_handler->allocator, data_json, "type");
        if (aws_string_eq_c_str(type, "Upgrade")) {
            // 基于 id 请求详细信息
            for (int i = 0; i < ota_handler->device_info_array_size; i++) {
                iot_ota_device_info_t deviceInfo = ota_handler->device_info_array[i];
                if (aws_string_eq_c_str(module, deviceInfo.module)) {
                    iot_ota_request_ota_job_info(ota_handler, &deviceInfo, (char *)aws_string_c_str(ota_job_id));
                }
            }
        }
        aws_string_destroy_secure(module);
        aws_string_destroy_secure(type);
    }
    aws_string_destroy_secure(ota_job_id);
    aws_json_value_destroy(payloadJson);
    aws_array_list_clean_up(&topic_split_data_list);
}

void _ota_rev_ota_upgrade_post_reply(const char* topic, const uint8_t *payload, size_t len, void *user_data) {
    struct aws_byte_cursor topic_byte_cursor = aws_byte_cursor_from_array(topic, strlen(topic));
    struct aws_byte_cursor payload_byte_cursor = aws_byte_cursor_from_array(payload, len);

    LOGI(TAG_OTA, "_ota_rev_ota_upgrade_post_reply topic = %.*s  payload = %.*s",
         AWS_BYTE_CURSOR_PRI(topic_byte_cursor),
         AWS_BYTE_CURSOR_PRI(payload_byte_cursor));
    iot_ota_handler_t *ota_handler = user_data;

//    topic = sys/6476ee400c910eb5b69001a6/skin_left/ota/upgrade/post_reply
//    payload = {"id":"2371671114253805","code":0,"data":{"ota_job_id":"639b2507886c5549e2afcb08",
//    "timeout_in_minutes":100,"size":9901544,"dest_version":"1.1.0",
//    "url":"5cM4KIrD/6h60ezkD7ak74JkSCUrZGm6Ia3iv0u9WIn79+HgCRX5tNO9eb2wCYIJtL5urmTIYCjAj/pkZ03YTHV+L7DeWtdFkwO0zlBHNjDQbABd7Isf0cEXoXkSteMOioR6OGveEKKBhRTaCJB5uKOdJFTPH1P3fsSOwUHEWJHsOMsHaZWY9Ye8tXNkNg8HMEeWdkf0p1+JIF2SfpDz6IJLBG2cUeByCReL5rikqLC304lhLAe6nCMlKcvFZb34DURTFEK3K0y77cbjfbPHEYAbHtvhhANmAsEZTuXyE0LXOelHygZ2sGALH/tHriMUo6fsLEabmpnhT+u1QRbVHjLy/HX7BE2BtcGhByGdOqUDyUoKqmKMm+Hiv52pX/MybhwWh1DsJ0lq+B+m1QL+jfzBHCR95klpAOJNdA45n0NP8Rixw0jk3RqxcfxUDFLiG7wPlLNkLpgATy0ffun/ooJj5+JhRrksP8AWxa1hSGAZGTbv1Eb61wLa594mDGMN2pUEr5MkKcZSk+4HeRiMz5tS2LWPQcwnuRo8D2f8L24=",
//    "module":"default"}}

    struct aws_json_value *payloadJson = aws_json_value_new_from_string(ota_handler->allocator, payload_byte_cursor);
    // 这里的id
    struct aws_json_value *data_json = aws_json_value_get_from_object(payloadJson, aws_byte_cursor_from_c_str("data"));
    if (data_json != NULL) {
        iot_ota_job_info_t *ota_job_info = _ota_data_json_to_ota_job_info(ota_handler->allocator, data_json);
        if (ota_job_info->url != NULL && ota_job_info->size > 0) {
            char kv_key[100];
            sprintf(kv_key, KEY_SAVE_JOB_INFO, ota_job_info->module);
            struct aws_byte_buf data_json_buf = aws_json_obj_to_byte_buf(ota_handler->allocator, data_json);
            struct aws_byte_cursor key_cur = aws_byte_cursor_from_c_str(kv_key);
            iot_add_kv_string(ota_handler->kv_ctx, key_cur, aws_byte_cursor_from_buf(&data_json_buf));

            if (ota_handler->get_job_info_callback != NULL) {
                ota_handler->get_job_info_callback(ota_handler, ota_job_info, ota_handler->get_job_info_callback_user_data);
            }
        }
    }
    aws_json_value_destroy(payloadJson);

}


iot_ota_job_info_t *_ota_data_json_to_ota_job_info(struct aws_allocator *allocator, struct aws_json_value *data_json) {
    iot_ota_job_info_t *ota_job_info = aws_mem_calloc(allocator, 1, sizeof(iot_ota_job_info_t));
    ota_job_info->ota_job_id = aws_json_get_str(allocator, data_json, "ota_job_id");
    ota_job_info->dest_version = aws_json_get_str(allocator, data_json, "dest_version");
    ota_job_info->module = aws_json_get_str(allocator, data_json, "module");
    ota_job_info->url = aws_json_get_str(allocator, data_json, "url");
    ota_job_info->timeout_in_minutes = (int32_t) aws_json_get_num_val(data_json, "timeout_in_minutes");
    ota_job_info->size = (uint64_t) aws_json_get_num_val(data_json, "size");
    ota_job_info->sign = aws_json_get_str(allocator, data_json, "sign");
    LOGD(TAG_OTA, "ota_job_info ota_job_id = %s", ota_job_info->ota_job_id);
    LOGD(TAG_OTA, "ota_job_info dest_version = %s", ota_job_info->dest_version);
    LOGD(TAG_OTA, "ota_job_info module = %s", ota_job_info->module);
    LOGD(TAG_OTA, "ota_job_info url = %s", ota_job_info->url);
    LOGD(TAG_OTA, "ota_job_info timeout_in_minutes = %d", ota_job_info->timeout_in_minutes);
    LOGD(TAG_OTA, "ota_job_info size = %llu", ota_job_info->size);
    LOGD(TAG_OTA, "ota_job_info sign = %s", ota_job_info->sign);
    return ota_job_info;
}

int32_t iot_ota_request_ota_job_info(iot_ota_handler_t *handler, iot_ota_device_info_t *device_info, char *job_id) {
    CHECK_NULL_POINTER(handler);
    CHECK_NULL_POINTER(device_info);

    struct aws_json_value *params_json = aws_json_value_new_object(handler->allocator);
    if (job_id != NULL) {
        aws_json_add_str_val_1(handler->allocator, params_json, "ota_job_id", job_id);
    }
    aws_json_add_str_val(params_json, "module", device_info->module);
    aws_json_add_str_val(params_json, "src_version", device_info->version);
    struct aws_json_value *payload_json = new_request_payload(handler->allocator, SDK_VERSION, params_json);

    struct aws_string *product_key = aws_string_new_from_c_str(handler->allocator, 
        handler->mqtt_handle->config->basic_config->product_key);
    struct aws_string *device_name = aws_string_new_from_c_str(handler->allocator, 
        handler->mqtt_handle->config->basic_config->device_name);
    char *topic = iot_get_common_topic(handler->allocator, OTA_REQUEST_UPGRADE_TOPIC, 
        product_key, device_name);
    struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(handler->allocator, payload_json);
    struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
    LOGD(TAG_OTA, "iot_ota_request_ota_job_info payload_cur = %.*s ", AWS_BYTE_CURSOR_PRI(payload_cur));

    iot_mqtt_publish(handler->mqtt_handle, topic, (uint8_t *)payload_cur.ptr, payload_cur.len, IOT_MQTT_QOS1);

    aws_mem_release(handler->allocator, topic);
    aws_byte_buf_clean_up(&payload_buf);
    aws_json_value_destroy(payload_json);
    aws_string_destroy(product_key);
    aws_string_destroy(device_name);
    return VOLC_OK;
}

/**
 * 上报版本号, 同时也是 OTA完成的上报, 判断OTA升级是否成功, 是否到了新版本
 * @param handler
 * @param device_info_array
 * @param device_info_array_size
 * @return
 */
int32_t iot_ota_report_version(iot_ota_handler_t *handler, iot_ota_device_info_t *device_info_array, int device_info_array_size) {
    if (handler == NULL || device_info_array == NULL || device_info_array_size <= 0) {
        return VOLC_ERR_NULL_POINTER;
    }

    struct aws_json_value *version_data_json = aws_json_value_new_object(handler->allocator);
    for (int i = 0; i < device_info_array_size; i++) {
        iot_ota_device_info_t deviceInfo = device_info_array[i];
        aws_json_add_str_val(version_data_json, deviceInfo.module, deviceInfo.version);
    }

    struct aws_json_value *payload_json = new_request_payload(handler->allocator, SDK_VERSION, version_data_json);
    struct aws_string *product_key = aws_string_new_from_c_str(handler->allocator, 
        handler->mqtt_handle->config->basic_config->product_key);
    struct aws_string *device_name = aws_string_new_from_c_str(handler->allocator, 
        handler->mqtt_handle->config->basic_config->device_name);
    char *topic = iot_get_common_topic(handler->allocator, OTA_VERSION_REPORT_TOPIC, 
        product_key, device_name);
    // struct aws_byte_cursor topic_cur = aws_byte_cursor_from_c_str(topic);
    struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(handler->allocator, payload_json);
    struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
    LOGD(TAG_OTA, "iot_ota_report_version payload_cur = %.*s ", AWS_BYTE_CURSOR_PRI(payload_cur));

    iot_mqtt_publish(handler->mqtt_handle, topic, (uint8_t *)payload_cur.ptr, payload_cur.len, IOT_MQTT_QOS1);

    aws_mem_release(handler->allocator, topic);
    aws_byte_buf_clean_up(&payload_buf);
    aws_json_value_destroy(payload_json);
    aws_string_destroy(product_key);
    aws_string_destroy(device_name);
    return VOLC_OK;
}

int32_t iot_ota_report_progress_success(iot_ota_handler_t *handler, char *jobId, enum ota_upgrade_device_status_enum upgrade_device_status) {
    ota_process_status_t status = {
            .upgrade_device_status = upgrade_device_status,
    };
    return iot_ota_report_progress(handler, jobId, &status);
}

int32_t iot_ota_report_progress_failed(iot_ota_handler_t *handler, char *jobId, enum ota_upgrade_device_status_enum upgrade_device_status, int32_t error_code, char *result_desc) {
    ota_process_status_t status = {
            .upgrade_device_status = upgrade_device_status,
            .error_code = error_code,
            .result_desc =result_desc
    };
    return iot_ota_report_progress(handler, jobId, &status);
}


int32_t iot_ota_report_installing(iot_ota_handler_t *handler, char *jobId) {
    ota_process_status_t status = {
            .upgrade_device_status = UpgradeDeviceStatusInstalling,
    };
    return iot_ota_report_progress(handler, jobId, &status);
}

int32_t iot_ota_report_install_success(iot_ota_handler_t *handler, char *jobId) {
    ota_process_status_t status = {
            .upgrade_device_status = UpgradeDeviceStatusInstalled,
    };
    return iot_ota_report_progress(handler, jobId, &status);
}

int32_t iot_ota_report_install_failed(iot_ota_handler_t *handler, char *jobId, char *result_desc) {
    ota_process_status_t status = {
            .upgrade_device_status = UpgradeDeviceStatusFailed,
            .error_code = VOLC_ERR_OTA_INSTALL_FAILED,
            .result_desc =result_desc
    };

    struct aws_hash_element *value_elem = NULL;

    // 标记待重试
    aws_hash_table_find(&handler->task_hash_map, jobId, &value_elem);
    iot_ota_job_task_info_t *last_task = NULL;
    // iot_ota_job_task_info_t* last_task = (iot_ota_job_task_info_t *)hashmap_get(handler->task_hash_map, jobId);
    //if (last_task != NULL) {
        // last_task->is_pending_to_retry = true;
    //}
    if (value_elem != NULL) {
        last_task = (iot_ota_job_task_info_t *) value_elem->value;
        last_task->is_pending_to_retry = true;
    }
    return iot_ota_report_progress(handler, jobId, &status);
}


int32_t iot_ota_report_progress(iot_ota_handler_t *handler, char *jobId, ota_process_status_t *status) {
    if (handler == NULL || status == NULL || jobId == NULL) {
        return VOLC_ERR_NULL_POINTER;
    }
    struct aws_json_value *data_json = aws_json_value_new_object(handler->allocator);
    aws_json_add_str_val(data_json, "status", iot_ota_job_status_enum_to_string(status->upgrade_device_status));
    aws_json_add_num_val(data_json, "result_code", status->error_code);
    aws_json_add_str_val(data_json, "result_desc", status->result_desc);
    aws_json_add_num_val(data_json, "time", (double) get_current_time_mil());
    struct aws_json_value *payload_json = new_request_payload(handler->allocator, SDK_VERSION, data_json);

    struct aws_string *product_key = aws_string_new_from_c_str(handler->allocator,
        handler->mqtt_handle->config->basic_config->product_key);
    struct aws_string *device_name = aws_string_new_from_c_str(handler->allocator,
        handler->mqtt_handle->config->basic_config->device_name);
    char *topic = iot_get_topic_with_1_c_str_param(handler->allocator, OTA_UPGRADE_PROGRESS_REPORT_TOPIC, 
        product_key, device_name, jobId);
    // struct aws_byte_cursor topic_cur = aws_byte_cursor_from_c_str(topic);
    struct aws_byte_buf payload_buf = aws_json_obj_to_byte_buf(handler->allocator, payload_json);
    struct aws_byte_cursor payload_cur = aws_byte_cursor_from_buf(&payload_buf);
    LOGD(TAG_OTA, "iot_ota_report_progress payload_cur = %.*s ", AWS_BYTE_CURSOR_PRI(payload_cur));

    iot_mqtt_publish(handler->mqtt_handle, topic, (uint8_t *)payload_cur.ptr, payload_cur.len, IOT_MQTT_QOS1);

    aws_mem_release(handler->allocator, topic);
    aws_byte_buf_clean_up(&payload_buf);
    aws_json_value_destroy(payload_json);
    aws_string_destroy(product_key);
    aws_string_destroy(device_name);
    return VOLC_OK;
}

// static void s_auto_request_ota_info_task(struct aws_task *task, void *arg, enum aws_task_status status) {
//     iot_ota_handler_t *handler = arg;
//     request_device_job_info_inner(handler);
//     if (handler->auto_request_ota_info_interval_sec > 0) {
//         iot_core_post_delay_task(task, handler->auto_request_ota_info_interval_sec);
//     } else {
//         aws_mem_release(handler->allocator, task);
//     }
//
// }

void request_device_job_info_inner(iot_ota_handler_t *handler) {
    for (int i = 0; i < handler->device_info_array_size; i++) {
        iot_ota_device_info_t deviceInfo = handler->device_info_array[i];

        iot_ota_request_ota_job_info(handler, &deviceInfo, NULL);
    }
}

// int32_t iot_start_auto_request_ota_info(iot_ota_handler_t *handler) {
//     if (handler == NULL) {
//         return CODE_USER_INPUT_NULL_POINTER;
//     }
//     if (handler->device_info_array == NULL || handler->device_info_array_size <= 0) {
//         return OTA_DEVICE_INFO_NOT_SET;
//     }
//     request_device_job_info_inner(handler);
//
//     // 延迟重试
//     struct aws_task *retry_task = aws_mem_acquire(handler->allocator, sizeof(struct aws_task));
//     aws_task_init(retry_task, s_auto_request_ota_info_task, handler, "retry_ota_task");
//     iot_core_post_delay_task(retry_task, handler->auto_request_ota_info_interval_sec);
//     return 0;
// }


void iot_ota_save_job_task_info(iot_ota_job_task_info_t *task_info) {
    LOGD(TAG_OTA, "iot_ota_save_job_task_info started");
    struct aws_json_value *task_json = aws_json_value_new_object(task_info->ota_handler->allocator);
    aws_json_add_str_val(task_json, "ota_file_path", task_info->ota_file_path);
    aws_json_add_aws_string_val1(task_info->ota_handler->allocator, task_json, "decode_url", task_info->decode_url);
    aws_json_add_num_val1(task_info->ota_handler->allocator, task_json, "retry_time", task_info->retry_time);
    aws_json_add_num_val1(task_info->ota_handler->allocator, task_json, "upgrade_device_status", task_info->upgrade_device_status);
    aws_json_add_aws_string_val1(task_info->ota_handler->allocator, task_json, "ota_file_path",
        aws_string_new_from_c_str(task_info->ota_handler->allocator,
            task_info->ota_file_path));

    char kv_key[100];
    sprintf(kv_key, KEY_SAVE_TASK_INFO, task_info->server_job_info->ota_job_id);
    struct aws_byte_buf data_json_buf = aws_json_obj_to_byte_buf(task_info->ota_handler->allocator, task_json);
    struct aws_byte_cursor key_cur = aws_byte_cursor_from_c_str(kv_key);
    iot_add_kv_string(task_info->ota_handler->kv_ctx, key_cur, aws_byte_cursor_from_buf(&data_json_buf));

    aws_byte_buf_clean_up(&data_json_buf);
    aws_json_value_destroy(task_json);

    LOGD(TAG_OTA, "iot_ota_save_job_task_info finished");
}


void iot_ota_delete_job_task_info(iot_ota_job_task_info_t *task_info) {
    char kv_key[100];
    sprintf(kv_key, KEY_SAVE_TASK_INFO, task_info->server_job_info->ota_job_id);
    iot_remove_key_str(task_info->ota_handler->kv_ctx, kv_key);
}
#endif