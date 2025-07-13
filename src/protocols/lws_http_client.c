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


#include <string.h>
#include <signal.h>

#include "libwebsockets.h"
#include "protocols/http.h"
#include "protocols/private_lws_http_client.h"
#include "error_code.h"
#include "platform_compat.h"

static int interrupted, bad = 1, status, conmon = 1, close_after_start = 0;
static int log_level = LLL_USER | LLL_ERR | LLL_WARN;
static int log_level_debug = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG;
static int log_level_trace = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG | LLL_EXT;

#if defined(LWS_WITH_HTTP2)
static int long_poll;
#endif
static struct lws *client_wsi;
static const char *ba_user, *ba_password;

static const lws_retry_bo_t retry = {
	.secs_since_valid_ping = 3,
	.secs_since_valid_hangup = 10,
};

#if defined(LWS_WITH_CONMON)
void
dump_conmon_data(struct lws *wsi)
{
	const struct addrinfo *ai;
	struct lws_conmon cm;
	char ads[48];

	lws_conmon_wsi_take(wsi, &cm);

	lws_sa46_write_numeric_address(&cm.peer46, ads, sizeof(ads));
	lwsl_notice("%s: peer %s, dns: %uus, sockconn: %uus, tls: %uus, txn_resp: %uus\n",
		    __func__, ads,
		    (unsigned int)cm.ciu_dns,
		    (unsigned int)cm.ciu_sockconn,
		    (unsigned int)cm.ciu_tls,
		    (unsigned int)cm.ciu_txn_resp);

	ai = cm.dns_results_copy;
	while (ai) {
		lws_sa46_write_numeric_address((lws_sockaddr46 *)ai->ai_addr, ads, sizeof(ads));
		lwsl_notice("%s: DNS %s\n", __func__, ads);
		ai = ai->ai_next;
	}

	/*
	 * This destroys the DNS list in the lws_conmon that we took
	 * responsibility for when we used lws_conmon_wsi_take()
	 */

	lws_conmon_release(&cm);
}
#endif

static const char *ua = "Mozilla/5.0 (X11; Linux x86_64) "
			"AppleWebKit/537.36 (KHTML, like Gecko) "
			"Chrome/51.0.2704.103 Safari/537.36",
		  *acc = "*/*";



