/**************************************************************
 *                                                            *
 *                            NEXUS                           *
 *                                                            *
 *              (c) 1990 Paul Sajda and Leif Finkel           *
 *                                                            *
 *                      (nexus_memory.c)                      *
 *                         DW 95.01.22                        *
 *                                                            *
 **************************************************************/

#include <stdlib.h>

#include	"nexus.h"
#include	"memory.h"


#ifdef NEXUS_BP
#include	"bp.h"
#endif

#ifdef NEXUS_INVERSION
#include	"inversion.h"
#endif

#ifdef NEXUS_RBF
#include	"rbf.h"
#endif



/*
 *  NOTE: all of the make_* functions are provided here ONLY for backward
 *    compatibility.  These have all been replaced by appropriate calls to
 *    either nxMalloc() or nxCalloc().  DW 95.04.02
 *
 *  make_connect_specs():
 *  Create SPECS list.  This structure stores the general location of
 *    connection emanating FROM a particular cell.  This increases the speed of
 *    determining forward connectivity since only backward pointers are stored
 *    in the simulation.
 *
 *  make_connection_un():
 *  Create malloc() connections. This is a temporary, UNcompressed listing of
 *    cellular connections.  After simulation connectivity has been fully
 *    established, this malloc() linked list will be freed, and the data stored
 *    in a calloc() array.  This reduces the simulation data size and
 *    increases processing speed by reducing disk swaps.  Initially this list
 *    is NULL for each cell.
 *
 *  make_connections():
 *  Final resting place of (compressed) connection data.
 *  Bug Fix: on SGIs, calloc(0,size_t) returns NULL.  On SUN's calloc(0,size_t)
 *    merely returns a valid pointer to an empty, albeit allocated, space.
 *    Now we return NULL if number was 0, not a valid pointer.  DW 94.11.14
 *
 *  make_extern_connection():
 *  Create "network->external_connection" to store info of external
 *    connections
 *  
 *  make_nexus():
 *  Create structure to store Nexus names and their file write directory
 *
 *  make_parameters():
 *  Create structure to store parameters for connectivity of networks.  this
 *    is a temporary structure and will be released by free() after connections
 *    have been made.
 *
 *  make_weight_array():
 *  Create array to store user defined connections located in a file
 *
 *  make_weight_list():
 *  Create list which will hold weights specified in a particular file
 */

CELL
make_cells( number )
    unsigned number;
{
  return (CELL) nxCalloc( number, SIZE_CELL );
}

SPECS
make_connect_specs( )
{
  return (SPECS) nxMalloc( SIZE_SPECS );
}

CONNECTION_UN
make_connection_un( )
{
  return (CONNECTION_UN) nxMalloc( SIZE_CONNECTION_UN );
}

CONNECTION
make_connections( number )
    int number;
{
  return (CONNECTION) nxCalloc( number, SIZE_CONNECTION );
}

EXTERN_CONNECTION
make_extern_connection( )
{
  return (EXTERN_CONNECTION) nxMalloc( SIZE_EXTERN_CONNECTION );
}

NETWORK
make_network( )
{
  return (NETWORK) nxMalloc( SIZE_NETWORK );
}

NEXUS
make_nexus( )
{
  return (NEXUS) nxMalloc( SIZE_NEXUS );
}

PARAMETERS
make_parameters( )
{
  return (PARAMETERS) nxMalloc( SIZE_PARAMETERS );
}

WEIGHT_ARRAY
make_weight_array( )
{
  return (WEIGHT_ARRAY) nxMalloc( SIZE_WEIGHT_ARRAY );
}

float *
make_weight_list( number )
    int number;
{
  return (float *) nxCalloc( number, sizeof( float ) );
}


/*****************************************************************************/

/*
 *  Make any type, similar to malloc( ) and calloc( ).  DW 94.11.15
 *    Return value must be typecast.
 */

