/**************************************************************
 *                                                            *
 *                            NEXUS                           *
 *                                                            *
 *              (c) 1990 Paul Sajda and Leif Finkel           *
 *                                                            *
 *                      (nexus_opengl.c)                      *
 *							      *
 *           Conversion from vogle code, DW 94.08.31          *
 *							      *
 **************************************************************/

/*
 *                                    
 * Graphics routines to display network activity
 * and connectivity.  Based on OpenGL graphics library.
 *
 */

#include "nexus.h"
#include "graphics.h"
#include "tkGLx.h"
#include "nexus_opengl.h"


/**
 **  Function prototypes.
 **/


/**
 **  Global variables.
 **/

extern Display *	dpy;
int			backing_store = ON;
static int		glXErrorBase;

/*
 *  Offsets to keep legends stationary regardless of View translations.
 */

static float		legend_x = 0.0,
			legend_y = 0.0,
			legend_z = LOOKAT_PZ;

float			translation_amount = 10.0;

/*
 *  Font variables.  A 'font' in OpenGL is actually a group of display lists,
 *    called with an offset defined when the 'font' is created (see
 *    makeBitmapFont()).  DW 94.10.09
 */

static GLuint		currFontOffset;
static GLuint		fontList[3];

/*
 *  For the refresh routines.  DW 94.10.31
 */

GLint			screen_width;
GLint			screen_height;

/*
 *  Colour section.
 */

float			rgb_index[(NUM_COLOURS * 4 + 2)][3];

/*
 *  Colormaps (blue->cyan->green->yellow->red), (dark_grey->light_grey)
 */

static int legend_red [NUM_COLOURS] = {
    0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
   16, 32, 48, 64, 80, 96,112,128,
  144,159,175,191,207,223,239,255,
  255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255
};

static int legend_green [NUM_COLOURS] = {
    0,  0,  0,  0,  0,  0,
   16, 32, 48, 64, 80, 96,112,128,
  144,159,175,191,207,223,239,255,
  255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,
  239,223,207,191,175,159,144,128,
  112, 96, 80, 64, 48, 32, 16,  0
};

static int legend_blue [NUM_COLOURS] = {
   96,128,159,191,223,255,
  255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,
  239,223,207,191,175,159,144,128,
  112, 96, 80, 64, 48, 32, 16,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0
};

static int legend_grey [NUM_COLOURS] = {
    7, 10, 14, 17, 21, 24, 28, 31,
   35, 38, 42, 45, 49, 52, 56, 59,
   63, 66, 70, 73, 77, 80, 84, 87,
   90, 93, 97,100,104,107,111,114,
  118,121,125,128,132,135,139,142,
  146,149,153,156,160,163,167,170,
  174,177,181,184,188,191,195,198,
  202,205,209,212,216,219,223,226,
  230,233,237,240,244,247
};





/***********
 *  nexus_opengl.c
 ***********/

/*  Initialization  */

GLuint
makeBitmapFont( fontName )
    char *fontName;
{
  XFontStruct *	fontInfo;
  Font		id;
  unsigned int	first, last;
  GLuint	base;


  /*
   *  This routine was basically lifted from the OpenGL Reference Manual,
   *    Addison-Wesley Publishing Company.  DW 94.10.09
   */

  fontInfo = XLoadQueryFont( dpy, fontName );
  if (fontInfo == NULL) {
    printf( "ERROR: No font found.\n" );
    exit( 0 );
  } 

  id = fontInfo->fid;
  first = fontInfo->min_char_or_byte2;
  last = fontInfo->max_char_or_byte2; 
  if ((base = glGenLists( last + 1 )) == 0) {
    printf( "GL ERROR: Out of display lists.\n" );
    exit( 0 );
  }

  glXUseXFont( id, first, last - first + 1, base + first );

  return base;
}



GLuint
makeStrokeFont( )
{
  GLuint	base;


  if ((base = glGenLists( 256 )) == 0) {
    printf( "GL ERROR: Out of display lists.\n" );
    exit( 0 );
  }

  tkCreateFilledFont( base );

  return base;
}



/*
 *  Any miscellaneous graphics-specific initialization is performed here
 *    OpenGL version, DW 94.10.30
 */

