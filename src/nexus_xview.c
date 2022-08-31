/****************************************************************
 *								*
 *				NEXUS				*
 *								*
 *		(c) 1990 Paul Sajda and Leif Finkel		*
 *								*
 *			  (nexus_xview.c)			*
 *								*
 *		    Xview toolkit-specific code			*
 *	    separated from nexus_main.c, cleaned up		*
 *			    DW 94.07.15				*
 *								*
 *      modified for Parallel Nexus.  10/30/91 by K.Sakai       *
 *		                      12/27/91 		        *
 ****************************************************************/

/* 
 *  GUI (Graphical User Interface) for simulator.  Based on the Xview
 *    toolkit.  Note that several variables must remain global
 *    due to the calling conventions used by Xview.
 *  Major clean-up of code, added smart object code, fixed other bugs.
 *  Conforms more to OPEN LOOK GUI Specification Guide.
 *    DW 94.07, 94.08.
 *
 *  DW 94.11.05  Finished removing non-XView-essential code to nexus_main.c
 *    and nexus_batch.c.
 */

#include "nexus.h"
#include "batch.h"
#include "nexus_xview.h"
#include <vogle.h>










/*****************************************************************************
 *  Global variables
 *****************************************************************************/

Display        *dpy;

char		view_network[NAME_SIZE] = "";
char		edit_connect_network[NAME_SIZE] = "";
char		edit_connect_function[FUNCTION_SIZE] = "";
int 		edit_connect_current_id = NO_CONNECTION;

extern void	translate_network( );

/***********
 *  Mouse support
 ***********/

float		locator_x = 0.0, locator_y = 0.0;
int		locator_button = 0;

/***********
 *  Global object declarations;  DW 940706
 ***********/

Frame		frame, 
		frame_about,
		frame_view,
		frame_simulate,
		frame_activity,
		frame_connect,
		frame_clamp,
		frame_load_activity,
		frame_edit_connect,
		frame_threshold,
		frame_save,
		frame_random, 
		frame_vogle,
		frame_filename;

/*		frame_print_screen,
		frame_graphics_filename,
		frame_plot,
		frame_plot_filename,
		frame_plot_menu, */

Menu		menu;

Canvas		vogle_canvas,
		plot_canvas;

/*  Other panels are local to init_objects().  */

Panel        	panel_filename,
		panel_simulate;

/*  These are updated by callbacks / routines.  */

Panel_item	filename_loaded;

/*  These are disabled during simulation.  */

Panel_item	simulate_go,
		simulate_stop,
		simulate_cancel,
		chbox_simulate_type,
		simulate_swap_rates_chbox,
		simulate_batch_learn_chbox,
		simulate_number_cycles;

Panel_item	chbox_activity_load,
		chbox_activity_record,
		chbox_connect_show_type,
		chbox_file_compressed,
		activity_file_list,
		amount,
		parameter_type, 
		parameter_value,
		parameter_network_text,
		edit_connect_id,
		list_ids_edit_connect,
		simulation_filename_text,
		filename_save,
		random_min,
		random_max,
		simulate_random_cells,

		activity_electrode,
		connect_electrode,
		view_electrode;

/*		plot_x_dim,
		plot_y_dim,
		plot_z_dim,
		move_plot_amount,
		rotate_plot_amount,
		print_screen_filename,
		plot_type,
		graphics_filename,
		print_data_precision; */

/*
 *  Network name lists
 */

Panel_item	list_networks_view,
		list_networks_activity_output,
/*		list_networks_plot, */
		list_networks_load_activity,
		list_networks_edit_connect,
		list_networks_clamp;

/*
 *  Server images (must be global to destroy() them at finale. DW 940729)
 */

Server_image	nexus_image, nexus_image2, nexus_image3, nexus_image4,
		pan_left_image, pan_right_image,
		pan_up_image, pan_down_image,
		zoom_in_image, zoom_out_image,
		rotate_x_clk_image, rotate_x_cclk_image,
		rotate_y_clk_image, rotate_y_cclk_image,
		rotate_z_clk_image, rotate_z_cclk_image,
		carriage_return_image;

#include "bitmaps.h"






int
main( argc, argv )
    int argc;
    char *argv[];
{
  extern int main_loop();
  extern int init_objects();
  extern void init_display();
  extern void reset_display();
  FILE *fp;

  /*
   *  All flags are OFF by default (value is 0).  To set any, add them
   *    to this statement.  DW 94.08.26
   */
  SET_FLAG( CONNECT_SHOW_TYPE, RETROGRADE );

  init_random_number_gen();

  if (init_objects( &argc, argv ))
    exit (-1);
  
  if (argc == 1) {
    introduction();
    xv_set( frame,
	   XV_SHOW, TRUE,
	   NULL );
    xv_set( frame_vogle,
	   XV_SHOW, TRUE,
	   NULL );
    XFlush( dpy );
    xv_set( filename_loaded,
	   PANEL_LABEL_STRING, "No file loaded.",
	   NULL );
  }
  else if (argc == 2) {
    printf( "Loading Build file <%s> . . .\n", argv[1] );
    fp = fopen( argv[1], "r" );

    if (fp == NULL)
      printf( "Can't load file.  Ignoring command line arguments.\n" );

    else if (read_network( fp ) == TRUE)
      init_display( argv[1] );
  }
  else if (argc == 3) {
    switch (argv[1][1]) {
    case 'b':
      printf( "Loading Build file <%s> . . .\n", argv[2] );
      fp = fopen( argv[2], "r" );
      
      if (fp == NULL)
	printf( "Can't load file.  Ignoring command line arguments.\n" );
      
      else if (read_network( fp ) == TRUE)
	init_display( argv[2] );

      break;

    case 'l':
      if (!(fp = fopen( argv[2], "r" ))) {
	printf( "Can't find '%s'.  Skipping load option.\n" );
	reset_display( "" );
	break;
      }
      else {
	fclose( fp );
	printf( "Loading Saved simulation file <%s> . . .\n", argv[2] );
	load_simulation( argv[2] );
	reset_display( argv[2] );
	break;
      }

    default:
      printf( "%s: unknown option.  Ignored.\n", argv[1] );
      reset_display( "" );
      break;
    }
  }
  else {
    printf( "Usage: nexus [-b filename | -l filename]\n" );
    exit (cleanup());
  }


  /*
   *  main_loop() should return OK if a simulation occurred, or
   *  ERROR if the user wants to exit.
   */
  while (main_loop() == OK);

  exit (cleanup());
} /* main() */










/*****************************************************************************
 *  Utility functions with XView-specific code.
 *****************************************************************************/

/*
 *  This function dynamically creates a list of all networks.
 */

get_network_names_list( panel_list )  
    Panel_item	panel_list;
{
  NETWORK temp_network;
  int count;
  
  temp_network = network_head;
  xv_set( panel_list,
	 PANEL_LIST_DELETE_ROWS,
		0, (int) xv_get( panel_list, PANEL_LIST_NROWS ),
	 NULL );
  for (count = 0; temp_network != NULL; count++) {
    xv_set( panel_list,
	   PANEL_LIST_INSERT,		count,
	   PANEL_LIST_STRING,		count,	temp_network->name,
	   PANEL_LIST_CLIENT_DATA,	count,	temp_network,
	   PANEL_LIST_SELECT,		count,	FALSE,
	   NULL );
    temp_network = temp_network->next;
  }
}  /* get_network_names_list() */

/*
 *  Place above-created list in all appropriate network lists.
 *  Should be called from any routine that changes the
 *    network_head list:
 *       nexus_build.c: read_network()
 *       nexus_load.c:  load_simulation()
 */

void
update_network_lists()
{
#ifdef NEXUS_HEBB
  extern Panel_item	hebbNetworkList;
#endif NEXUS_HEBB
#ifdef NEXUS_BP
  extern Panel_item	bp_network_hidden_list,
  			bp_network_output_list;
#endif NEXUS_BP
#ifdef NEXUS_INVERSION
  extern Panel_item	inversion_network_input_list,
			inversion_network_output_list;
#endif NEXUS_INVERSION
#ifdef NEXUS_RBF
  extern Panel_item	rbf_network_input_list,
			rbf_network_hidden_list,
			rbf_network_output_list;
#endif NEXUS_RBF



  get_network_names_list( list_networks_view );
  get_network_names_list( list_networks_activity_output );
  get_network_names_list( list_networks_load_activity );
  get_network_names_list( list_networks_edit_connect );
  get_network_names_list( list_networks_clamp );

#ifdef NEXUS_HEBB
  get_network_names_list( hebbNetworkList );
#endif NEXUS_HEBB
#ifdef NEXUS_BP
  get_network_names_list( bp_network_hidden_list );
  get_network_names_list( bp_network_output_list );
#endif NEXUS_BP
#ifdef NEXUS_INVERSION
  get_network_names_list( inversion_network_input_list );
  get_network_names_list( inversion_network_output_list );
#endif NEXUS_INVERSION
#ifdef NEXUS_RBF
  get_network_names_list( rbf_network_input_list );
  get_network_names_list( rbf_network_hidden_list );
  get_network_names_list( rbf_network_output_list );
#endif NEXUS_RBF
}  /* update_network_lists() */

int
is_in_frame_list( test_frame, enabled_frames )
    Frame test_frame;
    Frame enabled_frames[];
{
  int n = 0;
  
  while (enabled_frames[n])
    if (enabled_frames[n++] == test_frame)
      return TRUE;

  return FALSE;
}

/*
 *  Disable all frames EXCEPT those in the list - list must be NULL terminated
 */

void
disable_frames( enabled_frames )
    Frame enabled_frames[];
{
  int n = 1;
  Frame next_frame;
  
  if (is_in_frame_list( frame, enabled_frames ) == FALSE)
    xv_set( frame,
	   FRAME_BUSY, TRUE,
	   NULL );

  next_frame = xv_get( frame, FRAME_NTH_SUBFRAME, n );
  for (; next_frame != NULL; n++) {
    if (is_in_frame_list( next_frame, enabled_frames ) == FALSE)
      xv_set( next_frame,
	     FRAME_BUSY, TRUE,
	     NULL );

    next_frame = xv_get( frame, FRAME_NTH_SUBFRAME, n );
  }
}

/*
 *  Enable all frames EXCEPT those in the list - list must be NULL terminated
 */

void
enable_frames( enabled_frames )
    Frame enabled_frames[];
{
  int n = 1;
  Frame next_frame;
  
  /*  Check top frame  */
  if (is_in_frame_list( frame, enabled_frames ) == FALSE)
    xv_set( frame,
	   FRAME_BUSY, FALSE,
	   NULL );

  next_frame = xv_get( frame, FRAME_NTH_SUBFRAME, n );

  for (; next_frame != NULL; n++) {
    if (is_in_frame_list( next_frame, enabled_frames ) == FALSE)
      xv_set( next_frame,
	     FRAME_BUSY, FALSE,
	     NULL );

    next_frame = xv_get( frame, FRAME_NTH_SUBFRAME, n );
  }
}

void
redrawGraphics( )
{
  Frame			disabled_frames[1];


  XFlush( dpy );

  if (curr_electrode == MODE_ACTIVITY) {
    disabled_frames[0] = NULL;
    disable_frames( disabled_frames );
    clear_screen( );
    graph_activity( );
    enable_frames( disabled_frames );
  }

  else if (curr_electrode == MODE_CONNECT)
    draw_outline( TRUE );

  else
    draw_outline( FALSE );

  XFlush( dpy );
}

/*
 *  Set electrode to desired value and turn off the other electrodes.
 *    DW 94.08.17
 */

void
set_electrode( new_elect )
    int		new_elect;
{
  xv_set( activity_electrode,
	 PANEL_VALUE, (new_elect == MODE_ACTIVITY ? ON : OFF),
	 NULL );

  xv_set( connect_electrode,
	 PANEL_VALUE, (new_elect == MODE_CONNECT ? ON : OFF),
	 NULL );

  xv_set( view_electrode,
	 PANEL_VALUE, (new_elect == MODE_VIEW ? ON : OFF),
	 NULL );

  curr_electrode = new_elect;
}

void
reset_display( text )
    char *text;
{
  extern char	simulation_filename[];

  xv_set( frame,
	 XV_SHOW, TRUE,
	 NULL );
  xv_set( frame_vogle,
	 XV_SHOW, TRUE,
	 NULL );
  XFlush( dpy );
  redrawGraphics();
  xv_set( filename_loaded,
	 PANEL_LABEL_STRING, text,
	 NULL );
  xv_set( simulation_filename_text,
	 PANEL_VALUE, text,
	 NULL );
  strcpy( simulation_filename, text );
}

/*
 *  Initialize display after network is loaded / built.
 */
void
init_display( text )
    char *text;
{
  extern void connect_sim();
  Frame disabled_frames[1];
  
  disabled_frames[0] = NULL;
  disable_frames( disabled_frames );
  connect_sim();
  set_electrode( MODE_ACTIVITY );
  enable_frames( disabled_frames );
  reset_display( text );
}

/*
 *  Set nexus_flags appropriately, and handle any XView objects.  DW 94.08.26
 */
void
set_flag( which_flag, value )
    register flag_t	which_flag;
    register int	value;
{
  /*  Externally defined checkboxes */
  extern Panel_item		chbox_bp,
  				chbox_save_err_bp,
  				chbox_invers,
  				chbox_save_err_invers,
  				chbox_rbf,
  				chbox_save_err_rbf;

  SET_FLAG( which_flag, value );

  /*
   *  If the flag is READABLE _AND_ WRITEABLE by the program, place it in
   *    this switch statement.
   */

  switch( which_flag ) {

  case LEARN_BP:
    SET_CHBOX( chbox_bp, value );
    break;

  case SAVE_ERR_BP:
    SET_CHBOX( chbox_save_err_bp, value );
    break;

  case LEARN_INVERS:
    SET_CHBOX( chbox_invers, value );
    break;

  case SAVE_ERR_INVERS:
    SET_CHBOX( chbox_save_err_invers, value );
    break;

  case LEARN_RBF:
    SET_CHBOX( chbox_rbf, value );
    break;

  case SAVE_ERR_RBF:
    SET_CHBOX( chbox_save_err_rbf, value );
    break;

  case ACTIVITY_LOAD:
    SET_CHBOX( chbox_activity_load, value );
    break;

  case CONNECT_SHOW_TYPE:
    SET_CHBOX( chbox_connect_show_type, value );
    break;

  case FILE_COMPRESSED:
    SET_CHBOX( chbox_file_compressed, value );
    break;

  case ACTIVITY_CYCLE:
    SET_CHBOX( chbox_activity_record, value );
    break;
  }
}  /* set_flag() */

void
update_connect_id_list( string )
    char		*string;
{
  extern NETWORK	get_network_name();
  extern NETWORK	get_network_id();
  SPECS			temp_connect;
  int			count;
  char			temp_string[MAXARRAY];
  NETWORK		temp_net = network_head;



  while (temp_net && strcmp( temp_net->name, string ))
    temp_net = temp_net->next;
  if (!temp_net)
    return;

  xv_set( edit_connect_id,
	 PANEL_LABEL_STRING, "Connection:  ",
	 NULL );
  edit_connect_current_id = NO_CONNECTION;

  delete_list_entries( list_ids_edit_connect );
  temp_connect = get_network_name( string )->cells->specs;

  for (count = 0; temp_connect != NULL; count++) {
    if (temp_connect->type == RETROGRADE)
      sprintf( temp_string, "From-%s",
	      get_network_id( temp_connect->id )->name );
    else			/* ANTEROGRADE */
      sprintf( temp_string, "To-%s",
	      get_network_id( temp_connect->id )->name );

    xv_set( list_ids_edit_connect,
	   PANEL_LIST_INSERT, count,
	   PANEL_LIST_STRING, count, temp_string,
	   PANEL_LIST_CLIENT_DATA, count, temp_connect->connect_id,
	   NULL );

    temp_connect = temp_connect->next; /*next specs*/
  }
}  /* update_connect_id_list() */










/*****************************************************************************
 *  PANEL_BUTTON NOTIFY PROCedures
 *****************************************************************************/

/*
 *  Popup edit connectivity frame.  Also clear connections list.
 */

void
edit_connect( item, event )
    Panel_item	item;
    Event	*event;
{
  delete_list_entries( list_ids_edit_connect );
  xv_set( frame_edit_connect,
	 XV_SHOW, TRUE,
	 NULL );
}






/*
 *  Generic popup/popdown procedures replace all of the old procedures.
 *    DW 94.08.10
 *  Now incorporates load() and build() button notify procedures.  DW 94.08.20
 */

