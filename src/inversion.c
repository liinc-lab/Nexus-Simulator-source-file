/**************************************************************
 *                                                            *
 *                           NEXUS                            *
 *                                                            *
 *              (c) 1990 Paul Sajda and Leif Finkel           *
 *                                                            *
 *                     (nexus_inversion.c)                    *
 *                                                            *
 **************************************************************/

/* code to invert network using algorithm of Kindermann and Linden 1990 */

#include "nexus.h"
#include "inversion.h"

#include "tcl_main.h"
#include "graphics.h"

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



INV_NETWORK		inversion_network_head;
int			inversion_parameters_allocate = FALSE;
float			invLearningRate = 2.0,
			invSecondRatio = 0.0,
			invSecondApprox = 1.0,
			invDecay = 1.0;
char			invOutputFilename[NAME_SIZE],
  			invErrorFilename[NAME_SIZE],
			invInputNetwork[NAME_SIZE],
			invOutputNetwork[NAME_SIZE];


static INV_NETWORK	invAllocNetwork _ANSI_ARGS_(( void ));
static INV_CELL		invAllocCells _ANSI_ARGS_(( unsigned number ));
static FILE		*inversion_save_error_fp;

static void		invChangeInputs _ANSI_ARGS_(( INV_NETWORK ));

#ifdef NEXUS_SUN
Frame			inversion_frame;
Panel_item		chbox_invers,
			chbox_save_err_invers,
			inversion_network_input_list,
			inversion_network_output_list;

static Panel		inversion_panel;
#endif








char *
invAllocParam( )
{
  NETWORK	head;
  INV_NETWORK	inversion_head = NULL,
  		old_inversion_head;
  INV_CELL	cell;
  int		count_cells;


  if (inversion_parameters_allocate) {
    if (!invErrorQuery( "Inversion params already allocated.  Reallocate?" ))
      return "Aborting re-allocation.";
    printf( "Re-allocating Inversion parameters..." );

    invFreeParam( );
  }
  else
    printf( "Allocating Inversion parameters..." );

  if (strlen( invInputNetwork ) == 0)
    return "Input Network is not specified.";
  if (strlen( invOutputNetwork ) == 0)
    return "Output Network is not specified.";

  /*
   *  Find input network.
   */

  if (!(head = network_head))
    return "No networks have been loaded.";
  while (head && (strcmp( head->name, invInputNetwork ) != 0))
    head = head->next;
  if (!head)
    return "The Input Network specified does not exist.";

  /*
   *  Find output network.
   */

  while (head && (strcmp( head->name, invOutputNetwork ) != 0)) {

    old_inversion_head		= inversion_head;
    inversion_head		= invAllocNetwork( );
    inversion_head->id		= head->id;
    inversion_head->inv_cells	= invAllocCells( head->number_cells );
    inversion_head->net_ptr	= head;
    inversion_head->next	= old_inversion_head;

    for (count_cells = 0; count_cells < head->number_cells; count_cells++) {
      cell		= inversion_head->inv_cells + count_cells;
      cell->id		= (head->cells + count_cells)->id;
      cell->cell_ptr	= (head->cells + count_cells);
    }
    
    head = head->next;
  }
  if (!head)
    return "The Output Network specified does not exist.";

  old_inversion_head		= inversion_head;
  inversion_head		= invAllocNetwork( );
  inversion_head->id		= head->id;
  inversion_head->inv_cells	= invAllocCells( head->number_cells );
  inversion_head->net_ptr	= head;
  inversion_head->next		= old_inversion_head;

  for (count_cells = 0; count_cells < head->number_cells; count_cells++) {
    cell		= inversion_head->inv_cells + count_cells;
    cell->id		= (head->cells + count_cells)->id;
    cell->cell_ptr	= (head->cells + count_cells);
  }

  inversion_network_head	= inversion_head;
  inversion_parameters_allocate	= TRUE;

  printf( "done.\n" );

  return NULL;
}  /* invAllocParam( ) */



/*
 *  DW 95.01.27  Free INV-specific storage.
 */

