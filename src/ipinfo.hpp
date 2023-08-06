#ifndef __NSE_IP_INFO
#define __NSE_IP_INFO

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <sys/types.h>

typedef struct single_stream_info {
  public:
    single_stream_info(short stream_id, uint16_t prim_port, uint16_t sec_port, std::string_view prim_ip,
                       std::string_view sec_ip)
        : m_stream_id(stream_id),
          m_primary_port(prim_port),
          m_secondary_port(sec_port),
          m_primary_ip(prim_ip),
          m_secondary_ip(sec_ip)
    {
    }

    short m_stream_id;
    uint16_t m_primary_port;
    uint16_t m_secondary_port;
    std::string_view m_primary_ip;
    std::string_view m_secondary_ip;
} StreamPortIPInfo;

extern std::map<short, single_stream_info> stream_id_net_config;

#endif
