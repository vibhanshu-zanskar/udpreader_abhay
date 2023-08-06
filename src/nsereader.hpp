#ifndef __SUBSCRIPTION_MGR_H
#define __SUBSCRIPTION_MGR_H

#include "ipinfo.hpp"
#include "ringbuffer.hpp"
#include "udpreader.hpp"
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

namespace znsreader
{
class SubscriptionManager
{
  public:
    using ZnsReadCallBack = RingBuffer::ReaderCallBack;
    SubscriptionManager() = delete;
    SubscriptionManager(std::map<short, single_stream_info> &, bool use_huge_pages, ZnsReadCallBack);
    ~SubscriptionManager();

    void add_reader_callback(ZnsReadCallBack);

  private:
    static void start_writer(SubscriptionManager &sub_mgr);
    static void start_reader(SubscriptionManager &sub_mgr);
    static int zns_set_thread_affinity(std::thread &target_thread, int32_t cpu_core);

    std::thread m_reader_thread;
    std::thread m_writer_thread;
    AggregatedPacketReader m_aggr_reader;
    std::vector<ZnsReadCallBack> m_read_callbacks;
};
}

#endif
