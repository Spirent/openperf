# Background

OpenPerf uses lwIP, a Lightweight TCP/IP stack, for providing state-full
network traffic functionality.  At it's core, lwIP is a single threaded
stack implementation that uses message passing as the only mechanism for
programs to use the stack.

## Usage

The lwIP project's design goals are centered on embedded and reduced
resource computing.  However the implementation is very flexible with
numerous configuration options to tailor the code-base to its intended
environment.  Additionally, the code supports extensions via custom
platform specific functions and predefined macros that allow custom
functions to be inserted into the stack at specific points.

Even so, we had to modify core lwIP function in order to support
Generic Send/Receive Offloading and to use DPDK `mbufs` as lwIP
`pbufs`.  While forking the code could have solved this problem,
we instead chose to just replace whole compilation units with
derived code.  This approach *is* more work that strictly necessary,
but it provides two key benefits:

1. Generating diffs between upstream and utilized code is trivial.
2. Incorporating updates into our code base is straight forward.

## Future Work

This symbol replacement approach is sufficient to integrate lwIP with
our existing driver and enable all NIC transmit/receive offloads.  However,
this approach will not work if we choose to make the stack multi-threaded.

Luckily, lwIP has well established idioms for adding and extending
functionality. This typically involves C macros that can be redefined
via a header file.  Any fork we make *must* follow those idioms so that we
have the possibility of getting our changes merged upstream.

Finally, what we merge upstream *should* be limited to functionality that is
commonly available elsewhere, e.g. the lock-free data structures required for
a multi-threaded stack implementation *should* not be a part of lwIP directly.
Instead, we should create OpenPerf specific function hooks or macros that
allow extensions where we need them but are defaulted to provide the current
lwIP behavior.  This will allow us to replace the existing lwIP functionality
with our own logic.
