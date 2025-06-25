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

#include "iot_log.h"
#include "aws/common/array_list.h"
#include "aws/common/common.h"
#include "aws/common/date_time.h"
#include "util/util.h"

#define MAX_LOG_LINE_PREFIX_SIZE 150
#define LOG_SEND_MIN_INTERVAL_SEC 5
#define LOG_SEND_MIN_LOG_LINES 30


struct iot_log_ctx g_logCtx_internal;
struct iot_log_ctx *g_logCtx;

static const char *s_log_level_strings[COUNT] = {"NONE", "FATAL", "ERROR", "WARN", "INFO", "DEBUG"};

void _log_level_to_string(enum onesdk_log_level log_level, const char **level_string) {
    if (level_string != NULL) {
        *level_string = s_log_level_strings[log_level];
    }
}

enum onesdk_log_level _log_string_to_level(struct aws_byte_cursor *lowest_level_cur) {
    if (aws_byte_cursor_eq_c_str(lowest_level_cur, "debug") || aws_byte_cursor_eq_c_str(lowest_level_cur, "DEBUG")) {
        return DEBUG;
    } else if (aws_byte_cursor_eq_c_str(lowest_level_cur, "info") || aws_byte_cursor_eq_c_str(lowest_level_cur, "INFO")) {
        return INFO;
    } else if (aws_byte_cursor_eq_c_str(lowest_level_cur, "warn") || aws_byte_cursor_eq_c_str(lowest_level_cur, "WARN")) {
        return WARN;
    } else if (aws_byte_cursor_eq_c_str(lowest_level_cur, "error") || aws_byte_cursor_eq_c_str(lowest_level_cur, "ERROR")) {
        return ERROR;
    } else if (aws_byte_cursor_eq_c_str(lowest_level_cur, "fatal") || aws_byte_cursor_eq_c_str(lowest_level_cur, "FATAL")) {
        return FATAL;
    } else {
        return DEBUG;
    }
}

void iot_log_init(char *save_dir_path) {
    struct iot_log_ctx_option option;
    option.check_write_file_line_count = LOG_SEND_MIN_LOG_LINES;
    option.check_write_file_interval_sec = LOG_SEND_MIN_INTERVAL_SEC;
    if (save_dir_path == NULL || strlen(save_dir_path) == 0) {
        iot_log_init_with_option("iot_logs", option);
    } else {
        iot_log_init_with_option(save_dir_path, option);
    }
}

void iot_log_init_with_option(char *save_dir_path, struct iot_log_ctx_option option) {
    if (g_logCtx != NULL) {
        return;
    }

    g_logCtx = &g_logCtx_internal;
    AWS_ZERO_STRUCT(*g_logCtx);
    g_logCtx->save_dir_path = save_dir_path;
    g_logCtx->allocator = aws_alloc();
    aws_common_library_init(g_logCtx->allocator);
    g_logCtx->option = option;

    lws_pthread_mutex_init(&g_logCtx->sync);
    aws_array_list_init_dynamic(&g_logCtx->pending_log_lines, g_logCtx->allocator,
                                g_logCtx->option.check_write_file_line_count, sizeof(struct iot_log_obj *));

}

void iot_log_set_on_log_save_fn(iot_on_log_save_fn *on_save_fn, void *user_data) {
    if (g_logCtx == NULL) {
        return;
    }
    g_logCtx->on_log_save_fn = on_save_fn;
    g_logCtx->on_log_save_fn_user_data = user_data;
}

/**
 * 设置日志写入文件的触发 行数阈值, 超过这个阈值 则写入到文件
 * @param sec
 */
void log_set_check_write_file_line_count(int32_t count) {
    if (g_logCtx == NULL) {
        return;
    }
    g_logCtx->option.check_write_file_line_count = count;
}

char *iot_get_log_file_dir() {
    if (g_logCtx == NULL) {
        return NULL;
    }
    return g_logCtx->save_dir_path;
}

// 获取 log 文件路径
struct aws_string *iot_get_log_file_name() {
    char filename_array[1024];
    AWS_ZERO_ARRAY(filename_array);
    struct aws_byte_buf filename_buf = aws_byte_buf_from_empty_array(filename_array, sizeof(filename_array));

