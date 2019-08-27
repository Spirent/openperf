FW_DEPENDS += libzmq
FW_LDLIBS += -pthread

FW_SOURCES += \
	core/icp_cpuset.c \
	core/icp_event_loop_utils.c \
	core/icp_hashtab.c \
	core/icp_init.c \
	core/icp_list.c \
	core/icp_log.c \
	core/icp_modules.c \
	core/icp_options.c \
	core/icp_socket.c \
	core/icp_task.c \
	core/icp_version.c

ifeq ($(PLATFORM), linux)
	FW_LDLIBS += -rdynamic
	FW_SOURCES += \
	core/icp_event_loop_epoll.c \
	core/icp_exit_backtrace.c \
	core/icp_thread_linux.c \
	core/icp_cpuset_linux.c
else ifeq ($(PLATFORM), darwin)
	FW_SOURCES += \
	core/icp_event_loop_kqueue.c \
	core/icp_exit_backtrace.c \
	core/icp_thread_osx.c
else
	FW_SOURCES += \
	core/icp_cpuset_bsd.c \
	core/icp_event_loop_kqueue.c \
	core/icp_exit.c \
	core/icp_thread_bsd.c
endif

FW_VERSIONED_FILES := icp_version.o
FW_UNVERSIONED_OBJECTS :=\
	$(call icp_generate_objects,$(filter-out,$(FW_VERSIONED_FILES),$(FW_SOURCES)),$(FW_OBJ_DIR))

$(FW_OBJ_DIR)/core/icp_version.o: $(FW_UNVERSIONED_OBJECTS:.o=.d)
$(FW_OBJ_DIR)/core/icp_version.o: ICP_CFLAGS += \
        -DBUILD_VERSION="\"$(GIT_VERSION)\""