void
init_graphics( )
{
  register int	i;
  float		scale = legend_z / LOOKAT_PZ;

#ifdef DEBUG
  int		screenDepth = ( getgdesc( GL_GD_BITS_NORM_SNG_RED )
			       + getgdesc( GL_GD_BITS_NORM_SNG_GREEN )
			       + getgdesc( GL_GD_BITS_NORM_SNG_BLUE ) );

  fprintf( stderr, "init_graphics( ), screenDepth = %d\n", screenDepth );
#endif

  if ((glXQueryExtension( dpy, &glXErrorBase, NULL )) == GL_FALSE) {
    fprintf( stderr, "GLX Extension is not supported on this display.\n" );
    exit(1);
  }

  /*
   *  OpenGL State.
   */

  glDrawBuffer( GL_FRONT_AND_BACK );
  glShadeModel( GL_FLAT );
  glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
  glClearColor( 0.0, 0.0, 0.0, 1.0 );

  /*************************************************************************
   *  Set up viewing matrices.
   *************************************************************************/

  /*
   *  Old method: use 3D perspective to achieve zoom.
   *    glFrustum( -2.0, 2.0, -2.0, 2.0, 1000.0, 1000000.0 );
   *    gluLookAt( 0.0, 0.0, LOOKAT_VZ, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0 );
   *  New method: zoom entire Ortho window.
   */

  glMatrixMode( GL_PROJECTION );
  glLoadIdentity( );
  glOrtho( VIEW_LEFT / scale, VIEW_RIGHT / scale,
	  VIEW_BOTTOM / scale, VIEW_TOP / scale,
	  -100.0, 100.0 );
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity( );

  /*
   *  DW 94.11.02 Color set-up.  DW 95.02.11: don't use grey-scale.
   *  DW 95.03.16: add user choice of grey-scale vs. colour.
   */

  rgb_index[BLACK][0] = 0.0;
  rgb_index[BLACK][1] = 0.0;
  rgb_index[BLACK][2] = 0.0;
  rgb_index[WHITE][0] = 1.0;
  rgb_index[WHITE][1] = 1.0;
  rgb_index[WHITE][2] = 1.0;

  for (i = 0; i < NUM_COLOURS; i++) {
    rgb_index[ACT_INDEX + i][0] = legend_red[i] / 255.0;
    rgb_index[ACT_INDEX + i][1] = legend_green[i] / 255.0;
    rgb_index[ACT_INDEX + i][2] = legend_blue[i] / 255.0;
    rgb_index[COND_INDEX + i][0] = legend_red[i] / 255.0;
    rgb_index[COND_INDEX + i][1] = legend_green[i] / 255.0;
    rgb_index[COND_INDEX + i][2] = legend_blue[i] / 255.0;
    rgb_index[ACT_INDEX + GREY_INDEX + i][0] = legend_grey[i] / 255.0;
    rgb_index[ACT_INDEX + GREY_INDEX + i][1] = legend_grey[i] / 255.0;
    rgb_index[ACT_INDEX + GREY_INDEX + i][2] = legend_grey[i] / 255.0;
    rgb_index[COND_INDEX + GREY_INDEX + i][0] = legend_grey[i] / 255.0;
    rgb_index[COND_INDEX + GREY_INDEX + i][1] = legend_grey[i] / 255.0;
    rgb_index[COND_INDEX + GREY_INDEX + i][2] = legend_grey[i] / 255.0;
  }

#ifdef DEBUG

  for (i = 2; i < NUM_COLOURS * 4 + 2; i++) {
    fprintf( stderr, "rgb[%d] = {%f,%f,%f}\n",
	    i, rgb_index[i][0], rgb_index[i][1], rgb_index[i][2] );
  }
#endif /* DEBUG */

  /*
   *  Set up font.  DW 94.11.01
   */

  fontList[0] = makeBitmapFont( MAIN_FONT );
  fontList[1] = makeStrokeFont( );

  currFontOffset = fontList[0];
}



void
printString( x, y, s )
    double	x, y;
    char	*s;
{
  glPushAttrib( GL_COLOR_BUFFER_BIT );
  glColor3fv( rgb_index[COLOR_TEXT] );

  if (currFontOffset == fontList[0]) { /* BITMAP FONT */
    glRasterPos2d( x, y );
    glListBase( currFontOffset );
    glCallLists( strlen( s ), GL_UNSIGNED_BYTE, (unsigned char *) s );
  }
  else {  /* STROKE FONT */
    glPushMatrix( );
    glTranslated( x, y, 0.0 );
    glListBase( currFontOffset );
    glCallLists( strlen( s ), GL_UNSIGNED_BYTE, (unsigned char *) s );
    glPopMatrix( );
  }

  glPopAttrib( );
  glFlush( );
}  /* printString() */