void
proc_btn_subframe( item, event )
    Panel_item	item;
    Event	*event;
{
  extern char	simulation_filename[];
  static char	file[NAME_SIZE];
  Frame		which_frame = (Frame) xv_get( item, PANEL_CLIENT_DATA );
  int		state = (int) xv_get( item, XV_KEY_DATA, KEY_LOAD_BUILD );

  extern int	checkFilenameSuffix( );


  xv_set( which_frame,
	 XV_SHOW, TRUE,
	 NULL );



  if (which_frame == frame_filename) {

    /* Place keyboard focus on the text object when it pops up. DW 94.07.26 */
    xv_set( panel_filename,
	   PANEL_CARET_ITEM, simulation_filename_text,
	   PANEL_CLIENT_DATA, state,
	   NULL );

    /* Make sure filename has correct suffix for current state. DW 94.11.02 */
    (void) checkFilenameSuffix( state, simulation_filename, file );

    strcpy( simulation_filename, file );
    xv_set( simulation_filename_text,
	   PANEL_VALUE, file,
	   NULL );
  }  /* which_frame == frame_filename */

}  /* proc_btn_subframe() */






void
proc_btn_cancel( item, event )
    Panel_item	item;
    Event	*event;
{
  xv_set( (Frame) xv_get( item, PANEL_CLIENT_DATA ),
	 XV_SHOW, FALSE,
	 NULL );
}






void
quit( item, event )
    Panel_item	item;
    Event	*event;
{
  if (notice_prompt( (Panel) xv_get( item, PANEL_PARENT_PANEL ), NULL,
		    NOTICE_FOCUS_XY,		event_x( event ),
		    				event_y( event ),
		    NOTICE_MESSAGE_STRINGS,
		    		"Do you really want to quit NEXUS?", NULL,
		    NOTICE_BUTTON_YES,		"Yes",
		    NOTICE_BUTTON_NO,		"No",
		    NULL ) == NOTICE_YES) {
    do_exit = TRUE;
    notify_stop();
  }
}






void
btn_memory( item, event )
    Panel_item	item;
    Event	*event;
{
  do_memory( );
}  /* btn_memory() */






/***********
 * functions to change simulation viewing perspective
 ***********/ 

move_net_x( item, event )
    Panel_item	item;
    Event	*event;
{
  moveNets( 'x', (int) xv_get( item, PANEL_CLIENT_DATA ) );
}  /* move_net_x() */

move_net_y( item, event )
    Panel_item	item;
    Event	*event;
{
  moveNets( 'y', (int) xv_get( item, PANEL_CLIENT_DATA ) );
}  /* move_net_y() */

move_net_z( item, event )
    Panel_item	item;
    Event	*event;
{
  moveNets( 'z', (int) xv_get( item, PANEL_CLIENT_DATA ) );
}  /* move_net_z() */






/***********
 *  Change the dimensions of an on-screen object on-the-fly.
 ***********/

change_dimensions_net( item, event )
    Panel_item	item;
    Event	*event;
{
  NETWORK	head, get_network_name();
  Panel		panel = (Panel) xv_get( item, PANEL_PARENT_PANEL );


  head = get_network_name( view_network );
  if (head == NULL) {
    printf( "ERROR - no such network %s\n", view_network );
    notice_prompt( panel, NULL,
		  NOTICE_MESSAGE_STRING,
			"Error - no such network",
		  NULL );
    return;
  }
  head->cell_size *= change_dimension_amount;

  draw_outline( FALSE );
}  /* change_dimensions_net() */



change_dimensions_text_w( item, event )
    Panel_item	item;
    Event	*event;
{
  NETWORK	head, get_network_name();
  Panel		panel = (Panel) xv_get( item, PANEL_PARENT_PANEL );

  
  head = get_network_name( view_network );
  if (head == NULL) {
    printf( "ERROR - no such network %s\n", view_network );
    notice_prompt(panel,NULL,
		  NOTICE_MESSAGE_STRING,
			"Error - no such network",
		  NULL );
    return;
  }
  head->name_size_w *= change_dimension_amount;

  draw_outline( FALSE );
}  /* change_dimensions_text_w() */



change_dimensions_text_h( item, event )
    Panel_item	item;
    Event	*event;
{
  NETWORK	head, get_network_name();
  Panel		panel = (Panel) xv_get( item, PANEL_PARENT_PANEL );

  
  head = get_network_name( view_network );
  if (head == NULL) {
    printf( "ERROR - no such network %s\n", view_network );
    notice_prompt( panel,NULL,
		  NOTICE_MESSAGE_STRING,
			"Error - no such network",
		  NULL );
    return;
  }
  head->name_size_h *= change_dimension_amount;

  draw_outline( FALSE );
}  /* change_dimensions_text_h() */






/***********
 *  Refresh the display according to the electrode's state.
 ***********/

void
refresh_window( item, event )
    Panel_item	item;
    Event	*event;
{
  redrawGraphics();
}  /* refresh_window() */

/*
 *  Display all connections: not currently used;  DW 940615
 */

#if 0
void
show_all_connections( item, event )
    Panel_item	item;
    Event	*event;
{ 
  extern void	all_connections_graphics();

  all_connections_graphics();
}
#endif 0






/*
 *  List activity files in the batch file 'filename_load_activity'.
 */

void
btn_list_afiles( item, event )
    Panel_item	item;
    Event	*event;
{
  Panel			panel = (Panel) xv_get( item, PANEL_PARENT_PANEL );
  int			i;

  
  delete_list_entries( activity_file_list );

  if (readBatchFile( NXBATCH_IN, filename_load_activity ))
    notice_prompt( panel, NULL,
		  NOTICE_MESSAGE_STRINGS,
		  "Error - No such file",
		  NULL,
		  NULL );
  else
    for (i = 0; i < currBatchListLength[NXBATCH_IN]; i++) {
      xv_set( activity_file_list,
	     PANEL_LIST_INSERT, i,
	     PANEL_LIST_STRING, i, (currBatchList[NXBATCH_IN] + i)->filename,
	     NULL );
    }
}  /* btn_list_afiles() */






/* checks .save suffix 6/17/94 J.B. */
void
btn_save_now( item, event )
    Panel_item	item;
    Event	*event;
{
  static FILE		*fp;
  FILE			*fp2;
  char			file[NAME_SIZE], file2[NAME_SIZE], *name, *address;
  char			suggestion[NAME_SIZE + 13];
  NETWORK		head;
  Panel			panel = (Panel) xv_get( item, PANEL_PARENT_PANEL);
  int			result, warning;		/* JB 940711 */
  
  if (checkFilenameSuffix( LOAD,
			  (char *) xv_get( filename_save, PANEL_VALUE),
			  file )) {
    strcpy( suggestion, "Suggestion: " );
    strcat( suggestion, file );
    notice_prompt( panel, NULL,
		  NOTICE_MESSAGE_STRINGS,
		  "Error - Bad Suffix on Save File.",
		  suggestion,
		  NULL,
		  NULL );
    xv_set( item,
	   PANEL_NOTIFY_STATUS, XV_ERROR,
	   NULL );
    return;
  }

  warning = OFF;
  if ( !query_flag( FILE_COMPRESSED ) ) {
    /* overwrite warnings -- 7/11/94 J.B. */
    fp2 = fopen(file, "r");
    if (fp2 != NULL)
      warning = ON;
    fclose(fp2);
  }
  else {    /* compression is ON */
    strcat(file, ".Z");		/* add .Z suffix to check for existing file */
    fp2 = fopen(file, "r");
    if (fp2 != NULL)
      warning = ON;
    fclose(fp2);
    address = strrchr(file, '.'); /* remove .Z suffiz */
    *address = '\0';
  }

  result = NOTICE_YES;
  if (warning == ON) {
    result = notice_prompt(panel, NULL,
			   NOTICE_MESSAGE_STRINGS,
			   	"File exists -- overwrite?", NULL,
			   NOTICE_BUTTON_YES, "Yes",
			   NOTICE_BUTTON_NO, "No",
			   NULL);
  }
  
  if (result == NOTICE_YES) {
    save_simulation( network_head, file );
    printf( "Done.\n" );
  }
  else
    printf("Save aborted.\n");
}  /* btn_save_now() */

void
btn_go_simulate( item, event )
    Panel_item	item;
    Event	*event;
{
  xv_set( item,
	 PANEL_NOTIFY_STATUS, XV_ERROR,
	 NULL );
  notify_stop();
  stop_simulation = FALSE;
}

void
btn_stop_simulate( item, event )
    Panel_item	item;
    Event	*event;
{
  xv_set( item,
	 PANEL_NOTIFY_STATUS, XV_ERROR,
	 NULL );
  stop_simulation = TRUE;
}

int
check_interrupt( )
{
  notify_dispatch();
  XFlush( dpy );

  if (stop_simulation)
    return NXI_STOP;

  return NXI_IGNORE;
}

/*
 *  Change parameters of a network
 */

void
change_parameters( item, event )
    Panel_item	item;
    Event	*event;
{
  float			value;
  char			*trans_func_arg;
  NETWORK		head;
  Panel			panel = (Panel) xv_get( item, PANEL_PARENT_PANEL );
  int			choice = (int) xv_get( parameter_type, PANEL_VALUE );
  
  head = network_head;
  
  if (choice == SET_TRANS_FUNC) /* rev. 4/13/92 */
    trans_func_arg = (char *) xv_get( parameter_value, PANEL_VALUE );
  else				/* rev. 12/27/91 */
    value = atof( (char *) xv_get( parameter_value, PANEL_VALUE ) );
  
  while (head != NULL && strcmp(head->name, parameter_network) != 0)
    head = head->next;

  if (head == NULL) {
    notice_prompt( panel, NULL,
	NOTICE_FOCUS_XY,
		get_rect_left( parameter_network_text ) + 10,
		get_rect_top( parameter_network_text ) + 5,
	NOTICE_MESSAGE_STRINGS,
		"ERROR: No such network.",
		NULL,
	NULL );
    xv_set( item,
	   PANEL_NOTIFY_STATUS, XV_ERROR,
	   NULL );
    return;
  }

  /*
   *  Modified into a switch(), cleaner & easier to read, much less xv_get()'s.
   *  DW 940711
   */  
  switch (choice) {

  case CLAMP_ON:
    set_clamp( head,
	      CLAMP_ON,
	      NULL );
    printf( "Network %s: clamped.\n", head->name );
    break;

  case CLAMP_OFF:
    set_clamp( head,
	      CLAMP_OFF,
	      NULL );
    printf( "Network %s: unclamped.\n", head->name );
    break;

  case SET_ACTIVITY:
    set_clamp( head,
	      SET_ACTIVITY,
	      value );					/* rev. 12/27/91 */
    printf( "Network %s: cells' activity set to %f.\n", head->name, value );
    if (curr_electrode == MODE_ACTIVITY)
      redrawGraphics( );
    break;

  case SET_TRANS_FUNC:
    set_trans_func( head,
		   SET_SCALE,
		   trans_func_arg );
    printf( "Network %s: transfer function set to %s.\n",
	   head->name, trans_func_arg );
    break;

  case SET_THRESHOLD:
    set_clamp( head,
	      SET_THRESHOLD,
	      value );					/* rev. 12/27/91 */
    printf( "Network %s: threshold set to %f.\n", head->name, value );
    break;

  case SET_SCALE:
    set_param( head,
	      SET_SCALE,
	      value );
    printf( "Network %s: scale set to %f.\n", head->name, value );
    break;

  case SET_OFFSET:
    set_param( head,
	      SET_OFFSET,
	      value );					/* rev. 12/27/91 */
    printf( "Network %s: offset set to %f.\n", head->name, value );
    break;
  
#if 0
  case SET_MIN:
    set_param( head,
	      SET_MIN,
	      value );
    break;

  case SET_MAX:
    set_param( head,
	      SET_MAX,
	      value );

  case SET_SLOPE:
    set_param( head,
	      SET_SLOPE,
	      value );
#endif 0  /* rev. 4/13/92 */
  
  case SET_NUM_UPDATES:
    head->evaluations_per_cycle = (int) value ;
    printf( "Network %s: # updates/cycle set to %d.\n",
	   head->name, (int) value );
    break;

  case SET_DECAY:
    set_param( head,
	      SET_DECAY,
	      value);		/* rev. 12/27/91 */
    printf( "Network %s: decay set to %f.\n", head->name, value );
    break;

  default:
    break;
  }
}

void
load_1_activity_now( item, event )
    Panel_item	item;
    Event	*event;
{
  /* rev. 12/27/91 all of filename length is set to NAME_SIZE */
  char			*name;
  FILE			*fp;
  Panel			panel = (Panel) xv_get( item, PANEL_PARENT_PANEL );
  NETWORK		head;

  
  printf( "Loading file <%s> . . .\n", activity_filename );
  fp = nxFopen( activity_filename );
  if (fp == NULL)
    notice_prompt( panel, NULL,
		  NOTICE_MESSAGE_STRINGS, "Error - No such file",
		  NULL, NULL );
  else {
    name = network_load_activity;
    head = network_head;
    while (head && strcmp( head->name, name ))
      head = head->next;
    if (head == NULL) {
      notice_prompt( panel, NULL,
		    NOTICE_MESSAGE_STRING,
			"Error: No such network.  Check name case.",
		    NULL );
      xv_set( item,
	     PANEL_NOTIFY_STATUS, XV_ERROR,
	     NULL );
      return;
    }
    
    load_activity_file( fp, head );
    printf( "\nDone\n" );
    fclose(fp);
  }
}  /* load_1_activity_now() */



void
randomize_networks( item, event )
    Panel_item	item;
    Event	*event;
{
  float min = (atof( (char *) xv_get( random_min, PANEL_VALUE ) ));
  float max = (atof( (char *) xv_get( random_max, PANEL_VALUE ) ));
  set_network( network_head, min, max );
}  /* randomize_networks( ) */



void
btn_edit_connection( item, event )
    Panel_item	item;
    Event	*event;
{
  Panel		panel = (Panel) xv_get( item, PANEL_PARENT_PANEL );
  extern char	*edit_connection();
  char		*result;



  if (result = edit_connection( edit_connect_network,
			       edit_connect_current_id,
			       edit_connect_function )) {
    notice_prompt( panel, NULL,
		  NOTICE_MESSAGE_STRING, result,
		  NULL );
    xv_set( item,
	   PANEL_NOTIFY_STATUS, XV_ERROR,
	   NULL );
  }
}



/*
 *  Show current network parameters
 */

void
btn_show_parameters( item, event )
    Panel_item	item;
    Event	*event;
{
  NETWORK		head;


  head = network_head;
  while (head && strcmp( head->name, parameter_network ))
    head = head->next;
  
  if (head)
    showParameters( head );
  else
    printf( "Error: network %s does not exist.\n", parameter_network );
}  /* btn_show_parameters() */



/*
 *  Used to be go_plot_filename( ), with plot_filename[].  DW 94.11.09
 */
void
go_activity_output( item, event )
    Panel_item	item;
    Event	*event;
{
  Panel		panel = (Panel) xv_get( item, PANEL_PARENT_PANEL);
  FILE		*fp;

  
  if (!(fp = fopen( activity_output_filename, "w" ))) {
    notice_prompt( panel, NULL,
		  NOTICE_MESSAGE_STRINGS, "Error - can't write to file.",
		  NULL, NULL );
    fclose( fp );
    xv_set( item,
	   PANEL_NOTIFY_STATUS, XV_ERROR,
	   NULL );
    return;
  }

  if (output_activity( fp ) == ERROR) {
    xv_set( item,
	   PANEL_NOTIFY_STATUS, XV_ERROR,
	   NULL );
    return;
  }
  fclose( fp );
}  /* go_activity_output( ) */


#if 0
void
go_plot_filename( item, event )
    Panel_item	item;
    Event	*event;
{
  extern char	plot_filename[];
  FILE		*fp,
  		*fopen();
  Panel		panel = (Panel) xv_get( item, PANEL_PARENT_PANEL);
  
  fp = fopen( plot_filename, "w" );

  if (fp == NULL) {
    notice_prompt( panel, NULL,
		  NOTICE_MESSAGE_STRINGS, "Error - can't write to file.",
		  NULL, NULL );
    fclose( fp );
    xv_set( item,
	   PANEL_NOTIFY_STATUS, XV_ERROR,
	   NULL );
    return;
  }

  plot_network_to_file( fp );
  fclose( fp );
  xv_set( frame_plot_filename,
	 XV_SHOW, FALSE,
	 NULL );
}
#endif 0








/*****************************************************************************
 *  PANEL_TEXT NOTIFY PROCedures;  DW 94.07.26
 *****************************************************************************/

/*
 *  proc_enter_name() is for any name entry PANEL_TEXT object.  DW 94.07
 *  Works for networks and filenames alike.  DW 94.08.26
 *    Update any associated list if it exists.  DW 94.08.29
 */

