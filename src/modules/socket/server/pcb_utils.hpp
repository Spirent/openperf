#include <array>
#include <cstdint>
#include <functional>
#include <variant>
#include <list>

struct tcp_pcb;
struct udp_pcb;

namespace openperf::socket::server {

struct ip_pcb_stats
{
    uint32_t if_index;
    uint8_t ip_address_family;
    std::array<uint8_t, 16> local_ip;
    std::array<uint8_t, 16> remote_ip;
};

struct tcp_pcb_stats : ip_pcb_stats
{
    uint16_t local_port;
    uint16_t remote_port;
    uint8_t state;

    uint16_t snd_queuelen;
};

struct udp_pcb_stats : ip_pcb_stats
{
    uint16_t local_port;
    uint16_t remote_port;
};

struct raw_pcb_stats : ip_pcb_stats
{
    uint8_t protocol;
};

struct packet_pcb_stats
{
    uint32_t if_index;
    int packet_type;
    uint16_t protocol;
};

typedef std::variant<std::monostate,
                     ip_pcb_stats,
                     tcp_pcb_stats,
                     udp_pcb_stats,
                     raw_pcb_stats,
                     packet_pcb_stats>
    pcb_stats;

struct socket_id
{
    pid_t pid;
    uint32_t sid;
};

struct channel_stats
{
    uint64_t txq_len;
    uint64_t rxq_len;
};

struct socket_pcb_stats
{
    const void* pcb;
    socket_id id;
    channel_stats channel_stats;
    pcb_stats pcb_stats;
};

/**
 * Iterate over all TCP PCBs and call a function on each PCB.
 */
void foreach_tcp_pcb(std::function<void(const tcp_pcb*)>&&);

/**
 * Get TCP PCB stats for the tcp_pcb.
 */
tcp_pcb_stats get_tcp_pcb_stats(const tcp_pcb*);

/**
 * Iterate over all UDP PCBs and call a function on each PCB.
 */
void foreach_udp_pcb(std::function<void(const udp_pcb*)>&&);

/**
 * Get UDP PCB stats for the udp_pcb.
 */
udp_pcb_stats get_udp_pcb_stats(const udp_pcb*);

/**
 * Get matching merged socket and PCB stats.
 */
std::vector<socket_pcb_stats> get_matching_socket_pcb_stats(
    std::function<bool(const void*)>&& pcb_match_func);

/**
 * Get all merged socket and PCB stats.
 */
std::vector<socket_pcb_stats> get_all_socket_pcb_stats();

void dump_socket_pcb_stats();

} // namespace openperf::socket::server