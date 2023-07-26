#include "ipinfo.hpp"
#include "nsetypes.hpp"
#include "udpreader.hpp"
#include <algorithm>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace znsreader;

// FO market tick by tick feeds
std::map<short, struct streamPortIPInfo> FnOIPInfo{
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

/*
 * Idea here is that we should be subscribing to all the sockets in FO feed.
 */
int main()
{
    std::vector<PacketReader *> readerObjects;
    std::ofstream logFile("log.txt");

    readerObjects.reserve(FnOIPInfo.size());

    for (auto info : FnOIPInfo) {
        readerObjects.push_back(new PacketReader(info.second.m_primaryIP, info.second.m_primaryPort));
        readerObjects.push_back(new PacketReader(info.second.m_secondaryIP, info.second.m_secondaryPort));
    }

    unsigned char readBuf[65536];
    for (;;) {
        for (auto &reader : readerObjects) {
            ssize_t ret = reader->receivePackets(readBuf, 65536);
            if (ret < 0) {
                continue;
            }

            if (ret > 0) {
                const StreamPacket *packet = (StreamPacket *)readBuf;
                logFile << packet->streamHdr.streamId << ":" << packet->streamHdr.seqNo << std::endl;
            }
        }
    }
}
