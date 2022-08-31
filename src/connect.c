/*************************************************************
*							     *
*			     NEXUS			     *
*							     *
*	      (c) 1990 Paul Sajda and Leif Finkel	     *
*							     *
*			(nexus_connect.c)		     *
*							     *
*	   Modified for PNEXUS    Oct. 1991  by Ko Sakai     *
**************************************************************/

#include <stdlib.h>

#include "nexus.h"
#include "nexus_pgn_includes.h"
#include "memory.h"
#include "connect.h"

#define PI		3.14159
#define DEG_TO_RAD	2.0 * 3.14159 / 360.0


static void	connect_networks _ANSI_ARGS_(( void ));
static SPECS	connect_cell _ANSI_ARGS_(( CELL, NETWORK, int, int ));
static float	get_conductance _ANSI_ARGS_(( int, int, int, int, int,
					     int, int, char * ));
static void	compress_connections _ANSI_ARGS_(( NETWORK ));
static void	get_cells _ANSI_ARGS_(( CELL, int, char * ));
static void	copy_connections _ANSI_ARGS_(( CONNECTION, CONNECTION_UN ));
static char *	convert_connect_function_new _ANSI_ARGS_(( char * ));

int get_start_cell(int x, int y, int w, int h, int dim_x, int dim_y );
int get_end_cell(int x, int y, int w, int h, int dim_x, int dim_y );
int is_connected(CELL cell, CELL input_cell, int center_x, int center_y,
		 int cell_x, int cell_y, int major_dim, int minor_dim,
		 int angle, int mask_type, int feedback );
int get_number_connections(CONNECTION_UN list);




/***********
 *  nexus_connect.c
 ***********/

/*
 *  Print connections of cell to screen.
 */

void
printConnections( type, cell )
    int type;
    CELL cell;
{
  register int		count;
  CONNECTION		connection;

  if (type == ANTEROGRADE)
    return;

  printf( "\t%d connection%s:\n",
	 cell->number_connections,
	 (cell->number_connections == 1 ? "" : "s") );
  if (cell->number_connections)
    for (count = 0; count < cell->number_connections; count++) {
      connection = cell->connect_list + count;
      printf( "\t\tCell %d,\tNetwork %s \t%f\n",
	     connection->input_cell->id,
	     get_name_of_network( connection->input_cell->net_id ),
	     connection->conductance );
    }
  else
    printf( "\t    None.\n" );
};



/*
 *  Save connections of cell to file.
 */

int
saveConnections( type, cell )
    int type;
    CELL cell;
{
  CONNECTION		connection;
  FILE			*fp;
  extern char		send_connections_filename[];
  char			net_name[NAME_SIZE];
  register int		count;


  if (type == ANTEROGRADE)
    return OK;

  if (!(fp = fopen( send_connections_filename, "w" ))) {
    printf( "ERROR: unable to open file '%s'\n",
	   send_connections_filename );
    return ERROR;
  }

  if (cell->number_connections) {
    for (count = 0; count < cell->number_connections; count++) {
      connection = cell->connect_list + count;
      if (count == 0 || strcmp( net_name,
		get_name_of_network( connection->input_cell->net_id ) ) != 0) {
	strcpy( net_name,
	       get_name_of_network( connection->input_cell->net_id ) );
	fprintf( fp, "\nNetwork %s from Network %s\n\n",
		get_name_of_network( cell->net_id ), net_name );
      }

      fprintf( fp, "%d %d %f\n",
	      ((connection->input_cell->id - 1) %
	       get_network_id( (connection->input_cell)->net_id )->dim_x),
	      ((connection->input_cell->id - 1) /
	       get_network_id( (connection->input_cell)->net_id )->dim_x),
	      connection->conductance );
    }
  }
  else
    fprintf( fp, "No Connections to Network %s.\n",
	    get_name_of_network( cell->net_id ) );

  fclose( fp );

  return OK;
};



/* Scan networks and connect cells topographically */

static void
connect_networks( )
{
  register CELL		cell;
  register NETWORK	net_head;
  register SPECS	check_spec;
  register int		pos_x, pos_y, count;

  
  net_head = network_head;
  while (net_head != NULL) {
    if ( (net_head->extern_connections)->extern_from == TRUE ) { 
      /*
       * since dummy_ networks are set cell->connect_parameters = NULL 
       * in network_build, it'll be skipped by both connect_cell() and free*
       */
      printf("\nNetwork %s w/ extern_from connection.\n", net_head->name);
    }
    else { 

      cell = net_head->cells;
      for (count = 0; count < net_head->number_cells; count++) {
	pos_x = cell->id - ((cell->id - 1) / (net_head->dim_x)
			    * net_head->dim_x);
	pos_y = ((cell->id - 1) / net_head->dim_x) + 1;
	check_spec = connect_cell( cell, net_head, pos_x, pos_y );
	cell->specs = check_spec;
	cell = cell + 1;
      }

      cell = net_head->cells;
      for (count = 0; count < net_head->number_cells; count++) {
	free_parameters( cell->connect_parameters );
	cell->connect_parameters = NULL;
	cell = cell + 1;
      }
      printf("Network %s connected.\n", net_head->name);

    } 
    net_head = net_head->next;
  }
}  /* connect_networks( ) */



