/*
 * nexus_tcl_mod.c
 *
 * 	Dialogs in the "Modification Control" set
 *      Dialogs in the "Simulation" set
 *      Dialogs in the "Electrode" set
 */

#include "nexus.h"
#include "batch.h"
#include "nexus_tk.h"

#include "tcl_def.h"
#include "extern_functions.h"
#include "input_output.h"
#include "connect.h"
#include "main.h"
#include "run.h"

#include "graphics.h"

extern flag_t nexus_flags; /* used in the SET_FLAG macro. */

extern int stop_simulation;

int view_elect     = 0;
int connect_elect  = 0;
int activity_elect = 0;


extern void set_electrode _ANSI_ARGS_(( int ));


/*****************************************************************************
 *		MODULE INITIALIZATION
 *****************************************************************************/

void
TkNx_ElecInit( )
{
  view_elect		= (curr_electrode == MODE_VIEW);
  connect_elect		= (curr_electrode == MODE_CONNECT);
  activity_elect	= (curr_electrode == MODE_ACTIVITY);
}

void
TkNx_ModInit()
{
}

void
TkNx_SimInit()
{}

/*----------------------------------------------------------------------
 *
 *		RANDOMIZE ACTIVITY dialog
 *
 *----------------------------------------------------------------------*/

float random_cell_min_firing = DEFAULT_ACTIVITY_MIN;
float random_cell_max_firing = DEFAULT_ACTIVITY_MAX;

/*
 * TkNx_LoadCells --
 */
COMMAND_PROC(TkNx_LoadCells)
{
  TCL_CHK( "TkNx_LoadCells", argc, 0, "");


  set_network( network_head, random_cell_min_firing, random_cell_max_firing );
  return TCL_OK;
}



/*----------------------------------------------------------------------
 *
 *		SET PARAMETERS dialog
 *
 *----------------------------------------------------------------------
 */

char parameter_value[NAME_SIZE] = "";
int param_choice;

/*
 * TkNx_ChangeParam --
 *
 */
COMMAND_PROC(TkNx_ChangeParam)
{
  float		value;
  NETWORK	head;

  TCL_CHK( "TkNx_ChangeParam", argc, 0, "");


  head = network_head;

  if (param_choice != SET_TRANS_FUNC) {
    value = atof(parameter_value);
  }

  while (head != NULL && strcmp(head->name, parameter_network) != 0) {
    head = head->next;
  }

  if (head == NULL) {
    fprintf(stderr, "no such network %s\n.", parameter_network);
    return TCL_OK;
  }

  switch (param_choice) {

  case CLAMP_ON:
    set_clamp( head, CLAMP_ON, 0 );
    printf( "Network %s: clamped.\n", head->name );
    break;

  case CLAMP_OFF:
    set_clamp(head,
	      CLAMP_OFF,
	      0 );
    printf( "Network %s: unclamped.\n", head->name );
    break;

  case SET_ACTIVITY:
    set_clamp(head,
	      SET_ACTIVITY,
	      value );
    printf( "Network %s: cells' activity set to %f.\n",
	   head->name, value );
    if (curr_electrode == MODE_ACTIVITY)
      redrawGraphics( );
    break;

  case SET_TRANS_FUNC:
    set_trans_func(head,
		   SET_SCALE,
		   parameter_value);
    printf( "Network %s: transfer function set to %s.\n",
	   head->name, parameter_value );
    break;

  case SET_THRESHOLD:
    set_clamp(head,
	      SET_THRESHOLD,
	      value );
    printf( "Network %s: threshold set to %f.\n", head->name, value );
    break;

  case SET_SCALE:
    set_param(head,
	      SET_SCALE,
	      value );
    printf( "Network %s: scale set to %f.\n", head->name, value );
    break;

  case SET_OFFSET:
    set_param(head,
	      SET_OFFSET,
	      value );
    printf( "Network %s: offset set to %f.\n", head->name, value );
    break;

  case SET_NUM_UPDATES:
    head->evaluations_per_cycle = (int) value ;
    printf( "Network %s: # updates/cycle set to %d.\n",
	   head->name, (int) value );
    break;

  case SET_DECAY:
    set_param( head, SET_DECAY, value);
    printf( "Network %s: decay set to %f.\n", head->name, value );
    break;

  default:
    break;
  }

  return TCL_OK;
}



/*
 * TkNx_ShowParam --
 *
 */
