#
# Top level Makefile for openperf
#

all: openperf libopenperf-shim

.PHONY: deps
deps:
	@git submodule init && git submodule update

.PHONY: openperf
openperf: deps
	@cd targets/openperf && $(MAKE)

.PHONY: libopenperf-shim
libopenperf-shim: deps
	@cd targets/libopenperf-shim && $(MAKE)

.PHONY: test
test: test_unit test_aat

.PHONY: test_aat
test_aat: openperf libopenperf-shim
	@cd tests/aat && $(MAKE)

.PHONY: test_unit
test_unit: deps
	@cd tests/unit && $(MAKE)

.PHONY: clean
clean:
	@cd targets/openperf && $(MAKE) clean
	@cd targets/libopenperf-shim && $(MAKE) clean
	@cd tests/aat && $(MAKE) clean
	@cd tests/unit && $(MAKE) clean
	@rm -f compile_commands.json

# Generate code from schema definitions
# Since generated code is stored in the repo, manual generation is only required
# when the schema files are updated.  Commit any schema changes and resulting
# generated code updates back to the source repository.
.PHONY: codegen
codegen: schema_codegen

.PHONY: schema_codegen
schema_codegen: packet_protocol_codegen
	@cd api/schema && $(MAKE)

.PHONY: packet_protocol_codegen
packet_protocol_codegen: libpacket_codegen
	@cd src/modules/packet/protocol && $(MAKE)

.PHONY: libpacket_codegen
libpacket_codegen:
	@cd src/lib/packet && $(MAKE)


###
# Targets for code formatting and analysis
###

.PHONY: pretty
pretty: pretty-cpp pretty-c

# We skip a lot of the C-code....
# We want to leave white-space as-is for any LwIP derived code so that we can
# easily compare deltas when upgrading.
# We also skip over anything with the C11 _Generic keyword; clang-format
# doesn't handle it correctly.
pretty-c:
	@find src targets tests \
		-path src/swagger -prune -o \
		-type f \( \
			-iname \*.c -o \
			-iname \*.h \) \
		! -name bsd_tree.h \
		$(shell printf "! -name %s " $(shell basename -a $(shell grep -rl "This file is part of the lwIP" src))) \
		$(shell printf "! -name %s " $(shell basename -a $(shell grep -rl _Generic src))) \
		-print0 | xargs -0 clang-format -i -style=file

pretty-cpp:
	@find src targets tests \
		-path src/swagger -prune -o \
		-path src/lib/packet/protocol -prune -o \
		-path src/modules/packet/protocol/transmogrify -prune -o \
		-type f \( \
			-iname \*.cpp -o \
			-iname \*.hpp -o \
			-iname \*.tcc \) \
		-print0 | xargs -0 clang-format -i -style=file

# clang-tidy requires a compile_commands.json file which is oustide the scope
# of this Makefile.  Hence, we make the file a requirement but don't provide
# a rule to build it.
.PHONY: tidy

TIDY_ARGS:= -quiet
ifneq (,$(TIDY_JOBS))
	TIDY_ARGS+= -j $(TIDY_JOBS)
endif

tidy: compile_commands.json
	@find src \
		-path src/swagger -prune -o \
		-path src/lib/packet/protocol -prune -o \
		-path src/modules/packet/protocol/transmogrify -prune -o \
		-iname \*.cpp -print0 \
		| xargs -0 run-clang-tidy $(strip $(TIDY_ARGS)) \
			-header-filter='^$(shell pwd)/src/.*hpp,-^$(shell pwd)/src/modules/socket/dpdk/.*hpp' \
			-extra-arg=-Wno-shadow
