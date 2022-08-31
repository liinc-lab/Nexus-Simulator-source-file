/**************************************************************
 *                                                            *
 *                           NEXUS                            *
 *                                                            *
 *             (c) 1990 Paul Sajda and Leif Finkel            *
 *                                                            *
 *                        (nexus_bp.c)                        *
 *                                                            *
 **************************************************************/

#include "nexus.h"
#include "bp.h"
#include "batch.h"
#include "tcl_main.h"

#ifdef NEXUS_SGI
#include "nexus_tk.h"
#endif

#ifdef NEXUS_LINUX
#include "nexus_tk.h"
#endif

#ifdef NEXUS_SUN
#include <nexus_xview.h>
#include <nexus_xview_functions.h>
#endif


BP_NETWORK		bp_network_head;
int			bp_parameters_allocate = FALSE;
float			bpLearningRate = 0.0,
			bpParaMomentum = 0.0;
char			bpBatchFilename[NAME_SIZE],
  			bpErrorFilename[NAME_SIZE],
			bpHiddenNetwork[NAME_SIZE],
			bpOutputNetwork[NAME_SIZE];



static BP_NETWORK	bpAllocNetwork _ANSI_ARGS_(( void ));
static BP_CELL		bpAllocCells _ANSI_ARGS_(( unsigned number ));
static BP_CONNECT	bpAllocConnections _ANSI_ARGS_(( unsigned number ));
static FILE		*bp_save_error_fp;



#ifdef NEXUS_SUN
Frame			bp_frame;
Panel_item		chbox_bp,
			chbox_save_err_bp,
			bp_network_hidden_list,
			bp_network_output_list;

static Panel		bp_panel;
#endif







char *
bpAllocParam( )
{
  NETWORK	head;
  BP_NETWORK	bp_head = NULL,
  		old_bp_head;
  BP_CELL	cell;
  int		count_cells;


  if (bp_parameters_allocate) {
    if (!bpErrorQuery( "Backprop parameters already allocated.  Reallocate?" ))
      return "Aborting re-allocation.";
    fprintf( stderr, "Re-allocating Back Propagation parameters..." );

    bpFreeParam( );
  }
  else
    fprintf( stderr, "Allocating Back Propagation parameters..." );

  if (strlen( bpHiddenNetwork ) == 0)
    return "Hidden Network is not specified.";
  if (strlen( bpOutputNetwork ) == 0)
    return "Output Network is not specified.";

  /*
   *  Find hidden network.
   *
   *  Load network information into bp_network structures, starting
   *    with the Hidden Network layer and working down to the Output
   *    Network layer.
   */

  if (!(head = network_head))
    return "No networks have been loaded.";
  while (head && (strcmp( head->name, bpHiddenNetwork ) != 0))
    head = head->next;
  if (!head)
    return "The Hidden Network specified does not exist.";

  /*
   *  Find output network.
   *
   *  Caveat: we assume the networks between the two (Hidden and Output),
   *    if there are any, are simply multiple hidden layers.  Errors
   *    are propagated all the way from the Output network back to the
   *    *first* Hidden network.
   */

  while (head && (strcmp( head->name, bpOutputNetwork ) != 0)) {

    old_bp_head		= bp_head;
    bp_head		= bpAllocNetwork( );
    bp_head->id		= head->id;
    bp_head->bp_cells	= bpAllocCells( head->number_cells );
    bp_head->net_ptr	= head;
    bp_head->next	= old_bp_head;

    for (count_cells = 0; count_cells < head->number_cells; count_cells++) {
      cell			= bp_head->bp_cells + count_cells;
      cell->id			= (head->cells + count_cells)->id;
      cell->cell_ptr		= (head->cells + count_cells);
      cell->connect_list	=
	bpAllocConnections( (head->cells + count_cells)->number_connections );
    }
    
    head = head->next;
  }
  if (!head)
    return "The Output Network specified does not exist.";
  
  old_bp_head		= bp_head;
  bp_head		= bpAllocNetwork( );
  bp_head->id		= head->id;
  bp_head->bp_cells	= bpAllocCells( head->number_cells );
  bp_head->net_ptr	= head;
  bp_head->next		= old_bp_head;

  for (count_cells = 0; count_cells < head->number_cells; count_cells++) {
    cell			= bp_head->bp_cells + count_cells;
    cell->id			= (head->cells + count_cells)->id;
    cell->cell_ptr		= (head->cells + count_cells);
    cell->connect_list		=
      bpAllocConnections( (head->cells + count_cells)->number_connections );
  }

  bp_network_head		= bp_head;
  bp_parameters_allocate	= TRUE;

  fprintf( stderr, "done.\n" );

  return NULL;
}  /* bpAllocParam( ) */



