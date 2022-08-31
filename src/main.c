/**************************************************************
 *                                                            *
 *                            NEXUS                           *
 *                                                            *
 *              (c) 1990 Paul Sajda and Leif Finkel           *
 *                                                            *
 *                      (nexus_main.c)                        *
 *							      *
 *  VOGLE-specific and XView-specific code removed to their   *
 *			own files.  DW			      *
 *                                                            *
 **************************************************************/

#include "nexus.h"
#include "main.h"
#include "run.h"
#include "bp.h"
#include "rbf.h"
#include "inversion.h"
#include "graphics.h"

#ifdef NEXUS_SGI
#include "nexus_tk.h"
#include "tix.h"
#endif

#ifdef NEXUS_LINUX
#include "nexus_tk.h"
#include <tix.h>
#endif


/**
 **  Global variables.
 **/

/*
 *  Head of simulation data structure.
 */

NETWORK 	network_head;

/*
 *  Dynamically allocated array of network pointers, in order of network ID.
 *  Aids in looking up pointer, given a net_id.  DW 94.07.13
 */

NETWORK		*network_xref_list;

#if 0
/*
 *  Keep track of all the learning methods currently being used.
 */

int		methodCount = 0;
#endif

/*
 *  For backwards compatibility, here's the settings structure dinosaur.  :{
 *  Data structure used to hold parameter values for the simulation.
 */

/* allocate memory: this is not global! */
static ELEMENT_SETTINGS		settings_s;

/* set up global pointer to above */
SETTINGS			setting = &settings_s;

/*
 *  The following are updated by the User Interface.
 *  (Graphical UI's: XView, Tk)
 *  (Scripting UI: Tcl)
 */

flag_t		nexus_flags				= 0;

int		nxDebug					= FALSE;

int		stop_simulation				= FALSE;
int		simulate_type				= SEQUENTIAL;

int		curr_electrode				= MODE_ACTIVITY;

int		display_type				= DISP_CELL;
int		graphics_off				= FALSE;

int		simulation_state			= OFF;
int		explore_variables			= OFF;
int 		do_exit					= FALSE;
int		activity_output_precision		= ONE_DECIMAL;
int		number_cycles				= 1;
int		random_cells				= 5000;
int		setActivityPercentage			= 100;

float		conduct_display_range_min	= DEFAULT_CONDUCTANCE_MIN,
		conduct_display_range_max	= DEFAULT_CONDUCTANCE_MAX,
		activity_display_range_min	= DEFAULT_ACTIVITY_MIN,
		activity_display_range_max	= DEFAULT_ACTIVITY_MAX;
float		conduct_compute_range_min		= -1.0,
		conduct_compute_range_max		= 1.0;
float		change_dimension_amount			= 1.0;

int		activity_display_depth			= DISP_COLOUR;
int		conduct_display_depth			= DISP_COLOUR;

char		simulation_filename[NAME_SIZE]		= "";
char		send_connections_filename[NAME_SIZE]	= "";
char		activity_filename[NAME_SIZE]		= "";
char		activity_output_filename[NAME_SIZE]	= "";
char		activity_output_network_name[NAME_SIZE]	= "";
char		parameter_network[NAME_SIZE]		= "";

FILE		*activity_output_fp			= NULL;
/* FILE           *cycle_plot_fp; */

#ifdef NEXUS_SGI
char		curr_access_directory[FILENAME_MAX]	= "";
#endif
#ifdef NEXUS_LINUX
char		curr_access_directory[FILENAME_MAX]	= "";
#endif









/***********
 **  nexus_main.c
 ***********/

