/*****************************************************************
 *								 *
 *				NEXUS				 *
 *								 *
 *			   (nexus_hebb.c)			 *
 *								 *
 *			1/92 by  Shih-Cheng Yen			 *
 *								 *
 *****************************************************************/

#include "nexus.h"

#include "hebb.h"

#ifdef NEXUS_SGI
#include "nexus_tk.h"
#endif

#ifdef NEXUS_LINUX
#include "nexus_tk.h"
#endif

#ifdef NEXUS_SUN
#include <nexus_xview.h>
#include <nexus_xview_functions.h>
#endif


#define hebbWeightLimit 0.000

int do_hebb_rule(CELL cell);

int		hebbFactorChoice = 0,
		hebbPlusPlus = 75,
		hebbPlusMinus = 25,
		hebbMinusPlus = 25,
		hebbMinusMinus = 25;
float		hebbPreThreshold = 0.0,
		hebbPlusPostThreshold = 0.0,
		hebbMinusPostThreshold = 0.0;


#ifdef NEXUS_SUN
/*
 *  'main_frame' -> 'hebb_frame' to concur with rest of program.  DW 94.08.10
 *  'main_panel' -> 'hebb_panel', ibid.  DW 94.08.26
 */
Frame			hebb_frame;
Panel_item		hebbNetworkList;

static Panel		hebb_panel2;
static Panel_item	hebbNetworkName,
			hebbConnectionName,
			hebbConnectionList;
#endif


int
run_hebb( )
{
  NETWORK search_head;
  register int cell_id, flag = TRUE;


  search_head = network_head;
  printf("Doing Hebb Rule\n");
  while (search_head != NULL) {

    for (cell_id = 0;
	 cell_id < search_head->number_cells;
	 cell_id++)
      flag = do_hebb_rule( search_head->cells + cell_id );

    if (flag == TRUE)
      printf( "Changed connections for network %s\n", search_head->name );

    search_head = search_head->next;
  }

  return OK;
}  /* run_hebb( ) */



/*
 *  Perform hebb rule for specified cell by changing the conductance of
 *    connections according to the firing rates of the connected cells
 */

