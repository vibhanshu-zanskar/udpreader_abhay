#include "udpreader.hpp"
#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <iostream>
#include <iterator>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
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

    // This is recommeneded in NSE docs.
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

namespace znsreader
{
AggregatedPacketReader::AggregatedPacketReader(const std::map<short, StreamPortIPInfo> &streamInfo,
                                               PacketHandler handler)
    : m_handler(handler)
{
    for (auto &oneStreamInfo : streamInfo) {
        int pSocket = CreateUDPSocket(oneStreamInfo.second.m_primaryIP, oneStreamInfo.second.m_primaryPort);
        if (pSocket < 0) {
            perror("Failed to create socket");
            throw std::runtime_error("Failed to create primary socket");
        }

        m_sockets.push_back(pSocket);

        int sSocket = CreateUDPSocket(oneStreamInfo.second.m_secondaryIP, oneStreamInfo.second.m_secondaryPort);
        if (sSocket < 0) {
            perror("Failed to create socket");
            throw std::runtime_error("Failed to create secondary socket");
        }

        m_sockets.push_back(sSocket);
    }

    // Setup epoll structures.
    {
        m_epollfd = epoll_create1(0);
        if (m_epollfd < 0) {
            perror("epoll_create1 error:");
            throw std::runtime_error("epoll_create1 error");
        }

        {
            struct epoll_event ev;
            ev.events = EPOLLIN;

            for (auto &udpSocket : m_sockets) {
                ev.data.fd = udpSocket;

                int ret = epoll_ctl(m_epollfd, EPOLL_CTL_ADD, udpSocket, &ev);
                if (ret < 0) {
                    perror("epoll_ctl error:");
                    throw std::runtime_error("epoll_ctl error");
                }
            }
        }
    }
}

AggregatedPacketReader::~AggregatedPacketReader()
{
    for (auto &udpSocket : m_sockets) {
        close(udpSocket);
    }

    close(m_epollfd);

    m_sockets.clear();
}

ssize_t AggregatedPacketReader::receivePackets(unsigned char *buf, size_t bufLen, bool useBuf)
{
    struct epoll_event eventList[1024];
    struct timespec epollTimeout;
    unsigned char *local_buffer = buf;
    size_t local_buflen = bufLen;

    epollTimeout.tv_nsec = 10;
    epollTimeout.tv_sec = 0;

    for (;;) {
        int activeFds = epoll_pwait2(m_epollfd, eventList, 1024, nullptr, nullptr);
        if (activeFds < 0) {
            perror("epoll_wait error:");
            throw std::runtime_error("epoll_wait error");
        } else {
            local_buffer = buf;
            local_buflen = bufLen;
            for (int i = 0; i < activeFds; i++) {
                ssize_t rdSize = recv(eventList[i].data.fd, buf, bufLen, 0);
                if (rdSize < 0) {
                    perror("recv error:");
                    throw std::runtime_error("recv error");
                }

                if (rdSize > 0) {

                    std::cout << "Read : " << rdSize << std::endl;
                    // NOTE : rdSize can be zero.
                    local_buffer += rdSize;
                    local_buflen -= rdSize;
                }
            }

            if (m_handler && activeFds > 0) {
                m_handler(buf, bufLen);
            }
        }
    }
}

int AggregatedPacketReader::CreateUDPSocket(const std::string_view &ipv4Addr, uint16_t udpPort)
{
    int udpSocket;

    udpSocket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
    if (udpSocket < 0) {
        perror("Failed to create socket");
        throw std::runtime_error("Failed to create socket");
    }

    int reuse = 1;
    if (setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
        perror("setsockopt failed");
        throw std::runtime_error("Failed to set socket option");
    }

    // This is recommeneded in NSE docs.
    int rcvBufSize = 134217728;
    if (0 != setsockopt(udpSocket, SOL_SOCKET, SO_RCVBUF, &rcvBufSize, sizeof(rcvBufSize))) {
        perror("setsockopt failed");
        throw std::runtime_error("Failed to set socket option");
    }

    struct sockaddr_in localSock;
    localSock.sin_family = AF_INET;
    localSock.sin_port = htons(udpPort);
    localSock.sin_addr.s_addr = INADDR_ANY;

    if (bind(udpSocket, (struct sockaddr *)&localSock, sizeof(localSock)) < 0) {
        perror("bind failed:");
        throw std::runtime_error("Failed to bind socket");
    }

    if (!ipv4Addr.empty()) {
        struct ip_mreq group;
        group.imr_multiaddr.s_addr = inet_addr(std::string(ipv4Addr).c_str());
        group.imr_interface.s_addr = INADDR_ANY;
        int res = setsockopt(udpSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group));
        if (res < 0) {
            perror("setsockopt failed");
            throw std::runtime_error("Failed to join multicast group");
        }
    }

    return udpSocket;
}
}
