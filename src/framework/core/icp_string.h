#ifndef _ICP_STRING_H_
#define _ICP_STRING_H_

/**
 * Let the compiler help us manage strings.  The cleanup attribute causes
 * the specified function to be called when the variable goes out of scope
 */
void icp_free_heap_string(char **x);
#define SCOPED_HEAP_STRING(x) char *x __attribute__((__cleanup__(icp_free_heap_string)))

/**
 * Return a new string that replaces all elements of find in src with repl
 * Caller should free the returned string.
 */
char * icp_strrepl(const char *src, const char *find, const char *repl);

#endif /* _ICP_STRING_H_ */