int
do_hebb_rule( cell )
    register CELL cell;
{
  CONNECTION	temp_connection;
  SPECS		temp_specs;
  float		pre_activity, post_activity;
  float		weight, weight_change, scale_factor;
  float		currPlusPostThreshold, currMinusPostThreshold;
  int		count, cell_net, input_cell_net, flag;
  /*
   *  DW 95.04.05  Since random updating no longer calls do_hebb_rule(),
   *    it is now safe to replace swap_flag parameter with a direct call
   *    to query_flag( ACTIVITY_SWAP );
   */


  cell_net = cell->net_id;
  flag = FALSE;
  switch (hebbFactorChoice) {
  case 0:		scale_factor = (float) 0.01; break;
  case 1:		scale_factor = (float) 0.001; break;
  case 2:		scale_factor = (float) 0.0001; break;
  case 3: default:	scale_factor = (float) 0.00001; break;
  }
  if (!query_flag( LEARN_HEBB_TYPE )) {
    currPlusPostThreshold = hebbPreThreshold;
    currMinusPostThreshold = hebbPreThreshold;
  }
  else {
    currPlusPostThreshold = hebbPlusPostThreshold;
    currMinusPostThreshold = hebbMinusPostThreshold;
  }

  /* move through cell's connections */
  for (count = 0; count < (cell->number_connections); count++) {
    temp_connection = cell->connect_list + count;

    /*************************************************************************
     *  If weight is inhibitory or out of range, move on to next connection.
     *************************************************************************/
    weight = temp_connection->conductance;
    if (weight <= 0 ||
	weight < conduct_compute_range_min ||
	weight > conduct_compute_range_max)
      continue;

    /*************************************************************************
     *  Find spec specifying connection:
     *    First, check cell's own specs.  If not found, then check input
     *    cell's specs.  If still not found, or if found but not plastic,
     *    move on to next connection.
     *************************************************************************/
    /*  input cell's network  */
    input_cell_net = temp_connection->input_cell->net_id;
    /*  Try cell's own specs  */
    temp_specs = cell->specs;
    while (temp_specs != NULL &&
	   (temp_specs->id != input_cell_net ||
	    temp_specs->type != RETROGRADE))
      temp_specs = temp_specs->next;  
    if (temp_specs == NULL) {
      /*  Try input cell's specs  */
      temp_specs = temp_connection->input_cell->specs;
      while (temp_specs != NULL &&
	     (temp_specs->id != cell_net ||
	      temp_specs->type != ANTEROGRADE))
	temp_specs = temp_specs->next;
    }
    /*  not found / not plastic  */
    if (temp_specs == NULL || temp_specs->plasticity == OFF)
      continue;

    /*************************************************************************
     *  We have a winner!  If we get to this point,
     *  1)  We have found the specification, either in the cell's own specs
     *      list or in the input cell's specs list.
     *  2)  The connection was found to be plastic.
     *  3)  The connection is excitatory and within the proper range, as set
     *      in the computation section of the conductivity electrode dialog.
     *************************************************************************/

    if (!query_flag( LEARN_HEBB_TYPE ))
      /*  Get firing rate of post-synaptic cell. */
      post_activity = cell->firing_rate;
    else
      post_activity = cell->voltage;

    if (query_flag( ACTIVITY_SWAP ) == ON)
      pre_activity = temp_connection->input_cell->firing_rate_old;
    else
      pre_activity = temp_connection->input_cell->firing_rate;
	  
    if (pre_activity >= hebbPreThreshold &&
	post_activity >= currPlusPostThreshold) {
      weight_change = ((float) fabs((double) weight) * scale_factor * 
		       hebbPlusPlus);
      if (weight_change < (conduct_compute_range_max - weight)) {
	if (weight < 0 && (weight + weight_change) > 0)
	  temp_connection->conductance = hebbWeightLimit;
	else
	  temp_connection->conductance = weight + weight_change;
      }
      else
	temp_connection->conductance = conduct_compute_range_max;
    }

    else if (pre_activity >= hebbPreThreshold &&
	     post_activity < currPlusPostThreshold) {
      weight_change = ((float) fabs((double) weight) * scale_factor * 
		       hebbPlusMinus);
      if (weight_change < (weight - conduct_compute_range_min)) {
	if (weight > 0 && (weight - weight_change) < 0)
	  temp_connection->conductance = hebbWeightLimit;
	else
	  temp_connection->conductance = weight - weight_change;
      }
      else
	temp_connection->conductance = conduct_compute_range_min;
    }

    else if (pre_activity < hebbPreThreshold &&
	     post_activity >= currMinusPostThreshold) {
      weight_change = ((float) fabs((double) weight) * scale_factor * 
		       hebbMinusPlus);
      if (weight_change < (weight - conduct_compute_range_min)) {
	if (weight > 0 && (weight - weight_change) < 0)
	  temp_connection->conductance = hebbWeightLimit;
	else
	  temp_connection->conductance = weight - weight_change;
      }
      else
	temp_connection->conductance = conduct_compute_range_min;
    }

    else {		/*pre-minus,post-minus*/
      weight_change = ((float) fabs((double) weight) * scale_factor * 
		       hebbMinusMinus);
      if (weight_change < (weight - conduct_compute_range_min)) {
	if (weight > 0 && (weight - weight_change) < 0)
	  temp_connection->conductance = hebbWeightLimit;
	else
	  temp_connection->conductance = weight - weight_change;
      }
      else
	temp_connection->conductance = conduct_compute_range_min;
    }

    flag = TRUE;  /*  Yes, we _did_ modify the connection.  */
  }  /* for (cell->number_connections) */

  return flag;
}  /* do_hebb_rule( ) */