void
invFreeParam( )
{
  register INV_NETWORK	netPtr, oldNetPtr;
  register INV_CELL	cellPtr;
  register int		cellCount,
  			numCells;


  if (!inversion_parameters_allocate)
    return;

  netPtr = inversion_network_head;
  while (netPtr) {

    oldNetPtr = netPtr;
    cellPtr = netPtr->inv_cells;
    numCells = netPtr->net_ptr->number_cells;

    for (cellCount = 0; cellCount < numCells; cellCount++)
      free( cellPtr + cellCount );

    free( oldNetPtr );
    netPtr = netPtr->next;
  }

  inversion_network_head	= NULL;
  inversion_parameters_allocate	= FALSE;
}  /* invFreeParam( ) */



static INV_NETWORK
invAllocNetwork( )
{
  INV_NETWORK	head;


  if ((head = (INV_NETWORK) malloc( sizeof( ELEMENT_INV_NETWORK ) )))
    return head;

  printf( "FATAL: Memory allocation failure, invAllocNetwork.\n" );
  exit(1);
}  /* invAllocNetwork( ) */



/* create cells of network */
/* Note - calloc() is used to increase searching efficiency */

static INV_CELL
invAllocCells( number )
    unsigned number;
{
  INV_CELL	head;


  if ((head = (INV_CELL) calloc( number, sizeof( ELEMENT_INV_CELL ) )))
    return head;

  printf( "FATAL: Memory allocation failure, invAllocCells.\n" );
  exit(1);
}  /* invAllocCells( ) */



void
propagate_derivatives_inv( inversion_network, inversion_cell, number_cells )
    INV_NETWORK	inversion_network;
    INV_CELL	inversion_cell;
    int		number_cells;
{
  int		count, count_connections;
  float		dEx;
  INV_CELL	temp_cell;
  INV_NETWORK	temp_net;
  CONNECTION	connections, temp_connections;
  int		number_connections, net_id, cell_id;
  float		weight, firing_rate;

  

  for (count = 0; count < number_cells; count ++) {
    temp_cell =  inversion_cell + count;
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
      temp_net = inversion_network;
      while(temp_net->id != net_id)
	temp_net = temp_net->next;
      (temp_net->inv_cells + (cell_id - 1))->dEx =
	((temp_net->inv_cells + cell_id - 1)->dEx
	 + (dEx * weight * firing_rate * (1.0 - firing_rate)));
    }
  }
}  /* propagate_derivatives_inv( ) */


void
clear_derivatives_inv( inversion_network )
    INV_NETWORK	inversion_network;
{
  INV_NETWORK	temp_net;
  INV_CELL	temp_cell;
  int		number_cells, count;



  temp_net = inversion_network;
  while (temp_net != NULL) {
    number_cells = (temp_net->net_ptr)->number_cells;
    for (count = 0; count < number_cells; count++) {
      temp_cell = temp_net->inv_cells + count;
      temp_cell->dEx = 0.0;
    }
    temp_net = temp_net->next;
  }
}  /* clear_derivatives_inv( ) */



/*
 *  DW 95.02.25  Added display code.  Changed name and type from
 *    (int) change_inputs( ) to localize this function.
 */

static void
invChangeInputs( inversion_network )
    INV_NETWORK		inversion_network;
{
  INV_NETWORK	temp_net;
  INV_CELL	temp_cell;
  NETWORK	net, get_network_name();
  CELL		cell;
  int		number_cells, count;


  temp_net = inversion_network;
  while ((temp_net->net_ptr)->id != get_network_name(invInputNetwork)->id
	 && temp_net != NULL)
    temp_net = temp_net->next;
  if (temp_net == NULL)
    return;

  net = temp_net->net_ptr;

  number_cells = net->number_cells;
  for (count = 0; count < number_cells; count++) {
    temp_cell = temp_net->inv_cells + count;
    cell = net->cells + count;
    cell->firing_rate = invDecay * (cell->firing_rate
				    - (invLearningRate * (temp_cell->dEx))
				    + invSecondRatio * invSecondApprox);

    if (display_type == DISP_CELL)
      display_cell_activity( net, count + 1 );
  }

  if (display_type == DISP_NET)
    graph_network( net );
}  /* invChangeInputs ( ) */


      
void
closeErrorFileInv( )
{
  if (inversion_save_error_fp)	/* JB 94.06.09 */
    fclose( inversion_save_error_fp );
}



