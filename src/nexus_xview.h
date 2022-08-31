/**************************************************************
 *                                                            *
 *                            NEXUS                           *
 *                                                            *
 *              (c) 1990 Paul Sajda and Leif Finkel           *
 *                                                            *
 *                      (nexus_xview.h)                       *
 *                     Daniel Widyono, 940818		      *
 *                                                            *
 **************************************************************/

#include <xview/xv_error.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/openmenu.h>
#include <xview/notice.h>
#include <xview/canvas.h>
#include <xview/scrollbar.h>
#include <xview/cms.h>
#include <xview/svrimage.h>
#include <xview/icon.h>



#define XV_WIN_HEIGHT		800	/* View window, canvas package */
#define XV_WIN_WIDTH		800
#define V_WIN_HEIGHT		850	/* Paint window, canvas package */
#define V_WIN_WIDTH		1000	/* Should be resources... DW */
#define PLOT_WINDOW		0
#define GRAPHICS_WINDOW		1



#define LIST_NETWORKS		0
#define LIST_FILENAMES		1


#define KEY_MY_LIST		1
#define KEY_MY_TEXT		2
#define KEY_UPDATE_LIST		3
#define KEY_LIST_TYPE		4
#define KEY_LOAD_BUILD		10


/*****************************************************************************
 *  XV User Macro Definitions.   DW
 *****************************************************************************/

#define SET_CHBOX(which_chbox,value) \
	xv_set( (which_chbox), \
	       PANEL_VALUE, (value), \
	       NULL );

#define SUBFRAME \
		FRAME, \
		FRAME_WM_COMMAND_STRINGS, -1, NULL, \
		XV_WIDTH, 1000, \
		XV_HEIGHT, 1000

#define PANEL_TITLE \
		PANEL_MESSAGE, \
		XV_X, 10, \
		XV_Y, 10, \
		PANEL_LABEL_BOLD, TRUE

#define PANEL_TITLE_STRING \
		PANEL_LABEL_STRING

#define PANEL_NEWLINE \
		XV_X, 10, \
		PANEL_NEXT_ROW

#define SUBPANEL \
		PANEL, \
		XV_X, 0, \
		XV_WIDTH, 1000, \
		XV_HEIGHT, 1000, \
		OPENWIN_SHOW_BORDERS, TRUE

#define PANEL_NAME_TEXT(name_variable) \
		PANEL_TEXT, \
		PANEL_NOTIFY_PROC, proc_enter_name, \
		PANEL_CLIENT_DATA, (name_variable), \
		PANEL_VALUE_DISPLAY_LENGTH, NAME_DISPLAY_SIZE, \
		PANEL_VALUE_STORED_LENGTH, NAME_SIZE

#define PANEL_NETWORK_TEXT(network_variable) \
		PANEL_NAME_TEXT( network_variable ), \
		PANEL_LABEL_STRING, "Selected Network:", \
		PANEL_VALUE, "", \
		XV_KEY_DATA, KEY_UPDATE_LIST, TRUE

#define PANEL_FILENAME_TEXT(filename_variable) \
		PANEL_NAME_TEXT( filename_variable ), \
		PANEL_LABEL_STRING, "Filename:", \
		PANEL_VALUE, "", \
		XV_KEY_DATA, KEY_UPDATE_LIST, TRUE

#define PANEL_NETWORK_LIST(associated_text,network_variable) \
		PANEL_LIST, \
		PANEL_CHOOSE_ONE, TRUE, \
		PANEL_LIST_WIDTH, 300, \
		PANEL_LIST_DISPLAY_ROWS, 3, \
		PANEL_LIST_INSERT_DUPLICATE, FALSE, \
		PANEL_LIST_TITLE, "Available Networks:", \
		PANEL_LIST_STRINGS, "", NULL, \
		PANEL_LIST_SELECT, 0, FALSE, \
		PANEL_NOTIFY_PROC, proc_selection, \
		PANEL_CLIENT_DATA, (network_variable), \
		XV_KEY_DATA, KEY_MY_TEXT, (associated_text)

#define PANEL_FILENAME_LIST(associated_text,filename_variable) \
		PANEL_LIST, \
		PANEL_CHOOSE_ONE, TRUE, \
		PANEL_LIST_WIDTH, 300, \
		PANEL_LIST_DISPLAY_ROWS, 3, \
		PANEL_LIST_INSERT_DUPLICATE, FALSE, \
		PANEL_LIST_TITLE, "Available Files:", \
		PANEL_LIST_STRINGS, "", NULL, \
		PANEL_LIST_SELECT, 0, FALSE, \
		PANEL_NOTIFY_PROC, proc_selection, \
		PANEL_CLIENT_DATA, (filename_variable), \
		XV_KEY_DATA, KEY_MY_TEXT, (associated_text)

#define PANEL_PERCENTAGE_SLIDER(slider_variable) \
		PANEL_SLIDER, \
		PANEL_VALUE, (slider_variable), \
		PANEL_MIN_VALUE, 0, \
		PANEL_MAX_VALUE, 100, \
		PANEL_SLIDER_WIDTH, 100, \
		PANEL_TICKS, 5, \
		PANEL_NOTIFY_PROC, proc_percentage_slider, \
		PANEL_NOTIFY_LEVEL, PANEL_DONE, \
		PANEL_CLIENT_DATA, (&slider_variable)

#define get_rect(object)	((Rect *) xv_get( (object), XV_RECT ))
#define get_rect_left(object)	((get_rect(object))->r_left)
#define get_rect_top(object)	((get_rect(object))->r_top)
#define get_rect_width(object)	((get_rect(object))->r_width)
#define get_rect_height(object)	((get_rect(object))->r_height)
#define get_rect_right(object)	(rect_right(get_rect(object)))
#define get_rect_bottom(object)	(rect_bottom(get_rect(object)))
#define get_center(object)	((int) ((width - get_rect_width(object)) / 2))

#define disable_panel_item(item) \
	xv_set( (item), \
		PANEL_INACTIVE, TRUE, \
		NULL )

#define enable_panel_item(item) \
	xv_set( (item), \
		PANEL_INACTIVE, FALSE, \
		NULL )

/*  Changed from redundant loop code to this #define, DW 94.08.29 */
#define delete_list_entries(item) \
	xv_set( (item), \
		PANEL_LIST_DELETE_ROWS, \
			0, (int) xv_get( (item), PANEL_LIST_NROWS ), \
		NULL )










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
