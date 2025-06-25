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

#include "mcp_client.h"

#define LATEST_PROTOCOL_VERSION "2025-03-26"


static bool mcp_endpoint_got(mcp_context_t *ctx,int *timeout_ms);



static uint32_t jsonrpc_requestID = 1;
static uint32_t get_jsonrpc_requestID(){
    if(jsonrpc_requestID < 65535){
        return jsonrpc_requestID++;
    }else{
        jsonrpc_requestID = 1;
        return 1;
    }
}


static char *build_initialize_request(int request_id){
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"jsonrpc","2.0");
    cJSON_AddNumberToObject(root,"id",request_id);
    cJSON_AddStringToObject(root,"method","initialize");


    cJSON *params = cJSON_CreateObject();
    cJSON_AddItemToObject(root,"params",params);

    cJSON_AddStringToObject(params,"protocolVersion",LATEST_PROTOCOL_VERSION);

    cJSON *clientinfo = cJSON_CreateObject();
    cJSON_AddItemToObject(params,"clientInfo",clientinfo);
    cJSON_AddStringToObject(clientinfo,"name","onesdk");
    cJSON_AddStringToObject(clientinfo,"version","1.0.0");

    cJSON *capabilities = cJSON_CreateObject();
    cJSON_AddItemToObject(params,"capabilities",capabilities);

    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);
    return json_string;
}


void mcp_client_init(mcp_context_t *ctx){
    int timeout_ms = 5000;
    if(!mcp_endpoint_got(ctx,&timeout_ms)){
        fprintf(stderr, "mcp_endpoint_got failed");
        return;
    }
    http_request_context_t *http_ctx = new_http_ctx();
    char url[128]={0};
    sprintf(url,"http://%s:%d%s",ctx->address,ctx->port,ctx->endpoint);
    int request_id = get_jsonrpc_requestID();
    ctx->initialize_request_id = request_id;
    http_ctx_set_url(http_ctx, url);
    http_ctx_set_method(http_ctx, HTTP_POST);
    http_ctx_add_header(http_ctx,"Content-Type","application/json");
    char *json_body = build_initialize_request(request_id);
    http_ctx_set_json_body(http_ctx,json_body);
    http_request(http_ctx);
    http_ctx_release(http_ctx);
    free(json_body);
}

//获取sse message的request id
static int sse_parse_request_id(char *data){
    cJSON *root = cJSON_Parse(data);
    if(root == NULL){
        return -1;
    }
    cJSON *id = cJSON_GetObjectItem(root, "id");
    if(id == NULL){
        return -1;
    }
    int ret = id->valueint;
    cJSON_Delete(root);
    return ret;
}


//获取sse message的result
static char* sse_get_response_result(char *data){
    cJSON *root = cJSON_Parse(data);
    if(root == NULL){
        return NULL;
    }
    cJSON *result = cJSON_GetObjectItem(root, "result");
    if(result == NULL){
        return NULL;
    }
    char *result_str = cJSON_Print(result);
    cJSON_Delete(root);
    return result_str;
}

static tools_list_t *mcp_tools_list_parse(const char *json){
    cJSON *root = cJSON_Parse(json);
    if(root == NULL){
        return NULL;
    }
    cJSON *tools = cJSON_GetObjectItem(root, "tools");
    if(tools == NULL){
        return NULL;
    }
    if(!cJSON_IsArray(tools)){
        return NULL;
    }
    int count = cJSON_GetArraySize(tools);
    tools_list_t *tools_list = (tools_list_t *)calloc(1,sizeof(tools_list_t));
    
    tools_list->count = count;

    tools_list->tools = (tool_t *)calloc(count,sizeof(tool_t));
    for(int i = 0; i < count; i++){
        cJSON *tool = cJSON_GetArrayItem(tools, i);
        cJSON *name = cJSON_GetObjectItem(tool, "name");
        cJSON *description = cJSON_GetObjectItem(tool, "description");
        cJSON *input_schema = cJSON_GetObjectItem(tool, "inputSchema");
        tools_list->tools[i].name = strdup(name->valuestring);
        tools_list->tools[i].description = strdup(description->valuestring);
        tools_list->tools[i].input_schema_raw = cJSON_Print(input_schema);
        // cJSON *type = cJSON_GetObjectItem(input_schema, "type");
        // cJSON *properties = cJSON_GetObjectItem(input_schema, "properties");
        // cJSON *required = cJSON_GetObjectItem(input_schema, "required");
        // tools_list->tools[i].input_schema.type = strdup(type->valuestring);
        // tools_list->tools[i].input_schema.properties = cJSON_Print(properties->valuestring);
        // tools_list->tools[i].input_schema.required = cJSON_Print(required);
    }
    cJSON_Delete(root);
    return tools_list;
}