#ifdef NEXUS_SGI
int
main( argc, argv )
    int argc;
    char ** argv;
{
  Tcl_Interp * interp;

  
  /* Initialize the Tix wish shell:
   *	- create an interpreter
   *  - create the main window [in Tk_MainWindow(interp)]
   *
   * The third argument is the application run-time command (rc) file.
   *
   * The fourth argument specifies whether wish should prompt and read
   * from the standard input. Possible values are:
   *
   * 		TIX_STDIN_ALWAYS
   *		TIX_STDIN_OPTIONAL
   *		TIX_STDIN_NONE
   *
   */


  if (argv[0][0] == 'd')
    nxDebug = TRUE;

  /*
   * parse command-line, argv, read rc file
   * always prompt required.
   */
  interp = Tix_WishInit( &argc, argv, "~/.tixwishrc", TIX_STDIN_ALWAYS );


  /*
   * Initialize other optional modules here
   */

  if (TkGLx_Init( interp, Tk_MainWindow( interp ) ) == TCL_ERROR) {
    fprintf(stderr, "%s\n", interp->result);
    Tix_Exit(interp, 1);
  }

  if (TkNx_Init( interp, Tk_MainWindow( interp ) ) == TCL_ERROR) {
    fprintf(stderr, "%s\n", interp->result);
    Tix_Exit(interp, 1);
  }


  /*
   * Loop infinitely, waiting for commands to execute.  When there
   * are no windows left, Tk_MainLoop returns and we exit.
   */

  Tix_MainLoop( interp );

  /*
   * Don't exit directly, but rather invoke the Tcl "exit" command.
   * This gives the application the opportunity to redefine "exit"
   * to do additional cleanup.
   */

  Tix_Exit( interp, 0 );
}



#endif /* NEXUS_SGI */

#ifdef NEXUS_LINUX
int
Tcl_AppInit(interp)
    Tcl_Interp *interp;         /* Interpreter for application. */
{
  /* Initialize the Tcl, Tk and Tix packages (in this order) */

  if (Tcl_Init(interp) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Tk_Init(interp) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Tix_Init(interp) != TCL_OK) {
    return TCL_ERROR;
  }

  if (TkGLx_Init( interp, Tk_MainWindow( interp ) ) == TCL_ERROR) {
    fprintf(stderr, "%s\n", interp->result);
    Tix_Exit(interp, 1);
  }

  if (TkNx_Init( interp, Tk_MainWindow( interp ) ) == TCL_ERROR) {
    fprintf(stderr, "%s\n", interp->result);
    Tix_Exit(interp, 1);
  }

  Tix_SetRcFileName(interp, "~/.tixwishrc");

  Tk_MainLoop();

  return TCL_OK;
}

int
main(argc, argv)
    int argc;                   /* Number of command-line arguments. */
    char **argv;                /* Values of command-line arguments. */
{
  if (argv[0][0] == 'd')
    nxDebug = TRUE;

  Tk_Main(argc, argv, Tcl_AppInit);
  return 0;                   /* Needed only to prevent compiler warning. */
}
#endif /* NEXUS_LINUX */



#if defined(NEXUS_SGI) || defined(NEXUS_LINUX)
/*
 *  Initialization routine.  DW 95.01.22
 */

int
nexus_init( )
{
  /*
   *  All flags are OFF by default (value is 0).  To set any, add them
   *    to this statement.  DW 94.08.26
   */
  SET_FLAG( CONNECT_SHOW_TYPE, RETROGRADE );
  SET_FLAG( ACTIVITY_STATE, EXAMINE );

  init_random_number_gen( );

  /*
   *  Call learning method function-pointer structure-initialization
   *    routines here.  DW 95.01.22
   */

#if 0

#ifdef NEXUS_RBF
  (NEXUS_LEARN *) methodArray [methodCount++] = rbfInit( );
#endif /* NEXUS_RBF */

#ifdef NEXUS_BP
  (NEXUS_LEARN *) methodArray [methodCount++] = bpInit( );
#endif /* NEXUS_BP */

#ifdef NEXUS_INVERSION
  (NEXUS_LEARN *) methodArray [methodCount++] = inversionInit( );
#endif /* NEXUS_INVERSION */

#ifdef NEXUS_HEBB
  (NEXUS_LEARN *) methodArray [methodCount++] = hebbInit( );
#endif /* NEXUS_HEBB */

#endif /* 0 */

  return OK;
}
#endif /* defined(NEXUS_SGI) || defined(NEXUS_LINUX) */