/*
 *  DW 95.01.20  Free BP-specific storage.
 */

void
bpFreeParam( )
{
  register BP_NETWORK	netPtr, oldNetPtr;
  register BP_CELL	cellPtr;
  register int		cellCount,
  			numCells;


  if (!bp_parameters_allocate)
    return;

  netPtr = bp_network_head;
  while (netPtr) {

    oldNetPtr = netPtr;
    cellPtr = netPtr->bp_cells;
    numCells = netPtr->net_ptr->number_cells;

    for (cellCount = 0; cellCount < numCells; cellCount++) {
      free( (cellPtr + cellCount)->connect_list );
      free( cellPtr + cellCount );
    }

    free( oldNetPtr );
    netPtr = netPtr->next;
  }

  bp_network_head		= NULL;
  bp_parameters_allocate	= FALSE;
}



int
propagate_derivatives( bp_network, bp_cell, number_cells )
    BP_NETWORK bp_network;
    BP_CELL bp_cell;
    int number_cells;
{
  register int count, count_connections;
  float dEx;
  BP_CELL temp_cell;
  BP_NETWORK temp_net;
  CONNECTION connections, temp_connections;
  int number_connections, net_id, cell_id;
  float weight, firing_rate;


  for (count = 0; count < number_cells; count ++) {
    temp_cell =  bp_cell + count;
    dEx = temp_cell->dEx;
    connections = temp_cell->cell_ptr->connect_list;
    number_connections = temp_cell->cell_ptr->number_connections;

    for (count_connections = 0;
	 count_connections < number_connections;
	 count_connections++) {

      temp_connections = connections + count_connections;
      weight = temp_connections->conductance;
      net_id = temp_connections->input_cell->net_id;
      cell_id = temp_connections->input_cell->id;
      firing_rate = temp_connections->input_cell->firing_rate;

      temp_net = bp_network;
      while (temp_net && temp_net->id != net_id)
	temp_net = temp_net->next;

      if (!temp_net) {
	fprintf( stderr,
		"Error:  I think you have multiple Hidden networks\n" );
	fprintf( stderr,
		"  connected to the same Output / Input network.\n" );
	fprintf( stderr,
		"  Please re-check your network algorithm, and contact\n" );
	fprintf( stderr,
		"  NEXUS support if you still are having problems.\n" );
	exit(0);
      }

      (temp_net->bp_cells + (cell_id - 1))->dEx =
	((temp_net->bp_cells + (cell_id - 1))->dEx
	 + ((dEx * weight) * (firing_rate ) * (1.0 - (firing_rate ))));
    }
  }

  return OK;
}  /* propagate_derivatives( ) */



int
clear_derivatives( bp_network )
    BP_NETWORK bp_network;
{
  register BP_NETWORK	temp_net;
  register int		number_cells, count;


  temp_net = bp_network;
  while (temp_net != NULL) {
    number_cells = (temp_net->net_ptr)->number_cells;

    for (count = 0; count < number_cells; count++)
      (temp_net->bp_cells + count)->dEx = 0.0;

    temp_net = temp_net->next;
  }

  return 0;
}  /* clear_derivatives( ) */




