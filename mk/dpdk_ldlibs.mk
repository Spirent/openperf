#
# Makefile component to generate DPDK library list from a full
# configuration file
#
# This file is based on the library link generation code found
# in <DPDK_ROOT>/mk/rte.app.mk.
# Please update this file when updating DPDK code.
#

DPDK_FLAGS_REQ_VARS := \
	DPDK_DEFCONFIG \
	DPDK_LIB_DIR \
	DPDK_SRC_DIR
$(call icp_check_vars,$(DPDK_FLAGS_REQ_VARS))

include $(DPDK_DEFCONFIG)

# Do some fix-ups
ifeq ($(CONFIG_RTE_LIBRTE_IGB_PMD),y)
	CONFIG_RTE_LIBRTE_E1000_PMD=y
endif
ifeq ($(CONFIG_RTE_LIBRTE_EM_PMD),y)
	CONFIG_RTE_LIBRTE_E1000_PMD=y
endif

#
# Order is important: from higher level to lower level
#
_LDLIBS-$(CONFIG_RTE_LIBRTE_FLOW_CLASSIFY)  += -lrte_flow_classify
_LDLIBS-$(CONFIG_RTE_LIBRTE_PIPELINE)       += -Wl,--whole-archive
_LDLIBS-$(CONFIG_RTE_LIBRTE_PIPELINE)       += -lrte_pipeline
_LDLIBS-$(CONFIG_RTE_LIBRTE_PIPELINE)       += -Wl,--no-whole-archive
_LDLIBS-$(CONFIG_RTE_LIBRTE_TABLE)          += -Wl,--whole-archive
_LDLIBS-$(CONFIG_RTE_LIBRTE_TABLE)          += -lrte_table
_LDLIBS-$(CONFIG_RTE_LIBRTE_TABLE)          += -Wl,--no-whole-archive
_LDLIBS-$(CONFIG_RTE_LIBRTE_PORT)           += -Wl,--whole-archive
_LDLIBS-$(CONFIG_RTE_LIBRTE_PORT)           += -lrte_port
_LDLIBS-$(CONFIG_RTE_LIBRTE_PORT)           += -Wl,--no-whole-archive

_LDLIBS-$(CONFIG_RTE_LIBRTE_PDUMP)          += -lrte_pdump
_LDLIBS-$(CONFIG_RTE_LIBRTE_DISTRIBUTOR)    += -lrte_distributor
_LDLIBS-$(CONFIG_RTE_LIBRTE_IP_FRAG)        += -lrte_ip_frag
_LDLIBS-$(CONFIG_RTE_LIBRTE_METER)          += -lrte_meter
_LDLIBS-$(CONFIG_RTE_LIBRTE_LPM)            += -lrte_lpm
# librte_acl needs -Wl,--whole-archive because of weak functions
_LDLIBS-$(CONFIG_RTE_LIBRTE_ACL)            += -Wl,--whole-archive
_LDLIBS-$(CONFIG_RTE_LIBRTE_ACL)            += -lrte_acl
_LDLIBS-$(CONFIG_RTE_LIBRTE_ACL)            += -Wl,--no-whole-archive
_LDLIBS-$(CONFIG_RTE_LIBRTE_TELEMETRY)      += -Wl,--no-as-needed
_LDLIBS-$(CONFIG_RTE_LIBRTE_TELEMETRY)      += -Wl,--whole-archive
_LDLIBS-$(CONFIG_RTE_LIBRTE_TELEMETRY)      += -lrte_telemetry -ljansson
_LDLIBS-$(CONFIG_RTE_LIBRTE_TELEMETRY)      += -Wl,--no-whole-archive
_LDLIBS-$(CONFIG_RTE_LIBRTE_TELEMETRY)      += -Wl,--as-needed
_LDLIBS-$(CONFIG_RTE_LIBRTE_JOBSTATS)       += -lrte_jobstats
_LDLIBS-$(CONFIG_RTE_LIBRTE_METRICS)        += -lrte_metrics
_LDLIBS-$(CONFIG_RTE_LIBRTE_BITRATE)        += -lrte_bitratestats
_LDLIBS-$(CONFIG_RTE_LIBRTE_LATENCY_STATS)  += -lrte_latencystats
_LDLIBS-$(CONFIG_RTE_LIBRTE_POWER)          += -lrte_power

