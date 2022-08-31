/**************************************************************
 *                                                            *
 *                            NEXUS                           *
 *                                                            *
 *              (c) 1990 Paul Sajda and Leif Finkel           *
 *                                                            *
 *                     (nexus_graphics.c)                     *
 *          DW 95.04.02  Generic graphics functions.          *
 *							      *
 **************************************************************/

#include "nexus.h"
#include "graphics.h"

#include "rbf.h"
#include "tkGLx.h"


/* 
 * Graph all cells of a network, given a pointer to the network.  DW 94.07.05
 */

void
graph_network( head )
    register NETWORK	head;
{
  register CELL		cell;
  register float	cell_size;
  register int		x, y;


  if (graphics_off)
    return;

  /*
   * New algorithm for drawing a network, eliminating such calls as
   * get_cell_size(), and drawcell(), and multiple push/pop-matrix's
   * as well as translate's.  DW 94.06.27
   */

  Translate( head->pos_x, head->pos_y );

  cell = head->cells;
  cell_size = head->cell_size;

#ifdef NEXUS_SUN
  polyfill( SET );
#endif

  SetColor( COLOR_BORDER );
  DrawRect( -OUTLINE_WIDTH, -OUTLINE_WIDTH,
	   cell_size * (float) head->dim_x + OUTLINE_WIDTH,
	   cell_size * (float) head->dim_y + OUTLINE_WIDTH );

  for (y = 0; y < (head->number_cells) / (head->dim_x); y++) {
    for (x = 0; x < head->dim_x; x++) {
      SetColor( get_color( (cell++)->firing_rate, ACTIVITY ) );
      DrawRect( x * cell_size, y * cell_size,
	       (x+1) * cell_size, (y+1) * cell_size );
    }
  }

  Translate( -(head->pos_x), -(head->pos_y) );

  draw_name( head );

#ifdef NEXUS_SGI
  GlxwinUpdate( 0, 0, screen_width, screen_height );
#endif

#ifdef NEXUS_LINUX
  /*
  GlxwinUpdate( 0, 0, screen_width, screen_height );
  */
  glFlush();
#endif
}  /* graph_network( ) */



/*
 *  Display single cell activity with appropriate color.  Cells start with
 *    id #1; head points to the network containing the cell.
 *  Cleaned up math, eliminated need for cell_position routines
 *    which wasted time.  DW 94.07.12
 */

void
display_cell_activity( head, cell_id )
    register NETWORK	head;
    register int	cell_id;
{
  register int		x, y;
  register float	cell_size;


  if (!head) {
#ifdef DEBUG
    fprintf( stderr, "WARNING: display_cell_activity( NETWORK == NULL )\n" );
#endif
    return;
  }

  if (cell_id < 1 || cell_id > head->number_cells)
    return;

  cell_size = head->cell_size;

  /* Integer division truncates any fractional part.  pg. 41, K&R C, 2nd ed. */
  x = (cell_id - 1) % head->dim_x;
  y = (cell_id - 1) / head->dim_x;

  SetColor( get_color( (head->cells + cell_id - 1)->firing_rate, ACTIVITY ) );
  DrawRect( head->pos_x + x * cell_size,
	   head->pos_y + y * cell_size,
	   head->pos_x + (x+1) * cell_size,
	   head->pos_y + (y+1) * cell_size );

#ifdef NEXUS_SGI
  glFlush( );
#endif

#ifdef NEXUS_LINUX
  glFlush( );
#endif
}  /* display_cell_activity( ) */



/*
 *  Clear network drawing area.
 */

void
clear_screen( )
{
#ifdef NEXUS_SGI
  glClear( GL_COLOR_BUFFER_BIT );
#endif
#ifdef NEXUS_LINUX
  glClear( GL_COLOR_BUFFER_BIT );
#endif
#ifdef NEXUS_SUN
  color( BLACK ); clear( );
#endif

  drawlegends( );

#ifdef NEXUS_SGI
  GlxwinUpdate( 0, 0, screen_width, screen_height );
#endif
#ifdef NEXUS_LINUX
  GlxwinUpdate( 0, 0, screen_width, screen_height );
#endif
}



