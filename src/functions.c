/**************************************************************
 *                                                            *
 *                            NEXUS                           *
 *                                                            *
 *              (c) 1990 Paul Sajda and Leif Finkel           *
 *                                                            *
 *                      (nexus_functions.c)                   *
 *                                                            *
 *      modified for Parallel Nexus.  11/8/91 by K.Sakai      *
 *                                    12/27/91	              *
 *                                                            *
 *     modified for new PGN syntax.   1/24/92 by P. Sajda     *
 *                                                            *
 **************************************************************/


/*
 *  This is the transfer function specification file.  A user who wishes
 *    to create PGN cells must define the input and output instructions
 *    (i.e. write the do_pgn_functions() function) in their own
 *    pgn_functions.c file and subsequently compile and link that file
 *    into a new version of Nexus.  See the manual for details, and
 *    examine pgn_functions.c for an example.
 */

#include "nexus.h"
#include "nexus_pgn_includes.h"

#include "functions.h"

#include "stdlib.h"

#ifdef NEXUS_RBF
#include "rbf.h"
#endif /* NEXUS_RBF */




/*
 *  update_cell_activity():
 *
 *  This is the main cell calculation engine.
 *
 *
 *  Built-in transfer functions (for externals, see nexus_pgn_functions.c):
 *
 *    rbf_gaussian      RBF Gaussian
 *    rbf_logistic      RBF Logistic
 *    simple            Simple / Sigmoid
 *    linear            Linear
 *    binary            Binary / Step
 *    ln                Logarithmic
 *    exp		Exponential
 *    energy		Square of input      95.03.30
 *
 *    pgn		Programmable (see pgn_functions.c or user's pgn file)
 *
 *                    transfer function = pgn ( pgn_function_name , arg1 ... );
 *
 *    If you wish to add more built-in functions, make sure you add
 *      the parser in lex_build.l (one for arguments, and one for
 *      no arguments: make sure to supply default values), and make
 *      sure you add the interpreter in main.c, the set_trans_func()
 *      routine.  Also, the format for a function without arguments,
 *      in the .nx file, is "transfer function = func_name;".  Note
 *      the lack of parentheses.
 */