/* Connect individual cells */

static SPECS 
connect_cell( cell_head, net_head, x, y )
    CELL	cell_head;
    NETWORK	net_head;
    int		x, y;
{
  register CELL            cell;
  register PARAMETERS      para_head;
  register CONNECTION_UN   old_connection_list_un;
  register SPECS           head_specs, old_head_specs;
  register NETWORK         find_head;
  register int             max_height, max_width;
  register int             start_cell, end_cell, cell_num, count;
  register int             center_x = 0, center_y = 0;
  register int             cell_x, cell_y, connect_id = 1;


  if ((para_head = cell_head->connect_parameters) == NULL)
    return NULL;

  while (connect_id <= cell_head->net_connections) {

    find_head = network_head;
    while (find_head != NULL
	   && strcmp( find_head->name, para_head->name ) != 0)
      find_head = find_head->next;

    if (find_head == NULL) {
      printf("ERROR - unknown connection network %s - connect_cell()\n",
	     para_head->name);
      exit(0);
    }
    cell = find_head->cells;
    


    /**********************************************************************
     *  Determine center of receptive field, according to the projection
     *    type.
     **********************************************************************/

    if (para_head->projection_type == NORMALIZE) {
      /* normalize network dimensions for topological connections */
      center_x = (int) (x * ((float) find_head->dim_x /
			     (float) net_head->dim_x))
	+ para_head->shift_x;

      center_y = (int) (y * (float) find_head->dim_y /
			(float) net_head->dim_y)
	+ para_head->shift_y;

    }

    if (para_head->projection_type == DIRECT) {
      center_x = (int) (x)	/* use a direct 1-1 projection */
	+ para_head->shift_x;
      center_y = (int) (y)
	+ para_head->shift_y;
    }

#if 0
    fprintf( stderr, "x:%d,y:%d,s_x:%d,s_y:%d,",
	    x, y, para_head->shift_x, para_head->shift_y );
#endif



    /**********************************************************************
     *  Determine size of receptive field, according to the mask type
     *    and the given dimensions.
     **********************************************************************/

    if (para_head->mask_type == ELLIPSE) {
      max_height = (int) ((((float) para_head->major_dim)
			   * fabs(sin((float) para_head->angle
				      * DEG_TO_RAD))) + .5);
      max_width = (int) ((((float) para_head->minor_dim)
			  * fabs(sin((float) para_head->angle
				     * DEG_TO_RAD))) + .5);
      
      if ((int) para_head->major_dim
	  * fabs(cos((float) para_head->angle * DEG_TO_RAD)) > max_width)
	/*
	 * max_width and max_height specify area to
	 * sample for connectivity
	 */
	max_width = (int) (((float) para_head->major_dim
			    * fabs(cos((float) para_head->angle
				       * DEG_TO_RAD))) + .5);

      if ((int) ((float) para_head->minor_dim
		 * fabs(cos((float) para_head->angle * DEG_TO_RAD)))
	  > max_height)
	max_height = (int) (((float) para_head->minor_dim
			     * fabs(cos((float) para_head->angle
					* DEG_TO_RAD))) + .5);
      
    }
    else {  /* mask_type == RECTANGLE */
      max_height = (int) (((float) para_head->minor_dim)
			  * fabs(cos((float) para_head->angle * DEG_TO_RAD))
			  + ((float) para_head->major_dim)
			  * fabs(sin((float) para_head->angle * DEG_TO_RAD)));
      
      max_width = (int) (((float) para_head->major_dim)
			 * fabs(cos((float) para_head->angle * DEG_TO_RAD))
			 + ((float) para_head->minor_dim)
			 * fabs(sin((float) para_head->angle * DEG_TO_RAD)));
    }

#if 0
    fprintf( stderr, "m_w:%d,m_h:%d,d_x:%d,d_y:%d",
	    max_width, max_height, find_head->dim_x, find_head->dim_y );
#endif

    start_cell = get_start_cell(center_x, center_y, max_width, max_height,
				find_head->dim_x, find_head->dim_y);
    
    end_cell = get_end_cell(center_x, center_y, max_width, max_height,
			    find_head->dim_x, find_head->dim_y);
    
    cell_num = start_cell;
    count = (cell_num - 1) / find_head->dim_x + 1;

    /*
     * routine to limit sampling to only within a rectangle whose
     * area is defined by max_width and max_height
     */
    
    if ((center_x - (max_width / 2 + 1)) <= find_head->dim_x)
      while ((cell_num <= end_cell)) {

	if (cell_num > (count * find_head->dim_x) ||
	    cell_num > ((find_head->dim_x * (count - 1))
			+ (max_width / 2 + 1) + center_x)) {
	  cell_num = count * find_head->dim_x
	    + (start_cell - ((start_cell - 1) / find_head->dim_x)
	       * find_head->dim_x);
	  count = (cell_num - 1) / find_head->dim_x + 1;
	}

	if (cell_num > end_cell)
	  break;

	cell = cell + (cell_num - 1);
	cell_y = (cell_num - 1) / find_head->dim_x + 1;
	cell_x = cell_num - ((cell_num - 1) / find_head->dim_x)
	  * find_head->dim_x;
	if (is_connected(cell, cell_head, center_x, center_y,
			 cell_x, cell_y, para_head->major_dim,
			 para_head->minor_dim, para_head->angle,
			 para_head->mask_type,
			 para_head->feedback) == TRUE) {

#if 0
	  fprintf( stderr, "." );
#endif

	  if (para_head->type == ANTEROGRADE
	      || para_head->type == PGN_ANTEROGRADE) {

	    if (para_head->type == PGN_ANTEROGRADE) {

	      old_connection_list_un = cell_head->output_list_un;
	      cell_head->output_list_un =
		(CONNECTION_UN) nxMalloc( SIZE_CONNECTION_UN );
	      (cell_head->output_list_un)->input_cell = cell;
	      (cell_head->output_list_un)->conductance =
		get_conductance(center_x, center_y, cell_x, cell_y,
				para_head->angle,
				para_head->major_dim,
				para_head->minor_dim,
				para_head->function);
	      (cell_head->output_list_un)->next = old_connection_list_un;
	      
	    }
	    else {		/* ANTEROGRADE */
	      old_connection_list_un = cell->connect_list_un;
	      cell->connect_list_un =
		(CONNECTION_UN) nxMalloc( SIZE_CONNECTION_UN );
	      (cell->connect_list_un)->input_cell = cell_head;
	      (cell->connect_list_un)->conductance =
		get_conductance(center_x, center_y, cell_x, cell_y,
				para_head->angle,
				para_head->major_dim,
				para_head->minor_dim,
				para_head->function);
	      (cell->connect_list_un)->next = old_connection_list_un;
	    }
	  }
	  else {		/* RETROGRADE */
	    old_connection_list_un = cell_head->connect_list_un;
	    cell_head->connect_list_un =
		(CONNECTION_UN) nxMalloc( SIZE_CONNECTION_UN );
	    (cell_head->connect_list_un)->input_cell = cell;
	    (cell_head->connect_list_un)->conductance =
	      get_conductance(center_x, center_y, cell_x, cell_y,
			      para_head->angle,
			      para_head->major_dim,
			      para_head->minor_dim,
			      para_head->function);
	    (cell_head->connect_list_un)->next = old_connection_list_un;
	    
	  }
	  
	} /* is_connected? */

	cell = cell - (cell_num - 1);
	cell_num++;

      }	/* while (cell_num < end_cell) */

    
    /* specs structure for partial search */
    old_head_specs = cell_head->specs;
    head_specs = (SPECS) nxMalloc( SIZE_SPECS );
    head_specs->mask_type = para_head->mask_type;
    head_specs->type = para_head->type;
    head_specs->projection_type = para_head->projection_type;
    head_specs->id = find_head->id;
    head_specs->connect_id = connect_id;
    head_specs->max_width = max_width;
    head_specs->max_height = max_height;
    head_specs->center_x = center_x;
    head_specs->center_y = center_y;
    head_specs->major_dim = para_head->major_dim;
    head_specs->minor_dim = para_head->minor_dim;
    head_specs->angle = para_head->angle;
    head_specs->feedback = para_head->feedback;
    head_specs->next = old_head_specs;
    cell_head->specs = head_specs;
    
    
    para_head = para_head->next;
    connect_id++;
  }
  
#if 0
  fprintf( stderr, "\n" );
#endif

  return (cell_head->specs);
}  /* connect_cell( ) */