#ifdef NEXUS_SUN
/*
 *  Introduction screen.  Call intro graphics.
 */

introduction( )
{
  extern void	intro_graphics( );

  system( "clear" );
  printf( "\n\n\n" );
  printf( "                                 NEXUS\n\n" );
  printf( "              (c) 1990 University of Pennsylvania\n" );
  printf( "             Created by: Paul Sajda and Leif Finkel\n" );
  printf( "                         Version %s\n\n\n", NEXUS_VERSION );

  intro_graphics( );
}
#endif /* NEXUS_SUN */



/*
 *  Initialize random number generator - use time since 
 *  00:00:00 GMT(Sun) / UTC(SGI),  1/1/70 (secs) as seed
 */

void
init_random_number_gen( )
{
  time_t	timeofday = time( NULL );


  srandom( timeofday );
}



/*
 *  perhaps we will free some memory, hmmm...? DW 94.07.27
 */

int
cleanup( )
{
  extern int	cleanup_graphics( );
  int		result;


  close_error_files( );

#ifdef NEXUS_BP
  bpFreeParam( );
#endif

#ifdef NEXUS_RBF
  rbfFreeParam( );
#endif

#ifdef NEXUS_INVERSION
  invFreeParam( );
#endif

  if (network_xref_list)
    free( network_xref_list );

  if ((result = cleanup_graphics( ))) /* Exit graphics library; DW 94.07.29 */
    return result;
  else
    return OK;
}



/*****************************************************************************
 *  Utility routines without toolkit-specific code.
 *****************************************************************************/



/*
 *  set_network( NETWORK, float, float )    **RECURSIVE** routine!!!
 *  randomize_networks( ) is only other parent.  DW 94.07.25.
 *  Should replace with loop through net_xref_list.
 *  set_cells( ) eliminated... this was only parent.  DW 94.08.10
 */

void
set_network( head, rand_min, rand_max )
    NETWORK		head;
    float		rand_min, rand_max;
{
  register int	count;



#ifdef DEBUG
  printf( "rand_min = %f, rand_max = %f\n", rand_min, rand_max );
  printf( "random = %f\n", random() / 2.147483e+9 * (rand_max-rand_min)
	 + rand_min );
#endif

  if (head) {			/* Check for end of recursion */

    if (head->cells->clamp != CLAMP_ON) {
      for (count = 0; count < head->number_cells; count++) {
	if ((head->cells + count)->clamp == CLAMP_OFF)
	  (head->cells + count)->firing_rate_old =
	    (head->cells + count)->firing_rate =
	      get_random( rand_min, rand_max );
      }

      graph_network( head );
    }

    /*  Recurse through network list  */
    set_network( head->next, rand_min, rand_max );
  }
}



int
query_flag( which_flag )
    register flag_t	which_flag;
{
  extern flag_t	nexus_flags;	/* used in the QUERY_FLAG macro */

  return QUERY_FLAG( which_flag );
}



void
do_memory( )
{
  printf( "\n\nMemory information:\n" );

#ifdef NEXUS_SUN
#ifdef DEBUG
  fprintf( stderr, "vmstat; ps -v; pstat -T:\n" );
#endif /* DEBUG */
  system( "vmstat;ps -v;pstat -T" );
#endif

#ifdef NEXUS_SGI
#ifdef DEBUG
  fprintf( stderr, "ps -l:\n" );
#endif /* DEBUG */
  system( "ps -l" );
#endif

#ifdef NEXUS_LINUX
#ifdef DEBUG
  fprintf( stderr, "ps -l:\n" );
#endif /* DEBUG */
  system( "ps -l" );
#endif

  printf( "Each network     = %d bytes\n", (int)sizeof( ELEMENT_NETWORK ) );
  printf( "Each cell        = %d bytes\n", (int)sizeof( ELEMENT_CELL ) );
  printf( "Each connection  = %d bytes\n", (int)sizeof( ELEMENT_CONNECTION ) );
  printf( "Each conn. spec. = %d bytes\n\n", (int)sizeof( ELEMENT_SPECS ) );

}  /* do_memory() */



