/* 
 * nexus_tcl.c --
 *
 *	This is the main file of the TK based Nexus. It initializes the
 *	Nexus core.
 *
 */

#include "nexus.h"
#include "graphics.h"
#include "tcl_main.h"
#include "nexus_opengl.h"
#include "graphics.h"

#ifndef NEXUS_LIBRARY
#define NEXUS_LIBRARY "/usr/local/lib/nexus"
#endif



/***********
 *  Mouse support
 ***********/

float		locator_x = 0.0, locator_y = 0.0;
int		locator_button = 0;


/***********
 *  Global Variables
 ***********/

Display		*dpy;
Tcl_Interp	*nxInterp;
extern NETWORK	network_head;
extern char	curr_access_directory[];
extern int	graphics_off;
extern int	backing_store;



/***********************************************************************
 * 			Local functions
 *
 */

extern int	nexus_init( );
static void	TkNx_InitCommands(Tcl_Interp *interp);
extern void	TkNx_InitTraceVars(Tcl_Interp *interp);
static int	TkNx_InitTclLibrary(Tcl_Interp *interp);


/***********************************************************************
 * TkNx_Init()
 *
 * Initialization:
 *
 *	(1) Initialize the Nexus Core.
 *	(2) Create TCL commands to communicate with the TK Interface.
 *
 * Note: By the time this function is called, the OpenGL window is not
 * opened yet. Don't attempt to draw using OpenGL. Only initialize the
 * data structure here. All drawing actions will be invoked by the
 * User Interface.
 */

int
TkNx_Init(Tcl_Interp *interp, Tk_Window  topLevel)
{
  /* Initialize the global variables used in the TCL module */
  nxInterp = interp;

  dpy = Tk_Display( topLevel );

  /* Call initialization routines */
  nexus_init( );
  TkNx_InitCommands( interp );
  TkNx_InitTraceVars( interp );

  return TkNx_InitTclLibrary(interp);
}



/*  TkNx_InitCommands -
 *
 *    Initialize the TCL commands that are used to communicate with the GUI.
 */

