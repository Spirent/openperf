#include <cstdint>

#include <cpuid.h>

#include "instruction_set.hpp"

namespace openperf::cpu {

/* This bit data comes from https://en.wikipedia.org/wiki/CPUID */

struct cpuid_feature_bits
{
    static constexpr uint32_t EAX_FEATURE_BITS = 1;

    union
    {
        struct
        {
            uint32_t step : 4;
            uint32_t model : 4;
            uint32_t family : 4;
            uint32_t instruction_set : 2;
            uint32_t pad1 : 2;
            uint32_t emodel : 4;
            uint32_t efamily : 8;
            uint32_t pad2 : 4;
        };
        uint32_t data;
    } eax;
    struct
    {
        uint32_t data;
    } ebx;
    union
    {
        struct
        {
            uint32_t sse3 : 1;
            uint32_t pclmulqdq : 1;
            uint32_t dtes64 : 1;
            uint32_t monitor : 1;
            uint32_t dscpl : 1;
            uint32_t vmx : 1;
            uint32_t smx : 1;
            uint32_t est : 1;
            uint32_t tm2 : 1;
            uint32_t ssse3 : 1;
            uint32_t cntxid : 1;
            uint32_t sdbg : 1;
            uint32_t fma : 1;
            uint32_t cx16 : 1;
            uint32_t xtpr : 1;
            uint32_t pdcm : 1;
            uint32_t reserved1 : 1;
            uint32_t pcid : 1;
            uint32_t dca : 1;
            uint32_t sse41 : 1;
            uint32_t sse42 : 1;
            uint32_t x2apic : 1;
            uint32_t movbe : 1;
            uint32_t popcnt : 1;
            uint32_t tscdeadline : 1;
            uint32_t aes : 1;
            uint32_t xsave : 1;
            uint32_t osxsave : 1;
            uint32_t avx : 1;
            uint32_t f16c : 1;
            uint32_t rdrnd : 1;
            uint32_t hypervisor : 1;
        };
        uint32_t data;
    } ecx;
    union
    {
        struct
        {
            uint32_t fpu : 1;
            uint32_t vme : 1;
            uint32_t de : 1;
            uint32_t pse : 1;
            uint32_t tsc : 1;
            uint32_t msr : 1;
            uint32_t pae : 1;
            uint32_t mce : 1;
            uint32_t cx8 : 1;
            uint32_t apic : 1;
            uint32_t reserved1 : 1;
            uint32_t sep : 1;
            uint32_t mtrr : 1;
            uint32_t pge : 1;
            uint32_t mca : 1;
            uint32_t cmov : 1;
            uint32_t pat : 1;
            uint32_t pse36 : 1;
            uint32_t psn : 1;
            uint32_t clfsh : 1;
            uint32_t reserved2 : 1;
            uint32_t ds : 1;
            uint32_t acpi : 1;
            uint32_t mmx : 1;
            uint32_t fxsr : 1;
            uint32_t sse : 1;
            uint32_t sse2 : 1;
            uint32_t ss : 1;
            uint32_t htt : 1;
            uint32_t tm : 1;
            uint32_t ia64 : 1;
            uint32_t pbe : 1;
        };
        uint32_t data;
    } edx;

    cpuid_feature_bits()
    {
        __cpuid(EAX_FEATURE_BITS, eax.data, ebx.data, ecx.data, edx.data);
    }
};

struct cpuid_extended_bits
{
    static constexpr uint32_t EAX_EXTENDED_BITS = 7;