_LDLIBS-$(CONFIG_RTE_LIBRTE_EFD)            += -lrte_efd
_LDLIBS-$(CONFIG_RTE_LIBRTE_BPF)            += -lrte_bpf
ifeq ($(CONFIG_RTE_LIBRTE_BPF_ELF),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_BPF)            += -lelf
endif

_LDLIBS-$(CONFIG_RTE_LIBRTE_IPSEC)            += -lrte_ipsec

_LDLIBS-y += -Wl,--whole-archive

_LDLIBS-$(CONFIG_RTE_LIBRTE_CFGFILE)        += -lrte_cfgfile
_LDLIBS-$(CONFIG_RTE_LIBRTE_GRO)            += -lrte_gro
_LDLIBS-$(CONFIG_RTE_LIBRTE_GSO)            += -lrte_gso
_LDLIBS-$(CONFIG_RTE_LIBRTE_HASH)           += -lrte_hash
_LDLIBS-$(CONFIG_RTE_LIBRTE_MEMBER)         += -lrte_member
_LDLIBS-$(CONFIG_RTE_LIBRTE_VHOST)          += -lrte_vhost
_LDLIBS-$(CONFIG_RTE_LIBRTE_KVARGS)         += -lrte_kvargs
_LDLIBS-$(CONFIG_RTE_LIBRTE_MBUF)           += -lrte_mbuf
_LDLIBS-$(CONFIG_RTE_LIBRTE_NET)            += -lrte_net
_LDLIBS-$(CONFIG_RTE_LIBRTE_ETHER)          += -lrte_ethdev
_LDLIBS-$(CONFIG_RTE_LIBRTE_BBDEV)          += -lrte_bbdev
_LDLIBS-$(CONFIG_RTE_LIBRTE_CRYPTODEV)      += -lrte_cryptodev
_LDLIBS-$(CONFIG_RTE_LIBRTE_SECURITY)       += -lrte_security
_LDLIBS-$(CONFIG_RTE_LIBRTE_COMPRESSDEV)    += -lrte_compressdev
_LDLIBS-$(CONFIG_RTE_LIBRTE_EVENTDEV)       += -lrte_eventdev
_LDLIBS-$(CONFIG_RTE_LIBRTE_RAWDEV)         += -lrte_rawdev
_LDLIBS-$(CONFIG_RTE_LIBRTE_TIMER)          += -lrte_timer
_LDLIBS-$(CONFIG_RTE_LIBRTE_MEMPOOL)        += -lrte_mempool
_LDLIBS-$(CONFIG_RTE_LIBRTE_STACK)          += -lrte_stack
_LDLIBS-$(CONFIG_RTE_DRIVER_MEMPOOL_RING)   += -lrte_mempool_ring
_LDLIBS-$(CONFIG_RTE_LIBRTE_OCTEONTX2_MEMPOOL) += -lrte_mempool_octeontx2
_LDLIBS-$(CONFIG_RTE_LIBRTE_RING)           += -lrte_ring
_LDLIBS-$(CONFIG_RTE_LIBRTE_PCI)            += -lrte_pci
_LDLIBS-$(CONFIG_RTE_LIBRTE_EAL)            += -lrte_eal
_LDLIBS-$(CONFIG_RTE_LIBRTE_CMDLINE)        += -lrte_cmdline
_LDLIBS-$(CONFIG_RTE_LIBRTE_REORDER)        += -lrte_reorder
_LDLIBS-$(CONFIG_RTE_LIBRTE_SCHED)          += -lrte_sched
_LDLIBS-$(CONFIG_RTE_LIBRTE_RCU)            += -lrte_rcu

ifeq ($(CONFIG_RTE_EXEC_ENV_LINUXAPP),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_KNI)            += -lrte_kni
endif

ifeq ($(CONFIG_RTE_LIBRTE_PMD_OCTEONTX_CRYPTO),y)
_LDLIBS-y += -lrte_common_cpt
endif