/*
 *  select_network() and move_network() debugged, cleaned up.
 *  DW 94.07.31
 */

NETWORK
select_network( )
{
  static LOCATION curr_loc;
  NETWORK head;
  float cell_size;
  
  /*
   * unnecessary push/popmatrix()'s removed, cleaned up
   * DW 94.07.12
   */
  
  if (!curr_loc)
    curr_loc = (LOCATION) malloc( sizeof( LOCATION ) );

  if (locator_button == LEFT_BUTTON
      && (cell_select_mouse( curr_loc ))) {
    head = curr_loc->network;
    cell_size = head->cell_size;

    Translate( head->pos_x, head->pos_y );
    SetColor( WHITE );
    DrawRect( 0.0, 0.0,
	     cell_size * (float) head->dim_x,
	     cell_size * (float) head->dim_y );
    Translate( -(head->pos_x), -(head->pos_y) );

#ifdef NEXUS_SGI    
    GlxwinUpdate( 0, 0, screen_width, screen_height );
#endif
#ifdef NEXUS_LINUX
    GlxwinUpdate( 0, 0, screen_width, screen_height );
#endif

    return head;
  }

  /*
   * We are outside of the NET_SELECTED state,
   * the VIEW electrode is on, and a button other than
   * the LEFT BUTTON was pressed.  Keep us outside
   * of the NET_SELECTED state.
   */

  return NULL;
}  /* select_network( ) */

    

NETWORK
move_network( head )
    NETWORK head;
{
/*  int temp; */


  if (locator_button == RIGHT_BUTTON) {

#if 0
    /*  Align to "grid"; DW 94.08.16  */
    temp = (int) (locator_x / head->cell_size);
    head->pos_x = (float) temp * head->cell_size;
    temp = (int) (locator_y / head->cell_size);
    head->pos_y = ((float) (temp - head->dim_y + 1)) * head->cell_size;
    draw_outline( FALSE ); /* Just an outline */
#else

    head->pos_x = locator_x;
    head->pos_y = locator_y - head->dim_y * head->cell_size;
    draw_outline( FALSE );
#endif /* 0 */

    fprintf( stderr, "<%s> moved to position (%f,%f).\n",
	    head->name, head->pos_x, head->pos_y );

    /*
     *  Bring us back out of NET_SELECTED state.
     */
    return NULL;
  }

  /*
   * We are inside NET_SELECTED state, but
   * the user didn't press the RIGHT_BUTTON.
   * Stay in NET_SELECTED state and keep the
   * network's pointer.
   */

  return head;
}  /* move_network( ) */



/* 
 *  Handles ACTIVITY display of all networks.
 *    Put in nexus_activity.c, DW 95.04.02
 */

void
graph_activity( )
{
  NETWORK network = network_head;

  
#ifdef DEBUG
  fprintf( stderr, "graph_activity( )\n" );
#endif

  while (network) {
    graph_network( network );
    network = network->next;
  }
}  /* graph_activity( ) */



void
modify_activity( activity, type, flag )
    float activity;
    int type, flag;
{
  static LOCATION	curr_loc,
			prev_loc;
  static int		curr_mode = 0;


  /*  Allocate LOCATION variables' space when first called.  */
  if (!curr_loc) {
    curr_loc = (LOCATION) malloc( sizeof( ELEMENT_LOCATION ) );
    prev_loc = (LOCATION) malloc( sizeof( ELEMENT_LOCATION ) );
  }

  /*  Do nothing if currently out of bounds.  */
  if (!(cell_select_mouse( curr_loc )))
    return;

  if (flag == SET) {

    if (curr_mode) {
      if (!LOCATIONS_EQUAL( curr_loc, prev_loc ))
	cell_select_hilite( prev_loc, type, RESET, 0 );
    }

    curr_mode = 0;
    cell_select_hilite( curr_loc, type, SET, activity );
  }
  else if (flag == SHOW) {

    if (!curr_mode) {
      LOCATIONS_COPY( prev_loc, curr_loc );
      curr_mode = 1;
    }

    cell_select_hilite( curr_loc, type, SHOW, activity );
    if (!LOCATIONS_EQUAL( curr_loc, prev_loc ))
      cell_select_hilite( prev_loc, type, RESET, 0 );
    LOCATIONS_COPY( prev_loc, curr_loc );
  }
}  /* modify_activity( ) */