    struct
    {
        uint32_t data;
    } eax;
    union
    {
        struct
        {
            uint32_t fsgsbase : 1;
            uint32_t ia32tscadjust : 1;
            uint32_t sgx : 1;
            uint32_t bmi1 : 1;
            uint32_t hle : 1;
            uint32_t avx2 : 1;
            uint32_t reserved1 : 1;
            uint32_t smep : 1;
            uint32_t bmi2 : 1;
            uint32_t erms : 1;
            uint32_t invpcid : 1;
            uint32_t rtm : 1;
            uint32_t pqm : 1;
            uint32_t fpucsdsdeprecated : 1;
            uint32_t mpx : 1;
            uint32_t pqe : 1;
            uint32_t avx512f : 1;
            uint32_t avx512dq : 1;
            uint32_t rdseed : 1;
            uint32_t adx : 1;
            uint32_t smap : 1;
            uint32_t avx512ifma : 1;
            uint32_t pcommit : 1;
            uint32_t clflushopt : 1;
            uint32_t clwb : 1;
            uint32_t intelproctrace : 1;
            uint32_t avx512pf : 1;
            uint32_t avx512er : 1;
            uint32_t avx512cd : 1;
            uint32_t sha : 1;
            uint32_t avx512bw : 1;
            uint32_t avx512vl : 1;
        };
        uint32_t data;
    } ebx;
    union
    {
        struct
        {
            uint32_t prefetchwt1 : 1;
            uint32_t avx512vbmi : 1;
            uint32_t umip : 1;
            uint32_t pku : 1;
            uint32_t ospke : 1;
            uint32_t reserved1 : 1;
            uint32_t avx512vbmi2 : 1;
            uint32_t reserved2 : 1;
            uint32_t gfni : 1;
            uint32_t vaes : 1;
            uint32_t vpclmulqdq : 1;
            uint32_t avx512vnni : 1;
            uint32_t avx512bitalg : 1;
            uint32_t reserved3 : 1;
            uint32_t avx512vpopcntdq : 1;
            uint32_t reserved4 : 2;
            uint32_t mawau : 5;
            uint32_t rdpid : 1;
            uint32_t reserved5 : 7;
            uint32_t sgxlc : 1;
            uint32_t reserved6 : 1;
        };
        uint32_t data;
    } ecx;
    struct
    {
        uint32_t data;
    } edx;

    cpuid_extended_bits()
    {
        __cpuid_count(
            EAX_EXTENDED_BITS, 0, eax.data, ebx.data, ecx.data, edx.data);
    }
};

static uint64_t read_xcr(uint32_t idx)
{
    uint32_t eax, edx;
    __asm__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(idx));
    return (static_cast<uint64_t>(edx) << 32 | eax);
}

struct xcr0_bits
{
    union
    {
        struct
        {
            uint64_t x87 : 1;
            uint64_t sse : 1;
            uint64_t avx : 1;
            uint64_t bndreg : 1;
            uint64_t bndcsr : 1;
            uint64_t opmask : 1;
            uint64_t zmm_hi256 : 1;
            uint64_t hi16_zmm : 1;
            uint64_t reserved1 : 1;
            uint64_t pkru : 1;
        };
        uint64_t data;
    };

    xcr0_bits() { data = read_xcr(0); }
};

bool available(instruction_set t)
{
    auto features = cpuid_feature_bits();
    auto extended = cpuid_extended_bits();
    auto xcr0 = xcr0_bits();

    /*
     * In addition to verifying that the CPU has the necessary instructions,
     * we also need to check to make sure the OS has taken the proper steps
     * to enable saving/restoring the associated registers during context
     * switches.  Hence, we also check for the osxsave bit and various xcr0
     * bits.
     *
     * The explicit feature/extended flags checked here are the same flags
     * checked by ispc to verify a CPU can run the associated target code.
     * See ispc/builtins/dispatch.ll for details.
     */
    switch (t) {
    case instruction_set::SCALAR:
        /* No checks are needed for scalar or auto code */
        return (true);
    case instruction_set::SSE2:
        return (features.ecx.osxsave && xcr0.sse && features.edx.sse2);
    case instruction_set::SSE4:
        return (features.ecx.osxsave && xcr0.sse && features.ecx.sse41
                && features.ecx.sse42);
    case instruction_set::AVX:
        return (features.ecx.osxsave && xcr0.sse && xcr0.avx
                && features.ecx.avx);
    case instruction_set::AVX2:
        return (features.ecx.osxsave && xcr0.sse && xcr0.avx && features.ecx.avx
                && features.ecx.f16c && features.ecx.rdrnd
                && extended.ebx.avx2);
    case instruction_set::AVX512:
        return (features.ecx.osxsave && xcr0.sse && xcr0.avx && xcr0.opmask
                && xcr0.zmm_hi256 && xcr0.hi16_zmm && extended.ebx.avx2
                && extended.ebx.avx512f && extended.ebx.avx512dq
                && extended.ebx.avx512cd && extended.ebx.avx512bw
                && extended.ebx.avx512vl);
    default:
        return (false);
    }
}

} // namespace openperf::cpu
