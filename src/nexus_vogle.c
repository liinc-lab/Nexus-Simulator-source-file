/**************************************************************
 *                                                            *
 *                            NEXUS                           *
 *                                                            *
 *              (c) 1990 Paul Sajda and Leif Finkel           *
 *                                                            *
 *                      (nexus_vogle.c)                       *
 *							      *
 *	 modifications by Daniel Widyono, Jun '94 - Apr '95   *
 *                                                            *
 **************************************************************/

/*
 *                                    
 * Graphics routines to display network activity
 * and connectivity.  Based on VOGLE graphics library
 *
 */

#include "nexus.h"
#include "graphics.h"



/**
 **  Function prototypes.
 **/

static void	init_legends();

/**
 **  Global variables.
 **/

extern Display *	dpy;

/*
 *  Offsets to keep legends stationary regardless of View translations.
 */

static float		legend_x = 0.0,
			legend_y = 0.0,
			legend_z = LOOKAT_PZ;

float			translation_amount = 100.0;

/*
 *  Colormaps (blue->cyan->green->yellow->red), (dark_grey->light_grey)
 */

static int legend_red [NUM_COLOURS] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,128,159,191,223,
  255,255,255,255,255,255,255
};

static int legend_green [NUM_COLOURS] = {
   64, 96,128,159,191,223,255,255,
  255,255,255,255,255,255,255,255,
  255,223,191,159,128, 96, 64
};

static int legend_blue [NUM_COLOURS] = {
  255,255,255,255,255,255,255,223,
  191,159,128, 96,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0
};

static int legend_grey [NUM_COLOURS] = {
   56, 64, 73, 81, 90, 98,107,115,
  124,132,141,149,158,166,175,183,
  192,200,209,217,226,234,243
};



/***********
 *  nexus_vogle.c
 ***********/

/*  Initialization  */

#if 0
/* 
 * Initializes and creates Plot window.  Side-effect: calling
 * the vinit routine also initializes all vogle routines.  This
 * must be the first init. routine called, therefore.
 */

void
init_plot_window( win )
    Window win;
{
  vinit( "X11", win, PLOT_WINDOW, dpy );
  clipping( FALSE );
}
#endif 0



/* 
 * Initializes and creates VOGLE graphics window.  Note 
 * that we cannot get a handle on the window and therefore 
 * must treat it differently from a normal X window.
 */

void
init_graphics_window( win )
    Window win;
{
  vinit( "X11", win, GRAPHICS_WINDOW, dpy );
/*  vnewdev( "X11", win, GRAPHICS_WINDOW, dpy ); */
  clipping( FALSE );
  
  window( V_WIN_LEFT, V_WIN_RIGHT,
	 V_WIN_BOTTOM, V_WIN_TOP,
	 V_WIN_NEAR, V_WIN_FAR );
  lookat( 0.0, 0.0, LOOKAT_VZ, 0.0, 0.0, LOOKAT_PZ, 0.0 );
  viewport( VIEW_LEFT, VIEW_RIGHT, VIEW_BOTTOM, VIEW_TOP );
}



/*
 *  Any miscellaneous graphics-specific initialization is performed here
 *    and is executed after the vinit() call (see init_plot_window() ).
 * Initializes colour indices for VOGLE drawing , DW 94.06.15
 */

void
init_graphics( )
{
  register	i;


  polyfill( TRUE );

  /*
   * Maps NUM_COLOURS*2 colours (one set for activity, one for connectivity).
   * DW 94.06.23
   *
   * Sometime this code will need to be bullet-proofed to handle running
   * out of colour cells (add code to allocate new colormap, or
   * simply not run, or move down to less colours)
   */
  
  /*
   * BLACK and WHITE, background and foreground, respectively, are already
   * assigned in vogle.h and allocated, thus we are only looking for
   * NUM_COLOURS * 2 allocations (free colour cells)
   */

  for (i = 0; i < NUM_COLOURS; i++) {
    mapcolor( ACT_INDEX + i, legend_red[i], legend_green[i], legend_blue[i] );
    mapcolor( COND_INDEX + i, legend_red[i], legend_green[i], legend_blue[i] );
  }

  init_legends( );
}  