    aws_byte_buf_write_from_whole_cursor(&filename_buf, aws_byte_cursor_from_c_str(g_logCtx->save_dir_path));
    aws_byte_buf_write_from_whole_cursor(&filename_buf, aws_byte_cursor_from_c_str(AWS_PATH_DELIM_STR));
    aws_byte_buf_write_from_whole_cursor(&filename_buf, aws_byte_cursor_from_c_str("iot_"));
    // 获取时间字符串
    struct aws_date_time current_time;
    aws_date_time_init_now(&current_time);

    uint8_t date_output[AWS_DATE_TIME_STR_MAX_LEN];
    AWS_ZERO_ARRAY(date_output);
    struct aws_byte_buf str_output = aws_byte_buf_from_array(date_output, sizeof(date_output));
    str_output.len = 0;
    aws_date_time_to_local_time_short_str(&current_time, AWS_DATE_FORMAT_ISO_8601, &str_output);

    aws_byte_buf_write_from_whole_cursor(&filename_buf, aws_byte_cursor_from_buf(&str_output));

    aws_byte_buf_write_from_whole_cursor(&filename_buf, aws_byte_cursor_from_c_str(".log"));

    // 这里申请了内存, 会在 check_file 中销毁
    return aws_string_new_from_array(g_logCtx->allocator, filename_buf.buffer, filename_buf.len);
}

/**
 * 后台线程写入文件的等待条件,
 *
 * @param context
 * @return
 */
static bool s_background_wait(void *context) {
    if (g_logCtx == NULL) {
        return true;
    }
    struct iot_log_ctx *impl = (struct iot_log_ctx *) context;
    return impl->finished ||
        // 打到了最大数量
        aws_array_list_length(&impl->pending_log_lines) > g_logCtx->option.check_write_file_line_count ||
        // 到了最小间隔时间
        impl->isTimeToSendLog;
}

// 日志格式化
void _log_format(enum onesdk_log_level level, struct iot_log_obj *logObj, const char *logType, const char *tag,
    const char *format,
    va_list format_args) {
    va_list tmp_args;
    va_copy(tmp_args, format_args);
    #ifdef _WIN32
    int required_length = _vscprintf(format, tmp_args) + 10;
    #else
    int required_length = vsnprintf(NULL, 0, format, tmp_args) + 10;
    #endif
    va_end(tmp_args);

    struct aws_allocator *allocator = g_logCtx->allocator;


    // 这里创建合适大小的 string 对象
    int total_length = required_length + MAX_LOG_LINE_PREFIX_SIZE;
    struct aws_string *raw_string = aws_mem_calloc(allocator, 1, sizeof(struct aws_string) + total_length);
    logObj->logContent = raw_string;
    logObj->logType = logType;
    logObj->log_level = level;

    // 此时 log_line_buffer  为空
    char *log_line_buffer = (char *) raw_string->bytes;

    // 获取 level String
    const char *levelStr = NULL;
    _log_level_to_string(level, &levelStr);
    logObj->logLevelStr = levelStr;

    // 获取时间字符串
    struct aws_date_time current_time;
    aws_date_time_init_now(&current_time);
    logObj->time = aws_date_time_as_millis(&current_time);

    uint8_t date_output[AWS_DATE_TIME_STR_MAX_LEN];
    AWS_ZERO_ARRAY(date_output);
    struct aws_byte_buf str_output_buf = aws_byte_buf_from_array(date_output, sizeof(date_output));
    str_output_buf.len = 0;
    aws_date_time_to_local_time_str(&current_time, AWS_DATE_FORMAT_ISO_8601, &str_output_buf);

    // 写入日志前缀
    // FIXME
    snprintf(log_line_buffer, total_length, "[%s] [%s] [%.*s] %s : ", logType, levelStr,
    AWS_BYTE_BUF_PRI(str_output_buf), tag);
    // 释放申请的内存

    int curIndex = strlen(log_line_buffer);
    // 写入日志具体内容


    #ifdef _WIN32
    vsnprintf_s(log_line_buffer + curIndex, total_length - curIndex,  _TRUNCATE, format, format_args);
    #else
    vsnprintf(log_line_buffer + curIndex, total_length - curIndex, format, format_args);
    #endif /* _WIN32 */

    // 写入 换行符
    curIndex = strlen(log_line_buffer);
    snprintf(log_line_buffer + curIndex, total_length - curIndex, "\n");

    *(struct aws_allocator **) (&raw_string->allocator) = allocator;
    *(size_t *) (&raw_string->len) = strlen(log_line_buffer);

    // 输出到控制台
    printf("%s", log_line_buffer);
}