void mcp_tools_list_free(tools_list_t *tools_list){
    for(int i = 0; i < tools_list->count; i++){
        free(tools_list->tools[i].name);
        free(tools_list->tools[i].description);
        free(tools_list->tools[i].input_schema_raw);
        // free(tools_list->tools[i].input_schema.type);
        // free(tools_list->tools[i].input_schema.properties);
        // free(tools_list->tools[i].input_schema.required);
    }
    free(tools_list->tools);
    free(tools_list);
}


static char *build_tools_list_request(int request_id){
    cJSON *root = cJSON_CreateObject();
    if(root == NULL){
        return NULL;
    }
    cJSON_AddStringToObject(root,"jsonrpc","2.0");
    cJSON_AddNumberToObject(root,"id",request_id);
    cJSON_AddStringToObject(root,"method","tools/list");
    // cJSON *params = cJSON_CreateObject();
    // cJSON_AddItemToObject(root,"params",params);
    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);
    return json_string;
}


static tool_result_t *mcp_tool_result_parse(const char *json){
    cJSON *root = cJSON_Parse(json);
        if(root == NULL){
        return NULL;
    }

    tool_result_t *tool_result = (tool_result_t *)calloc(1,sizeof(tool_result_t));

    cJSON *isError = cJSON_GetObjectItem(root, "isError");
    if(isError != NULL){
        tool_result->isError = isError->valueint;
    }
    cJSON *content = cJSON_GetObjectItem(root, "content");
    if(content == NULL){
        return NULL;
    }
    tool_result->content_count = cJSON_GetArraySize(content);
    tool_result->content = (tool_result_content_t *)calloc(tool_result->content_count,sizeof(tool_result_content_t));
    for(int i = 0; i < tool_result->content_count; i++){
        cJSON *content_item = cJSON_GetArrayItem(content, i);
        cJSON *type = cJSON_GetObjectItem(content_item, "type");
        cJSON *text = cJSON_GetObjectItem(content_item, "text");
        tool_result->content[i].type = strdup(type->valuestring);
        tool_result->content[i].text = strdup(text->valuestring);
    }
    cJSON_Delete(root);
    return tool_result;
}



void mcp_tool_result_free(tool_result_t *tool_result){
    for(int i = 0; i < tool_result->content_count; i++){
        free(tool_result->content[i].type);
        free(tool_result->content[i].text);
    }
    free(tool_result->content);
    free(tool_result);
}

char *mcp_get_tool_result_text(tool_result_t *tool_result){
    if(tool_result->content_count == 0){
        return NULL;
    }
    char *all_text = (char *)calloc(1,sizeof(char));
    for(int i = 0; i < tool_result->content_count; i++){
        if(strcmp(tool_result->content[i].type,"text") == 0){
            all_text = (char *)realloc(all_text,strlen(all_text)+strlen(tool_result->content[i].text)+strlen(" ")+1);
            strcat(all_text," ");
            strcat(all_text,tool_result->content[i].text);
        }
    }
    return all_text;
}


static char *build_tool_call_request(int request_id, const char *method,const char *arguments){
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"jsonrpc","2.0");
    cJSON_AddNumberToObject(root,"id",request_id);
    cJSON_AddStringToObject(root,"method","tools/call");


    cJSON *params = cJSON_CreateObject();
    cJSON_AddItemToObject(root,"params",params);

    cJSON_AddStringToObject(params,"name",method);

    cJSON *_arguments = cJSON_Parse(arguments);
    cJSON_AddItemToObject(params,"arguments",_arguments);

    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);
    return json_string;
}


