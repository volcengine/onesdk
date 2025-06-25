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

#include "CppUTest/CommandLineTestRunner.h"
#define ENABLE_AI
#define ENABLE_AI_REALTIME

// IMPORT_TEST_GROUP(auth); // onesdk_new_http_ctx leaks
// IMPORT_TEST_GROUP(util); // ok
// IMPORT_TEST_GROUP(iot_basic); // ok
// IMPORT_TEST_GROUP(llm_config); // ok
IMPORT_TEST_GROUP(dynreg);
IMPORT_TEST_GROUP(hardware_id);

int main(int argc, char** argv)
{
    return RUN_ALL_TESTS(argc, argv);
}
