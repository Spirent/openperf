#
# Makefile component for framework unit tests
#

TEST_DEPENDS += config_file framework

TEST_SOURCES += \
	framework/test_config_file_utils.cpp \
	framework/test_config_utils.cpp \
	framework/test_enum_flags.cpp \
	framework/test_hashtab.cpp \
	framework/test_init_factory.cpp \
	framework/test_list.cpp \
	framework/test_logging.cpp \
	framework/test_mac_address.cpp \
	framework/test_ipv4_address.cpp \
	framework/test_ipv4_network.cpp \
	framework/test_offset_ptr.cpp \
	framework/test_recycle.cpp \
	framework/test_soa_container.cpp \
	framework/test_std_allocator.cpp \
	framework/test_task.cpp \
	framework/test_units_rate.cpp \
	framework/test_uuid.cpp \
