#ifndef _RUN_H
#define _RUN_H

#include "nexus.h"

extern int run_simulation _ANSI_ARGS_((int cycles, int cells_per_cycle,
				       int type));
extern void close_error_files();

#endif /* _RUN_H */
