# inception-core
How to bind ICP shim-layer enabled executables (i.e., their sockets) to specific interfaces
## environment in which this feature was tested
* machine - MacBook Pro running Mojave (Version 10.14.4)
* virtual machine (vm) - VMware Fusion (Version 11.0.2)
  * Number of cores must be >= 3
  * Number of network adaptors must be >= 2
    * set adaptor networking type to ***Private to my Mac***
    * MAC address can be auto-generated or manually assigned (**same MAC is used in ICP create interface definition**)
* vm OS - Ubuntu 64-bit 18.10 (ubuntu-18.10-desktop-amd64.iso)

## adaptor stuff
  * Disconnect the set of adaptors in Ubuntu that will be used by inception
    * ***nmcli device disconnect*** ifname
    * ***nmcli device show*** ifname (verify that *GENERAL.STATE* is *disconnected*)
    
## DPDK stuff
  * Configure DPDK in Ubuntu by invoking *dpdk-setup.sh* and selecting options: 16, 20, 23, 26 (for each adaptor)
  
        inception.core/deps/dpdk/usertools/dpdk-setup.sh
        ----------------------------------------------------------
        Step 1: Select the DPDK environment to build
        ----------------------------------------------------------
        [16] x86_64-native-linuxapp-clang
        ----------------------------------------------------------
        Step 2: Setup linuxapp environment 
        ----------------------------------------------------------
        [20] Insert IGB UIO module
        [23] Setup hugepage mappings for non-NUMA systems 
        [26] Bind Ethernet/Crypto device to IGB UIO module
  
## inception stuff
  * Start inception stack (***w*** option identifies the PCI address of each adaptor)
        
        ./inception-core/build/stack-linux-x86_64-testing/bin/inception --log-level trace --dpdk="-w02:01.0,-w02:06.0" -p 9000 > /tmp/inception.log 2>&1
        
  * Create ICP interface definitions
  
        {
          "config": {
              ...
              "mac_address": USE MAC address specified when adaptor X was created by VMware
              "static":{
                "address": "192.168.7.130",
                "prefix_length": 24,
                "gateway": "192.168.7.1"
                ...
          },
          "port_id": "0"
        }
        
        $ curl "http://localhost:9000/interfaces" -d@X.json
        
                {
          "config": {
              ...
              "mac_address": USE MAC address specified when adaptor Y was created by VMware
              "static":{
                "address": "192.168.7.139",
                "prefix_length": 24,
                "gateway": "192.168.7.1"
                ...
          },
          "port_id": "1"
        }
        
        $ curl "http://localhost:9000/interfaces" -d@Y.json
 
 ## use *nc* command to test if the bound sockets are working correctly
  * Configuring and running ICP shim-layer executable
    * New environment variable ICP_BINDTODEVICE needs to be set to internal lwip interface name (*io*N), where N is the value assigned to *id* in the return json string reply from the REST API interface creation request
    * In one window/shell, start the *listener*:
    
            $ sudo ICP_BINDTODEVICE=io0 ICP_TRACE=1 LD_PRELOAD=./inception.core/build/libicp-shim-linux-x86_64-testing/lib/libicp-shim.so nc -l 192.168.7.130 9999
    * In another window/shell, start the non-listener:
    
            $ sudo ICP_BINDTODEVICE=io1 ICP_TRACE=1 LD_PRELOAD=./inception.core/build/libicp-shim-linux-x86_64-testing/lib/libicp-shim.so nc 192.168.7.130 9999