int
set_plasticity( net1, net2, select, type )
    NETWORK        net1, net2;
    int            select, type;
{
  register SPECS	temp_specs;
  register CELL		temp_cell = net1->cells;
  register int		counter, skip;
  register int		number_loops = net1->number_cells;
  register int		loops = 0;



  for (counter = 0; counter < number_loops; counter++) {
    temp_specs = (temp_cell + counter)->specs;

    if (loops == 0)
      /*  If loop not performed before, find relevant specs. */
      while (temp_specs->id != net2->id || temp_specs->type != type) {
	temp_specs = temp_specs->next;
	loops++; /*keeps track of number of loops performed*/
      }
    else
      /*  If loop performed before, pointer moves directly to relevant specs */
      for (skip = 0; skip < loops; skip++)
	temp_specs = temp_specs->next;

    if (select)     /*if selecting unselected connection*/
      temp_specs->plasticity = ON;   /*turns plasticity on*/
    else                             /*if unselecting selected connection*/
      temp_specs->plasticity = OFF;  /*turns plasticity off*/
  }

  printf( "Plasticity turned %s for connection '%s %s %s'.\n",
	 (select) ? "ON" : "OFF",
	 net1->name,
	 (type) ? "from" : "to",
	 net2->name );

  return OK;
}



#if defined(NEXUS_SGI) || defined(NEXUS_LINUX)
/*
 * TkNx_GetHebbConnections
 *
 * argv[1] = name of the network
 * argv[2] = pathname of the listbox to fill in the connection name
 * argv[3] = pathname of the listbox to fill in the connection flag
 * 
 * return value: a list of all the id
 */

