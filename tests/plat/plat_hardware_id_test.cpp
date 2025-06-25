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

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C"
{
  #include "CppUTest/TestHarness_c.h"
  #include "plat/platform.h"
}


TEST_GROUP(hardware_id) {
    void setup() {
    }

    void teardown() {
    }
};



TEST(hardware_id, test_hardware_id) {
    char *hw_id = plat_hardware_id();
    char *f1 = fallback_mac_addr();
    char *f2 = fallback_mac_addr();

    STRCMP_EQUAL(f1, f2);
    CHECK(hw_id != NULL);

    cpputest_free(hw_id);
    cpputest_free(f1);
    cpputest_free(f2);
}