static void
TkNx_InitCommands( Tcl_Interp *interp )
{
  /***********
   *  nexus_tcl.c
   ***********/

  Tcl_CmdProc TkNx_Cls;
  Tcl_CmdProc TkNx_MouseEvent;
  Tcl_CmdProc TkNx_RedrawGraphWin;
  Tcl_CmdProc TkNx_RedrawLegend;
  Tcl_CmdProc TkNx_ConfigureGraphWin;
  Tcl_CmdProc TkNx_Memory;
  Tcl_CmdProc TkNx_SetCAD;
  Tcl_CmdProc TkNx_Cleanup;
  Tcl_CmdProc TkNx_SetBackingStore;

  /***********
   *  nexus_tcl_sim.c
   ***********/

  Tcl_CmdProc TkNx_ReadFile;
  Tcl_CmdProc TkNx_DoSimulate;
  Tcl_CmdProc TkNx_SaveSimulation;

  /***********
   *  nexus_tcl_mod.c
   ***********/

  Tcl_CmdProc TkNx_LoadCells;
  Tcl_CmdProc TkNx_ChangeParam;
  Tcl_CmdProc TkNx_ShowParam;
  Tcl_CmdProc TkNx_GetNetworkNames;
  Tcl_CmdProc TkNx_LoadActivity;
  Tcl_CmdProc TkNx_LoadBatchFile;
  Tcl_CmdProc TkNx_GetConnections;
  Tcl_CmdProc TkNx_GetHebbConnections;
  Tcl_CmdProc TkNx_SetHebbConnection;

  /***********
   *  nexus_tcl_elec.c
   ***********/

  Tcl_CmdProc TkNx_MoveNets;
  Tcl_CmdProc TkNx_WriteActivityFile;
  Tcl_CmdProc TkNx_ChangeDimension;
  Tcl_CmdProc TkNx_ChangeConnection;
  Tcl_CmdProc TkNx_ResetView;

  /***********
   *  Learning methods
   ***********/
  Tcl_CmdProc TkNx_RBFAllocParam;
  Tcl_CmdProc TkNx_BpAllocParam;
  Tcl_CmdProc TkNx_InvAllocParam;

  /***********
   *  nexus_tcl_utils.c
   ***********/
  Tcl_CmdProc TkNx_RaiseWindow;

  Tcl_CmdProc TkNx_Debug;

#ifdef DEBUG
  Tcl_CmdProc TkNx_Print;
  Tcl_CmdProc TkNx_ListNetworks;
  Tcl_CmdProc TkNx_ShowNetwork;
#endif /* DEBUG */


  CMD_CREATE3( interp, "TkNx_Cls",		TkNx_Cls );
  CMD_CREATE3( interp, "TkNx_Cleanup",		TkNx_Cleanup );
  CMD_CREATE3( interp, "TkNx_MouseEvent",	TkNx_MouseEvent );
  CMD_CREATE3( interp, "TkNx_RedrawGraphWin",	TkNx_RedrawGraphWin );
  CMD_CREATE3( interp, "TkNx_RedrawLegend",	TkNx_RedrawLegend );
  CMD_CREATE3( interp, "TkNx_ConfigureGraphWin",TkNx_ConfigureGraphWin );
  CMD_CREATE3( interp, "TkNx_SetBackingStore",  TkNx_SetBackingStore );
  CMD_CREATE3( interp, "TkNx_ReadFile",		TkNx_ReadFile );
  CMD_CREATE3( interp, "TkNx_DoSimulate",	TkNx_DoSimulate );
  CMD_CREATE3( interp, "TkNx_SaveSimulation",	TkNx_SaveSimulation );
  CMD_CREATE3( interp, "TkNx_LoadCells",	TkNx_LoadCells );
  CMD_CREATE3( interp, "TkNx_ChangeParam",	TkNx_ChangeParam );
  CMD_CREATE3( interp, "TkNx_ShowParam",	TkNx_ShowParam );
  CMD_CREATE3( interp, "TkNx_GetNetworkNames",  TkNx_GetNetworkNames );
  CMD_CREATE3( interp, "TkNx_MoveNets",		TkNx_MoveNets );
  CMD_CREATE3( interp, "TkNx_WriteActivityFile",TkNx_WriteActivityFile );

  CMD_CREATE3( interp, "TkNx_LoadActivity",   	TkNx_LoadActivity );
  CMD_CREATE3( interp, "TkNx_LoadBatchFile",   	TkNx_LoadBatchFile );
  CMD_CREATE3( interp, "TkNx_GetConnections",	TkNx_GetConnections );
  CMD_CREATE3( interp, "TkNx_GetHebbConnections",TkNx_GetHebbConnections );
  CMD_CREATE3( interp, "TkNx_SetHebbConnection", TkNx_SetHebbConnection );
  CMD_CREATE3( interp, "TkNx_ChangeDimension",	TkNx_ChangeDimension );
  CMD_CREATE3( interp, "TkNx_ChangeConnection",	TkNx_ChangeConnection );
  CMD_CREATE3( interp, "TkNx_ResetView",	TkNx_ResetView );
  CMD_CREATE3( interp, "TkNx_Memory", 		TkNx_Memory );
  CMD_CREATE3( interp, "TkNx_SetCAD",		TkNx_SetCAD );

  CMD_CREATE3( interp, "TkNx_RBFAllocParam",	TkNx_RBFAllocParam );
  CMD_CREATE3( interp, "TkNx_BpAllocParam",	TkNx_BpAllocParam );
  CMD_CREATE3( interp, "TkNx_InvAllocParam",	TkNx_InvAllocParam );
  CMD_CREATE3( interp, "TkNx_RaiseWindow",	TkNx_RaiseWindow );

  CMD_CREATE3( interp, "TkNx_Debug",		TkNx_Debug );

#ifdef DEBUG
  CMD_CREATE3( interp, "TkNx_Print",		TkNx_Print );
  CMD_CREATE3( interp, "TkNx_List",		TkNx_ListNetworks );
  CMD_CREATE3( interp, "TkNx_Show",		TkNx_ShowNetwork );
#endif /* DEBUG */
}

