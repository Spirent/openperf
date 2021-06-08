#include "packet/generator/learning.hpp"

#include "utils/overloaded_visitor.hpp"
#include "lwip/priv/tcpip_priv.h"
#include "lwip/etharp.h"
#include "arpa/inet.h"
#include "stack/lwip/nd6_op.h"

#include <future>

namespace openperf::packet::generator {

static constexpr std::chrono::seconds start_callback_timeout(1);
static constexpr std::chrono::seconds check_callback_timeout(1);

static constexpr std::chrono::seconds poll_check_interval(1);
static constexpr int max_poll_count = 30;

static constexpr int ND_NEIGHBOR_CACHE_NO_ENTRY =
    -127; // -1 and -4 are already used to signal other errors by LwIP.

bool all_addresses_resolved(const learning_results& results)
{
    auto ipv4_resolved = std::all_of(
        results.ipv4.begin(), results.ipv4.end(), [](const auto& address_pair) {
            return (address_pair.second);
        });

    auto ipv6_resolved = std::all_of(
        results.ipv6.begin(), results.ipv6.end(), [](const auto& address_pair) {
            return (address_pair.second.next_hop_mac);
        });

    return (ipv4_resolved && ipv6_resolved);
}

// Structure that gets "passed" to the stack thread via tcpip_callback.
struct start_learning_params
{
    netif* intf = nullptr;       // lwip interface to use.
    learning_results& to_learn;  // addresses to learn.
    std::promise<err_t> barrier; // keep generator and stack threads in sync.
};

static err_t send_ipv4_learning_requests(start_learning_params& slp)
{
    // start_learning_params contains a unique list of IP addresses to learn.
    // Addresses are assumed to be on-link. Caller must first apply routing
    // logic. Caller must not send IP addresses from a different subnet.

    err_t overall_result = ERR_OK;
    std::for_each(
        slp.to_learn.ipv4.begin(),
        slp.to_learn.ipv4.end(),
        [&](const auto& addr_pair) {
            // If we've already had an error don't bother trying to learn this
            // address.
            if (overall_result != ERR_OK) { return; }

            // Is this entry unresolved?
            // Don't repeat learning for addresses we've already resolved.
            if (addr_pair.second) { return; }

            ip4_addr_t target{
                .addr = htonl(addr_pair.first.template load<uint32_t>())};

            OP_LOG(OP_LOG_TRACE,
                   "Sending ARP request for IP: %s\n",
                   to_string(addr_pair.first).c_str());

            auto result = etharp_query(slp.intf, &target, nullptr);
            if (result != ERR_OK) {
                OP_LOG(OP_LOG_ERROR,
                       "Error (%s) encountered while requesting ARP for "
                       "address: %s",
                       strerror(err_to_errno(result)),
                       to_string(addr_pair.first).c_str());
                overall_result = result;
            }
        });

    return (overall_result);
}

static libpacket::type::ipv6_address
make_libpacket_address(const ip6_addr_t& stack_address)
{
    // Function assumes address from stack is in NETWORK byte order!
    uint32_t host_byte_order_ip[4];
    host_byte_order_ip[0] = ntohl(stack_address.addr[0]);
    host_byte_order_ip[1] = ntohl(stack_address.addr[1]);
    host_byte_order_ip[2] = ntohl(stack_address.addr[2]);
    host_byte_order_ip[3] = ntohl(stack_address.addr[3]);

    return (libpacket::type::ipv6_address(host_byte_order_ip));
}

using nd_cache_entry_data =
    std::pair<libpacket::type::ipv6_address, // next hop ipv6 address
              libpacket::type::mac_address   // next hop mac address
              >;

static std::optional<nd_cache_entry_data>
get_nd_cache_entry(int cache_entry_index)
{
    ip6_addr_t* ip_addr = nullptr;
    u8_t* ll_addr = nullptr;

    if (neighbor_cache_get_entry(cache_entry_index, &ip_addr, &ll_addr)) {
        auto next_hop = make_libpacket_address(*ip_addr);

        return (std::make_optional(
            std::make_pair(next_hop, libpacket::type::mac_address(ll_addr))));
    }

    return (std::nullopt);
}

static ip6_addr_t
make_stack_address(const libpacket::type::ipv6_address& libpacket_address)
{
    // Resulting address is in NETWORK byte order!
    ip6_addr_t to_return;
    to_return.addr[0] = htonl(libpacket_address.template load<uint32_t>(0));
    to_return.addr[1] = htonl(libpacket_address.template load<uint32_t>(1));
    to_return.addr[2] = htonl(libpacket_address.template load<uint32_t>(2));
    to_return.addr[3] = htonl(libpacket_address.template load<uint32_t>(3));

    return (to_return);
}

static err_t send_ipv6_learning_requests(start_learning_params& slp)
{
    // start_learning_params contains a unique list of IP addresses to learn.
    // For IPv6 the stack performs "routing" (i.e. on/off link decisions.)

    err_t overall_result = ERR_OK;
    std::for_each(
        slp.to_learn.ipv6.begin(),
        slp.to_learn.ipv6.end(),
        [&](auto& addr_pair) {
            // If we've already had an error don't bother trying to learn this
            // address.
            if (overall_result != ERR_OK) { return; }

            // Is this entry unresolved?
            // Don't repeat learning for addresses we've already resolved.
            if (addr_pair.second.next_hop_mac) { return; }

            auto target = make_stack_address(addr_pair.first);

            ip6_addr_assign_zone(&target, IP6_UNICAST, slp.intf);

            OP_LOG(OP_LOG_TRACE,
                   "Sending ND request for IP: %s\n",
                   to_string(addr_pair.first).c_str());

            auto result = nd6_get_next_hop_entry(&target, slp.intf);

            switch (result) {
            case ERR_RTE:
                // no route found. not an error but don't populate anything
                // further.
                addr_pair.second.neighbor_cache_offset = result;
                return;
            case ERR_MEM:
                // could not allocate memory neighbor or destination cache.
                // definite error.
                overall_result = result;
                return;
            default:
                // keep going; no error
                [[fallthrough]];
            }

            // Grab the next hop address for result and sanity checking
            // purposes.
            auto neighbor_entry = get_nd_cache_entry(result);
            if (!neighbor_entry) {
                OP_LOG(OP_LOG_ERROR,
                       "Could not find neighbor cache entry at index %d that "
                       "we just created!",
                       result);

                return;
            }

            // Keep track of where in the neighbor/destination cache
            // this entry is at.
            addr_pair.second.neighbor_cache_offset = result;

            addr_pair.second.next_hop_address =
                std::get<libpacket::type::ipv6_address>(*neighbor_entry);
        });

    return (overall_result);
}

static void send_learning_requests(void* arg)
{
    auto* slp = reinterpret_cast<start_learning_params*>(arg);
    assert(slp);

    auto ipv4_result = send_ipv4_learning_requests(*slp);
    if (ipv4_result != ERR_OK) {
        slp->barrier.set_value(ipv4_result);
        return;
    }

    auto ipv6_result = send_ipv6_learning_requests(*slp);
    if (ipv6_result != ERR_OK) {
        slp->barrier.set_value(ipv6_result);
        return;
    }

    // If we've gotten this far nothing went wrong. Return ERR_OK.
    slp->barrier.set_value(ERR_OK);
}

// Return true if learning started, false otherwise.
bool learning_state_machine::start_learning(
    const std::unordered_set<libpacket::type::ipv4_address>& to_learn_ipv4,
    const std::unordered_set<libpacket::type::ipv6_address>& to_learn_ipv6,
    resolve_complete_callback callback)
{
    // If we're already learning, don't start again.
    if (in_progress()) { return (false); }

    // Are we being asked to learn nothing?
    if (to_learn_ipv4.empty() && to_learn_ipv6.empty()) { return (false); }

    m_results.ipv4.clear();
    m_results.ipv6.clear();

    // Populate results with IPv4 addresses.
    std::transform(to_learn_ipv4.begin(),
                   to_learn_ipv4.end(),
                   std::inserter(m_results.ipv4, m_results.ipv4.end()),
                   [](const auto& ip_addr) {
                       return (std::make_pair(ip_addr, std::nullopt));
                   });

    // Populate results with IPv6 addresses.
    std::transform(to_learn_ipv6.begin(),
                   to_learn_ipv6.end(),
                   std::inserter(m_results.ipv6, m_results.ipv6.end()),
                   [](const auto& ip_addr) {
                       return (std::make_pair(
                           ip_addr,
                           ipv6_nd_result{.neighbor_cache_offset =
                                              ND_NEIGHBOR_CACHE_NO_ENTRY}));
                   });

    m_resolved_callback = std::move(callback);

    return (start_learning_impl());
}

bool learning_state_machine::retry_learning()
{
    // If we're already learning, don't start again.
    if (in_progress()) { return (false); }

    // Are we being asked to retry when no items failed?
    if (all_addresses_resolved(m_results)) { return (true); }

    return (start_learning_impl());
}

bool learning_state_machine::start_learning_impl()
{
    // Do we have a valid interface to learn on?
    auto* intf =
        const_cast<netif*>(std::any_cast<const netif*>(m_interface.data()));

    if (intf == nullptr) { return (false); }

    m_current_state = state_start{};

    start_learning_params slp = {.intf = intf, .to_learn = this->m_results};
    auto barrier = slp.barrier.get_future();

    // tcpip_callback executes the given function in the stack thread passing it
    // the second argument as void*.
    // send_learning_requests is smart enough to only send requests for results
    // in the unresolved state.
    if (auto res = tcpip_callback(send_learning_requests, &slp);
        res != ERR_OK) {
        m_current_state = state_timeout{};
        return (true);
    }

    // Wait for the all the learning requests to send.
    // We could just return. But it's useful to know if the process succeeded.
    // Plus, the process is non-blocking on the stack side so this won't take
    // long.
    if (barrier.wait_for(start_callback_timeout) != std::future_status::ready) {
        OP_LOG(OP_LOG_ERROR, "Timed out while starting learning.");
        m_current_state = state_timeout{};
        return (false);
    }

    auto learning_status = barrier.get();
    if (learning_status != ERR_OK) { m_current_state = state_timeout{}; }

    m_current_state = state_learning{};

    return (start_status_polling());
}

static int handle_poll_timeout(const struct op_event_data* data, void* arg)
{
    auto* lsm = reinterpret_cast<learning_state_machine*>(arg);
    assert(lsm);

    if (data->timeout_id != lsm->timeout_id()) {
        throw std::runtime_error(
            "Error with event loop while performing MAC learning.");
    }

    return (lsm->check_learning());
}

bool learning_state_machine::start_status_polling()
{
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        poll_check_interval);

