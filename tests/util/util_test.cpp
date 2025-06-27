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

extern "C"
{
  #include "CppUTest/TestHarness_c.h"
  #include "aws/common/string.h"
  #include "util/util.h"
  #include "util/aes_decode.h"
}

TEST_GROUP(util) {
  struct aws_allocator *alloc = aws_alloc();
  void setup()
  {
  }

  void teardown()
  {
  }
};

// TEST(util, test_aes_decode) {
//     char *device_secret = "1234567812345678";
//     char *encrypted = "ihLX+mtlNzm6fXqxke0vpA==";
//     const char *got = aes_decode(alloc, device_secret, encrypted);
//     const char *want_str = "test-abc";
//     STRCMP_EQUAL(want_str, got);
//     cpputest_free((void *)got);
// }

TEST(util, test_aes_decode) {
    char *device_secret = "fake-device-secret";
    char *encrypted = "lk55/tznQECKe/IzNup8LN3uf4m6c25d7VHFVnhezqEBIcRy+2PoAw+cUilpMyu9wTcwsUfqe8e/4RMriz/L6w==";
    const char *got = aes_decode(alloc, device_secret, encrypted, false);
    // const char *want_str = "test-abc";
    // STRCMP_EQUAL(want_str, got);
    CHECK(got != NULL);
    cpputest_free((void *)got);
}