static char *build_initialize_notification_request(){
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"jsonrpc","2.0");
    cJSON_AddStringToObject(root,"method","notifications/initialized");


    cJSON *params = cJSON_CreateObject();
    cJSON_AddItemToObject(root,"params",params);

    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);
    return json_string;
}


bool mcp_send_initialize_notification(mcp_context_t *ctx){
    int timeout_ms = 5000;
    if(!mcp_endpoint_got(ctx,&timeout_ms)){
        fprintf(stderr, "mcp_endpoint_got failed");
        return false;
    }
    http_request_context_t *http_ctx = new_http_ctx();
    char url[128]={0};
    sprintf(url,"http://%s:%d%s",ctx->address,ctx->port,ctx->endpoint);
    int request_id = get_jsonrpc_requestID();
    http_ctx_set_url(http_ctx, url);
    http_ctx_set_method(http_ctx, HTTP_POST);
    http_ctx_add_header(http_ctx,"Content-Type","application/json");
    char *json_body = build_initialize_notification_request();
    http_ctx_set_json_body(http_ctx,json_body);
    http_response_t * response = http_request(http_ctx);
    free(json_body);
    http_ctx_release(http_ctx);
    return true;
}


static void mcp_packet_handle(sse_context_t *sse_ctx,bool is_last_chunk,void *cb_user_data){
    mcp_context_t *ctx = (mcp_context_t *)cb_user_data;
    lws_pthread_mutex_lock(&ctx->lock);
    if(!ctx->endpoint_got && strcmp(sse_ctx->event,"endpoint") == 0){
        ctx->endpoint = strdup(sse_ctx->data);
        ctx->endpoint_got = true;
        mcp_client_init(ctx);
        lws_pthread_mutex_unlock(&ctx->lock);
        return;
    }
    if(ctx->messages.result){
        free(ctx->messages.result);
        ctx->messages.result = NULL;
    }
    int request_id = sse_parse_request_id(sse_ctx->data);
    ctx->messages.request_id = request_id;
    if(!ctx->initialized){
        if(request_id == ctx->initialize_request_id){
            mcp_send_initialize_notification(ctx);
            ctx->initialized = true;
        }
    }
    char *result = sse_get_response_result(sse_ctx->data);
    ctx->messages.result = result;
    lws_pthread_mutex_unlock(&ctx->lock);
}


mcp_context_t *mcp_ctx_new(){
    mcp_context_t *ctx = (mcp_context_t *)calloc(1,sizeof(mcp_context_t));
    ctx->http_ctx = new_http_ctx();
    return ctx;
}


void mcp_receiver_init(mcp_context_t *ctx,const char *ip, int port){
    lws_pthread_mutex_init(&ctx->lock);
    ctx->address = strdup(ip);
    ctx->port = port;
    char url[128]={0};
    sprintf(url,"http://%s:%d/sse",ctx->address,ctx->port);
    http_ctx_set_url(ctx->http_ctx, url); 
    http_ctx_set_method(ctx->http_ctx, HTTP_GET);
    http_ctx_add_header(ctx->http_ctx, "Accept", "text/event-stream");
    http_ctx_add_header(ctx->http_ctx, "Cache-Control", "no-cache");
    http_ctx_add_header(ctx->http_ctx, "Connection", "keep-alive");
    http_ctx_set_on_get_sse_cb(ctx->http_ctx, mcp_packet_handle, ctx);
}

int block_receive_from_sse_server(mcp_context_t *ctx){
    if(ctx == NULL){
        return -1;
    }
    http_request_async(ctx->http_ctx);
    http_wait_complete(ctx->http_ctx);
    return 0;
}

void mcp_ctx_free(mcp_context_t *ctx){
    if(ctx->messages.result){
        free(ctx->messages.result);
    }
    free(ctx->endpoint);
    free(ctx->address);
    free(ctx);
}



