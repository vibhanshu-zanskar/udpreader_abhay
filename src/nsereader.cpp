#include "ipinfo.hpp"
#include "nsetypes.hpp"
#include "udpreader.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <math.h>
#include <memory>
#include <pthread.h>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace znsreader;

// FO market tick by tick feeds
std::map<short, StreamPortIPInfo> FnOIPInfo{
    { 1, streamPortIPInfo(1, 17741, 10831, "239.70.70.41", "239.70.70.31") },
    { 2, streamPortIPInfo(2, 17742, 10832, "239.70.70.42", "239.70.70.32") },
    { 3, streamPortIPInfo(3, 17743, 10833, "239.70.70.43", "239.70.70.33") },
    { 4, streamPortIPInfo(4, 17744, 10834, "239.70.70.44", "239.70.70.34") },
    { 5, streamPortIPInfo(5, 17745, 10835, "239.70.70.45", "239.70.70.35") },
    { 6, streamPortIPInfo(6, 17746, 10836, "239.70.70.46", "239.70.70.36") },
    { 7, streamPortIPInfo(7, 17747, 10837, "239.70.70.47", "239.70.70.37") },
    { 8, streamPortIPInfo(8, 17748, 10838, "239.70.70.48", "239.70.70.38") },
    { 9, streamPortIPInfo(9, 17749, 10839, "239.70.70.49", "239.70.70.39") },
    { 10, streamPortIPInfo(10, 17750, 10840, "239.70.70.50", "239.70.70.40") },
    { 11, streamPortIPInfo(11, 17761, 10871, "239.70.70.61", "239.70.70.71") },
    { 12, streamPortIPInfo(12, 17762, 10872, "239.70.70.62", "239.70.70.72") },
    { 13, streamPortIPInfo(13, 17763, 10873, "239.70.70.63", "239.70.70.73") },
    { 14, streamPortIPInfo(14, 17764, 10874, "239.70.70.64", "239.70.70.74") },
    { 15, streamPortIPInfo(15, 17765, 10875, "239.70.70.65", "239.70.70.75") },
    { 16, streamPortIPInfo(16, 17766, 10876, "239.70.70.66", "239.70.70.76") },
};

int setThreadAffinity(std::thread &targetThread, int32_t cpuCore)
{
    if (cpuCore < 0) {
        return -1;
    }

    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    CPU_SET(cpuCore, &my_set);

    int rc = pthread_setaffinity_np(targetThread.native_handle(), sizeof(cpu_set_t), &my_set);
    if (rc != 0) {
        perror("pthread_setaffinity_np error:");
    }

    return rc;
}

size_t socket_to_ringbuf_writer(int fd, unsigned char *buf, std::size_t bufLen)
{
    ssize_t read_bytes = recv(fd, buf, bufLen, 0);
    if (read_bytes < 0) {
        return 0;
    }

    return read_bytes;
}

std::ofstream logFile("logs.txt");

size_t ringbuf_packer_processor(const unsigned char *buf, std::size_t bufLen)
{
    const unsigned char *packet = nullptr;

    for (packet = buf; packet < (buf + bufLen);) {
        const StreamPacket *full_packet = (StreamPacket *)packet;
        const StreamMsg *msgPtr = (StreamMsg *)(&full_packet->streamData);

        logFile << full_packet->streamHdr.streamId << ":" << full_packet->streamHdr.seqNo << std::endl;
        if (full_packet->streamHdr.msgLen > 0) {
            packet += full_packet->streamHdr.msgLen;
        } else {
            exit(-1);
            break;
        }
    }

    return (buf - packet);
}

/*
 *
 */
void tryPacketWriter(AggregatedPacketReader &packetReader)
{
    packetReader.write_packets_to_ringbuf();
}

/*
 * Create epoll objects and try to get packets.
 */
void tryPacketReader(AggregatedPacketReader &packetReader)
{
    packetReader.read_packets_from_ringbuf();
}

/*
 * Idea here is that we should be subscribing to all the sockets in FO feed.
 */
int main()
{
    std::thread readerThread;
    std::thread writerThread;
    AggregatedPacketReader packetReader(FnOIPInfo, socket_to_ringbuf_writer, ringbuf_packer_processor);

    readerThread = std::thread(tryPacketReader, std::ref(packetReader));
    writerThread = std::thread(tryPacketWriter, std::ref(packetReader));

    int ret = setThreadAffinity(writerThread, 0);
    if (ret < 0) {
        throw std::runtime_error("writer setaffinity error:");
    }

    ret = setThreadAffinity(readerThread, 1);
    if (ret < 0) {
        throw std::runtime_error("reader setaffinity error:");
    }

    writerThread.join();
    readerThread.join();
}
