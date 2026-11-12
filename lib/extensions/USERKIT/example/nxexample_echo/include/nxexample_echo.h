#ifndef NEXSOS_USERKIT_EXAMPLE_ECHO_H
#define NEXSOS_USERKIT_EXAMPLE_ECHO_H
#include <stddef.h>
#define NXEXAMPLE_ECHO_PORT "OS1nx_example_echo"
int NX_example_echo(const char *message, char *reply, size_t reply_size);
#endif
