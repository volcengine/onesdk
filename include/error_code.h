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

#ifndef ONESDK_ERROR_CODE_H
#define ONESDK_ERROR_CODE_H

/*

位域	高8位	中间8位	低16位
含义	模块ID	错误类型分类	具体错误码
范围	0x00-0xFF	0x00-0xFF	0x0000-0xFFFF
示例值	0x01 (HTTP)	0x01 (网络类)	0x0001 (连接超时)
*/

// 模块ID定义
#define MODULE_ID_HTTP      0x01
#define MODULE_ID_MQTT      0x02
#define MODULE_ID_WEBSOCKET 0x03

// 错误类型分类
#define ERROR_CAT_SYSTEM    0x01 // 系统错误（内存、文件IO等）
#define ERROR_CAT_NETWORK   0x02 // 网络错误（连接超时、DNS失败）
#define ERROR_CAT_PROTOCOL  0x03 // 协议错误（HTTP 404、MQTT断开）
#define ERROR_CAT_SECURITY  0x04 // 安全错误（证书无效、权限拒绝）
#define ERROR_CAT_BUSINESS  0x05 // 业务逻辑错误（参数无效、状态冲突）


// 错误码定义
#define VOLC_OK                  0
#define VOLC_ERR_INIT           -1  // 上下文初始化失败
#define VOLC_ERR_CONNECT        -2  // 连接失败
#define VOLC_ERR_SEND           -3  // 发送失败
#define VOLC_ERR_MALLOC         -4  // 内存分配失败
#define VOLC_ERR_INVALID_PARAM  -5  // 非法参数
#define VOLC_ERR_NULL_POINTER   -6  // 空指针
#define VOLC_ERR_FILE_OPEN      -7  // 文件打开失败
#define VOLC_ERR_FILE_WRITE     -8  // 文件写入失败
#define VOLC_ERR_FILE_DELETE    -9  // 文件删除失败

#define VOLC_ERR_MQTT_SUB    -10000  // MQTT订阅失败
#define VOLC_ERR_MQTT_PUB    -10001  // MQTT发布失败

/** start ota error code **/
#define VOLC_ERR_OTA_DOWNLOAD_FILE_SIZE -501
#define VOLC_ERR_OTA_DOWNLOAD_FILE_EMPTY -502
#define VOLC_ERR_OTA_DOWNLOAD_FILE_SIGN_CHECK -503
// #define OTA_DEVICE_INFO_NOT_SET -504
#define VOLC_ERR_OTA_INSTALL_FAILED -505
/** end ota error code **/

#define VOLC_ERR_TM_USER_INPUT_OUT_RANGE -601
#define VOLC_ERR_DM_PUBLISH_TYPE_UNKNOWN -602

// HTTP模块统一错误码

#define VOLC_ERR_HTTP_MALLOC_FAILED     0x01010001
#define VOLC_ERR_HTTP_INVALID_PARAM     0x01010002
#define VOLC_ERR_INVALID_HTTP_CONTEXT   0x01010003
#define VOLC_ERR_HTTP_CONN_INIT_FAILED  0x01010004
#define VOLC_ERR_HTTP_CONN_TIMEOUT      0x01020001
#define VOLC_ERR_HTTP_CONN_FAILED       0x01020002
#define VOLC_ERR_HTTP_SEND_TIMEOUT      0x01020003
#define VOLC_ERR_HTTP_SEND_FAILED       0x01020004
#define VOLC_ERR_HTTP_RECV_FAILED       0x01020005
#define VOLC_ERR_HTTP_RECV_TIMEOUT      0x01020006
#define VOLC_ERR_HTTP_RECV_EMPTY        0x01020007
#define VOLC_ERR_HTTP_RECV_TOO_LARGE    0x01020008

#endif //ONESDK_ERROR_CODE_H