#include "pcapreader.hpp"
#include <cstring>

namespace znsreader
{
PcapPacketServer::PcapPacketServer(const std::filesystem::path &filename, const std::string &destAddr,
                                   uint16_t destPort, short streamID)
    : m_relayAddress(destAddr), m_relayPort(destPort), m_streamID(streamID)
{
    // open the pcap file
    char errbuf[PCAP_ERRBUF_SIZE];
    m_pcap = pcap_open_offline(filename.c_str(), errbuf);
    if (m_pcap == nullptr) {
        throw std::runtime_error(errbuf);
    }

    // create the UDP socket
    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sock < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    memset(&m_sockAddr, 0, sizeof(m_sockAddr));
    m_sockAddr.sin_family = AF_INET;
    m_sockAddr.sin_port = htons(destPort);
    m_sockAddr.sin_addr.s_addr = inet_addr(destAddr.c_str());
}

PcapPacketServer::~PcapPacketServer()
{
    close(m_sock);
    pcap_close(m_pcap);
}

void PcapPacketServer::relayPackets()
{
    pcap_loop(m_pcap, 0, handlePacket, reinterpret_cast<unsigned char *>(this));
}
}
