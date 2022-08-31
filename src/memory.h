#ifndef _NEXUS_AICH
#include <nexus.h>
#endif

#ifndef NEXUS_MEMORY_H
#define NEXUS_MEMORY_H

/****************************************************************************
 *  Memory allocation - specific functions
 ****************************************************************************/


/*
 *  Definitions that allow this header file to be used either with or
 *    without ANSI C features like function prototypes.  Taken from tcl.h.
 */

#undef _ANSI_ARGS_
#undef CONST
#if ((defined(__STDC__) || defined(SABER)) && !defined(NO_PROTOTYPE)) || defined(__cplusplus)
#   define _USING_PROTOTYPES_ 1
#   define _ANSI_ARGS_(x)	x
#   define CONST const
#   ifdef __cplusplus
#       define VARARGS (...)
#   else
#       define VARARGS ()
#   endif
#else
#   define _ANSI_ARGS_(x)	()
#   define CONST
#endif



/*****************************************************************************
 *  Function Declarations.
 *****************************************************************************/

extern void			add_xref_list ( );
extern void			clear_simulation ( );
extern CELL			make_cells _ANSI_ARGS_(( unsigned ));
extern SPECS			make_connect_specs ( );
extern CONNECTION_UN		make_connection_un ( );
extern CONNECTION		make_connections _ANSI_ARGS_(( int ));
extern EXTERN_CONNECTION	make_extern_connection ( );
extern NETWORK			make_network ( );
extern NEXUS			make_nexus ( );
extern PARAMETERS		make_parameters ( );
extern WEIGHT_ARRAY		make_weight_array ( );
extern float *			make_weight_list _ANSI_ARGS_(( int ));
extern void			make_xref_list ( );
extern void *			nxCalloc _ANSI_ARGS_(( size_t, size_t ));
extern void *			nxMalloc _ANSI_ARGS_(( size_t ));
extern void			freeLearningMethods _ANSI_ARGS_(( void ));
extern void			free_network _ANSI_ARGS_(( NETWORK ));
extern void			free_cells _ANSI_ARGS_(( int, int, CELL ));
extern void			free_parameters _ANSI_ARGS_(( PARAMETERS ));

#endif /* NEXUS_MEMORY.H */










/* Emacs editing section. DW 94.07.19 */

/*
Local Variables:
mode:C
c-indent-level:2
c-continued-statement-offset:2
c-brace-offset:0
c-brace-imaginary-offset:0
c-argdecl-indent:4
c-label-offset:-2
c-auto-newline:nil
truncate-partial-width-windows:nil
truncate-lines:nil
End:
*/
