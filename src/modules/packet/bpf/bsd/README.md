# BPF

## NetBSD 9.0 BPF code
The BPF code in this directory was ported from NetBSD 9.0

The C files only required a few modifications so these changes
are conditionally enabled with the OPENPERF_NETBSD_BPF define.

sys/net/bpf_filter.c
sys/net/bpfjit.c

The sys/net/bpf.h and sys/net/bpfjit.h required more changes so only
the required structures and defines from these files were added to bpf.h.

The bpf_validate() and bpf_filter() function names conflicted
with functions in both libpcap and DPDK so these functions
were renamed to op_bpf_validate() and op_bpf_filter().
For consistency the bpfjit_generate_code() and bpfjit_free_code()
functions were also renamed to op_bpfjit_generate_code() and
op_bpfjit_free_code().

## sljit
The NetBSD BPF code requires the sljit library r333.
Later sljit versions have breaking API changes.

svn co https://svn.code.sf.net/p/sljit/code@r333 deps/sljit

