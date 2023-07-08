#include "nsetypes.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <functional>
#include <iostream>
#include <netinet/in.h>
#include <pcap.h>
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
    RawUdpPacketServer(const std::string &filename, const std::string &destinationAddress, uint16_t destinationPort)
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

        struct in_addr addr;
        addr.s_addr = inet_addr(destinationAddress.c_str());
        int res = setsockopt(m_sock, IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof(addr));
        if (res < 0)
        {
            perror("setsockopt failed");
            throw std::runtime_error("Failed to join multicast group");
        }
    }

    ~RawUdpPacketServer()
    {
        close(m_sock);
        pcap_close(m_pcap);
    }

    void
    relayPackets()
    {
        pcap_loop(m_pcap, -1, handlePacket, reinterpret_cast<unsigned char *>(this));
    }

  private:
    static void
    handlePacket(unsigned char *user, const struct pcap_pkthdr *h, const unsigned char *bytes)
    {
        RawUdpPacketServer *relay = reinterpret_cast<RawUdpPacketServer *>(user);

        // send the packet over the UDP socket
        struct sockaddr_in dest;
        dest.sin_family = AF_INET;
        dest.sin_port = htons(relay->m_destinationPort);
        dest.sin_addr.s_addr = inet_addr(relay->m_destinationAddress.c_str());
        sendto(relay->m_sock, bytes, h->caplen, 0, (struct sockaddr *)&dest, sizeof(dest));
        std::cout << "Sent:" << h->len << std::endl;
    }

    int m_sock;
    uint16_t m_destinationPort;
    pcap_t *m_pcap;
    std::string m_destinationAddress;
};

void
runPcapServer()
{
    std::string relayFile = "./data/NSE.pcap";
    std::string serverAddr = "192.168.1.4";
    uint16_t serverPort = 22222;

    RawUdpPacketServer rawPacketServer(relayFile, serverAddr, serverPort);
    rawPacketServer.relayPackets();
}

void
handlePacket(const unsigned char *data, size_t size)
{
    std::cout << "Recieved:" << size << std::endl;
}

void
runPcapClient()
{
    // std::string serverAddr = "239.70.70.41";
    // uint16_t serverPort = 17741;
    std::string serverAddr = "192.168.1.4";
    uint16_t serverPort = 22222;

    RawUdpPacketReader rawReader(serverAddr, serverPort, handlePacket);
    rawReader.receivePackets();
}

int
main()
{
    std::thread serverThread(runPcapServer);
    std::thread clientThread(runPcapClient);

    clientThread.join();
    serverThread.join();

    return 0;
}
