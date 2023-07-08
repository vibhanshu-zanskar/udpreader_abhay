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