ifeq ($(CONFIG_RTE_LIBRTE_PMD_OCTEONTX_SSOVF)$(CONFIG_RTE_LIBRTE_OCTEONTX_MEMPOOL),yy)
_LDLIBS-y += -lrte_common_octeontx
endif
OCTEONTX2-y := $(CONFIG_RTE_LIBRTE_OCTEONTX2_MEMPOOL)
OCTEONTX2-y += $(CONFIG_RTE_LIBRTE_PMD_OCTEONTX2_EVENTDEV)
OCTEONTX2-y += $(CONFIG_RTE_LIBRTE_PMD_OCTEONTX2_DMA_RAWDEV)
OCTEONTX2-y += $(CONFIG_RTE_LIBRTE_OCTEONTX2_PMD)
ifeq ($(findstring y,$(OCTEONTX2-y)),y)
_LDLIBS-y += -lrte_common_octeontx2
endif

MVEP-y := $(CONFIG_RTE_LIBRTE_MVPP2_PMD)
MVEP-y += $(CONFIG_RTE_LIBRTE_MVNETA_PMD)
MVEP-y += $(CONFIG_RTE_LIBRTE_PMD_MVSAM_CRYPTO)
ifneq (,$(findstring y,$(MVEP-y)))
_LDLIBS-y += -lrte_common_mvep -L$(LIBMUSDK_PATH)/lib -lmusdk
endif

ifeq ($(CONFIG_RTE_LIBRTE_DPAA_BUS),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_COMMON_DPAAX)   += -lrte_common_dpaax
endif
ifeq ($(CONFIG_RTE_LIBRTE_FSLMC_BUS),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_COMMON_DPAAX)   += -lrte_common_dpaax
endif

_LDLIBS-$(CONFIG_RTE_LIBRTE_PCI_BUS)        += -lrte_bus_pci
_LDLIBS-$(CONFIG_RTE_LIBRTE_VDEV_BUS)       += -lrte_bus_vdev
_LDLIBS-$(CONFIG_RTE_LIBRTE_DPAA_BUS)       += -lrte_bus_dpaa
ifeq ($(CONFIG_RTE_EAL_VFIO),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_FSLMC_BUS)      += -lrte_bus_fslmc
endif

ifeq ($(CONFIG_RTE_BUILD_SHARED_LIB),n)
# plugins (link only if static libraries)

_LDLIBS-$(CONFIG_RTE_DRIVER_MEMPOOL_BUCKET) += -lrte_mempool_bucket
_LDLIBS-$(CONFIG_RTE_DRIVER_MEMPOOL_STACK)  += -lrte_mempool_stack
ifeq ($(CONFIG_RTE_LIBRTE_DPAA_BUS),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_DPAA_MEMPOOL)   += -lrte_mempool_dpaa
endif
ifeq ($(CONFIG_RTE_EAL_VFIO)$(CONFIG_RTE_LIBRTE_FSLMC_BUS),yy)
_LDLIBS-$(CONFIG_RTE_LIBRTE_DPAA2_MEMPOOL)  += -lrte_mempool_dpaa2
endif