void
intro_graphics( )
{
  color( WHITE );
  clear( );
  color( BLACK );

  font( "times.rb" );
  boxtext( 20.0, 90.0, 180.0, 50.0, "NEXUS" );
  boxtext( 0.0, 40.0, 200.0, 22.5, "Neural Simulator" );

  font( "times.r" );
  boxtext( 20.0, 10.0, 180.0, 7.5, "(c) 1990 Paul Sajda and Leif Finkel" );
  boxtext( 20.0, 0.0, 180.0, 7.5, "University of Pennsylvania" );

  font( "futura.l" );
}



/*
 *  Last graphic routine to be called, shuts down (VOGLE) graphics library
 *  and cleans up after X Toolkit.  DW 94.07.29
 */

int
cleanup_graphics( )
{
  extern int cleanup_objects( );
  int result;

  /* LAST VOGLE CALL */
  vexit();

  if (result = cleanup_objects())
    return result;
  else
    return OK;
}



/*
 *  Draws network's name (char head->name[]) above network graphic.
 *    Current xform matrix must be at (0.0, 0.0, 0.0).
 *    2 is added to the y position to space the name from the network.
 *  The original name_size can be affected by the text_width and
 *    text_height parameters in the .nx file.
 *  DW 94.07.14 (Vogle version)
 */

void
draw_name( head )
    NETWORK head;
{
  color( WHITE );
  boxtext( head->pos_x,
	  head->cell_size * (float) head->dim_y + head->pos_y + 2.0,
	  head->name_size_w * 80.0 * NETTEXT_WIDTH,
	  head->name_size_h * NETTEXT_HEIGHT,
	  head->name );
}



/*
 *  Initialize network drawing area.
 *    If flag is TRUE then fill, else use wire frame.
 */

void
draw_outline( flag )
    int flag;
{
  register NETWORK head;
  register float cell_size;
  register int temp_colour;
  
  head = network_head;

  clear_screen( );
  if (flag)
    temp_colour = COND_INDEX;	/* COND_INDEX ? perhaps better.  DW 94.07.14*/
  else
    temp_colour = ACT_INDEX;
  
  while (head) {

    polyfill( FALSE );
    translate( head->pos_x, head->pos_y, 0.0 );
    cell_size = head->cell_size;

    if (flag) {
      color( WHITE );
      polyfill( FALSE );
      makepoly();
      rect( -1.0, -1.0,
	   cell_size * (float) head->dim_x + 1,
	   cell_size * (float) head->dim_y + 1 );
      closepoly();
      polyfill( TRUE );
    }

    color( temp_colour );
    makepoly();
    rect( 0.0, 0.0,
	 cell_size * (float) head->dim_x,
	 cell_size * (float) head->dim_y );
    closepoly();
    polyfill( TRUE );

    translate( -(head->pos_x), -(head->pos_y), 0.0 );

    draw_name( head );

    head = head->next;
  }
}


    
/*
 *  Function to zoom and translate networks on screen display.
 *    'flag' indicates whether color filling is on.
 */

void
translate_network( axis, move_amount, flag )
    char axis;
    float move_amount;
    int flag;
{
  /*
   * Translated (no pun intended) into a switch statement.  More robust.
   * DW 94.07.12
   */
  switch (axis) {
  case 'x':
    translate( move_amount, 0.0, 0.0 );
    legend_x -= move_amount;
    break;
  case 'y':
    translate( 0.0, move_amount, 0.0 );
    legend_y -= move_amount;
    break;
  case 'z':
    /*
     * Adapted to keep view from turning upside down (VOGLE bug) in 
     * perspective transformations.  Basically, keep the networks in 
     * front of the viewpoint.   DW 940629
     */

    if (move_amount <= 0 ||
	(move_amount > 0 && legend_z + move_amount <= LOOKAT_VZ)) {
      translate( 0.0, 0.0, move_amount );
      legend_z += move_amount;
    }
    break;			/* This is for insurance, in the event that
				   someone adds another case... */
  }

  draw_outline( flag );
}



/*
 *  Given locator position (in screen coordinates), translate into
 *  a network and cell id.  Return TRUE if a cell is found.
 *  DW 94.07.31
 */

