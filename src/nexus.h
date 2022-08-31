#ifndef _NEXUS_AICH
#define _NEXUS_AICH

/*************************************************************
*                                                            *
*                            NEXUS                           *
*                                                            *
*              (c) 1990 Paul Sajda and Leif Finkel           *
*                                                            *
*                         (nexus.h)                          *
*                                                            *
*      modified for Parallel Nexus.  11/1/91 by K.Sakai      *
**************************************************************/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define NEXUS_VERSION "1.0"

/*
 *  Definitions that allow this header file to be used either with or
 *    without ANSI C features like function prototypes.  Taken from tcl.h.
 */

#undef _ANSI_ARGS_
#undef CONST
#if ((defined(__STDC__) || defined(SABER)) && !defined(NO_PROTOTYPE)) || defined(__cplusplus)
#   define _USING_PROTOTYPES_ 1
#   define _ANSI_ARGS_(x)	x
#   define CONST const
#   ifdef __cplusplus
#       define VARARGS (...)
#   else
#       define VARARGS ()
#   endif
#else
#   define _ANSI_ARGS_(x)	()
#   define CONST
#endif



/* not constant 4's anymore: DW 94.08.25 */
#define FLOAT_SIZE		sizeof(float)
#define INT_SIZE		sizeof(int)

/* Unix-specific #define's: NAME_SIZE really ought to be FILENAME_MAX, *
 *   which is defined in stdio.h (on SGI's)                            */
#define NAME_SIZE		80
#define NAME_DISPLAY_SIZE	30
#define FILE_BUFSIZE		256
#define COMMAND_SIZE		256

/* PGN function #define's: F_ARG_SIZE should not be overloaded, but is... */
#define FUNCTION_SIZE		300
#define FUNCTION_ARG_SIZE	50

#define MAXARRAY		100




/***********
 *  Flag declarations.
 ***********/


#define NO_CONNECTION		-1
#define NOT_EXIST		-1
#define ERROR			-1
#define OK			0
#ifndef NULL
#define NULL			0
#endif

#define FALSE			0
#define TRUE			1

#define OFF			0
#define ON			1

#define ACTIVITY		0
#define CONNECTIONS		1

#define ANTEROGRADE		0
#define RETROGRADE		1

#define SCATTER			0
#define BAR			1

#define RECTANGLE		0
#define ELLIPSE			1

#define NORMALIZE		0
#define DIRECT			1

#define LOAD			0
#define BUILD			1

#define CHANGE			0
#define EXAMINE			1

#define LOAD_FILES		0
#define LOAD_COUNT		1

#define LOAD_NOT_PICKED		0
#define LOAD_PICKED		1

/**
 **  Connections.
 **/
#define PGN_ANTEROGRADE		2 /* use output_list to store connections */

#define SEQUENTIAL		0
#define RANDOM			1
#define DOG			2
#define LINE			3

/**
 **  Parameter selection menu / checkbox / what-have-you.
 **/

/*
 *  Parameters section.  See clamp_run() in nexus_main.c for details. DW 940711
 *    NB: The order of these values MUST match the order in the parameter_type
 *    menu (see nexus_main.c), and MUST be consecutive integers starting at 0
 */

/*  For backward-compatibility with save-files, we have this order:  */
#define CLAMP_ON		0
#define CLAMP_OFF		1

/*
 *  When this support is finished and added, and the widget redefined to add
 *    the new button, these numbers will need to be updated.  DW 94.07.11
 *    CLAMP_SELECTIVE_ON can not be defined earlier than the completion of the
 *    coding, though, IF IT IS TO BE USED IN THE PARAMETER_TYPE WIDGET!!!
 */

#define CLAMP_SELECTIVE_ON	-1		/* added 5/25/94 J.B. */

#define SET_ACTIVITY		2
#define SET_TRANS_FUNC		3
#define SET_THRESHOLD		4
#define SET_SCALE		5
#define SET_OFFSET		6
#define SET_NUM_UPDATES		7
#define SET_DECAY		8

/**
 **  Electrode mode.  DW 940811
 **/

#define MODE_OFF	0
#define MODE_ACTIVITY	1
#define MODE_CONNECT	2
#define MODE_VIEW	4

/**
 **  Selective delay of display updates: DISP_CELL means display each cell
 **    update while DISP_NET waits for entire network to be updated.
 **    DISP_NET is default, since many systems are fast enough, and many
 **    simulations aren't that big.  DW 94.07.06
 **/

#define DISP_OFF	0
#define DISP_CELL	1
#define DISP_NET	2

/**
 **  Shall we display in grey-scale or colour?  DW 95.03.16
 **/

#define DISP_GREY	0
#define DISP_COLOUR	1

