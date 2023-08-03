#ifndef __UDP_READER_H
#define __UDP_READER_H

#include "ipinfo.hpp"
#include "ringbuffer.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <vector>

namespace znsreader
{
class PacketReader
{
  public:
    PacketReader() = delete;
    PacketReader(const std::string_view &address, uint16_t port);
    ~PacketReader();
    ssize_t receivePackets(unsigned char *buf, size_t bufLen);

    PacketReader(const PacketReader &) = delete;
    PacketReader &operator=(PacketReader const &) = delete;

    PacketReader(PacketReader &&) = default;
    PacketReader &operator=(PacketReader &&) = default;

  private:
    int m_sock;
    uint16_t m_port;
    std::string_view m_address;
};

class AggregatedPacketReader
{
  public:
    AggregatedPacketReader() = delete;
    AggregatedPacketReader(const std::map<short, StreamPortIPInfo> &, RingBuffer::WriterCallBack,
                           RingBuffer::ReaderCallBack);
    ~AggregatedPacketReader();
    void write_packets_to_ringbuf();
    void read_packets_from_ringbuf();

    AggregatedPacketReader(const AggregatedPacketReader &) = delete;
    AggregatedPacketReader &operator=(AggregatedPacketReader const &) = delete;

  private:
    int CreateUDPSocket(const std::string_view &ipv4Addr, uint16_t udpPort);

    int m_epollfd;
    std::vector<int> m_sockets;
    RingBuffer m_spsc_buffer;
};
}

#endif