int
cell_select_mouse( location )
    LOCATION location;
{
  float size, pos_x, pos_y;
  NETWORK head;


  head = network_head;
  while (head) {
    size = head->cell_size;
    pos_x = head->pos_x;
    pos_y = head->pos_y;
    
    /*
     *  Math fixed. DW 94.06.16
     *  Vogle bug fixed.  (See matrix.c in vogle source code)  DW 94.07.30
     *
     *  If we're in a network, return the network's ID and cell ID
     */

    if ((locator_x + 1      >    pos_x)
	&& (locator_x + 1   <=   pos_x + head->dim_x * size)
	&& (locator_y + 1   >    pos_y)
	&& (locator_y + 1   <=   pos_y + head->dim_y * size)) {

      location->network = head;
      location->cell_id = ( ((int) ((locator_y + 1 - pos_y) / size)) * head->dim_x
			   + ((int) ((locator_x + 1 - pos_x) / size)) + 1 );
      return TRUE;
    }
    head = head->next;
  }
  return FALSE;
}  /* cell_select_mouse( ) */



/*
 *  Draw color legends in top 3/20th of the screen.  (DW 94.06.27)
 *
 *  Added object code, DW 94.07.12
 *
 *  Note: this is the ONLY place we should be changing the viewport!
 *    Thus, let us first save the VIEW viewport on the stack -- pushviewport().
 */

void
drawlegends( )
{
  static char number[10];
  
  pushviewport();
  pushmatrix();

  /*
   * Undo all the transformations from moving the networks around.
   */
  viewport( LEGEND_VL, LEGEND_VR, LEGEND_VB, LEGEND_VT );
  translate( legend_x, legend_y, -legend_z + LEGEND_POS_Z );

  /*
   * Draw graphics bars by calling the object.  Faster than drawing each time.
   */
  callobj( OBJ_LEGEND );

  /*
   * Write out Minimum and Maximum Range values on either ends of the legends
   */
  color( WHITE );
  sprintf( number, "%9.3f\0", activity_display_range_min );
  boxtext( LEGEND_POS_X + LEGEND_NAME_WIDTH, LEGEND_ACT_POS_Y,
	  LEGEND_TEXT_WIDTH, LEGEND_TEXT_HEIGHT, number );
  sprintf( number, "%9.3f\0", conduct_display_range_min );
  boxtext( LEGEND_POS_X + LEGEND_NAME_WIDTH, LEGEND_COND_POS_Y,
	  LEGEND_TEXT_WIDTH, LEGEND_TEXT_HEIGHT, number );
  sprintf( number, "%9.3f\0", activity_display_range_max );
  boxtext( (LEGEND_BAR_WIDTH*(NUM_COLOURS+1) + LEGEND_POS_X
	    + LEGEND_TEXT_WIDTH + LEGEND_NAME_WIDTH), LEGEND_ACT_POS_Y,
	  LEGEND_TEXT_WIDTH, LEGEND_TEXT_HEIGHT, number );
  sprintf( number, "%9.3f\0", conduct_display_range_max );
  boxtext( (LEGEND_BAR_WIDTH*(NUM_COLOURS+1) + LEGEND_POS_X
	    + LEGEND_TEXT_WIDTH + LEGEND_NAME_WIDTH), LEGEND_COND_POS_Y,
	  LEGEND_TEXT_WIDTH, LEGEND_TEXT_HEIGHT, number );
  
  popmatrix();
  popviewport();
}

