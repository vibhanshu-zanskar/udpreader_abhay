#ifndef __NSE_IP_INFO
#define __NSE_IP_INFO

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <sys/types.h>

typedef struct streamPortIPInfo
{
  public:
    streamPortIPInfo(short streamID, uint16_t primaryPort, uint16_t secondaryPort, const char *primaryIP,
                     const char *secondaryIP)
        : m_streamID(streamID),
          m_primaryPort(primaryPort),
          m_secondaryPort(secondaryPort),
          m_primaryIP(primaryIP),
          m_secondaryIP(secondaryIP)
    {
    }

    short m_streamID;
    uint16_t m_primaryPort;
    uint16_t m_secondaryPort;
    std::string_view m_primaryIP;
    std::string_view m_secondaryIP;
} StreamPortIPInfo;

extern std::map<short, StreamPortIPInfo> FnOIPInfo;

#endif
