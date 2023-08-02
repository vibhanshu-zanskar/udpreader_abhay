#ifndef __PCAP_READER_H
#define __PCAP_READER_H

#include "nsetypes.hpp"
#include <filesystem>
#include <pcap/pcap.h>
#include <unistd.h>

namespace znsreader
{
class PcapPacketServer
{
  public:
    PcapPacketServer() = delete;
    PcapPacketServer(const std::filesystem::path &filename, const std::string &destAddr, uint16_t destPort,
                     short streamID);
    ~PcapPacketServer();
    void relayPackets();

  private:
    static void handlePacket(unsigned char *user, const struct pcap_pkthdr *h, const unsigned char *bytes)
    {
        PcapPacketServer *relay = reinterpret_cast<PcapPacketServer *>(user);

        size_t headersLen = sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct udphdr);

        // send the packet over the UDP socket
        int res = sendto(relay->m_sock, bytes + headersLen, (h->caplen - headersLen), 0,
                         (struct sockaddr *)&relay->m_sockAddr, sizeof(m_sockAddr));
        if (res < 0) {
            perror("sendto failed");
            throw std::runtime_error("sendto failed:");
        }
    }

    pcap_t *m_pcap;
    int m_sock;
    short m_streamID;
    uint16_t m_relayPort;
    std::string_view m_relayAddress;
    struct sockaddr_in m_sockAddr;
};
}

#endif
