/**************************************************************
 *                                                            *
 *                            NEXUS                           *
 *                                                            *
 *              (c) 1990 Paul Sajda and Leif Finkel           *
 *                                                            *
 *                      (nexus_opengl.h)                      *
 *                                                            *
 **************************************************************/

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>



#define MAIN_FONT "-*-courier-medium-r-normal--17-*-*-*-m-100-iso8859-1"

/* location of SIM in graphics window: full width, bottom 17/20th of screen */
#define V_WIN_HEIGHT		750	/* Paint window, canvas package */
#define V_WIN_WIDTH		750	/* Should be resources... DW */

/*
 *  These numbers affect the calculation of normX and normY in
 *    the function windowToWorld( ).  Instead of -1 to 1, coordinates range
 *    from -100 to 100, thus we divide by 100 to find normX and normY.  DW
 */

#define VIEW_LEFT		-100.0
#define VIEW_RIGHT		100.0
#define VIEW_BOTTOM		-100.0
#define VIEW_TOP		100.0

#define LOOKAT_PZ		100.0 /* reference point */

/* location of LEGEND in graphics window: full width, top 3/20th of screen */
#define LEGEND_VR		V_WIN_WIDTH
#define LEGEND_VL		0
#define LEGEND_VB		(V_WIN_HEIGHT - 40)
#define LEGEND_VT		V_WIN_HEIGHT

#define LEGEND_POS_X		-20.0
#define LEGEND_ACT_POS_Y	40.0
#define LEGEND_COND_POS_Y	0.0

/*
 *  Display Lists Section, DW 94.10.31
 */

#define OBJ_LEGEND	1000
#define OBJ_CELL	1001

/*
 * Improved colour support section, DW 94.11.02
 */

#define NUM_COLOURS	70	/*  Usable number of colours (sans WHITE or
				 *  BLACK): same number for activity
				 *  and conductivity
				 */

#define ACT_INDEX	2
#define COND_INDEX	(ACT_INDEX + NUM_COLOURS)
#define GREY_INDEX	(NUM_COLOURS * 2)

#define LEGEND_BAR_HEIGHT	18.0
#define LEGEND_BAR_WIDTH     	(75.9 / NUM_COLOURS)



/*****************************************************************************
 *
 *  Function declarations.
 *
 *****************************************************************************/

extern GLenum	tkCreateStrokeFont	_ANSI_ARGS_(( GLuint fontBase ));
extern GLenum	tkCreateFilledFont	_ANSI_ARGS_(( GLuint fontBase ));


extern void windowToWorld _ANSI_ARGS_((float x, float y,
				       float *worldxp, float *worldyp));

extern void update_size _ANSI_ARGS_((int width, int height));


/* Emacs editing section. DW 94.07.19 */

/*
Local Variables:
mode:C
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