/*
 *  JB 94.06.11
 *    Can save error without turning learning on.
 *  DW 94.11.07
 *    Added return value to stop simulation if fatal error occurred.  Keeps
 *      NEXUS from crashing if it can't find training file, for instance.
 *      "filename" was overloaded: replaced with better names.
 *        This aided eventually in separating XView code.
 *      fp_output is now static local variable; NO NEED for a global.
 *      Also, renamed "fp" to "fp_train" so that we know what it means.
 */

int
run_inversion( count_cycles, cycles )
    int count_cycles, cycles;
{
  extern int		LOAD_MARKER;
  int			do_outputs, num, x, y;
  INV_NETWORK		temp_net;
  INV_CELL		temp_cell;
  float			code, error;

  static FILE		*fp_output;


  if (inversion_parameters_allocate == FALSE) {
    printf( "Inversion parameters were not allocated.\n" );
    printf( "  Turning off INVERSION learning.\n");
    set_flag( LEARN_INVERS, OFF );
    return ERROR;
  }

  /*
   *  On the first cycle, initialize file handles and variables.
   *  Open list of training output filenames, and error save file.
   */

  if (count_cycles == 0) {

    if (!(fp_output = nxFopen( invOutputFilename ))) {
      printf( "Can not open training file '%s';\n", invOutputFilename );
      printf( "  ABORTing simulation.  Check 'Desired Output File'\n" );
      printf( "  parameter in Network Inversion learning menu.\n" );
      return ERROR;
    }

    /*
     *  DW 94.11.08: If we can't open this file, still continue the
     *    simulation, but without saving error.
     */
    if (query_flag( SAVE_ERR_INVERS )) {   /* 6/4/94 J.B. */
      if(!(inversion_save_error_fp = fopen( invErrorFilename, "w" ))) {
	printf( "Can not open Inversion error file '%s'.\n",
	       invErrorFilename );
	printf( "  Turning error saving off and continuing simulation.\n\n" );
	set_flag( SAVE_ERR_INVERS, OFF );
      }
    }

  } /* count_cycles == 0 */

  do_outputs = TRUE;

  if (query_flag( LEARN_INVERS )) {
    clear_derivatives_inv( inversion_network_head );
    printf( "Calculating derivatives . . . \n" );
  }

  temp_net = inversion_network_head;
  if (temp_net == NULL) {
    printf("ERROR - no networks loaded.\n");
    return ERROR;
  }

  while (temp_net != NULL) {
    temp_cell = temp_net->inv_cells;
    if(do_outputs != FALSE) {
      rewind(fp_output);
      for(error = 0.0,y=temp_net->net_ptr->dim_y -1;y>=0;y--) 
	for(x=0;x<temp_net->net_ptr->dim_x;x++) {
	  num = y*temp_net->net_ptr->dim_y + x;
	  if(fscanf(fp_output,"%f",&code) == EOF)
	    break;
	  if (query_flag( LEARN_INVERS ))
	    (temp_cell + num)->dEx =
	      (((temp_cell + num)->cell_ptr->firing_rate - code)
	       * ((temp_cell + num)->cell_ptr->firing_rate)
	       * (1.0 - (temp_cell + num)->cell_ptr->firing_rate));

	  error += (((temp_cell + num)->cell_ptr->firing_rate - code)
		    * ((temp_cell + num)->cell_ptr->firing_rate - code));
	}  
      printf("sum squared error = %f\n",error);

      /*
       *  Write error to file.
       */
      if (query_flag( SAVE_ERR_INVERS )) {
	fprintf( inversion_save_error_fp, "%f", error );

	if (query_flag( LOAD_RANDOM ))
	  /* record load marker if load=random 6/27/94 J.B. */
	  fprintf( inversion_save_error_fp, " %d", LOAD_MARKER + 1 );

	fprintf( inversion_save_error_fp, "\n" );
      }

      if (query_flag( LEARN_INVERS )
	  && temp_net->next != NULL)
	propagate_derivatives_inv( temp_net,
				  temp_cell,
				  (temp_net->net_ptr)->number_cells );
      do_outputs = FALSE;
    }
    else {
      if (query_flag( LEARN_INVERS )
	  && temp_net->next != NULL)
	propagate_derivatives_inv( temp_net,
				  temp_cell,
				  (temp_net->net_ptr)->number_cells );
    }
    temp_net = temp_net->next;
  }  /* while temp_net != NULL */

  printf("Changing inputs . . . \n");
  invChangeInputs( inversion_network_head );

  /*
   *  After last cycle, clean up.
   */  
  if (count_cycles == cycles - 1) {
    fclose (fp_output);
    if (query_flag( SAVE_ERR_INVERS ))
      fclose (inversion_save_error_fp);
  }

  return OK;
}  /* run_inversion() */



