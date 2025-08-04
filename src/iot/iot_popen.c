/*
 * Copyright 2022-2024 Beijing Volcano Engine Technology Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "onesdk_config.h"
#ifdef ONESDK_ENABLE_IOT

#include <stdio.h>
#include "iot/iot_popen.h"
#include <stdlib.h>
#ifdef _WIN32
#include <process.h>
#define pclose _pclose
#define popen _popen
#endif

int iot_popen(const char* cmd, const char* mode, const char* data, int data_len) {
    if (cmd == NULL || mode == NULL) {
        return -1;
    }

    int do_read = 0, do_write = 0;
    const char* type = mode;
    while (*type != '\0') {
        switch (*type++)
        {
            case 'r':
                do_read = 1;
                break;
            case 'w':
                do_write = 1;
                break;

            default:
                return -1;
        }
    }
    // exec cmd will remove first byte
    if ((do_read ^ do_write) == 0) {
        return -1;
    }

    FILE* file = NULL;
    // if mode == 'r', file fd will dup stdout , instead file fd will dup stdin
    // file = popen(cmd, mode);
    if (file == NULL) {
        return -1;
    }

    // write stdin, data will input params
    if (do_write == 1 && data != NULL) {
//        fputs(data, file);
    }

    // read stdout, data will output params
    if (do_read == 1 && data != NULL) {
        while (fgets((char *)data, data_len, file) != NULL) {
            // nothing to do，we also read stdout
        }
    }

    // close file
    if (file != NULL) {
        return pclose(file);
    }
    return 0;
}

int iot_system(const char* cmd) {
    return system(cmd);
}
#endif //ONESDK_ENABLE_IOT