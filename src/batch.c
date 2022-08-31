/**************************************************************
 *                                                            *
 *                            NEXUS                           *
 *                                                            *
 *                      (nexus_batch.c)                       *
 *                                                            *
 *      Created to remove time stress during simulation       *
 *             caused by file storage media.                  *
 *                        DW 94.11.05                         *
 *                                                            *
 **************************************************************/

#include <stdlib.h>

#include "nexus.h"
#include "batch.h"


/**
 **  Global variables.
 **/

int             LOAD_MARKER				= 0;
int             END_OF_BATCH_FILES			= FALSE;

static FILE *	batchFilePtr[NUM_BAT_FILES]		= { NULL, NULL };
static char	batchPrevFilename[NUM_BAT_FILES][NAME_SIZE] = { "", "" };
static int	currBatchIndex[NUM_BAT_FILES]		= { 0, 0 };
static NETWORK	batchDefaultNetwork			= NULL;

/*
 *  The following are updated by the User Interface.
 *  (Graphical UI's: XView, Tk)
 *  (Scripting UI: Tcl)
 */

#if NUM_BAT_FILES == 1
BATCHLIST	currBatchList[NUM_BAT_FILES]		= NULL;
int		currBatchListLength[NUM_BAT_FILES]	= 0;
#elif NUM_BAT_FILES == 2
BATCHLIST	currBatchList[NUM_BAT_FILES]		= { NULL, NULL };
int		currBatchListLength[NUM_BAT_FILES]	= { 0, 0 };
#else
#  error "wrong NUM_BAT_FILES"
#endif

char		filename_load_activity[NAME_SIZE]	= "";
char		network_load_activity[NAME_SIZE]	= "";










/***********
 *  nexus_batch.c
 ***********/

int
readBatchFile( batchType, filename )
    batch_t batchType;
    char * filename;
{
  BATCHLIST	currPtr;
  char		loadArgs[FUNCTION_SIZE];
  int		count;



  printf( "Reading in batch file '%s' . . .\n", filename );
  if (!(batchFilePtr[batchType] = nxFopen( filename )))
    return ERROR;

  for (currBatchListLength[batchType] = 0;
       (fscanf( batchFilePtr[batchType], "%[^\n]%*c", loadArgs )) > 0;
       currBatchListLength[batchType]++);
  rewind( batchFilePtr[batchType] );

  if (currBatchListLength[batchType] == 0) {
    fprintf( stderr, "ERROR: Batch file is empty or corrupt.\n" );
    fprintf( stderr, "  Turning off cycle loading.\n" );
    set_flag( ACTIVITY_LOAD, OFF );
    return OK;
  }

  if (currBatchList[batchType])
    free( currBatchList[batchType] );
  if (!(currBatchList[batchType] = (BATCHLIST)
	calloc( currBatchListLength[batchType],
	       sizeof( ELEMENT_BATCHLIST ) ))) {
    fprintf( stderr, "FATAL: Memory allocation failure, readBatchFile.\n" );
    exit(1);
  }

  for (count = 0; count < currBatchListLength[batchType]; count++) {
    currPtr = currBatchList[batchType] + count;
    fscanf( batchFilePtr[batchType], "%[^\n]%*c", loadArgs );
    currPtr->numArgs = sscanf( loadArgs, "%s %s",
			      currPtr->filename,
			      currPtr->networkName );
    currPtr->flagPicked = FALSE;
  }

  fclose( batchFilePtr[batchType] );

  return OK;
}  /* readBatchFile( ) */



/*
 *  Call this before using load_activity_cycle( ).  Took this out of
 *    load_activity_cycle( )  to eliminate passing the cycle # in to
 *    it.  Also standardized run_simulation( ).  DW 94.11.16
 */

int
init_activity_cycle( )
{
  extern NETWORK	get_network_name( );
  register int		i;



  /*
   *  If no previous batch file was opened, or if the last batch file was
   *    different from the one currently desired, (re)initialize the file
   *    pointer.  Also intialize on the first cycle of every new simulation.
   */

  if (!currBatchList[NXBATCH_IN] ||
      (strcmp( batchPrevFilename[NXBATCH_IN], filename_load_activity ) != 0)) {

    if (readBatchFile( NXBATCH_IN, filename_load_activity )) {
      printf( "ERROR: Can not open Input Batch File '%s'.\n",
	     filename_load_activity );
      printf( "  Turning off cycle loading, and ABORTing simulation.\n\n" );
      set_flag( ACTIVITY_LOAD, OFF );
      return ERROR;
    }
    strcpy( batchPrevFilename[NXBATCH_IN], filename_load_activity );

  }

  if (!(batchDefaultNetwork = get_network_name( network_load_activity ))) {
    printf( "ERROR: No such network '%s'.\n", network_load_activity );
    printf( "  Please check 'Network name' field of Load Activity menu.\n" );
    printf( "  Turning off cycle loading, but continuing simulation.\n\n" );
    set_flag( ACTIVITY_LOAD, OFF );
    return OK;
  }

  for (i = 0; i < currBatchListLength[NXBATCH_IN]; i++)
    (currBatchList[NXBATCH_IN] + i)->flagPicked = FALSE;
  currBatchIndex[NXBATCH_IN] = 0;

  return OK;
}