_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_AF_PACKET)  += -lrte_pmd_af_packet
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_AF_XDP)     += -lrte_pmd_af_xdp -lbpf
_LDLIBS-$(CONFIG_RTE_LIBRTE_ARK_PMD)        += -lrte_pmd_ark
_LDLIBS-$(CONFIG_RTE_LIBRTE_ATLANTIC_PMD)   += -lrte_pmd_atlantic
_LDLIBS-$(CONFIG_RTE_LIBRTE_AVF_PMD)        += -lrte_pmd_avf
_LDLIBS-$(CONFIG_RTE_LIBRTE_AXGBE_PMD)      += -lrte_pmd_axgbe
_LDLIBS-$(CONFIG_RTE_LIBRTE_BNX2X_PMD)      += -lrte_pmd_bnx2x -lz
_LDLIBS-$(CONFIG_RTE_LIBRTE_BNXT_PMD)       += -lrte_pmd_bnxt
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_BOND)       += -lrte_pmd_bond
_LDLIBS-$(CONFIG_RTE_LIBRTE_CXGBE_PMD)      += -lrte_pmd_cxgbe
ifeq ($(CONFIG_RTE_LIBRTE_DPAA_BUS),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_DPAA_PMD)       += -lrte_pmd_dpaa
endif
ifeq ($(CONFIG_RTE_EAL_VFIO)$(CONFIG_RTE_LIBRTE_FSLMC_BUS),yy)
_LDLIBS-$(CONFIG_RTE_LIBRTE_DPAA2_PMD)      += -lrte_pmd_dpaa2
endif
_LDLIBS-$(CONFIG_RTE_LIBRTE_E1000_PMD)      += -lrte_pmd_e1000
_LDLIBS-$(CONFIG_RTE_LIBRTE_ENA_PMD)        += -lrte_pmd_ena
_LDLIBS-$(CONFIG_RTE_LIBRTE_ENETC_PMD)      += -lrte_pmd_enetc
_LDLIBS-$(CONFIG_RTE_LIBRTE_ENIC_PMD)       += -lrte_pmd_enic
_LDLIBS-$(CONFIG_RTE_LIBRTE_FM10K_PMD)      += -lrte_pmd_fm10k
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_FAILSAFE)   += -lrte_pmd_failsafe
_LDLIBS-$(CONFIG_RTE_LIBRTE_HINIC)          += -lrte_pmd_hinic
_LDLIBS-$(CONFIG_RTE_LIBRTE_I40E_PMD)       += -lrte_pmd_i40e
_LDLIBS-$(CONFIG_RTE_LIBRTE_IAVF_PMD)       += -lrte_pmd_iavf
_LDLIBS-$(CONFIG_RTE_LIBRTE_ICE_PMD)        += -lrte_pmd_ice
_LDLIBS-$(CONFIG_RTE_LIBRTE_IXGBE_PMD)      += -lrte_pmd_ixgbe
ifeq ($(CONFIG_RTE_LIBRTE_KNI),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_KNI)        += -lrte_pmd_kni
endif
_LDLIBS-$(CONFIG_RTE_LIBRTE_LIO_PMD)        += -lrte_pmd_lio
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_MEMIF)      += -lrte_pmd_memif
_LDLIBS-$(CONFIG_RTE_LIBRTE_MLX4_PMD)       += -lrte_pmd_mlx4
_LDLIBS-$(CONFIG_RTE_LIBRTE_MLX5_PMD)       += -lrte_pmd_mlx5 -lmnl
ifeq ($(CONFIG_RTE_IBVERBS_LINK_DLOPEN),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_MLX4_PMD)       += -ldl
_LDLIBS-$(CONFIG_RTE_LIBRTE_MLX5_PMD)       += -ldl
else ifeq ($(CONFIG_RTE_IBVERBS_LINK_STATIC),y)
LIBS_IBVERBS_STATIC = $(shell $(DPDK_SRC_DIR)/buildtools/options-ibverbs-static.sh)
_LDLIBS-$(CONFIG_RTE_LIBRTE_MLX4_PMD)       += $(LIBS_IBVERBS_STATIC)
_LDLIBS-$(CONFIG_RTE_LIBRTE_MLX5_PMD)       += $(LIBS_IBVERBS_STATIC)
else
_LDLIBS-$(CONFIG_RTE_LIBRTE_MLX4_PMD)       += -libverbs -lmlx4
_LDLIBS-$(CONFIG_RTE_LIBRTE_MLX5_PMD)       += -libverbs -lmlx5
endif
_LDLIBS-$(CONFIG_RTE_LIBRTE_MVPP2_PMD)      += -lrte_pmd_mvpp2
_LDLIBS-$(CONFIG_RTE_LIBRTE_MVNETA_PMD)     += -lrte_pmd_mvneta
_LDLIBS-$(CONFIG_RTE_LIBRTE_NFP_PMD)        += -lrte_pmd_nfp
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_NULL)       += -lrte_pmd_null
_LDLIBS-$(CONFIG_RTE_LIBRTE_OCTEONTX2_PMD)  += -lrte_pmd_octeontx2 -lm
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_PCAP)       += -lrte_pmd_pcap -lpcap
_LDLIBS-$(CONFIG_RTE_LIBRTE_QEDE_PMD)       += -lrte_pmd_qede
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_RING)       += -lrte_pmd_ring
ifeq ($(CONFIG_RTE_LIBRTE_SCHED),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_SOFTNIC)      += -lrte_pmd_softnic
endif
_LDLIBS-$(CONFIG_RTE_LIBRTE_SFC_EFX_PMD)    += -lrte_pmd_sfc_efx
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_SZEDATA2)   += -lrte_pmd_szedata2 -lsze2
_LDLIBS-$(CONFIG_RTE_LIBRTE_NFB_PMD)        += -lrte_pmd_nfb
_LDLIBS-$(CONFIG_RTE_LIBRTE_NFB_PMD)        +=  $(shell command -v pkg-config > /dev/null 2>&1 && pkg-config --libs netcope-common)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_TAP)        += -lrte_pmd_tap
_LDLIBS-$(CONFIG_RTE_LIBRTE_THUNDERX_NICVF_PMD) += -lrte_pmd_thunderx_nicvf
_LDLIBS-$(CONFIG_RTE_LIBRTE_VDEV_NETVSC_PMD) += -lrte_pmd_vdev_netvsc
_LDLIBS-$(CONFIG_RTE_LIBRTE_VIRTIO_PMD)     += -lrte_pmd_virtio
ifeq ($(CONFIG_RTE_LIBRTE_VHOST),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_VHOST)      += -lrte_pmd_vhost
ifeq ($(CONFIG_RTE_EAL_VFIO),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_IFC_PMD) += -lrte_pmd_ifc
endif # $(CONFIG_RTE_EAL_VFIO)
endif # $(CONFIG_RTE_LIBRTE_VHOST)
_LDLIBS-$(CONFIG_RTE_LIBRTE_VMXNET3_PMD)    += -lrte_pmd_vmxnet3_uio

