#include "nsereader.hpp"
#include "ipinfo.hpp"
#include "nsetypes.hpp"
#include "udpreader.hpp"
#include <cstdio>
#include <fstream>
#include <memory>
#include <pthread.h>
#include <stdexcept>
#include <thread>
#include <vector>

// FO market tick by tick feeds
std::map<short, single_stream_info> stream_id_net_config{
    { 1, single_stream_info(1, 17741, 10831, "239.70.70.41", "239.70.70.31") },
    { 2, single_stream_info(2, 17742, 10832, "239.70.70.42", "239.70.70.32") },
    { 3, single_stream_info(3, 17743, 10833, "239.70.70.43", "239.70.70.33") },
    { 4, single_stream_info(4, 17744, 10834, "239.70.70.44", "239.70.70.34") },
    { 5, single_stream_info(5, 17745, 10835, "239.70.70.45", "239.70.70.35") },
    { 6, single_stream_info(6, 17746, 10836, "239.70.70.46", "239.70.70.36") },
    { 7, single_stream_info(7, 17747, 10837, "239.70.70.47", "239.70.70.37") },
    { 8, single_stream_info(8, 17748, 10838, "239.70.70.48", "239.70.70.38") },
    { 9, single_stream_info(9, 17749, 10839, "239.70.70.49", "239.70.70.39") },
    { 10, single_stream_info(10, 17750, 10840, "239.70.70.50", "239.70.70.40") },
    { 11, single_stream_info(11, 17761, 10871, "239.70.70.61", "239.70.70.71") },
    { 12, single_stream_info(12, 17762, 10872, "239.70.70.62", "239.70.70.72") },
    { 13, single_stream_info(13, 17763, 10873, "239.70.70.63", "239.70.70.73") },
    { 14, single_stream_info(14, 17764, 10874, "239.70.70.64", "239.70.70.74") },
    { 15, single_stream_info(15, 17765, 10875, "239.70.70.65", "239.70.70.75") },
    { 16, single_stream_info(16, 17766, 10876, "239.70.70.66", "239.70.70.76") },
};

namespace znsreader
{
SubscriptionManager::SubscriptionManager(std::map<short, single_stream_info> &stream_config, bool use_huge_pages,
                                         ZnsReadCallBack reader_cbk)
    : m_aggr_reader(stream_config, use_huge_pages, reader_cbk)
{
    m_reader_thread = std::thread(start_reader, std::ref(*this));
    m_writer_thread = std::thread(start_writer, std::ref(*this));

    int ret = zns_set_thread_affinity(m_reader_thread, 0);
    if (ret < 0) {
        throw std::runtime_error("writer setaffinity error:");
    }

    ret = zns_set_thread_affinity(m_writer_thread, 1);
    if (ret < 0) {
        throw std::runtime_error("reader setaffinity error:");
    }
}

SubscriptionManager::~SubscriptionManager()
{
    m_writer_thread.join();
    m_reader_thread.join();
}

void SubscriptionManager::start_reader(SubscriptionManager &sub_mgr)
{
    sub_mgr.m_aggr_reader.read_packets_from_ringbuf();
}

void SubscriptionManager::start_writer(SubscriptionManager &sub_mgr)
{
    sub_mgr.m_aggr_reader.write_packets_to_ringbuf();
}

int SubscriptionManager::zns_set_thread_affinity(std::thread &target_thread, int32_t cpu_core)
{
    if (cpu_core < 0) {
        return -1;
    }

    cpu_set_t reader_set;
    CPU_ZERO(&reader_set);
    CPU_SET(cpu_core, &reader_set);

    int rc = pthread_setaffinity_np(target_thread.native_handle(), sizeof(cpu_set_t), &reader_set);
    if (rc != 0) {
        perror("pthread_setaffinity_np error:");
        throw std::runtime_error("failed to set cpu affinity");
    }

    return rc;
}

}

#include<iostream>
#include"pcapwriter.hpp"

znsreader::PacketToFileWriter pcap_write(stream_id_net_config, true, false);

size_t ringbuf_packet_processor(const unsigned char *buf, std::size_t bufLen)
{
    const unsigned char *packet = nullptr;

    for (packet = buf; packet < (buf + bufLen);) {
        const StreamPacket *full_packet = (StreamPacket *)packet;
        const StreamMsg *msgPtr = (StreamMsg *)(&full_packet->streamData);

        if (full_packet->streamHdr.msgLen > 0) {
            try{
                pcap_write.ingest_packet(packet, full_packet->streamHdr.msgLen);
            }
            catch(std::exception E){
                std::cerr << E.what() << " " << full_packet->streamHdr.streamId << ":" << full_packet->streamHdr.seqNo << std::endl;
                std::cout << E.what() << " " << full_packet->streamHdr.streamId << ":" << full_packet->streamHdr.seqNo << std::endl;
            }
            packet += full_packet->streamHdr.msgLen;
        } else {
            throw std::runtime_error("Found invalid msgLen");
        }
    }

    return (buf - packet);
}

int main()
{
    znsreader::SubscriptionManager zns_sub_manager(stream_id_net_config, true, ringbuf_packet_processor);
}
