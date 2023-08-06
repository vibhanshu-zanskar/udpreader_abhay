#include "udpreader.hpp"
#include "ipinfo.hpp"
#include "ringbuffer.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <iostream>
#include <iterator>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace znsreader
{
AggregatedPacketReader::AggregatedPacketReader(const std::map<short, single_stream_info> &ip_port_config,
                                               bool use_huge_pages, RingBuffer::ReaderCallBack reader_fn)
    : m_spsc_buffer(1024 * 1024 * 1024, use_huge_pages, socket_to_ringbuf_writer, reader_fn)
{
    for (auto &one_stream : ip_port_config) {
        int p_socket = create_udp_socket(one_stream.second.m_primary_ip, one_stream.second.m_primary_port);
        if (p_socket < 0) {
            perror("Failed to create socket");
            throw std::runtime_error("Failed to create primary socket");
        }

        m_sockets.push_back(p_socket);

        int s_socket = create_udp_socket(one_stream.second.m_secondary_ip, one_stream.second.m_secondary_port);
        if (s_socket < 0) {
            perror("Failed to create socket");
            throw std::runtime_error("Failed to create secondary socket");
        }

        m_sockets.push_back(s_socket);
    }

    // Setup epoll structures.
    {
        struct epoll_event ev;
        ev.events = EPOLLIN;

        m_epollfd = epoll_create1(0);
        if (m_epollfd < 0) {
            perror("epoll_create1 error:");
            throw std::runtime_error("epoll_create1 error");
        }

        for (auto &udp_socket_fd : m_sockets) {
            ev.data.fd = udp_socket_fd;

            int ret = epoll_ctl(m_epollfd, EPOLL_CTL_ADD, udp_socket_fd, &ev);
            if (ret < 0) {
                perror("epoll_ctl error:");
                throw std::runtime_error("epoll_ctl error");
            }
        }
    }
}

AggregatedPacketReader::~AggregatedPacketReader()
{
    for (auto &udp_socket_fd : m_sockets) {
        close(udp_socket_fd);
    }

    close(m_epollfd);

    m_sockets.clear();
}

void AggregatedPacketReader::write_packets_to_ringbuf()
{
    struct epoll_event eventList[1024];

    struct timespec epollTimeout;
    epollTimeout.tv_nsec = 10;
    epollTimeout.tv_sec = 0;

    for (;;) {
        int activeFds = epoll_pwait2(m_epollfd, eventList, 1024, nullptr, nullptr);
        if (activeFds < 0) {
            perror("epoll_wait error:");
            throw std::runtime_error("epoll_wait error");
        } else {
            for (int i = 0; i < activeFds; i++) {
                while (m_spsc_buffer.push(eventList[i].data.fd, 131072) == 0) {
                }
            }
        }
    }
}

void AggregatedPacketReader::read_packets_from_ringbuf()
{
    for (;;) {
        while (m_spsc_buffer.pop_all() != 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }
}

int AggregatedPacketReader::create_udp_socket(const std::string_view &ipv4Addr, uint16_t udpPort)
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

std::size_t AggregatedPacketReader::socket_to_ringbuf_writer(int fd, unsigned char *buf, std::size_t bufLen)
{
    ssize_t read_bytes = recv(fd, buf, bufLen, 0);
    if (read_bytes < 0) {
        // NOTE : Might have to handle this error, returning zero for now.
        return 0;
    }

    return read_bytes;
}
}