int
update_cell_activity( cell, function, network, swap_flag )
    register CELL	cell;
    register char	*function;
    register NETWORK	network;
    register int	swap_flag;
{
  register int		count,
  			num_connections = cell->number_connections;
  register CONNECTION	connection;
  char			pgn_func[FUNCTION_SIZE];
  char			char1 = *function;
  float			curr_voltage;

  extern int	do_pgn_functions();
  CONNECTION	get_net_connection();
  NETWORK	get_network_name();



  /*
   *  DW 95.01.21  Added the first character (char1) checks to increase
   *    lookup efficiency.
   */


#ifdef NEXUS_RBF

  /*
   *  RBF Gauss-ian transfer function
   */

  if (char1 == 'r' && strcmp( function, "rbf_gaussian" ) == 0) {
    if (rbf_parameters_allocate == TRUE) {
      return update_cell_rbf_gaussian( cell, network );
    }
    else {
      printf( "RBF parameters were not allocated\n" );
      return ERROR;
    }
  }

#endif





  /***************************************************************************
   *  The following computations are common to the rest of the transfer
   *    functions.  SY 95.01.19
   *  Deferred assignment of cell->voltage until end of this section, to
   *    take the dereferencing of cell-> out of the loops.  DW 95.01.20
   *  The code might look suspiciously redundant, but keeping the swap_flag
   *    check outside of the loop saves time.  Remember, this function is
   *    called for each cell, every cycle.
   *  Bug Fix: DW 94.08.19, cell->voltage *= network->decay used to be
   *    evaluated twice when swap_flag was ON.
   */

  curr_voltage = 0.0;
  connection = cell->connect_list;

  if (swap_flag == ON) {
    for (count = 0; count < num_connections; count++) {
      curr_voltage += (connection->conductance
		       * connection->input_cell->firing_rate_old);
      connection++;
    }
  }
  else {
    for (count = 0; count < num_connections; count++) {
      curr_voltage += (connection->conductance
		  * connection->input_cell->firing_rate);
      connection++;
    }
  }

  cell->voltage = curr_voltage * network->decay;

  /*
   *  End of common computations section.
   ***************************************************************************/





#ifdef NEXUS_RBF

  /*
   *  RBF logistic transfer function
   */

  if (char1 == 'r' && strcmp( function, "rbf_logistic" ) == 0) {
    if (rbf_parameters_allocate == TRUE) {
      return update_cell_rbf_logistic( cell, network );
    }
    else {
      printf("RBF parameters were not allocated\n");
      return ERROR;
    }
  }

#endif /* NEXUS_RBF */





  /*
   *  Simple transfer function.  Took out redundant algebra to increase
   *    calculation speed (old code is inside #if 0/#endif).  DW 94.08
   *  'simple' is the historic name for 'sigmoid'.
   */

  if (char1 == 's' && strcmp(function, "simple") == 0) {
    cell->firing_rate = network->function_min
      + (network->function_max - network->function_min) /
	(1 + exp((cell->threshold - cell->voltage)
		 * network->function_slope));

#if 0
    cell->firing_rate = (((1 / (1 + exp(-(cell->voltage - cell->threshold)
					* network->function_slope)))
			  * (network->function_max - network->function_min))
			 + network->function_min);
#endif /* 0 */
  }





  /*
   *  Linear transfer function.
   */

  else if (char1 == 'l' && strcmp(function, "linear") == 0) {
    cell->firing_rate =
      (cell->voltage - cell->threshold) * network->function_slope;

    /*  clamp to min / max  */
    if (cell->firing_rate < network->function_min)
      cell->firing_rate = network->function_min;
    if (cell->firing_rate > network->function_max)
      cell->firing_rate = network->function_max;
  }





  /*
   *  Binary transfer function. ('binary' is the historic name for 'step').
   */

  else if (char1 == 'b' && strcmp( function, "binary" ) == 0) {
    if (cell->voltage >= cell->threshold)
      cell->firing_rate = network->function_max;
    else
      cell->firing_rate = network->function_min;
  }





  /*
   *  Logarithmic transfer function.
   */

  else if (char1 == 'l' && strcmp( function, "ln" ) == 0) {
    if (cell->voltage >= cell->threshold)
      cell->firing_rate = log( network->function_slope
			      * (cell->voltage - cell->threshold
				 + 1/(network->function_slope)));
    else
      cell->firing_rate = 0.0;
  }





  /*
   *  Exponential transfer function.
   */

  else if (char1 == 'e' && strcmp( function, "exp" ) == 0) {
    if (cell->voltage >= cell->threshold)
      cell->firing_rate =
	-exp(-(cell->voltage
	       - (cell->threshold + log( network->function_slope ))))
	  + network->function_slope;
    else
      cell->firing_rate = 0.0;
  }





  /*
   *  Energy (square of input) transfer function.
   *    DW 95.03.30  Added to built-in function list.
   */

  else if (char1 == 'e' && strcmp( function, "energy" ) == 0) {
    cell->firing_rate =
      (cell->voltage >= cell->threshold ?
       (cell->voltage) * (cell->voltage) : 0.0);
  }





  /*
   *  PGN (externally defined) transfer function.  These functions are
   *    defined in pgn_functions.c, and nexus must be re-compiled
   *    if the user wishes to use their own pgn functions.
   *
   *  DW 95.01.20  Bug fix.  If pgn function does not exist, stop
   *    immediately.  Otherwise nexus will continue simulating without
   *    meaningful results.
   */

  else if (char1 == 'p') {
    /*  Convert '-' to '_' */
    sscanf( function, "%*[^-]%*c%s", pgn_func );

    if (do_pgn_functions( cell, pgn_func, network ) == ERROR) {
      fprintf( stderr, "ABORTing.  Recompile with correct pgn functions.\n" );
      exit( ERROR );
    }
  }





  /*
   *  If none of the above, print error message.  KS 91.11.11
   */

  else {
    fprintf( stderr, " ERROR: No such transfer function '%s'.\n", function );
    return ERROR;
  }





  /*
   *  Common closure.
   */
  cell->firing_rate = network->offset + (network->scale * cell->firing_rate);
  return OK;
}



/*
 * is input between one and other, inclusive?
 * altered to handle one being greater than other.
 * made into MACRO instead of FUNCTION to reduce
 * significant overhead
 * DW 94.07.13
 */

#define between(input, one, other) \
	( ( (input) >= (one) && (input) <= (other) ) \
	 || ( (input) <= (one) && (input) >= (other) ) ? TRUE : FALSE )



/* is a value between min and max (inclusive) */
/* between(input, min,max)
 * float input, min, max;
 * {
 *   if(input >= min && input <= max)
 *     return(TRUE);
 *   else
 *     return(FALSE);
 * }
 */



/*
 *  Return TRUE if cell is at the edge of a network, FALSE otherwise.
 */

