#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char * ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r);

static void *ngx_http_mytest_create_loc_conf(ngx_conf_t *cf);

static char *ngx_conf_set_myconfig(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static char *ngx_http_mytest_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

ngx_int_t ngx_http_mytest_post_conf(ngx_conf_t *cf);

typedef struct
{
    ngx_str_t       my_str;
    ngx_int_t       my_num;
    ngx_flag_t      my_flag;
    size_t      my_size;
    ngx_array_t*    my_str_array;
    ngx_array_t*    my_keyval;
    off_t       my_off;
    ngx_msec_t      my_msec;
    time_t      my_sec;
    ngx_bufs_t      my_bufs;
    ngx_uint_t      my_enum_seq;
    ngx_uint_t  my_bitmask;
    ngx_uint_t      my_access;
    ngx_path_t* my_path;
    /*test my own config*/
    ngx_str_t       my_config_str;
    ngx_int_t       my_config_num;
} ngx_http_mytest_conf_t;

static ngx_conf_enum_t test_enums[] = {
    {ngx_string("apple"), 1},
    {ngx_string("banana"), 2},
    {ngx_string("orange"), 3},
    {ngx_null_string, 0},
};

static ngx_conf_bitmask_t test_bitmasks[] = {
    {ngx_string("good"), 0x0002},
    {ngx_string("better"), 0x0004},
    {ngx_string("best"), 0x0008},
    {ngx_null_string, 0},
};
//ngx_http_mytest_commands,数组结尾需要用ngx_null_command表示
static ngx_command_t  ngx_http_mytest_commands[] =
{
    {
        ngx_string("mytest"),//配置项名称name
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,//配置项类型,决定配置可以出现在哪些地方
        ngx_http_mytest,//出现了name中指定的配置项后,会回调set方法处理
        NGX_HTTP_LOC_CONF_OFFSET,//在配置文件中的偏移量
        0,//当前配置项在整个存储配置项的结构体中的偏移位置
        NULL//post 指针,
    },
    {//ngx_conf_set_flag_slot
        ngx_string("test_flag"),
        NGX_HTTP_LOC_CONF | NGX_CONF_FLAG,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mytest_conf_t, my_flag),
        NULL
    },
    {
        ngx_string("test_str"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mytest_conf_t, my_str),
        NULL
    },
    {
        ngx_string("test_str_array"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_array_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mytest_conf_t, my_str_array),
        NULL
    },
    {
        ngx_string("test_keyval"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE2,
        ngx_conf_set_keyval_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mytest_conf_t, my_keyval),
        NULL
    },
    {
        ngx_string("test_num"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_num_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mytest_conf_t, my_num),
        NULL
    },
    {
        ngx_string("test_size"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mytest_conf_t, my_size),
        NULL
    },
    {
        ngx_string("test_off"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_off_slot, NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mytest_conf_t, my_off),
        NULL
    },
    {
        ngx_string("test_msec"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_msec_slot, NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mytest_conf_t, my_msec),
        NULL
    },
    {
        ngx_string("test_sec"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_sec_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mytest_conf_t, my_sec),
        NULL
    },
    {
        ngx_string("test_bufs"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE2,
        ngx_conf_set_bufs_slot, NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mytest_conf_t, my_bufs),
        NULL
    },
    {
        ngx_string("test_enum"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_enum_slot, NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mytest_conf_t, my_enum_seq),
        test_enums
    },
    {
        ngx_string("test_bitmask"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_bitmask_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mytest_conf_t, my_bitmask),
        test_bitmasks
    },
    {
        ngx_string("test_access"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE123,
        ngx_conf_set_access_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mytest_conf_t, my_access),
        NULL
    },
    {
        ngx_string("test_path"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1234,
        ngx_conf_set_path_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mytest_conf_t, my_path),
        NULL
    },
    {
        ngx_string("test_myconfig"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE12,
        ngx_conf_set_myconfig,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};

static ngx_http_module_t  ngx_http_mytest_module_ctx =
{
    NULL,/* preconfiguration,解析配置文件前调用 */
    ngx_http_mytest_post_conf,/* postconfiguration,完成配置文件的解析后调用*/
    NULL,/* create main configuration, 需要创建数据结构存储main级别（直属于http{...}块的配置项的全局配置）） */
    NULL,/* init main configuration, 初始化main级别的配置 */
    NULL,/* create server configuration */
    NULL,/* merge server configuration */
    ngx_http_mytest_create_loc_conf,/* create location configuration */
    ngx_http_mytest_merge_loc_conf /* merge location configuration */
};

ngx_module_t  ngx_http_mytest_module =
{
    NGX_MODULE_V1,
    &ngx_http_mytest_module_ctx,           /* module context */
    ngx_http_mytest_commands,              /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static char * ngx_http_mytest_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_log_error(NGX_LOG_ALERT, cf->log, 0, "ngx_http_mytest_merge_loc_conf");


    ngx_http_mytest_conf_t *prev = (ngx_http_mytest_conf_t *)parent;
    ngx_http_mytest_conf_t *conf = (ngx_http_mytest_conf_t *)child;
    ngx_conf_merge_str_value(conf->my_str,
                             prev->my_str, "defaultstr");
    ngx_conf_merge_value(conf->my_flag, prev->my_flag, 0);


    return NGX_CONF_OK;
}


static char* ngx_conf_set_myconfig(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_log_error(NGX_LOG_ALERT, cf->log, 0, "ngx_conf_set_myconfig");

    //conf为HTTP框架传给用户在ngx_http_mytest_create_loc_conf回调方法中分配的结构体ngx_http_mytest_conf_t
    ngx_http_mytest_conf_t  *mycf = conf;

    ngx_str_t* value = cf->args->elts;

    if (cf->args->nelts > 1)
    {
        mycf->my_config_str = value[1];
    }
    if (cf->args->nelts > 2)
    {
        //将字符串形式转换为整数
        mycf->my_config_num = ngx_atoi(value[2].data, value[2].len);
        if (mycf->my_config_num == NGX_ERROR)
        {
            return "invalid number";
        }
    }
    ngx_log_error(NGX_LOG_ALERT, cf->log, 0, "my_config_str=%V, my_config_num=%i", &mycf->my_config_str, mycf->my_config_num);

    return NGX_CONF_OK;
}



static void* ngx_http_mytest_create_loc_conf(ngx_conf_t *cf)
{
    ngx_log_error(NGX_LOG_ALERT, cf->log, 0, "ngx_http_mytest_create_loc_conf");

    ngx_http_mytest_conf_t  *mycf;

    mycf = (ngx_http_mytest_conf_t  *)ngx_pcalloc(cf->pool, sizeof(ngx_http_mytest_conf_t));
    if (mycf == NULL)
    {
        return NULL;
    }

    mycf->my_flag = NGX_CONF_UNSET;
    mycf->my_num = NGX_CONF_UNSET;
    mycf->my_str_array = NGX_CONF_UNSET_PTR;
    mycf->my_keyval = NULL;
    mycf->my_off = NGX_CONF_UNSET;
    mycf->my_msec = NGX_CONF_UNSET_MSEC;
    mycf->my_sec = NGX_CONF_UNSET;
    mycf->my_size = NGX_CONF_UNSET_SIZE;

    return mycf;
}

//for test
extern ngx_module_t  ngx_http_module;
extern ngx_module_t  ngx_http_core_module;

void traversal(ngx_conf_t *cf, ngx_http_location_tree_node_t* node)
{
    if (node != NULL)
    {
        traversal(cf, node->left);
        ngx_http_core_loc_conf_t* loc = NULL;
        if (node->exact != NULL)
        {
            loc = (ngx_http_core_loc_conf_t*)node->exact;
        }
        else if (node->inclusive != NULL)
        {
            loc = (ngx_http_core_loc_conf_t*)node->inclusive;
        }

        if (loc != NULL)
        {
            ngx_http_mytest_conf_t  *mycf = (ngx_http_mytest_conf_t  *)loc->loc_conf[ngx_http_mytest_module.ctx_index];
            ngx_log_error(NGX_LOG_ALERT, cf->log, 0, "in location[name=%V]{} test_str=%V",
                          &loc->name, &mycf->my_str);
        }
        else
        {
            ngx_log_error(NGX_LOG_ALERT, cf->log, 0, "wrong location tree");
        }

        traversal(cf, node->right);
    }
}

ngx_int_t ngx_http_mytest_post_conf(ngx_conf_t *cf)
{
    ngx_log_error(NGX_LOG_ALERT, cf->log, 0, "ngx_http_mytest_post_conf");

    ngx_uint_t i = 0;
    ngx_http_conf_ctx_t* http_root_conf = (ngx_http_conf_ctx_t*)ngx_get_conf(cf->cycle->conf_ctx, ngx_http_module);

    ngx_http_mytest_conf_t  *mycf;
    mycf = (ngx_http_mytest_conf_t  *)http_root_conf->loc_conf[ngx_http_mytest_module.ctx_index];
    ngx_log_error(NGX_LOG_ALERT, cf->log, 0, "in http{} test_str=%V", &mycf->my_str);

    ngx_http_core_main_conf_t* core_main_conf = (ngx_http_core_main_conf_t*)
                                                http_root_conf->main_conf[ngx_http_core_module.ctx_index];

    for (i = 0; i < core_main_conf->servers.nelts; i++)
    {
        ngx_http_core_srv_conf_t* tmpcoresrv = *((ngx_http_core_srv_conf_t**)
                                                 (core_main_conf->servers.elts) + i);
        mycf = (ngx_http_mytest_conf_t  *)tmpcoresrv->ctx->loc_conf[ngx_http_mytest_module.ctx_index];
        ngx_log_error(NGX_LOG_ALERT, cf->log, 0, "in server[name=%V]{} test_str=%V",
                      &tmpcoresrv->server_name, &mycf->my_str);

        ngx_http_core_loc_conf_t* tmpcoreloc = (ngx_http_core_loc_conf_t*)
                                               tmpcoresrv->ctx->loc_conf[ngx_http_core_module.ctx_index];

        traversal(cf, tmpcoreloc->static_locations);
    }

    return NGX_OK;
}

static char * ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    clcf->handler = ngx_http_mytest_handler;

    return NGX_CONF_OK;
}


static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r)
{
    if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD)))
    {
        return NGX_HTTP_NOT_ALLOWED;
    }

    ngx_int_t rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK)
    {
        return rc;
    }

    ngx_str_t type = ngx_string("text/plain");
    ngx_str_t response = ngx_string("Hello test!");
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = response.len;
    r->headers_out.content_type = type;

    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
    {
        return rc;
    }

    ngx_buf_t                 *b;
    b = ngx_create_temp_buf(r->pool, response.len);
    if (b == NULL)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ngx_memcpy(b->pos, response.data, response.len);
    b->last = b->pos + response.len;
    b->last_buf = 1;

    ngx_chain_t out;
    out.buf = b;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);
}

/*
http {
    test_str main;
    server {
        listen 80;
        test_str server80;
        location /url1 {
            mytest;
            test_str locl;
            test_myconfig aaa 1;
        }
        location /url2 {
            mytest;
            test_str loc2;
            test_myconfig bbb 2;
        }
    }
    server {
        listen 8080;
        test_str server8080;
        location /url3 {
            mytest;
            test_str loc3;
        }
    }
}

sudo rm -fr /usr/local/nginx_teste-1.0.15 && ./configure --prefix=/usr/local/nginx_teste-1.0.15 --add-module=./src/hellohttp/ && make && sudo make install
*/