_LDLIBS-$(CONFIG_RTE_LIBRTE_VMBUS)          += -lrte_bus_vmbus
_LDLIBS-$(CONFIG_RTE_LIBRTE_NETVSC_PMD)     += -lrte_pmd_netvsc

ifeq ($(CONFIG_RTE_LIBRTE_BBDEV),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_BBDEV_NULL)     += -lrte_pmd_bbdev_null
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_FPGA_LTE_FEC) += -lrte_pmd_fpga_lte_fec

# TURBO SOFTWARE PMD is dependent on the FLEXRAN library
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_BBDEV_TURBO_SW) += -lrte_pmd_bbdev_turbo_sw
ifeq ($(CONFIG_RTE_BBDEV_SDK_AVX2),y)
# Dependency on the FLEXRAN SDK library if available
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_BBDEV_TURBO_SW) += -L$(FLEXRAN_SDK)/lib_crc -lcrc
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_BBDEV_TURBO_SW) += -L$(FLEXRAN_SDK)/lib_turbo -lturbo
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_BBDEV_TURBO_SW) += -L$(FLEXRAN_SDK)/lib_rate_matching -lrate_matching
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_BBDEV_TURBO_SW) += -L$(FLEXRAN_SDK)/lib_common -lcommon
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_BBDEV_TURBO_SW) += -lirc -limf -lstdc++ -lipps -lsvml
ifeq ($(CONFIG_RTE_BBDEV_SDK_AVX512),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_BBDEV_TURBO_SW) += -L$(FLEXRAN_SDK)/lib_LDPC_ratematch_5gnr -lLDPC_ratematch_5gnr
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_BBDEV_TURBO_SW) += -L$(FLEXRAN_SDK)/lib_ldpc_encoder_5gnr -lldpc_encoder_5gnr
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_BBDEV_TURBO_SW) += -L$(FLEXRAN_SDK)/lib_ldpc_decoder_5gnr -lldpc_decoder_5gnr
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_BBDEV_TURBO_SW) += -L$(FLEXRAN_SDK)/lib_rate_dematching_5gnr -lrate_dematching_5gnr
endif # CONFIG_RTE_BBDEV_SDK_AVX512
endif # CONFIG_RTE_BBDEV_SDK_AVX2
endif # CONFIG_RTE_LIBRTE_BBDEV

