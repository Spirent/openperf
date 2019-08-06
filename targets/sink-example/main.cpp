#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include <chrono>
#include <thread>

#include "zmq.h"

#include "core/icp_core.h"
#include "packetio/internal_client.h"
#include "packetio/packet_buffer.h"
#include "packet_counter.h"

static bool _icp_done = false;
void signal_handler(int signo __attribute__((unused)))
{
    _icp_done = true;
}

template <typename T>
static void log_counter(packet_counter<T>& counter, double delta)
{
    if (!counter.pkts_.load(std::memory_order_relaxed)) return;

    auto pkts = counter.pkts_.load(std::memory_order_relaxed);
    auto pps = delta ? pkts / delta : 0;
    auto octets = counter.octets_total_.load(std::memory_order_relaxed);
    auto Mbps = delta ? octets / delta / 131072 : 0;

    ICP_LOG(ICP_LOG_INFO, "%s counters: packets %zu (%.02f pps), length %zu/%zu/%zu (%.02f Mbps)\n",
            counter.name_.c_str(),
            pkts, pps,
            counter.octets_min_.load(std::memory_order_relaxed),
            octets / pkts,
            counter.octets_max_.load(std::memory_order_relaxed),
            Mbps);

    counter.reset();
}

class test_sink {
    icp::core::uuid m_id;
    std::unique_ptr<packet_counter<uint64_t>> m_counter;
    std::thread m_logger;

public:
    test_sink(uint16_t port_id)
        : m_id(icp::core::uuid::random())
        , m_counter(std::make_unique<packet_counter<uint64_t>>("Port " + std::to_string(port_id)))
    {
        m_logger = std::thread([](packet_counter<uint64_t>* counter){
                                   using namespace std::chrono_literals;
                                   using namespace std::chrono;
                                   while (!_icp_done) {
                                       auto start = high_resolution_clock::now();
                                       std::this_thread::sleep_for(1s);
                                       auto period = duration_cast<duration<double>>(
                                           high_resolution_clock::now() - start);
                                       log_counter(*counter, period.count());
                                   }
                               }, m_counter.get());
    }

    test_sink(test_sink&& other)
        : m_id(other.m_id)
        , m_counter(std::move(other.m_counter))
        , m_logger(std::move(other.m_logger))
    {}

    test_sink& operator=(test_sink&& other)
    {
        if (this != &other) {
            m_id = other.m_id;
            m_counter = std::move(other.m_counter);
            m_logger = std::move(other.m_logger);
        }
        return (*this);
    }

    ~test_sink()
    {
        if (m_counter) m_logger.join();
    }

    std::string id() const
    {
        return (icp::core::to_string(m_id));
    }

    uint16_t push(icp::packetio::packets::packet_buffer* const packets[], uint16_t length) const
    {
        for(uint16_t i = 0; i < length; i++) {
            m_counter->add(icp::packetio::packets::length(packets[i]));
        }
        return (length);
    }
};

void test_sinks(void *context)
{
    auto client = icp::packetio::internal::api::client(context);

    auto sink0 = icp::packetio::packets::generic_sink(test_sink(0));
    auto success = client.add_sink("port0", sink0);
    if (!success) {
        throw std::runtime_error("Could not add sink to port0");
    }
}

int main(int argc, char* argv[])
{
    icp_thread_setname("icp_main");

    /* Block child threads from intercepting SIGINT or SIGTERM */
    sigset_t newset, oldset;
    sigemptyset(&newset);
    sigaddset(&newset, SIGINT);
    sigaddset(&newset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &newset, &oldset);

    void *context = zmq_ctx_new();
    if (!context) {
        icp_exit("Could not initialize ZeroMQ context!");
    }

    icp_init(context, argc, argv);

    /* Install our signal handler so we can property shut ourselves down */
    struct sigaction s;
    s.sa_handler = signal_handler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = 0;
    sigaction(SIGINT, &s, NULL);
    sigaction(SIGTERM, &s, NULL);

    /* Restore the default signal mask */
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);

    test_sinks(context);

    /* Block until we're done ... */
    sigset_t emptyset;
    sigemptyset(&emptyset);
    while (!_icp_done) {
        sigsuspend(&emptyset);
    }

    /* ... then clean up and exit. */
    icp_halt(context);

    exit(EXIT_SUCCESS);
}