/**
 **  Cycle control of display.  DW 94.09.09
 **/

#define FIRST_CYCLE	-1
#define MIDDLE_CYCLE	0
#define LAST_CYCLE	-2


/***********
 *  Binary flags are stored in one global variable using bitwise operators.
 *  All are READ-ONLY by the program unless otherwise specified.  (If they
 *    are READ-WRITE, then the program changed their value on-the-fly.  If
 *    they are READ-ONLY, only the user can change their value, through
 *    whatever interface is provided, be it graphical or script). DW  94.08.25
 *  To Do:  in order to incorporate (possibly) more flags, this should be
 *    a pointer index table to actual flags which are created at run-time.
 ***********/

enum user_flags {
  LEARN_BP		= 0000001L, /* RW */
  LEARN_HEBB		= 0000002L,
  LEARN_HEBB_TYPE	= 0000004L,
  LEARN_INVERS		= 0000010L, /* RW */
  LEARN_RBF		= 0000020L, /* RW */
  LEARN_BATCH		= 0000040L,
  SAVE_ERR_BP		= 0000100L,
  SAVE_ERR_INVERS	= 0000200L,
  SAVE_ERR_RBF		= 0000400L,
  ACTIVITY_LOAD		= 0001000L, /* RW */
  ACTIVITY_STATE	= 0002000L,
  ACTIVITY_SWAP		= 0004000L,
  CONNECT_SHOW_TYPE	= 0010000L, /* RW */
  CONNECT_PRINT		= 0020000L,
  CONNECT_SAVE		= 0040000L,
  ACTIVITY_CYCLE	= 0100000L, /* RW */
  FILE_COMPRESSED	= 0200000L, /* RW */
  LOAD_RANDOM		= 0400000L
};
typedef unsigned long int	flag_t;
typedef int			flag_value;

#define SET_FLAG(which_flag,value) \
	((value) ? \
	 (nexus_flags |= (which_flag)) : \
	 (nexus_flags &= ~(which_flag)))

#define QUERY_FLAG(which_flag) \
	((nexus_flags & (which_flag)) != 0 ? 1 : 0)	

void	set_flag( /* flag_t which_flag, int value */ );
int	query_flag( /* flag_t which_flag */ );


/***********
 *  These are not related to flag operations.
 ***********/

#define POSITIVE		1
#define NEGATIVE		-1

/**
 **  Interrupt handling symbols.  Negative means an interrupt in processing.
 **    DW 94.08.30
 **/

#define NXI_IGNORE	0
#define NXI_STOP	-1
#define NXI_SUSPEND	-2


#define LEFT_BUTTON    1
#define MIDDLE_BUTTON  2	/* added by DW 94.06.11 for completeness */
#define RIGHT_BUTTON   4

/**
 **  Miscellaneous overused flags.
 **/

#define RESET		0
#define SET		1
#define SHOW		2

/*
 *  Replaces settings-> structure members.  We don't read in settings anymore.
 *    DW 94.07.17.  Slope added 94.07.21.
 *  Changed Default Activity Max value to 1.0 from 100.0
 *    corresponds better to most learning algorithms.  DW 94.11.29
 */

#define DEFAULT_ACTIVITY_MIN		0.0
#define DEFAULT_ACTIVITY_MAX		1.0
#define DEFAULT_ACTIVITY_SLOPE		1.0
#define DEFAULT_CONDUCTANCE_MIN		-1.0
#define DEFAULT_CONDUCTANCE_MAX		1.0

/*
 *  'max' is a function, 'MAX' is already defined, thus we have 'Max'.
 *  Likewise for 'Min'.  DW 94.08.29
 */

#define Max(a,b) ( (a) > (b) ? (a) : (b) )
#define Min(a,b) ( (a) < (b) ? (a) : (b) )

/*
 *  Specify precision in activity output file:  should match
 *    activity_output_precision table.
 */

#define ONE_DECIMAL	0
#define THREE_DECIMAL	1
#define SIX_DECIMAL	2

/*
 *  A little debugging code... DW 94.10.31
 */

#ifdef DEBUG
#define DEBUG_LINE(num) printf("debug %d\n",(num))
#else /* DEBUG */
#define DEBUG_LINE(num)
#endif /* DEBUG */



/*****************************************************************************
 *  Structure definitions.
 *****************************************************************************/

struct nexus {				
  int id;
  char name[NAME_SIZE];		/* local nexus name = "self"	*/
  char file_write_dir[NAME_SIZE];
  struct nexus *next;
};
typedef struct nexus	ELEMENT_NEXUS;
typedef ELEMENT_NEXUS *	NEXUS;
#define SIZE_NEXUS	sizeof( ELEMENT_NEXUS )