static void
init_legends()
{
  int i;

  makeobj( OBJ_LEGEND );

  color( WHITE );

  boxtext( LEGEND_POS_X, LEGEND_ACT_POS_Y,
	  LEGEND_NAME_WIDTH, LEGEND_TEXT_HEIGHT,
	  "ACTIVITY:      " );
  
  boxtext( LEGEND_POS_X, LEGEND_COND_POS_Y,
	  LEGEND_NAME_WIDTH, LEGEND_TEXT_HEIGHT,
	  "CONDUCTIVITY:  " );

  for( i=0; i<NUM_COLOURS; i++) {
    color( i+ACT_INDEX );
    makepoly();
    rect( (LEGEND_BAR_WIDTH*(i+1) + LEGEND_POS_X + LEGEND_TEXT_WIDTH + LEGEND_NAME_WIDTH), LEGEND_ACT_POS_Y, (LEGEND_BAR_WIDTH*(i+2) + LEGEND_POS_X + LEGEND_TEXT_WIDTH + LEGEND_NAME_WIDTH), LEGEND_ACT_POS_Y + LEGEND_BAR_HEIGHT );
    closepoly();

    color( i+COND_INDEX );
    makepoly();
    rect( (LEGEND_BAR_WIDTH*(i+1) + LEGEND_POS_X + LEGEND_TEXT_WIDTH + LEGEND_NAME_WIDTH), LEGEND_COND_POS_Y, (LEGEND_BAR_WIDTH*(i+2) + LEGEND_POS_X + LEGEND_TEXT_WIDTH + LEGEND_NAME_WIDTH), LEGEND_COND_POS_Y + LEGEND_BAR_HEIGHT );
    closepoly();
  }

  closeobj( OBJ_LEGEND );
}



/*
 *  DW 95.03.15
 */

void
resetView( )
{
  legend_x = 0.0;
  legend_y = 0.0;
  legend_z = LOOKAT_PZ;
}  /* resetView( ); */



void
updateNetPositions( )
{
  NETWORK tempNet = network_head;


  while (tempNet) {
    tempNet->pos_x -= legend_x;
    tempNet->pos_y -= legend_y;
    tempNet->cell_size *= legend_z / (LOOKAT_VZ - LOOKAT_PZ) + 1.0;
    tempNet = tempNet->next;
  }
  resetView( );
  redrawGraphics( );

}  /* updateNetPositions( ); */



/*
 *  'number_networks()' replaced by the global variable in nexus_lex_build.l
 *    (and thus nexus_lex_build.c).  DW 94.07.13
 */


#if 0

/*
 *  This routine is not currently used.  See show_all_connections().
 *  DW 94.06.15
 */

/*
 * Display all connections in simulation as color lines between points
 */

void
all_connections_graphics()
{
  NETWORK head;
  void graph_connections();
  
  color( BLACK ); clear();
  drawlegends();

  head = network_head;
  while( head ) {
    translate( head->pos_x, head->pos_y, 0.0 );
    graph_connections( head );
    translate( -(head->pos_x), -(head->pos_y), 0.0 );
    head = head->next;
  }
}

/*
 *  This routine is not currently used.
 *  DW 94.06.27
 */

/* 
 * Used to display all connections in the network.  Current
 * display matrix is the head of the network (head->pos_x, head->pos_y, 0.0).
 * DW 94.07.14
 */

void
graph_connections( head )
    NETWORK head;
{
  CELL cell;
  int id, cell_color;
  float cell_size;
  int number_connections;
  CONNECTION connection;
  NETWORK connected_network, current_network, get_network_id();
  int x_offset, y_offset;

  cell = head->cells;

  pushmatrix();
  pushmatrix();
  
  for( id = 1;id <= head->number_cells;id++ ) {
    cell_size = head->cell_size;
    cell_color = COND_INDEX;

    connection = cell->connect_list;
    current_network = get_network_id(cell->net_id);
    
    color( cell_color );
    makepoly();
    rect( 0.0, 0.0, cell_size, cell_size );
    closepoly();
    
    for( number_connections = 0;
	number_connections < cell->number_connections;
	number_connections++ ) {

      color( get_color( connection->conductance,CONNECTIONS ) );
      connected_network = get_network_id( connection->input_cell->net_id );
      
      x_offset = ( (connection->input_cell->id - 1) % connected_network->dim_x
		  - (cell->id - 1) % current_network->dim_x );

      y_offset = ( (connection->input_cell->id - 1) / connected_network->dim_x
		  - (cell->id - 1) / current_network->dim_x );

      /*
       *  insert something about DRAWING the actual CONNECTION...
       *  :(  DW 94.07.14
       */

      connection++;
    }
    
    cell++;
    if( id > 1 )
      if( ! ((id - 1) % head->dim_x) ) {
	popmatrix();
	translate( 0.0, cell_size, 0.0 );
	pushmatrix();
      }
      else
	translate( cell_size, 0.0, 0.0 );
  }

  popmatrix();
  popmatrix();

  draw_name( head );
}
#endif 0


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
