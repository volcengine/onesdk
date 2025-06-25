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

#ifndef ONESDK_LWS_HTTP_CLIENT_H
#define ONESDK_LWS_HTTP_CLIENT_H

#include "libwebsockets.h"
#include "http.h"


int lws_http_client_init(http_request_context_t *http_ctx);

int lws_http_client_connect(http_request_context_t *http_ctx);

int lws_http_client_request(http_request_context_t *http_ctx);

int lws_http_client_send_request_body(http_request_context_t *http_ctx);

http_response_t* lws_http_client_send_request(http_request_context_t *http_ctx);

int lws_http_client_receive_response(http_request_context_t *http_ctx);
int lws_http_client_receive_response_body(http_request_context_t *http_ctx);


void lws_http_client_disconnect(http_request_context_t *http_ctx);

void lws_http_client_disconnect_new(http_request_context_t *http_ctx);

void lws_http_client_destroy_network_context(http_request_context_t *http_ctx);

int lws_http_client_deinit(http_request_context_t *http_ctx);

// helper methods
int lws_http_client_init_client_connect_info(http_request_context_t *http_ctx, struct lws_client_connect_info *ccinfo);

int lws_http_client_parse_url(char *uri, const char **protocol, const char **host, int *port, const char **path);

// internal interfaces
void _http_client_data_free(http_client_data_t *data);
void _http_client_data_sse_free(sse_context_t *sse);
#endif //ONESDK_LWS_HTTP_CLIENT_H