Panel_setting
proc_enter_name( item, event )
    Panel_item		item;
    Event		*event;
{
  Panel			panel = (Panel) xv_get( item, PANEL_PARENT_PANEL );
  Panel_item		list;
  static Frame		disabled_frames[1] = { NULL };
  int			i, state, nrows;
  char			entered_name[FUNCTION_SIZE + 25];
  char			temp_name[NAME_SIZE],
  			suggestion[NAME_SIZE + 13];
  char			*suffix;
  FILE			*fp, *fp2, *fopen( );   /* JB 94.07.11 */

  extern void		init_display( );
  extern int		checkFilenameSuffix( );





  if (panel == panel_filename) {
    state = (int) xv_get( panel, PANEL_CLIENT_DATA );
    strcpy( simulation_filename, (char *) xv_get( item, PANEL_VALUE ) );

    if (checkFilenameSuffix( state, simulation_filename, temp_name )) {
      strcpy( suggestion, "Suggestion: " );
      strcat( suggestion, temp_name );
      notice_prompt( panel, NULL,
		    NOTICE_FOCUS_XY, event_x( event ), event_y( event ),
		    NOTICE_MESSAGE_STRINGS,
		    "Error: Bad suffix in filename.",
		    suggestion,
		    NULL,
		    NULL );
      
      xv_set( item,
	     PANEL_NOTIFY_STATUS, XV_ERROR,
	     NULL );
      return;
    }

    if (state == BUILD) {
      fp = fopen( temp_name, "r" );
      if (fp == NULL) {
	notice_prompt( panel, NULL,
		      NOTICE_FOCUS_XY, event_x( event ), event_y( event ),
		      NOTICE_MESSAGE_STRINGS,
		      "Error - No such file",
		      NULL,
		      NULL );
	xv_set( item,
	       PANEL_NOTIFY_STATUS, XV_ERROR,
	       NULL );
	return;
      }
    }  /* state == BUILD */

    if (state == LOAD) {
      fp = fopen(temp_name,"r");	/* check if ascii save file exists */
      strcat(temp_name, ".Z");
      fp2 = fopen(temp_name, "r"); /* check if compressed save file exists */

      if(fp == NULL && fp2 == NULL) {   /* neither exist: */
	notice_prompt( panel, NULL,
		      NOTICE_FOCUS_XY, event_x( event ), event_y( event ),
		      NOTICE_MESSAGE_STRINGS, "Error - No such file",
		      NULL, NULL );
	fclose( fp );
	fclose( fp2 );
	xv_set( item,
	       PANEL_NOTIFY_STATUS, XV_ERROR,
	       NULL );
	return;
      }

      else if (fp != NULL && fp2 != NULL) {   /* both exist: */
	notice_prompt( panel, NULL,
		      NOTICE_FOCUS_XY, event_x( event ), event_y( event ),
		      NOTICE_MESSAGE_STRINGS,
		      "WARNING: file exists both in ASCII and compressed form.",
		      " Overriding compressed version, loading ASCII text file.",
		      NULL,
		      NULL );
	set_flag( FILE_COMPRESSED, FALSE );
      }

      else if (fp != NULL && fp2 == NULL)
	/* only ascii file exists */
	set_flag( FILE_COMPRESSED, FALSE );

      else if (fp == NULL && fp2 != NULL)
	/* only compressed file exists */
	set_flag( FILE_COMPRESSED, TRUE );


      fclose(fp);
      fclose(fp2);

      if ( !query_flag( FILE_COMPRESSED ) ) {
	suffix = strrchr(temp_name, '.');    /* remove .Z suffix */
	*suffix = '\0';
      }
    }  /* state = LOAD */

    xv_set( frame_filename,
	   XV_SHOW, FALSE,
	   NULL );

    /* BUILD */
    if (state == BUILD) {
      printf( "Loading Build file <%s>...", temp_name );
      if (read_network( fp ) == FALSE) {
	printf("ERROR: No .nx file was loaded.\n");
	xv_set( item,
	       PANEL_NOTIFY_STATUS, XV_ERROR,
	       NULL );
	return;
      }
      init_display( temp_name );
      printf( "Done.\n" );
    }

    /* LOAD */
    if (state == LOAD) {
      disable_frames( disabled_frames );
      printf( "Loading Saved simulation <%s>...", temp_name );
      load_simulation( temp_name );
      enable_frames( disabled_frames );
      reset_display( temp_name );
      printf( "Done.\n" );
    }

    return PANEL_NONE;
  } /* panel == panel_filename */





  strcpy( entered_name, (char *) xv_get( item, PANEL_VALUE ) );

  if ((int) xv_get( item, XV_KEY_DATA, KEY_UPDATE_LIST )) {
    list = (Panel_item) xv_get( item, XV_KEY_DATA, KEY_MY_LIST );
    nrows = (int) xv_get( list, PANEL_LIST_NROWS );

    if (strlen( entered_name ))
      for (i = 0; i < nrows; i++) {
	if (!strcmp( entered_name,
		    (char *) xv_get( list, PANEL_LIST_STRING, i ))) {
	  xv_set( list,
		 PANEL_LIST_SELECT, i, TRUE,
		 NULL );
	  strcpy( (char *) xv_get( item, PANEL_CLIENT_DATA ),
		 entered_name );
	  return PANEL_NEXT;
	}
      }
    else		/* There are no networks.  Don't do anything. */
      return PANEL_NEXT;

    strcat( entered_name, " is not a valid network." );
    notice_prompt( panel, NULL,
		  NOTICE_FOCUS_XY,
			(int) xv_get( item, XV_X ) + 100,
			(int) xv_get( item, XV_Y ),
		  NOTICE_MESSAGE_STRING,
			entered_name,
		  NULL );
    xv_set( item,
	   PANEL_NOTIFY_STATUS, XV_ERROR,
	   PANEL_VALUE, "",
	   NULL );
    return PANEL_NONE;
  }

  /*  Not a network name, and not a filename in panel_filename. */

  strcpy( (char *) xv_get( item, PANEL_CLIENT_DATA ),
	 entered_name );
  return PANEL_NEXT;
  
}  /* proc_enter_name() */


/*
 *  Assign the value of the text to the float variable pointed to by
 *  PANEL_CLIENT_DATA.  DW 940731
 */
Panel_setting
proc_enter_float( item, event )
    Panel_item		item;
    Event		*event;
{
  float			*which_variable;


  which_variable = (float *) xv_get( item, PANEL_CLIENT_DATA );
  (*which_variable) = (atof( (char *) xv_get( item, PANEL_VALUE ) ));

  if (which_variable == &conduct_compute_range_min
      || which_variable == &conduct_compute_range_max) {
    /*
     *  Backward compatibility hack.  DW 94.08.31
     */
    setting->conduct_min = conduct_compute_range_min;
    setting->conduct_max = conduct_compute_range_max;
    return PANEL_NEXT;
  }

  /*
   *  Make sure the current display reflects the change.
   *    Only if necessary. DW 94.09.09
   */
  if (which_variable != &change_dimension_amount)
    redrawGraphics( );

  return PANEL_NEXT;
}  /* proc_enter_float( ) */


/*
 *  Assign the value of the text to the int variable pointed to by
 *  PANEL_CLIENT_DATA, making sure it's positive.  DW 940801
 */
Panel_setting
proc_enter_pos_int( item, event )
    Panel_item item;
    Event *event;
{
  int		temp;


  temp = (int) xv_get( item, PANEL_VALUE );
  temp = (temp < 0 ? -(temp) : temp);

  (* (int *) xv_get( item, PANEL_CLIENT_DATA )) = temp;
  xv_set( item,
	 PANEL_VALUE, temp,
	 NULL );
  return PANEL_NEXT;
}


/*
 *  Assign the value of the text to the int variable pointed to by
 *  PANEL_CLIENT_DATA.  DW 940801
 */
Panel_setting
proc_enter_int( item, event )
    Panel_item item;
    Event *event;
{
  int *which_variable;

  which_variable = (int *) xv_get( item, PANEL_CLIENT_DATA );
  (*which_variable) = atoi( (char *) xv_get( item, PANEL_VALUE ) );
  return PANEL_NEXT;
}


/*
 *  Assign the value of the text to the string variable pointed to by
 *  PANEL_CLIENT_DATA, and update the associated list pointed to by
 *  XV_KEY_DATA, KEY_MY_LIST.  DW 940731
 */
Panel_setting
proc_enter_network( item, event )
    Panel_item item;
    Event *event;
{
  Panel_item my_list = (Panel_item) xv_get( item, XV_KEY_DATA, KEY_MY_LIST );

  strcpy( (char *) xv_get( item, PANEL_CLIENT_DATA ),
	 (char *) xv_get( item, PANEL_VALUE ) );
  return PANEL_NEXT;
}










/*****************************************************************************
 *  PANEL_LIST NOTIFY PROCedures:
 *  Place list selection into the appropriate PANEL_VALUE.
 *  Update associated text item.
 *****************************************************************************/

int
proc_selection( item, string, client_data, op, event, row )
    Panel_item		item;
    char		*string;
    Xv_opaque		client_data;
    Panel_list_op	op;
    Event		*event;
    int			row;
{
  if (op == PANEL_LIST_OP_SELECT) {
    xv_set( xv_get( item, XV_KEY_DATA, KEY_MY_TEXT ),
	   PANEL_VALUE, string,
	   NULL );
    strcpy( (char *) xv_get( item, PANEL_CLIENT_DATA ), string );
    if (item == list_networks_edit_connect)
      update_connect_id_list( string );
    return XV_OK;
  }
  else
    return XV_ERROR;
}

int
proc_select_id( item, string, client_data, op, event, row )
    Panel_item		item;
    char		*string;
    Xv_opaque		client_data;
    Panel_list_op	op;
    Event		*event;
    int			row;
{
  char		tempstr[FUNCTION_SIZE];

  strcpy( tempstr, "Connection: " );
  if (op == PANEL_LIST_OP_SELECT) {

    /*  Display while editing is not currently implemented. DW 94.08.29 */
#if 0
    if (string[4] == '-')	/* if prefix is "From-" */
      set_flag( CONNECT_SHOW_TYPE, RETROGRADE );
    else			/* if prefix is "To-" */
      set_flag( CONNECT_SHOW_TYPE, ANTEROGRADE );
#endif 0

    xv_set( edit_connect_id,
	   PANEL_LABEL_STRING, strcat( tempstr, string ),
	   NULL );
    edit_connect_current_id = (int) client_data;
  }
  else
    return XV_ERROR;
}










/*****************************************************************************
 *  PANEL_CHECK_BOX NOTIFY PROCedures;  DW 94.08.10
 *****************************************************************************/

/*
 *  Generic checkbox notify procedure.  DW 94.08.17
 */

void
proc_chbox_generic( item, value, event )
    Panel_item	item;
    int		value;
    Event	*event;
{
  *((int *) xv_get( item, PANEL_CLIENT_DATA )) = value;

  if (item == chbox_simulate_type)
    if (value == RANDOM)
      xv_set( simulate_random_cells,
	     PANEL_INACTIVE, FALSE,
	     NULL );
    else				/* SEQUENTIAL */
      xv_set( simulate_random_cells,
	     PANEL_INACTIVE, TRUE,
	     NULL );
}  /* proc_chbox_generic() */

/*
 *  Checkbox which specifies value of a binary flag.  Alters global flags.
 *  DW 94.08.26
 */

void
proc_chbox_binary( item, value, event )
    Panel_item	item;
    int		value;
    Event	*event;
{
  SET_FLAG( (flag_t) xv_get( item, PANEL_CLIENT_DATA ),
	   value );
}  /* proc_chbox_binary() */


/* 
 *  When the user changes the electrode's state, the display resets.  DW 940713
 */

void
checkbox_view_elect( item, value, event )
    Panel_item	item;
    int		value;
    Event	*event;
{
  if (value == ON && curr_electrode != MODE_VIEW) {
    set_electrode( MODE_VIEW );
    redrawGraphics();
  }
  else if (curr_electrode == MODE_VIEW)
    curr_electrode = MODE_OFF;
}

void
checkbox_activity_elect( item, value, event )
    Panel_item	item;
    int		value;
    Event	*event;
{
  int		old_electrode = curr_electrode;

  if (value == ON && old_electrode != MODE_ACTIVITY) {
    set_electrode( MODE_ACTIVITY );
    if (old_electrode == MODE_CONNECT)
      redrawGraphics();
    else if (old_electrode != MODE_OFF)
      draw_outline( FALSE );
  }
  else if (old_electrode == MODE_ACTIVITY)
    curr_electrode = MODE_OFF;
}

void
checkbox_connect_elect( item, value, event )
    Panel_item	item;
    int		value;
    Event	*event;
{
  if (value == ON && curr_electrode != MODE_CONNECT) {
    set_electrode( MODE_CONNECT );
    redrawGraphics();
  }
  else if (curr_electrode == MODE_CONNECT)
    curr_electrode = MODE_OFF;
}










/*****************************************************************************
 *  PANEL_SLIDER NOTIFY PROCedures;  DW 94.11.08
 *****************************************************************************/

void
proc_percentage_slider( item, value, event )
    Panel_item		item;
    int			value;
    Event		*event;
{
  *((int *) xv_get( item, PANEL_CLIENT_DATA )) = value;
}










/*****************************************************************************
 *  EVENT PROCedures
 *****************************************************************************/

/*
 * Allow the mouse to interact with both XVIEW and VOGLE
 * The state of the electrode (mouse) determines if connections
 * are displayed or cells can be stimulated 
 */

vogle_event_proc( item, event, arg )
    Panel_item item;
    Event *event;
    Notify_arg arg;
{
  extern NETWORK	select_network();
  extern NETWORK	move_network();

  Panel			panel = (Panel) xv_get( item, PANEL_PARENT_PANEL );




  /*
   *  State variable replaces "net" in nexus_main.c: is a network selected?
   *    If no, it is NULL; if yes, it points to the selected network.
   *  DW 94.07.12 (changed from 'int' flag to 'NETWORK' pointer, DW 94.08.01)
   */
  static NETWORK net_selected = NULL;

  if (curr_electrode == MODE_OFF)
    return;

  /*
   *  When a mouse button is released, skip everything.  DW 94.07.27
   *    locator() is a VOGLE-specific routine.
   */
  if ((locator_button = locator( &locator_x, &locator_y )) == 0)
    return;

  /*
   *  Don't accept LOC_DRAG (MotionNotify with a button pressed) in
   *    these circumstances:				DW 94.08.17
   */
  if (event_action( event ) == LOC_DRAG &&

      (curr_electrode == MODE_CONNECT

       || curr_electrode == MODE_VIEW))

/*       || (curr_electrode == MODE_ACTIVITY
	   && query_flag( ACTIVITY_STATE ) == EXAMINE))) */

    return;

  /*
   *  When a mouse button is pressed, this routine is called.
   *  We need to check to see what state we are in: 
   *    1) Display Connections
   *    2) Display Activity
   *    3) View Networks
   *
   *  Then we act accordingly.
   */

  /* Display Connections */
  
  if (curr_electrode == MODE_CONNECT) {

    if (query_flag( CONNECT_SHOW_TYPE ) == ANTEROGRADE) {
      notice_prompt( panel, NULL,
		    NOTICE_MESSAGE_STRINGS,
		    "Error - Anterograde option is not currently implemented.",
		    "Resetting to Retrograde", NULL,
		    NULL );
      set_flag( CONNECT_SHOW_TYPE, RETROGRADE );
    }

    do_connectivity( query_flag( CONNECT_SHOW_TYPE ),
		    query_flag( CONNECT_PRINT ),
		    query_flag( CONNECT_SAVE ) );

    return;
  }
  
  /* Display Activity */
  
  if (curr_electrode == MODE_ACTIVITY) {

    if (query_flag( ACTIVITY_STATE ) == CHANGE)
      modify_activity( setActivityPercentage / 100.0
		      * activity_display_range_max,
		      ACTIVITY,
		      SET );
    else
      modify_activity( 0.0,
		      ACTIVITY,
		      SHOW );

    return;
  }
  
  /* View Networks */

  if (curr_electrode == MODE_VIEW) {
    
    /*
     *  If no network is selected, try to select one.
     *  Else, try to move the previously selected network.
     */
    if (!net_selected)
      net_selected = select_network();
    else
      net_selected = move_network( net_selected );

    return;
  }
}










/*****************************************************************************
 *  Main loop, which has some toolkit-specific instructions in it.
 *****************************************************************************/

