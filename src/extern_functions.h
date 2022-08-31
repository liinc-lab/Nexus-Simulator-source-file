#ifndef _EXTERN_FUNCTIONS_H
#define _EXTERN_FUNCTIONS_H

#include "nexus.h"

extern int init_extern_connections _ANSI_ARGS_((NETWORK net_head));
extern void save_simulation _ANSI_ARGS_((NETWORK head, char *file));
extern int extern_get_file _ANSI_ARGS_((NETWORK net_head));
extern void extern_put_file _ANSI_ARGS_((NETWORK net_head));

#endif /* _EXTERN_FUNCTIONS_H */
