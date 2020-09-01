FW_DEPENDS += libzmq
FW_LDLIBS += -pthread -ldl

FW_SOURCES += \
	core/op_cpuset.c \
	core/op_event_loop_utils.c \
	core/op_hashtab.c \
	core/op_init.c \
	core/op_list.c \
	core/op_log.c \
	core/op_modules.c \
	core/op_options.c \
	core/op_socket.c \
	core/op_task.c \
	core/op_version.c

ifeq ($(PLATFORM), linux)
	FW_LDLIBS += -rdynamic
	FW_SOURCES += \
	core/op_event_loop_epoll.c \
	core/op_exit_backtrace.c \
	core/op_thread_linux.c \
	core/op_cpuset_linux.c \
	core/op_modules_linux.c
else ifeq ($(PLATFORM), darwin)
	FW_SOURCES += \
	core/op_event_loop_kqueue.c \
	core/op_exit_backtrace.c \
	core/op_thread_osx.c
else
	FW_SOURCES += \
	core/op_cpuset_bsd.c \
	core/op_event_loop_kqueue.c \
	core/op_exit.c \
	core/op_thread_bsd.c
endif

FW_VERSIONED_FILES := op_version.o
FW_UNVERSIONED_OBJECTS :=\
	$(call op_generate_objects,$(filter-out,$(FW_VERSIONED_FILES),$(FW_SOURCES)),$(FW_OBJ_DIR))

$(FW_OBJ_DIR)/core/op_version.o: $(FW_UNVERSIONED_OBJECTS:.o=.d)
$(FW_OBJ_DIR)/core/op_version.o: OP_CFLAGS += \
        -DBUILD_VERSION="\"$(GIT_VERSION)\""
