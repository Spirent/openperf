#ifndef _OP_SOCKET_SERVER_COMPAT_LINUX_TCP_H_
#define _OP_SOCKET_SERVER_COMPAT_LINUX_TCP_H_

#include <stdint.h>

enum linux_tcp_state {
    LINUX_TCP_ESTABLISHED = 1,
    LINUX_TCP_SYN_SENT,
    LINUX_TCP_SYN_RECV,
    LINUX_TCP_FIN_WAIT1,
    LINUX_TCP_FIN_WAIT2,
    LINUX_TCP_TIME_WAIT,
    LINUX_TCP_CLOSE,
    LINUX_TCP_CLOSE_WAIT,
    LINUX_TCP_LAST_ACK,
    LINUX_TCP_LISTEN,
    LINUX_TCP_CLOSING /* now a valid state */
};

/* Linux compatibility-isms */
#define LINUX_TCP_NODELAY 1
#define LINUX_TCP_MAXSEG 2
#define LINUX_TCP_DEFER_ACCEPT 9
#define LINUX_TCP_INFO 11
#define LINUX_TCP_CONGESTION 13

struct tcp_info
{
    uint8_t tcpi_state;
    uint8_t tcpi_ca_state;
    uint8_t tcpi_retransmits;
    uint8_t tcpi_probes;
    uint8_t tcpi_backoff;
    uint8_t tcpi_options;
    uint8_t tcpi_snd_wscale : 4, tcpi_rcv_wscale : 4;
    uint8_t tcpi_delivery_rate_app_limited : 1;

    uint32_t tcpi_rto;
    uint32_t tcpi_ato;
    uint32_t tcpi_snd_mss;
    uint32_t tcpi_rcv_mss;

    uint32_t tcpi_unacked;
    uint32_t tcpi_sacked;
    uint32_t tcpi_lost;
    uint32_t tcpi_retrans;
    uint32_t tcpi_fackets;

    /* Times. */
    uint32_t tcpi_last_data_sent;
    uint32_t tcpi_last_ack_sent; /* Not remembered, sorry. */
    uint32_t tcpi_last_data_recv;
    uint32_t tcpi_last_ack_recv;

    /* Metrics. */
    uint32_t tcpi_pmtu;
    uint32_t tcpi_rcv_ssthresh;
    uint32_t tcpi_rtt;
    uint32_t tcpi_rttvar;
    uint32_t tcpi_snd_ssthresh;
    uint32_t tcpi_snd_cwnd;
    uint32_t tcpi_advmss;
    uint32_t tcpi_reordering;

    uint32_t tcpi_rcv_rtt;
    uint32_t tcpi_rcv_space;

    uint32_t tcpi_total_retrans;

    uint64_t tcpi_pacing_rate;
    uint64_t tcpi_max_pacing_rate;
    uint64_t tcpi_bytes_acked;    /* RFC4898 tcpEStatsAppHCThruOctetsAcked */
    uint64_t tcpi_bytes_received; /* RFC4898 tcpEStatsAppHCThruOctetsReceived */
    uint32_t tcpi_segs_out;       /* RFC4898 tcpEStatsPerfSegsOut */
    uint32_t tcpi_segs_in;        /* RFC4898 tcpEStatsPerfSegsIn */

    uint32_t tcpi_notsent_bytes;
    uint32_t tcpi_min_rtt;
    uint32_t tcpi_data_segs_in;  /* RFC4898 tcpEStatsDataSegsIn */
    uint32_t tcpi_data_segs_out; /* RFC4898 tcpEStatsDataSegsOut */

    uint64_t tcpi_delivery_rate;

    uint64_t tcpi_busy_time;      /* Time (usec) busy sending data */
    uint64_t tcpi_rwnd_limited;   /* Time (usec) limited by receive window */
    uint64_t tcpi_sndbuf_limited; /* Time (usec) limited by send buffer */
};

#endif /* _OP_SOCKET_SERVER_COMPAT_LINUX_TCP_H_ */
