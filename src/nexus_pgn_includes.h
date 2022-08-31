#ifndef _NEXUS_PGN_INCLUDES
#define _NEXUS_PGN_INCLUDES

/*************************************************************
*                                                            *
*                            NEXUS                           *
*                                                            *
*              (c) 1990 Paul Sajda and Leif Finkel           *
*                                                            *
*                    (nexus_pgn_includes.h)                  *
*                                                            *
*                                                            *
**************************************************************/

/***********
 *  PGN library functions
 ***********/

/*
 *  int
 *  get_argument( char *pgn_func, int arg_num, char *argument )
 *
 *  pgn_func is a string (char *, or char[]) containing the call to the
 *    PGN function.  This includes the name (argument 0) and the provided
 *    arguments in the .nx file (arguments 1 - n).
 *
 *  arg_num is the number of the argument sought after.
 *
 *  argument MUST be a pointer to previously-allocated memory, large enough
 *    to hold the largest argument to be given (we suggest 50 units).
 */

extern void		get_argument(char *function, int number,
				     char *returned_argument);

extern CELL		get_cell_at_position();

/*
 *  These are now #define's to speed things up and simplify program code.
 */

#define cell_position_x(cell,dim_x,dim_y) \
	(((cell) - 1) % (dim_x))
#define cell_position_y(cell,dim_x,dim_y) \
	(((cell) - 1) / (dim_x))

extern int		get_current_cycle();

extern int		determine_pgn_function();

extern char		*get_name_of_network();

extern CELL		get_nearest_neighbor();

extern CONNECTION	get_net_connection();

extern NETWORK		get_network_id();

extern NETWORK		get_network_name();

extern int		not_at_edge();

#endif /* _NEXUS_PGN_INCLUDES */