int
main_loop()
{
  Frame			disabled_frames[2];


  XFlush( dpy );
  
  /*
   *  notify_start() will only return when notify_stop() is called.
   *  This happens when
   *    (1)  The user presses GO in the Simulate frame: see btn_go_simulate();
   *    (2)  The user presses QUIT and answers YES to the resulting
   *           notice_prompt: see quit();
   */
  notify_start();

  /*
   *  If (1) then do the simulation.
   *  This section was btn_go_simulate()...DW 94.08.10
   */
  if (do_exit == FALSE) {
    disabled_frames[0]=frame_simulate;
    disabled_frames[1]=NULL;
    disable_frames( disabled_frames );

    /*
     *  Prevent user from cancelling simulate panel, thereby not being
     *    able to STOP the simulation!  Bug fix, DW 94.08.18
     *  Also during simulation, disable various items on simulate panel
     *    which wouldn't make sense to change during the simulation.
     */
    disable_panel_item( simulate_go );
    enable_panel_item( simulate_stop );
    disable_panel_item( simulate_cancel );
    disable_panel_item( chbox_simulate_type );
    disable_panel_item( simulate_batch_learn_chbox );
    disable_panel_item( simulate_swap_rates_chbox );
    disable_panel_item( simulate_number_cycles );
    disable_panel_item( simulate_random_cells );

    init_extern_connections( network_head );

    /* Check added DW 940817 */
    if (run_simulation( number_cycles, random_cells, simulate_type) == OK)
      printf( "\n**********    Simulation finished.    **********\n" );
    else
      printf( "\n**********    Simulation ABORTED.    **********\n" );

    set_electrode( MODE_ACTIVITY );
    if (display_type == DISP_OFF)
      graph_activity( );

    /*
     *  Return buttons and other panels, to their original state.
     */
    enable_panel_item( simulate_go );
    disable_panel_item( simulate_stop );
    enable_panel_item( simulate_cancel );
    enable_panel_item( chbox_simulate_type );
    enable_panel_item( simulate_batch_learn_chbox );
    enable_panel_item( simulate_swap_rates_chbox );
    enable_panel_item( simulate_number_cycles );
    if (simulate_type == RANDOM)
      enable_panel_item( simulate_random_cells );
    disabled_frames[0] = NULL;
    enable_frames( disabled_frames );

    return OK;
  }

  /*
   *  If (2) then return ERROR to exit main loop (see nexus_main.c).
   */
  return ERROR;
}










/*****************************************************************************
 *  Start of init_objects().  DW 940715
 *  Initialize xview first, then begin with top level frame ("frame").
 *****************************************************************************/


int
init_objects( argc_p, argv )
    int *argc_p;
    char *argv[];
{
  extern Panel	panel_simulate;
  Panel 	panel1,
        	panel2,
  		panel3,
        	panel4,
        	panel_view1,
		panel_view2,
		panel_view3,
        	panel_connect,
        	panel_connect_message,
        	panel_activity,
        	panel_clamp,
        	panel_save,
        	panel_edit_connect,
        	panel_random;

/*        	panel_plot,
        	panel_plot_filename,
        	panel_print_screen,
		panel_graphics_filename; */

  Panel		temp_panel;

  Window 	window;	/* vogle window */
  Scrollbar	h_scrollbar,
  		v_scrollbar;

  Rect 		 rect;	/* multi-purpose, for object placement and sizing */

  Panel_item	temp_item1, temp_item2, temp_item3, temp_item4, temp_item5,
  		temp_item6, temp_item7, temp_item8, temp_item9, temp_item0;

  Panel_item	temp_text, temp_list;
  int		width, temp1, temp2, temp3;


  /***********
   *  External function declarations
   ***********/
  
  /* nexus_plot.c */
  extern int	plot(),
		do_plot(),
		do_plot_now(),
  		quit_plot(),
  		move_plot_x(),
  		move_plot_y(),
  		move_plot_z(),
  		rotate_x(),
  		rotate_y(),
  		rotate_z(),
  		reset_view(),
		print_screen_menu(),
  		print_screen();
/*		go_graphics_filename(); */

  extern int	plot_event_proc();

  /* nexus_*.c, where * is each learning method */
  extern int 	setup_menu_bp();
  extern int 	setup_menu_hebb();
  extern int 	setup_menu_inversion();
  extern int 	setup_menu_rbf();
  extern void	go_backprop(),
		go_hebb(),
  		go_inversion(),
  		go_rbf();

  /* nexus_vogle.c */
  extern void     init_graphics();
  extern void     init_graphics_window();
  extern void     init_plot_window();





  /***********
   *  External global variable declarations
   ***********/

  /*  nexus_plot.c */
/*  extern char	plot_filename[];
    extern char	plot_network_name[]; */



  /***********
   *  Misc.
   ***********/

  char *temp_str = "00000000000000000000";
  Xv_singlecolor bg;










  /********************************************************************
   *  Create objects.  Must initialize Xview first.
   ********************************************************************/


  /*
   * general background colour for all panels
   * only needs to be set in top frame; all other children adopt
   * these colours (set WIN_INHERIT_COLORS to TRUE)
   */
  bg.red = 204, bg.green = 204, bg.blue = 204;

  printf( "\nXview is initializing.  Please wait..." );
  xv_init( XV_INIT_ARGC_PTR_ARGV, argc_p, argv, NULL );

  printf( "Done.\nNEXUS is setting up graphics images.  Please wait..." );

  /*
   *  First, server images.
   */

  nexus_image = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, nexus_icon_width,
				XV_HEIGHT, nexus_icon_height,
				SERVER_IMAGE_X_BITS, nexus_icon_bits,
				NULL );

  nexus_image2 = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, nexus2_icon_width,
				XV_HEIGHT, nexus2_icon_height,
				SERVER_IMAGE_X_BITS, nexus2_icon_bits,
				NULL );

  nexus_image3 = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, nexus3_icon_width,
				XV_HEIGHT, nexus3_icon_height,
				SERVER_IMAGE_X_BITS, nexus3_icon_bits,
				NULL );

  nexus_image4 = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, nexus4_icon_width,
				XV_HEIGHT, nexus4_icon_height,
				SERVER_IMAGE_X_BITS, nexus4_icon_bits,
				NULL );

  pan_left_image = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, pan_left_width,
				XV_HEIGHT, pan_left_height,
				SERVER_IMAGE_X_BITS, pan_left_bits,
				NULL );

  pan_right_image = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, pan_right_width,
				XV_HEIGHT, pan_right_height,
				SERVER_IMAGE_X_BITS, pan_right_bits,
				NULL );

  pan_up_image = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, pan_up_width,
				XV_HEIGHT, pan_up_height,
				SERVER_IMAGE_X_BITS, pan_up_bits,
				NULL );

  pan_down_image = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, pan_down_width,
				XV_HEIGHT, pan_down_height,
				SERVER_IMAGE_X_BITS, pan_down_bits,
				NULL );

  zoom_in_image = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, zoom_in_width,
				XV_HEIGHT, zoom_in_height,
				SERVER_IMAGE_X_BITS, zoom_in_bits,
				NULL );

  zoom_out_image = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, zoom_out_width,
				XV_HEIGHT, zoom_out_height,
				SERVER_IMAGE_X_BITS, zoom_out_bits,
				NULL );

  rotate_x_clk_image = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, rotate_x_clk_width,
				XV_HEIGHT, rotate_x_clk_height,
				SERVER_IMAGE_X_BITS, rotate_x_clk_bits,
				NULL );

  rotate_x_cclk_image = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, rotate_x_cclk_width,
				XV_HEIGHT, rotate_x_cclk_height,
				SERVER_IMAGE_X_BITS, rotate_x_cclk_bits,
				NULL );

  rotate_y_clk_image = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, rotate_y_clk_width,
				XV_HEIGHT, rotate_y_clk_height,
				SERVER_IMAGE_X_BITS, rotate_y_clk_bits,
				NULL );

  rotate_y_cclk_image = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, rotate_y_cclk_width,
				XV_HEIGHT, rotate_y_cclk_height,
				SERVER_IMAGE_X_BITS, rotate_y_cclk_bits,
				NULL );

  rotate_z_clk_image = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, rotate_z_clk_width,
				XV_HEIGHT, rotate_z_clk_height,
				SERVER_IMAGE_X_BITS, rotate_z_clk_bits,
				NULL );

  rotate_z_cclk_image = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, rotate_z_cclk_width,
				XV_HEIGHT, rotate_z_cclk_height,
				SERVER_IMAGE_X_BITS, rotate_z_cclk_bits,
				NULL );

  carriage_return_image = (Server_image) xv_create( NULL, SERVER_IMAGE,
				XV_WIDTH, carriage_return_width,
				XV_HEIGHT, carriage_return_height,
				SERVER_IMAGE_X_BITS, carriage_return_bits,
				NULL );



  






  /********************************************************************
   *  Create main frame, graphics frame, plot frame.
   *  Create sub-objects which are displayed in these frames.
   ********************************************************************/


  /*
   * Main control frame; anchor all others from this one.
   */
  
  frame = (Frame) xv_create( NULL, FRAME,
			    FRAME_LABEL, "NEXUS Control",
			    WIN_INHERIT_COLORS, TRUE,
			    FRAME_BACKGROUND_COLOR, &bg,
			    NULL );

  /*
   * Vogle Graphics Frame
   *      Contains:
   *           Canvas, Horizontal & Vertical Scrollbars
   *
   *      The canvas is what is actually drawn on.
   */
  
  /* 
   * Create an XVIEW canvas and use VOGLE graphics calls to write to it.  
   * We must rewrite the VOGLE library to change X11_init so that 
   * VOGLE does not create its own window.
   */
  
  frame_vogle = (Frame) xv_create( frame, FRAME,
				  FRAME_LABEL, "NEXUS Graphics",
				  FRAME_WM_COMMAND_STRINGS, -1, NULL,
				  WIN_INHERIT_COLORS, TRUE,
				  FRAME_BACKGROUND_COLOR, &bg,
				  FRAME_MIN_SIZE, 200, 0,
				  NULL );
  /* see nexus_vogle.h for the constants */
  vogle_canvas = (Canvas) xv_create( frame_vogle, CANVAS,
				    CANVAS_AUTO_SHRINK, FALSE,
				    CANVAS_AUTO_EXPAND, FALSE,
				    CANVAS_WIDTH, V_WIN_WIDTH,
				    CANVAS_HEIGHT, V_WIN_HEIGHT,
				    CANVAS_AUTO_CLEAR, FALSE,
				    XV_WIDTH, XV_WIN_WIDTH,
				    XV_HEIGHT, XV_WIN_HEIGHT,
				    NULL );
  h_scrollbar = (Scrollbar)xv_create( vogle_canvas, SCROLLBAR,
				     SCROLLBAR_DIRECTION, SCROLLBAR_HORIZONTAL,
				     SCROLLBAR_SPLITTABLE, TRUE,
				     NULL );
  v_scrollbar = (Scrollbar)xv_create( vogle_canvas, SCROLLBAR,
				     SCROLLBAR_DIRECTION, SCROLLBAR_VERTICAL,
				     SCROLLBAR_SPLITTABLE, FALSE,
				     NULL );
  window_fit( frame_vogle );






#if 0
  /*
   * Plot Graphics Frame
   *      Contains:
   *           Canvas
   *
   *      The canvas is what is actually drawn on.
   */
  
  frame_plot = (Frame) xv_create( frame, FRAME,
				 FRAME_LABEL, "NEXUS Plot Graphics",
				 FRAME_WM_COMMAND_STRINGS, -1, NULL,
				 WIN_INHERIT_COLORS, TRUE,
				 FRAME_BACKGROUND_COLOR, &bg,
				 FRAME_NO_CONFIRM, FALSE,
				 NULL );
  plot_canvas = (Canvas) xv_create( frame_plot, CANVAS,
				   CANVAS_AUTO_SHRINK, FALSE,
				   CANVAS_AUTO_EXPAND, FALSE,
				   CANVAS_WIDTH, 400,  
				   CANVAS_HEIGHT,400,
				   CANVAS_AUTO_CLEAR, FALSE,
				   XV_WIDTH, 400,
				   XV_HEIGHT, 400,
				   NULL );
  window_fit( frame_plot );
