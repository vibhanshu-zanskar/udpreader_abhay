#ifndef __UDP_READER_H
#define __UDP_READER_H

#include "ipinfo.hpp"
#include "ringbuffer.hpp"
#include <map>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

namespace znsreader
{
class AggregatedPacketReader
{
  public:
    AggregatedPacketReader() = delete;
    AggregatedPacketReader(const std::map<short, single_stream_info> &, bool, RingBuffer::ReaderCallBack);
    ~AggregatedPacketReader();

    AggregatedPacketReader(const AggregatedPacketReader &) = delete;
    AggregatedPacketReader &operator=(AggregatedPacketReader const &) = delete;

    void write_packets_to_ringbuf();
    void read_packets_from_ringbuf();

    static std::size_t socket_to_ringbuf_writer(int fd, unsigned char *buf, std::size_t bufLen);

  private:
    int create_udp_socket(const std::string_view &ipv4Addr, uint16_t udpPort);

    int m_epollfd;
    std::vector<int> m_sockets;
    RingBuffer m_spsc_buffer;
};
}

#endif