void
load_activity_file( fp, head )
    FILE *fp;
    NETWORK head;
{
  register int 	num,
		netx, nety,
		x, y;
  register CELL	cell;
  float		rate;


  cell = head->cells;
  netx = head->dim_x;
  nety = head->dim_y;
  
  for (y = nety - 1; y >= 0; y--)
    for (x = 0; x < netx; x++) {
      num = y * netx + x;
      if (fscanf( fp, "%f", &rate ) == EOF) {
	printf( "Warning: Activity file is shorter than network <%s>!\n",
	       head->name );
	goto breakout;  /* DW 95.03.16  break will only break out of x loop */
      }
      (cell + num)->firing_rate =  rate;
      (cell + num)->firing_rate_old =  rate;
    }

 breakout:

  if (fscanf( fp, "%f", &rate) != EOF)
    printf( "Warning: Activity file is larger than network <%s>!\n",
	   head->name );

  if (display_type != DISP_OFF && !graphics_off)
    graph_network( head );
}



int
output_activity( fp )
    FILE	*fp;
{
  extern char		activity_output_network_name[];
  extern NETWORK	get_network_name( );
  NETWORK		activity_output_network;
  CELL			curr_cell;
  int			netx, nety, x, y;



  if (!(activity_output_network =
	get_network_name( activity_output_network_name ))) {
    /* 6/20/94 J. B. */
    fprintf( stderr, "ERROR: can't output network to file;\n" );
    fprintf( stderr, "  Network '%s' is nonexistent.\n",
	    activity_output_network_name );
    return ERROR;
  }

  curr_cell = activity_output_network->cells;
  netx = activity_output_network->dim_x;
  nety = activity_output_network->dim_y;

  for (y = nety - 1; y >= 0; y--) {
    for (x = 0; x < netx; x++) {
      switch (activity_output_precision) {
      case ONE_DECIMAL:
	fprintf( fp, "%.1f\t", (curr_cell + y * netx + x)->firing_rate );
	break;
      case THREE_DECIMAL:
	fprintf( fp, "%.3f\t", (curr_cell + y * netx + x)->firing_rate );
	break;
      default:
	fprintf( fp, "%f\t", (curr_cell + y * netx + x)->firing_rate );
	break;
      }
    }
    fprintf( fp, "\n" );
  }

  return OK;
}



/*
 *  Takes care of setting / resetting clamp status,
 *     setting activity levels of entire network,
 *     and setting the cells' threshold level.
 */

void
set_clamp( head, clamp_flag, value )
    NETWORK	head;
    int		clamp_flag;
    float	value;
{
  CELL head_cell;
  int count;

  head_cell = head->cells;

  for( count = 0; count < head->number_cells; count++ ) {
    switch (clamp_flag) {

    case CLAMP_ON: case CLAMP_OFF:
      (head_cell + count)->clamp = clamp_flag;
      break;

    case SET_ACTIVITY:
      (head_cell + count)->firing_rate = value;
      (head_cell + count)->firing_rate_old = value;
      break;
      
    case SET_THRESHOLD:
      (head_cell + count)->threshold = value;
      break;

    } /* switch */
  } /* for */
}



void
set_param( network, parameter, value )		/* rev. 12/17/92  */
    NETWORK         network;
    int             parameter; /* changed, DW 940725 */
    float           value;
{
  if (parameter == SET_SCALE)
    network->scale = value;
  
  if (parameter == SET_DECAY)
    network->decay = value;
  
  if (parameter == SET_OFFSET)
    network->offset = value;
}