#endif 0



 
  /*
   *  Frame: Simulation Filename
   *  Used by: Load Simulation
   *           Build Simulation
   *  Parent: Simulation Control (panel1)
   *
   *  Default value is ".nx".
   */
  
  frame_filename = (Frame) xv_create( frame, FRAME,
				     FRAME_LABEL, "Simulation filename",
				     FRAME_WM_COMMAND_STRINGS, -1, NULL,
				     WIN_INHERIT_COLORS, TRUE,
				     NULL );
  panel_filename = (Panel) xv_create( frame_filename, PANEL,
				     PANEL_LAYOUT, PANEL_HORIZONTAL,
				     PANEL_CLIENT_DATA, BUILD,
				     XV_WIDTH, 1000,
				     XV_HEIGHT, 1000,
				     NULL );
  simulation_filename_text = (Panel_item)
    xv_create( panel_filename,
	      PANEL_NAME_TEXT( simulation_filename ),
	      PANEL_LABEL_STRING,	"Simulation File",
	      PANEL_VALUE,		simulation_filename,
	      XV_X,			10,
	      XV_Y,			10,
	      NULL );
  (void) xv_create( panel_filename, PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE, carriage_return_image,
		   NULL );
  temp_item1 = xv_create( panel_filename, PANEL_BUTTON,
			 PANEL_LABEL_STRING, "Cancel",
			 PANEL_NEWLINE, 20,
			 PANEL_NOTIFY_PROC, proc_btn_cancel,
			 PANEL_CLIENT_DATA, frame_filename,
			 NULL );
  window_fit( panel_filename );
  window_fit( frame_filename );
  width = (get_rect_width( panel_filename )
	   - get_rect_width( temp_item1 )) / 2;
  xv_set( temp_item1,
	 XV_X, width,
	 NULL );





  /*
   *  Frame: Simulate
   *  Used by: Simulate
   *  Parent: Simulation Control (panel1)
   *
   *  frame to set simulation parameters (i.e. number cycles, cells/cycle)
   *  and type of updating scheme (i.e. random or sequential)
   *  and type of realtime activity display.     (10.31.91)
   */
  
  frame_simulate = (Frame)
    xv_create( frame,			SUBFRAME,
	      FRAME_LABEL,		"Simulate",
	      NULL );
  panel_simulate = (Panel)
    xv_create( frame_simulate,		PANEL,
	      PANEL_LAYOUT,		PANEL_HORIZONTAL,
	      OPENWIN_SHOW_BORDERS,	TRUE,
	      NULL );

  simulate_go =
    (Panel_item) xv_create( panel_simulate, PANEL_BUTTON,
			   PANEL_LABEL_STRING, "GO",
			   XV_Y, 10,
			   XV_X, 10,
			   PANEL_NOTIFY_PROC, btn_go_simulate,
			   NULL );

  simulate_stop =
    (Panel_item) xv_create( panel_simulate, PANEL_BUTTON,
			   PANEL_LABEL_STRING, "STOP",
			   PANEL_NOTIFY_PROC, btn_stop_simulate,
			   PANEL_INACTIVE, TRUE,
			   NULL );

  simulate_cancel =
    (Panel_item)  xv_create( panel_simulate, PANEL_BUTTON,
			    PANEL_LABEL_STRING, "Cancel",
			    PANEL_NOTIFY_PROC, proc_btn_cancel,
			    PANEL_CLIENT_DATA, frame_simulate,
			    NULL );

  chbox_simulate_type =
    (Panel_item) xv_create( panel_simulate,		PANEL_CHECK_BOX,
			   PANEL_CHOOSE_ONE,		TRUE,
			   PANEL_LAYOUT,		PANEL_VERTICAL,
			   PANEL_NEWLINE,		10,
			   PANEL_LABEL_STRING,		"Updating: ",
			   PANEL_CHOICE_STRINGS,
				"Sequential",
				"Random",
				NULL,
			   PANEL_NOTIFY_PROC,		proc_chbox_generic,
			   PANEL_CLIENT_DATA,		&simulate_type,
			   PANEL_VALUE,			simulate_type,
			   NULL );  

  simulate_swap_rates_chbox = (Panel_item)
    xv_create( panel_simulate,		PANEL_CHECK_BOX,
	      PANEL_CHOOSE_ONE,		TRUE,
	      PANEL_LAYOUT,		PANEL_VERTICAL,
	      PANEL_NEWLINE,		10,
	      PANEL_LABEL_STRING,	"Swap Firing Rate ",
	      PANEL_CHOICE_STRINGS,	"OFF", "ON", NULL,
	      PANEL_NOTIFY_PROC,	proc_chbox_binary,
	      PANEL_CLIENT_DATA,	ACTIVITY_SWAP,
	      PANEL_VALUE,		query_flag( ACTIVITY_SWAP ),
	      NULL );

  simulate_batch_learn_chbox = (Panel_item)
    xv_create( panel_simulate,		PANEL_CHECK_BOX,
	      PANEL_CHOOSE_ONE,		TRUE,
	      PANEL_LAYOUT,		PANEL_VERTICAL,
	      PANEL_NEWLINE,		10,
	      PANEL_LABEL_STRING,	"Batch Learning",
	      PANEL_CHOICE_STRINGS,	"OFF", "ON", NULL,
	      PANEL_NOTIFY_PROC,	proc_chbox_binary,
	      PANEL_CLIENT_DATA,	LEARN_BATCH,
	      PANEL_VALUE,		query_flag( LEARN_BATCH ),
	      NULL );  

  (void) xv_create( panel_simulate,		PANEL_CHECK_BOX,
		   PANEL_LABEL_STRING,		"Realtime Display",
		   PANEL_NEWLINE,		10,
		   PANEL_NOTIFY_PROC,		proc_chbox_generic,
		   PANEL_CLIENT_DATA,		&display_type,
		   PANEL_VALUE,			display_type,
		   PANEL_CHOOSE_ONE,		TRUE,
		   PANEL_LAYOUT,		PANEL_VERTICAL,
		   PANEL_CHOICE_STRINGS,
		   	"OFF",			/* DISP_OFF */
		   	"CELL by CELL",		/* DISP_CELL */
		   	"NETWORK by NETWORK",	/* DISP_NET */
		   NULL,
		   NULL );

  sprintf( temp_str, "%d", number_cycles );
  simulate_number_cycles =
    (Panel_item) xv_create( panel_simulate,		PANEL_TEXT,
			   PANEL_LABEL_STRING,		"# cycles",
			   PANEL_NEWLINE,		20,
			   PANEL_NOTIFY_PROC,		proc_enter_int,
			   PANEL_CLIENT_DATA,		&number_cycles,
			   PANEL_VALUE,			temp_str,
			   PANEL_VALUE_DISPLAY_LENGTH,	8,
			   NULL );
  (void) xv_create( panel_simulate, PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE, carriage_return_image,
		   NULL );

  (void) xv_create( panel_simulate,	PANEL_MESSAGE,
		   PANEL_LABEL_STRING,	"Random updating:",
		   PANEL_NEWLINE,	20,
		   PANEL_LABEL_BOLD,	TRUE,
		   NULL );

  sprintf( temp_str, "%d", random_cells );
  simulate_random_cells =
    (Panel_item) xv_create( panel_simulate,		PANEL_TEXT,
			   PANEL_LABEL_STRING,		"# cells",
			   PANEL_NEWLINE,		10,
			   PANEL_NOTIFY_PROC,		proc_enter_int,
			   PANEL_CLIENT_DATA,		&random_cells,
			   PANEL_VALUE,			temp_str,
			   PANEL_VALUE_DISPLAY_LENGTH,	8,
			   PANEL_INACTIVE,		TRUE,
			   NULL );
  (void) xv_create( panel_simulate, PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE, carriage_return_image,
		   NULL );

  window_fit( panel_simulate );
  window_fit( frame_simulate );
  






  /*
   *  Frame: Connection Display
   *  Used by: Connectivity
   *  Parent: Electrode Control (panel2)
   *
   *  frame for displaying simulation connectivity.  Choice between 
   *  retrograde (input) or anterograde (output) connections.  All
   *  connections are shown with retrograde selected but only those established
   *  with 'connect_to' are displayed with anterograde. Note that 
   *  left button selects and right button quits display routine.  This quit
   *  routine will be used until VOGLE and XVIEWS are interfaced correctly.
   */
  
  frame_connect = (Frame) xv_create( frame, FRAME,
				    FRAME_LABEL, "Connection display",
				    FRAME_WM_COMMAND_STRINGS, -1, NULL,
				    XV_WIDTH, 1000,
				    XV_HEIGHT, 1000,
				    NULL );
  panel_connect = (Panel) xv_create( frame_connect, PANEL, 
				    OPENWIN_SHOW_BORDERS, TRUE,
				    PANEL_LAYOUT, PANEL_HORIZONTAL,
				    NULL );
  connect_electrode = (Panel_item) xv_create( panel_connect, PANEL_CHECK_BOX,
					     PANEL_CHOOSE_ONE, TRUE,
					     PANEL_LAYOUT, PANEL_VERTICAL,
					     XV_X, 10,
					     XV_Y, 10,
					     PANEL_LABEL_STRING, "Electrode ",
					     PANEL_CHOICE_STRINGS,
					     	"OFF", "ON", NULL,
					     PANEL_NOTIFY_PROC,
					     	checkbox_connect_elect,
					     PANEL_VALUE, OFF,
					     NULL ); 
  chbox_connect_show_type = (Panel_item)
    xv_create( panel_connect,		PANEL_CHECK_BOX,
	      PANEL_CHOOSE_ONE,		TRUE,
	      PANEL_LAYOUT,		PANEL_VERTICAL,
	      XV_X,		      get_rect_right( connect_electrode ) + 30,
	      PANEL_LABEL_STRING,	"Show ",
	      PANEL_CHOICE_STRINGS,	"Anterograde", "Retrograde", NULL,
	      PANEL_NOTIFY_PROC,	proc_chbox_binary,
	      PANEL_CLIENT_DATA,	CONNECT_SHOW_TYPE,
	      PANEL_VALUE,		query_flag( CONNECT_SHOW_TYPE ),
	      PANEL_INACTIVE,		TRUE,
	      NULL ); 

  (void) xv_create( panel_connect,	PANEL_MESSAGE,
		   PANEL_LABEL_STRING,	"Output weights...",
		   PANEL_NEWLINE,	20,
		   PANEL_LABEL_BOLD,	TRUE,
		   NULL );
		   

  (void) xv_create( panel_connect,		PANEL_CHECK_BOX,
		   PANEL_CHOOSE_ONE,		TRUE,
		   PANEL_NEWLINE,		10,
		   PANEL_LABEL_STRING,		"   to Screen",
		   PANEL_CHOICE_STRINGS,	"OFF", "ON", NULL,
		   PANEL_NOTIFY_PROC,		proc_chbox_binary,
		   PANEL_CLIENT_DATA,		CONNECT_PRINT,
		   PANEL_VALUE,			query_flag( CONNECT_PRINT ),
		   NULL ); 

  (void) xv_create( panel_connect,		PANEL_CHECK_BOX,
		   PANEL_CHOOSE_ONE,		TRUE,
		   PANEL_NEWLINE,		10,
		   PANEL_LABEL_STRING,		"   to File",
		   PANEL_CHOICE_STRINGS,	"OFF", "ON", NULL,
		   PANEL_NOTIFY_PROC,		proc_chbox_binary,
		   PANEL_CLIENT_DATA,		CONNECT_SAVE,
		   PANEL_VALUE,			query_flag( CONNECT_SAVE ),
		   NULL ); 

  (void) xv_create( panel_connect,		PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Filename: ",
		   PANEL_NEWLINE,		10,
		   XV_X,			30,
		   PANEL_NOTIFY_PROC,		proc_enter_name,
		   PANEL_CLIENT_DATA,		send_connections_filename,
		   PANEL_VALUE,			send_connections_filename,
		   PANEL_VALUE_DISPLAY_LENGTH,	NAME_DISPLAY_SIZE,
		   PANEL_VALUE_STORED_LENGTH,	NAME_SIZE,
		   NULL ); 
  (void) xv_create( panel_connect, PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE, carriage_return_image,
		   NULL );

  (void) xv_create( panel_connect, PANEL_MESSAGE,
		   PANEL_LABEL_STRING, "Conductivity range adjustment:",
		   PANEL_NEWLINE, 20,
		   PANEL_LABEL_BOLD, TRUE,
		   NULL );
  (void) xv_create( panel_connect, PANEL_MESSAGE,
		   PANEL_LABEL_STRING, "Display",
		   PANEL_NEXT_ROW, 15,
		   XV_X, 20,
		   PANEL_LABEL_BOLD, FALSE,
		   NULL );

  sprintf( temp_str, "%9.3f", DEFAULT_CONDUCTANCE_MIN );
  (void) xv_create( panel_connect,		PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Min. Value: ",
		   PANEL_NEXT_ROW,		8,
		   XV_X,			30,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&conduct_display_range_min,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( panel_connect, PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE, carriage_return_image,
		   NULL );

  sprintf( temp_str, "%9.3f", DEFAULT_CONDUCTANCE_MAX );
  (void) xv_create( panel_connect,		PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Max. Value: ",
		   PANEL_NEXT_ROW,		5,
		   XV_X,			30,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&conduct_display_range_max,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( panel_connect, PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE, carriage_return_image,
		   NULL );

  (void) xv_create( panel_connect, PANEL_MESSAGE,
		   PANEL_LABEL_STRING, "Computation",
		   PANEL_NEXT_ROW, 15,
		   XV_X, 20,
		   PANEL_LABEL_BOLD, FALSE,
		   NULL );

  sprintf( temp_str, "%9.3f", DEFAULT_CONDUCTANCE_MIN );
  (void) xv_create( panel_connect,		PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Min. Value: ",
		   PANEL_NEXT_ROW,		8,
		   XV_X,			30,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&conduct_compute_range_min,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( panel_connect, PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE, carriage_return_image,
		   NULL );

  sprintf( temp_str, "%9.3f", DEFAULT_CONDUCTANCE_MAX );
  (void) xv_create( panel_connect,		PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Max. Value: ",
		   PANEL_NEXT_ROW,		5,
		   XV_X,			30,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&conduct_compute_range_max,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( panel_connect, PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE, carriage_return_image,
		   NULL );


  temp_item1 = (Panel_item) xv_create( panel_connect, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Cancel",
				      PANEL_NEXT_ROW, 20,
				      PANEL_NOTIFY_PROC, proc_btn_cancel,
				      PANEL_CLIENT_DATA, frame_connect,
				      NULL );

  window_fit( panel_connect );
  window_fit( frame_connect );
  xv_set( temp_item1,
	 XV_X, (int) ((get_rect_width( panel_connect )
		       - get_rect_width( temp_item1 )) / 2),
	 NULL );






  /*
   * frame to change activity of given cells.  Can be used as simple 
   * input to the simulation.
   *
   * min, max #define'd in nexus.h, eliminated settings structure DW 940721
   * range min, max #define'd in nexus_vogle.h  DW 940718
   */
 
  frame_activity = (Frame) xv_create( frame, FRAME,
				     FRAME_LABEL, "Modify Cell Activity",
				     FRAME_INHERIT_COLORS, TRUE,
				     FRAME_WM_COMMAND_STRINGS, -1, NULL,
				     XV_WIDTH, 1000,
				     XV_HEIGHT, 1000,
				     NULL );
  panel_activity = (Panel) xv_create( frame_activity, PANEL,
				     PANEL_LAYOUT, PANEL_HORIZONTAL,
				     OPENWIN_SHOW_BORDERS, TRUE,
				     NULL );
  activity_electrode = (Panel_item)
    xv_create( panel_activity, PANEL_CHECK_BOX,
	      PANEL_CHOOSE_ONE, TRUE,
	      PANEL_LAYOUT, PANEL_VERTICAL,
	      XV_X, 10,
	      XV_Y, 10,
	      PANEL_LABEL_STRING, "Electrode ",
	      PANEL_CHOICE_STRINGS, "OFF", "ON", NULL,
	      PANEL_NOTIFY_PROC, checkbox_activity_elect,
	      PANEL_VALUE, OFF,
	      NULL );

  (void) xv_create( panel_activity,		PANEL_CHECK_BOX,
		   PANEL_CHOOSE_ONE,		TRUE,
		   PANEL_LAYOUT,		PANEL_VERTICAL,
		   XV_X,	     get_rect_right( activity_electrode ) + 30,
		   PANEL_LABEL_STRING,		"Electrode ",
		   PANEL_CHOICE_STRINGS,	"Change", "Examine", NULL,
		   PANEL_NOTIFY_PROC,		proc_chbox_binary,
		   PANEL_CLIENT_DATA,		ACTIVITY_STATE,
		   PANEL_VALUE,			query_flag( ACTIVITY_STATE ),
		   NULL );

  (void) xv_create( panel_activity, PANEL_MESSAGE,
		   PANEL_LABEL_STRING, "Activity Range Adjustment:",
		   PANEL_NEWLINE, 20,
		   PANEL_LABEL_BOLD, TRUE,
		   NULL );

  sprintf( temp_str, "%9.3f", DEFAULT_ACTIVITY_MIN );
  (void) xv_create( panel_activity,		PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Min. Value: ",
		   PANEL_NEXT_ROW,		10,
		   XV_X,			30,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&activity_display_range_min,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( panel_activity,		PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  sprintf( temp_str, "%9.3f", DEFAULT_ACTIVITY_MAX );
  (void) xv_create( panel_activity,		PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Max. Value: ",
		   PANEL_NEXT_ROW,		10,
		   XV_X,			30,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&activity_display_range_max,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( panel_activity,		PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  /*
   *  Since XView sliders can't handle floating point values, the
   *    functionality has been changed to PERCENTAGE of Maximum Firing Rate.
   *    DW 94.10.10
   */
  (void) xv_create( panel_activity, PANEL_MESSAGE,
		   PANEL_LABEL_STRING,		"Firing Rate Adjustment, as a percentage\nof the Activity Range defined above:",
		   PANEL_NEWLINE,		20,
		   PANEL_LABEL_BOLD,		TRUE,
		   NULL );

  (void) xv_create( panel_activity,
		   PANEL_PERCENTAGE_SLIDER( setActivityPercentage ),
		   PANEL_LABEL_STRING,		"Percentage: ",
		   PANEL_NEWLINE,		10,
		   XV_X,			30,
		   PANEL_MIN_VALUE_STRING,	"0%",
		   PANEL_MAX_VALUE_STRING,	"100%",
		   NULL );
  (void) xv_create( panel_activity,		PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  chbox_activity_record = (Panel_item)
    xv_create( panel_activity,			PANEL_CHECK_BOX,
	      PANEL_CHOOSE_ONE,			TRUE,
	      PANEL_NEWLINE,			50,
	      PANEL_LABEL_STRING,		"Cycle Record: ",
	      PANEL_CHOICE_STRINGS,		"OFF", "ON", NULL,
	      PANEL_NOTIFY_PROC,		proc_chbox_binary,
	      PANEL_CLIENT_DATA,		ACTIVITY_CYCLE,
	      PANEL_VALUE,			query_flag( ACTIVITY_CYCLE ),
	      NULL );

  temp_text = (Panel_item)
    xv_create( panel_activity,
	      PANEL_NETWORK_TEXT( activity_output_network_name ),
	      PANEL_NEWLINE,			20,
	      NULL );
  list_networks_activity_output = (Panel_item)
    xv_create( panel_activity,
	      PANEL_NETWORK_LIST( temp_text, activity_output_network_name ),
	      PANEL_NEWLINE,			10,
	      NULL );
  xv_set( temp_text,
	 XV_KEY_DATA, KEY_MY_LIST, list_networks_activity_output,
	 NULL );

  (void) xv_create( panel_activity,
		   PANEL_NAME_TEXT( activity_output_filename ),
		   PANEL_LABEL_STRING,		"Activity Output File: ",
		   PANEL_VALUE,			activity_output_filename,
		   PANEL_NEWLINE,		10,
		   NULL );
  (void) xv_create( panel_activity,		PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  /*
   *  This list MUST match the defined values in nexus.h.  DW 94.11.09
   */

  (void) xv_create( panel_activity,		PANEL_CHECK_BOX,
		   PANEL_CHOOSE_ONE,		TRUE,
		   PANEL_NEWLINE,		10,
		   PANEL_LABEL_STRING,		"Output Precision: ",
		   PANEL_CHOICE_STRINGS,
		   	"x.x",
		   	"x.xxx",
		   	"x.xxxxxx",
		   	NULL,
		   PANEL_NOTIFY_PROC,		proc_chbox_generic,
		   PANEL_CLIENT_DATA,		&activity_output_precision,
		   PANEL_VALUE,			activity_output_precision,
		   NULL );

  (void) xv_create( panel_activity,		PANEL_BUTTON,
		   PANEL_NEWLINE,		20,
		   PANEL_LABEL_STRING,		"Output Activity Now",
		   PANEL_NOTIFY_PROC,		go_activity_output,
		   NULL ); 

  (void) xv_create( panel_activity,		PANEL_BUTTON,
		   PANEL_LABEL_STRING,		"Cancel",
		   PANEL_NOTIFY_PROC,		proc_btn_cancel,
		   PANEL_CLIENT_DATA,		frame_activity,
		   NULL );
  window_fit( panel_activity );
  window_fit( frame_activity );






  /*
   * frame for tramsforming viewing perspective of simulation structure
   * The frame comes with a movement value and three buttons to determine 
   * x, y or z translation.  Note that +z is out of screen. The checkbox
   * allows the user to point and click for postioning networks.
   */
  
  frame_view = (Frame) xv_create( frame, FRAME,
				 FRAME_LABEL, "Simulation View",
				 FRAME_WM_COMMAND_STRINGS, -1, NULL,
				 NULL );
  panel_view1 =(Panel) xv_create( frame_view, PANEL,
				 PANEL_LAYOUT, PANEL_HORIZONTAL,
				 XV_WIDTH, 1000,
				 XV_HEIGHT, 1000,
				 XV_X, 0,
				 XV_Y, 0,
				 OPENWIN_SHOW_BORDERS, TRUE,
				 NULL );

  view_electrode = (Panel_item)
    xv_create( panel_view1,		PANEL_CHECK_BOX,
	      PANEL_CHOOSE_ONE,		TRUE,
	      XV_X,			10,
	      XV_Y,			10,
	      PANEL_LAYOUT,		PANEL_VERTICAL,
	      PANEL_LABEL_STRING,	"View Electrode",
	      PANEL_CHOICE_STRINGS,	"OFF", "ON", NULL,
	      PANEL_NOTIFY_PROC,	checkbox_view_elect,
	      PANEL_VALUE,		OFF,
	      NULL );

  temp_text = (Panel_item)
    xv_create( panel_view1,
	      PANEL_NETWORK_TEXT( view_network ),
	      XV_X, get_rect_right( view_electrode ) + 30,
	      NULL );
  list_networks_view = (Panel_item)
    xv_create( panel_view1,
	      PANEL_NETWORK_LIST( temp_text, view_network ),
	      XV_Y, get_rect_bottom( temp_text ) + 10,
	      XV_X, get_rect_left( temp_text ),
	      NULL );
  xv_set( temp_text,
	 XV_KEY_DATA, KEY_MY_LIST, list_networks_view,
	 NULL );

  window_fit( panel_view1 );

  panel_view2 = (Panel) xv_create( frame_view, PANEL,
				  PANEL_LAYOUT, PANEL_HORIZONTAL,
				  XV_WIDTH, 1000,
				  XV_HEIGHT, 1000,
				  XV_X, 0,
				  XV_Y, get_rect_bottom( panel_view1 ),
				  OPENWIN_SHOW_BORDERS, TRUE,
				  NULL );
  temp_item8 = (Panel_item) xv_create( panel_view2, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Net Dim",
				      PANEL_NOTIFY_PROC, change_dimensions_net,
				      XV_X, 10,
				      XV_Y, 20,
				      NULL );
  temp_item9 = (Panel_item) xv_create( panel_view2, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Text Dim (Width)",
				      PANEL_NOTIFY_PROC,
				      	change_dimensions_text_w,
				      NULL );
  temp_item0 = (Panel_item) xv_create( panel_view2, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Text Dim (Height)",
				      PANEL_NOTIFY_PROC,
				      	change_dimensions_text_h,
				      NULL );
  sprintf( temp_str, "%9.3f", change_dimension_amount );
  temp_text = (Panel_item)
    xv_create( panel_view2,			PANEL_TEXT,
	      PANEL_LABEL_STRING,		"Scaling factor:",
	      PANEL_NEWLINE,			10,
	      PANEL_NOTIFY_PROC,		proc_enter_float,
	      PANEL_CLIENT_DATA,		&change_dimension_amount,
	      PANEL_VALUE,			temp_str,
	      PANEL_VALUE_DISPLAY_LENGTH,	9,
	      NULL );
  window_fit( panel_view2 );

  panel_view3 = (Panel) xv_create( frame_view, SUBPANEL,
				  PANEL_LAYOUT, PANEL_HORIZONTAL,
				  PANEL_ITEM_X_GAP, 15,
				  PANEL_ITEM_Y_GAP, 15,
				  XV_Y, get_rect_bottom( panel_view2 ),
				  NULL );
  temp_item1 = (Panel_item) xv_create( panel_view3, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, pan_up_image,
				      PANEL_NOTIFY_PROC, move_net_y,
				      PANEL_CLIENT_DATA, POSITIVE,
				      XV_Y, 25,
				      NULL );
  temp_item2 = (Panel_item) xv_create( panel_view3, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, pan_left_image,
				      PANEL_NOTIFY_PROC, move_net_x,
				      PANEL_CLIENT_DATA, NEGATIVE,
				      PANEL_NEXT_ROW, -1,
				      NULL );
  temp_item3 = (Panel_item) xv_create( panel_view3, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, zoom_out_image,
				      PANEL_CLIENT_DATA, NEGATIVE,
				      PANEL_NOTIFY_PROC, move_net_z,
				      NULL );
  temp_item4 = (Panel_item) xv_create( panel_view3, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, zoom_in_image,
				      PANEL_CLIENT_DATA, POSITIVE,
				      PANEL_NOTIFY_PROC, move_net_z,
				      NULL );
  temp_item5 = (Panel_item) xv_create( panel_view3, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, pan_right_image,
				      PANEL_CLIENT_DATA, POSITIVE,
				      PANEL_NOTIFY_PROC, move_net_x,
				      NULL );
  temp_item6 = (Panel_item) xv_create( panel_view3, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, pan_down_image,
				      PANEL_CLIENT_DATA, NEGATIVE,
				      PANEL_NEXT_ROW, -1,
				      PANEL_NOTIFY_PROC, move_net_y,
				      NULL );
  sprintf( temp_str, "%9.3f", translation_amount );
  amount = xv_create( panel_view3,			PANEL_TEXT,
		     PANEL_LABEL_STRING,		"Movement amount:",
		     PANEL_NEWLINE,			20,
		     PANEL_NOTIFY_PROC,			proc_enter_float,
		     PANEL_CLIENT_DATA,			&translation_amount,
		     PANEL_VALUE,			temp_str,
		     PANEL_VALUE_DISPLAY_LENGTH,	9,
		     NULL );
  temp_item7 = (Panel_item) xv_create( panel_view3, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Cancel",
				      PANEL_NEXT_ROW, 10,
				      PANEL_NOTIFY_PROC, proc_btn_cancel,
				      PANEL_CLIENT_DATA, frame_view,
				      NULL );
  window_fit( panel_view3 );

  width = Max( get_rect_width( panel_view1 ), get_rect_width( panel_view2 ) );
  width = Max( get_rect_width( panel_view3 ), width );
  xv_set( panel_view1,
	 XV_WIDTH, width,
	 NULL );
  xv_set( panel_view2,
	 XV_WIDTH, width,
	 NULL );
  xv_set( panel_view3,
	 XV_WIDTH, width,
	 NULL );

  xv_set( temp_item1,
	 XV_X, get_center( temp_item1 ),
	 NULL );
  temp1 = (int) (width - (get_rect_width( temp_item2 )
			  + get_rect_width( temp_item3 )
			  + get_rect_width( temp_item4 )
			  + get_rect_width( temp_item5 ))) / 5;
  xv_set( temp_item2,
	 XV_X, temp1,
	 NULL );
  xv_set( temp_item3,
	 XV_X, get_rect_right( temp_item2 ) + temp1,
	 NULL );
  xv_set( temp_item4,
	 XV_X, get_rect_right( temp_item3 ) + temp1,
	 NULL );
  xv_set( temp_item5,
	 XV_X, get_rect_right( temp_item4 ) + temp1,
	 NULL );
  xv_set( temp_item6,
	 XV_X, get_center( temp_item6 ),
	 NULL );
  xv_set( temp_item7,
	 XV_X, get_center( temp_item7 ),
	 NULL );
  temp1 = (int) (width - (get_rect_width( temp_item8 )
			  + get_rect_width( temp_item9 )
			  + get_rect_width( temp_item0 ))) / 4;
  xv_set( temp_item8,
	 XV_X, temp1,
	 NULL );
  xv_set( temp_item9,
	 XV_X, get_rect_right( temp_item8 ) + temp1,
	 NULL );
  xv_set( temp_item0,
	 XV_X, get_rect_right( temp_item9 ) + temp1,
	 NULL );
  window_fit( frame_view );
  xv_set( temp_text,
	 XV_X, get_center( temp_text ),
	 NULL );
  (void) xv_create( panel_view2, PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE, carriage_return_image,
		   XV_Y, get_rect_top( temp_text ),
		   XV_X, get_rect_right( temp_text ) + 15,
		   NULL );
  xv_set( amount,
	 XV_X, get_center( amount ),
	 NULL );






  /*
   * frame for loading cells with random activity.  We can specify 
   * the max and min values for the uniform distribution.
   */
  
  frame_random = (Frame) xv_create( frame, FRAME,
				   FRAME_LABEL, "Randomize Cell Activity",
				   FRAME_WM_COMMAND_STRINGS, -1, NULL,
				   XV_WIDTH, 1000,
				   XV_HEIGHT, 1000,
				   NULL );
  panel_random = (Panel) xv_create( frame_random, PANEL,
				   PANEL_LAYOUT, PANEL_HORIZONTAL,
				   OPENWIN_SHOW_BORDERS, TRUE,
				   NULL );
  sprintf( temp_str, "%9.3f", DEFAULT_ACTIVITY_MIN );
  random_min = (Panel_item) xv_create( panel_random, PANEL_TEXT,
				      PANEL_LABEL_STRING, "Min firing rate: ",
				      XV_X, 10,
				      XV_Y, 10,
				      PANEL_VALUE, temp_str, /* DW 940717 */
				      PANEL_VALUE_DISPLAY_LENGTH, 9,
				      NULL ); 
  sprintf( temp_str, "%9.3f", DEFAULT_ACTIVITY_MAX );
  random_max = (Panel_item) xv_create( panel_random, PANEL_TEXT,
				      PANEL_LABEL_STRING, "Max firing rate: ",
				      XV_X, 10,
				      XV_Y, get_rect_bottom( random_min ) + 10,
				      PANEL_VALUE, temp_str, /* DW 940717 */
				      PANEL_VALUE_DISPLAY_LENGTH, 9,
				      NULL ); 
  temp_item1 = (Panel_item) xv_create( panel_random, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Load cells",
				      PANEL_NEXT_ROW, 20,
				      PANEL_NOTIFY_PROC,  randomize_networks,
				      NULL );
  temp_item2 = (Panel_item) xv_create( panel_random, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Cancel",
				      PANEL_NOTIFY_PROC, proc_btn_cancel,
				      PANEL_CLIENT_DATA, frame_random,
				      NULL );
  window_fit( panel_random );
  window_fit( frame_random );
  width = get_rect_width( panel_random );
  temp1 = (int) ((width - (get_rect_width( temp_item1 )
			   + get_rect_width( temp_item2 ))) / 3);
  xv_set( temp_item1,
	 XV_X, temp1,
	 NULL );
  xv_set( temp_item2,
	 XV_X, get_rect_right( temp_item1 ) + temp1,
	 NULL );







  /* frame for editing the weights of existing connections.
   * The user can specify the network to change, the cells and which set 
   * of connections, as well as the ne conducctance function.  This saves
   * a great deal of time especially when dealing with very large networks
   * where the precise values of the connections are uncertain
   */
  
  frame_edit_connect = (Frame) xv_create( frame, SUBFRAME,
					 FRAME_LABEL, "Edit Connections",
					 NULL );
  panel_edit_connect = (Panel) xv_create( frame_edit_connect, PANEL,
					 PANEL_LAYOUT, PANEL_VERTICAL,
					 OPENWIN_SHOW_BORDERS, TRUE,
					 NULL );
  temp_text = (Panel_item)
    xv_create( panel_edit_connect,
	      PANEL_NETWORK_TEXT( edit_connect_network ),
	      XV_X, 10, XV_Y, 10,
	      NULL );
  list_networks_edit_connect = (Panel_item)
    xv_create( panel_edit_connect,
	      PANEL_NETWORK_LIST( temp_text, edit_connect_network ),
	      XV_X, 10,
	      NULL );
  xv_set( temp_text,
	 XV_KEY_DATA, KEY_MY_LIST, list_networks_edit_connect,
	 NULL );

  edit_connect_id = (Panel_item)
    xv_create( panel_edit_connect, PANEL_MESSAGE,
	      PANEL_LABEL_STRING, "Connection: ",
	      PANEL_LABEL_BOLD, TRUE,
	      XV_X, 10,
	      XV_Y, get_rect_bottom( list_networks_edit_connect ) + 20,
	      NULL ); 
  list_ids_edit_connect = (Panel_item)
    xv_create( panel_edit_connect, PANEL_LIST,
	      PANEL_LIST_STRINGS, "", NULL,
	      XV_X, 10,
	      PANEL_LIST_WIDTH, 300,
	      PANEL_LIST_DISPLAY_ROWS, 4,
	      PANEL_NOTIFY_PROC, proc_select_id,
	      NULL );

  temp_item1 = (Panel_item)
    xv_create( panel_edit_connect, PANEL_TEXT,
	      PANEL_NOTIFY_PROC, proc_enter_name,
	      PANEL_CLIENT_DATA, edit_connect_function,
	      PANEL_VALUE_DISPLAY_LENGTH, NAME_DISPLAY_SIZE,
	      PANEL_VALUE_STORED_LENGTH, FUNCTION_SIZE,
	      PANEL_LABEL_STRING, "Connection Function:",
	      XV_X, 10,
	      XV_Y, get_rect_bottom( list_ids_edit_connect ) + 20,
	      XV_KEY_DATA, KEY_UPDATE_LIST, FALSE,
	      NULL );

  temp_item1 = (Panel_item)
    xv_create( panel_edit_connect, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Change Connection",
	      XV_Y, get_rect_bottom( temp_item1 ) + 15,
	      PANEL_NOTIFY_PROC, btn_edit_connection,
	      NULL );
  temp_item2 = (Panel_item)
    xv_create( panel_edit_connect, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Cancel",
	      XV_Y, get_rect_top( temp_item1 ),
	      PANEL_NOTIFY_PROC, proc_btn_cancel,
	      PANEL_CLIENT_DATA, frame_edit_connect,
	      NULL );
  window_fit( panel_edit_connect );
  window_fit( frame_edit_connect );
  width = get_rect_width( panel_edit_connect );
  temp1 = (int) ((width - (get_rect_width( temp_item1 )
			   + get_rect_width( temp_item2 ))) / 3);
  xv_set( temp_item1,
	 XV_X, temp1,
	 NULL );
  xv_set( temp_item2,
	 XV_X, get_rect_right( temp_item1 ) + temp1,
	 NULL );




  

  
  /* 
   * frame to load activity into a given network
   * the network will usually be the input laye (Retina) 
   * but this is not required.  The file specified contains
   * the names of the input files generated by  appropriate 
   * bytemap software.
   */
  
  frame_load_activity = (Frame) xv_create( frame, FRAME,
					  FRAME_LABEL, "Load Activity",
					  FRAME_WM_COMMAND_STRINGS, -1, NULL,
					  XV_WIDTH, 1000,
					  XV_HEIGHT, 1000,
					  NULL );
  temp_panel = (Panel) xv_create( frame_load_activity, PANEL,
				 PANEL_LAYOUT, PANEL_HORIZONTAL,
				 OPENWIN_SHOW_BORDERS, TRUE,
				 NULL );

  chbox_activity_load = (Panel_item)
    xv_create( temp_panel,		PANEL_CHECK_BOX,
	      PANEL_CHOOSE_ONE,		TRUE,
	      PANEL_LAYOUT,		PANEL_VERTICAL,
	      XV_X,			10,
	      XV_Y,			10,
	      PANEL_LABEL_STRING,	"Cycle load",
	      PANEL_CHOICE_STRINGS,	"OFF", "ON", NULL,
	      PANEL_NOTIFY_PROC,	proc_chbox_binary,
	      PANEL_CLIENT_DATA,	ACTIVITY_LOAD,
	      PANEL_VALUE,		query_flag( ACTIVITY_LOAD ),
	      NULL );

  (void) xv_create( temp_panel,			PANEL_CHECK_BOX,
		   PANEL_CHOOSE_ONE,		TRUE,
		   PANEL_LAYOUT,		PANEL_VERTICAL,
		   XV_X,	    get_rect_right( chbox_activity_load ) + 30,
		   PANEL_LABEL_STRING,		"Randomize Order",
		   PANEL_CHOICE_STRINGS,	"OFF", "ON", NULL,
		   PANEL_NOTIFY_PROC,		proc_chbox_binary,
		   PANEL_CLIENT_DATA,		LOAD_RANDOM,
		   PANEL_VALUE,			query_flag( LOAD_RANDOM ),
		   NULL );

  temp_text = (Panel_item)
    xv_create( temp_panel,
	      PANEL_NETWORK_TEXT( network_load_activity ),
	      PANEL_NEWLINE, 20,
	      NULL );
  list_networks_load_activity = (Panel_item)
    xv_create( temp_panel,
	      PANEL_NETWORK_LIST( temp_text, network_load_activity ),
	      PANEL_NEWLINE, 10,
	      NULL );
  xv_set( temp_text,
	 XV_KEY_DATA, KEY_MY_LIST, list_networks_load_activity,
	 NULL );


  (void) xv_create( temp_panel,
		   PANEL_NAME_TEXT( filename_load_activity ),
		   PANEL_LABEL_STRING, "Batch Filename",
		   PANEL_VALUE, filename_load_activity,
		   PANEL_NEWLINE, 20,
		   NULL );

  temp_item1 = (Panel_item) xv_create( temp_panel, PANEL_BUTTON,
				      PANEL_NEWLINE, 10,
				      PANEL_LABEL_STRING, "Load Batch File",
				      PANEL_NOTIFY_PROC,  btn_list_afiles,
				      NULL );

  temp_text = (Panel_item)
    xv_create( temp_panel,
	      PANEL_FILENAME_TEXT( activity_filename ),
	      PANEL_LABEL_STRING, "Activity File:",
	      PANEL_NEWLINE, 20,
	      NULL );
  activity_file_list = (Panel_item)
    xv_create( temp_panel,
	      PANEL_FILENAME_LIST( temp_text, activity_filename ),
	      PANEL_NEWLINE, 10,
	      NULL );
  xv_set( temp_text,
	 XV_KEY_DATA, KEY_MY_LIST, activity_file_list,
	 NULL );

  temp_item2 = (Panel_item)
    xv_create( temp_panel, PANEL_BUTTON,
	      PANEL_NEWLINE, 10,
	      PANEL_LABEL_STRING, "Load Single File",
	      PANEL_NOTIFY_PROC,  load_1_activity_now,
	      NULL );

  temp_item3 = (Panel_item) xv_create( temp_panel, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Cancel",
				      PANEL_NOTIFY_PROC,  proc_btn_cancel,
				      PANEL_CLIENT_DATA, frame_load_activity,
				      NULL );
  window_fit( temp_panel );
  window_fit( frame_load_activity );
  width = get_rect_width( temp_panel );
  temp1 = (int) ((width - get_rect_width( temp_item1 )) / 2);
  xv_set( temp_item1,
	 XV_X, temp1,
	 NULL );
  temp1 = (int) ((width - (get_rect_width( temp_item2 )
			   + get_rect_width( temp_item3 ))) / 3);
  xv_set( temp_item2,
	 XV_X, temp1,
	 NULL );
  xv_set( temp_item3,
	 XV_X, get_rect_right( temp_item2 ) + temp1,
	 NULL );








  /* 
   *  Frame: Set Parameters
   *  Used by: Set Parameters
   *  Parent: Modification Control (panel3)
   *
   * frame to clamp or unclamp a given set of cells
   * at a particular activity.  (Clamping means that these cells will 
   * not be updated and will maintain their current state 
   */
  
  frame_clamp = (Frame) xv_create( frame, FRAME,
				  FRAME_LABEL, "Set Parameters",
				  FRAME_WM_COMMAND_STRINGS, -1, NULL,
				  XV_WIDTH, 1000,
				  XV_HEIGHT, 1000,
				  NULL );
  panel_clamp = (Panel) xv_create( frame_clamp, PANEL,
				  PANEL_LAYOUT, PANEL_HORIZONTAL,
				  OPENWIN_SHOW_BORDERS, TRUE,
				  NULL );
  
  /*
   *  This is a much more appropriate name for this panel because it doesn't
   *    concentrate on clamping alone.  The old name, clamp_type, was a
   *    misnomer since it could contain such values as "SET_TRANS_FUNC."
   *  The PANEL_CHOICE_STRINGS __MUST__ be in the same order as defined
   *    in nexus.h!  (See nexus.h for more details).
   *  DW 940711
   */

  /* rev. 12/27/91 */
  parameter_type = (Panel_item)
    xv_create( panel_clamp, PANEL_CHECK_BOX,
	      PANEL_CHOOSE_ONE, TRUE,
	      PANEL_LABEL_STRING,
	      "Set Network... ",
	      PANEL_CHOICE_NCOLS, 3,

	      PANEL_CHOICE_STRINGS,
/* CLAMP_ON, 0 */			  "Fully Clamped",
/* CLAMP_OFF, 1 */			  "Unclamped",
/* SET_ACTIVITY, 2 */			  "Activity",
/* SET_TRANS_FUNC, 3 */			  "Trans. Function",
/* SET_THRESHOLD, 4 */			  "Threshold",
/* SET_SCALE, 5 */			  "Scale",
/* SET_OFFSET, 6 */			  "Offset",
/* SET_NUM_UPDATES, 7 */		  "# Updates",
/* SET_DECAY, 8 */			  "Decay",
/* CLAMP_SELECTIVE_ON....................."Selectively Clamped", */
	      NULL,

	      PANEL_VALUE, CLAMP_OFF,
	      NULL ); 

  parameter_network_text = (Panel_item)
    xv_create( panel_clamp,
	      PANEL_NETWORK_TEXT( parameter_network ),
	      PANEL_NEWLINE, 20,
	      NULL );
  list_networks_clamp = (Panel_item)
    xv_create( panel_clamp,
	      PANEL_NETWORK_LIST( parameter_network_text, parameter_network ),
	      PANEL_NEWLINE, 10,
	      NULL );
  xv_set( parameter_network_text,
	 XV_KEY_DATA, KEY_MY_LIST, list_networks_clamp,
	 NULL );

  /* rev. 12/27/91 */
  parameter_value = (Panel_item) xv_create( panel_clamp, PANEL_TEXT,
					   PANEL_LABEL_STRING, "Value",
					   PANEL_NEWLINE, 20,
					   PANEL_VALUE, "",
					   PANEL_VALUE_DISPLAY_LENGTH, 40,
					   NULL );
  temp_item1 = (Panel_item) xv_create( panel_clamp, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Change Value",
				      PANEL_NOTIFY_PROC,  change_parameters,
				      PANEL_NEWLINE, 20,
				      NULL );
  temp_item2 = (Panel_item) xv_create( panel_clamp, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Show Current Values",
				      PANEL_NOTIFY_PROC,  btn_show_parameters,
				      NULL );
  temp_item3 = (Panel_item) xv_create( panel_clamp, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Cancel",
				      PANEL_NOTIFY_PROC,  proc_btn_cancel,
				      PANEL_CLIENT_DATA, frame_clamp,
				      NULL );
  window_fit( panel_clamp );
  window_fit( frame_clamp );
  width = get_rect_width( panel_clamp );
  temp1 = (int) (width - (get_rect_width( temp_item1 )
			  + get_rect_width( temp_item2 )
			  + get_rect_width( temp_item3 ))) / 4;
  xv_set( temp_item1,
	 XV_X, temp1,
	 NULL );
  xv_set( temp_item2,
	 XV_X, get_rect_right( temp_item1 ) + temp1,
	 NULL );
  xv_set( temp_item3,
	 XV_X, get_rect_right( temp_item2 ) + temp1,
	 NULL );








  /*
   *  Frame: Save Simulation
   *  Used by: Save Simulation
   *  Parent: Utilities Control (panel4)
   *
   *  Frame used to save simulation to a file.
   *  Once a simulation is saved it can be loaded without being
   *  built.
   *
   *  Gave default filename extension ".save".  DW 940718
   */
  
  frame_save = (Frame) xv_create( frame, FRAME,
				 FRAME_LABEL, "Save Simulation",
				 FRAME_WM_COMMAND_STRINGS, -1, NULL,
				 XV_WIDTH, 1000,
				 XV_HEIGHT, 1000,
				 NULL );
  panel_save = (Panel) xv_create( frame_save, PANEL,
				 PANEL_LAYOUT, PANEL_HORIZONTAL,
				 OPENWIN_SHOW_BORDERS, TRUE,
				 NULL );
  filename_save = (Panel_item) xv_create( panel_save, PANEL_TEXT,
					 PANEL_LABEL_STRING, "Save to file",
					 XV_X, 10,
					 XV_Y, 10,
					 PANEL_VALUE, ".save",
					 PANEL_VALUE_DISPLAY_LENGTH, 20,
					 NULL );
  chbox_file_compressed = (Panel_item)
    xv_create( panel_save,		PANEL_CHECK_BOX,
	      PANEL_NEXT_ROW,		20,
	      PANEL_CHOOSE_ONE,		TRUE,
	      PANEL_LABEL_STRING,	"Save format:",
	      PANEL_LAYOUT,		PANEL_VERTICAL,
	      PANEL_CHOICE_STRINGS,	"ASCII", "Compressed", NULL,
	      PANEL_NOTIFY_PROC,	proc_chbox_binary,
	      PANEL_CLIENT_DATA,	FILE_COMPRESSED,
	      PANEL_VALUE,		query_flag( FILE_COMPRESSED ),
	      NULL );

  temp_item1 = (Panel_item) xv_create( panel_save, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Save Now",
				      PANEL_NEXT_ROW, 20,
				      PANEL_NOTIFY_PROC, btn_save_now,
				      NULL );

  temp_item2 = (Panel_item) xv_create( panel_save, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Cancel",
				      PANEL_NOTIFY_PROC, proc_btn_cancel,
				      PANEL_CLIENT_DATA, frame_save,
				      NULL );

  window_fit( panel_save );
  window_fit( frame_save );
  width = get_rect_width( panel_save );
  temp1 = (int) ((width - (get_rect_width( temp_item1 )
			   + get_rect_width( temp_item2 ))) / 3);
  xv_set( temp_item1,
	 XV_X, temp1,
	 NULL );
  xv_set( temp_item2,
	 XV_X, get_rect_right( temp_item1 ) + temp1,
	 NULL );
  temp1 = (int) ((width - (get_rect_width( chbox_file_compressed ))) / 2);
  xv_set( chbox_file_compressed,
	 XV_X, temp1,
	 NULL );




#if 0

  /*
   *  Get filename to send postscript output of display
   */
  
  frame_graphics_filename = (Frame) xv_create( frame, FRAME,
					      FRAME_LABEL,
					      	"Output postscript file",
					      FRAME_WM_COMMAND_STRINGS, -1, NULL,
					      WIN_INHERIT_COLORS, TRUE,
					      XV_WIDTH, 1000,
					      XV_HEIGHT, 1000,
					      NULL );
  panel_graphics_filename = (Panel) xv_create( frame_graphics_filename, PANEL,
					      PANEL_LAYOUT, PANEL_HORIZONTAL,
					      OPENWIN_SHOW_BORDERS, TRUE,
					      NULL );
  graphics_filename = (Panel_item) xv_create( panel_graphics_filename,
					     PANEL_TEXT,
					     PANEL_LABEL_STRING,
					     	"Postscript File",
					     XV_X, 10,
					     XV_Y, 10,
					     PANEL_VALUE, "",
					     PANEL_VALUE_DISPLAY_LENGTH, 23,
					     NULL );
  temp_item1 = (Panel_item) xv_create( panel_graphics_filename, PANEL_BUTTON,
				      PANEL_NEWLINE, 20,
				      PANEL_LABEL_STRING, "OK",
				      PANEL_NOTIFY_PROC, go_graphics_filename,
				      NULL ); 
  temp_item2 = (Panel_item) xv_create( panel_graphics_filename, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Cancel",
				      PANEL_NOTIFY_PROC, proc_btn_cancel,
				      PANEL_CLIENT_DATA,
				      	frame_graphics_filename,
				      NULL );
  window_fit( panel_graphics_filename );
  window_fit( frame_graphics_filename );
  width = get_rect_width( panel_graphics_filename );
  temp1 = (int) ((width - (get_rect_width( temp_item1 )
			   + get_rect_width( temp_item2 ))) / 3);
  xv_set( temp_item1,
	 XV_X, temp1,
	 NULL );
  xv_set( temp_item2,
	 XV_X, get_rect_right( temp_item1 ) + temp1,
	 NULL );
  







  /* window for inputing plot output filename */
  
  frame_plot_filename =
    (Frame) xv_create( frame_plot, FRAME,
		      FRAME_LABEL, "Output filename",
		      FRAME_WM_COMMAND_STRINGS, -1, NULL,
		      WIN_INHERIT_COLORS, TRUE,
		      XV_WIDTH, 1000,
		      XV_HEIGHT, 1000,
		      NULL );
  panel_plot_filename =
    (Panel) xv_create( frame_plot_filename, PANEL,
		      PANEL_LAYOUT, PANEL_HORIZONTAL,
		      OPENWIN_SHOW_BORDERS, TRUE,
		      NULL );

  (void) xv_create( panel_plot_filename,	PANEL_CHECK_BOX,
		   PANEL_CHOOSE_ONE,		TRUE,
		   PANEL_LABEL_STRING,		"Cycle Record ",
		   PANEL_CHOICE_STRINGS,	"OFF", "ON", NULL,
		   PANEL_NOTIFY_PROC,		proc_chbox_binary,
		   PANEL_CLIENT_DATA,		PLOT_CYCLE,
		   PANEL_VALUE,			query_flag( PLOT_CYCLE ),
		   NULL );

  (void) xv_create( panel_plot_filename,	PANEL_TEXT,
		   PANEL_NEWLINE,		10,
		   PANEL_LABEL_STRING,		"Output File",
		   PANEL_NOTIFY_PROC,		proc_enter_name,
		   PANEL_CLIENT_DATA,		plot_filename,
		   PANEL_VALUE,			plot_filename,
		   PANEL_VALUE_DISPLAY_LENGTH,	26,
		   NULL );

  print_data_precision =
    (Panel_item) xv_create( panel_plot_filename,	PANEL_CHECK_BOX,
			   PANEL_NEWLINE,		10,
			   PANEL_CHOOSE_ONE,		TRUE,
			   PANEL_LABEL_STRING,		"precision: ",
			   PANEL_CHOICE_STRINGS, "x.x",	"x.xxx ", "x.xxxxxx ",
							NULL,
			   PANEL_VALUE,			ONE_DECIMAL,
			   NULL );

  temp_item1 =
    (Panel_item) xv_create( panel_plot_filename,	PANEL_BUTTON,
			   PANEL_NEWLINE,		20,
			   PANEL_LABEL_STRING,		"Write Now",
			   PANEL_NOTIFY_PROC,		go_plot_filename,
			   NULL ); 

  temp_item2 =
    (Panel_item) xv_create( panel_plot_filename,	PANEL_BUTTON,
			   PANEL_LABEL_STRING,		"Cancel",
			   PANEL_NOTIFY_PROC,		proc_btn_cancel,
			   PANEL_CLIENT_DATA,		frame_plot_filename,
			   NULL );

  window_fit( panel_plot_filename );
  window_fit( frame_plot_filename );
  width = get_rect_width( panel_plot_filename );
  temp1 = (int) ((width - (get_rect_width( temp_item1)
			   + get_rect_width( temp_item2))) / 3);
  xv_set( temp_item1,
	 XV_X, temp1,
	 NULL );
  xv_set( temp_item2,
	 XV_X, get_rect_right( temp_item1 ) + temp1,
	 NULL );
  







  /* Frame for plotting data directly within NEXUS.   */

  frame_plot_menu = (Frame) xv_create( frame, FRAME,
				      FRAME_LABEL, "Plot Data",
				      FRAME_WM_COMMAND_STRINGS, -1, NULL,
				      XV_WIDTH, 1000,
				      XV_HEIGHT, 1000,
				      NULL );
  panel_plot = (Panel) xv_create( frame_plot_menu, PANEL,
				 PANEL_LAYOUT, PANEL_HORIZONTAL,
				 OPENWIN_SHOW_BORDERS, TRUE,
				 NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Do Plot",
				      PANEL_NOTIFY_PROC, do_plot_now,
				      XV_X, 10,
				      XV_Y, 10,
				      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Reset View",
				      PANEL_NOTIFY_PROC, reset_view,
				      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Cancel",
				      PANEL_NOTIFY_PROC, quit_plot,
				      PANEL_CLIENT_DATA, frame_plot_menu,
				      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Send to File...",
				      PANEL_NEWLINE, -1,
				      PANEL_NOTIFY_PROC, proc_btn_subframe,
				      PANEL_CLIENT_DATA, frame_plot_filename,
				      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Print Screen...",
				      PANEL_NOTIFY_PROC, print_screen_menu,
				      NULL );
  plot_type = (Panel_item) xv_create( panel_plot, PANEL_CHECK_BOX,
				     PANEL_CHOOSE_ONE, TRUE,
				     PANEL_NEXT_ROW, 20,
				     PANEL_LABEL_STRING, "type: ",
				     PANEL_CHOICE_STRINGS,
				     	"scatter", "bar ", NULL,
				     PANEL_VALUE, SCATTER,
				     NULL );
  temp_text = (Panel_item)
    xv_create( panel_plot,
	      PANEL_NETWORK_TEXT( plot_network_name ),
	      PANEL_NEWLINE, 20,
	      NULL );
  list_networks_plot = (Panel_item)
    xv_create( panel_plot,
	      PANEL_NETWORK_LIST( temp_text, plot_network_name ),
	      PANEL_NEWLINE, 10,
	      NULL );
  xv_set( temp_text,
	 XV_KEY_DATA, KEY_MY_LIST, list_networks_plot,
	 NULL );

  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, rotate_x_clk_image,
				      PANEL_NOTIFY_PROC, rotate_x,
				      PANEL_CLIENT_DATA, POSITIVE,
				      PANEL_NEWLINE, 20,
				      XV_X, 80,
				      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, rotate_y_clk_image,
				      PANEL_NOTIFY_PROC, rotate_y,
				      PANEL_CLIENT_DATA, POSITIVE,
				      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, rotate_z_clk_image,
				      PANEL_NOTIFY_PROC, rotate_z,
				      PANEL_CLIENT_DATA, POSITIVE,
				      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, rotate_x_cclk_image,
				      PANEL_NOTIFY_PROC, rotate_x,
				      PANEL_CLIENT_DATA, NEGATIVE,
				      PANEL_NEWLINE, 10,
				      XV_X, 80,
				      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, rotate_y_cclk_image,
				      PANEL_NOTIFY_PROC, rotate_y,
				      PANEL_CLIENT_DATA, NEGATIVE,
				      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, rotate_z_cclk_image,
				      PANEL_NOTIFY_PROC, rotate_z,
				      PANEL_CLIENT_DATA, NEGATIVE,
				      NULL );
  rotate_plot_amount = (Panel_item) xv_create( panel_plot, PANEL_SLIDER,
					      PANEL_LABEL_STRING, "Degrees: ",
					      PANEL_NEWLINE, 20,
					      PANEL_VALUE, 67,
					      PANEL_MIN_VALUE, 0,
					      PANEL_MAX_VALUE, 360 ,
					      PANEL_SLIDER_WIDTH, 100,
					      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, pan_up_image,
				      PANEL_NEXT_ROW, 20,
				      PANEL_NOTIFY_PROC, move_plot_y,
				      PANEL_CLIENT_DATA, POSITIVE,
				      XV_X, 117,
				      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, pan_left_image,
				      PANEL_NEXT_ROW, 10,
				      PANEL_NOTIFY_PROC, move_plot_x,
				      PANEL_CLIENT_DATA, NEGATIVE,
				      XV_X, 17,
				      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, zoom_out_image,
				      PANEL_CLIENT_DATA, POSITIVE,
				      PANEL_NOTIFY_PROC, move_plot_z,
				      XV_X, 82,
				      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, zoom_in_image,
				      PANEL_CLIENT_DATA, NEGATIVE,
				      PANEL_NOTIFY_PROC, move_plot_z,
				      XV_X, 147,
				      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, pan_right_image,
				      PANEL_CLIENT_DATA, POSITIVE,
				      PANEL_NOTIFY_PROC, move_plot_x,
				      XV_X, 212,
				      NULL );
  temp_item1 = (Panel_item) xv_create( panel_plot, PANEL_BUTTON,
				      PANEL_LABEL_IMAGE, pan_down_image,
				      PANEL_CLIENT_DATA, NEGATIVE,
				      PANEL_NEXT_ROW, 10,
				      XV_X, 117,
				      PANEL_NOTIFY_PROC, move_plot_y,
				      NULL );
  move_plot_amount = (Panel_item) xv_create( panel_plot, PANEL_TEXT,
					    PANEL_LABEL_STRING,
					    	"movement amount",
					    PANEL_VALUE, "5",
					    PANEL_VALUE_DISPLAY_LENGTH, 18,
					    PANEL_NEWLINE, 10,
					    NULL );
  window_fit( panel_plot );
  window_fit( frame_plot_menu );








  /* window to create postscript file of 3D plot screen */

  frame_print_screen = (Frame) xv_create( frame_plot, FRAME,
					 FRAME_LABEL, "Output filename",
					 FRAME_WM_COMMAND_STRINGS, -1, NULL,
					 WIN_INHERIT_COLORS, TRUE,
					 XV_WIDTH, 1000,
					 XV_HEIGHT, 1000,
					 NULL );
  panel_print_screen = (Panel) xv_create( frame_print_screen, PANEL,
					 PANEL_LAYOUT, PANEL_HORIZONTAL,
					 OPENWIN_SHOW_BORDERS, TRUE,
					 NULL );

  print_screen_filename = (Panel_item) xv_create( panel_print_screen,
						 PANEL_TEXT,
						 PANEL_LABEL_STRING,
						 	"Output File",
						 PANEL_VALUE, "",
						 PANEL_VALUE_DISPLAY_LENGTH,
						 	25,
						 NULL );
  temp_item1 =
    (Panel_item) xv_create( panel_print_screen,	PANEL_BUTTON,
			   PANEL_NEWLINE,	20,
			   PANEL_LABEL_STRING,	"OK",
			   PANEL_NOTIFY_PROC,	print_screen,
			   NULL );

  temp_item2 =
    (Panel_item) xv_create( panel_print_screen,	PANEL_BUTTON,
			   PANEL_LABEL_STRING,	"Cancel",
			   PANEL_NOTIFY_PROC,	proc_btn_cancel,
			   PANEL_CLIENT_DATA,	frame_print_screen,
			   NULL );

  window_fit( panel_print_screen );
  window_fit( frame_print_screen );
  width = get_rect_width( panel_print_screen );
  temp1 = (int) ((width - (get_rect_width( temp_item1)
			   + get_rect_width( temp_item2))) / 3);
  xv_set( temp_item1,
	 XV_X, temp1,
	 NULL );
  xv_set( temp_item2,
	 XV_X, get_rect_right( temp_item1 ) + temp1,
	 NULL );

#endif 0    /*  Eliminated PLOT section  */  






  /***************************************************************************
   *  Now, finally, we produce the buttons in the main frame, in four distinct
   *    panels.  We needed to delay the creation of these so that we already
   *    have the handles to the various sub-frames.
   *
   *      Panel 1, Simulation Control.
   *      Panel 2, Display Control.
   *      Panel 3, Modification Control.
   *      Panel 4, Utilities Control.
   ***************************************************************************/

  /*
   * Panel 1; Simulation Control:
   *      Load Simulation
   *      Build Simulation
   *      Simulate
   *      Quit
   */

  panel1 = (Panel) xv_create( frame, SUBPANEL,
			     PANEL_LAYOUT, PANEL_VERTICAL,
			     XV_Y, 0,
			     NULL );
  (void) xv_create( panel1, PANEL_TITLE,
		   PANEL_TITLE_STRING, "Simulation Control",
		   NULL );
  filename_loaded = (Panel_item) xv_create( panel1, PANEL_MESSAGE,
					   PANEL_LABEL_STRING, "no file",
					   PANEL_VALUE_DISPLAY_LENGTH, 22,
					   NULL );
  (void) xv_create( panel1, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Load Simulation...",
		   PANEL_NOTIFY_PROC, proc_btn_subframe,
		   PANEL_CLIENT_DATA, frame_filename,
		   XV_KEY_DATA, KEY_LOAD_BUILD, LOAD,
		   NULL );
  (void) xv_create( panel1, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Build Simulation...",
		   PANEL_NOTIFY_PROC, proc_btn_subframe,
		   PANEL_CLIENT_DATA, frame_filename,
		   XV_KEY_DATA, KEY_LOAD_BUILD, BUILD,
		   NULL );
  temp_item2 = (Panel_item) xv_create( panel1, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Simulate...",
				      PANEL_NOTIFY_PROC, proc_btn_subframe,
				      PANEL_CLIENT_DATA, frame_simulate,
				      NULL );
  (void) xv_create( panel1, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Quit",
		   PANEL_NOTIFY_PROC, quit,
		   PANEL_CLIENT_DATA, frame,
		   NULL );
  (void) xv_create( panel1, PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE, nexus_image,
		   XV_X, get_rect_right( temp_item2 ) + 20,
		   XV_Y, get_rect_top( temp_item2 ),
		   NULL );
  window_fit( panel1 );

  /*
   * Panel 2; Electrode Control
   *      Connectivity
   *      Activity
   *      View
   *      Refresh Window
   */

  panel2 = (Panel) xv_create( frame, SUBPANEL,
			     PANEL_LAYOUT, PANEL_VERTICAL,
			     XV_Y, get_rect_bottom( panel1 ),
			     NULL );
  (void) xv_create( panel2, PANEL_TITLE,
		   PANEL_TITLE_STRING, "Electrode Control",
		   NULL );
  (void) xv_create( panel2, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Connectivity...",
		   PANEL_NOTIFY_PROC, proc_btn_subframe,
		   PANEL_CLIENT_DATA, frame_connect,
		   NULL );
  temp_item1 = (Panel_item) xv_create( panel2, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Activity...",
				      PANEL_NOTIFY_PROC, proc_btn_subframe,
				      PANEL_CLIENT_DATA, frame_activity,
				      NULL );
  temp_item2 = (Panel_item) xv_create( panel2, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "View...",
				      PANEL_NOTIFY_PROC, proc_btn_subframe,
				      PANEL_CLIENT_DATA, frame_view,
				      NULL );
  (void) xv_create( panel2, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Refresh Window",
		   XV_Y, get_rect_bottom( temp_item2 ) + 15,
		   PANEL_NOTIFY_PROC, refresh_window,
		   NULL );
  (void) xv_create( panel2, PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE, nexus_image4,
		   XV_X, get_rect_right( temp_item1 ) + 20,
		   XV_Y, get_rect_top( temp_item1 ),
		   NULL ); 
  window_fit( panel2 );


  /*
   * Panel 3; Modification Control:
   *      Randomize Activity
   *      Edit Connections
   *      Load Map Activity
   *      Set Parameters
   *      Learning Menu
   *      -       Backpropagation
   *      -       Hebb Rule
   *      -       RBF Learning
   *      -       Network Inversion
   */

  panel3 = (Panel) xv_create( frame, SUBPANEL,
			     PANEL_LAYOUT, PANEL_VERTICAL,
			     XV_Y, get_rect_bottom( panel2 ),
			     NULL );
  (void) xv_create( panel3, PANEL_TITLE,
		   PANEL_TITLE_STRING, "Modification Control",
		   NULL );
  (void) xv_create( panel3, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Randomize Activity...",
		   PANEL_NOTIFY_PROC, proc_btn_subframe,
		   PANEL_CLIENT_DATA, frame_random,
		   NULL );
  (void) xv_create( panel3, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Edit Connections...",
		   PANEL_NOTIFY_PROC, edit_connect,
		   NULL );
  (void) xv_create( panel3, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Load Map Activity...",
		   PANEL_NOTIFY_PROC, proc_btn_subframe,
		   PANEL_CLIENT_DATA, frame_load_activity,
		   NULL );
  (void) xv_create( panel3, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Set Parameters...",
		   PANEL_NOTIFY_PROC, proc_btn_subframe,
		   PANEL_CLIENT_DATA, frame_clamp,
		   NULL );
  /* Menu code updated, DW 940717 */
  menu = (Menu) xv_create( NULL, MENU,
			  
			  MENU_ACTION_ITEM,
			  	"Backpropagation...", go_backprop,
			  MENU_ACTION_ITEM,
			  	"Hebb Rule...", go_hebb,
			  MENU_ACTION_ITEM,
			  	"RBF Learning...", go_rbf,
			  MENU_ACTION_ITEM,
			  	"Network Inversion...", go_inversion,

			  NULL );
  (void) xv_create( panel3, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Learning",
		   PANEL_ITEM_MENU, menu,
		   NULL );
  temp_item3 = (Panel_item) xv_create( panel3, PANEL_MESSAGE,
				      PANEL_LABEL_IMAGE, nexus_image2,
				      NULL );
  window_fit( panel3 );
  
  /*
   * Panel 4; Utilities Control:
   *      Save Simulation
   *      Print Display
   *      Memory
   *      Plot
   */
  
  panel4 = (Panel) xv_create( frame, SUBPANEL,
			     PANEL_LAYOUT, PANEL_VERTICAL,
			     XV_Y, get_rect_bottom( panel3 ),
			     NULL );
  (void) xv_create( panel4, PANEL_TITLE,
		   PANEL_TITLE_STRING, "Utilities Control",
		   NULL );
  (void) xv_create( panel4, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Save Simulation...",
		   PANEL_NOTIFY_PROC, proc_btn_subframe,
		   PANEL_CLIENT_DATA, frame_save,
		   NULL );
/*  (void) xv_create( panel4, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Print Display...",
		   PANEL_NOTIFY_PROC, proc_btn_subframe,
		   PANEL_CLIENT_DATA, frame_graphics_filename,
		   NULL ); */
  temp_item4 = (Panel_item) xv_create( panel4, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Memory",
				      PANEL_NOTIFY_PROC, btn_memory,
				      NULL );
/*  (void) xv_create( panel4, PANEL_BUTTON,
		   PANEL_LABEL_STRING, "Plot...",
		   PANEL_NOTIFY_PROC, plot,
		   NULL ); */
  (void) xv_create( panel4, PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE, nexus_image3,
		   XV_X, get_rect_right( temp_item4 ) + 20,
		   XV_Y, get_rect_top( temp_item4 ),
		   NULL );
  window_fit( panel4 );

  width = Max( (get_rect( panel1 ))->r_width, (get_rect( panel2 ))->r_width );
  width = Max( (get_rect( panel3 ))->r_width, width );
  width = Max( (get_rect( panel4 ))->r_width, width );

  xv_set( panel1,
	 XV_WIDTH, width,
	 NULL );
  xv_set( panel2,
	 XV_WIDTH, width,
	 NULL );
  xv_set( panel3,
	 XV_WIDTH, width,
	 NULL );
  xv_set( panel4,
	 XV_WIDTH, width,
	 NULL );
  xv_set( temp_item3,
	 XV_X, (int) ((width - get_rect_width( temp_item3 )) / 2),
	 NULL );
  window_fit( frame );

  /***************************************************************************
   *  End of main object creation
   ***************************************************************************/










  /**********
   *  Globally used variables
   **********/

  dpy = (Display *) xv_get( frame, XV_DISPLAY );

  /*
   *  Set initial position of frame on screen (unless the user's
   *  Window Manager is in interactive placement mode, in which case she
   *    places the windows herself).  This should be resources; DW.
   *  Fixed.  DW 940816, used to get stupid values for width & height
   *    once in a while.  (Not easily reproducible, mind you.)
   */

  rect.r_top = 27;		/* Arbitrary. */
  rect.r_left = XV_WIN_WIDTH;	/* Place to the right of the VOGLE canvas. */
  rect.r_width = width;		/* From frame creation above. */
  rect.r_height = (int) xv_get( frame, XV_HEIGHT );
  frame_set_rect( frame, &rect );


  /*
   *  Must call init_plot_window before using vogle calls, because
   *    init_plot_window has vinit call, which initializes vogle routines
   *    DW 940629
   *
   *  PLUS, PLOT_WINDOW has a hardcoded (!!!) ID of 0,
   *    before GRAPHICS_WINDOW (ID = 1)
   *    DW 940615
   */



#if 0
  /*
   *  Set Event handler for Plot window
   */

  window = (Window) xv_get( canvas_paint_window( plot_canvas ), XV_XID );
  xv_set( canvas_paint_window( plot_canvas ),
	 WIN_EVENT_PROC, plot_event_proc,
	 WIN_CONSUME_EVENTS,
	 WIN_MOUSE_BUTTONS,	/* First event to consume in PLOT window */
/*	 LOC_DRAG, */		/* Movement while any mouse button depressed */
	 NULL,			/* End of CONSUME_EVENTS */
	 NULL );
  init_plot_window( window );
#endif 0



  /*
   *  Set Event handler for VOGLE graphics window
   *  (actually, we catch them in the graphics canvas and interpret
   *  them first, then send drawing commands to VOGLE) -DW 940613
   */

  window = (Window) xv_get( canvas_paint_window( vogle_canvas ), XV_XID );
  xv_set( canvas_paint_window( vogle_canvas ),
	 WIN_EVENT_PROC, vogle_event_proc,
	 WIN_CONSUME_EVENTS,
	 WIN_MOUSE_BUTTONS,	/* First event to consume in GRAPHICS window */
	 LOC_DRAG,		/* Movement while any mouse button depressed */
	 NULL,			/* End of CONSUME_EVENTS */
	 NULL );
  init_graphics_window( window );


  /*
   *  Now that VOGLE has been initialized (vinit() in init_plot_window()),
   *  initialize colours etc.
   */

  init_graphics();

  printf( "Done.\nSetting up learning method menus...\n" );

  vswitch_window( GRAPHICS_WINDOW );
/*  xv_set( frame_plot,
	 XV_SHOW, FALSE,
	 NULL ); */

#ifdef NEXUS_BP
  printf( "\tBack Propagation.\n" );
  setup_menu_bp( frame );
#endif NEXUS_BP

#ifdef NEXUS_HEBB
  printf( "\tHebb Plasticity.\n" );
  setup_menu_hebb( frame );
#endif NEXUS_HEBB

#ifdef NEXUS_INVERSION
  printf( "\tNetwork Inversion.\n" );
  setup_menu_inversion( frame );
#endif NEXUS_INVERSION

#ifdef NEXUS_RBF
  printf( "\tRadial Basis Functions.\n" );
  setup_menu_rbf( frame );
#endif NEXUS_RBF

/*  printf( "Initializing plot window. Please wait..." );
  do_plot( INITIALIZE );
  printf( "done.\n\n" ); */

  return NULL;
}






/*
 *  Destroy XView objects.
 */
int
cleanup_objects()
{
  xv_destroy( nexus_image );
  xv_destroy( nexus_image2 );
  xv_destroy( nexus_image3 );
  xv_destroy( nexus_image4 );
  xv_destroy( pan_left_image );
  xv_destroy( pan_right_image );
  xv_destroy( pan_up_image );
  xv_destroy( pan_down_image );
  xv_destroy( zoom_in_image );
  xv_destroy( zoom_out_image );
  xv_destroy( rotate_x_clk_image );
  xv_destroy( rotate_x_cclk_image );
  xv_destroy( rotate_y_clk_image );
  xv_destroy( rotate_y_cclk_image );
  xv_destroy( rotate_z_clk_image );
  xv_destroy( rotate_z_cclk_image );
  xv_destroy( frame );

  return OK;
}










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
