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

#include "iot/iot_ota_utils.h"
#include "iot_log.h"
#include "aws/common/string.h"
#include "error_code.h"

#define TAG_OTA "ota"

static const char *s_ota_upgrade_device_status[UpgradeDeviceStatusCount] = {"ToUpgrade",
                                                                            "Downloading",
                                                                            "Downloaded",
                                                                            "DiffRecovering",
                                                                            "DiffRecovered",
                                                                            "Installing",
                                                                            "Installed",
                                                                            "Success",
                                                                            "Failed"};


const char *iot_ota_job_status_enum_to_string(enum ota_upgrade_device_status_enum status) {
    return s_ota_upgrade_device_status[status];
}

enum ota_upgrade_device_status_enum iot_ota_job_status_str_to_status_enum(struct aws_string *status_str) {
    if (aws_string_eq_c_str(status_str, s_ota_upgrade_device_status[UpgradeDeviceStatusDownloading])) {
        return UpgradeDeviceStatusDownloading;
    } else if (aws_string_eq_c_str(status_str, s_ota_upgrade_device_status[UpgradeDeviceStatusDownloaded])) {
        return UpgradeDeviceStatusDownloaded;
    } else if (aws_string_eq_c_str(status_str, s_ota_upgrade_device_status[UpgradeDeviceStatusInstalling])) {
        return UpgradeDeviceStatusInstalling;
    } else if (aws_string_eq_c_str(status_str, s_ota_upgrade_device_status[UpgradeDeviceStatusInstalled])) {
        return UpgradeDeviceStatusInstalled;
    } else if (aws_string_eq_c_str(status_str, s_ota_upgrade_device_status[UpgradeDeviceStatusSuccess])) {
        return UpgradeDeviceStatusSuccess;
    } else if (aws_string_eq_c_str(status_str, s_ota_upgrade_device_status[UpgradeDeviceStatusDiffRecovering])) {
        return UpgradeDeviceStatusDiffRecovering;
    } else if (aws_string_eq_c_str(status_str, s_ota_upgrade_device_status[UpgradeDeviceStatusDiffRecovered])) {
        return UpgradeDeviceStatusDiffRecovered;
    } else if (aws_string_eq_c_str(status_str, s_ota_upgrade_device_status[UpgradeDeviceStatusFailed])) {
        return UpgradeDeviceStatusFailed;
    } else {
        return UpgradeDeviceStatusFailed;
    }
}

char* extract_filename(const char* url) {
    const char* filename_start = strrchr(url, '/'); // 找到最后一个 '/' 的位置
    if (filename_start == NULL) {
        return NULL; // 如果没有找到 '/'，返回NULL
    }

    const char* filename_end = strchr(filename_start, '?'); // 找到第一个 '?' 的位置
    if (filename_end == NULL) {
        // 如果没有找到 '?'，则提取从 '/' 之后的所有内容
        return strdup(filename_start + 1);
    }

    // 计算文件名的长度
    size_t filename_length = filename_end - (filename_start + 1);
    char* filename = (char*)malloc(filename_length + 1);
    strncpy(filename, filename_start + 1, filename_length);
    filename[filename_length] = '\0'; // 确保字符串以 '\0' 结尾

    return filename;
}

char* concat_const_strings(const char* str1, const char* str2) {
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    char* result = (char*)malloc(len1 + len2 + 1); // 分配足够的内存

    if (result == NULL) {
        return NULL; // 内存分配失败
    }

    strcpy(result, str1); // 复制第一个字符串
    strcat(result, str2); // 拼接第二个字符串

    return result;
}

int delete_fw_info_file(const char *file_name)
{
    if (remove(file_name) != 0) {
        LOGE(TAG_OTA, "delete file(%s) failed", file_name);
        return VOLC_ERR_FILE_DELETE;
    }
    return VOLC_OK;
}
#endif //ONESDK_ENABLE_IOT