static bool mcp_endpoint_got(mcp_context_t *ctx,int *timeout_ms){
    if(ctx->endpoint_got){
        return true;
    }
    while (!ctx->endpoint_got && *timeout_ms > 0)
    {
        usleep(10 * 1000);
        *timeout_ms -= 10;
    }
    return ctx->endpoint_got;
}

static bool mcp_initialized(mcp_context_t *ctx,int *timeout_ms){
    if(ctx->initialized){
        return true;
    }
    while (!ctx->initialized && *timeout_ms > 0)
    {
        usleep(10 * 1000);
        *timeout_ms -= 10;
    }
    return ctx->initialized;
}


tool_result_t *mcp_tool_call(mcp_context_t *ctx,const char *name,const char *arguments){
    int timeout_ms = 5000;
    if(!mcp_endpoint_got(ctx,&timeout_ms)){
        fprintf(stderr, "mcp_endpoint_got failed");
        return NULL;
    }
    if(!mcp_initialized(ctx,&timeout_ms)){
        fprintf(stderr, "mcp_initialized failed");
        return NULL;
    }
    http_request_context_t *http_ctx = new_http_ctx();
    int request_id = get_jsonrpc_requestID();
    http_ctx_set_method(http_ctx, HTTP_POST);
    char url[256] = {0};
    sprintf(url,"http://%s:%d%s",ctx->address,ctx->port,ctx->endpoint);
    http_ctx_set_url(http_ctx,url);
    http_ctx_add_header(http_ctx,"Content-Type","application/json");
    char *json_body = build_tool_call_request(request_id,name,arguments);
    http_ctx_set_json_body(http_ctx, json_body);
    http_request(http_ctx);
    free(json_body);
    while (ctx->messages.request_id != request_id && timeout_ms > 0)
    {
        usleep(10 * 1000);
        timeout_ms -= 10;
    }
    if(timeout_ms == 0){
        fprintf(stderr, "mcp_tool_call timeout");
        http_ctx_release(http_ctx);
        return NULL;
    }
    if(ctx->messages.request_id == request_id){
        lws_pthread_mutex_lock(&ctx->lock);
        tool_result_t *result = mcp_tool_result_parse(ctx->messages.result);
        lws_pthread_mutex_unlock(&ctx->lock);
        http_ctx_release(http_ctx);
        return result;
    }
    http_ctx_release(http_ctx);
    return NULL;
}

tools_list_t *mcp_tools_list(mcp_context_t *ctx){
    int timeout_ms = 5000;
    if(!mcp_endpoint_got(ctx,&timeout_ms)){
        fprintf(stderr, "mcp_endpoint_got failed");
        return NULL;
    }
    if(!mcp_initialized(ctx,&timeout_ms)){
        fprintf(stderr, "mcp_initialized failed");
        return NULL;
    }
    http_request_context_t *http_ctx = new_http_ctx();
    int request_id = get_jsonrpc_requestID();
    http_ctx_set_method(http_ctx, HTTP_POST);
    char url[256] = {0};
    sprintf(url,"http://%s:%d%s",ctx->address,ctx->port,ctx->endpoint);
    http_ctx_set_url(http_ctx, url);
    http_ctx_add_header(http_ctx,"Content-Type","application/json");
    char *json_body = build_tools_list_request(request_id);
    http_ctx_set_json_body(http_ctx, json_body);
    http_request(http_ctx);
    free(json_body);
    while (ctx->messages.request_id != request_id && timeout_ms > 0)
    {
        usleep(10000);
        timeout_ms -= 10;
    }
    if(timeout_ms <= 0){
        fprintf(stderr, "mcp_tools_list timeout");
        http_ctx_release(http_ctx);
        return NULL;
    }
    if(ctx->messages.request_id == request_id){
        lws_pthread_mutex_lock(&ctx->lock);
        tools_list_t *result = mcp_tools_list_parse(ctx->messages.result);
        lws_pthread_mutex_unlock(&ctx->lock);
        http_ctx_release(http_ctx);
        return result;
    }
    http_ctx_release(http_ctx);
    return NULL;
}