/**************************************************************
 *                                                            *
 *                            NEXUS                           *
 *                                                            *
 *              (c) 1990 Paul Sajda and Leif Finkel           *
 *                                                            *
 *                      (nexus_vogle.h)                       *
 *                                                            *
 **************************************************************/

#include <vogle.h>



/* location of SIM in graphics window: full width, bottom 17/20th of screen */
#define VIEW_RIGHT     		1.0
#define VIEW_LEFT     		-1.0
#define VIEW_BOTTOM   		-1.0
#define VIEW_TOP      		0.7

#define V_WIN_LEFT 		-120.0
#define V_WIN_RIGHT 		120.0
#define V_WIN_TOP   		110.0
#define V_WIN_BOTTOM 		-110.0
#define V_WIN_NEAR  		800.0
#define V_WIN_FAR  		-800.0

/* location of LEGEND in graphics window: full width, top 3/20th of screen */
#define LEGEND_VR		1.0
#define LEGEND_VL		-1.0
#define LEGEND_VB		0.7
#define LEGEND_VT		1.0

#define LOOKAT_VZ		2000.0	/* viewpoint */
#define LOOKAT_PZ		0.0	/* reference point */

#define LEGEND_NAME_WIDTH   	60.0
#define LEGEND_TEXT_WIDTH	30.0
#define LEGEND_TEXT_HEIGHT   	60.0

#define LEGEND_POS_X		-144.0
#define LEGEND_ACT_POS_Y	0.0
#define LEGEND_COND_POS_Y	80.0
#define LEGEND_POS_Z		1000.0

/* id's for graphics and plot windows */
/* Shouldn't be like this!!! DW */
#define PLOT_WINDOW     0	
#define GRAPHICS_WINDOW 1	

/*
 *  Objects Section, DW 94.07.12
 */

#define OBJ_LEGEND	1000
#define OBJ_CELL	1001

/*
 * Improved colour support section, DW 94.06.23
 */

#define NUM_COLOURS	23	/*  Usable number of colours (sans WHITE or
				 *  BLACK): same number for activity
				 *  and conductivity
				 */

#define ACT_INDEX	8
#define COND_INDEX	(ACT_INDEX + NUM_COLOURS)

#define LEGEND_BAR_HEIGHT	60.0
#define LEGEND_BAR_WIDTH     	(138.0 / NUM_COLOURS)