void
bpChangeWeights( bp_network )
    BP_NETWORK bp_network;
{
  BP_NETWORK temp_net;
  BP_CELL temp_cell;
  NETWORK net;
  CONNECTION connections;
  BP_CONNECT bp_connections;
  int number_cells, number_connections, count, count_connections;


  if (query_flag( LEARN_BATCH ) && END_OF_BATCH_FILES)
    printf("Changing conductances...");
  else if (query_flag( LEARN_BATCH ))
    printf("Storing conductance changes...");
  else
    printf("Changing conductances...");

  temp_net = bp_network;

  while (temp_net != NULL) {
    net = temp_net->net_ptr;
    number_cells = net->number_cells;

    for (count = 0; count < number_cells; count++) {
      temp_cell = temp_net->bp_cells + count;
      number_connections = temp_cell->cell_ptr->number_connections;

      for (count_connections = 0;
	   count_connections < number_connections;
	   count_connections++) {

	connections = temp_cell->cell_ptr->connect_list + count_connections;

#if 0
	/* v0.82: BUG */
	connections->conductance = connections->conductance 
	  - (bpLearningRate * 
	     (temp_cell->dEx) * 
	     (connections->input_cell)->firing_rate);

#else
	/* v0.9 */
	bp_connections = temp_cell->connect_list + count_connections;

	if (query_flag( LEARN_BATCH )) {
	  bp_connections->batch_conductance += ((temp_cell->dEx) * 
	     (connections->input_cell)->firing_rate);
	  if (END_OF_BATCH_FILES) {
	    connections->conductance +=
	      (bp_connections->momentum = 
	       (-bpLearningRate * bp_connections->batch_conductance 
		+ bpParaMomentum * bp_connections->momentum));
	    bp_connections->batch_conductance = (float) 0.0;
	  }
	}
	else 
	  connections->conductance += 
	    (bp_connections->momentum =
	     (-bpLearningRate * (temp_cell->dEx) *
	      (connections->input_cell)->firing_rate
	      + bpParaMomentum * bp_connections->momentum));

#endif
      }  /* for (number_connections) */

    }  /* for (number_cells) */

    temp_net = temp_net->next;
  }
  
  if (END_OF_BATCH_FILES)
    END_OF_BATCH_FILES = FALSE;
  printf("done.\n");
}  /* bpChangeWeights( ) */



static BP_NETWORK
bpAllocNetwork( )
{
  BP_NETWORK	head;


  if ((head = (BP_NETWORK) malloc( sizeof( ELEMENT_BP_NETWORK ) )))
    return head;

  printf( "FATAL: Memory allocation failure, bpAllocNetwork.\n" );
  exit(1);
}  /* bpAllocNetwork( ) */



/* create cells of network */
/* Note - calloc() is used to increase searching efficiency */

static BP_CELL
bpAllocCells( number )
    unsigned number;
{
  BP_CELL	head;


  if ((head = (BP_CELL) calloc( number, sizeof( ELEMENT_BP_CELL ) )))
    return head;

  printf( "FATAL: Memory allocation failure, bpAllocCells.\n" );
  exit(1);
}  /* bpAllocCells( ) */



static BP_CONNECT
bpAllocConnections( number )
    unsigned number;
{
  BP_CONNECT	head;


  if (number == 0)
    return NULL;

  if ((head = (BP_CONNECT) calloc( number, sizeof( ELEMENT_BP_CONNECT ) )))
    return head;

  printf( "FATAL: Memory allocation failure, bpAllocConnections.\n");
  exit(1);
}  /* bpAllocConnections( ) */



/*
 *  JB 94.06.09
 *    Can save error without turning learning on.
 *  DW 94.08.19
 *    Added return value to stop simulation if fatal error occurred.  Keeps
 *      NEXUS from crashing if it can't find training file, for instance.
 *      "filename" was overloaded: replaced with better names.
 *        This aided eventually in separating XView code.
 *      fp_output is now static local variable; NO NEED for a global.
 *      Also, renamed "fp" to "fp_train" so that we know what it means.
 */

