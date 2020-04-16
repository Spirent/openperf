/* Enhanced Packet Block without actual packet, options, and trailing
   Block Total Length */

namespace openperf::packet::capture::pcapng {

enum common_option_type : uint16_t { OPT_END = 0, COMMENT = 1 };

enum class interface_option_type : uint16_t {
    OPT_END = 0,
    COMMENT = 1,
    IF_NAME = 2,
    IF_DESCRIPTION = 3,
    IF_IPV4ADDR = 4,
    IF_IPV6ADDR = 5,
    IF_MACADDR = 6,
    IF_EUIADDR = 7,
    IF_SPEED = 8,
    IF_TSRESOL = 9,
    IF_TZONE = 10,
    IF_FILTER = 11,
    IF_OS = 12,
    IF_FCSLEN = 13,
    IF_TSOFFSET = 14,
    IF_HARDWARE = 15,
};

struct __attribute__((packed)) option_header
{
    uint16_t option_code;
    uint16_t option_length;
};

struct __attribute__((packed)) interface_option_header
{
    interface_option_type option_code;
    uint16_t option_length;
};

enum class block_type : uint32_t {
    SECTION = 0x0A0D0D0A,
    INTERFACE_DESCRIPTION = 1,
    ENHANCED_PACKET = 6,
};

enum class link_type : uint16_t {
    NONE = 0,
    ETHERNET = 1,
};

const uint32_t BYTE_ORDER_MAGIC = 0x1A2B3C4D;
const uint64_t SECTION_LENGTH_UNSPECIFIED = 0xFFFFFFFFFFFFFFFF;

// Pad block length to 4 byte boundaries
static inline uint32_t pad_block_length(uint32_t length)
{
    return (length + 3) & ~(uint32_t)0x03;
}

struct block_header
{
    block_type block_type;
    uint32_t block_length;
} __attribute__((packed));

struct section_block
{
    block_type block_type;
    uint32_t block_total_length;
    uint32_t byte_order_magic;
    uint16_t major_version;
    uint16_t minor_version;
    uint64_t section_length;
} __attribute__((packed));

struct interface_description_block
{
    block_type block_type;
    uint32_t block_total_length;
    link_type link_type;
    uint16_t reserved;
    uint32_t snap_len;
} __attribute__((packed));

struct interface_default_options
{
    struct
    {
        interface_option_header hdr;
        uint8_t resolution;
        uint8_t pad[3];
    } ts_resol;
    struct
    {
        interface_option_header hdr;
    } opt_end;
} __attribute__((packed));

struct enhanced_packet_block
{
    block_type block_type;
    uint32_t block_total_length;
    uint32_t interface_id;
    uint32_t timestamp_high;
    uint32_t timestamp_low;
    uint32_t captured_len;
    uint32_t packet_len;
} __attribute__((packed));

} // namespace openperf::packet::capture::pcapng