/* 
 *  Display connectivity of simulation by clicking on 
 *    the appropriate cell.
 *  Eliminated wasteful use of globals new_ret and old_ret.  DW 94.07.31
 *  Re-vamped algorithm to prevent ALL core dumps.  Make sure location
 *    is VALID (in a network) before doing ANYTHING.  Move globals into
 *    local statics, so that each electrode (select_network(), move_network(),
 *    do_connectivity(), and modify_activity() ) has their own current and
 *    previous location pointers.  DW 94.12.22
 *  Moved into nexus_connect.c from graphics files.  DW 95.03.31
 */

void
do_connectivity( show_conn, print_conn, send_conn_to_file )
    int show_conn, print_conn, send_conn_to_file;
{
  static LOCATION	curr_loc,
  			prev_loc;
  static int		valid_state = 0;


  /*  Allocate LOCATION variables' space when first called.  */
  if (!curr_loc) {
    curr_loc = (LOCATION) malloc( sizeof( ELEMENT_LOCATION ) );
    prev_loc = (LOCATION) malloc( sizeof( ELEMENT_LOCATION ) );
  }

  /*  Only Left Button used.  Ignore the others.  */
  if (locator_button != LEFT_BUTTON)
    return;

  /*  Do nothing if currently out of bounds.  */
  if (!(cell_select_mouse( curr_loc )))
    return;

  /*  On first valid location, set up LOCATION variables.  */
  if (!valid_state) {
    select_connections_hilite( curr_loc, SET,
			      show_conn, print_conn, send_conn_to_file );
    LOCATIONS_COPY( prev_loc, curr_loc );
    valid_state = 1;
    return;
  }

  /*  Don't waste time with the SAME location.  */
  if (LOCATIONS_EQUAL( curr_loc, prev_loc ))
    return;

  /*
   *  First reset the old location, then set the new location, then
   *    set the old to the new.
   */
  select_connections_hilite( prev_loc, RESET,
			    show_conn, print_conn, send_conn_to_file );
  select_connections_hilite( curr_loc, SET,
			    show_conn, print_conn, send_conn_to_file );
  LOCATIONS_COPY( prev_loc, curr_loc );

  return;
}



/*
 * Hilite the cell which was selected
 */

void
cell_select_hilite( location, type, set_flag, activity )
    LOCATION location;
    int type, set_flag;
    float activity;
{
  NETWORK	head;
  int		cell_id;
  CELL		cell;
  int		cell_color;
  int		x, y;
  float		cell_size;


  if (!(location->network))
    return;
  if (graphics_off)
    return;

  head = location->network;
  cell_id = location->cell_id;
  cell = head->cells + cell_id - 1;
  cell_size = head->cell_size;
  y = (int) ((cell_id - 1) / head->dim_x);
  x = (cell_id - 1) - y * head->dim_x;

  if (set_flag != RESET)
    printf( "Cell %d in Network %s:\n\tPosition x: %d, y: %d\n",
	   cell_id, head->name, x, y );

  if ((type == CONNECTIONS) && (set_flag == SET))
    cell_color = COLOR_COND_SET;

  if ((type == CONNECTIONS) && (set_flag == RESET))
    cell_color = COLOR_COND_BG;

  if ((type == ACTIVITY) && (set_flag == SET)) {
    cell->firing_rate = activity;
    cell->firing_rate_old = activity;
    printf( "\tActivity set to: %f\n", activity );
    cell_color = get_color( activity, ACTIVITY );
  }

  if ((type == ACTIVITY) && (set_flag == RESET))
    cell_color = get_color( (head->cells + cell_id - 1)->firing_rate,
			   ACTIVITY );

  if ((type == ACTIVITY) && (set_flag == SHOW)) {
    printf( "\tFiring rate: %f\n\tTotal input: %f\n\tThreshold: %f\n",
	   cell->firing_rate, cell->voltage, cell->threshold );

#ifdef NEXUS_RBF
    rbf_print_bias( head->id, cell_id );
#endif /* NEXUS_RBF */

    cell_color = COLOR_ACT_SHOW;
  }

  Translate( head->pos_x, head->pos_y );
  SetColor( cell_color );
  DrawRect( x * cell_size,
	   y * cell_size,
	   (x+1) * cell_size,
	   (y+1) * cell_size );
  Translate( -(head->pos_x), -(head->pos_y) );

#ifdef NEXUS_SGI
  GlxwinUpdate( 0, 0, screen_width, screen_height );
#endif
#ifdef NEXUS_LINUX
  GlxwinUpdate( 0, 0, screen_width, screen_height );
#endif

  return;
}  /* cell_select_hilite( ) */