ifeq ($(CONFIG_RTE_LIBRTE_CRYPTODEV),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_AESNI_MB)    += -lrte_pmd_aesni_mb
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_AESNI_MB)    += -lIPSec_MB
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_AESNI_GCM)   += -lrte_pmd_aesni_gcm
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_AESNI_GCM)   += -lIPSec_MB
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_CCP)         += -lrte_pmd_ccp -lcrypto
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_OPENSSL)     += -lrte_pmd_openssl -lcrypto
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_NULL_CRYPTO) += -lrte_pmd_null_crypto
ifeq ($(CONFIG_RTE_LIBRTE_PMD_QAT),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_QAT_SYM)     += -lrte_pmd_qat -lcrypto
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_QAT_ASYM)    += -lrte_pmd_qat -lcrypto
endif # CONFIG_RTE_LIBRTE_PMD_QAT
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_SNOW3G)      += -lrte_pmd_snow3g
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_SNOW3G)      += -L$(LIBSSO_SNOW3G_PATH)/build -lsso_snow3g
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_KASUMI)      += -lrte_pmd_kasumi
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_KASUMI)      += -L$(LIBSSO_KASUMI_PATH)/build -lsso_kasumi
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_ZUC)         += -lrte_pmd_zuc
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_ZUC)         += -L$(LIBSSO_ZUC_PATH)/build -lsso_zuc
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_ARMV8_CRYPTO)    += -lrte_pmd_armv8
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_ARMV8_CRYPTO)    += -L$(ARMV8_CRYPTO_LIB_PATH) -larmv8_crypto
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_MVSAM_CRYPTO) += -L$(LIBMUSDK_PATH)/lib -lrte_pmd_mvsam_crypto -lmusdk
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_OCTEONTX_CRYPTO) += -lrte_pmd_octeontx_crypto
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_CRYPTO_SCHEDULER) += -lrte_pmd_crypto_scheduler
ifeq ($(CONFIG_RTE_LIBRTE_SECURITY),y)
ifeq ($(CONFIG_RTE_EAL_VFIO)$(CONFIG_RTE_LIBRTE_FSLMC_BUS),yy)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_DPAA2_SEC)   += -lrte_pmd_dpaa2_sec
endif # CONFIG_RTE_LIBRTE_FSLMC_BUS
ifeq ($(CONFIG_RTE_LIBRTE_DPAA_BUS),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_DPAA_SEC)   += -lrte_pmd_dpaa_sec
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_CAAM_JR)   += -lrte_pmd_caam_jr
endif # CONFIG_RTE_LIBRTE_DPAA_BUS
endif # CONFIG_RTE_LIBRTE_SECURITY
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_VIRTIO_CRYPTO) += -lrte_pmd_virtio_crypto
endif # CONFIG_RTE_LIBRTE_CRYPTODEV

ifeq ($(CONFIG_RTE_LIBRTE_COMPRESSDEV),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_ISAL) += -lrte_pmd_isal_comp
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_ISAL) += -lisal
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_OCTEONTX_ZIPVF) += -lrte_pmd_octeontx_zip
# Link QAT driver if it has not been linked yet
ifeq ($(CONFIG_RTE_LIBRTE_PMD_QAT_SYM),n)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_QAT)  += -lrte_pmd_qat
endif # CONFIG_RTE_LIBRTE_PMD_QAT_SYM
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_ZLIB) += -lrte_pmd_zlib
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_ZLIB) += -lz
endif # CONFIG_RTE_LIBRTE_COMPRESSDEV

ifeq ($(CONFIG_RTE_LIBRTE_EVENTDEV),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_SKELETON_EVENTDEV) += -lrte_pmd_skeleton_event
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_SW_EVENTDEV) += -lrte_pmd_sw_event
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_DSW_EVENTDEV) += -lrte_pmd_dsw_event
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_OCTEONTX_SSOVF) += -lrte_pmd_octeontx_ssovf
ifeq ($(CONFIG_RTE_LIBRTE_DPAA_BUS),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_DPAA_EVENTDEV) += -lrte_pmd_dpaa_event
endif # CONFIG_RTE_LIBRTE_DPAA_BUS
ifeq ($(CONFIG_RTE_EAL_VFIO)$(CONFIG_RTE_LIBRTE_FSLMC_BUS),yy)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_DPAA2_EVENTDEV) += -lrte_pmd_dpaa2_event
endif # CONFIG_RTE_LIBRTE_FSLMC_BUS

