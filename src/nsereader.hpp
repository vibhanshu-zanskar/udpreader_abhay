#ifndef __SUBSCRIPTION_MGR_H
#define __SUBSCRIPTION_MGR_H

#include "udpsequencer.hpp"
#include <functional>
#include <vector>

namespace znsreader
{
class PacketSubscriptonManager
{
  public:
    using PacketHandler = std::function<void(const unsigned char *data, size_t size)>;

    PacketSubscriptonManager();
    ~PacketSubscriptonManager();
    ssize_t SubscribeStream(short streamID, PacketHandler callback);
    ssize_t UnsubscribeStream(short streamID, PacketHandler callback);

  private:
    int m_epollFd;
};
}

#endif
