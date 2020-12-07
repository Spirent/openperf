#include "packet/generator/learning.hpp"

#include "lwip/priv/tcpip_priv.h"
#include "lwip/etharp.h"
#include "arpa/inet.h"

#include <future>

namespace openperf::packet::generator {

static constexpr std::chrono::seconds start_callback_timeout(1);
static constexpr std::chrono::seconds check_callback_timeout(1);

static constexpr std::chrono::seconds poll_check_interval(1);
static constexpr int max_poll_count = 30;

bool all_addresses_resolved(const learning_result_map& results)
{
    return (std::none_of(
        results.begin(), results.end(), [](const auto& address_pair) {
            return std::holds_alternative<unresolved>(address_pair.second);
        }));
}

// Structure that gets "passed" to the stack thread via tcpip_callback.
struct start_learning_params
{
    netif* intf = nullptr;               // lwip interface to use.
    const learning_result_map& to_learn; // addresses to learn.
    std::promise<err_t> barrier; // keep generator and stack threads in sync.
};

static void send_learning_requests(void* arg)
{
    auto* slp = reinterpret_cast<start_learning_params*>(arg);
    assert(slp);

    // start_learning_params contains a unique list of IP addresses to learn.
    // Addresses are assumed to be on-link. Caller must first apply routing
    // logic. Caller must not send IP addresses from a different subnet.

    err_t overall_result = ERR_OK;
    std::for_each(
        slp->to_learn.begin(), slp->to_learn.end(), [&](const auto& addr_pair) {
            // If we've already had an error don't bother trying to learn this
            // address.
            if (overall_result != ERR_OK) { return; }

            // Is this entry unresolved?
            // Don't repeat learning for addresses we've already resolved.
            if (!std::holds_alternative<unresolved>(addr_pair.second)) {
                return;
            }

            ip4_addr_t target{
                .addr = htonl(addr_pair.first.template load<uint32_t>())};

            OP_LOG(OP_LOG_TRACE,
                   "Sending ARP request for IP: %s\n",
                   to_string(addr_pair.first).c_str());

            auto result = etharp_query(slp->intf, &target, nullptr);
            if (result != ERR_OK) {
                OP_LOG(OP_LOG_ERROR,
                       "Error (%s) encountered while requesting ARP for "
                       "address: %s",
                       lwip_strerr(result),
                       to_string(addr_pair.first).c_str());
                overall_result = result;
            }
        });

    // Return ERR_OK if nothing went wrong, else first error we encountered.
    // Since we stop trying after the first error technically that would be the
    // only error.
    slp->barrier.set_value(overall_result);
}

// Return true if learning started, false otherwise.
bool learning_state_machine::start_learning(
    const std::vector<libpacket::type::ipv4_address>& to_learn,
    resolve_complete_callback callback)
{
    // If we're already learning, don't start again.
    if (in_progress()) { return (false); }

    // Are we being asked to learn nothing?
    if (to_learn.empty()) { return (false); }

    m_results.clear();

    // Populate results with IP addresses.
    std::transform(
        to_learn.begin(),
        to_learn.end(),
        std::inserter(m_results, m_results.end()),
        [](auto& ip_addr) { return (std::make_pair(ip_addr, unresolved{})); });

    m_resolved_callback = std::move(callback);

    return (start_learning_impl());
}

bool learning_state_machine::retry_failed()
{
    // Are we being asked to retry when no items failed?
    if (all_addresses_resolved(m_results)) { return (false); }

    return (start_learning_impl());
}

bool learning_state_machine::start_learning_impl()
{
    // If we're already learning, don't start again.
    if (in_progress()) { return (false); }

    // Are we being asked to learn nothing?
    if (m_results.empty()) { return (false); }

    // Do we have a valid interface to learn on?
    if (m_intf == nullptr) { return (false); }

    m_current_state = state_start{};

    start_learning_params slp = {.intf = this->m_intf,
                                 .to_learn = this->m_results};
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
        throw std::runtime_error("Error with event loop while performing MAC learning.");
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
    auto result = m_loop.add(timeout, &callbacks, this, &timeout_id);

    if (result < 0) { return (false); }

    m_loop_timeout_id = timeout_id;

    m_polls_remaining = max_poll_count;

    return (true);
}

// Structure that gets "passed" to the stack thread via tcpip_callback.
struct check_learning_params
{
    learning_result_map& results;
    std::promise<void> barrier;
};

static void check_arp_cache(void* arg)
{
    auto* clp = reinterpret_cast<check_learning_params*>(arg);
    assert(clp);

    ip4_addr_t* entryAddrPtr = nullptr;
    eth_addr* entryMacPtr = nullptr;
    netif* entryIntfPtr = nullptr;

    for (size_t i = 0; i < ARP_TABLE_SIZE; i++) {
        // LwIP refers to resolved MACs as "stable entries" in the ARP cache.
        auto stable_entry =
            etharp_get_entry(i, &entryAddrPtr, &entryIntfPtr, &entryMacPtr);

        if (!stable_entry) { continue; }

        auto found_result = clp->results.find(
            libpacket::type::ipv4_address(ntohl(entryAddrPtr->addr)));
        if (found_result == clp->results.end()) {
            // Guess we weren't looking for this address.
            // Remember, the stack is shared by all generators.
            continue;
        }

        if (std::holds_alternative<libpacket::type::mac_address>(
                found_result->second)) {
            // We already know this address.
            continue;
        }

        found_result->second.emplace<libpacket::type::mac_address>(
            reinterpret_cast<const uint8_t*>(entryMacPtr->addr));
    }

    clp->barrier.set_value();
}

int learning_state_machine::check_learning()
{
    // Are there results to check?
    if (m_results.empty()) { return (-1); }

    // Are we in the process of learning?
    if (!in_progress()) { return (-1); }

    if ((m_polls_remaining--) <= 0) {
        OP_LOG(OP_LOG_WARNING,
               "Not all addresses resolved during ARP process.");
        return (-1);
    }

    check_learning_params clp = {.results = this->m_results};
    auto barrier = clp.barrier.get_future();

    // tcpip_callback executes the given function in the stack thread passing it
    // the second argument as void*.
    if (auto res = tcpip_callback(check_arp_cache, &clp); res != ERR_OK) {
        m_current_state = state_timeout{};
        OP_LOG(OP_LOG_ERROR,
               "Got error %s while checking stack ARP cache.",
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
               "Successfully resolved all %lu ARP requests.",
               m_results.size());

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
    m_loop.del(m_loop_timeout_id);

    // Figure out which state we're going to end in.
    if (all_addresses_resolved(m_results)) {
        m_current_state = state_done{};
        return;
    }

    m_current_state = state_timeout{};
}

} // namespace openperf::packet::generator