/*
 *  Determine if connection should be made.
 */

int
is_connected( cell, input_cell, center_x, center_y,
	     cell_x, cell_y, major_dim, minor_dim,
	     angle, mask_type, feedback )
    register int	center_x, center_y, cell_x, cell_y,
    			major_dim, minor_dim, angle, mask_type, feedback;
    register CELL	cell, input_cell;
{
  float x, y, x1, y1, a, b;
  float ans, mj, mn;
  float	fcenter_x, fcenter_y;

  
  if (cell == input_cell) {
    if (feedback == TRUE)
      return (TRUE);
    else
      return (FALSE);
  }
  
  /*
   *  For even number connection field , move center by +.5
   *  DW 94.12.05  BUG FIX:  remainder() function --> modulus operator so
   *    that it works on SGI as well.
   */
  if (major_dim % 2 == 0)
    fcenter_x = (float) center_x + 0.5;
  else
    fcenter_x = (float) center_x;
  
  if (minor_dim % 2 == 0)
    fcenter_y = (float) center_y + 0.5;
  else
    fcenter_y = (float) center_y;
  
  x1 = (float) cell_x - fcenter_x;
  y1 = (float) cell_y - fcenter_y;
  a = cos(((float) angle) * DEG_TO_RAD);
  b = sin(((float) angle) * DEG_TO_RAD);
  x = a * x1 - b * y1;
  y = b * x1 + a * y1;
  
  if (mask_type == ELLIPSE) {
    mj = ((float) major_dim) / 2.0;
    mn = ((float) minor_dim) / 2.0;
    ans = (x * x) / (mj * mj) + (y * y) / (mn * mn);
    if (ans <= 1.0)
      return (TRUE);
    else
      return (FALSE);
    
  } else {
    if (x <= ((float) major_dim) / 2.0 && x >= -((float) major_dim) / 2.0) {
      if (y <= ((float) minor_dim) / 2.0 && y >= -((float) minor_dim) / 2.0)
	return (TRUE);
    }
    return (FALSE);
  }
}



