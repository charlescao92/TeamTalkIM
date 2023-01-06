#ifndef LOGIN_SERVER_DISCOVERY_H_
#define LOGIN_SERVER_DISCOVERY_H_

#include <string>
#include <vector>

typedef struct
{
    std::string reg_center_addr;
    std::string service_id;  // 作为key
    std::string service_dir;
    std::string host_ip;
    int ttl;
    int client_port;
    int http_port;
    int msg_port;  
}RegisterCenterInfo;

typedef struct {
	std::string reg_center_addr;
	std::string service_dir;
	std::string host_ip;
	int ttl;
}LoginServerRegisterCenter;

int login_server_discovery_init(const LoginServerRegisterCenter *reg_center, std::string msg_ip1, std::string msg_ip2,
                                int msg_port, int msg_max_conn);
                                
#endif