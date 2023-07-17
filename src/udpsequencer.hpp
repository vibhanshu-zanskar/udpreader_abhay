#include "udpreader.hpp"
#include <atomic>
#include <sys/types.h>

namespace znsreader
{
class PacketSequencer
{
  public:
    PacketSequencer(short streamID, const std::string &primaryAddress, uint16_t primaryPort,
                    const std::string &secondaryAddress, uint16_t secondaryPort);
    ~PacketSequencer();
    ssize_t rcvSequencedPackets(unsigned char *buf, size_t bufLen);

  private:
    short m_streamID;
    std::atomic_int32_t m_lastSeqNo;
    PacketReader m_primaryReader;
    PacketReader m_secondaryReader;
};
}