/* Routine to turn connection list from malloc() linked list to 
 * calloc() contiguous list.  Saves 4 bytes per connection and increases
 * speed by reducing disk swapping. 
 */

static void
compress_connections( network )
    NETWORK	network;
{
  if (network) {
    get_cells( network->cells, network->number_cells, network->class );
    printf( "Network %s compressed.\n", network->name );
    compress_connections( network->next );
  }
}



/*
 *  Connect and compress simulation.
 */

void
connect_sim( )
{
  printf( "\nConnecting simulation . . .\n" );
  connect_networks( );
  printf( "\nCompressing simulation . . .\n" );
  compress_connections( network_head );
}



static float 
get_conductance( center_x, center_y, cell_x, cell_y, angle,
		major_dim, minor_dim, func )
    int		center_x, center_y, cell_x, cell_y, angle, major_dim,
    		minor_dim;
    char	*func;
{
  float		get_random _ANSI_ARGS_(( float, float ));
  float		p1, p2, p3, p4;
  float		x1, y1, a, b, x, y;
  float		max, min;
  float		sig_e, sig_i, scale_e, scale_i;
  float		constant;
  float		gab_period, gab_orient, gab_rspread, gab_tspread,
		gab_rotate, gab_scale;
  double	wx, wy, rx, tx, sigmar, sigmat, wndw;
  float		*ptr_weight_array, *find_weight_array();
  float		fcenter_x, fcenter_y;
  int		even_dim_x, even_dim_y;

  char		function[FUNCTION_SIZE], func_name[10];
  char		filename[20];



  /*
   *  For even number connection field , move center by +.5
   *  DW 94.12.05  BUG FIX:  remainder() function --> modulus operator so
   *    that it works on SGI as well.
   */

  if (major_dim % 2 == 0) {
    fcenter_x = (float) center_x + 0.5;
    even_dim_x = TRUE;
  } else {
    fcenter_x = (float) center_x;
    even_dim_x = FALSE;
  }
  if (minor_dim % 2 == 0) {
    fcenter_y = (float) center_y + 0.5;
    even_dim_y = TRUE;
  } else {
    fcenter_y = (float) center_y;
    even_dim_y = FALSE;
  }
  x1 = (float) cell_x - fcenter_x;
  y1 = (float) cell_y - fcenter_y;
  a = cos((-(float) angle) * DEG_TO_RAD);
  b = sin((-(float) angle) * DEG_TO_RAD);
  x = a * x1 + b * y1;
  y = a * y1 - b * x1;
  
  strcpy(function, func);
  sscanf(function, "%[^_] %*c", func_name);
  
  /* printf( "# connect:  func name <%s>\n",func_name ); */
  /*
   * connection matrix loaded from a files currently does not support
   * RF rotatation
   */
  
  if (strcmp(func_name, "file") == 0) {
    sscanf(function, "%*[^_] %*c %s", filename);
    ptr_weight_array = find_weight_array(filename);
    
    /* for even number connection field, move center by +1.0 */
    if ( even_dim_x == TRUE ) {
      fcenter_x = (float) center_x + 1.0;	
      x1 = (float) cell_x - fcenter_x;
    }
    if ( even_dim_y == TRUE ) {
      fcenter_y = (float) center_y + 1.0;
      y1 = (float) cell_y - fcenter_y;
    }
    return (*(ptr_weight_array + ((int)y1 + (int) (minor_dim / 2))
	      * major_dim + (int)x1 + (int) (major_dim / 2)));
  }
  if (strcmp(func_name, "rand") == 0) {
    sscanf(function, "%*[^_] %*c %*[^_] %*c %f %*c %*[^_]  %*c %f",
	   &max, &min);
    
    return (get_random(min, max));
  }
  if (strcmp(func_name, "dog") == 0) {
    sscanf(function,
	   "%*[^_] %*c %*[^_] %*c %f %*c %*[^_]  %*c %f %*c %*[^_]  %*c %f %*c  %*[^_]  %*c %f",
	   &sig_e, &scale_e, &sig_i, &scale_i);
    p1 = 1.0 / sig_e * exp(-1.0 * pow((float) (x), 2.0) /
			   (2.0 * pow(sig_e, 2.0)));
    p2 = 1.0 / sig_i * exp(-1.0 * pow((float) (x), 2.0) /
			   (2.0 * pow(sig_i, 2.0)));
    p3 = 1.0 / sig_e * exp(-1.0 * pow((float) (y), 2.0) /
			   (2.0 * pow(sig_e, 2.0)));
    p4 = 1.0 / sig_i * exp(-1.0 * pow((float) (y), 2.0) /
			   (2.0 * pow(sig_i, 2.0)));
    
    return ((1.0 / 2.0 * PI) * (scale_e * p1 * p3
				- scale_i * p2 * p4));
  }
  if (strcmp(func_name, "line") == 0) {
    sscanf(function, "%*[^_] %*c %*[^_] %*c %f %*c %*[^_]  %*c %f %*c %*[^_]  %*c %f %*c %*[^_]  %*c %f",
	   &sig_e, &scale_e, &sig_i, &scale_i);
    p1 = 1.0 / sig_e * exp(-1.0 * pow((float) (x), 2.0) /
			   (2.0 * pow(sig_e, 2.0)));
    p2 = 1.0 / sig_i * exp(-1.0 * pow((float) (x), 2.0) /
			   (2.0 * pow(sig_i, 2.0)));
    
    return ((1.0 / sqrt(2.0 * PI))
	    * (scale_e * p1 - scale_i * p2));
  }
  if (strcmp(func_name, "exp") == 0) {
    sscanf(function, "%*[^_] %*c %*[^_] %*c %f %*c %*[^_]  %*c %f %*c %*[^_]  %*c %f",
	   &max, &min, &sig_e);
    return ((max - min) * exp(sqrt(x * x + y * y) / sig_e) + min);
  }
  
  if (strcmp(func_name, "const") == 0) {
    sscanf(function, "%*[^_] %*c %f", &constant);
    return ( constant);
  }

  if((strcmp(func_name, "gaborEven") == 0) || (strcmp(func_name, "gaborOdd") == 0)) {
    sscanf(function, "%*[^_] %*c %*[^_] %*c %f %*c %*[^_]  %*c %f %*c %*[^_]  %*c %f %*c %*[^_]  %*c %f %*c %*[^_]  %*c %f %*c %*[^_]  %*c %f",
	   &gab_period, &gab_orient, &gab_rspread, &gab_tspread, &gab_rotate, &gab_scale);

    gab_orient =	 (gab_orient - 90) * DEG_TO_RAD;
    gab_rotate =	(gab_rotate - 90) * DEG_TO_RAD;

    wx =		cos( gab_orient );
    wy =		sin( gab_orient );

    rx =		cos( gab_rotate ) * x1 + sin( gab_rotate ) * y1;
    tx =		-sin( gab_rotate ) * x1 + cos( gab_rotate ) * y1;

    sigmar =		gab_rspread / 2;
    sigmat =		gab_tspread / 2;

    wndw = gab_scale / (2.0*PI) * exp( -.5 * pow( ( fabs(rx/sigmar) ),(double)2.0) -.5 * pow( ( fabs(tx/sigmat) ),(double)2.0));

    if (strcmp( func_name, "gaborEven" ) == 0)
      return ((float) (cos( 2.0 * PI / gab_period * (x1 * cos( gab_orient ) + y1 * sin(gab_orient)) ) * wndw));
    else
      return ((float) (sin( 2.0 * PI / gab_period * (x1 * cos( gab_orient ) + y1 * sin(gab_orient)) ) * wndw));
  }
  else {
    printf("ERROR - unknown connection function '%s'\n",func_name);
    return (HUGE);
  }

}