/* 
 *  Determine which cells are connected to the selected 
 *    cell and hilite these cells.  The brightness of the
 *    cell represents the magnitude of the connection. 
 *  Updated to eliminate unnecessary calculation function calls,
 *    cleaned up.  DW 94.07.14
 */

int
select_connections_hilite( location, set_flag,
			  show_conn, print_conn, send_conn_to_file )
    LOCATION location;
    int set_flag, show_conn, print_conn, send_conn_to_file;
{
  NETWORK		head;
  CELL			cell;
  CONNECTION		connection;
  int			cell_color;
  register float	cell_x, cell_y;
  register int		count;
  float			cell_size;
#if 0
  /* ANTEROGRADE variables */
  SPECS			specs;
  int			start_cell, end_cell;
  register int		current_cell;
#endif /* 0 */


  if (!(location->network))
    return 0;
  if (graphics_off)
    return 0;

  head = location->network;
  cell = head->cells + (location->cell_id - 1);
  cell_select_hilite( location, CONNECTIONS, set_flag, 0 );


#if 0
  /* show ANTEROGRADE connections */
  if (show_conn == ANTEROGRADE) {
    specs = cell->specs;
    while (specs) {
      if (specs->type == ANTEROGRADE || specs->type == PGN_ANTEROGRADE) {

	head = *(network_xref_list + specs->id);

	start_cell = get_start_cell( specs->center_x, specs->center_y,
				    specs->max_width, specs->max_height,
				    head->dim_x, head->dim_y );
	end_cell = get_end_cell( specs->center_x, specs->center_y,
				specs->max_width, specs->max_height,
				head->dim_x, head->dim_y );
	cell_size = head->cell_size;
	
	for (current_cell = start_cell;
	     current_cell <= end_cell;
	     current_cell++) {
	  
	  if (check_for_connection( cell, head, current_cell )) {
	    if (set_flag == SET)
	      cell_color = get_connection_color( cell, head, current_cell );
	    else 
	      cell_color = COLOR_COND_BG;

	    cell_x = ((current_cell - 1) % head->dim_x) * cell_size;
	    cell_y = ((current_cell - 1) / head->dim_x) * cell_size;

	    Translate( head->pos_x + cell_x, head->pos_y + cell_y );
	    SetColor( cell_color );
	    DrawRect( 0.0, 0.0, cell_size, cell_size );
	    Translate( -(head->pos_x + cell_x), -(head->pos_y + cell_y) );
	  }  /* if */
	}  /* for (current_cell) */
	specs = specs->next;

      }  /* if (specs->type) */
    }  /* while (specs) */

  }  /* if ANTEROGRADE */
#endif /* 0 */


  /* show retrograde connections */
  if (show_conn == RETROGRADE) {

    if (!graphics_off) {
      for (count = 0; count < cell->number_connections; count++) {
	connection = cell->connect_list + count;
	if (set_flag == SET)
	  cell_color = get_color( connection->conductance, CONNECTIONS );
	else 
	  cell_color = COLOR_COND_BG;

	head = *(network_xref_list + connection->input_cell->net_id);

	cell_size = head->cell_size;
	cell_x = (((connection->input_cell->id - 1) % head->dim_x)
		  * cell_size + head->pos_x);
	cell_y = (((connection->input_cell->id - 1) / head->dim_x)
		  * cell_size + head->pos_y);

	SetColor( cell_color );
	DrawRect( cell_x, cell_y, cell_x + cell_size, cell_y + cell_size );
      }  /* for (cell->number_connections) */

    }  /* if (!graphics_off) */
    
    if (print_conn == ON && set_flag == SET)
      printConnections( show_conn, cell );

    if (send_conn_to_file == ON && set_flag == SET)
      if (saveConnections( show_conn, cell ) == ERROR)
	return 0;

  }  /* RETROGRADE */

#ifdef NEXUS_SGI
  GlxwinUpdate( 0, 0, screen_width, screen_height );
#endif
#ifdef NEXUS_LINUX
  GlxwinUpdate( 0, 0, screen_width, screen_height );
#endif

  return 0;
}  /* select_connections_hilite( ) */