COMMAND_PROC(TkNx_GetHebbConnections)
{
    NETWORK 		netPtr;
    SPECS		conPtr;
    char		buff[MAXARRAY], buffon[MAXARRAY], buffoff[MAXARRAY];

    TCL_CHK("TkNx_GetHebbConnections", argc, 3, 
	    "network_name conn_list flag_list");

    if ((netPtr = get_network_name(argv[1])) == 0) {
	return TCL_ERROR;
    }

    sprintf(buffon,  "%s insert end ON", argv[3]);
    sprintf(buffoff, "%s insert end {}", argv[3]);

    for (conPtr = netPtr->cells->specs; conPtr; conPtr=conPtr->next) {
	char * prefix, *name;

	if (conPtr->type == RETROGRADE) {
	    prefix = "From-";
	} else {
	    prefix = "To-";
	}
	name = get_network_id(conPtr->id)->name;

	/* fill in the name */
	sprintf(buff, "%s insert end {%s%s}", argv[2], prefix, name);
	if (Tcl_Eval(interp, buff)!= TCL_OK) {
	    return TCL_ERROR;
	}

	/* fill in the flag */
	if (conPtr->plasticity == ON) {
	    if (Tcl_Eval(interp, buffon)!= TCL_OK) {
		return TCL_ERROR;
	    }
	} else {
	    if (Tcl_Eval(interp, buffoff)!= TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }

    return TCL_OK;
}



/*
 * TkNx_SetHebbConnection
 *
 * argv[1] = name of the network
 * argv[2] = name of connection
 * argv[3] = "0" = OFF, "1" = ON;
 * 
 * return value: a list of all the id
 */
COMMAND_PROC(TkNx_SetHebbConnection)
{
  int connect_type;

  TCL_CHK("TkNx_SetHebbConnection", argc, 3,
	  "network_name conn_name value");

  if (argv[2][2] == '-') {
    argv[2] += 3;
    connect_type = ANTEROGRADE;
  } else {
    argv[2] += 5;
    connect_type = RETROGRADE;
  }


  /*
   *  Query to see if the selected item is now highlighted ("ON").  If so,
   *    set this connection's plasticity; otherwise, reset it.  (The actual
   *    toggle expression is in nxHebb.tcl.  The third argument presented
   *    here simply translates the string passed by nxHebb:ListSelect.)
   */
  set_plasticity( get_network_name( argv[1] ),
		 get_network_name( argv[2] ),
		 (argv[3][0] == '0' ? 0 : 1),
		 connect_type );

  return TCL_OK;
}
#endif /* defined(NEXUS_SGI) || defined(NEXUS_LINUX) */
	      
	
#ifdef NEXUS_SUN
/*
 *  List all of a network's connections when one is selected.
 */

void
hebbSetupConnectionList( network )
    NETWORK	network;
{
  NETWORK		get_network_id( );
  SPECS			temp_connect;
  int			count;
  char			ret_string[MAXARRAY],
  			ant_string[MAXARRAY];



  delete_list_entries( hebbConnectionList );
  temp_connect = network->cells->specs; /*gets first specs*/

  for (count = 0; temp_connect != NULL; count++) {
    strcpy(ret_string, "From-"); /*specify type of connection in list*/
    strcpy(ant_string, "To-");

    /*
     *  Get connected network and type, and insert into list
     */
    if (temp_connect->type == RETROGRADE) {
      strcat( ret_string, get_network_id( temp_connect->id )->name );
      xv_set( hebbConnectionList,
	     PANEL_LIST_INSERT, count,
	     PANEL_LIST_STRING, count, ret_string,   
	     NULL );
    }
    else {
      strcat( ant_string, get_network_id( temp_connect->id )->name );
      xv_set( hebbConnectionList,
	     PANEL_LIST_INSERT, count,
	     PANEL_LIST_STRING, count, ant_string,
	     NULL );
    }

    /* shows status of plasticity flag in menu */
    if (temp_connect->plasticity == ON)
      xv_set( hebbConnectionList,
	     PANEL_LIST_SELECT, count, TRUE,
	     NULL );
    temp_connect = temp_connect->next; /*next specs*/
  }
}



void
hebbProcSelectNetwork( item, string, client_data, op, event, row )
    Panel_item		item;
    char		*string;
    Xv_opaque		client_data;
    Panel_list_op	op;
    Event		*event;
    int			row;
{
  extern NETWORK	get_network_name( );


  
  xv_set( hebbNetworkName,
	 PANEL_VALUE, string,
	 NULL );
  hebbSetupConnectionList( get_network_name( string ) );
}



/*
 *  Took code from proc_enter_name( ) to standardize this entry.  DW 94.11.09
 */

Panel_setting
hebbProcEnterNetwork( item, event )
    Panel_item		item;
    Event		*event;
{
  extern NETWORK	get_network_name( );
  char			entered_name[NAME_SIZE + 25];
  int			i, nrows = (int) xv_get( hebbNetworkList,
						PANEL_LIST_NROWS );



  strcpy( entered_name, (char *) xv_get( item, PANEL_VALUE ) );
  if (strlen( entered_name )) {
    for (i = 0; i < nrows; i++) {
      if (strcmp( entered_name,
		 (char *) xv_get( hebbNetworkList,
				 PANEL_LIST_STRING, i )) == 0) {
	xv_set( hebbNetworkList,
	       PANEL_LIST_SELECT, i, TRUE,
	       NULL );
	hebbSetupConnectionList( get_network_name( entered_name ) );
	return PANEL_NEXT;
      }
    }
  }
  else			/* There are no networks.  Don't do anything. */
    return PANEL_NEXT;

  strcat( entered_name, " is not a valid network." );
  notice_prompt( hebb_panel2, NULL,
		NOTICE_MESSAGE_STRING, entered_name,
		NULL );
  xv_set( item,
	 PANEL_NOTIFY_STATUS, XV_ERROR,
	 PANEL_VALUE, "",
	 NULL );
  return PANEL_NONE;
}



Panel_setting
hebbProcEnterConnection( item, event )
    Panel_item		item;
    Event		*event;
{
  char  net1[NAME_SIZE], net2[NAME_SIZE];
  char	string[NAME_SIZE + 25];
  int   connect_type, selected, i;
  int	nrows = (int) xv_get( hebbConnectionList, PANEL_LIST_NROWS );

  extern int	set_plasticity( );


  strcpy( net1, (char *) xv_get( hebbNetworkName, PANEL_VALUE ) );
  strcpy( string, (char *) xv_get( hebbConnectionName, PANEL_VALUE ) );
  
  if (string[2] == '-') {
    strcpy( net2, &string[3] ); /*if prefix is "To-"*/
    connect_type = ANTEROGRADE;
  }
  else if (string[4] == '-') {
    strcpy( net2, &string[5] ); /*if prefix is "From-"*/
    connect_type = RETROGRADE;
  }
  else {
    strcat( string, " is not a valid connection." );
    notice_prompt( hebb_panel2, NULL,
		  NOTICE_MESSAGE_STRING, string,
		  NULL );
    xv_set( item,
	   PANEL_NOTIFY_STATUS, XV_ERROR,
	   PANEL_VALUE, "",
	   NULL );
    return PANEL_NONE;
  }

  for (i = 0; i < nrows; i++)
    if (strcmp( string,
	       (char *) xv_get( hebbConnectionList,
			       PANEL_LIST_STRING, i )) == 0) {
      selected = xv_get( hebbConnectionList, PANEL_LIST_SELECTED, i );
      selected = (selected ? FALSE : TRUE);
      xv_set( hebbConnectionList,
	     PANEL_LIST_SELECT, i, selected,
	     NULL );

      /*
       *  Query to see if the selected item is now highlighted ("ON").  If so,
       *    set this connection's plasticity; otherwise, reset it.
       */
      set_plasticity( get_network_name( net1 ),
		     get_network_name( net2 ),
		     selected,
		     connect_type );

      return PANEL_NEXT;
    }

  strcat( string, " is not a valid connection." );
  notice_prompt( hebb_panel2, NULL,
		NOTICE_MESSAGE_STRING, string,
		NULL );
  xv_set( item,
	 PANEL_NOTIFY_STATUS, XV_ERROR,
	 PANEL_VALUE, "",
	 NULL );
  return PANEL_NONE;
}



void
hebbProcSelectConnection( item, string, client_data, op, event, row )
    Panel_item		item;
    char		*string;
    Xv_opaque		client_data;
    Panel_list_op	op;
    Event		*event;
    int			row;
{
  char  net1[MAXARRAY], net2[MAXARRAY];
  int   connect_type, selected;



  strcpy( net1, (char *) xv_get( hebbNetworkName, PANEL_VALUE ) );
  
  if (string[2] == '-') {
    strcpy( net2, &string[3] ); /*if prefix is "To-"*/
    connect_type = ANTEROGRADE;
  }
  else {
    strcpy( net2, &string[5] ); /*if prefix is "From-"*/
    connect_type = RETROGRADE;
  }
  xv_set( hebbConnectionName,
	 PANEL_VALUE, string,
	 NULL );

  selected = xv_get( hebbConnectionList, PANEL_LIST_SELECTED, row );
  set_plasticity( net1, net2, selected, connect_type );
}









/*****************************************************************************
 *  Set up the Hebbian Learning window.
 *****************************************************************************/

int
setup_menu_hebb( top_frame )
    Frame top_frame;
{
  extern int	hebbFactorChoice,
		hebbPlusPlus,
		hebbPlusMinus,
		hebbMinusPlus,
		hebbMinusMinus;
  extern float	hebbPreThreshold,
		hebbPlusPostThreshold,
		hebbMinusPostThreshold;
  Panel		hebb_panel;

  char		*temp_str = "00000000000000000000";



  hebb_frame = (Frame) xv_create( top_frame, FRAME,
				 FRAME_LABEL, "Hebb Plasticity",
				 FRAME_INHERIT_COLORS, TRUE,
				 FRAME_WM_COMMAND_STRINGS, -1, NULL,
				 XV_WIDTH, 1000,
				 XV_HEIGHT, 1000,
				 NULL );
  hebb_panel = (Panel) xv_create( hebb_frame, PANEL,
				 OPENWIN_SHOW_BORDERS, TRUE,
				 PANEL_LAYOUT,	PANEL_HORIZONTAL,
				 NULL );

  (void) xv_create( hebb_panel,			PANEL_CHECK_BOX,
		   PANEL_CHOOSE_ONE,		TRUE,
		   XV_X,			10,
		   XV_Y,			10,
		   PANEL_LABEL_STRING,		"HEBB RULE",
		   PANEL_CHOICE_STRINGS,	"OFF", "ON", NULL,
		   PANEL_NOTIFY_PROC,		proc_chbox_binary,
		   PANEL_CLIENT_DATA,		LEARN_HEBB,
		   PANEL_VALUE,			query_flag( LEARN_HEBB ),
		   NULL );

  (void) xv_create( hebb_panel,			PANEL_CHECK_BOX,
		   PANEL_CHOOSE_ONE,		TRUE,
		   PANEL_NEWLINE,		10,
		   PANEL_LABEL_STRING,		"Using Post-synaptic",
		   PANEL_CHOICE_STRINGS,	"Firing Rate","Voltage", NULL,
		   PANEL_NOTIFY_PROC,		proc_chbox_binary,
		   PANEL_CLIENT_DATA,		LEARN_HEBB_TYPE,
		   PANEL_VALUE,			query_flag( LEARN_HEBB_TYPE ),
		   NULL );

  sprintf( temp_str, "%9.3f", hebbPreThreshold );
  (void) xv_create( hebb_panel,			PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Pre-Synaptic Threshold: ",
		   PANEL_NEWLINE,		20,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&hebbPreThreshold,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( hebb_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  (void) xv_create(hebb_panel, PANEL_MESSAGE,
		   PANEL_LABEL_STRING,		"Post-Synaptic Thresholds if:",
		   PANEL_LABEL_BOLD,		TRUE,
		   PANEL_NEWLINE,		10,
		   NULL);

  sprintf( temp_str, "%9.3f", hebbPlusPostThreshold );
  (void) xv_create( hebb_panel,			PANEL_TEXT,
		   PANEL_LABEL_STRING,		"(Pre +): ",
		   PANEL_NEWLINE,		5,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&hebbPlusPostThreshold,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( hebb_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  sprintf( temp_str, "%9.3f", hebbMinusPostThreshold );
  (void) xv_create( hebb_panel,			PANEL_TEXT,
		   PANEL_LABEL_STRING,		"(Pre -): ",
		   PANEL_NEWLINE,		5,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&hebbMinusPostThreshold,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( hebb_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  (void) xv_create( hebb_panel,			PANEL_CHECK_BOX,
		   PANEL_CHOOSE_ONE,		TRUE,
		   PANEL_NEWLINE,		20,
		   PANEL_LABEL_STRING,		"Scaling Factor:  ",
		   PANEL_CHOICE_NCOLS,		2,

		   PANEL_CHOICE_STRINGS,
		   "0.01",
		   "0.001",
		   "0.0001",
		   "0.00001",
		   NULL,

		   PANEL_NOTIFY_PROC,		proc_chbox_generic,
		   PANEL_CLIENT_DATA,		&hebbFactorChoice,
		   PANEL_VALUE,			hebbFactorChoice,
		   NULL);

  (void) xv_create( hebb_panel,
		   PANEL_TITLE,
		   PANEL_NEWLINE, 20,
		   PANEL_TITLE_STRING, "Multiply the following by the above.",
		   NULL );

  (void) xv_create( hebb_panel,
		   PANEL_PERCENTAGE_SLIDER( hebbPlusPlus ),
		   PANEL_LABEL_STRING, "Pre + Post + ",
		   PANEL_NEWLINE, 15,
		   XV_X, 30,
		   NULL );
  (void) xv_create( hebb_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  (void) xv_create( hebb_panel,
		   PANEL_PERCENTAGE_SLIDER( hebbPlusMinus ),
		   PANEL_LABEL_STRING, "Pre + Post - ",
		   PANEL_NEWLINE, 15,
		   XV_X, 30,
		   NULL );
  (void) xv_create( hebb_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  (void) xv_create( hebb_panel,
		   PANEL_PERCENTAGE_SLIDER( hebbMinusPlus ),
		   PANEL_LABEL_STRING, "Pre - Post + ",
		   PANEL_NEWLINE, 15,
		   XV_X, 30,
		   NULL );
  (void) xv_create( hebb_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  (void) xv_create( hebb_panel,
		   PANEL_PERCENTAGE_SLIDER( hebbMinusMinus ),
		   PANEL_LABEL_STRING, "Pre - Post - ",
		   PANEL_NEWLINE, 15,
		   XV_X, 30,
		   NULL );
  (void) xv_create( hebb_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );
  window_fit( hebb_panel );

  hebb_panel2 = (Panel) xv_create( hebb_frame, PANEL,
				 OPENWIN_SHOW_BORDERS, TRUE,
				 PANEL_LAYOUT,	PANEL_HORIZONTAL,
				 NULL );
  hebbNetworkName = (Panel_item)
    xv_create( hebb_panel2,			PANEL_TEXT,
	      PANEL_LABEL_STRING,		"Change Network: ",
	      PANEL_NEWLINE,			50,
	      PANEL_VALUE,			"",
	      PANEL_VALUE_DISPLAY_LENGTH,	15,
	      PANEL_NOTIFY_PROC,		hebbProcEnterNetwork,
	      NULL );
  (void) xv_create( hebb_panel2,		PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );
  hebbNetworkList = (Panel_item)
    xv_create( hebb_panel2,			PANEL_LIST,
	      PANEL_NEWLINE,			10,
	      PANEL_CHOOSE_ONE,			TRUE,
	      PANEL_LIST_WIDTH,			300,
	      PANEL_LIST_DISPLAY_ROWS,		3,
	      PANEL_LIST_INSERT_DUPLICATE,	FALSE,
	      PANEL_LIST_TITLE,			"Available Networks:",
	      PANEL_LIST_STRINGS,		"", NULL,
	      PANEL_LIST_SELECT,		0, FALSE,
	      PANEL_NOTIFY_PROC,		hebbProcSelectNetwork,
	      NULL );

  hebbConnectionName = (Panel_item)
    xv_create( hebb_panel2,			PANEL_TEXT,
	      PANEL_LABEL_STRING,		"Connections: ",
	      PANEL_NEWLINE,			15,
	      PANEL_VALUE,			"",
	      PANEL_VALUE_DISPLAY_LENGTH,	15,
	      PANEL_NOTIFY_PROC,		hebbProcEnterConnection,
	      NULL );
  (void) xv_create( hebb_panel2,		PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );
  hebbConnectionList = (Panel_item)
    xv_create( hebb_panel2,			PANEL_LIST,
	      PANEL_NEWLINE,			10,
	      PANEL_CHOOSE_ONE,			FALSE,
	      PANEL_LIST_WIDTH,			300,
	      PANEL_LIST_DISPLAY_ROWS,		3,
	      PANEL_LIST_INSERT_DUPLICATE,	FALSE,
	      PANEL_LIST_TITLE,			"Available Connections:",
	      PANEL_LIST_STRINGS,		"", NULL,
	      PANEL_LIST_SELECT,		0, FALSE,
	      PANEL_NOTIFY_PROC,		hebbProcSelectConnection,
	      NULL);

  (void) xv_create( hebb_panel2,		PANEL_BUTTON,
		   PANEL_LABEL_STRING,		"Cancel",
		   PANEL_NEWLINE,		45,
		   PANEL_NOTIFY_PROC,		proc_btn_cancel,
		   PANEL_CLIENT_DATA,		hebb_frame,
		   NULL );

  window_fit( hebb_panel2 );
  xv_set( hebb_panel2,
	 XV_HEIGHT, (int) xv_get( hebb_panel, XV_HEIGHT ),
	 NULL );
  window_fit( hebb_frame );
}  /* setup_menu_hebb() */



/* function called when Hebb Rule button selected in learning menu */

void
go_hebb( item, event )
    Panel_item	item;
    Event	*event;
{
  delete_list_entries( hebbConnectionList );
  xv_set( hebb_frame,
	 XV_SHOW, TRUE,
	 NULL );
}
#endif







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