    auto callbacks = op_event_callbacks{.on_timeout = handle_poll_timeout};
    auto timeout = duration_ns.count();
    uint32_t timeout_id = 0;
    auto result = m_loop.get().add(timeout, &callbacks, this, &timeout_id);

    if (result < 0) { return (false); }

    m_loop_timeout_id = timeout_id;

    m_polls_remaining = max_poll_count;

    return (true);
}

// Structure that gets "passed" to the stack thread via tcpip_callback.
struct check_learning_params
{
    learning_results& results;
    std::promise<void> barrier;
};

static void check_arp_cache(check_learning_params& clp)
{
    ip4_addr_t* entryAddrPtr = nullptr;
    eth_addr* entryMacPtr = nullptr;
    netif* entryIntfPtr = nullptr;

    for (size_t i = 0; i < ARP_TABLE_SIZE; i++) {
        // LwIP refers to resolved MACs as "stable entries" in the ARP cache.
        auto stable_entry =
            etharp_get_entry(i, &entryAddrPtr, &entryIntfPtr, &entryMacPtr);

        if (!stable_entry) { continue; }

        auto found_result = clp.results.ipv4.find(
            libpacket::type::ipv4_address(ntohl(entryAddrPtr->addr)));
        if (found_result == clp.results.ipv4.end()) {
            // Guess we weren't looking for this address.
            // Remember, the stack is shared by all generators.
            continue;
        }

        if (found_result->second.has_value()) {
            // We already know this address.
            continue;
        }

        found_result->second.emplace<libpacket::type::mac_address>(
            reinterpret_cast<const uint8_t*>(entryMacPtr->addr));
    }
}