/*
 *  Main color ranging function.
 *  Made activity and conductivity use similar legend and colour scale
 *  DW 94.06.16
 */

int
get_color( value, type )
    register float value;
    register int type;
{
  register int temp;
  register int index;
  register float min, max;



  if (type == ACTIVITY) {
    min = activity_display_range_min;
    max = activity_display_range_max;
#ifdef NEXUS_SGI
    index = ACT_INDEX + (activity_display_depth == DISP_GREY ? GREY_INDEX : 0);
#endif
#ifdef NEXUS_LINUX
    index = ACT_INDEX + (activity_display_depth == DISP_GREY ? GREY_INDEX : 0);
#endif
#ifdef NEXUS_SUN
    index = ACT_INDEX;
#endif
  }
  else if (type == CONNECTIONS) {
    min = conduct_display_range_min;
    max = conduct_display_range_max;
#ifdef NEXUS_SGI
    index = COND_INDEX + (conduct_display_depth == DISP_GREY ? GREY_INDEX : 0);
#endif
#ifdef NEXUS_LINUX
    index = COND_INDEX + (conduct_display_depth == DISP_GREY ? GREY_INDEX : 0);
#endif
#ifdef NEXUS_SUN
    index = COND_INDEX;
#endif
  }
  else
    return BLACK;		/* Keep program from crashing and burning */

  /*
   *  Generalized sub-ranging routine follows; the actual formula is:
   *
   *  size_of_sub_section = (range_max - range_min) / number_of_sections;
   *  section_number = floor( (value - range_min) / size_of_sub_section );
   * 
   *  DW 94.07.06
   */

  temp = ((int) ((value - min) * NUM_COLOURS / (max - min) ));

  if (temp < 0)
    temp = 0;
  else if (temp > NUM_COLOURS - 1)
    temp = NUM_COLOURS - 1;

  return (temp + index);
}



/*
 *  Check to see if cell is connected to selected cell.
 *  Speed counts very much here, so register those automatics and params!
 *  "number" assignment should be kept to prevent multiple evaluations
 *   in the loop.
 *     "check_cell" is now "connection".
 *     "connection = connection + 1" is now "connection++".
 *    DW 94.07.31
 *  Moved into nexus_connect.c from graphics files.  DW 95.03.31
 */

int
check_for_connection( cell, head, current_cell )
    register CELL cell;
    register NETWORK head;
    register int current_cell;
{
  register CONNECTION connection;
  register int count = 0;
  register int number;

  
  if ((head->dim_x * head->dim_y) < current_cell)
    return FALSE;

  connection = (head->cells + (current_cell - 1))->connect_list;
  number = (head->cells + (current_cell - 1))->number_connections;

  while (count++ < number)
    if ((connection++)->input_cell == cell)
      return TRUE;

  return FALSE;
}



/*
 *  Get color which corresponds to magnitude of conductance.  Only used
 *    for ANTEROGRADE connections.  SPEED COUNTS, so register all.  DW 94.07.31
 *  Moved into nexus_connect.c from graphics files.  DW 95.03.31
 */

int
get_connection_color( cell, head, current_cell )
    register CELL cell;
    register NETWORK head;
    register int current_cell;
{
  register CONNECTION connection;
  register int count = 0;
  register int number;
  

  connection = (head->cells + (current_cell - 1))->connect_list;
  number = (head->cells + (current_cell - 1))->number_connections;

  while (count++ < number)
    if ((connection++)->input_cell == cell)
      return (get_color( connection->conductance, CONNECTIONS ));

  /* Changed to produce meaningful result if error should occur, DW 94.07.31 */
  return BLACK;
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