static int
TkNx_InitTclLibrary( Tcl_Interp *interp )
{
  static char initCmd[] =
    "if {![file exists $nexus_library/nexus.tcl] && [info exist argv0]} {\n"
    "  if {[file pathtype $argv0] != {absolute}} {\n"
    "    set com_path [file dirname [file join [pwd] $argv0]]\n"
    "  } else {\n"
    "    set com_path [file dirname $argv0]\n"
    "  }\n"
    "  set nexus_library [file join $com_path ../lib/nexus]\n"
    "}\n"
    "lappend auto_path $nexus_library \n\
        if {[file exists $nexus_library/nexus.tcl]} {\n\
	    source $nexus_library/nexus.tcl\n\
	} else {\n\
	   set msg \"Can't find $nexus_library/nexus.tcl; \\nperhaps you \"\n\
	   append msg \"need to install Nexus or set your NEXUS_LIBRARY \"\n\
	   append msg \"environment variable?\"\n\
	   error $msg\n\
	}";
  char * libDir;

  /* Set up the TCL variable "nexus_library" according to the environment
   * variable.
   */
  libDir = getenv("NEXUS_LIBRARY");
  if (libDir == NULL) {
    libDir = NEXUS_LIBRARY;
  }
  Tcl_SetVar(interp, "nexus_library", libDir, TCL_GLOBAL_ONLY);

  return Tcl_Eval(interp, initCmd);
}


/*
 * redrawGraphics --
 *
 *	Currently this function acts as the "work horse" for re-drawing
 *	the graphics window. If the rendering mode is set to "render off
 *	screen, this function doesn't make sure that the on-screen
 *	graphics window is updated. To update BOTH the backing store AND
 *	the screen, call renderGraphics().
 */
/* " */
void
redrawGraphics( )
{
  extern int		curr_electrode;



  if (graphics_off)
    return;

  if (explore_variables)
    switch (curr_electrode) {
    case MODE_ACTIVITY:
      fprintf( stderr, "Displaying activity.\n" );
      break;
    case MODE_CONNECT:
      fprintf( stderr, "Displaying connectivity.\n" );
      break;
    default:
      fprintf( stderr, "Displaying view.\n" );
      break;
    }


  if (curr_electrode == MODE_ACTIVITY) {
    clear_screen( );
    graph_activity( );
  }

  else if (curr_electrode == MODE_CONNECT) {
    draw_outline( TRUE );
  }
  else {
    draw_outline( FALSE );
  }

  glFlush();
}



/*
 *  This routine is called when the graphics window needs to be explicitly
 *    updated.
 */

int
TkNx_RedrawGraphWin( ClientData clientdata,
		     Tcl_Interp *interp,
		     int argc, char **argv )
{
  static int	init_flag = 0;
  int		width, height;


  TCL_CHK("TkNx_RedrawGraphWin", argc, 3, "widget width height");

#ifdef DEBUG
  fprintf( stderr, "TkNx_RedrawGraphWin %s %s %s\n",
	  argv[1], argv[2], argv[3] );
#endif

  if (!init_flag) {
    (void) init_graphics( );
    init_flag = 1;
    glFlush( );
    return TCL_OK;
  }

  width   = atoi(argv[2]);
  height  = atoi(argv[3]);
  update_size( width, height );

  redrawGraphics();
  glFlush( );

  return TCL_OK;
}



int
TkNx_RedrawLegend( ClientData clientdata,
		  Tcl_Interp *interp,
		  int argc, char **argv )
{
  TCL_CHK("TkNx_RedrawLegend", argc, 0, "");

  drawlegends( );
  return TCL_OK;
}


int
TkNx_ResetView( ClientData clientdata,
	       Tcl_Interp *interp,
	       int argc, char **argv )
{
  TCL_CHK("TkNx_ResetView", argc, 0, "");

  resetView( );
  redrawGraphics( );
  return TCL_OK;
}



/*
 *  This routine is called when the window is resized.
 */

int
TkNx_ConfigureGraphWin( ClientData clientdata,
		     Tcl_Interp *interp,
		     int argc, char **argv )
{
  int		width, height;


  TCL_CHK( "TkNx_ConfigureGraphWin", argc, 3, "widget width height" );

#ifdef DEBUG
  fprintf( stderr, "TkNx_ConfigureGraphWin %s %s %s\n",
	  argv[1], argv[2], argv[3] );
#endif

  width   = atoi(argv[2]);
  height  = atoi(argv[3]);
  update_size( width, height );

  return TCL_OK;
}



int
TkNx_SetBackingStore( ClientData clientdata,
		     Tcl_Interp *interp,
		     int argc, char **argv )
{
  TCL_CHK( "TkNx_SetBackingStore", argc, 1, "state" );

  backing_store = atoi( argv[1] );

#ifdef DEBUG
  fprintf( stderr, "Set backing_store to %d.\n", backing_store );
#endif

  return TCL_OK;
}