// XXX: In an ideal world this would be constexpr.
// But, the underlying std::array type doesn't zero out POD values.
// So there's no guarantee that the MAC would end up as "00:00:00:00:00:00" for
// all platforms/compilers.
static const libpacket::type::mac_address unspecified_mac =
    libpacket::type::mac_address("00:00:00:00:00:00");

static void check_nd_cache(check_learning_params& slp)
{
    std::for_each(
        slp.results.ipv6.begin(),
        slp.results.ipv6.end(),
        [](auto& ipv6_result) {
            // Is this entry already resolved?
            if (ipv6_result.second.next_hop_mac.has_value()) { return; }

            switch (ipv6_result.second.neighbor_cache_offset) {
            case ERR_RTE:
                // Stack indicated no route to destination.
                return;
            case ND_NEIGHBOR_CACHE_NO_ENTRY:
                // Checking a result that was not accepted by the stack. Let's
                // maybe not do that. This could happen at scale when the stack
                // runs out of cache entries at request time.
                return;
            default:
                [[fallthrough]];
            };

            auto neighbor_entry =
                get_nd_cache_entry(ipv6_result.second.neighbor_cache_offset);
            if (neighbor_entry) {
                auto next_hop_address =
                    std::get<libpacket::type::ipv6_address>(*neighbor_entry);

                // Sanity check: is the MAC associated with the expected next
                // hop address from when we started learning?
                if (ipv6_result.second.next_hop_address.has_value()
                    && (ipv6_result.second.next_hop_address.value()
                        == next_hop_address)) {

                    auto mac_address =
                        std::get<libpacket::type::mac_address>(*neighbor_entry);

                    // Entry resolved, update the results.
                    if (mac_address != unspecified_mac) {
                        ipv6_result.second.next_hop_mac = mac_address;
                    }
                }
            }
        });
}

