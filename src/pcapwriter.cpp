#include "pcapwriter.hpp"
#include "nsetypes.hpp"
#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <net/ethernet.h>
#include <netinet/udp.h>
#include <pcap/dlt.h>
#include <pcap/pcap.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace znsreader
{
PacketToFileWriter::PacketToFileWriter(const std::map<short, StreamPortIPInfo> &stream_info, bool write_to_pcap = true,
                                       bool append_existing = false)
    : m_write_to_pcap(write_to_pcap), m_stream_info(stream_info), m_append_to_existing(append_existing)
{
    const size_t max_stream_id = get_max_streamid_key();

    if (max_stream_id == 0) {
        throw std::runtime_error("stream ids are incorrect in config");
    }

    m_file_fds.resize(max_stream_id + 1);
    m_file_names.resize(max_stream_id + 1);
    m_latest_seq_no.resize(max_stream_id + 1);

    for (auto &stream_info : m_stream_info) {
        const std::string &filename = generate_filename(stream_info.first);
        int fd;

        m_file_names.insert(m_file_names.begin() + stream_info.first, filename);

        if (m_append_to_existing) {
            fd = ::open(filename.c_str(), O_RDWR | O_APPEND);
        } else {
            fd = ::open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC, (S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP));
            if (fd < 0) {
                perror("open error:");
                if (errno == EEXIST) {
                    fd = ::open(filename.c_str(), O_RDWR | O_APPEND);
                }
            }
        }

        if (fd < 0) {
            throw std::runtime_error("Failed to create/open file: " + filename);
        }

        if (!m_append_to_existing && fd > 0) {
            struct pcap_file_header file_header;

            file_header.magic = 0xa1b23c4d; // 0xa1b2c3d4
            file_header.version_major = 2;
            file_header.version_minor = 4;
            file_header.thiszone = 0;
            file_header.sigfigs = 0;
            file_header.snaplen = 0x10000;
            file_header.linktype = DLT_NULL;

            size_t ret = ::write(fd, (void *)&file_header, sizeof(struct pcap_file_header));
            assert(ret == sizeof(struct pcap_file_header));
        }

        m_file_fds.insert(m_file_fds.begin() + stream_info.first, fd);
    }
}

PacketToFileWriter::~PacketToFileWriter()
{
    for (auto &onefd : m_file_fds) {
        ::close(onefd);
    }

    m_file_fds.clear();
    m_file_names.clear();
    m_latest_seq_no.clear();
}

int PacketToFileWriter::ingest_packet(const unsigned char *packet, size_t packet_len)
{
    short stream_id = ((StreamPacket *)packet)->streamHdr.streamId;
    int seq_number = ((StreamPacket *)packet)->streamHdr.seqNo;
    int active_fd = m_file_fds[stream_id];

    if (seq_number != 0) {
        if (seq_number <= m_latest_seq_no[stream_id]) {
            return 0;
        } else {
            // assert(m_latest_seq_no[stream_id] == (seq_number - 1));
            m_latest_seq_no[stream_id] = seq_number;
        }
    }

    if (m_write_to_pcap) {
        // Create a pcap packet and update result and sz.
        struct pcap_packet_hdr pcap_hdr;
        struct ether_header eth_hdr;
        struct ip ip_hdr;
        struct udphdr udp_hdr;

        {
            size_t total_len = sizeof(pcap_hdr) + sizeof(eth_hdr) + sizeof(ip_hdr) + sizeof(udp_hdr) + packet_len;
            int64_t time_in_nanos = time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now())
                                        .time_since_epoch()
                                        .count();
            pcap_hdr.tv_sec = time_in_nanos / 1e9;
            pcap_hdr.tv_usec = time_in_nanos - (1e9 * pcap_hdr.tv_sec);
            pcap_hdr.caplen = total_len;
            pcap_hdr.len = total_len;

            ::write(active_fd, &pcap_hdr, sizeof(pcap_hdr));
        }

        {
            eth_hdr.ether_type = ETHERTYPE_IP;
            ::write(active_fd, &eth_hdr, sizeof(eth_hdr));
        }

        {
            ip_hdr.ip_hl = 5;
            ip_hdr.ip_v = 4;
            ip_hdr.ip_p = IPPROTO_UDP;
            ip_hdr.ip_dst.s_addr = inet_addr(std::string(m_stream_info.find(stream_id)->second.m_primaryIP).c_str());
            ::write(active_fd, &ip_hdr, sizeof(ip_hdr));
        }

        {
            udp_hdr.dest = m_stream_info.find(stream_id)->second.m_primaryPort;
            udp_hdr.len = sizeof(udp_hdr) + packet_len;

            ::write(active_fd, &udp_hdr, sizeof(udp_hdr));
        }
    }

    ssize_t written = ::write(active_fd, packet, packet_len);
    if (written < 0) {
        throw std::runtime_error("Failed to write");
    }

    if (written < packet_len) {
        return -1;
    } else {
        return written;
    }
}
}