_LDLIBS-$(CONFIG_RTE_LIBRTE_OCTEONTX_MEMPOOL) += -lrte_mempool_octeontx
_LDLIBS-$(CONFIG_RTE_LIBRTE_OCTEONTX_PMD) += -lrte_pmd_octeontx
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_OCTEONTX2_EVENTDEV) += -lrte_pmd_octeontx2_event
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_OPDL_EVENTDEV) += -lrte_pmd_opdl_event
endif # CONFIG_RTE_LIBRTE_EVENTDEV

ifeq ($(CONFIG_RTE_LIBRTE_RAWDEV),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_SKELETON_RAWDEV) += -lrte_pmd_skeleton_rawdev
ifeq ($(CONFIG_RTE_EAL_VFIO)$(CONFIG_RTE_LIBRTE_FSLMC_BUS),yy)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_DPAA2_CMDIF_RAWDEV) += -lrte_pmd_dpaa2_cmdif
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_DPAA2_QDMA_RAWDEV) += -lrte_pmd_dpaa2_qdma
endif # CONFIG_RTE_LIBRTE_FSLMC_BUS
_LDLIBS-$(CONFIG_RTE_LIBRTE_IFPGA_BUS)      += -lrte_bus_ifpga
ifeq ($(CONFIG_RTE_LIBRTE_IFPGA_BUS),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_IFPGA_RAWDEV) += -lrte_pmd_ifpga_rawdev
_LDLIBS-$(CONFIG_RTE_LIBRTE_IPN3KE_PMD)       += -lrte_pmd_ipn3ke
endif # CONFIG_RTE_LIBRTE_IFPGA_BUS
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_IOAT_RAWDEV)   += -lrte_rawdev_ioat
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_NTB_RAWDEV) += -lrte_rawdev_ntb
_LDLIBS-$(CONFIG_RTE_LIBRTE_PMD_OCTEONTX2_DMA_RAWDEV) += -lrte_rawdev_octeontx2_dma
endif # CONFIG_RTE_LIBRTE_RAWDEV

endif # !CONFIG_RTE_BUILD_SHARED_LIBS

_LDLIBS-y += -Wl,--no-whole-archive

ifeq ($(CONFIG_RTE_BUILD_SHARED_LIB),n)
# The static libraries do not know their dependencies.
# So linking with static library requires explicit dependencies.
_LDLIBS-$(CONFIG_RTE_LIBRTE_EAL)            += -lrt
ifeq ($(CONFIG_RTE_EXEC_ENV_LINUXAPP)$(CONFIG_RTE_EAL_NUMA_AWARE_HUGEPAGES),yy)
_LDLIBS-$(CONFIG_RTE_LIBRTE_EAL)            += -lnuma
endif
ifeq ($(CONFIG_RTE_EXEC_ENV_LINUXAPP)$(CONFIG_RTE_USE_LIBBSD),yy)
_LDLIBS-$(CONFIG_RTE_LIBRTE_EAL)            += -lbsd
endif
_LDLIBS-$(CONFIG_RTE_LIBRTE_SCHED)          += -lm
_LDLIBS-$(CONFIG_RTE_LIBRTE_SCHED)          += -lrt
_LDLIBS-$(CONFIG_RTE_LIBRTE_MEMBER)         += -lm
_LDLIBS-$(CONFIG_RTE_LIBRTE_METER)          += -lm
ifeq ($(CONFIG_RTE_LIBRTE_VHOST_NUMA),y)
_LDLIBS-$(CONFIG_RTE_LIBRTE_VHOST)          += -lnuma
endif
_LDLIBS-$(CONFIG_RTE_PORT_PCAP)             += -lpcap
endif # !CONFIG_RTE_BUILD_SHARED_LIBS

# all the words except the first one
allbutfirst = $(wordlist 2,$(words $(1)),$(1))

# Eliminate duplicates without sorting, only keep the last occurrence
filter-libs = \
	$(if $(1),$(strip\
		$(if \
			$(and \
				$(filter $(firstword $(1)),$(call allbutfirst,$(1))),\
				$(filter -l%,$(firstword $(1)))),\
			,\
			$(firstword $(1))) \
		$(call filter-libs,$(call allbutfirst,$(1)))))

ICP_LDLIBS += $(call filter-libs,$(_LDLIBS-y))
