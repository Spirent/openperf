#ifndef _NETBSD_BPF_H_
#define _NETBSD_BPF_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Compatbility definitions */
#ifndef BPF_COP
#define BPF_COP  0x20
#define BPF_COPX 0x40
#endif

#ifndef BPF_MOD
#define BPF_MOD 0x90
#endif

#ifndef BPF_XOR
#define BPF_XOR 0xa0
#endif

/*
 * Number of scratch memory words (for BPF_LD|BPF_MEM and BPF_ST).
 */
#define BPF_MEMWORDS            16

/*
 * bpf_memword_init_t: bits indicate which words in the external memory
 * store will be initialised by the caller before BPF program execution.
 */
typedef uint32_t bpf_memword_init_t;
#define BPF_MEMWORD_INIT(k)     (UINT32_C(1) << (k))

#define BPF_MAX_MEMWORDS        30


struct bpf_ctx;
typedef struct bpf_ctx bpf_ctx_t;

typedef struct bpf_args {
        const uint8_t * pkt;
        size_t          wirelen;
        size_t          buflen;
        /*
         * The following arguments are used only by some kernel
         * subsystems.
         * They aren't required for classical bpf filter programs.
         * For such programs, bpfjit generated code doesn't read
         * those arguments at all. Note however that bpf interpreter
         * always needs a pointer to memstore.
         */
        uint32_t *      mem; /* pointer to external memory store */
        void *          arg; /* auxiliary argument for a copfunc */
} bpf_args_t;

#if defined(_KERNEL) || defined(__BPF_PRIVATE)

typedef uint32_t (*bpf_copfunc_t)(const bpf_ctx_t *, bpf_args_t *, uint32_t);

struct bpf_ctx {
        /*
         * BPF coprocessor functions and the number of them.
         */
        const bpf_copfunc_t *   copfuncs;
        size_t                  nfuncs;

        /*
         * The number of memory words in the external memory store.
         * There may be up to BPF_MAX_MEMWORDS words; if zero is set,
         * then the internal memory store is used which has a fixed
         * number of words (BPF_MEMWORDS).
         */
        size_t                  extwords;

        /*
         * The bitmask indicating which words in the external memstore
         * will be initialised by the caller.
         */
        bpf_memword_init_t      preinited;
};
#endif

struct bpf_insn;

int     op_bpf_validate(const struct bpf_insn * insn, int len);

u_int   op_bpf_filter(const struct bpf_insn * insn, const u_char * data, u_int wirelen, u_int buflen);

typedef unsigned int (*bpfjit_func_t)(const bpf_ctx_t*, bpf_args_t*);

void op_bpfjit_set_verbose(int verbose);

bpfjit_func_t
op_bpfjit_generate_code(const bpf_ctx_t*, const struct bpf_insn*, size_t);

void op_bpfjit_free_code(bpfjit_func_t);

u_int static inline op_bpfjit_filter(bpfjit_func_t func, const u_char *data, u_int wirelen, u_int buflen) {
	bpf_args_t args = { .pkt = data, .wirelen = wirelen, .buflen = buflen, .mem = 0, .arg = 0 };
	return func(0, &args);
}

#ifdef __cplusplus
}
#endif

#endif /* _NETBSD_BPF_H_ */