int
not_at_edge( cell_id, net_id, name )
    int cell_id, net_id;
    char *name;
{
  NETWORK get_network_id(),get_network_name();



  if((cell_id % get_network_name(name)->dim_x) != 0 &&
     (cell_id % get_network_id(net_id)->dim_x) != 1 &&
     cell_id > get_network_id(net_id)->dim_x &&
     cell_id < ((get_network_id(net_id)->dim_x * 
		 get_network_id(net_id)->dim_y)- 
		get_network_id(net_id)->dim_x))
    return TRUE;
  else
    return FALSE;
}



/*
 *  From the old days, for backward compatibility.  DW 94.09.02
 */

#define get_nearest_neighboor(aaa,bbb,ccc) \
	get_nearest_neighbor((aaa),(bbb),(ccc))



CELL
get_nearest_neighbor( input_cell, net_head2, id )   /* rev. 1/3/92 */
    CELL		input_cell;
    NETWORK		net_head2;
    register int	id;
{							     
  int delta_cell;                                                  
  /* 41 42 43 44 45 46 47 
     39 19 20 21 22 23 40
     37 17  5  6  7 18 38
     35 15  3  x  4 16 36
     33 13  0  1  2 14 34
     31  8  9 10 11 12 32
     24 25 26 27 28 29 30 */



  /*
   *  Changed multiple if's to switch() statement.  DW 94.11.15
   */

  switch (id) {

  case 0:
    delta_cell = (- net_head2->dim_x - 1); break;
  case 1:
    delta_cell = ( - net_head2->dim_x); break;
  case 2:
    delta_cell = (- net_head2->dim_x + 1); break;
  case 3:
    delta_cell = (- 1); break;
  case 4:
    delta_cell = (1); break;
  case 5:
    delta_cell = (net_head2->dim_x - 1); break;
  case 6:
    delta_cell = (net_head2->dim_x); break;
  case 7:
    delta_cell = (net_head2->dim_x + 1); break;
  
  /* rev. 1/3/92 */
  case 8:
    delta_cell = (- 2*(net_head2->dim_x) - 2); break;
  case 9:
    delta_cell = (- 2*(net_head2->dim_x) - 1); break;
  case 10:
    delta_cell = (- 2*(net_head2->dim_x) ); break;
  case 11:
    delta_cell = (- 2*(net_head2->dim_x) + 1); break;
  case 12:
    delta_cell = (- 2*(net_head2->dim_x) + 2); break;

  case 13:
    delta_cell = ( - net_head2->dim_x - 2); break;
  case 14:
    delta_cell = (- net_head2->dim_x + 2); break;
  case 15:
    delta_cell = (- 2); break;
  case 16:
    delta_cell = (2); break;
  case 17:
    delta_cell = (net_head2->dim_x - 2); break;
  case 18:
    delta_cell = (net_head2->dim_x + 2); break;
  
  case 19:
    delta_cell = (2*(net_head2->dim_x) - 2); break;
  case 20:
    delta_cell = (2*(net_head2->dim_x) - 1); break;
  case 21:
    delta_cell = (2*(net_head2->dim_x) ); break;
  case 22:
    delta_cell = (2*(net_head2->dim_x) + 1); break;
  case 23:
    delta_cell = (2*(net_head2->dim_x) + 2); break;
  
  /* rev. 6/5/92 */
  case 24:
    delta_cell = (- 3*(net_head2->dim_x) - 3); break;
  case 25:
    delta_cell = (- 3*(net_head2->dim_x) - 2); break;
  case 26:
    delta_cell = ( - 3*(net_head2->dim_x) - 1); break;
  case 27:
    delta_cell = (- 3*(net_head2->dim_x)); break;
  case 28:
    delta_cell = (- 3*(net_head2->dim_x) + 1); break;
  case 29:
    delta_cell = (- 3*(net_head2->dim_x) + 2); break;
  case 30:
    delta_cell = ( - 3*(net_head2->dim_x) + 3); break;

  case 31:
    delta_cell = (- 2*net_head2->dim_x - 3); break;
  case 32:
    delta_cell = (- 2*net_head2->dim_x + 3); break;
  case 33:
    delta_cell = (- net_head2->dim_x - 3); break;
  case 34:
    delta_cell = (- net_head2->dim_x + 3); break;
  case 35:
    delta_cell = (- 3); break;
  case 36:
    delta_cell = (3); break;
  case 37:
    delta_cell = (net_head2->dim_x - 3); break;
  case 38:
    delta_cell = (net_head2->dim_x + 3); break;
  case 39:
    delta_cell = (2*net_head2->dim_x - 3); break;
  case 40:
    delta_cell = (2*net_head2->dim_x + 3); break;

  case 41:
    delta_cell = (3*(net_head2->dim_x) - 3); break;
  case 42:
    delta_cell = (3*(net_head2->dim_x) - 2); break;
  case 43:
    delta_cell = (3*(net_head2->dim_x) - 1); break;
  case 44:
    delta_cell = (3*(net_head2->dim_x)); break;
  case 45:
    delta_cell = (3*(net_head2->dim_x) + 1); break;
  case 46:
    delta_cell = (3*(net_head2->dim_x) + 2); break;
  case 47:
    delta_cell = (3*(net_head2->dim_x) + 3); break;

  default:
    delta_cell = 0;
  }


  /*
   *  Make sure we are pointing to a valid cell!
   */
  if (input_cell->id + delta_cell > 0 &&  /* rev. 12/18/92 */
      input_cell->id + delta_cell <= net_head2->number_cells)
    return (input_cell + delta_cell);
  
  return NULL;
}