int
TkNx_Cls( ClientData clientdata,
	 Tcl_Interp *interp,
	 int argc, char **argv )
{
  clear_screen( );
  return TCL_OK;
}



int
TkNx_Cleanup( ClientData clientdata,
	     Tcl_Interp *interp,
	     int argc, char **argv )
{
  cleanup( );
  Tcl_Eval( interp, "exit" );

  /*  We never reach this point; thus we never return from this routine.  */
  return TCL_OK;
}




int
TkNx_Debug( ClientData clientdata,
	   Tcl_Interp *interp,
	   int argc, char **argv )
{
/*  FILE *fp; */

/*  if (fp = fopen( "/tmp/duh", "a" )) { */

#ifdef DEBUG
    if (argc == 1)
      fprintf( stderr, "TkNx_Debug.\n" );
    else
      fprintf( stderr, "TkNx_Debug( %s ).\n", argv[1] );
#endif


/*    fclose( fp ); */
/*   }              */

  return TCL_OK;
}



#ifdef DEBUG

int
TkNx_Print( ClientData clientdata,
	   Tcl_Interp *interp,
	   int argc, char **argv )
{
  double xpos, ypos;

  TCL_CHK( "TkNx_Print", argc, 3, "xpos ypos string" );

  if (Tcl_GetDouble( interp, argv[1], &xpos ) != TCL_OK)
    return TCL_ERROR;
  if (Tcl_GetDouble( interp, argv[2], &ypos ) != TCL_OK)
    return TCL_ERROR;
  (void) test_print( xpos, ypos, argv[3] );

  return TCL_OK;
}



int
TkNx_ListNetworks( ClientData clientdata,
		  Tcl_Interp *interp,
		  int argc, char **argv )
{
  extern NETWORK *network_xref_list;
  extern int	number_networks;	/* from nexus_lex_build.l */
  int		i;



  TCL_CHK( "TkNx_List", argc, 0, "" );

  if (!network_head)
    printf( "No networks loaded.\n" );
  else
    for (i = 1; i <= number_networks; i++)
      printf( "ID: %d\tName: %s.\n", i,
	     (*(network_xref_list + i))->name );

  return TCL_OK;
}



int
TkNx_ShowNetwork( ClientData clientdata,
		 Tcl_Interp *interp,
		 int argc, char **argv )
{
  extern NETWORK	*network_xref_list;
  extern int		number_networks;	/* from nexus_lex_build.l */
  NETWORK		network;
  int			i;

  extern NETWORK	get_network_name( );



  TCL_CHK( "TkNx_Show", argc, 1, "network" );

  if (!network_head) {
    printf( "No networks loaded.\n" );
    return TCL_OK;
  }
  else if (!(network = get_network_name( argv[1] ))) {
    fprintf( stderr, "%s is not one of the following:\n", argv[1] );
    for (i = 1; i <= number_networks; i++)
      printf( "ID: %d\tName: %s.\n", i,
	     (*(network_xref_list + i))->name );
    return TCL_OK;
  }

  showParameters( network );

  if (network->next)
    fprintf( stderr, "Next network:\t%s\n", network->next->name );
  else
    fprintf( stderr, "Last network.\n" );
  return TCL_OK;
}
#endif /* DEBUG */



/*****************************************************************************
 *  EVENT PROCEDURE
 *****************************************************************************/

/*
 *  Allow the mouse to interact with OpenGL.  Events come from Tcl.
 *  The state of the electrode (mouse) determines if connections
 *    are displayed or cells can be stimulated .
 *
 *  Type is (B)utton, (R)elease, or (M)ovement (Dragging).
 *  Buttonmask is defined in nexus.h.
 */

