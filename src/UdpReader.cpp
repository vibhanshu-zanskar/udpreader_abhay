#include "nsetypes.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <pcap/pcap.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

class RawUdpPacketReader
{
  public:
    using RawPacketHandler = std::function<void(const unsigned char *data, size_t size)>;

    RawUdpPacketReader(const std::string &multicastAddress, uint16_t port, RawPacketHandler handler)
        : m_multicastAddress(multicastAddress), m_port(port), m_handler(handler)
    {
        m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_sock < 0)
        {
            perror("Failed to create socket");
            throw std::runtime_error("Failed to create socket");
        }

        int reuse = 1;
        if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
        {
            perror("setsockopt failed");
            throw std::runtime_error("Failed to set socket option");
        }

        struct sockaddr_in localSock;
        localSock.sin_family = AF_INET;
        localSock.sin_port = htons(port);
        localSock.sin_addr.s_addr = INADDR_ANY;

        if (bind(m_sock, (struct sockaddr *)&localSock, sizeof(localSock)) < 0)
        {
            perror("bind failed:");
            throw std::runtime_error("Failed to bind socket");
        }

        if (!multicastAddress.empty())
        {
            struct ip_mreq group;
            group.imr_multiaddr.s_addr = inet_addr(multicastAddress.c_str());
            group.imr_interface.s_addr = INADDR_ANY;
            int res = setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group));
            if (res < 0)
            {
                perror("setsockopt failed");
                throw std::runtime_error("Failed to join multicast group");
            }
        }
    }

    ~RawUdpPacketReader()
    {
        close(m_sock);
    }

    void
    receivePackets()
    {
        unsigned char buffer[65536];

        while (true)
        {
            ssize_t nbytes = recv(m_sock, buffer, sizeof(buffer), 0);
            if (nbytes > 0)
            {
                m_handler(buffer, nbytes);
            }
        }
    }

  private:
    int m_sock;
    uint16_t m_port;
    std::string m_multicastAddress;
    RawPacketHandler m_handler;
};

class RawUdpPacketServer
{
  public:
    RawUdpPacketServer(const std::filesystem::path &filename, const std::string &destinationAddress,
                       uint16_t destinationPort)
        : m_destinationAddress(destinationAddress), m_destinationPort(destinationPort)
    {
        // open the pcap file
        char errbuf[PCAP_ERRBUF_SIZE];
        m_pcap = pcap_open_offline(filename.c_str(), errbuf);
        if (m_pcap == nullptr)
        {
            throw std::runtime_error(errbuf);
        }

        // create the UDP socket
        m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_sock < 0)
        {
            throw std::runtime_error("Failed to create socket");
        }

        memset(&destAddr, 0, sizeof(destAddr));
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(destinationPort);
        destAddr.sin_addr.s_addr = inet_addr(destinationAddress.c_str());
    }

    ~RawUdpPacketServer()
    {
        close(m_sock);
        pcap_close(m_pcap);
    }

    void
    relayPackets()
    {
        pcap_loop(m_pcap, 0, handlePacket, reinterpret_cast<unsigned char *>(this));
    }

  private:
    static void
    handlePacket(unsigned char *user, const struct pcap_pkthdr *h, const unsigned char *bytes)
    {
        RawUdpPacketServer *relay = reinterpret_cast<RawUdpPacketServer *>(user);

        size_t headersLen = sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct udphdr);

        // send the packet over the UDP socket
        int res = sendto(relay->m_sock, bytes + headersLen, (h->caplen - headersLen), 0,
                         (struct sockaddr *)&relay->destAddr, sizeof(destAddr));
        if (res < 0)
        {
            perror("sendto failed");
            throw std::runtime_error("sendto failed:");
        }
    }

    uint16_t m_destinationPort;
    int m_sock;
    pcap_t *m_pcap;
    std::string m_destinationAddress;
    struct sockaddr_in destAddr;
};

void
runPcapServer(const std::filesystem::path &pcapFile)
{
    if (pcapFile.empty())
    {
        throw std::runtime_error("Pcap filename not passed");
    }

    if (!std::filesystem::exists(pcapFile))
    {
        throw std::runtime_error("Pcap file doesnt exist");
    }

    const std::string &serverAddr = "239.69.69.69";
    uint16_t serverPort = 22222;

    RawUdpPacketServer rawPacketServer(pcapFile, serverAddr, serverPort);
    rawPacketServer.relayPackets();
}

void
handlePacket(const unsigned char *rawPacket, size_t size)
{
    const StreamPacket *packet = (StreamPacket *)rawPacket;
    const StreamMsg *msgPtr = (StreamMsg *)(&packet->streamData);

    std::cout << "Msg :" << msgPtr->cMsgType << " size: " << size << std::endl;
    switch (static_cast<nseMsgType>(msgPtr->cMsgType))
    {
    case heartBeatMsg:
        break;
    case newOrderMsg:
        break;
    case modOrderMsg:
        break;
    case cancelOrderMsg:
        break;
    case tradeMesg:
        break;
    case newSpreadOrderMsg:
        break;
    case modSpreadOrderMsg:
        break;
    case cancelSpreadOrderMsg:
        break;
    case spreadTradeMsg:
        break;
    default:
        std::cout << "Unknown msg:" << static_cast<int8_t>(msgPtr->cMsgType) << " size: " << size << std::endl;
    }
}

void
runPcapClient()
{
    std::string serverAddr = "239.70.70.41";
    uint16_t serverPort = 17741;
    // std::string serverAddr = "239.69.69.69";
    // uint16_t serverPort = 22222;

    RawUdpPacketReader rawReader(serverAddr, serverPort, handlePacket);
    rawReader.receivePackets();
}

int
main(int argc, const char *argv[])
{
    bool runServer = true;
    std::thread serverThread;
    if (argc < 2)
    {
        std::cout << "No pcap file to stream" << std::endl;
        runServer = false;
    }

    std::thread clientThread(runPcapClient);

    if (runServer)
    {
        serverThread = std::thread(runPcapServer, argv[1]);
        serverThread.join();
    }

    clientThread.join();

    return 0;
}