/*
 *  DW 94.10.30- Not really necessary anymore, but hey, you never know...
 */
int
cleanup_graphics()
{
#ifdef DEBUG
  fprintf( stderr, "cleanup_graphics( )\n" );
#endif

  return OK;
}



/*
 *  Draws network's name (char head->name[]) above network graphic.
 *    Current xform matrix must be at (0.0, 0.0, 0.0).
 *    2 is added to the y position to space the name from the network.
 *  The original name_size can be affected by the text_width and
 *    text_height parameters in the .nx file.
 *  DW 94.10.30 (OpenGL version)  Scaling added DW 95.03.20
 */

void
draw_name( head )
    NETWORK head;
{
  glPushAttrib( GL_COLOR_BUFFER_BIT | GL_LIST_BIT );
  glColor3fv( rgb_index[COLOR_TEXT] );

  glPushMatrix( );
  glTranslated( head->pos_x,
	       head->cell_size * head->dim_y + head->pos_y + 2.0,
	       0.0 );
  glScalef( head->name_size_w, head->name_size_h, (float) 1.0 );
  glListBase( fontList[1] );
  glCallLists( strlen( head->name ),
	      GL_UNSIGNED_BYTE,
	      (unsigned char *) head->name );
  glPopMatrix( );
  glPopAttrib( );
}



/*
 *  Transform window coordinates to world coordinates.  DW 94.11.12
 */

void
windowToWorld( x, y, worldxp, worldyp )
    float	x, y;
    float	*worldxp, *worldyp;
{
/*  double normX, normY, scale = legend_z / 10000.0; */
  double normX, normY, scale = legend_z / LOOKAT_PZ;


  normX = (double) (2.0 * ((x - screen_width / 2.0) / screen_width)) * 100.0;
  normY = (double) ((2.0 * (screen_height - y) / screen_height) - 1.0) * 100.0;

  *worldxp = (normX - legend_x) / scale;
  *worldyp = (normY - legend_y) / scale;
}



void
update_size( width, height )
    int width, height;
{
  float		scale = legend_z / LOOKAT_PZ;


  screen_width = (GLint) width;
  screen_height = (GLint) height;

  glViewport( 0, 0, width, height );
  glScissor( 0, 0, width, height - 60);
  /*
   *  Old method: use 3D perspective to achieve zoom.
   *    glFrustum( -2.0, 2.0, -2.0, 2.0, 1000.0, 1000000.0 );
   *    gluLookAt( 0.0, 0.0, LOOKAT_VZ, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0 );
   *  New method: zoom entire Ortho window.
   */

  glMatrixMode( GL_PROJECTION );
  glLoadIdentity( );
  glOrtho( (VIEW_LEFT - legend_x) / scale, (VIEW_RIGHT - legend_x) / scale,
	  (VIEW_BOTTOM - legend_y) / scale, (VIEW_TOP - legend_y) / scale,
	  -100.0, 100.0 );

  glMatrixMode( GL_MODELVIEW );
  glEnable( GL_SCISSOR_TEST );
}



/*
 * Initialize network drawing area.
 */

void
draw_outline( flag )
    int flag;
{
  register NETWORK head;
  register float cell_size;


#ifdef DEBUG
  fprintf( stderr, "draw_outline( %s );\n", flag ? "TRUE" : "FALSE" );
#endif

  if (graphics_off)
    return;

  head = network_head;
  clear_screen( );
  
  while (head) {
    glTranslatef( head->pos_x,  head->pos_y,  0.0 );
    cell_size = head->cell_size;

    glColor3fv( rgb_index[COLOR_BORDER] );
    glRectf( -OUTLINE_WIDTH, -OUTLINE_WIDTH,
	    cell_size * (float) head->dim_x + OUTLINE_WIDTH,
	    cell_size * (float) head->dim_y + OUTLINE_WIDTH );

#if 0
    glColor3fv( rgb_index[(flag ? COLOR_COND_BG : COLOR_VIEW_BG)] );
#else
    glColor3fv( rgb_index[COLOR_COND_BG] );
#endif

    glRectf( 0.0, 0.0,
	    cell_size * (float) head->dim_x,
	    cell_size * (float) head->dim_y );

    glTranslatef( -(head->pos_x),  -(head->pos_y),  0.0 );
    draw_name( head );
    head = head->next;
  }

  GlxwinUpdate( 0, 0, screen_width, screen_height );
}