static int
callback_onesdk_http(struct lws *wsi, enum lws_callback_reasons reason,
	      void *user, void *in, size_t len)
{
	switch (reason) {

	/* because we are protocols[0] ... */
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		lwsl_err("CLIENT_CONNECTION_ERROR: %s\n",in ? (char *)in : "(null)");
		interrupted = 1;
		bad = 3; /* connection failed before we could make connection */
		lws_cancel_service(lws_get_context(wsi));
		// also call complete here
		http_request_context_t *http_ctx = (http_request_context_t *)user;
		if (http_ctx == NULL) {
			lwsl_warn("http_ctx is NULL, report issue to developer\n");
			return 0;
		}
		if (http_ctx->response != NULL) {
			http_ctx->response->inner_error_code = VOLC_ERR_HTTP_CONN_FAILED;
			lwsl_err("connection error: %s 0x%lx\n", in ? (char *)in : "(null)", http_ctx->response->inner_error_code);
		}
		http_ctx->is_connection_completed = 1;
		if (http_ctx->download_ctx != NULL) {
			http_ctx->download_ctx->on_download_error(VOLC_ERR_HTTP_CONN_FAILED, http_ctx->download_ctx->user_data);
		}
		if (http_ctx->on_error_cb) {
			http_ctx->on_error_cb(VOLC_ERR_HTTP_CONN_FAILED, in ? (char *)in : "(null)", http_ctx->on_error_cb_user_data);
		}

#if defined(LWS_WITH_CONMON)
	if (conmon)
		dump_conmon_data(wsi);
#endif
		break;

	case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
		{
			lwsl_debug("callback_onesdk_http LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP");
			char buf[128];

			lws_get_peer_simple(wsi, buf, sizeof(buf));
			status = (int)lws_http_client_http_response(wsi);
			http_ctx = (http_request_context_t *)user;
			if (http_ctx == NULL) {
				lwsl_warn("http_ctx is NULL, report issue to developer\n");
				return 0;
			}
			// http_ctx->response->error_code = status;
			http_ctx->client->response_code = status;
			http_ctx->response->error_code = status;
#if defined(LWS_WITH_ALLOC_METADATA_LWS)
			_lws_alloc_metadata_dump_lws(lws_alloc_metadata_dump_stdout, NULL);
#endif

			lwsl_user("Connected to %s, http response: %d\n", buf, status);
			if (status > 300) {
            	/* 新增错误信息处理 */
				const char *http_msg = in ? (const char *)in : "No error message";
				char safe_msg[128];
				int msg_len = len < sizeof(safe_msg)-1 ? len : sizeof(safe_msg)-1;
				
				memcpy(safe_msg, http_msg, msg_len);
				safe_msg[msg_len] = '\0';
				
				lwsl_err("HTTP Error %d: %.*s\n", status, (int)len, (const char *)in);
			}
		}
#if defined(LWS_WITH_HTTP2)
		if (long_poll) {
			lwsl_debug("%s: Client entering long poll mode\n", __func__);
			lws_h2_client_stream_long_poll_rxonly(wsi);
		}
#endif

		if (lws_fi_user_wsi_fi(wsi, "user_reject_at_est"))
			return -1;

		break;

	/* you only need this if you need to do Basic Auth or Bearer Auth */
	case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
		lwsl_debug("LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: adding auth");
		if (lws_http_is_redirected_to_get(wsi))
			break;
		http_ctx = (http_request_context_t *)user;
		if (http_ctx == NULL) {
			lwsl_warn("http_ctx is NULL, report issue to developer\n");
			return 0;
		}
		// add bearer auth header
		{
			if (http_ctx->client->bearer_token != NULL) {
				unsigned char **p = (unsigned char **)in, *end = (*p) + len;
				char * bearer_token = http_ctx->client->bearer_token;
				int bearer_len = strlen(bearer_token);
				lwsl_debug("LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: adding bearer auth，len %d, bytes:\n%s\n",
					bearer_len, bearer_token);

				if(lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_AUTHORIZATION,
						(unsigned char *)bearer_token, bearer_len,
						p, end))
					return -1;
			}
			// add content-type
			{
				char *content_type = http_ctx->client_data->post_content_type;
				if (content_type != NULL) {
					unsigned char **p = (unsigned char **)in, *end = (*p) + len;
					int content_len = strlen(content_type);
					lwsl_debug("LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: adding content-type, len %d, bytes:\n%s\n",
						content_len, content_type);
					if(lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_CONTENT_TYPE,
							(unsigned char *)content_type, content_len,
							p, end))
						return -1;
				}
			}
			// add content-length
			{
				unsigned char **p = (unsigned char **)in, *end = (*p) + len;
				int content_len = http_ctx->client_data->post_buf_len;
				lwsl_debug("LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: adding content-length, len %d\n",
					content_len);
				if(lws_add_http_header_content_length(wsi, content_len, p, end))
					return -1;
			}
// add custom headers
        {
            if (http_ctx->headers != NULL) {
                unsigned char **p = (unsigned char **)in, *end = (*p) + len;
                char *headers = http_ctx->headers;
                char *header, *value, *saveptr;
                
                // 按行分割headers
                for (header = strtok_r(headers, "\r\n", &saveptr); header; 
                     header = strtok_r(NULL, "\r\n", &saveptr)) {
                    // 分割header和value
                    value = strchr(header, ':');
                    if (value) {
                        *value++ = '\0';  // 分割header和value
                        while (*value == ' ') value++;  // 跳过空格
                        
                        if (lws_add_http_header_by_name(wsi, 
                                (unsigned char *)header,
                                (unsigned char *)value, strlen(value),
                                p, end)) {
                            return -1;
                        }
                        lwsl_debug("Added custom header: %s: %s\n", header, value);
                    }
                }
            }
        }

		}
	{
		unsigned char **p = (unsigned char **)in, *end = (*p) + len;

		if (lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_USER_AGENT,
				(unsigned char *)ua, (int)strlen(ua), p, end))
			return -1;

		if (lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_ACCEPT,
				(unsigned char *)acc, (int)strlen(acc), p, end))
			return -1;
#if defined(LWS_WITH_HTTP_BASIC_AUTH)
		{
		char b[128];

		if (ba_user != NULL && ba_password != NULL) {
			if (lws_http_basic_auth_gen(ba_user, ba_password, b, sizeof(b))) {
				lwsl_warn("LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: failed to generate basic auth\n");
				break;
			}
			if (lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_AUTHORIZATION,
				(unsigned char *)b, (int)strlen(b), p, end))
				return -1;
		}
		}