#if defined(NEXUS_SGI) || defined(NEXUS_LINUX)
/*
 * TkNx_InvAllocParam
 */
COMMAND_PROC(TkNx_InvAllocParam)
{
  char			buff[1024];
  char			*result;

  extern char		*invAllocParam( );



  TCL_CHK( "TkNx_InvAllocParam", argc, 0, "" );

  if ((result = invAllocParam( ))) {
    sprintf( buff, "error {in invAllocParam(): %s}", result );
    Tcl_AppendResult( interp, buff, NULL );
  }

  return TCL_OK;
}



/*
 *  Returns TRUE if user answers YES to the query.  DW 94.11.09
 */

int
invErrorQuery( char *string )
{
  return queryDialog( string );
}
#endif /* defined(NEXUS_SGI) || defined(NEXUS_LINUX) */


#ifdef NEXUS_SUN
void
create_inversion( item, event )
    Panel_item	item;
    Event	*event;
{
  char			*result;

  extern char		*invAllocParam( );



  if (result = invAllocParam( )) {
    notice_prompt( inversion_panel, NULL,
		  NOTICE_MESSAGE_STRINGS,	result, NULL,
		  NULL );
    xv_set( item,
	   PANEL_NOTIFY_STATUS, XV_ERROR,
	   NULL );
  }
}



/*****************************************************************************
 *  Set up the Network Inversion Learning window.
 *****************************************************************************/