/* get starting and ending cell number of sampling area */

int
get_start_cell( x, y, w, h, dim_x, dim_y )
    register int	x, y,
			w, h,
			dim_x, dim_y;
{
  register int	a = Max( x - w/2 - 1, 1 ),
		b = Max( y - h/2, 1 ) - 1;

  return (a + b * dim_x);
}



int
get_end_cell( x, y, w, h, dim_x, dim_y )
    register int	x, y,
			w, h,
			dim_x, dim_y;
{
  register int	a = Min( x + w/2 + 1, dim_x ),
	  	b = Min( y + h/2, dim_y );

  if((a + b * dim_x) >= (dim_x * dim_y))
    return (dim_x * dim_y);
  else
    return (a + b * dim_x);
}



/*
 *  get_cells( head of cell list, number of cells, class of network )
 *  Bug Fix: handle num_connections == 0 case.  DW 94.11.14
 */
static void
get_cells( head, number, class )
    CELL	head;
    int		number;
    char *	class;
{
  int		count, num_connections;



  /*
   *  Loop through each cell of the network.
   */
  for (count = 0; count < number; count++) {

    head->connect_list = NULL;
    head->output_list = NULL;

    head->number_connections = num_connections =
      get_number_connections( head->connect_list_un );

    if (num_connections) {
      head->connect_list =
	(CONNECTION) nxCalloc( num_connections, SIZE_CONNECTION );
      copy_connections( head->connect_list, head->connect_list_un );
      head->connect_list_un = NULL;
    }

    /*
     *  If this is a PGN network, set up a section for the function output.
     */
    
    head->number_outputs = num_connections =
      get_number_connections( head->output_list_un );

    if (num_connections && class[0] == 'p' && class[1] == 'g') {
      head->output_list =
	(CONNECTION) nxCalloc( num_connections, SIZE_CONNECTION );
      copy_connections( head->output_list, head->output_list_un );
      head->output_list_un = NULL;
    }

    head++;
  }
}



