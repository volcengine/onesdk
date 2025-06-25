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

#ifndef _VEI_UTIL_AES_DECODE_H
#define _VEI_UTIL_AES_DECODE_H

#include "aws/common/byte_buf.h"
#include "aws/common/string.h"
#include "aws/common/encoding.h"

char *aes_decode(struct aws_allocator *allocator, const char *device_secret, const char *encrypt_data, bool partial_secret);

#endif //_VEI_UTIL_AES_DECODE_H