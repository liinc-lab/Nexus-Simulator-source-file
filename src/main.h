#ifndef _MAIN_H
#define _MAIN_H

#include <stdio.h>
#include "nexus.h"

extern int set_trans_func _ANSI_ARGS_((NETWORK network,
					int clamp_flag, char *trans_func_arg));
extern void set_param _ANSI_ARGS_((NETWORK network,
				   int parameter, float value));
extern int output_activity _ANSI_ARGS_((FILE *fp));

#endif /* _MAIN_H */