int
set_trans_func(network, clamp_flag, trans_func_arg)	/* rev. 4/13/92 */
    NETWORK         network;
    int             clamp_flag;
    char            *trans_func_arg;
{
  int 	check=0;
  int 	i,j,k;
  char	transfer_func[FUNCTION_SIZE];
  char	transfer_func_type[FUNCTION_ARG_SIZE],
  	transfer_func_content[FUNCTION_SIZE];
  char	pgn_arg[FUNCTION_SIZE], pgn_arguments[FUNCTION_ARG_SIZE];
  int 	pgn_arc;
  int	comma[50];
  
  
  if ( (check = sscanf(trans_func_arg,"%[^(] %*[(] %[^)]",
		       transfer_func_type, transfer_func_content)) == 0 ) {
    printf(" INVALID transfer function \n");
  }
  /* Remove spaces */
  sscanf( transfer_func_type, "%s", transfer_func_type );
  
  if (check == 1) {
    /* no argument , thus set default min , max ,slope */
    network->function_min = DEFAULT_ACTIVITY_MIN;
    network->function_max = DEFAULT_ACTIVITY_MAX;
    network->function_slope = DEFAULT_ACTIVITY_SLOPE;

    /* to convert simnoid to simple, step to binary */
    if ( strcmp( transfer_func_type, "sigmoid" ) == 0 ) {
      sprintf( network->class, "simple");
      return(OK);
    }
    if ( strcmp( transfer_func_type, "step" ) == 0 ) {
      sprintf( network->class, "binary");
      return(OK);
    }

    /* linear */
    if ( strcmp( transfer_func_type, "linear" ) == 0 ) {	
      sprintf( network->class,"%s",transfer_func_type );
      return(OK);
    }

    /* allow other funcs for connect( ) */
    sscanf( transfer_func,"%s",network->class );
    return(OK);
  }

  if (check == 2) {
    /* arguments exist */
    /* set defaults of min, max, slope */ 		
    network->function_min = DEFAULT_ACTIVITY_MIN;
    network->function_max = DEFAULT_ACTIVITY_MAX;
    network->function_slope = DEFAULT_ACTIVITY_SLOPE;
    
    /* sigmoid, 3 arguments, convert simnoid to simple */
    if ( strcmp( transfer_func_type, "sigmoid" ) == 0 ) {	
      sprintf( network->class, "simple");
      if ( sscanf(transfer_func_content, "%f %*[,] %f %*[,] %f",
		  &network->function_min, &network->function_max,
		  &network->function_slope ) != 3 )
	fprintf( stderr,
		" ERROR in sigmoid function of network<%s>, defaults used.\n",
		network->name );
      /* fprintf( stderr,"# slope %f \n",network->function_slope); */
      return(OK);
    }

    /* step, 2 arguments, convert to binary */
    if ( strcmp( transfer_func_type, "step" ) == 0 ) {
      sprintf( network->class, "binary");
      if ( sscanf(transfer_func_content, "%f %*[,] %f",
		  &network->function_min, &network->function_max ) != 2 )
	fprintf( stderr,
		" ERROR in step function of network<%s>, defaults used.\n",
		network->name );
      return(OK);
    }

    /* linear 3 argument */
    if ( strcmp( transfer_func_type, "linear" ) == 0 ) {	
      sprintf( network->class,"%s",transfer_func_type );
      if ( sscanf(transfer_func_content, "%f %*[,] %f %*[,] %f",
		  &network->function_min, &network->function_max,
		  &network->function_slope ) != 3 )
	fprintf( stderr,
		" ERROR in linear function of network<%s>, defaults used.\n",
		network->name );
      return(OK);
    }

    /* ln 1 argument SY 94.11.08 */
    if ( strcmp( transfer_func_type, "ln" ) == 0 ) {
      sprintf( network->class, "%s", transfer_func_type );
      if (sscanf(transfer_func_content, "%f",
		 &network->function_slope ) != 1)
	fprintf( stderr,
		" ERROR in ln function of network<%s>, defaults used. \n",
		network->name );
      return OK;
    }

    /* exp 1 argument SY 94.11.08 */
    if ( strcmp( transfer_func_type, "exp" ) == 0 ) {
      sprintf( network->class, "%s", transfer_func_type );
      if (sscanf( transfer_func_content, "%f",
		 &network->function_slope ) != 1)
	fprintf( stderr,
		" ERROR in exp function of network<%s>, defaults used. \n",
		network->name );
      return OK;
    }

    /* pgn is not _ 			*/
    /* content may have plural arguments separated by commas */
    if ( strcmp( transfer_func_type, "pgn" ) == 0 ) {
      if ( ( check=sscanf( transfer_func_content,"%[^)]",pgn_arg ) ) != 1 )
	fprintf (stderr," ERROR: network in nexus  \n");

      /* get arguments */	
      comma[0] = -1;
      j = 1;    i = 0;
      while ( pgn_arg[i] != '\0') { /* yyleng != # ofchars of networks */
	if ( pgn_arg[i] == ',' ) {
	  comma[j] = i;
	  j++;
	}
	i++;
      }
      comma[j] = i ;
      pgn_arc = j;

      /* set nets_in_nex: name, extern_from/to = FALSE */

      for (i = 0; i < pgn_arc; i++) {
	for (j = 0; j < FUNCTION_ARG_SIZE; j++) /* initialize to clean up  */ 
	  pgn_arguments[j] = ' ';
	k = 0;
	for (j = (comma[i] + 1); j < comma[i+1]; j++) {
	  pgn_arguments[k] = pgn_arg[j];
	  k++;
	}
	sscanf( pgn_arguments, "%s", pgn_arguments );	/* remove spaces */
	if (i == 0)
	  sprintf( transfer_func_content,"%s",pgn_arguments );
	else 
	  sprintf( transfer_func_content,
		  "%s,%s",transfer_func_content, pgn_arguments );
      }
      sprintf( network->class,
	      "%s-%s",transfer_func_type, transfer_func_content );
      /* fprintf( stderr,"pgn function: %s\n",network->class); */ 
      return(OK);
    } else {
      /* allow other functions */
      /* ...expect one argument */
      sscanf( transfer_func_content,"%s",transfer_func_content);
      sprintf( network->class,
	      "%s_%s",transfer_func_type, transfer_func_content );
    }	

    return(OK);
  }
  return ERROR;
}  /* set_trans_func( ) */