void _write_log(struct iot_log_ctx *logCtx) {
    if (g_logCtx == NULL) {
        return;
    }

    bool finished = logCtx->finished;
    if (finished) {
        lws_pthread_mutex_unlock(&logCtx->sync);
        return;
    }
    struct aws_array_list log_lines;
    aws_array_list_init_dynamic(&log_lines, logCtx->allocator, LOG_SEND_MIN_LOG_LINES, sizeof(struct iot_log_obj *));

    size_t line_count = aws_array_list_length(&logCtx->pending_log_lines);
    if (line_count == 0) {
        lws_pthread_mutex_unlock(&logCtx->sync);
        return;
    }

    g_logCtx->isTimeToSendLog = false;
    // 交换全局变量中的数据, 方便 pending_log_lines 可以继续写入数据
    // 这里需要注意锁的处理,  写入到list 也需要加锁才行, 不然数据会有问题, 具体看 s_background_channel_send
    aws_array_list_swap_contents(&logCtx->pending_log_lines, &log_lines);
    lws_pthread_mutex_unlock(&logCtx->sync);

    // 上报日志到服务端.
    if (g_logCtx->on_log_save_fn != NULL) {
        g_logCtx->on_log_save_fn(&log_lines, g_logCtx->on_log_save_fn_user_data);
    }

    for (size_t i = 0; i < line_count; ++i) {
        struct iot_log_obj *logObj = NULL;
        AWS_FATAL_ASSERT(aws_array_list_get_at(&log_lines, &logObj, i) == AWS_OP_SUCCESS);

        // 销毁对应的日志数据
        aws_string_destroy_secure(logObj->logContent);
        aws_mem_release(logCtx->allocator, logObj);
    }

    aws_array_list_clean_up(&log_lines);
}

int _s_background_channel_send(struct iot_log_ctx *impl, struct iot_log_obj *logObj) {
    // 写入数据到 list 也需要加锁, 防止多线程数据异常
    lws_pthread_mutex_lock(&impl->sync);
    aws_array_list_push_back(&impl->pending_log_lines, &logObj);
    // 处理日志
    _write_log(impl);
    lws_pthread_mutex_unlock(&impl->sync);
    return AWS_OP_SUCCESS;
}

void sdk_log_print(enum onesdk_log_level level, const char *logType, const char *tag, const char *format, ...) {
    va_list format_args;
    va_start(format_args, format);
    if (g_logCtx == NULL || g_logCtx->finished) {
        return;
    }
    struct aws_allocator *allocator = g_logCtx->allocator;
    struct iot_log_obj *logObj = aws_mem_calloc(allocator, 1, sizeof(struct iot_log_obj));

    // 日志格式化, 并写入到 logObj;
    _log_format(level, logObj, logType, tag, format, format_args);
    va_end(format_args);


    if (g_logCtx != NULL) {
        // 写入到 list
        _s_background_channel_send(g_logCtx, logObj);
    }
}

void iot_log_deinit() {
    g_logCtx->finished = true;
    size_t line_count = aws_array_list_length(&g_logCtx->pending_log_lines);
    for (size_t i = 0; i < line_count; ++i) {
        struct iot_log_obj *logObj = NULL;
        if (aws_array_list_get_at(&g_logCtx->pending_log_lines, &logObj, i) == AWS_OP_SUCCESS) {
            // 销毁对应的日志数据
            aws_string_destroy_secure(logObj->logContent);
            aws_mem_release(g_logCtx->allocator, logObj);
        }
    }
    aws_array_list_clean_up(&g_logCtx->pending_log_lines);
    aws_string_destroy_secure(g_logCtx->file_path);
    aws_mem_release(g_logCtx->allocator, g_logCtx->send_log_task);

    // if (g_logCtx != NULL) {
    //     aws_mem_release(g_logCtx->allocator, g_logCtx);
    // }
    // g_logCtx = NULL;
}
#endif