int
get_number_connections( list )
    CONNECTION_UN list;
{
  int count = 0;

  while (list) {
    count++;
    list = list->next;
  }

  return count;
}
  


/*
 *  Copy data from malloc() linked list to calloc array.
 *  NOTE: after this routine returns, the pointer passed to list2 is
 *    NO LONGER VALID.  It should thus be set to NULL to avoid confusion.
 */

static void
copy_connections( list1, list2 )
    CONNECTION		list1;
    CONNECTION_UN	list2;
{
  CONNECTION_UN		list2_old;



  while (list2 != NULL) {
    list1->input_cell	= list2->input_cell;
    list1->conductance	= list2->conductance;
    list2_old		= list2->next;
    free( list2 );
    list2		= list2_old;
    list1++;
  }
}



/*
 *  Returns pointer to correct weight array which was created from a file.
 */

float *find_weight_array(name)
    char *name;
{
  extern WEIGHT_ARRAY weights_from_files;
  WEIGHT_ARRAY    head;

  head = weights_from_files;
  while (head != NULL) {
    if (strcmp(head->name, name) == 0)
      return (head->list_of_weights);
    head = head->next;
  }

  return NULL;
}



static char *
convert_connect_function_new( function_new )
    char *	function_new;
{
  int		check;
  char		weight_type[FUNCTION_ARG_SIZE];
  char		weight_content[120];
  static char	function_return[120];
  float		contents_value[10];
  

  check = sscanf( function_new, "%[^(] %*[(] %[^)] %*[)]",
		 weight_type, weight_content);
  sscanf( weight_type, "%s", weight_type );	/* to remove spaces */

  if (check != 2)
    return "_ERROR: weight function invalid";

  
  /* file function */
    if (strcmp( weight_type, "file" ) == 0) {
    sprintf( function_return, "%s_%s",
	    weight_type, weight_content );
    return function_return;
  }
  
  
  /* const function */
  if ( strcmp( weight_type, "const" ) == 0 ) {
    check = sscanf( weight_content, "%f",
		   &contents_value[1] );
    if ( check == 1 ){
      sprintf (function_return, "%s_%.3f",
	       weight_type, contents_value[1] );
      return function_return;
      
    }
    else {
      return "_ERROR: invalid arguments of weight function";
    }
  }
  
  /* line function */
  if ( strcmp( weight_type, "line" ) == 0 ) {
    check = sscanf( weight_content, "%f , %f , %f , %f",
		   &contents_value[1], &contents_value[2],
		   &contents_value[3], &contents_value[4] )
      ;
    if ( check == 4 ){
      sprintf (function_return,
	       "%s_sige_%.3f_scalee_%.3f_sigi_%.3f_scalei_%.3f",
	       weight_type, contents_value[1], contents_value[2], 
	       contents_value[3], contents_value[4] );
      return function_return;
      
    }
    else{
      return "_ERROR: invalid arguments of weight function";
    }
  }
  
  /* dog function */
  if ( strcmp( weight_type, "dog" ) == 0 ) {
    check = sscanf( weight_content, "%f , %f , %f , %f",
		   &contents_value[1], &contents_value[2], &contents_value[3], 
		   &contents_value[4] );
    if ( check == 4 ){
      sprintf ( function_return,
	       "%s_sige_%.3f_scalee_%.3f_sigi_%.3f_scalei_%.3f",
	       weight_type, contents_value[1], contents_value[2],
	       contents_value[3], contents_value[4] );
      return function_return;
      
    }
    else{
      return "_ERROR: invalid arguments of weight function";
    }
  }
  
  /* rand function */
  if ( strcmp( weight_type, "random") == 0
      || strcmp( weight_type, "rand" ) == 0 ) {
    check = sscanf( weight_content, "%f , %f",
		   &contents_value[1], &contents_value[2] );
    if ( check == 2 ){
      sprintf (function_return, "rand_min_%.3f_max_%.3f",
	       contents_value[1], contents_value[2] );
      return function_return;
      
    }
    else{
      return "_ERROR: invalid arguments of weight function";
    }
  }
  
  /* exp function */
  if ( strcmp( weight_type, "exp" ) == 0 ) {
    check = sscanf( weight_content, "%f , %f , %f",
		   &contents_value[1], &contents_value[2],
		   &contents_value[3] );
    if ( check == 3 ){
      sprintf (function_return, "%s_max_%.3f_min_%.3f_tau_%.3f",
	       weight_type, contents_value[1], contents_value[2],
	       contents_value[3] );
      return function_return;
      
    }
    else{
      return "_ERROR: invalid arguments of weight function";
    }
  }

  /* Gabor function */
  if ((strcmp( weight_type, "gaborEven" ) == 0) || (strcmp( weight_type, "gaborOdd" ) == 0)) {
    check = sscanf( weight_content, "%f,%f,%f,%f,%f,%f",
		   &contents_value[1], &contents_value[2],
		   &contents_value[3], &contents_value[4],
		   &contents_value[5], &contents_value[6]);
    if ( check == 6 )  {
      sprintf ( function_return,
	       "%s_p_%.3f_o_%.3f_r_%.3f_t_%.3f_r_%.3f_s_%.3f", weight_type,
	       contents_value[1], contents_value[2],
	       contents_value[3], contents_value[4],
	       contents_value[5], contents_value[6] );
      return function_return;
    }
    else {
      printf(" ERROR: arguments of weight function <%s>\n",
	     weight_type);
      return "error";
    }
  }

  return "_ERROR: weight function invalid";
}




