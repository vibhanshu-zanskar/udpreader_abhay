#include <cstdint>
#include <string>
#include <sys/types.h>

namespace znsreader
{
class PacketReader
{
  public:
    PacketReader(const std::string_view &address, uint16_t port);
    ~PacketReader();
    ssize_t receivePackets(unsigned char *buf, size_t bufLen);

    PacketReader(const PacketReader &) = delete;
    PacketReader &operator=(PacketReader const &) = delete;

    PacketReader(PacketReader &&) = default;
    PacketReader &operator=(PacketReader &&) = default;

  protected:
    int m_sock;

  private:
    uint16_t m_port;
    std::string_view m_address;
};
}
