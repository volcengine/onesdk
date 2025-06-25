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
#include "cJSON.h"

#include "onesdk_hello_world.h"


// 解析 JSON 字符串到结构体
int parse_json(const char *json_str, ChatCompletionChunk *chunk) {
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
            return -1;
        }
        return -2;
    }

    // 提取基本信息
    cJSON *id = cJSON_GetObjectItem(root, "id");
    if (cJSON_IsString(id)) {
        strncpy(chunk->id, id->valuestring, sizeof(chunk->id) - 1);
        chunk->id[sizeof(chunk->id) - 1] = '\0';
    }

    cJSON *object = cJSON_GetObjectItem(root, "object");
    if (cJSON_IsString(object)) {
        strncpy(chunk->object, object->valuestring, sizeof(chunk->object) - 1);
        chunk->object[sizeof(chunk->object) - 1] = '\0';
    }

    cJSON *created = cJSON_GetObjectItem(root, "created");
    if (cJSON_IsNumber(created)) {
        chunk->created = created->valueint;
    }

    cJSON *model = cJSON_GetObjectItem(root, "model");
    if (cJSON_IsString(model)) {
        strncpy(chunk->model, model->valuestring, sizeof(chunk->model) - 1);
        chunk->model[sizeof(chunk->model) - 1] = '\0';
    }

    // 提取 choices 数组中的第一个元素
    cJSON *choices = cJSON_GetObjectItem(root, "choices");
    if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
        cJSON *choice = cJSON_GetArrayItem(choices, 0);

        cJSON *index = cJSON_GetObjectItem(choice, "index");
        if (cJSON_IsNumber(index)) {
            chunk->index = index->valueint;
        }

        cJSON *delta = cJSON_GetObjectItem(choice, "delta");
        if (cJSON_IsObject(delta)) {
            cJSON *delta_content = cJSON_GetObjectItem(delta, "content");
            if (cJSON_IsString(delta_content)) {
                strncpy(chunk->delta_content, delta_content->valuestring, sizeof(chunk->delta_content) - 1);
                chunk->delta_content[sizeof(chunk->delta_content) - 1] = '\0';
            }

            cJSON *delta_role = cJSON_GetObjectItem(delta, "role");
            if (cJSON_IsString(delta_role)) {
                strncpy(chunk->delta_role, delta_role->valuestring, sizeof(chunk->delta_role) - 1);
                chunk->delta_role[sizeof(chunk->delta_role) - 1] = '\0';
            }
        }

        cJSON *finish_reason = cJSON_GetObjectItem(choice, "finish_reason");
        if (cJSON_IsString(finish_reason)) {
            strncpy(chunk->finish_reason, finish_reason->valuestring, sizeof(chunk->finish_reason) - 1);
            chunk->finish_reason[sizeof(chunk->finish_reason) - 1] = '\0';
        }

        cJSON *content_filter_results = cJSON_GetObjectItem(choice, "content_filter_results");
        if (cJSON_IsObject(content_filter_results)) {
            cJSON *hate = cJSON_GetObjectItem(content_filter_results, "hate");
            if (cJSON_IsObject(hate)) {
                cJSON *hate_filtered = cJSON_GetObjectItem(hate, "filtered");
                if (cJSON_IsBool(hate_filtered)) {
                    chunk->hate_filtered = cJSON_IsTrue(hate_filtered);
                }
            }

            cJSON *self_harm = cJSON_GetObjectItem(content_filter_results, "self_harm");
            if (cJSON_IsObject(self_harm)) {
                cJSON *self_harm_filtered = cJSON_GetObjectItem(self_harm, "filtered");
                if (cJSON_IsBool(self_harm_filtered)) {
                    chunk->self_harm_filtered = cJSON_IsTrue(self_harm_filtered);
                }
            }

            cJSON *sexual = cJSON_GetObjectItem(content_filter_results, "sexual");
            if (cJSON_IsObject(sexual)) {
                cJSON *sexual_filtered = cJSON_GetObjectItem(sexual, "filtered");
                if (cJSON_IsBool(sexual_filtered)) {
                    chunk->sexual_filtered = cJSON_IsTrue(sexual_filtered);
                }
            }

            cJSON *violence = cJSON_GetObjectItem(content_filter_results, "violence");
            if (cJSON_IsObject(violence)) {
                cJSON *violence_filtered = cJSON_GetObjectItem(violence, "filtered");
                if (cJSON_IsBool(violence_filtered)) {
                    chunk->violence_filtered = cJSON_IsTrue(violence_filtered);
                }
            }
        }
    }

    // 提取 system_fingerprint
    cJSON *system_fingerprint = cJSON_GetObjectItem(root, "system_fingerprint");
    if (cJSON_IsString(system_fingerprint)) {
        strncpy(chunk->system_fingerprint, system_fingerprint->valuestring, sizeof(chunk->system_fingerprint) - 1);
        chunk->system_fingerprint[sizeof(chunk->system_fingerprint) - 1] = '\0';
    }

    // 释放 cJSON 对象
    cJSON_Delete(root);
    return 0;
}

// int main() {
//     const char *json_str = "{\"id\":\"021739861067234373db2f1c4e5a58c7df479d3506180f130f65a\",\"object\":\"chat.completion.chunk\",\"created\":1739861067,\"model\":\"doubao-1-5-vision-pro-32k-250115\",\"choices\":[{\"index\":0,\"delta\":{\"content\":\" a\",\"role\":\"assistant\"},\"finish_reason\":null,\"content_filter_results\":{\"hate\":{\"filtered\":false},\"self_harm\":{\"filtered\":false},\"sexual\":{\"filtered\":false},\"violence\":{\"filtered\":false}}}],\"system_fingerprint\":\"\"}";
//     ChatCompletionChunk chunk;
//     memset(&chunk, 0, sizeof(chunk));
//
//     parse_json(json_str, &chunk);
//
//     // 打印解析结果
//     printf("id: %s\n", chunk.id);
//     printf("object: %s\n", chunk.object);
//     printf("created: %ld\n", chunk.created);
//     printf("model: %s\n", chunk.model);
//     printf("index: %d\n", chunk.index);
//     printf("delta_content: %s\n", chunk.delta_content);
//     printf("delta_role: %s\n", chunk.delta_role);
//     printf("finish_reason: %s\n", chunk.finish_reason);
//     printf("hate_filtered: %d\n", chunk.hate_filtered);
//     printf("self_harm_filtered: %d\n", chunk.self_harm_filtered);
//     printf("sexual_filtered: %d\n", chunk.sexual_filtered);
//     printf("violence_filtered: %d\n", chunk.violence_filtered);
//     printf("system_fingerprint: %s\n", chunk.system_fingerprint);
//
//     return 0;
// }