#endif
		/* inform lws we have http body to send */
		lws_client_http_body_pending(wsi, 1);
		lws_callback_on_writable(wsi);
		lwsl_debug("LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: done\n");
		break;
	}
	case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
    {
		// 发送 JSON 数据
		lwsl_debug("LWS_CALLBACK_CLIENT_HTTP_WRITEABLE(user = %p): begin\n", user);
		if (lws_http_is_redirected_to_get(wsi))
			break;
		http_request_context_t *http_ctx = (http_request_context_t *)user;
		if (http_ctx == NULL) {
			lwsl_warn("http_ctx is NULL, report issue to developer\n");
			return 0;
		}
		// // 发送 custom header 数据 不包括 AUTHORIZATION header
		// char *custom_headers = http_ctx->headers;
		// unsigned char buf[LWS_PRE + strlen(custom_headers)];
		// unsigned char *p_header = &buf[LWS_PRE];
		// int header_len = strlen(custom_headers);
		// memcpy(p_header, custom_headers, header_len);
		// lws_write(wsi, p_header, header_len, LWS_WRITE_HTTP_HEADERS);
		// lwsl_debug("LWS_CALLBACK_CLIENT_HTTP_WRITEABLE: wrote custom headers:%d, bytes\n%s\n", header_len, p_header);

		// 发送 JSON body 数据

		if (http_ctx->client_data->post_buf == NULL) {
			lwsl_warn("http_ctx->client_data->post_buf is NULL, not sending body\n");
			lws_client_http_body_pending(wsi, 0);
			return 0;
		}
		const char *json_data = http_ctx->client_data->post_buf;
		int body_len = strlen(json_data);
		unsigned char *buf_body = malloc(LWS_PRE + body_len);
		if (!buf_body) {
			lwsl_err("malloc failed for buf_body\n");
			return -1;
		}
		unsigned char *p_body = &buf_body[LWS_PRE];

		memcpy(p_body, json_data, body_len);
		lws_write(wsi, p_body, body_len, LWS_WRITE_HTTP_FINAL);
		lwsl_debug("LWS_CALLBACK_CLIENT_HTTP_WRITEABLE: wrote body length: %d\n", body_len);
		//fprintf(stderr, "LWS_CALLBACK_CLIENT_HTTP_WRITEABLE: wrote body bytes: %s\n", json_data);
		// 结束 HTTP 请求,如果还有，请设置1
		lws_client_http_body_pending(wsi, 0);
		lwsl_debug("LWS_CALLBACK_CLIENT_HTTP_WRITEABLE: end\n");
		free(buf_body);

		break;
	}
	case LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH: {
		lwsl_debug("LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH: begin\n");
		http_request_context_t *http_ctx = (http_request_context_t *)user;
		if (http_ctx == NULL) {
			lwsl_warn("http_ctx is NULL, report issue to developer\n");
			return 0;
		}
		// 在这里可以读取 HTTP 响应头
		// const unsigned char *header;
		int header_len;
		// TODO(wangxu) 抽取到独立的函数做解析
		// 读取特定的 HTTP 响应头，例如 "Content-Type"
		// header = lws_token_to_string(WSI_TOKEN_HTTP_CONTENT_TYPE);
		header_len = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_CONTENT_TYPE);
		if (header_len > 0) {
			char *content_type = malloc(header_len + 1);
			if (!content_type) {
				lwsl_err("malloc failed for content_type\n");
				return -1;
			}
			lws_hdr_copy(wsi, content_type, header_len + 1, WSI_TOKEN_HTTP_CONTENT_TYPE);
			lwsl_debug("Content-Type: %s\n", content_type);
			if (strncmp(content_type, "text/event-stream", strlen("text/event-stream")) == 0) {
				http_ctx->client_data->is_sse = true;
			}
			free(content_type);
		}
		// 读取特定的 HTTP 响应头，例如 "Content-Length"
		// header = lws_token_to_string(WSI_TOKEN_HTTP_CONTENT_LENGTH);
		header_len = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_CONTENT_LENGTH);
		if (header_len > 0) {
			char *content_length = malloc(header_len + 1);
			if (!content_length) {
				lwsl_err("malloc failed for content_length\n");
				return -1;
			}
			lws_hdr_copy(wsi, content_length, header_len + 1, WSI_TOKEN_HTTP_CONTENT_LENGTH);
			lwsl_debug("Content-Length: %s\n", content_length);
			// http_ctx->response->body_size = atoi(content_length);
			free(content_length);
		}
		// 读取特定的 HTTP 响应头，例如 "Transfer-Encoding"
		// header = lws_token_to_string(WSI_TOKEN_HTTP_TRANSFER_ENCODING);
		header_len = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_TRANSFER_ENCODING);
		if (header_len > 0) {
			char *transfer_encoding = malloc(header_len + 1);
			if (!transfer_encoding) {
				lwsl_err("malloc failed for transfer_encoding\n");
				return -1;
			}
			lws_hdr_copy(wsi, transfer_encoding, header_len + 1, WSI_TOKEN_HTTP_TRANSFER_ENCODING);
			lwsl_debug("Transfer-Encoding: %s\n", transfer_encoding);
			if (strcmp(transfer_encoding, "chunked") == 0) {
				// chunked data
				http_ctx->client_data->is_chunked = true;
			}
			free(transfer_encoding);
		}
		// 读取特定的 HTTP 响应头，例如 "Range" 做断点续传
		// header = lws_token_to_string(WSI_TOKEN_HTTP_RANGE);
		header_len = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_RANGE);
		if (header_len > 0) {
			char *range = malloc(header_len + 1);
			if (!range) {
				lwsl_err("malloc failed for range\n");
				return -1;
			}
			lws_hdr_copy(wsi, range, header_len + 1, WSI_TOKEN_HTTP_RANGE);
			lwsl_debug("Range: %s\n", range);
			free(range);
		}
		// TODO, add logic to parse more http response headers
		break;
	}
	/* chunks of chunked content, with header removed */
	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
