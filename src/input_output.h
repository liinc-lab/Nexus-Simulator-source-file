#ifndef _INPUT_OUTPUT_H
#define _INPUT_OUTPUT_H

#include <stdio.h>
#include "nexus.h"

extern void load_simulation _ANSI_ARGS_((char *file));
extern int read_network _ANSI_ARGS_((FILE *fp));

#endif /* _INPUT_OUTPUT_H */
