#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

int main()
{
	uint64_t val = (uint64_t)lrand48();
	printf("val = %lu\n", val);
}