float
get_random( min, max )
    float min, max;
{

  return ((float) random( ) / 2.147483e+9 * (max - min) + min);

}



/*
 *  checkFilenameSuffix:  Make sure the suffix of the provided filename agrees
 *    with the current state (LOAD or BUILD).  Original suffix code: JB
 *    GL & toolkit independent.  DW 94.11.02
 */

int
checkFilenameSuffix( state, filename, buffer )
    int		state;
    char	*filename, *buffer;
{
  char		*suffixp;
  char		*good_suffix, *bad_suffix;

  strcpy( buffer, filename );
  suffixp = strrchr( buffer, '.' );

  /*
   *  If we are loading, the good suffix is actually .save, thus we
   *    switch them around; this combines two routines into one.  DW 94.08.22
   */
  bad_suffix = ".save";
  good_suffix = ".nx";

  if (state == LOAD) {
    bad_suffix = ".nx";
    good_suffix = ".save";
  }

  /*  Add good suffix if no suffix exists. */
  if (!suffixp) {
    strcat( buffer, good_suffix );
    return ERROR;
  }
    
  /*  Replace a bad suffix with a good one, if need be.  */
  else if (strcmp( suffixp, good_suffix ) != 0) {

    /*
     *  If the suffix is the opposite of what is expected (.nx, not .save,
     *  or vice versa), simply replace the entire suffix.
     */
    if (strcmp( suffixp, bad_suffix ) == 0)
      *suffixp = '\0';

    /*
     *  If the suffix is not standard, salvage it by adding ours to it.
     *  i.e.  leaf.blah will turn into leaf.blah.nx or leaf.blah.save.
     */
    strcat( buffer, good_suffix );

    return ERROR;
  }

  return OK;
}   /* checkFilenameSuffix( ) */