int
run_bp( count_cycles, cycles )
    int count_cycles, cycles;
{
  FILE			*fp_train;
  char			bpTrainFilename[NAME_SIZE];
  int			do_outputs, num, count, x, y;
  BP_NETWORK		temp_net;
  BP_CELL		temp_cell;
  float			code, error;

  static FILE		*fp_output;


  
  if (bp_parameters_allocate == FALSE) {
    printf( "ERROR: Back Prop parameters were not allocated.\n" );
    printf( "  Turning off BP learning, and ABORTing simulation.\n\n" );
    set_flag( LEARN_BP, OFF );
    return ERROR;
  }
  
  /*
   *  On the first cycle, initialize file handles and variables.
   *  Open list of training output filenames, and error save file.
   */

  if (count_cycles == 0) {

    if (!(fp_output = nxFopen( bpBatchFilename ))) {
      printf( "Can not open training batch file '%s';\n", bpBatchFilename );
      printf( "  ABORTing simulation.  Check 'Training Batch File'\n" );
      printf( "  parameter in Back Propagation learning menu.\n" );
      return ERROR;
    }

    /*
     *  DW 94.11.08: If we can't open this file, still continue the
     *    simulation, but without saving error.
     */
    if (query_flag( SAVE_ERR_BP )) {
      if (!(bp_save_error_fp = fopen( bpErrorFilename, "w" ))) {
	printf( "Can not open Backprop. error file '%s'.\n", bpErrorFilename );
	printf( "  Turning error saving off and continuing simulation.\n\n" );
	set_flag( SAVE_ERR_BP, OFF );
      }
    }

  } /* count_cycles == 0 */

  do_outputs = TRUE;



  /*
   *  run_bp() is called when either LEARN_BP or SAVE_ERR_BP is TRUE.
   *    If we're only saving the errors, skip all derivative code.
   */

  if (query_flag( LEARN_BP )) {
    clear_derivatives( bp_network_head );
    printf( "Calculating derivatives . . . \n" );
  }

  temp_net = bp_network_head;
  while (temp_net != NULL) {
    temp_cell = temp_net->bp_cells;
    
    if (do_outputs != FALSE) {

      if (query_flag( LOAD_RANDOM )) {
	rewind(fp_output);
	for (count = 0; count < LOAD_MARKER; count++)
	  fscanf( fp_output, "%*[^\n]%*c" );
      }

      if (fscanf( fp_output, "%s", bpTrainFilename ) == EOF) {
	rewind( fp_output );
	fscanf( fp_output, "%s", bpTrainFilename );
      }

      printf( "Comparing with output file '%s'...\n", bpTrainFilename );
      fp_train = nxFopen( bpTrainFilename );
      if (fp_train == NULL) {
	printf( "ERROR: Can not open training file '%s'.\n", bpTrainFilename );
	printf( "  ABORTing simulation.  Please check the batch file\n" );
	printf( "  '%s' for errors.\n", bpBatchFilename );
	return ERROR;
      }

      error = 0.0;

      for (y = temp_net->net_ptr->dim_y - 1; y >= 0; y--)
	for (x = 0; x < temp_net->net_ptr->dim_x; x++) {
	  num = y*temp_net->net_ptr->dim_x + x;

	  if (fscanf( fp_train, "%f", &code ) == EOF)
	    break;

	  if (query_flag( LEARN_BP ))
	    (temp_cell + num)->dEx =
	      (((temp_cell + num)->cell_ptr->firing_rate - code)
	       * ((temp_cell + num)->cell_ptr->firing_rate)
	       * (1.0 - (temp_cell + num)->cell_ptr->firing_rate));

	  error += (((temp_cell + num)->cell_ptr->firing_rate - code)
		    * ((temp_cell + num)->cell_ptr->firing_rate - code));

	}  /* for (x), for (y) */


      fclose( fp_train );    
      printf("sum squared error = %f\n",error);

      /*
       *  Write error to file.
       */
      if (query_flag( SAVE_ERR_BP )) {
	fprintf( bp_save_error_fp, "%f", error );

	/* record load marker if load=random 6/27/94 J.B. */
	if (query_flag( LOAD_RANDOM ))
	  fprintf( bp_save_error_fp, " %d", LOAD_MARKER + 1 );

	fprintf( bp_save_error_fp, "\n" );  
      }

      if (query_flag( LEARN_BP )
	  && temp_net->next != NULL)
	propagate_derivatives( temp_net, temp_cell,
			      (temp_net->net_ptr)->number_cells );

      do_outputs = FALSE;

    }  /* do_outputs == TRUE */


    else if (query_flag( LEARN_BP )
	     && temp_net->next != NULL)
      propagate_derivatives( temp_net, temp_cell,
			    (temp_net->net_ptr)->number_cells );

    temp_net = temp_net->next;
  } /* while temp_net != NULL */

  bpChangeWeights( bp_network_head );


  /*
   *  After last cycle, clean up.
   */
  if (count_cycles == cycles - 1) {
    fclose( fp_output );
    if (query_flag( SAVE_ERR_BP ))
      fclose( bp_save_error_fp );
  }

  return OK;
}  /* run_bp( ) */