COMMAND_PROC(TkNx_ShowParam)
{
    NETWORK	head;


    TCL_CHK( "TkNx_ShowParam", argc, 0, "");

    /* Find the selected network */
    head = network_head;
    while (head && strcmp(head->name, parameter_network)) {
	head = head->next;
    }
  
    if (head)
      showParameters( head );
    else
      printf( "Error: network %s does not exist.\n", parameter_network );

    return TCL_OK;
}

/*
 * TkNx_GetNetworkNames
 *
 * argv[1] = pathname of the listbox to fill in
 */
COMMAND_PROC(TkNx_GetNetworkNames)
{
    NETWORK ptr;
    char buff[1024];

    TCL_CHK( "TkNx_GetNetworkNames", argc, 1, "listbox_pathname");

    for (ptr = network_head; ptr; ptr=ptr->next) {
	sprintf(buff, "%s insert end {%s}", argv[1], ptr->name);
	if (Tcl_Eval(interp, buff)!= TCL_OK) {
	    return TCL_ERROR;
	}
    }

    return TCL_OK;
}

/*----------------------------------------------------------------------
 *
 *		LOAD_MAP Dialog
 *
 *----------------------------------------------------------------------
 */

/*----------------------------------------------------------------------
 * TkNx_LoadBatchFile --
 *
 * argv[1] = batch filename
 * argv[2] = list to fill in activity names
 */