int
TkNx_MouseEvent( ClientData clientdata,
		Tcl_Interp *interp,
		int argc, char **argv )
{
  extern int		curr_electrode;
  extern int		setActivityPercentage;
  extern float		activity_display_range_max,
  			activity_display_range_min;

  extern NETWORK	select_network( );
  extern NETWORK	move_network( );

  char			type;
  int			winx, winy;

  /*
   *  State variable replaces "net" in nexus_main.c: is a network selected?
   *    Standard Boolean flag.  DW 94.07.12
   *  Changed from 'int' flag to 'NETWORK' pointer, DW 94.08.01
   *    If no, it is NULL; if yes, it points to the selected network.
   */
  static NETWORK	net_selected = NULL;



  TCL_CHK( "TkNx_MouseEvent", argc, 4, "type x y buttonmask" );

  type = argv[1][0];
  winx = atoi( argv[2] );
  winy = atoi( argv[3] );
  locator_button = atoi( argv[4] );

  windowToWorld( (float) winx, (float) winy, &locator_x, &locator_y );

  if (curr_electrode == MODE_OFF || type == 'R')
    return TCL_OK;

  /*
   *  Don't accept MotionNotify with a button pressed, in
   *    these circumstances:				DW 94.08.17
   */

  if (type == 'M' && curr_electrode == MODE_VIEW)

    /*      (curr_electrode == MODE_CONNECT
	    || curr_electrode == MODE_VIEW))
	    || (curr_electrode == MODE_ACTIVITY
	    && query_flag( ACTIVITY_STATE ) == EXAMINE))) */

    return TCL_OK;

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
      fprintf( stderr,
	      "Error: Anterograde option is not currently implemented.\n" );
      fprintf( stderr, "Resetting to Retrograde.\n" );
      set_flag( CONNECT_SHOW_TYPE, RETROGRADE );
    }

    do_connectivity( query_flag( CONNECT_SHOW_TYPE ),
		    query_flag( CONNECT_PRINT ),
		    query_flag( CONNECT_SAVE ) );

    return TCL_OK;
  }
  
  /* Display Activity */
  
  if (curr_electrode == MODE_ACTIVITY) {

    if (query_flag( ACTIVITY_STATE ) == CHANGE) {
      if (locator_button == LEFT_BUTTON)
	modify_activity( setActivityPercentage / 100.0
			* activity_display_range_max,
			ACTIVITY,
			SET );
      else if (locator_button == MIDDLE_BUTTON)
	modify_activity( activity_display_range_max,
			ACTIVITY,
			SET );
      else if (locator_button == RIGHT_BUTTON)
	modify_activity( activity_display_range_min,
			ACTIVITY,
			SET );
    }
    else
      modify_activity( 0.0,
		      ACTIVITY,
		      SHOW );

    return TCL_OK;
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

    return TCL_OK;
  }

  return TCL_OK;
}



void
update_network_lists( )
{
  Tcl_Eval(nxInterp, "nxReloadNetworkNames");
}



/* TkNx_RaiseWindow --
 *
 */
COMMAND_PROC(TkNx_RaiseWindow)
{
    Tk_Window tkwin;

    TCL_CHK( "TkNx_RaiseWindow", argc, 1, "pathName");

    if (!(tkwin=Tk_NameToWindow(interp, argv[1], Tk_MainWindow(interp)))) {
	return TCL_ERROR;
    }

    XRaiseWindow(Tk_Display(tkwin), Tk_WindowId(tkwin));
    return TCL_OK;
}


/*----------------------------------------------------------------------
 *
 *		The "Memory" button in the main window
 *
 *----------------------------------------------------------------------
 */
COMMAND_PROC(TkNx_Memory)
{
  do_memory( );

  return TCL_OK;
}



COMMAND_PROC(TkNx_SetCAD)
{
  TCL_CHK( "TkNx_SetCAD", argc, 1, "directory" );

  strcpy( curr_access_directory, argv[1] );
  strcat( curr_access_directory, "/" );
  return TCL_OK;
}



/*
 *  Returns TRUE if user answers YES to the query.  DW 94.11.09
 */

int
queryDialog( char *string )
{
  char buff[1024];


  sprintf( buff, "nxQueryYesNo {%s}", string );
  if (Tcl_Eval( nxInterp, buff ) == TCL_OK) {
    if (strcmp( nxInterp->result, "1" ) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}



void
errorDialog( char *string )
{
  char buff[1024];

  sprintf( buff, "nxErrorMsg {%s}", string );
  Tcl_Eval( nxInterp, buff );
}



/*
 * renderGraphics --
 *
 *	Force to redraw everything in the graphics window. This will update
 *	the data stored in the backing store if we are running in a "indirect
 *	graphics mode".
 *
 */
void
renderGraphics( )
{
  if (Tcl_Eval(nxInterp, "nxGraphRedraw $nexus(glx)") != TCL_OK) {
    Tcl_AddErrorInfo(nxInterp,
	"\n    (command executed by the C function renderGraphics())");
    Tk_BackgroundError(nxInterp);
  }
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
