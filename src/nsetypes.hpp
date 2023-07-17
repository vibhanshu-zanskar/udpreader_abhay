#ifndef _NSE_TYPES_H
#define _NSE_TYPES_H
#include <cstdint>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

static_assert(sizeof(short) == 2, "Type: short size is not 2 bytes");
static_assert(sizeof(int) == 4, "Type: int size is not 4 bytes");

#define ZNS_GCC_PACKED_ATTRIBUTE __attribute__((packed, aligned(1)))

typedef struct streamHeader
{
    short msgLen;
    short streamId;
    int seqNo; // For Equity derivative, data type shall be ‘unsigned int’)
} ZNS_GCC_PACKED_ATTRIBUTE StreamHeader;

static_assert(sizeof(StreamHeader) == 8, "Type: StreamHeader size is not 8 bytes");

enum nseMsgType : int8_t
{
    // Spread order messages
    newSpreadOrderMsg = 'G',
    modSpreadOrderMsg = 'H',
    cancelSpreadOrderMsg = 'J',
    spreadTradeMsg = 'K',

    // Normal order messages
    modOrderMsg = 'M',
    newOrderMsg = 'N',
    tradeMesg = 'T',
    cancelOrderMsg = 'X',

    heartBeatMsg = 'Z',

    // Tick recovery messages
    recoveryRequestMsg = 'R',
    recoveryResponseMsg = 'Y',
};

typedef struct heartBeatData
{
    int seqNo;
} ZNS_GCC_PACKED_ATTRIBUTE HeartBeatData;

typedef struct tickRecReqData
{
    short streamID;
    int startSeqNo;
    int endSeqNo;
} ZNS_GCC_PACKED_ATTRIBUTE TickRecReqData;

typedef struct tickRecRspData
{
    char reqStatus;
} ZNS_GCC_PACKED_ATTRIBUTE TickRecRspData;

typedef struct spreadTradeData
{
    int64_t timeStamp;
    double buyOrderID;
    double sellOrderID;
    int tokenID;
    int tradePrice;
    int quantity;
} ZNS_GCC_PACKED_ATTRIBUTE SpreadTradeData;

typedef struct spreadOrderData
{
    long timeStamp;
    double orderID;
    int tokenID;
    char orderType;
    int price;
    int quantity;
} ZNS_GCC_PACKED_ATTRIBUTE SpreadOrderData;

typedef struct tradeData
{
    long timeStamp;
    double buyOrderID;
    double sellOrderID;
    int tokenID;
    int tradePrice;
    int quantity;
} ZNS_GCC_PACKED_ATTRIBUTE TradeData;

typedef struct orderData
{
    long timeStamp;
    double orderID;
    int tokenID;
    char orderType;
    int price;
    int quantity;
} ZNS_GCC_PACKED_ATTRIBUTE OrderData;

typedef struct streamMsg
{
    char cMsgType;
    union
    {
        HeartBeatData hbData;
        TradeData tradeData;
        OrderData orderData;
        SpreadOrderData spdOrderData;
        SpreadTradeData spdTradeData;
    } p;
} ZNS_GCC_PACKED_ATTRIBUTE StreamMsg;

static_assert(sizeof(int8_t) == sizeof(char), "Type: size of int8_t and char are not same");

typedef struct streamPacket
{
    struct streamHeader streamHdr;
    struct streamMsg streamData;
} ZNS_GCC_PACKED_ATTRIBUTE StreamPacket;

#endif // _NSE_TYPES_H