/*
 *  Function to zoom and translate networks on screen display.
 *    'flag' indicates whether color filling is on.
 *  No longer translate for this: we now move the clipping planes instead
 *    with glOrtho( ).  DW 94.11.12
 */

void
translate_network( axis, move_amount, flag )
    char axis;
    float move_amount;
    int flag;
{
  float		scale;


  /*
   * Translated (no pun intended) into a switch statement.  More robust.
   * DW 94.07.12
   */
  switch (axis) {
  case 'x':
    legend_x += move_amount;
    break;
  case 'y':
    legend_y += move_amount;
    break;
  case 'z':
    if ((legend_z + move_amount) > 0.0)
      legend_z += move_amount;
    break;
  }

  scale = legend_z / LOOKAT_PZ;

  glMatrixMode( GL_PROJECTION );
  glLoadIdentity( );
  glOrtho( (VIEW_LEFT - legend_x) / scale, (VIEW_RIGHT - legend_x) / scale,
	  (VIEW_BOTTOM - legend_y) / scale, (VIEW_TOP - legend_y) / scale,
	  -100.0, 100.0 );
  glMatrixMode( GL_MODELVIEW );

  draw_outline( FALSE );
}



/*
 *  Given locator position (in world coordinates), translate into
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
     *  Math fixed.  DW 94.06.16
     *  Math altered to fit Tcl world coordinates.  DW 94.11.15
     *
     *  If we're in a network, return the network's ID and cell ID
     */

    if ((locator_x      >=   pos_x)
	&& (locator_x   <    pos_x + head->dim_x * size)
	&& (locator_y   >=   pos_y)
	&& (locator_y   <    pos_y + head->dim_y * size)) {

      location->network = head;
      location->cell_id = ( ((int) ((locator_y - pos_y) / size)) * head->dim_x
			   + ((int) ((locator_x - pos_x) / size)) + 1 );

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
 *  DW 95.03.16  Multi colour choice code.
 */

void
drawlegends( )
{
  register int 		i;
  register int		j = (activity_display_depth == DISP_GREY ?
			     GREY_INDEX : 0);
  register int		k = (conduct_display_depth == DISP_GREY ?
			     GREY_INDEX : 0);
  register float	vertex_x;
  char			number[10];
  int			viewport[4];



  /*
   *  DW 95.03.10  glPushAttrib( GL_VIEWPORT_BIT ) does not work:
   *    workaround = push and pop viewport ourself.
   */

  glGetIntegerv( GL_VIEWPORT, &viewport[0] );

  glPushAttrib( GL_VIEWPORT_BIT | GL_TRANSFORM_BIT | GL_SCISSOR_BIT );

  glViewport( 0, screen_height - 80, screen_width, 80 );
  glScissor( 0, screen_height - 60, screen_width, 60 );
  glMatrixMode( GL_PROJECTION );
  glPushMatrix( );
  glLoadIdentity();
  glOrtho( -100.0, 100.0, 0.0, 100.0, -100.0, 100.0 );
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix( );
  glLoadIdentity();

  glClear( GL_COLOR_BUFFER_BIT );

  for (i = 0; i < NUM_COLOURS; i++) {
    vertex_x = (LEGEND_BAR_WIDTH * (i+1) + LEGEND_POS_X);
    glColor3fv( rgb_index[i + ACT_INDEX + j] );
    glRectf( vertex_x, 70.0,
	    vertex_x + LEGEND_BAR_WIDTH, 70.0 + LEGEND_BAR_HEIGHT );
    glColor3fv( rgb_index[i + COND_INDEX + k] );
    glRectf( vertex_x, 40.0,
	    vertex_x + LEGEND_BAR_WIDTH, 40.0 + LEGEND_BAR_HEIGHT );
  }

  /*
   *  Text:
   *  Write out Minimum and Maximum Range values on either end of the legends
   */

  printString( -95.0, 70.0, "ACTIVITY:      " );
  printString( -95.0, 40.0, "CONDUCTIVITY:  " );
  sprintf( number, "%7.3f", activity_display_range_min );
  printString( -40.0, 70.0, number );
  sprintf( number, "%7.3f", conduct_display_range_min );
  printString( -40.0, 40.0, number );
  sprintf( number, "%7.3f", activity_display_range_max );
  printString( 75.0, 70.0, number );
  sprintf( number, "%7.3f", conduct_display_range_max );
  printString( 75.0, 40.0, number );

  /*
   *  Divider bar.
   */

  glColor3fv( rgb_index[WHITE] );
  glRectf( -100.0, 28.0, 100.0, 30.0 );

  glPopMatrix();
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopAttrib();

  glViewport( viewport[0], viewport[1], viewport[2], viewport[3] );
}  /* drawlegends( ); */



/*
 *  DW 95.03.15
 */

void
resetView( )
{
  double	scale;


  legend_x = 0.0;
  legend_y = 0.0;
  legend_z = LOOKAT_PZ;
  scale = legend_z / LOOKAT_PZ;

  glMatrixMode( GL_PROJECTION );
  glLoadIdentity( );
  glOrtho( (VIEW_LEFT - legend_x) / scale, (VIEW_RIGHT - legend_x) / scale,
	  (VIEW_BOTTOM - legend_y) / scale, (VIEW_TOP - legend_y) / scale,
	  -100.0, 100.0 );
  glMatrixMode( GL_MODELVIEW );
}  /* resetView( ); */



void
updateNetPositions( )
{
  NETWORK	tempNet = network_head;
  double	scale = legend_z / LOOKAT_PZ;


  while (tempNet) {
    tempNet->pos_x = (tempNet->pos_x * scale + legend_x);
    tempNet->pos_y = (tempNet->pos_y * scale + legend_y);
    tempNet->name_size_w *= scale;
    tempNet->name_size_h *= scale;
    tempNet->cell_size *= scale;
    tempNet = tempNet->next;
  }
  resetView( );
  redrawGraphics( );

}  /* updateNetPositions( ); */



#ifdef DEBUG
void
test_print( xpos, ypos, string )
    double xpos, ypos;
    char *string;
{
  printString( xpos, ypos, string );
}
#endif /* DEBUG */


/*
 *  'number_networks()' replaced by the global variable in nexus_lex_build.l
 *    (and thus nexus_lex_build.c).  DW 94.07.13
 */


#if 0

/*
 *    DW 94.06.15 :  Take unused code out of compilation.
 */

/* 
 *  Used to display all connections in the network.  Current
 *  display matrix is the head of the network (head->pos_x, head->pos_y, 0.0).
 *  DW 94.07.14
 *
 *    These routines are not currently used.
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

  glPushMatrix();
  glPushMatrix();
  
  for( id = 1;id <= head->number_cells;id++ ) {
    cell_size = head->cell_size;
    cell_color = BLACK;

    connection = cell->connect_list;
    current_network = get_network_id(cell->net_id);
    
    glColor3fv( rgb_index[cell_color] );
    glRectf( 0.0,  0.0,  cell_size,  cell_size ); 
    
    for (number_connections = 0;
	 number_connections < cell->number_connections;
	 number_connections++) {

      glColor3fv( rgb_index[get_color( connection->conductance,
				       CONNECTIONS )] );
      connected_network = get_network_id( connection->input_cell->net_id );
      
      x_offset = ( (connection->input_cell->id - 1) % connected_network->dim_x
		  - (cell->id - 1) % current_network->dim_x );

      y_offset = ( (connection->input_cell->id - 1) / connected_network->dim_x
		  - (cell->id - 1) / current_network->dim_x );

      /*
       *  Insert something about DRAWING the actual CONNECTION...
       *    DW 94.07.14
       */

      connection++;
    }
    
    cell++;
    if (id > 1)
      if( ! ((id - 1) % head->dim_x) ) {
	glPopMatrix();
	glTranslatef( 0.0,  cell_size,  0.0 );
	glPushMatrix();
      }
      else
	glTranslatef( cell_size,  0.0,  0.0 );
  }

  draw_name( head );
  
  glPopMatrix();
  glPopMatrix();

  GlxwinUpdate( 0, 0, screen_width, screen_height );
}



/*
 *  Display all connections in simulation as color lines between points
 *
 *    This routine is not currently used.  See show_all_connections().
 */

void
all_connections_graphics()
{
  NETWORK head;

  
  clear_screen( );

  head = network_head;
  while( head ) {
    glTranslatef( head->pos_x,  head->pos_y, (float) 0.0 );
    graph_connections( head );
    glTranslatef( -(head->pos_x),  -(head->pos_y), (float) 0.0 );
    head = head->next;
  }
}

#endif /* 0 */








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