#if 0  /* enable to dump the html */
		{
			const char *p = in;

			while (len--)
				if (*p < 0x7f)
					putchar(*p++);
				else
					putchar('.');
		}
#endif
		lwsl_debug("LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ（user = %p): read len = %d, in = %p\n",user, len, in);
		http_ctx = (http_request_context_t *)user;
		if (http_ctx == NULL) {
			lwsl_warn("http_ctx is NULL, report issue to developer\n");
			return 0;
		}

		// 处理 server sent event(SSE) 数据
		if (http_ctx->client_data->is_sse) {
			sse_context_t *sse = http_ctx->client_data->sse_ctx;
			
			// 1. 改进缓冲区处理：部分拷贝而非全部丢弃
			size_t available = sizeof(sse->buffer) - sse->used_len - 1; // 保留结束符空间
			if (len > available) {
				lwsl_notice("SSE buffer full, truncating %zd bytes to %zd\n", len, available);
				len = available; // 仅拷贝能容纳的部分
			}
			memcpy(sse->buffer + sse->used_len, in, len);
			sse->used_len += len;
			sse->buffer[sse->used_len] = '\0'; // 确保字符串终止

			// 2. 优化日志：仅调试模式输出
			lwsl_hexdump_debug(in, len);
			// 3. 支持多种分隔符：\n\n 和 \r\n\r\n
			char *cur_pos = sse->buffer;
			while (cur_pos < sse->buffer + sse->used_len) {
				char *event_end = NULL;
				if (sse->event) free(sse->event);
				if (sse->id) free(sse->id);
				if (sse->data) free(sse->data);
				sse->event = NULL;
				sse->id = NULL;
				sse->data = NULL;
				
				// 优先检测 \r\n\r\n
				if ((event_end = onesdk_memmem(cur_pos, sse->used_len - (cur_pos - sse->buffer), "\r\n\r\n", 4))) {
					size_t event_len = event_end - cur_pos + 4;
					parse_sse_message(sse, cur_pos, event_len);
					
					if (http_ctx->on_get_sse_cb) {
						// 传递事件内容而非解析器状态
						http_ctx->on_get_sse_cb(sse, false, http_ctx->on_get_sse_cb_user_data);
					}
					cur_pos = event_end + 4;
				}
				// 其次检测 \n\n
				else if ((event_end = onesdk_memmem(cur_pos, sse->used_len - (cur_pos - sse->buffer), "\n\n", 2))) {
					size_t event_len = event_end - cur_pos + 2;
					parse_sse_message(sse, cur_pos, event_len);
					
					if (http_ctx->on_get_sse_cb) {
						http_ctx->on_get_sse_cb(sse, false, http_ctx->on_get_sse_cb_user_data);
					}
					cur_pos = event_end + 2;
				}
				else {
					break; // 无完整事件
				}
			}

			// 4. 安全移动剩余数据
			size_t remaining = sse->buffer + sse->used_len - cur_pos;
			if (remaining > 0) {
				memmove(sse->buffer, cur_pos, remaining);
			}
			sse->used_len = remaining;
			sse->buffer[remaining] = '\0';

		}

		// 处理 chunked data
		if (http_ctx->client_data->is_chunked) {
			lwsl_debug("begin to process chunked data\n");

		}

		// 处理普通数据
		if (http_ctx->on_get_body_cb) {
			lwsl_debug("LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ: on_get_body_cb, len=%d\n", len);
			http_ctx->on_get_body_cb(in, len, false, http_ctx->on_get_body_cb_user_data);
		}
		// lwsl_debug("http_ctx->download_ctx : %p\n", http_ctx->download_ctx);
		// lwsl_debug("http_ctx->download_ctx->on_download_data : %p\n", http_ctx->download_ctx->on_download_data);
		if (http_ctx->download_ctx && http_ctx->download_ctx->on_download_data) {
			lwsl_debug("LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ: on_download_data, len=%d\n", len);
			if (http_ctx->download_ctx->on_download_data) {
				http_ctx->download_ctx->on_download_data(in, len, false, http_ctx->download_ctx->user_data);
			}
		}
		// 对于错误的请求，仍然需要读取数据（主要是error code > 300的情况）
		if (http_ctx->is_async_request && !(http_ctx->response != NULL && http_ctx->response->error_code > 300)) {
			// 异步请求，不会将数据写入 response_body
			// 通过回调函数处理数据
			return 0;
		}
		if (http_ctx->response != NULL) {
			if (http_ctx->response->response_body == NULL && http_ctx->response->body_size == 0) {
				http_ctx->response->response_body = (char *)malloc(len + 1);
				if (http_ctx->response->response_body == NULL) {
					lwsl_err("malloc failed response_body\n");
					return VOLC_ERR_HTTP_MALLOC_FAILED;
				}
				memset(http_ctx->response->response_body, 0, len + 1);
				memcpy(http_ctx->response->response_body, in, len);
				http_ctx->response->response_body[len] = '\0'; // avoid strlen panic
				http_ctx->response->body_size = len;
				lwsl_debug("http_ctx->response->body_size: %d, %s\n", http_ctx->response->body_size, http_ctx->response->response_body);
			} else {
				// 接收新内容
				char *curr = http_ctx->response->response_body;
				int curr_len = http_ctx->response->body_size;
				int new_len = curr_len + len;
				if (new_len > ONESDK_HTTP_MAX_BODY_SIZE) {
					lwsl_err("response_body is too large, new_len = %d, consider increasing ONESDK_HTTP_MAX_BODY_SIZE macro\n", new_len);
					return VOLC_ERR_HTTP_RECV_TOO_LARGE;
				}
				char *new_curr = (char *)realloc(curr, new_len + 1);
				if (new_curr == NULL) {
					lwsl_err("remalloc failed response_body\n");
					return VOLC_ERR_HTTP_MALLOC_FAILED;
				}
				http_ctx->response->response_body = new_curr;
				memcpy(new_curr + curr_len, in, len);
				http_ctx->response->response_body[new_len] = '\0';
				http_ctx->response->body_size = new_len;
				lwsl_debug("http_ctx->response->body_size: %d, %s\n", http_ctx->response->body_size, http_ctx->response->response_body);
			}
		}

		lwsl_hexdump_notice(in, len);
		// read tail by len
