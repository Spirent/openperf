PIO_DEPENDS += immer

PIO_WORKER_SOURCES += \
	callback.cpp \
	epoll_poller.cpp \
	event_loop_adapter.cpp \
	forwarding_table_impl.cpp \
	internal_worker.cpp \
	recycle_impl.cpp \
	rx_queue.cpp \
	transmit_table_impl.cpp \
	tx_mempool_allocator.cpp \
	tx_queue.cpp \
	tx_scheduler.cpp \
	tx_source.cpp \
	worker_client.cpp \
	worker_controller.cpp \
	worker_factory.cpp \
	worker_queues.cpp \
	worker_transmogrify.cpp \
	worker_tx_functions.cpp \
	worker.cpp \
	zmq_socket.cpp

###
# Annoyingly, immer makes use of unused parameters. Silence those warnings
# until we can fix them.
###
$(PIO_OBJ_DIR)/workers/dpdk/%.o: OP_CPPFLAGS += -Wno-unused-parameter -Wno-shadow