void
closeErrorFileBP( )
{
  if (bp_save_error_fp)
    fclose( bp_save_error_fp );
}



#if defined(NEXUS_SGI) || defined(NEXUS_LINUX)
/*
 * TkNx_BpAllocParam
 */
COMMAND_PROC(TkNx_BpAllocParam)
{
  char			buff[1024];
  char			*result;


  TCL_CHK( "TkNx_BpAllocParam", argc, 0, "");

  if ((result = bpAllocParam( ))) {
    sprintf( buff, "error {in bpAllocParam(): %s}", result );
    Tcl_AppendResult( interp, buff, NULL );
  }

  return TCL_OK;
}



/*
 *  Returns TRUE if user answers YES to the query.  DW 94.11.09
 */

int
bpErrorQuery( char *string )
{
  return queryDialog( string );
}
#endif /* defined(NEXUS_SGI) || defined(NEXUS_LINUX) */



#ifdef NEXUS_SUN
void
create_bp( item, event )
    Panel_item	item;
    Event	*event;
{
  char			*result;

  extern char		*bpAllocParam( );



  if (result = bpAllocParam( )) {
    notice_prompt( bp_panel, NULL,
		  NOTICE_MESSAGE_STRINGS,	result, NULL,
		  NULL );
    xv_set( item,
	   PANEL_NOTIFY_STATUS, XV_ERROR,
	   NULL );
  }
}



/*****************************************************************************
 *  Set up the Back Propagation Learning window.
 *****************************************************************************/

