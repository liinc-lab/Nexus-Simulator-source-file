#ifndef NEXUS_GRAPHICS_H
#define NEXUS_GRAPHICS_H

/**************************************************************
 *                                                            *
 *                            NEXUS                           *
 *                                                            *
 *              (c) 1990 Paul Sajda and Leif Finkel           *
 *                                                            *
 *                     (nexus_graphics.h)                     *
 *        DW 94.08.31, only contains data pertinent to        *
 *                   ALL graphics libraries.                  *
 *                                                            *
 **************************************************************/

#include "nexus.h"

/*
 *  The following are updated by the User Interface.
 *  (Graphical UI's: XView, Tk)
 *  (Scripting UI: Tcl)
 */

#ifdef NEXUS_SGI
#include "nexus_opengl.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "tk.h"
#include "nexus_tk.h"

#define SIZE_FACTOR		1.0 /* cell size multiplier */
#define OUTLINE_WIDTH		0.5
#define BLACK			0
#define WHITE			1

#endif

#ifdef NEXUS_LINUX
#include "nexus_opengl.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "tk.h"
#include "nexus_tk.h"

#define SIZE_FACTOR		1.0 /* cell size multiplier */
#define OUTLINE_WIDTH		0.5
#define BLACK			0
#define WHITE			1

#endif

#ifdef NEXUS_SUN
#include <nexus_vogle.h>
#include <X11/Xlib.h>

#define SIZE_FACTOR		4.0 /* cell size multiplier */
#define OUTLINE_WIDTH		0.5

#endif

#define MID_INDEX	(NUM_COLOURS / 2)
#define COLOR_ACT_SHOW	BLACK
#define COLOR_COND_BG	BLACK
#define COLOR_COND_SET	WHITE
#define COLOR_VIEW_BG	BLACK
#define COLOR_BORDER	WHITE
#define COLOR_TEXT	WHITE

#define NETTEXT_WIDTH	0.66	/*  Multiplier of size of text (to print */
#define NETTEXT_HEIGHT	10.0	/*    network name in graphics window). */

/*
 *  Used in nexus_lex_build.l: if no x pos is given, use sane value.
 *  DW 94.07.13
 */

#define DEFAULT_X_GUTTER  30.0


#ifdef NEXUS_SGI
#define DEFAULT_X_POSITION	-80.0

#else /* NEXUS_SUN */
#define DEFAULT_X_POSITION	0.0

#endif

/*
 *  Locator Support Section, DW 94.07.31
 */

extern float		locator_x, locator_y;
extern int		locator_button;

struct location {
  int cell_id;
  NETWORK network;
};
typedef struct location ELEMENT_LOCATION;
typedef ELEMENT_LOCATION *LOCATION;

#define LOCATIONS_EQUAL(x,y) \
	((x)->cell_id == (y)->cell_id && \
	 (x)->network == (y)->network)
#define LOCATIONS_COPY(x,y) \
	{ \
		(x)->cell_id = (y)->cell_id; \
		(x)->network = (y)->network; \
	}


/*
 *  Plot Window Section, DW 94.07.14
 */

#define CUBE_SIZE	0.2

#define RECT		9
#define CUBE		10

#define AXES		0
#define FIRING_RATE	1
#define PRINT_PS	2
#define INITIALIZE	3


/*
 *  function declarations (common to all graphics libraries)
 */

extern void	init_graphics _ANSI_ARGS_(( void ));
extern void	intro_graphics _ANSI_ARGS_(( void ));
extern int	cleanup_graphics _ANSI_ARGS_(( void ));
extern void	redrawGraphics _ANSI_ARGS_(( void ));
extern void	printString _ANSI_ARGS_(( double, double, char * ));
extern void	draw_name _ANSI_ARGS_(( NETWORK ));
extern void	display_cell_activity _ANSI_ARGS_(( NETWORK, int ));
extern void	graph_activity _ANSI_ARGS_(( void ));
extern void	graph_network _ANSI_ARGS_(( NETWORK ));
extern void	clear_screen _ANSI_ARGS_(( void ));
extern void	draw_outline _ANSI_ARGS_(( int ));
extern NETWORK	select_network _ANSI_ARGS_(( void ));
extern NETWORK	move_network _ANSI_ARGS_(( NETWORK ));
extern void	translate_network _ANSI_ARGS_(( char, float, int ));
extern int	cell_select_mouse _ANSI_ARGS_(( LOCATION ));
extern void	cell_select_hilite _ANSI_ARGS_(( LOCATION, int, int, float ));
extern void	do_connectivity _ANSI_ARGS_(( int, int, int ));
extern int	select_connections_hilite _ANSI_ARGS_(( LOCATION, int, int,
						       int, int ));
extern int	get_color _ANSI_ARGS_(( float, int ));
extern void	drawlegends _ANSI_ARGS_(( void ));
extern void	resetView _ANSI_ARGS_(( void ));
extern void	updateNetPositions _ANSI_ARGS_(( void ));

extern void	modify_activity _ANSI_ARGS_((float activity,
					     int type, int flag ));


#ifdef DEBUG
extern void	test_print _ANSI_ARGS_(( double, double, char * ));
#endif /* DEBUG */

#if 0
extern void	all_connections_graphics _ANSI_ARGS_(( void ));
#endif /* 0 */

/*
 *  DW 95.04.02  Translate common graphics functions into generic code, and
 *    place in nexus_graphics.c.
 */

#if defined(NEXUS_SGI) || defined(NEXUS_LINUX)

extern float		rgb_index[][3];
extern GLint		screen_width;
extern GLint		screen_height;

#define DrawRect(x1,y1,x2,y2)	glRectf((x1),(y1),(x2),(y2))
#define SetColor(colour)	glColor3fv(rgb_index[(colour)])
#define Translate(xpos,ypos)	glTranslatef((xpos),(ypos),(float)0.0)

#else /* NEXUS_SUN */

#define DrawRect(x1,y1,x2,y2) \
	makepoly(); \
	rect((x1),(y1),(x2),(y2)); \
	closepoly()
#define SetColor(colour)	color(colour)
#define Translate(xpos,ypos)	translate((xpos),(ypos),0.0)

#endif /* NEXUS_SGI */

#endif /* NEXUS_GRAPHICS_H */



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