CONNECTION
get_net_connection( name, head, number )
    register char	*name;
    register CONNECTION	head;
    register int	number;
{
  register int		count;


  for (count = 0; count < number; count++)
    if (strcmp( name,
	       (*(network_xref_list
		  + (head + count)->input_cell->net_id))->name	) == 0)
      return (head + count);

  printf( "ERROR - no such network %s - ABORT(1)\n", name );
  return NULL;
}



NETWORK
get_network_id( net_id )
    register int	net_id;
{
#if 0
  NETWORK         head2;
  
  head2 = network_head;
  while (head2 != NULL && head2->id != net_id)
    head2 = head2->next;
  if (head2 == NULL) {
    printf( "ERROR - no such network %d - ABORT(2)\n", net_id );
    return NULL;
  }
  
  return head2;
#else
  return (*(network_xref_list + net_id)); /* DW */
#endif /* 0 */
}



NETWORK
get_network_name( name )
    char *name;
{
  NETWORK         head2;

  
  head2 = network_head;
  while (head2 && strcmp( head2->name, name ) != 0)
    head2 = head2->next;

  return head2;
}



char *
get_name_of_network( net_id )
    int net_id;
{
#if 0
  NETWORK         head2;
  
  head2 = network_head;
  while (head2 != NULL && head2->id != net_id)
    head2 = head2->next;
  if (head2 == NULL) {
    printf("ERROR - no such network id %d - ABORT(4)\n", net_id);
  }

  return (head2->name);
#else /* 0 */
  return ((*(network_xref_list + net_id))->name);
#endif /* 0 */
}



int
determine_pgn_function( pgn_function, function_name )
    register char *pgn_function, *function_name;
{
  static char name[FUNCTION_ARG_SIZE];
  
  get_argument( pgn_function, 0, name );
  return (strcmp( name, function_name ) == 0);
}



/* function which assigns the argument `returned_argument` the value (string)
   associated with the `number`th argument
*/
void
get_argument(function, number, returned_argument)
    char *function;
    int number;
    char *returned_argument;
{
  char temp_function[FUNCTION_SIZE];
  char argument[FUNCTION_ARG_SIZE];
  int count;

  if(number<0){
    printf("ERROR -- argument number %d does not exist\n",number);
    return;
  }
  strcpy(temp_function,function);
  for(count=0;count<=number;count++){
    if(sscanf(temp_function,"%[^,]%*c%s",argument,temp_function)==0){
      printf("ERROR -- no such argument\n");
      return;
    }
  }
  strcpy(returned_argument,argument);
  
  return;
}

/*
 *  get_current_cycle() moved to nexus_run.c to prevent globalizing the
 *    variable count_cycles.  DW 94.10.10
 */

CELL
get_cell_at_position(network_name,x,y)
    char *network_name;
    int x,y;
{
  NETWORK network;
  CELL network_cells;
  
  network=get_network_name(network_name);
  
  network_cells = network->cells;
  
  if(x >= network->dim_x || y >= network->dim_y){
    printf("ERROR - location %d %d OUT OF BOUNDS\n",x,y);
    return(NULL);
  }
  
  return(network_cells + network->dim_x*y + x);
}

/*
 *  DW 94.07.14:  cell_position_? routines replaced as follows:
 *
 *    cell_position_x( int cell, int dim_x, int dim_y ) ...
 *      is now ----> (cell_id - 1) % head->dim_x;
 *    where head points to the NETWORK which holds this cell.
 *
 *    cell_position_y( int cell, int dim_x, int dim_y ) ...
 *      is now ----> (cell_id - 1) / head->dim_x;
 *    since integer division truncates the fractional part.
 *
 *  DW 94.08.18:  cell_position_? routines are now #define's similar to above
 *    code for backward PGN function compatibility.  See nexus_pgn_includes.h.
 */






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