int
setup_menu_bp( top_frame )
    Frame top_frame;
{
  extern char	bpBatchFilename[],
  		bpErrorFilename[],
  		bpHiddenNetwork[],
		bpOutputNetwork[];
  extern float	bpLearningRate,
		bpParaMomentum;

  Panel_item	temp_text;
  char			*temp_str = "00000000000000000000";


  
  bp_frame = (Frame)
    xv_create( top_frame,		FRAME,
	      FRAME_LABEL, 		"Back Propagation",
	      XV_WIDTH,			1000,
	      XV_HEIGHT,		1000,
	      NULL );
  bp_panel = (Panel)
    xv_create( bp_frame,		PANEL, 
	      PANEL_LAYOUT,		PANEL_HORIZONTAL,
	      OPENWIN_SHOW_BORDERS,	TRUE,
	      NULL );

  chbox_bp = (Panel_item)
    xv_create( bp_panel,			PANEL_CHECK_BOX,
	      PANEL_CHOOSE_ONE,			TRUE,
	      PANEL_LABEL_STRING,		"Back Prop. Learning",
	      PANEL_CHOICE_STRINGS,		"OFF", "ON", NULL,
	      PANEL_NOTIFY_PROC,		proc_chbox_binary,
	      PANEL_CLIENT_DATA,		LEARN_BP,
	      PANEL_VALUE,			query_flag( LEARN_BP ),
	      NULL); 
  
  (void) xv_create( bp_panel,
		   PANEL_NAME_TEXT( bpBatchFilename ),
		   PANEL_NEWLINE,		20,
		   PANEL_LABEL_STRING,		"Training Batch File: ",
		   PANEL_VALUE,			"",
		   NULL );
  (void) xv_create( bp_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );
  
  temp_text = (Panel_item)
    xv_create( bp_panel,
	      PANEL_NAME_TEXT( bpHiddenNetwork ),
	      PANEL_NEWLINE,			20,
	      PANEL_LABEL_STRING,		"Hidden Unit Network: ",
	      PANEL_VALUE,			"",
	      XV_KEY_DATA,			KEY_UPDATE_LIST, TRUE,
	      NULL );
  (void) xv_create( bp_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );
  bp_network_hidden_list = (Panel_item)
    xv_create( bp_panel,
	      PANEL_NETWORK_LIST( temp_text, bpHiddenNetwork ),
	      PANEL_NEWLINE,			10,
	      NULL );
  xv_set( temp_text,
	 XV_KEY_DATA, KEY_MY_LIST, bp_network_hidden_list,
	 NULL );

  temp_text = (Panel_item)
    xv_create( bp_panel,
	      PANEL_NAME_TEXT( bpOutputNetwork ),
	      PANEL_NEWLINE,			20,
	      PANEL_LABEL_STRING,		"Output Network: ",
	      PANEL_VALUE,			"",
	      XV_KEY_DATA,			KEY_UPDATE_LIST, TRUE,
	      NULL );
  (void) xv_create( bp_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );
  bp_network_output_list = (Panel_item)
    xv_create( bp_panel,
	      PANEL_NETWORK_LIST( temp_text, bpOutputNetwork ),
	      PANEL_NEWLINE,			10,
	      NULL );
  xv_set( temp_text,
	 XV_KEY_DATA, KEY_MY_LIST, bp_network_output_list,
	 NULL );

  sprintf( temp_str, "%9.3f", bpLearningRate );
  (void) xv_create( bp_panel,			PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Learning rate: ",
		   PANEL_NEWLINE,		20,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&bpLearningRate,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( bp_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  sprintf( temp_str, "%9.3f", bpParaMomentum );
  (void) xv_create( bp_panel,			PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Momentum: ",
		   PANEL_NEWLINE,		10,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&bpParaMomentum,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( bp_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  chbox_save_err_bp = (Panel_item)
    xv_create( bp_panel,			PANEL_CHECK_BOX,
	      PANEL_CHOOSE_ONE,			TRUE,
	      PANEL_NEWLINE,			20,
	      PANEL_LABEL_STRING,		"Save Error: ",
	      PANEL_CHOICE_STRINGS,		"OFF", "ON", NULL,
	      PANEL_NOTIFY_PROC,		proc_chbox_binary,
	      PANEL_CLIENT_DATA,		SAVE_ERR_BP,
	      PANEL_VALUE,			query_flag( SAVE_ERR_BP ),
	      NULL );

  (void) xv_create( bp_panel,
		   PANEL_NAME_TEXT( bpErrorFilename ),
		   PANEL_NEWLINE,		10,
		   PANEL_LABEL_STRING,		"Error Filename: ",
		   PANEL_VALUE,			"",
		   NULL );
  (void) xv_create( bp_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  (void) xv_create( bp_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING,		"Allocate Parameters",
		   PANEL_NEWLINE,		20,
		   PANEL_NOTIFY_PROC,		create_bp,
		   NULL);

  (void) xv_create( bp_panel, PANEL_BUTTON,
		   PANEL_LABEL_STRING,		"Cancel",
		   PANEL_NOTIFY_PROC,		proc_btn_cancel,
		   PANEL_CLIENT_DATA,		bp_frame,
		   NULL);

  window_fit( bp_panel );
  window_fit( bp_frame );
}  /* setup_menu_bp() */



void
go_backprop( item, event )
    Panel_item	item;
    Event	*event;
{
  xv_set( bp_frame,
	 XV_SHOW, TRUE,
	 NULL );
}



/*
 *  Returns TRUE if user answers YES to the query.  DW 94.11.07
 */

int
bpErrorQuery( string )
    char *string;
{
  return ((notice_prompt( bp_panel, NULL,
			NOTICE_MESSAGE_STRINGS, string,	NULL,
			NOTICE_BUTTON_YES,	"Yes",
			NOTICE_BUTTON_NO,	"No",
			NULL )) == NOTICE_YES);
}
#endif





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
