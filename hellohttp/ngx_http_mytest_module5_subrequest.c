
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


static char * ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void * conf);
static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r);



static ngx_command_t ngx_http_mytest_commands[] = {
    {
        ngx_string("mytest"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_NOARGS,
        ngx_http_mytest,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};


static ngx_http_module_t ngx_http_mytest_module_ctx = {
    NULL,//preconfiguration
    NULL,//postconfiguration
    NULL,//create main configuration
    NULL,//init main configuration
    NULL,//create server configuration
    NULL,//merge server configuraion
    NULL,//create location configuration
    NULL//merge location configruation
};

ngx_module_t ngx_http_mytest_module = {
    NGX_MODULE_V1,
    &ngx_http_mytest_module_ctx,//module context
    ngx_http_mytest_commands,//module directives
    NGX_HTTP_MODULE,//module type
    NULL,//init master
    NULL,//init module
    NULL,//init process
    NULL,//init thread
    NULL,//exit thread
    NULL,//exit process

    NULL,//exit master
    NGX_MODULE_V1_PADDING
};


typedef ngx_int_t (*ngx_http_post_subrequest_pt)
(ngx_http_request_t *r, void *data, ngx_int_t rc);

static ngx_int_t mytest_subrequest_post_handler(ngx_http_request_t *r, 
    void *data, ngx_int_t rc);

static void mytest_post_handler(ngx_http_request_t *r);


typedef struct
{
    ngx_str_t stock[6];
} ngx_http_mytest_ctx_t;


static ngx_int_t mytest_subrequest_post_handler(ngx_http_request_t *r, 
    void *data, ngx_int_t rc)
{
    ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "mytest_subrequest_post_handler" );

    //r是子请求, parent是父请求
    ngx_http_request_t *pr = r->parent;
    ngx_http_mytest_ctx_t *myctx = ngx_http_get_module_ctx(pr, ngx_http_mytest_module);

    pr->headers_out.status = r->headers_out.status;
    //NGX_HTTP_OK, 新浪服务器处理成功, 开始解析HTTP包体
    if (r->headers_out.status == NGX_HTTP_OK)
    {
        int flag = 0;
        /*buffer保存上游服务器的响应*/
        ngx_buf_t *pRecvBuf = &r->upstream->buffer;
        /*解析上游服务器的响应, 并将解析出的值赋到上下文结构体myctx->stock数组中*/
        for (; pRecvBuf->pos != pRecvBuf->last; pRecvBuf->pos++)
        {

            if (* pRecvBuf->pos == ',' || * pRecvBuf->pos == '\"')
            {

                ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "pRecvBuf->pos: %s", pRecvBuf->pos);

                if (flag > 0)
                {
                    myctx->stock[flag - 1].len = pRecvBuf->pos - myctx->stock[flag - 1].data;
                }
                flag++;
                myctx->stock[flag - 1].data = pRecvBuf->pos + 1;
            }

            if (flag > 6)
            {
                break;
            }
        }
    }
    pr->write_event_handler = mytest_post_handler;

    return NGX_OK;
}

static void mytest_post_handler(ngx_http_request_t *r)
{
    //不是ok,直接将错误码返回给用户
    if (r->headers_out.status != NGX_HTTP_OK)
    {
        ngx_http_finalize_request(r, r->headers_out.status);
        return;
    }
    //当前请求是父请求,直接获取上下文
    ngx_http_mytest_ctx_t *myctx = ngx_http_get_module_ctx(r, ngx_http_mytest_module);

    ngx_str_t output_format = ngx_string("stock[%V], Today current price: %V, volumn: %V");
    //计算待发包体的长度
    int bodylen = output_format.len + myctx->stock[0].len + myctx->stock[1].len
    + myctx->stock[4].len - 6;
    r->headers_out.content_length_n = bodylen;

    //分配内存保存包体
    ngx_buf_t *b = ngx_create_temp_buf(r->pool, bodylen);

ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "stock0:%V", &myctx->stock[0]);
ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "stock1:%V", &myctx->stock[1]);
ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "stock4:%V", &myctx->stock[4]);


    ngx_snprintf(b->pos, bodylen, (char *)output_format.data, 
        &myctx->stock[0], &myctx->stock[1], &myctx->stock[4]);
    b->last = b->pos + bodylen;
    b->last_buf = 1;
    ngx_chain_t out;
    out.buf = b;
    out.next = NULL;

    static ngx_str_t type = ngx_string("text/plain, charset=GBK");
    r->headers_out.content_type  = type;
    r->headers_out.status = NGX_HTTP_OK;
    r->connection->buffered |=  NGX_HTTP_WRITE_BUFFERED;

    ngx_int_t ret = ngx_http_send_header(r);
    ret = ngx_http_output_filter(r, &out);
    //必须手动调用 ngx_http_finalize_request结束请求,HTTP框架不会再帮忙调用
    ngx_http_finalize_request(r, ret);
}

static char * ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void * conf){
    ngx_http_core_loc_conf_t *clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_mytest_handler;

    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r)
{
    ngx_http_mytest_ctx_t *myctx = ngx_http_get_module_ctx(r, ngx_http_mytest_module);
    if (myctx == NULL)
    {
        myctx = ngx_palloc(r->pool, sizeof(ngx_http_mytest_ctx_t));
        if (myctx == NULL)
        {
            return NGX_ERROR;
        }
        //将上下文设置到原始请求r中
        ngx_http_set_ctx(r, myctx, ngx_http_mytest_module);
    }
    //ngx_http_post_subrequest_t决定子请求的回调方法
    ngx_http_post_subrequest_t *psr = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
    if (psr == NULL)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    //设置子请求回调方法
    psr->handler = mytest_subrequest_post_handler;
    psr->data = myctx;

    ngx_str_t sub_prefix = ngx_string("/list=");
    ngx_str_t sub_location;
    sub_location.len = sub_prefix.len + r->args.len;
    sub_location.data = ngx_palloc(r->pool, sub_location.len);

    ngx_snprintf(sub_location.data, sub_location.len, "%V%V", &sub_prefix, &r->args);

    ngx_http_request_t *sr;
    ngx_int_t rc = ngx_http_subrequest(r, &sub_location, NULL, &sr, psr, NGX_HTTP_SUBREQUEST_IN_MEMORY);
    if (rc != NGX_OK)
    {
        return NGX_ERROR;
    }
    return NGX_DONE;
}

/*
server {
    listen 8080;

    location /list {
        proxy_pass http://hq.sinajs.cn;
        proxy_set_header  Accept-Encoding  "";
    }

    location /query {
        mytest;
    }

}

http://127.0.0.1:8080/query?list=s_sh000001

http://127.0.0.1:8080/query?s_sh000001
*/