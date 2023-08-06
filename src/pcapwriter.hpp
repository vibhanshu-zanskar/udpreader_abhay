#ifndef __ZNS_PACKET_TOFILEWRITER_H
#define __ZNS_PACKET_TOFILEWRITER_H

#include "ipinfo.hpp"
#include "nsetypes.hpp"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>

namespace znsreader
{
class PacketToFileWriter
{
  public:
    PacketToFileWriter() = delete;
    PacketToFileWriter(const std::map<short, single_stream_info> &, bool write_to_pcap, bool append_existing);
    ~PacketToFileWriter();
    int ingest_packet(const unsigned char *packet, size_t packet_len);

  private:
    inline short get_max_streamid_key()
    {
        size_t max_stream_id = 0;

        for (auto &one_stream : m_stream_info) {
            if (one_stream.first > max_stream_id) {
                max_stream_id = one_stream.first;
            }
        }

        return max_stream_id;
    }

    inline std::string generate_filename(short stream_id)
    {
        auto const time_today = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
        std::string date_today = std::format("{:%Y_%m_%d}", time_today);

        const auto &stream_info = m_stream_info.find(stream_id);
        if (stream_info == m_stream_info.end()) {
            throw std::runtime_error("stream id is not in map");
        }

        std::string primary_ip = std::string(stream_info->second.m_primary_ip);
        std::replace(primary_ip.begin(), primary_ip.end(), '.', '_');
        auto full_name = primary_ip + "__" + std::to_string(stream_info->second.m_primary_port) + "__"
                         + std::to_string(stream_id) + "__" + date_today;

        if (m_write_to_pcap) {
            return full_name + ".pcap";
        } else {
            return full_name + ".bin";
        }
    }

    struct pcap_packet_hdr {
        uint32_t tv_sec;
        uint32_t tv_usec;
        uint32_t caplen;
        uint32_t len;
    };

    bool m_write_to_pcap;
    bool m_append_to_existing;
    // TODO : Manage lifetime.
    const std::map<short, StreamPortIPInfo> &m_stream_info;
    // All these vectors are indexed by stream_id;
    std::vector<int> m_file_fds;
    std::vector<int64_t> m_latest_seq_no;
    std::vector<std::filesystem::path> m_file_names;
};
}

#endif // __ZNS_PACKET_TOFILEWRITER_H