struct network {
  int id;
  char name[NAME_SIZE];
  float pos_x;
  float pos_y;
  float cell_size;
  float name_size_w;
  float name_size_h;
  float decay;
  float function_slope;		/* for sigmoid transfer function */
  float function_min;		/* min & max of output, subject to
				   scale and offset */
  float function_max;
  float scale;			/* for output scaling */
  float offset;			/* for output offset */
  
  char class[NAME_SIZE];	/* transfer function executed by cell */
  int number_cells;
  int dim_x;			/* network dimensions in space */
  int dim_y; 
  int evaluations_per_cycle;	/* # times to update network per cycle */
  struct extern_connection *extern_connections;
  struct cell *cells;
  struct network *next;
};
typedef struct network		ELEMENT_NETWORK;
typedef ELEMENT_NETWORK *	NETWORK;
#define SIZE_NETWORK		sizeof( ELEMENT_NETWORK )


struct extern_connection {
  int extern_to;		/* a flag for external connection to	*/
  int extern_from;		/* a flag for external connection fro	*/
  int extern_to_times;		/* # of times written			*/
  int extern_from_times;	/* # of times read			*/
  char extern_to_dir[NAME_SIZE]; /* a directory to write a file		*/
  char extern_from_dir[NAME_SIZE]; /* a directory to write a file	*/
  char extern_from_net[NAME_SIZE]; /* name of external network		*/
  char extern_from_nex[NAME_SIZE]; /* name of external nexus		*/
  char extern_from_opt[NAME_SIZE]; /* an option for external data read	*/
};
typedef struct extern_connection	ELEMENT_EXTERN_CONNECTION;
typedef ELEMENT_EXTERN_CONNECTION *	EXTERN_CONNECTION;
#define SIZE_EXTERN_CONNECTION		sizeof( ELEMENT_EXTERN_CONNECTION )


struct cell {
  int id;
  int net_id;
  float voltage;
  float threshold;
  float firing_rate;
  float firing_rate_old;
  int clamp; 						/* clamp flag */
  int net_connections;
  int number_connections;
  int number_outputs;  	 				/* for pgn cells */

  struct connection_parameters *connect_parameters;
  struct connection *connect_list;

  struct connection *output_list;			/* for vn cells */
  struct connection_uncompressed *output_list_un;	/* for vn cells */

  struct connection_uncompressed *connect_list_un;
  struct connect_specs *specs;
};
typedef struct cell	ELEMENT_CELL;
typedef ELEMENT_CELL *	CELL;
#define SIZE_CELL	sizeof( ELEMENT_CELL )


struct connection_uncompressed {
  struct cell *input_cell;
  float conductance;
  struct connection_uncompressed *next;
};
typedef struct connection_uncompressed	ELEMENT_CONNECTION_UN;
typedef ELEMENT_CONNECTION_UN *		CONNECTION_UN;
#define SIZE_CONNECTION_UN		sizeof( ELEMENT_CONNECTION_UN )


struct connection {
  struct cell *input_cell;
  float conductance;
};
typedef struct connection	ELEMENT_CONNECTION;
typedef ELEMENT_CONNECTION *	CONNECTION;
#define SIZE_CONNECTION		sizeof( ELEMENT_CONNECTION )


struct connection_parameters {	/* specify connectivity before building */
  int type;			/* anterograde or retrograde */
  int mask_type;		/* rect. or elliptical mask */
  int projection_type;		/* normalize size of networks before
				   topographic connections or use
				   direct projection */
  char name[NAME_SIZE];
  int major_dim;
  int minor_dim; 
  int angle;
  char function[FUNCTION_SIZE];
  int shift_x;
  int shift_y;
  int feedback;			/* should direct feedback be permited */
  struct connection_parameters *next;
};
typedef struct connection_parameters	ELEMENT_PARAMETERS;
typedef ELEMENT_PARAMETERS *		PARAMETERS;
#define SIZE_PARAMETERS			sizeof( ELEMENT_PARAMETERS )


struct connect_specs {
  int type;
  int mask_type;		/*  rect. or elliptical mask 	*/
  int projection_type;		/*  normalize size of networks before topo-
				 *    graphic connections or use direct pro-
				 *    jection
				 */
  int connect_id;		/* id to differentiate between diff. connex */
  int id;			/* id of network connected to 	*/
  int max_width;
  int max_height;
  int center_x;
  int center_y;
  int major_dim;
  int minor_dim;
  int angle;
  int feedback;			/* should direct feedback be permited */
  int plasticity;		/* flag indicates if connection is plastic
				   - for hebbian learning*/
  struct connect_specs *next;
};
typedef struct connect_specs	ELEMENT_SPECS;
typedef ELEMENT_SPECS *		SPECS;
#define SIZE_SPECS		sizeof( ELEMENT_SPECS )


