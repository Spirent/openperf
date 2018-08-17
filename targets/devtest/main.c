#include <stdlib.h>

#include "rte_eal.h"
#include "rte_per_lcore.h"
#include "rte_lcore.h"

#include "zmq.h"

static int lcore_hello(void *arg __attribute__((unused)))
{
    unsigned lcore_id = rte_lcore_id();
    printf("Hello from core %u\n", lcore_id);
    return (0);
}

int main(int argc, char* argv[])
{
    void *ctx = zmq_ctx_new();

    int ret = rte_eal_init(argc, argv);
    if (ret >= 0) {
        unsigned lcore_id = -1;
        RTE_LCORE_FOREACH_SLAVE(lcore_id) {
            rte_eal_remote_launch(lcore_hello, NULL, lcore_id);
        }

        lcore_hello(NULL);

        rte_eal_mp_wait_lcore();

        zmq_ctx_destroy(ctx);
        return (EXIT_SUCCESS);
    }

    zmq_ctx_destroy(ctx);
    return (EXIT_FAILURE);
}
