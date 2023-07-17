#include <cstdint>
#include <string>
#include <sys/types.h>

namespace znsreader
{
class PacketReader
{
  public:
    PacketReader(const std::string &address, uint16_t port);
    ~PacketReader();
    ssize_t receivePackets(unsigned char *buf, size_t bufLen);

  protected:
    int m_sock;

  private:
    uint16_t m_port;
    std::string m_address;
};
}