// #if defined(LWS_WITH_HTTP2)
// 		if (long_poll) {
// 			char dotstar[128];
// 			lws_strnncpy(dotstar, (const char *)in, len,
// 				     sizeof(dotstar));
// 			lwsl_notice("long poll rx: %d '%s'\n", (int)len,
// 					dotstar);
// 		}
// #endif
		// read head by len
		// if (lws_http_client_read(wsi, &in, &len) < 0)
		// 	return -1;
		// lwsl_hexdump_notice(in, len);

		// read tail by len
#if 0
		lwsl_hexdump_notice(in, len);
#endif

		return 0; /* don't passthru */

	/* uninterpreted http content */
	case LWS_CALLBACK_CLIENT_RECEIVE:
		// 接收到服务器数据
			lwsl_info("Received data from server: %.*s\n", (int)len, (const char *)in);

	// 检查 HTTP 响应状态码
	if (lws_http_client_http_response(wsi) == HTTP_STATUS_NOT_FOUND) {
		lwsl_err("HTTP 404: Resource not found\n");
	} else {
		lwsl_info("HTTP response code: %d\n", lws_http_client_http_response(wsi));
	}
	break;
	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
		lwsl_debug("LWS_CALLBACK_RECEIVE_CLIENT_HTTP len = %d\n", len);

		{
			char buffer[1024 + LWS_PRE];
			char *px = buffer + LWS_PRE;
			int lenx = sizeof(buffer) - LWS_PRE;

			if (lws_fi_user_wsi_fi(wsi, "user_reject_at_rx"))
				return -1;

			if (lws_http_client_read(wsi, &px, &lenx) < 0)
				return -1;
		}
		return 0; /* don't passthru */

	case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
		// 实际处理response body的回调，并填充到http_ctx->response
		lwsl_debug("LWS_CALLBACK_COMPLETED_CLIENT_HTTP\n");
		// interrupted = 1;
		bad = status != 200;
		lws_cancel_service(lws_get_context(wsi)); /* abort poll wait */
		http_ctx = (http_request_context_t *)user;
		if (http_ctx == NULL) {
			lwsl_warn("http_ctx is NULL, report issue to developer\n");
			return 0;
		}
		if (http_ctx->response != NULL && http_ctx->response->error_code > 300) {
			lwsl_err("LWS_CALLBACK_COMPLETED_CLIENT_HTTP: error_code = %ld\n", (int32_t)http_ctx->response->error_code);
			if (http_ctx->download_ctx && http_ctx->download_ctx->on_download_error) {
				http_ctx->download_ctx->on_download_error(http_ctx->response->error_code, http_ctx->download_ctx->user_data);
			}
			//printf("before on_error_cb %p\n", http_ctx->on_error_cb);
			if (http_ctx->on_error_cb) {
				lwsl_err("LWS_CALLBACK_COMPLETED_CLIENT_HTTP: on_error_cb, %s\n", http_ctx->response->response_body);
				http_ctx->on_error_cb(http_ctx->response->error_code, http_ctx->response->response_body, http_ctx->on_error_cb_user_data);
			}
			break; // http error try as error
		}
		http_ctx->is_connection_completed = 1;
		if (http_ctx->download_ctx && http_ctx->download_ctx->on_download_complete) {
			lwsl_debug("LWS_CALLBACK_COMPLETED_CLIENT_HTTP: on_download_complete\n");
			if (http_ctx->download_ctx->on_download_complete) {
				http_ctx->download_ctx->on_download_complete(http_ctx->download_ctx->user_data);
			}
		}

		lwsl_debug("LWS_CALLBACK_COMPLETED_CLIENT_HTTP: is_chunked = %d, is_sse = %d\n", http_ctx->client_data->is_chunked, http_ctx->client_data->is_sse);
		if (http_ctx->on_complete_cb != NULL) {
			lwsl_debug("LWS_CALLBACK_COMPLETED_CLIENT_HTTP: on_complete_cb\n");
			http_ctx->on_complete_cb(http_ctx->on_complete_cb_user_data);
		}
		break;

	case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
		lwsl_debug("LWS_CALLBACK_CLOSED_CLIENT_HTTP\n");
		status = (int)lws_http_client_http_response(wsi);
		lwsl_debug("LWS_CALLBACK_CLOSED_CLIENT_HTTP: status=%d, reason =%d\n", status, reason);
		// interrupted = 1;
		bad = status != 200;
		lws_cancel_service(lws_get_context(wsi)); /* abort poll wait */
		http_ctx = (http_request_context_t *)user;
		if (http_ctx == NULL) {
			lwsl_warn("http_ctx is NULL, report issue to developer\n");
			return 0;
		}
		http_ctx->is_connection_completed = 1;
