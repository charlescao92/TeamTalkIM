#include "LoginServerDiscovery.h"

#include "cetcd.h"
#include "json/json.h"

#include "LoginServConn.h"
#include "ServInfo.h"

static RegisterCenterInfo g_reg_center;
static std::vector<serv_info_t> g_login_server_vector;

bool g_got_login_server  = false;
std::string g_msg_ip1;
std::string g_msg_ip2;
int g_msg_port;
int g_msg_max_conn;

int login_server_response_parse_callback (void *userdata, cetcd_response_node *node, int is_pre_node)
{
    printf("callback: %s\n", is_pre_node ?"pre_node":"node");
    printf("1 Node TTL: %lu\n", node->ttl);
    printf("1 Node ModifiedIndex: %lu\n", node->modified_index);
    printf("1 Node CreatedIndex: %lu\n", node->created_index);
    printf("2 Node Key: %s\n", node->key);
    printf("2 Node Value: %s\n", node->value);
    printf("1 Node Dir: %d\n", node->dir);
    printf("\n");

    if (node->dir > 0) {
        return 0;
    }
    
    Json::Reader reader;
    Json::Value value;

    if (!reader.parse(node->value, value))
    {
        printf("json parse failed, node->value=%s \n", node->value);
        log_error("json parse failed, node->value=%s \n", node->value);
        return 0;
    }

    serv_info_t server;
    server.server_ip = value["host_ip"].asString();
    server.server_port = value["msg_port"].asInt();
    printf("loginserver -> host_ip:%s, msg_port:%d \n", server.server_ip.c_str(), server.server_port);
    log("loginserver -> host_ip:%s, msg_port:%d \n", server.server_ip.c_str(), server.server_port);

    if (!g_got_login_server)
    {
        // 检测到服务后，后续不再更新服务地址
        // 这里只有没有发现LoginServer的情况下才去获取
        g_login_server_vector.push_back(server);
    }
    
    return 0;
}

void login_server_discovery_timer_callback (void *userdata, uint8_t msg, uint32_t handle, void *pParam)
{
    cetcd_client cli;
    cetcd_response *resp;
    cetcd_array addrs;

    cetcd_array_init(&addrs, 3);
    cetcd_array_append(&addrs, (void *)g_reg_center.reg_center_addr.c_str());

    cetcd_client_init(&cli, &addrs);

    resp = cetcd_lsdir(&cli, g_reg_center.service_dir.c_str(), 1, 1);
    if (resp->err)
    {
        printf("error :%d, %s (%s) \n", resp->err->ecode, resp->err->message, resp->err->cause);
    }
    cetcd_response_print(resp);
    printf("\n--------cetcd_response_parse-----------\n");
    cetcd_response_parse(resp, login_server_response_parse_callback, nullptr);

    size_t server_count = g_login_server_vector.size();
    if(!g_got_login_server && server_count > 0)
    {
        serv_info_t *server_list = new serv_info_t[server_count];
        for (size_t i = 0; i < server_count; i++)
        {
            server_list[i].server_ip = g_login_server_vector[i].server_ip;
            server_list[i].server_port = g_login_server_vector[i].server_port;
        }
        printf("login_server_discovery_timer_callback init_login_serv_conn \n");
        init_login_serv_conn(server_list, server_count, g_msg_ip1.c_str(), g_msg_ip2.c_str(), g_msg_port, g_msg_max_conn);
        g_got_login_server = true;
    }

    cetcd_response_release(resp);

    cetcd_array_destroy(&addrs);
    cetcd_client_destroy(&cli);
}

int login_server_discovery_init(const LoginServerRegisterCenter *reg_center, std::string msg_ip1, std::string msg_ip2,
                                int msg_port, int msg_max_conn)
{
    g_reg_center.reg_center_addr  = reg_center->reg_center_addr;
    g_reg_center.service_dir = reg_center->service_dir;
    g_reg_center.host_ip = reg_center->host_ip;
    g_reg_center.ttl = reg_center->ttl;
    g_msg_ip1 = msg_ip1;
    g_msg_ip2 = msg_ip2;
    g_msg_port = msg_port;
    g_msg_max_conn = msg_max_conn;

    // 第一步， 检测service dir是否存在，如果不存在则创建
    cetcd_client cli;
    cetcd_response *resp;
    cetcd_array addrs;

    printf("etcd addr: %s \n", g_reg_center.reg_center_addr.c_str());

    cetcd_array_init(&addrs, 3);
    cetcd_array_append(&addrs, (void *)g_reg_center.reg_center_addr.c_str());

    cetcd_client_init(&cli, &addrs);

    resp = cetcd_lsdir(&cli, g_reg_center.service_dir.c_str(), 1, 1);
    if (resp->err)
    {
        printf("error :%d, %s (%s) \n", resp->err->ecode, resp->err->message, resp->err->cause);
    }
    cetcd_response_print(resp);
    printf("\n--------cetcd_response_parse-----------\n");
    cetcd_response_parse(resp, login_server_response_parse_callback, nullptr);

    size_t server_count = g_login_server_vector.size();
    if(server_count > 0)
    {
        serv_info_t *server_list = new serv_info_t[server_count];
        for (size_t i = 0; i < server_count; i++)
        {
            server_list[i].server_ip = g_login_server_vector[i].server_ip;
            server_list[i].server_port = g_login_server_vector[i].server_port;
        }
        printf("login_server_discovery_init init_login_serv_conn \n");
        init_login_serv_conn(server_list, server_count, g_msg_ip1.c_str(), g_msg_ip2.c_str(), g_msg_port, g_msg_max_conn);
        g_got_login_server = true;
    }

    cetcd_response_release(resp);

    cetcd_array_destroy(&addrs);
    cetcd_client_destroy(&cli);

    netlib_register_timer(login_server_discovery_timer_callback, NULL, g_reg_center.ttl);
}