/*
 *  This gets called every update cycle.
 *  Force display update on last cycle.  DW 94.09.09
 *  Replaced load_history[LOAD_FILES] and load_history[LOAD_COUNT] with
 *    their respective global variables, currBatchListLength and
 *    currBatchIndex.  Replaced load_history[] array functionality with
 *    structure element currBatchList->flagPicked.  DW 94.11.17
 */

int
load_activity_cycle( )
{
  FILE		*fp;
  NETWORK	head;
  char		currFilename[NAME_SIZE];
  char		currNetworkName[NAME_SIZE]; /* modified 10/12/92 by K.S. */
  int		i, index;
  register int	num;


  /*
   *  Tell learning methods we're at end of batch learning file.  Time
   *    to update connections...
   */
  if (query_flag( LEARN_BATCH ) &&
      currBatchIndex[NXBATCH_IN] == currBatchListLength[NXBATCH_IN] - 1)
    END_OF_BATCH_FILES = TRUE;

  if (query_flag( LOAD_RANDOM )) {

    if (currBatchIndex[NXBATCH_IN] == currBatchListLength[NXBATCH_IN]) {
      for (i = 0; i < currBatchListLength[NXBATCH_IN]; i++)
	(currBatchList[NXBATCH_IN] + i)->flagPicked = FALSE;
      currBatchIndex[NXBATCH_IN] = 0;
    }

    if (currBatchIndex[NXBATCH_IN] == currBatchListLength[NXBATCH_IN] - 1) {
      for (i = 0; (currBatchList[NXBATCH_IN] + i)->flagPicked; i++);
      num = i;
    }

    else
      do
	num = ((int) get_random( (float) 0.0,
				(float) currBatchListLength[NXBATCH_IN]
				* 100.0 )
	       % currBatchListLength[NXBATCH_IN]);
      while ((currBatchList[NXBATCH_IN] + num)->flagPicked);


    index = LOAD_MARKER = num;
    (currBatchList[NXBATCH_IN] + num)->flagPicked = TRUE;
  }
  else {			/*  SEQUENTIAL CYCLE LOADING  */
    if (currBatchIndex[NXBATCH_IN] == currBatchListLength[NXBATCH_IN])
      currBatchIndex[NXBATCH_IN] = 0;
    index = currBatchIndex[NXBATCH_IN];
  }

  strcpy( currFilename, (currBatchList[NXBATCH_IN] + index)->filename );
  strcpy( currNetworkName, (currBatchList[NXBATCH_IN] + index)->networkName );

  /*
   *  If two arguments are supplied, the first is the filename and
   *    the second is the network.  Check the validity of the network.
   *
   *  Otherwise, only the filename is supplied.  The network is chosen
   *    in the Load Activity menu.
   */

  if ((currBatchList[NXBATCH_IN] + index)->numArgs == 2) {
    if (!(head = get_network_name( currNetworkName ))) {
      printf( "No such network '%s'; using DEFAULT network '%s' for load\n",
	     currNetworkName, batchDefaultNetwork->name );
      head = batchDefaultNetwork;
    }
  }
  else
    head = batchDefaultNetwork;

  if (!(fp = nxFopen( currFilename ))) {
    printf( "ERROR - file %s returns (nil)\n", currFilename );
    printf( "  Turning off cycle loading and ABORTing simulation.\n" );
    set_flag( ACTIVITY_LOAD, OFF );
    return ERROR;
  }
  else {
    printf( "Loading network '%s' from file '%s'\n",
	   head->name, currFilename );
    load_activity_file( fp, head );
    fclose( fp );
  }

  currBatchIndex[NXBATCH_IN]++;

  return OK;
}  /* load_activity_cycle( ) */



/*
 *  getBatchActivityFile()  DW 95.01.27
 *
 *  Accesses the filenames in currBatchList manually.
 *  Returns OK or ERROR status.
 *
 *  index: index into list, starting at: 0 = first activity file.
 *
 *  network: network to load into.  If this is NULL, use the network
 *    specified in the batch file.  If that is also null, use the
 *    default (network_load_activity).
 *
 *  filename: char array to store the accessed activity file's name.
 *    If this is NULL, do not bother.
 */

int
getBatchActivityFile( batchType, index, network, filename )
    batch_t batchType;
    int index;
    char * network, * filename;
{
  FILE *	batchActivityFilePtr;
  BATCHLIST	batchIndex;


  if (index > currBatchListLength[batchType])
    return ERROR;

  batchIndex = currBatchList[batchType] + index;

  if (!(batchActivityFilePtr = nxFopen( batchIndex->filename )))
    return ERROR;

  if (network) {
    load_activity_file( batchActivityFilePtr,
		       get_network_name( network ));
  }
  else if (batchIndex->numArgs == 2) {
    load_activity_file( batchActivityFilePtr,
		       get_network_name( batchIndex->networkName ) );
  }
  else {
    load_activity_file( batchActivityFilePtr,
		       get_network_name( network_load_activity ) );
  }

  /*
   *  If a (char *) is passed in the fourth parameter, store the current
   *    filename in that space.
   */

  if (filename) {
    strcpy( filename, batchIndex->filename );
  }

  return OK;
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