#if defined(LWS_WITH_CONMON)
		if (conmon)
			dump_conmon_data(wsi);
#endif
		break;

	default:
		break;
	}

	return lws_callback_http_dummy(wsi, reason, user, in, len);
}

static const struct lws_protocols protocols[] = {
	{
		"http",
		callback_onesdk_http,
		0, 0, 0, NULL, 0
	},
	LWS_PROTOCOL_LIST_TERM
};

static void
sigint_handler(int sig)
{
	interrupted = 1;
}



static int
system_notify_cb_onesdk(lws_state_manager_t *mgr, lws_state_notify_link_t *link,
            int current, int target)
{
	lwsl_info("onesdk %s: %d -> %d\n", __func__, current, target);
    if (current != LWS_SYSTATE_OPERATIONAL || target != LWS_SYSTATE_OPERATIONAL)
        return 0;

    lwsl_info("%s: operational\n", __func__);
	struct lws_context *context = mgr->parent;
	http_request_context_t *http_ctx = (http_request_context_t *)lws_context_user(context);
    // 连接http server
	if (lws_http_client_connect(http_ctx) != 0) {
        return 1;
    }
    if (close_after_start)
        lws_wsi_close(client_wsi, LWS_TO_KILL_SYNC);

    return 0;
}

int lws_http_client_init(http_request_context_t *http_ctx) {
	lws_set_log_level(log_level, NULL);
	// lws_set_log_level(log_level_debug, NULL);
	lwsl_debug("lws http init\n");
    // init http client
	int n = 0, expected = 0;

    lws_state_notify_link_t notifier = { { NULL, NULL, NULL },
							system_notify_cb_onesdk, "app" };
	lws_state_notify_link_t *na[] = { &notifier, NULL };
	struct lws_context_creation_info ccinfo;
	struct lws_context *context;
	memset(&ccinfo, 0, sizeof ccinfo); /* otherwise uninitialized garbage */
	// lws_cmdline_option_handle_builtin(argc, argv, &info);

	lwsl_debug("LWS http client lws_http_init\n");

	ccinfo.port = CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
	ccinfo.protocols = protocols;
	ccinfo.user = http_ctx; // received in system_notify_cb_onesdk
	// ccinfo.register_notifier_list = na;
	if (http_ctx->timeout_ms) {
		ccinfo.timeout_secs = (http_ctx->timeout_ms + 999) / 1000;
		lwsl_info("lws http client set timeout %d secs", ccinfo.timeout_secs);
	}
	if (http_ctx->connect_timeout_ms <= 0) {
	    ccinfo.connect_timeout_secs = 30;
    }
    else {
        ccinfo.connect_timeout_secs = (http_ctx->connect_timeout_ms + 999) / 1000;
    }
	lwsl_debug("lws http client connect timeout %d seconds.", ccinfo.connect_timeout_secs);
	lwsl_debug("lws http client timeout %d seconds.", ccinfo.timeout_secs);

    /*
	 * since we know this lws context is only ever going to be used with
	 * one client wsis / fds / sockets at a time, let lws know it doesn't
	 * have to use the default allocations for fd tables up to ulimit -n.
	 * It will just allocate for 1 internal and 1 (+ 1 http2 nwsi) that we
	 * will use.
	 */
	// ccinfo.fd_limit_per_thread = 1 + 1 + 10;
	// ssl 证书
	// ssl相关内容必须启用，否则https会异常
	// ccinfo.port = CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
	ccinfo.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT | // 启用SSL
		LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT | // 允许非SSL端口
		LWS_SERVER_OPTION_H2_JUST_FIX_WINDOW_UPDATE_OVERFLOW;
	if (http_ctx->ca_crt != NULL || http_ctx->is_ssl) {
		lwsl_debug("lws_http_client_init set ca with:%s", http_ctx->ca_crt);
        ccinfo.client_ssl_ca_mem_len = strlen(http_ctx->ca_crt);
        ccinfo.client_ssl_ca_mem = http_ctx->ca_crt;
    }
	// if (http_ctx->verify_ssl) {
	// 	// ccinfo.options |= LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT |
	// 	// LWS_SERVER_OPTION_H2_JUST_FIX_WINDOW_UPDATE_OVERFLOW;
	// }

    context = lws_create_context(&ccinfo);
	if (!context) {
		lwsl_err("lws http init failed\n");
		// set error code for async request
		http_ctx->response->inner_error_code = VOLC_ERR_INVALID_HTTP_CONTEXT;
		return VOLC_ERR_INVALID_HTTP_CONTEXT;
	}
	http_ctx->network_ctx = context;// 设置网络上下文
    lwsl_info("[1]lws_http_client_init succeeded http_ctx->network_ctx %p\n", http_ctx->network_ctx);
	return VOLC_OK;
}