/*
 *  moveNets:  Move all networks.  GL & toolkit independent.  DW 94.11.03
 */

void
moveNets( axis, direction )
    char	axis;
    int		direction;
{
  extern float translation_amount;


  translate_network( axis, direction * translation_amount, FALSE );
}



/*
 *  showParameters:  Display current network's parameters.  Modified for
 *    selective clamp:  JB 94.05.27.  GL & toolkit independent.  DW 94.11.03
 */

void
showParameters( network )
    NETWORK	network;
{
  static char		clamp_state[10];
  static char		transfer_func_type[FUNCTION_ARG_SIZE];


  if (network) {
    if ((network->cells)->clamp == CLAMP_ON)
      strcpy( clamp_state, "on" );
    else if((network->cells)->clamp == CLAMP_OFF)
      strcpy( clamp_state, "off" );
    else {
#ifdef DEBUG
      printf( "clamp: %d\n" );
#endif
      strcpy( clamp_state, "selective" );
    }

    /* change historical name into conventional name */	
    sprintf( transfer_func_type, "%s", network->class );
    if (strcmp( network->class, "simple" ) == 0) 
      sprintf( transfer_func_type, "sigmoid" );
    if (strcmp( network->class, "binary" ) == 0) 
      sprintf( transfer_func_type, "step" );
    
    printf( "Network %s:\n", network->name );
    printf( "\tClamp (cell #1):     %s\n", clamp_state );
    printf( "\tActivity (cell #1):  %f\n", (network->cells)->firing_rate );
    printf( "\tThreshold (cell #1): %f\n", (network->cells)->threshold );
    printf( "\tTransfer function:   %s\n", transfer_func_type );
    printf( "\tMin.:                %f\n", network->function_min );
    printf( "\tMax.:                %f\n", network->function_max );
    printf( "\tSlope:               %f\n", network->function_slope );
    printf( "\tScale:               %f\n", network->scale );
    printf( "\tOffset:              %f\n", network->offset );
    printf( "\tDecay:               %f\n", network->decay );
    printf( "\tX Position:          %.3f\n", network->pos_x );
    printf( "\tY Position:          %.3f\n", network->pos_y );
    printf( "\t# updates per cycle: %d\n\n", network->evaluations_per_cycle );
  }
  else {
    printf( "NEXUS Internal Warning: Null Network Pointer.\n" );
    printf( "  showParameters( );\n" );
  }
}



/*
 *  DW 95.01.20  nxFopen() replaces any reads in which fopen() is used.
 *    That is, only fopen()'s of the form 'fopen( filename, "r" )' should
 *    be replaced by this.  Writes should still be opened in the current
 *    directory only (not the current _access_ directory).
 */
FILE *
nxFopen( filename )
     const char * filename;
{

#if defined(NEXUS_SGI) || defined(NEXUS_LINUX)
  FILE *retVal;
  char tmpFilename[FILENAME_MAX];


  if ((retVal = fopen( filename, "r" ))) {
    return retVal;
  }

  if (!strlen( curr_access_directory )) {
    return NULL;
  }

  strcpy( tmpFilename, curr_access_directory );
  strcat( tmpFilename, filename );

  return fopen( tmpFilename, "r" );

#else /* NEXUS_SUN */

  return fopen( filename, "r" );

#endif
}



#if 0
cycle_plot_network( count_cycles, cycles )
    int count_cycles,cycles;
{
  extern char	plot_filename[];
  
  if (count_cycles == 0) {
    cycle_plot_fp = fopen( plot_filename, "w" );
    if (cycle_plot_fp == NULL) {
      printf( "Cannot open plot file %s\n", plot_filename );
      return;
    }
  }
  
  plot_network_to_file( cycle_plot_fp );
  
  if (count_cycles == cycles - 1)
    fclose( cycle_plot_fp );
}
#endif










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
