#include <cstdint>

static_assert(sizeof(short) == 2, "Type: short size is not 2 bytes");
static_assert(sizeof(int) == 4, "Type: int size is not 4 bytes");

typedef struct streamHeader
{
    short msgLen;
    short streamId;
    int seqNo; // For Equity derivative, data type shall be ‘unsigned int’)
} StreamHeader;

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

typedef struct streamData
{
    char cMsgType;
    char packetData[0];
} StreamData;

static_assert(sizeof(int8_t) == sizeof(char), "Type: size of int8_t and char are not same");

typedef struct heartBeatData
{
    int seqNo;
} HeartBeatData;

typedef struct tickRecReqData
{
    short streamID;
    int startSeqNo;
    int endSeqNo;
} TickRecReqData;

typedef struct tickRecRspData
{
    char reqStatus;
} TickRecRspData;

typedef struct spreadTradeData
{
    int64_t timeStamp;
    double buyOrderID;
    double sellOrderID;
    int tokenID;
    int tradePrice;
    int quantity;
} SpreadTradeData;

typedef struct spreadOrderData
{
    long timeStamp;
    double orderID;
    int tokenID;
    char orderType;
    int price;
    int quantity;
} SpreadOrderData;

typedef struct tradeData
{
    long timeStamp;
    double buyOrderID;
    double sellOrderID;
    int tokenID;
    int tradePrice;
    int quantity;
} TradeData;

typedef struct orderData
{
    long timeStamp;
    double orderID;
    int tokenID;
    char orderType;
    int price;
    int quantity;
} orderData;
