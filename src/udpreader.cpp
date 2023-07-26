#include "udpreader.hpp"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace znsreader
{
PacketReader::PacketReader(const std::string_view &address, uint16_t port) : m_address(address), m_port(port)
{
    m_sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
    if (m_sock < 0) {
        perror("Failed to create socket");
        throw std::runtime_error("Failed to create socket");
    }

    int reuse = 1;
    if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
        perror("setsockopt failed");
        throw std::runtime_error("Failed to set socket option");
    }

    int rcvBufSize = 134217728;
    if (0 != setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, &rcvBufSize, sizeof(rcvBufSize))) {
        perror("setsockopt failed");
        throw std::runtime_error("Failed to set socket option");
    }

    struct sockaddr_in localSock;
    localSock.sin_family = AF_INET;
    localSock.sin_port = htons(port);
    localSock.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_sock, (struct sockaddr *)&localSock, sizeof(localSock)) < 0) {
        perror("bind failed:");
        throw std::runtime_error("Failed to bind socket");
    }

    if (!address.empty()) {
        struct ip_mreq group;
        group.imr_multiaddr.s_addr = inet_addr(std::string(address).c_str());
        group.imr_interface.s_addr = INADDR_ANY;
        int res = setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group));
        if (res < 0) {
            perror("setsockopt failed");
            throw std::runtime_error("Failed to join multicast group");
        }
    }
}

PacketReader::~PacketReader()
{
    std::cout << "Reader destroyed" << std::endl;
    close(m_sock);
}

ssize_t PacketReader::receivePackets(unsigned char *buf, size_t bufLen)
{
    return recv(m_sock, buf, bufLen, 0);
}
}
