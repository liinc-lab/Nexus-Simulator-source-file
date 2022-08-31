/**************************************************************************
 *                                                                        *
 *                            NEXUS                                       *
 *                                                                        *
 *              (c) 1990 Paul Sajda and Leif Finkel                       *
 *                                                                        *
 *                      (pgn_functions.c)                                 *
 *							                  *
 * Example (dummy) pgn_function declarations file.                        *
 * Provides user with one (1) function 'pgn_test', which takes two        *
 * arguments and simply outputs them to the screen.                       *
 *                                                                        *
 * To compile, use the PGN_FUNCTIONS= and OUTPUT= settings, i.e.          *
 *                                                                        *
 *  make -f Makefile.sgi PGN_FUNCTIONS=$PWD/myfuncs OUTPUT=$PWD/mynexus   *
 *                                                                        *
 * NOTE:  Remember to leave off the ".c", and remember to supply the FULL *
 *        PATHNAMES (hence, the $PWD's) of your files.                    *
 *                                                                        *
 **************************************************************************/

#include "nexus.h"
#include "nexus_pgn_includes.h"


/**
 **  Global variables.
 **/

/*  (We don't have any global variables here)  */


/***********
 *  pgn_functions.c
 ***********/

/*
 *  For historical reasons this is declared as a function returning an
 *    integer.  However, in the code as of v1.0, the return value does
 *    NOT get processed.
 *
 *  FUNCTION_ARG_SIZE is still 50, which is backward-compatible with
 *    the old 'MAX_ARG_LEN'; however, FUNCTION_ARG_SIZE is used in
 *    other code, making it more consistent and uniform.  It should be
 *    used, therefore, in future PGN code, instead of MAX_ARG_LEN.
 */

int
do_pgn_functions( cell, pgn_func, network )
    CELL	cell;
    char	*pgn_func;
    NETWORK	network;
{
  /*  You must declare your C functions before you call them.  */
  extern void test_pgn( );

  /*
   *  The maximum number of arguments any PGN function in this file needs,
   *    is two.
   */
  char arg1[FUNCTION_ARG_SIZE],
       arg2[FUNCTION_ARG_SIZE];


  /*
   *  The one PGN function we have written is labelled "test".  This means
   *    that the transfer function must be written (in a .nx file) as
   *    "transfer function = pgn( test, arg1, arg2 );".
   */

  if (determine_pgn_function( pgn_func, "test" )) {
    get_argument( pgn_func, 1, arg1 );
    get_argument( pgn_func, 2, arg2 );
    test_pgn( arg1, arg2 );
    return OK;
  }
  
  /*
   *  DW 95.01.20  Bug fix.  If pgn function does not exist, stop
   *    immediately (see update_cell_activity() in functions.c).
   *    Otherwise nexus will continue simulating without meaningful results.
   */
  printf( "ERROR -- no such pgn function %s\n", pgn_func );
  return ERROR;
}

	     

void
test_pgn( arg1, arg2 )
    char *arg1, *arg2;
{
  printf( "Arg1 = \"%s\", Arg2 = \"%s\".\n", arg1, arg2 );
}










/* Emacs editing section. DW 94.07.19 */

/*
Local Variables:
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