int
setup_menu_inversion( top_frame )
    Frame	top_frame;
{
  extern char		invOutputFilename[],
  			invErrorFilename[],
  			invInputNetwork[],
  			invOutputNetwork[];
  extern float		invLearningRate,
  			invSecondRatio,
  			invSecondApprox,
  			invDecay;

  Panel_item		temp_text;
  char			*temp_str = "00000000000000000000";



  inversion_frame = (Frame)
    xv_create( top_frame,			FRAME,
	      FRAME_LABEL,			"Network Inversion",
	      XV_WIDTH,				1000,
	      XV_HEIGHT,			1000,
	      NULL );
  inversion_panel = (Panel)
    xv_create( inversion_frame,			PANEL,
	      PANEL_LAYOUT,			PANEL_HORIZONTAL,
	      OPENWIN_SHOW_BORDERS,		TRUE,
	      NULL );

  chbox_invers = (Panel_item)
    xv_create( inversion_panel,			PANEL_CHECK_BOX,
	      PANEL_CHOOSE_ONE,			TRUE,
	      PANEL_LABEL_STRING,		"Inversion Learning",
	      PANEL_CHOICE_STRINGS,		"OFF", "ON", NULL,
	      PANEL_NOTIFY_PROC,		proc_chbox_binary,
	      PANEL_CLIENT_DATA,		LEARN_INVERS,
	      PANEL_VALUE,			query_flag( LEARN_INVERS ),
	      NULL );

  (void) xv_create( inversion_panel,
		   PANEL_NAME_TEXT( invOutputFilename ),
		   PANEL_NEWLINE,		20,
		   PANEL_LABEL_STRING,		"Desired Output File: ",
		   PANEL_VALUE,			"",
		   NULL );
  (void) xv_create( inversion_panel,		PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  temp_text = (Panel_item)
    xv_create( inversion_panel,
	      PANEL_NAME_TEXT( invInputNetwork ),
	      PANEL_NEWLINE,			20,
	      PANEL_LABEL_STRING,		"Input Network name: ",
	      PANEL_VALUE,			"",
	      XV_KEY_DATA,			KEY_UPDATE_LIST, TRUE,
	      NULL );
  (void) xv_create( inversion_panel,		PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );
  inversion_network_input_list = (Panel_item)
    xv_create( inversion_panel,
	      PANEL_NETWORK_LIST( temp_text, invInputNetwork ),
	      PANEL_NEWLINE,			10,
	      NULL );
  xv_set( temp_text,
	 XV_KEY_DATA, KEY_MY_LIST, inversion_network_input_list,
	 NULL );

  (void) xv_create( inversion_panel,		PANEL_MESSAGE,
		   PANEL_LABEL_STRING,		"Clamp input between .1 and .9!",
		   PANEL_NEWLINE,		10,
		   XV_X,			30,
		   NULL );

  temp_text = (Panel_item)
    xv_create( inversion_panel,
	      PANEL_NAME_TEXT( invOutputNetwork ),
	      PANEL_NEWLINE,			20,
	      PANEL_LABEL_STRING,		"Output Network name: ",
	      PANEL_VALUE,			"",
	      XV_KEY_DATA,			KEY_UPDATE_LIST, TRUE,
	      NULL );
  (void) xv_create( inversion_panel,		PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );
  inversion_network_output_list = (Panel_item)
    xv_create( inversion_panel,
	      PANEL_NETWORK_LIST( temp_text, invOutputNetwork ),
	      PANEL_NEWLINE,			10,
	      NULL );
  xv_set( temp_text,
	 XV_KEY_DATA, KEY_MY_LIST, inversion_network_output_list,
	 NULL );


  sprintf( temp_str, "%9.3f", invLearningRate );
  (void) xv_create( inversion_panel,		PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Learning rate: ",
		   PANEL_NEWLINE,		20,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&invLearningRate,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( inversion_panel,		PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  sprintf( temp_str, "%9.3f", invSecondApprox );
  (void) xv_create( inversion_panel,		PANEL_TEXT,
		   PANEL_LABEL_STRING,		"2nd approximation: ",
		   PANEL_NEWLINE,		10,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&invSecondApprox,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( inversion_panel,		PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  sprintf( temp_str, "%9.3f", invSecondRatio );
  (void) xv_create( inversion_panel,		PANEL_TEXT,
		   PANEL_LABEL_STRING,		"2nd approximation ratio: ",
		   PANEL_NEWLINE,		10,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&invSecondRatio,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( inversion_panel,		PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  sprintf( temp_str, "%9.3f", invDecay );
  (void) xv_create( inversion_panel,		PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Decay: ",
		   PANEL_NEWLINE,		10,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&invDecay,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( inversion_panel,		PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  chbox_save_err_invers = (Panel_item)
    xv_create( inversion_panel,			PANEL_CHECK_BOX,
	      PANEL_NEWLINE,			20,
	      PANEL_CHOOSE_ONE,			TRUE,
	      PANEL_LABEL_STRING,		"Save Error: ",
	      PANEL_CHOICE_STRINGS,		"OFF", "ON", NULL,
	      PANEL_NOTIFY_PROC,		proc_chbox_binary,
	      PANEL_CLIENT_DATA,		SAVE_ERR_INVERS,
	      PANEL_VALUE,			query_flag( SAVE_ERR_INVERS ),
	      NULL );

  (void) xv_create( inversion_panel,
		   PANEL_NAME_TEXT( invErrorFilename ),
		   PANEL_NEWLINE,		10,
		   PANEL_LABEL_STRING,		"Error Filename: ",
		   PANEL_VALUE,			"",
		   NULL );
  (void) xv_create( inversion_panel,		PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  (void) xv_create( inversion_panel,		PANEL_BUTTON,
		   PANEL_LABEL_STRING,		"Allocate Parameters",
		   PANEL_NEWLINE,		20,
		   PANEL_NOTIFY_PROC,		create_inversion,
		   NULL );
  
  (void) xv_create( inversion_panel,		PANEL_BUTTON,
		   PANEL_LABEL_STRING,		"Cancel",
		   PANEL_NOTIFY_PROC,		proc_btn_cancel,
		   PANEL_CLIENT_DATA,		inversion_frame,
		   NULL );

  window_fit( inversion_panel );
  window_fit( inversion_frame );
}  /* setup_menu_inversion() */



void
go_inversion( item, event )
    Panel_item	item;
    Event	*event;
{
  xv_set( inversion_frame,
	 XV_SHOW, TRUE,
	 NULL );
}



/*
 *  Returns TRUE if user answers YES to the query.  DW 94.11.07
 */

int
invErrorQuery( string )
    char *string;
{
  return ((notice_prompt( inversion_panel, NULL,
			NOTICE_MESSAGE_STRINGS, string,	NULL,
			NOTICE_BUTTON_YES,	"Yes",
			NOTICE_BUTTON_NO,	"No",
			NULL )) == NOTICE_YES);
}
#endif








/* Emacs editing section. DW 940719 */

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