COMMAND_PROC(TkNx_LoadBatchFile)
{
    char buff[1024];
    int  i;

    TCL_CHK("TkNx_LoadBatchFile", argc, 2, "filename listbox_pathname");

    if (readBatchFile( NXBATCH_IN, filename_load_activity )) {
	sprintf(buff, "error {No such file %s}", filename_load_activity);
	Tcl_AppendResult(interp, buff, NULL);
    }
    else {
	for (i = 0; i < currBatchListLength[NXBATCH_IN]; i++) {
	    sprintf(buff, "%s insert end {%s}", argv[2],
		    (currBatchList[NXBATCH_IN] + i)->filename);
	    if (Tcl_Eval(interp, buff) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * TkNx_LoadActivity --
 *
 */
COMMAND_PROC(TkNx_LoadActivity)
{
  char 		buff[1024];
  char      	      * name;
  FILE      	      * fp;
  extern NETWORK	network_head;
  NETWORK		head;


  TCL_CHK("TkNx_LoadActivity", argc, 0, "");

  /* Find the network */
  name = network_load_activity;
  head = network_head;
  while (head && strcmp( head->name, name )) {
    head = head->next;
  }

  if (head == NULL) {
    sprintf(buff, "error {No such network %s}", name);
    Tcl_AppendResult(interp, buff, NULL);
    return TCL_OK;
  }

  printf( "Loading file <%s> . . .\n", activity_filename );


  if ((fp = nxFopen( activity_filename )) == NULL) {
    sprintf(buff, "error {No such file %s}", activity_filename );
    Tcl_AppendResult(interp, buff, NULL);
  } else {
    load_activity_file( fp, head );
    printf( "\nDone\n" );
    fclose(fp);
  }

  return TCL_OK;
}



/*----------------------------------------------------------------------
 *
 *		Edit Connection Dialog
 *
 *----------------------------------------------------------------------
 */
char		edit_connect_network[NAME_SIZE] = "";
char		edit_connect_function[FUNCTION_SIZE] = "";
int 		edit_connect_current_id = NO_CONNECTION;


/*
 * TkNx_GetConnections
 *
 * argv[1] = pathname of the listbox to fill in
 * 
 * return value: a list of all the id
 */
COMMAND_PROC(TkNx_GetConnections)
{
  extern NETWORK	get_network_name();
  extern NETWORK	get_network_id();
  NETWORK 		netPtr;
  SPECS			conPtr;
  char			buff[1024];
  Tcl_DString		dString;
  int			code = TCL_OK;

  TCL_CHK( "TkNx_GetConnections", argc, 1, "listbox_pathname");

  if ((netPtr = get_network_name( edit_connect_network )) == 0) {
    return TCL_ERROR;
  }

  Tcl_DStringInit( &dString );

  for (conPtr = netPtr->cells->specs; conPtr; conPtr = conPtr->next) {
    char *prefix, *name;

    if (conPtr->type == RETROGRADE) {
      prefix = "From-";
    } else {
      prefix = "To-";
    }
    name = get_network_id( conPtr->id )->name;

    sprintf( buff, "%s insert end {%s%s}", argv[1], prefix, name );

    if (Tcl_Eval(interp, buff) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }

    sprintf( buff, " %d", conPtr->connect_id );

    Tcl_DStringAppend( &dString, buff, strlen(buff) ); 
  }

  Tcl_SetResult (interp, dString.string, TCL_VOLATILE);

 done:
  Tcl_DStringFree (&dString);
  return code;
}


/*
 * TkNx_ChangeConnection
 *
 * argv[1] = pathname of the listbox to fill in
 * 
 * return value: a list of all the id
 */
COMMAND_PROC(TkNx_ChangeConnection)
{
    extern char	* edit_connection();
    char	* result, buff[1024];

    TCL_CHK( "TkNx_ChangeConnection", argc, 0, "");

    result = edit_connection( edit_connect_network,
			     edit_connect_current_id,
			     edit_connect_function );

    if (result) {
	sprintf( buff, "error {in edit_connection(): %s}", result );
	Tcl_AppendResult( interp, buff, NULL );
    }
    return TCL_OK;
}



/*----------------------------------------------------------------------
 *
 *  DIALOG: "Read File" and "Build Simulation"
 *
 *----------------------------------------------------------------------
 */
/* TkNx_ReadFile --
 * 
 *	Read in a build file (.nx) or a simulation file (.saved).
 *
 * argv[1] = type
 * argv[2] = filename
 *
 */
int
TkNx_ReadFile(ClientData clientdata,
	      Tcl_Interp *interp,
	      int argc, char **argv )
{
  FILE		*fp;


  TCL_CHK( "TkNx_ReadFile", argc, 2, "type filename");

  switch(argv[1][0]) {
  case 'L':                 /* Simulation file .saved */
    load_simulation( argv[2] );
    set_electrode( MODE_ACTIVITY );
    printf( "Done.\n" );
    break;
  case 'B':                 /* Build file .nx */
    if ((fp = fopen( argv[2], "r" ))) {
      if (read_network( fp ) == FALSE) {
	sprintf( interp->result, "ERROR: No .nx file was loaded.\n" );
	break;
      }
      (void) connect_sim( );
      set_electrode( MODE_ACTIVITY );
      printf( "Done.\n" );
      break;
    }
    else {
      sprintf( interp->result, "ERROR: Can not open file '%s'.", argv[2] );
      break;
    }
  default:
    break;
  } /* switch(argv[1][0]) */

  return TCL_OK;
}



/*----------------------------------------------------------------------
 *
 *			The SIMULATE dialog
 *
 *----------------------------------------------------------------------
 */

int
check_interrupt( )
{
    Tk_Window toplevel = Tk_MainWindow(nxInterp);

    while (XPending(Tk_Display(toplevel))) {
	Tk_DoOneEvent(TK_ALL_EVENTS);
	while (Tk_DoOneEvent(TK_IDLE_EVENTS) != 0) {
	    /* Empty loop body 
	     * Flush all the idle events
	     */
	}
    }


    if (stop_simulation) {
	return NXI_STOP;
    } else {
	return NXI_IGNORE;
    }
}



int
TkNx_DoSimulate( ClientData clientdata,
		Tcl_Interp *interp,
		int argc, char **argv )
{
  TCL_CHK( "TkNx_DoSimulate", argc, 1, "{go stop}");

  if (argv[1][0] == 'g') {		/* go */
    stop_simulation = FALSE;
    init_extern_connections( network_head );
    if (run_simulation( number_cycles, random_cells, simulate_type ) == OK)
      printf( "\n**********    Simulation finished.    **********\n" );
    else
      printf( "\n**********    Simulation ABORTED.    **********\n" );
    set_electrode( MODE_ACTIVITY );
    if (display_type == DISP_OFF)
      graph_activity( );
  }
  else {				/* stop */
    stop_simulation = TRUE;
  }

  return TCL_OK;
}



/*----------------------------------------------------------------------
 *
 *  DIALOG: "Save Simulation"
 *
 *----------------------------------------------------------------------
 */

int
TkNx_SaveSimulation(ClientData clientdata,
	      Tcl_Interp *interp,
	      int argc, char **argv )
{
    TCL_CHK( "TkNx_SaveSimulation", argc, 2, "filename compressed");

    /* Set the FILE_COMPRESSED flag according to parameter */
    if (argv[2][0] == 'a') {
	SET_FLAG(FILE_COMPRESSED, 0);
    } else {
	SET_FLAG(FILE_COMPRESSED, 1);
    }

    save_simulation( network_head, argv[1] );

    return TCL_OK;
}



void
set_electrode( int new_elect )
{
  Tcl_SetVar(nxInterp, "view.electrode",
	     (new_elect == MODE_VIEW ? "on": "off"),
	     TCL_GLOBAL_ONLY);

  Tcl_SetVar(nxInterp, "activity.electrode",
	     (new_elect == MODE_ACTIVITY ? "on": "off"),
	     TCL_GLOBAL_ONLY);

  Tcl_SetVar(nxInterp, "connect.electrode",
	     (new_elect == MODE_CONNECT ? "on": "off"),
	     TCL_GLOBAL_ONLY);

  curr_electrode = new_elect;
}



void
SetElectrode( int * which, int value )
{
  static int entered = 0;

  if (! entered) {
    /*
     *  When we set the TCL variables, this function is recursed. Don't
     *    want to enter infinite recursion.
     */
    entered = 1;

    if (which == &view_elect) {

      if (value == ON && curr_electrode != MODE_VIEW) {
	set_electrode( MODE_VIEW );
	redrawGraphics();
      }
      else if (value == OFF && curr_electrode == MODE_VIEW) {
	curr_electrode = MODE_OFF;
      }
    }
    else if (which == &connect_elect) {

      if (value == ON && curr_electrode != MODE_CONNECT) {
	set_electrode( MODE_CONNECT );
	redrawGraphics();
      }
      else if (value == OFF && curr_electrode == MODE_CONNECT) {
	curr_electrode = MODE_OFF;
      }
    }
    else if (which == &activity_elect) {
      int old_electrode = curr_electrode;

      if (value == ON && old_electrode != MODE_ACTIVITY) {
	set_electrode( MODE_ACTIVITY );
	redrawGraphics();
      }
      else if (value == OFF && old_electrode == MODE_ACTIVITY) {
	curr_electrode = MODE_OFF;
      }
    }

    entered = 0;
  }
}


/*----------------------------------------------------------------------
 *
 *		The activity dialog
 *
 *----------------------------------------------------------------------
 */

/*
 * TkNx_WriteActivityFile --
 *
 */
COMMAND_PROC(TkNx_WriteActivityFile)
{
    FILE      * fp;
    char	buff[1024];

    TCL_CHK( "TkNx_WriteActivityFile", argc, 0, "");

    if (!(fp = fopen( activity_output_filename, "w" ))) {
	sprintf(buff, "error {Cannot write to file %s}",
		activity_output_filename);
	Tcl_AppendResult(interp, buff, NULL);
	return TCL_OK;
    }
    if (output_activity( fp ) == ERROR) {
	sprintf(buff, "error {output_activity() failed}");
	Tcl_AppendResult(interp, buff, NULL);
    }

    fclose(fp);
    return TCL_OK;
}

/*----------------------------------------------------------------------
 *
 *		The VIEW dialog
 *
 *----------------------------------------------------------------------
 */
char view_network[NAME_SIZE] = "";

int
TkNx_MoveNets( ClientData clientdata,
	      Tcl_Interp *interp,
	      int argc, char **argv )
{
  extern float translation_amount;
  float temp_amount = translation_amount;
  extern void	moveNets( );


/*  TCL_CHK( "TkNx_MoveNets", argc, 2, "x|y|z   n|p" ); */

  if (argc == 3)
    moveNets( argv[1][0], (argv[2][0] == 'p' ? 1 : -1) );
  else if (argc == 4) {
    translation_amount = (float) atof( argv[3] );
    moveNets( argv[1][0], (argv[2][0] == 'p' ? 1 : -1) );
    translation_amount = temp_amount;
  }
  else
    return TCL_ERROR;
    
  return TCL_OK;
}

/*
 * TkNx_ChangeDimension --
 *
 */
COMMAND_PROC(TkNx_ChangeDimension)
{
    NETWORK	head, get_network_name();
    char 	buff[1024];

    TCL_CHK( "TkNx_ChangeDimension", argc, 1, "{N|H|W");

    head = get_network_name( view_network );
    if (head == NULL) {
	sprintf(buff, "error {no such network %s}", view_network);
	return TCL_OK;
    } 

    switch (argv[1][0]) {
      case 'N':
	head->cell_size   *= change_dimension_amount;
	break;
      case 'H':
	head->name_size_h *= change_dimension_amount;
	break;
      case 'W':
	head->name_size_w *= change_dimension_amount;
    }

    draw_outline( FALSE );

    return TCL_OK;
} 







/* Emacs editing section. DW 94.07.19 */

/*
Local Variables:
c-indent-level:2
c-continued-statement-offset:2
c-block-comments-indent-p:nil
c-brace-offset:0
c-brace-imaginary-offset:0
c-argdecl-indent:4
c-label-offset:-2
c-auto-newline:nil
truncate-partial-width-windows:nil
truncate-lines:nil
End:
*/