char *
edit_connection( which_network, connection_id, new_function )
    char *which_network;
    int connection_id;
    char *new_function;
{
  NETWORK		head;
  /* IKL 95.01.12 Non-static function[] will return garbage. */
  static char		function[120];
  CELL			cell, cell_head;
  NETWORK		net_head;
  CONNECTION		connect_list;
  SPECS			specs;
  int			cell_x, cell_y, count_cells;
  int			cell_num, start_cell, end_cell, count;
  int			start_edit, end_edit, count_weights,x,y;
  float *		ptr_to_weights, * find_weight_array();
  FILE *		fpweights;
  char			file_load[NAME_SIZE];
  extern WEIGHT_ARRAY	weights_from_files;
  WEIGHT_ARRAY		weights_files;
/*  WEIGHT_ARRAY		old_weights_files; */
  float			returned_weight;


  head = network_head;
  while (head && (strcmp( head->name, which_network ) != 0))
    head = head->next;
  if (!head)
    return "Error - No such network.  Check name case.";
  
  start_edit = 1;
  end_edit = head->dim_x * head->dim_y;
  
  net_head = head;
  head = network_head;
  cell_head = net_head->cells;
  specs = (cell_head->specs);

  while (specs != NULL && specs->connect_id != connection_id)
    specs = specs->next;
  if (specs == NULL)
    return "Error - No such connection.";
  while (head->id != specs->id)
    head = head->next;
  
  strcpy( function, convert_connect_function_new( new_function ) );
  if (function[0] == '_' && !strncmp( function, "_ERROR:", 7 ))
    return function+7;

  /* if conductances are in a file read them in */
  if (function[0] == 'f') {
    sscanf(function, "%*[^_]%*c%s", file_load);
    fpweights = nxFopen( file_load );
    if (fpweights == NULL) {
      printf("ERROR - no such file %s for loading new connections - ABORT\n", 
	     file_load);
      exit( ERROR );		/* Terminate program with error status. */
    } 
    else {
      printf("\tLoading Connection file <%s>\n", file_load);
      ptr_to_weights = find_weight_array( file_load );
      if (ptr_to_weights == NULL) {
/*	old_weights_files = weights_from_files; */
	weights_files = (WEIGHT_ARRAY) nxMalloc( SIZE_WEIGHT_ARRAY );
	strcpy(weights_files->name, file_load);
	weights_files->list_of_weights =
	  (float *) nxCalloc( specs->major_dim * specs->minor_dim,
			     sizeof( float ) );
	ptr_to_weights = weights_files->list_of_weights;
	weights_files->next = weights_from_files;
	weights_from_files = weights_files;
	
      }
      
      for (y = specs->minor_dim - 1; y >= 0; y--)
	for (x = 0; x < specs->major_dim; x++) {
	  count_weights = y * specs->major_dim + x;
	  /* store to this address offset */
	  fscanf( fpweights, "%f", (ptr_to_weights + count_weights) );
	}
    }
    fclose( fpweights );
  }
  
  for (count_cells = start_edit; count_cells <= end_edit; count_cells++) {
    cell = head->cells;
    cell_head = net_head->cells + count_cells - 1;
    specs = cell_head->specs;

    while(specs != NULL && specs->connect_id != connection_id)
      specs = specs->next;

    start_cell = get_start_cell( specs->center_x, specs->center_y,
				specs->max_width, specs->max_height,
				head->dim_x, head->dim_y );
    end_cell = get_end_cell( specs->center_x, specs->center_y,
			    specs->max_width, specs->max_height,
			    head->dim_x, head->dim_y );
    cell_num = start_cell;
    count  = (cell_num - 1)/head->dim_x + 1;
    
    /*
     *  Limit sampling to only within a rectangle whose area
     *    is defined by max_width and max_height
     */
    
    if ((specs->center_x - (specs->max_width/2 + 1)) <= head->dim_x)
      while (cell_num <= end_cell) {

	if (cell_num > (count * head->dim_x) || 
	    cell_num >  ((head->dim_x * (count - 1)) + (specs->max_width/2 + 1)
			 + specs->center_x)) {

	  cell_num = count * head->dim_x 
	    + (start_cell - ((start_cell - 1)/head->dim_x) * head->dim_x);
	  count  = (cell_num - 1)/head->dim_x + 1;
	}

	if (cell_num > end_cell)
	  break;

	cell = cell + (cell_num - 1);
	cell_y = (cell_num - 1)/head->dim_x + 1;
	cell_x = cell_num - ((cell_num - 1)/head->dim_x)*head->dim_x ;
	
	if (is_connected( cell, cell_head, specs->center_x, specs->center_y,
			 cell_x, cell_y, specs->major_dim, specs->minor_dim,
			 specs->angle, specs->mask_type, specs->feedback)
	    == TRUE) {

	  if (specs->type == ANTEROGRADE) {

	    connect_list = cell->connect_list;
	    while (connect_list->input_cell != cell_head)
	      connect_list++;

	    returned_weight = get_conductance(specs->center_x, 
					      specs->center_y,
					      cell_x,
					      cell_y,
					      specs->angle,
					      specs->major_dim,
					      specs->minor_dim,
					      function);

	    /*
	     *  Use HUGE to detect error in weight function syntax
	     *  (HUGE == ERROR)
	     */
	    if (returned_weight == HUGE) {
	      return "Error - No such weight function";
	    }
	    connect_list->conductance = returned_weight;
	  }

	  else {		/* RETROGRADE */

	    connect_list = cell_head->connect_list;
	    if (connect_list) {
	      while (connect_list->input_cell != cell)
		connect_list++;

	      returned_weight =  get_conductance( specs->center_x, 
						 specs->center_y,
						 cell_x, cell_y,
						 specs->angle,
						 specs->major_dim,
						 specs->minor_dim,
						 function );

	      if (returned_weight == HUGE) {
		return "Error - No such weight function";
	      }

	      connect_list->conductance = returned_weight;
	    }  /* if (connect_list) */

	  }  /* RETROGRADE */

	}  /* is_connected? */

	cell = cell - (cell_num - 1);
	cell_num++;
      }
  }   

  printf("\nNetwork %s RE-connected\n",net_head->name);
  return NULL;

}  /* edit_connection() */










/* Emacs editing section. DW 940719 */

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
