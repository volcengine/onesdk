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


#ifndef ARENAL_IOT_IOT_POPEN_H
#define ARENAL_IOT_IOT_POPEN_H
#ifdef ONESDK_ENABLE_IOT

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

int iot_popen(const char* cmd, const char* mode, const char* data, int data_len);

int iot_system(const char* cmd);
#endif // ARENAL_IOT_IOT_POPEN_H
#endif // ONESDK_ENABLE_IOT