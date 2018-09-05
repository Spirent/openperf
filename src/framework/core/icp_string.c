#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "core/icp_string.h"

char * icp_strrepl(const char *src, const char *find, const char *repl)
{
    size_t src_length = strlen(src) + 1;  /* include terminating NUL */
    size_t dst_length = src_length;
    size_t repl_length = strlen(repl);
    size_t find_length = strlen(find);

    /* First, figure out how large destination needs to be */
    if (repl_length > find_length) {
        /* Loop through src looking for find */
        size_t nb_finds = 0;
        const char *scratch = src;
        while ((scratch = strstr(scratch, find)) != NULL) {
            nb_finds++;
            scratch++;
        }

        /*
         * Figure out new size of string after replacing all
         * instances of find.
         */
        dst_length += (repl_length - find_length) * nb_finds;
    }

    char *dst = calloc(dst_length + 1, sizeof(char));
    if (!dst)
        return (NULL);

    const char *prev_scursor = src;
    const char *scursor = src;
    char *dcursor = dst;

    /*
     * Find instances of find in src.  Copy preceding string into
     * dst followed by repl.  Update pointers and repeat...
     */
    while ((scursor = strstr(scursor, find)) != NULL) {
        /* copy src part of string to dst */
        strncpy(dcursor, prev_scursor, scursor - prev_scursor);
        dcursor += scursor - prev_scursor;
        /* copy repl to dst */
        strncpy(dcursor, repl, repl_length);
        /* update pointers */
        dcursor += repl_length;
        prev_scursor = scursor = scursor + find_length;
    }

    /* copy any leftovers */
    size_t remainder = (src + src_length - prev_scursor);
    if (remainder) {
        strncpy(dcursor, prev_scursor, remainder);
    }

    dst[dst_length] = '\0';  /* terminate string */

    return (dst);
}

void icp_free_heap_string(char **str)
{
    if (*str) {
        free(*str);
        *str = NULL;
    }
}