static void check_learning_caches(void* arg)
{
    auto* clp = reinterpret_cast<check_learning_params*>(arg);
    assert(clp);

    check_arp_cache(*clp);
    check_nd_cache(*clp);

    clp->barrier.set_value();
}

int learning_state_machine::check_learning()
{
    // Are there results to check?
    if (m_results.ipv4.empty() && m_results.ipv6.empty()) { return (-1); }

    // Are we in the process of learning?
    if (!in_progress()) { return (-1); }

    if ((m_polls_remaining--) <= 0) {
        m_current_state = state_timeout{};
        OP_LOG(OP_LOG_WARNING,
               "Not all addresses resolved during ARP/ND process.");
        return (-1);
    }

    check_learning_params clp = {.results = this->m_results};
    auto barrier = clp.barrier.get_future();

    // tcpip_callback executes the given function in the stack thread passing it
    // the second argument as void*.
    if (auto res = tcpip_callback(check_learning_caches, &clp); res != ERR_OK) {
        m_current_state = state_timeout{};
        OP_LOG(OP_LOG_ERROR,
               "Got error %s while checking stack ARP/ND caches.",
               strerror(res));
        return (-1);
    }

    // Wait for the check to finish.
    // We could just return. But it's useful to know if the process succeeded.
    // Plus, the process is non-blocking on the stack side so this won't take
    // long (essentially iterating over an in-memory array).
    if (barrier.wait_for(check_callback_timeout) != std::future_status::ready) {
        OP_LOG(OP_LOG_ERROR, "Timed out while checking learning status.");
        m_current_state = state_timeout{};
        return (-1);
    }

    if (all_addresses_resolved(m_results)) {
        m_current_state = state_done{};
        OP_LOG(OP_LOG_TRACE,
               "Successfully resolved all %lu ARP and %lu ND requests.",
               m_results.ipv4.size(),
               m_results.ipv6.size());

        // Deliver results via callback.
        m_resolved_callback(*this);
        return (-1);
    }

    return (0);
}

void learning_state_machine::stop_learning()
{
    // If we're not in the progress of learning, we're already stopped.
    if (!in_progress()) { return; }

    // Delete the polling timer.
    auto result = m_loop.get().del(m_loop_timeout_id);
    if (result != 0) {
        throw std::runtime_error("Error while unregistering from event loop in "
                                 "packet generator module.");
    }

    // Figure out which state we're going to end in.
    if (all_addresses_resolved(m_results)) {
        m_current_state = state_done{};
        return;
    }

    m_current_state = state_timeout{};
}

std::string to_string(const learning_resolved_state& state)
{
    switch (state) {
    case learning_resolved_state::unsupported:
        return (std::string("unsupported"));
        break;
    case learning_resolved_state::unresolved:
        return (std::string("unresolved"));
        break;
    case learning_resolved_state::resolving:
        return (std::string("resolving"));
        break;
    case learning_resolved_state::resolved:
        return (std::string("resolved"));
        break;
    case learning_resolved_state::timed_out:
        return (std::string("timed_out"));
        break;
    default:
        throw std::invalid_argument("unknown state value");
    }
}

learning_resolved_state learning_state_machine::state() const
{
    return (std::visit(utils::overloaded_visitor(
                           [](const std::monostate&) {
                               return (learning_resolved_state::unresolved);
                           },
                           [](const state_start&) {
                               return (learning_resolved_state::resolving);
                           },
                           [](const state_learning&) {
                               return (learning_resolved_state::resolving);
                           },
                           [](const state_done&) {
                               return (learning_resolved_state::resolved);
                           },
                           [](const state_timeout&) {
                               return (learning_resolved_state::timed_out);
                           }),
                       m_current_state));
}

} // namespace openperf::packet::generator