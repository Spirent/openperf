REGURGITATE_SOURCES += \
	ispc/merge_float_double.ispc \
	ispc/merge_float_float.ispc \
	ispc/sort_float_double.ispc \
	ispc/sort_float_float.ispc

# XXX: ISPC seems to be missing __ceil_uniform_double for Aarch64, so add
# the proper symbol for it.
ifeq ($(ARCH),aarch64)
	REGURGITATE_SOURCES += ispc/ceil_uniform_double.c
endif
