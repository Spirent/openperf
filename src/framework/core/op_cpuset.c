#include "op_common.h"
#include "op_cpuset.h"
#include "op_log.h"

#include <errno.h>
#include <inttypes.h>
#include <string.h>

/*
 * Symbols for inline functions.
 */
extern void
op_cpuset_set_range(op_cpuset_t cpuset, size_t start, size_t len, bool val);
extern op_cpuset_t op_cpuset_create_from_string(const char* str);
extern int op_cpuset_get_first(op_cpuset_t cpuset, size_t* cpu);
extern int op_cpuset_get_next(op_cpuset_t cpuset, size_t* cpu);

op_cpuset_t op_cpuset_all(void)
{
    op_cpuset_t cpuset = op_cpuset_create();
    for (uint16_t core = 0; core < op_get_cpu_count(); core++) {
        op_cpuset_set(cpuset, core, true);
    }

    return cpuset;
}

uint64_t op_cpuset_get_uint64(op_cpuset_t cpuset, size_t off, size_t len)
{
    uint64_t val = 0;
    int i;

    assert(len <= 64);
    for (i = len - 1; i >= 0; --i) {
        val = (val << 1) | (op_cpuset_get(cpuset, off + i) ? 1 : 0);
    }
    return val;
}

void op_cpuset_set_uint64(op_cpuset_t cpuset,
                          size_t off,
                          size_t len,
                          uint64_t val)
{
    uint64_t mask = 0x1;
    size_t i, n = off + len;

    assert(len <= 64);
    for (i = off; i < n; ++i) {
        op_cpuset_set(cpuset, i, (val & mask) == mask);
        mask <<= 1;
    }
}

int op_cpuset_from_string(op_cpuset_t cpuset, const char* str)
{
    uint64_t val;
    size_t word_size = 64;
    size_t bits_per_char = 4; // Each character is 4 bits

    op_cpuset_clear(cpuset);

    if (strncmp(str, "0x", 2) == 0) {
        // Hex string
        char tmpstr[64];
        const char* p = str + 2;
        size_t nchars = strlen(p);
        size_t bit_off = 0;
        while (nchars) {
            size_t xchars = nchars;
            size_t xbits = xchars * bits_per_char;
            if (xbits > word_size) {
                xbits = word_size;
                xchars = word_size / bits_per_char;
            }
            // Extract trailing characters to temp buffer
            strncpy(tmpstr, p + (nchars - xchars), xchars);
            tmpstr[xchars] = 0;
            val = strtoull(tmpstr, 0, 16);
            if (val == 0) {
                if (errno == EINVAL) return -1;
            }
            op_cpuset_set_uint64(cpuset, bit_off, xbits, val);
            nchars -= xchars;
            bit_off += xbits;
        }
    } else {
        // Parse it as an integer
        val = strtoull(str, 0, 0);
        if (val == 0) {
            if (errno == EINVAL) return -1;
        }
        op_cpuset_set_uint64(cpuset, 0, 64, val);
    }

    return 0;
}

void op_cpuset_to_string(op_cpuset_t cpuset, char* buf, size_t buflen)
{
    uint64_t val = 0;
    size_t max_cpus;
    size_t word_size = 64;
    size_t words, remainder;
    int off = 0, tlen;
    bool nonzero = false;
    bool zeropad = false;

    max_cpus = op_cpuset_size(cpuset);
    words = max_cpus / word_size;
    remainder = max_cpus % word_size;

    tlen = snprintf(buf + off, buflen - off, "0x");
    if (tlen < 0) {
        OP_LOG(OP_LOG_ERROR, "%s %s\n", __FUNCTION__, strerror(errno));
        return;
    }
    off += tlen;

    if (remainder) {
        val = op_cpuset_get_uint64(cpuset, words * word_size, remainder);
        if (val) nonzero = true;
        if (nonzero) {
            tlen = snprintf(buf + off, buflen - off, "%" PRIx64, val);
            if (tlen < 0) {
                OP_LOG(OP_LOG_ERROR, "%s %s\n", __FUNCTION__, strerror(errno));
                return;
            }
            off += tlen;
            zeropad = true;
        }
    }
    for (int i = words - 1; i >= 0; --i) {
        val = op_cpuset_get_uint64(cpuset, i * word_size, word_size);
        if (val) nonzero = true;
        if (nonzero) {
            if (zeropad) {
                tlen = snprintf(buf + off, buflen - off, "%016" PRIx64, val);
            } else {
                tlen = snprintf(buf + off, buflen - off, "%" PRIx64, val);
            }
            if (tlen < 0) {
                OP_LOG(OP_LOG_ERROR, "%s %s\n", __FUNCTION__, strerror(errno));
                return;
            }
            off += tlen;
            zeropad = true;
        }
    }
    if (!nonzero) { snprintf(buf + off, buflen - off, "0"); }
}