int lws_http_client_connect(http_request_context_t *http_ctx) {
	lwsl_debug("lws_http_client_connect begin\n");
    // client connection info
    struct lws_client_connect_info client_info;
	memset(&client_info, 0, sizeof client_info); /* otherwise uninitialized garbage */

	int r =  lws_http_client_init_client_connect_info(http_ctx, &client_info);
    if (r != 0) {
        lwsl_err("lws http create client connect info failed\n");
    	http_ctx->response->inner_error_code = r;
        return 1;
    }
	lwsl_debug("[2]lws_http_connect with http_ctx->network_ctx %p\n", http_ctx->network_ctx);
    // connect to server
	struct lws *ret = lws_client_connect_via_info(&client_info);
    if (ret == NULL) {
        lwsl_err("lws client creation failed\n");
    	http_ctx->response->inner_error_code = VOLC_ERR_HTTP_CONN_FAILED;
        // interrupted = 1;
        bad = 2; /* could not even start client connection */
        lws_cancel_service(client_info.context);
		lwsl_debug("lws service cancelled due to lws_client_connect_via_info returned NULL");
        return VOLC_ERR_HTTP_CONN_FAILED;
    }
	lwsl_info("lws http connect started %s:%d\n", client_info.address, client_info.port);
    return 0;
}

http_response_t* lws_http_client_send_request(http_request_context_t *http_ctx) {
	lwsl_debug("lws_http_client_send_request begins\n");
	int n = 0;
    struct lws_context *ctx = (struct lws_context *)http_ctx->network_ctx;
    if (http_ctx->network_ctx == NULL) {
        lwsl_err("lws http send request network_ctx is null, call this after lws_http_connect.\n");
		return http_ctx->response;
    }
    // send http request
    if (http_ctx->is_async_request) {
		// async request
		lwsl_info("lws http async request\n");

	} else {
		lwsl_info("lws http sync request network_ctx %p\n", ctx);
		while (n >= 0 && !http_ctx->is_connection_completed && !interrupted)
			// lwsl_debug("lws http send request n %d\n", n);
			n = lws_service(ctx, 0);

		// if (close_after_start)
		// 	lws_wsi_close(client_wsi, LWS_TO_KILL_SYNC);

    }
	lwsl_debug("lws http send request done\n");
	return http_ctx->response;
}