/*
 * array used to hold connection strengths is connection
 * functions are to be read from a file 
 */

struct weight_array {
  char name[NAME_SIZE];
  float *list_of_weights;
  struct weight_array *next;
};
typedef struct weight_array	ELEMENT_WEIGHT_ARRAY;
typedef ELEMENT_WEIGHT_ARRAY *	WEIGHT_ARRAY;
#define SIZE_WEIGHT_ARRAY	sizeof( ELEMENT_WEIGHT_ARRAY )


/*
 *    DW 94.07.21  all references to old settings struct removed from source.
 *    DW 94.09  for backwards compatibility with old PGN functions, settings
 *      struct is back...
 */

struct settings {
  /* name of default file to load */
  char filename[NAME_SIZE];

  /* name of file to save simulator auto notes */
  char filename_notes[NAME_SIZE];

  /* max, min and division of firing rate */
  float firing_rate_max;
  float firing_rate_min;

  /* for computation and display */
  float firing_rate_div;

  /* slope of sigmoidal function */
  float firing_rate_slope;

  /* max, min and division of conductance - for display  */
  float conduct_max;
  float conduct_min;

  float conduct_div;

  /* initial placement of simulation structure in world coordinates */
  float pos_x;
  float pos_y;
  float pos_z;

  /* width and height of network name display */
  float nettext_w;
  float nettext_h;

  float max_clamp;
};
typedef struct settings ELEMENT_SETTINGS;
typedef ELEMENT_SETTINGS *SETTINGS;


/*****************************************************************************
 *  Function Declarations.
 *****************************************************************************/

extern float	get_random _ANSI_ARGS_(( float, float ));
extern void	init_random_number_gen ( );
extern int	introduction ( );
extern void	load_activity_file _ANSI_ARGS_(( FILE *, NETWORK ));
extern FILE *	nxFopen _ANSI_ARGS_(( const char * ));
extern void	set_network _ANSI_ARGS_(( NETWORK, float, float ));
extern void	update_network_lists _ANSI_ARGS_(( void ));
extern int	cleanup _ANSI_ARGS_(( void ));
extern void	showParameters _ANSI_ARGS_(( NETWORK ));
extern void	do_memory _ANSI_ARGS_(( void ));
extern void	set_clamp _ANSI_ARGS_(( NETWORK, int, float ));
extern void	printConnections _ANSI_ARGS_(( int, CELL ));
extern int	saveConnections _ANSI_ARGS_(( int, CELL ));
extern NETWORK	get_network_id _ANSI_ARGS_(( int ));
extern NETWORK	get_network_name _ANSI_ARGS_(( char * ));
extern int	check_for_connection _ANSI_ARGS_(( CELL, NETWORK, int ));
extern int	get_connection_color _ANSI_ARGS_(( CELL, NETWORK, int ));

/* The following is provided by toolkit-dependent modules. */

extern int	check_interrupt _ANSI_ARGS_(( void ));


/*****************************************************************************
 *  Global Variable Declarations.
 *****************************************************************************/

extern NETWORK		network_head;

/* lookup table: net_id -> NETWORK */
extern NETWORK *	network_xref_list;

/* from yylex() (nexus_lex_build.l) */
extern int		number_networks;

extern int		nxDebug;

extern int		do_exit;  /* SUN version */
extern int		graphics_off;
extern int		stop_simulation;
extern int		simulation_state;
extern int		curr_electrode;
extern int		explore_variables;
extern int		simulate_type;
extern int		display_type;
extern int		number_cycles;
extern int		random_cells;

extern float		activity_display_range_min, activity_display_range_max,
			conduct_compute_range_min, conduct_compute_range_max,
			conduct_display_range_min, conduct_display_range_max;
extern int		activity_display_depth, conduct_display_depth;

extern SETTINGS		setting;
extern flag_t		nexus_flags; /* used in the SET_FLAG macro. */

extern char		activity_output_filename[];
extern FILE *		activity_output_fp;
extern char		activity_output_network_name[];
extern int		activity_output_precision;

extern char		send_connections_filename[];
extern float		change_dimension_amount;
extern float		translation_amount;
extern int		setActivityPercentage;

extern char		activity_filename[];

extern char		edit_connect_network[NAME_SIZE];
extern char		edit_connect_function[FUNCTION_SIZE];
extern int 		edit_connect_current_id;

extern char		activity_network[NAME_SIZE];
extern int		activity_cycle_record;
extern int		activity_precision;

extern char		view_network[NAME_SIZE];

extern char		parameter_network[];

#endif /* !def _NEXUS_AICH */










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