void *
nxMalloc( size )
    size_t size;
{
  void	*pointer;


  if (size == 0) {
    fprintf( stderr, "WARNING: nxMalloc( size = 0 )\n" );
    return NULL;
  }

  if ((pointer = (void *) malloc( size )))
    return pointer;

  fprintf( stderr, "FATAL: Memory allocation failure.  nxMalloc( ).\n" );
  exit(1);
}  /* nxMalloc( ) */



void *
nxCalloc( nobj, size )
    size_t nobj, size;
{
  void	*pointer;


  if (size == 0 || nobj == 0) {
    fprintf( stderr, "WARNING: nxCalloc( size or nobj = 0 )\n" );
    return NULL;
  }

  if ((pointer = (void *) calloc( nobj, size )))
    return pointer;

  fprintf( stderr, "FATAL: Memory allocation failure.  nxCalloc( ).\n" );
  exit(1);
}  /* nxCalloc( ) */



/*
 *  Create network xref list.  DW 94.07.13
 */

void
make_xref_list( )
{
  NETWORK	*xref_list;
  NETWORK	head;
  int		curr_id;


  if (network_xref_list)
    free( network_xref_list );

  xref_list = (NETWORK *) nxCalloc( number_networks + 1, sizeof( NETWORK * ) );

  for (head = network_head, curr_id = 1; head; head = head->next)
    *(xref_list + curr_id++ ) = head;

  /*  Set main global variable.  */
  network_xref_list = xref_list;
}



void
add_xref_list( network )
    NETWORK network;
{
  NETWORK *xref_list;
  int i;


  xref_list = (NETWORK *) nxCalloc( number_networks + 1, sizeof( NETWORK * ) );
  
  for( i = 1; i < number_networks; i++ )
    *(xref_list + i) = *(network_xref_list + i);
  
  *(xref_list + number_networks) = network;

  /*  Update main global variable.  */
  free( network_xref_list );
  network_xref_list = xref_list;
}



/*****************************************************************************
 *  Routines to clear networks from memory
 *****************************************************************************/

void
freeLearningMethods( )
{
#ifdef NEXUS_BP
  bpFreeParam( );
#endif

#ifdef NEXUS_RBF
  rbfFreeParam( );
#endif

#ifdef NEXUS_INVERSION
  invFreeParam( );
#endif
}



/*
 *  DW 95.03.07  Bug fix: free_parameters() BEFORE free( head ).  Used to
 *    cause Bus error with large networks.
 *  DW 95.04.02  Changed to iterative procedure (from recursive)
 */

void
free_parameters( head )
    register PARAMETERS head;
{
  register PARAMETERS next;

  /*  fprintf(stderr, "free %p\n", head); /* DEBUG */
  /** kinnkyuuhinan **/
  if(head != NULL)
    head = NULL;
  /** kinnkyuuhinan **/

  while (head) {
    next = head->next;
    /*    head->next = 99898989; /* DEBUG */
    free( head );
    head = next;
    /*    fprintf(stderr, " free %p\n", head); /* DEBUG */
  }
}



/*
 *  The following are not currently used...
 */

void
clear_simulation( )
{
  char            ans[4];

  
  printf("WARNING - Simulation will be removed from active memory!!\n");
  printf("\nDo yo want to proceed in clearing the simulator? (y/n) ");
  scanf("%s", ans);
  if (ans[0] == 'y') {
    free_network( network_head );
    network_head = NULL;
    printf( "\nSIMULATOR CLEARED\n" );
  }
  else
    printf( "CLEAR IGNORED \n" );
}



void
free_network( head )
    NETWORK         head;
{
  NETWORK         next;
  CELL            cells;
  int             number, id;

  
  if (head) {
    cells = head->cells;
    next = head->next;
    number = head->number_cells;
    id = head->id;

    free( head );
    
    free_cells( number, 0, cells );
    free( cells );    /* cfree () -> free (), compatibility DW 95.02.24 */
    printf("Network# %d cleared \n", id);
    free_network(next);
  }
}



void
free_cells( number, count, head )
    CELL            head;
    int             number, count;
{
  if (count < number) {
    free_parameters(head->connect_parameters);
    free_cells(number, count + 1, head + 1);
  }
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