int lws_http_client_deinit(http_request_context_t *http_ctx) {
    // release any internal http buffer
	lwsl_debug("lws_http_client_deinit begins\n");
	if (http_ctx->network_ctx != NULL) {
		lws_context_destroy(http_ctx->network_ctx);
	}
	lwsl_info("lws http deinit ends\n");
    return 0;
}

void lws_http_client_disconnect(http_request_context_t *http_ctx) {
	if (http_ctx->network_ctx == NULL) {
		lwsl_debug("lws http disconnect network_ctx is null, nothing to do\n");
		return;
	}
	struct lws_context *context = http_ctx->network_ctx;
	lwsl_info("lws http disconnect begin\n");
	lws_cancel_service(context); // cancel service to release any pending data
	http_ctx->is_connection_completed = 1;
	lwsl_info("lws http disconnect done\n");
}


void lws_http_client_destroy_network_context(http_request_context_t *http_ctx) {
	if (http_ctx->network_ctx == NULL) {
		lwsl_debug("lws http disconnect network_ctx is null, nothing to do\n");
		return;
	}
	struct lws_context *context = http_ctx->network_ctx;
	lws_context_destroy(context); // destroy context to free lws context memory
	lwsl_info("lws http disconnect done\n");
}



// create lwsc client connect info from onesdk http request context
int lws_http_client_init_client_connect_info(http_request_context_t *http_ctx, struct lws_client_connect_info *ccinfo) {
     // client connection info
     ccinfo->context = (struct lws_context *)http_ctx->network_ctx; // TODO must set to lws_context before use
     // parse url to host, path,
	 ccinfo->port = http_ctx_get_port(http_ctx);
     ccinfo->address = http_ctx_get_host(http_ctx);
     ccinfo->path = http_ctx_get_path(http_ctx);
     ccinfo->host = ccinfo->address;
     ccinfo->origin = ccinfo->address;
	 ccinfo->method = http_ctx_get_method_s(http_ctx);
	 lwsl_debug("lws_http_client_create_client_connect_info\n"
		"method %s\n"
		"host %s\n"
		"port %d\n"
		"path %s\n",
		 ccinfo->method,
		 ccinfo->host,
		 ccinfo->port,
		 ccinfo->path);
     ccinfo->ssl_connection = 0; //disable ssl by default
     // if set ca_crt or 443 port, enable ssl
     if (http_ctx->ca_crt != NULL || http_ctx_get_port(http_ctx) == HTTPS_PORT)  {
        lwsl_info("lws_http_client_create_client_connect_info enabled\n");
        ccinfo->ssl_connection = LCCSCF_USE_SSL;
	 }
	 ccinfo->alpn = "http/1.1";
#if defined(LWS_WITH_HTTP2)
        /* requires h2 */
        // if (lws_cmdline_option(a->argc, a->argv, "--long-poll")) {
            // lwsl_user("%s: long poll mode\n", __func__);
            // long_poll = 1;
        // }
		lwsl_debug("LWS_WITH_HTTP2 enabled");
		ccinfo->alpn = "h2,http/1.1";
#endif

	 if (http_ctx->client != NULL) {
	 	lwsl_debug("onesdk_http_ctx->client is not null, set user-agent and accept header");
	 	if (http_ctx->client->auth_user != NULL)
	 		ba_user = http_ctx->client->auth_user;
	 	if (http_ctx->client->auth_password!= NULL)
	 		ba_password = http_ctx->client->auth_password;
	 }
	 // tls/ssl related settings
	 if (http_ctx->ca_crt != NULL || http_ctx->is_ssl) {
		ccinfo->ssl_connection = LCCSCF_USE_SSL;
	 }
 	 if (!http_ctx->verify_ssl) {
	 	ccinfo->ssl_connection |= LCCSCF_ALLOW_SELFSIGNED; // 允许自签证书
		ccinfo->ssl_connection |= LCCSCF_ALLOW_EXPIRED; // 允许过期证书
 		ccinfo->ssl_connection |= LCCSCF_ALLOW_INSECURE; // 不验证证书
	}
     ccinfo->retry_and_idle_policy = &retry;
     ccinfo->userdata = http_ctx; // received in callback_onesdk_http
     ccinfo->protocol = protocols[0].name;
	 // ccinfo->pwsi = &client_wsi;
     // ccinfo->fi_wsi_name = "user";
    return 0;
}

int lws_http_client_parse_url(char *uri, const char **protocol, const char **host, int *port, const char **path) {
    // parse url to host, path, port
    // 解析 URI
	lwsl_debug("lws http parse url %s\n", uri);
    int result = lws_parse_uri(uri, protocol, host, port, path);
    if (result < 0) {
        fprintf(stderr, "Failed to parse URI: %s\n", uri);
        return 1;
    }
	return 0;
}

