#include "udpsequencer.hpp"
#include <cstddef>
#include <stdexcept>

namespace znsreader
{

PacketSequencer::PacketSequencer(short streamID, const std::string &primaryAddress, uint16_t primaryPort,
                                 const std::string &secondaryAddress, uint16_t secondaryPort)
    : m_primaryReader(primaryAddress, primaryPort),
      m_secondaryReader(secondaryAddress, secondaryPort),
      m_lastSeqNo(0),
      m_streamID(streamID)
{
    if (primaryAddress == secondaryAddress)
    {
        throw std::runtime_error("Primary and secondary address cant be same");
    }
    else if (primaryPort == secondaryPort)
    {
        throw std::runtime_error("Primary and secondary port cant be same");
    }
}

PacketSequencer::~PacketSequencer()
{
}

ssize_t
PacketSequencer::rcvSequencedPackets(unsigned char *buf, size_t bufLen)
{
    return m_primaryReader.receivePackets(buf, bufLen);
}

}
