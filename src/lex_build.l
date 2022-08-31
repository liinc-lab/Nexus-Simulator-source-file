 /*	nexus_lex_build.l
 * 	11/5/91  by Ko Sakai
 *   
 *	DO NOT MODIFY nexus_lex_build.c !!!  
 *
 *	Any modifications MUST be made to nexus_lex_build.l, and then
 *	recompiled to yield a new nexus_lex_build.c.
 *  
 *	Compilation:
 *		lex -t nexus_lex_build.l > nexus_lex_build.c
 *	
 * 	put [{]* at top if expecting ( lex bug ?)
 *	cannot put comment in rule section except inside of action
 *
 *	stdout is redirected to /dev/null.
 *	stdin  is redirected to a  file pointer.
 *
 *	cannot take * in [] with other symbols.
 */

%a 		5500
%o		7000
%e		1200

%{
#include 	"nexus.h"
#include	"graphics.h"
#include	"memory.h"

#define 	NETWORK_ARGS 10		/* specify # of arguments in pseudo-code line */
#define 	CELL_ARGS 4		/* used to check for syntax errors 	*/
#define 	CONNECT_ARGS 11
#define		DEFAULT_TIMEOUT	"60"	/* default time to wait (-w or -p)	*/
#define		FULL	1
#define		AOI	0
FILE		*fpin, *fpout;		/* get from fopen	*/	
int		net_check[500][10];	/* for syntax check  	*/
int		number_weight_by_file;	/* if intern, not work !? */

/*
 *  READ_ONLY external variable.  written by read_network() and
 *    load_simulation() ONLY.  DW 94.07.13
 */
int		number_networks;
%}

blanks		[ \t\n]
any_char	[-.,/_A-Za-z0-9]
char		[-_A-Za-z]
digit		[-0-9.]

%%
	extern NEXUS		nexus_head;
	extern WEIGHT_ARRAY 	weights_from_files;
	extern FILE		*fpin, *fpout;
	extern int		net_check[][10];
	extern int		number_weight_by_file;

	int	number_nexus=0;
	int 	number_left_brace=0, number_right_brace=0;
	int	number_connections=0;
	int	number_comma=0;
	int	cell_number_connections=0;

	register int	i, j, k;
	register int	check=0, count=0;
	int	extern_flag=OFF;
	float	pos_x;
	float	net_size_scale;
	float	name_h_scale, name_w_scale;
	char	connect_type[FUNCTION_ARG_SIZE],
		connect_names[NAME_SIZE];
	int	parameters_store_flag=OFF;
	char	transfer_func[FUNCTION_SIZE];
	char	transfer_func_type[FUNCTION_ARG_SIZE],
		transfer_func_content[FUNCTION_SIZE];
	char	pgn_arg[FUNCTION_SIZE],
		pgn_arguments[FUNCTION_ARG_SIZE];
		/* network->class[80] !! */

	int 	pgn_arc;
	char	clamp_value[FUNCTION_ARG_SIZE];
	int	net_clamp=OFF;
	int	comma[50];
	float 	min, max;
	char 	thrshd_type[FUNCTION_ARG_SIZE],
		thrshd_range[FUNCTION_ARG_SIZE];
	char	connect_net_name[NAME_SIZE];
	char	connect_nex_name[NAME_SIZE];
	int 	projection_type=FULL;
	char	projection_statement[NAME_SIZE];
	int 	first_cell_number;
	char 	aoi_range_char[4][10];
	int	aoi_range[4];
	char	init_firing_type[FUNCTION_ARG_SIZE],
		init_firing_range[FUNCTION_ARG_SIZE];
	int	char_start, char_end;
	char	mapping_type[FUNCTION_ARG_SIZE];
	char	rf_shape[FUNCTION_ARG_SIZE];
	char	weight_type[FUNCTION_ARG_SIZE];
	char	weight_content[FUNCTION_SIZE];
	float	contents_value[10];
	char	feed_back[FUNCTION_ARG_SIZE];
	int	x, y, xs, xe, ys, ye;
	int	seq_cell_number;
	int 	set_firing_rate_by_file();
	int	set_weights_by_file(), set_weights_by_const();
	int	cp_check[11];
	char	errorstr[NAME_SIZE];

	NETWORK		network;
	NEXUS		nexuses;
	CELL		cell_head;

	/* first cell of a particular connection(to/from) */
	CELL		first_cell;

	CELL		new_cell;
	PARAMETERS 	connect_params, params;
	WEIGHT_ARRAY    weights_files, old_weights_files;

	char	*c_sigmoid 	= "sigmoid";
	char	*c_step	  	= "step";
	char	*c_random  	= "random";
	char	*c_const	= "const";
	char	*c_on		= "on";
	char	*c_off		= "off";
	char	*c_full		= "full";
	char	*c_aoi		= "aoi";
	char	*c_file		= "file";
	char	*c_direct	= "direct";
	char	*c_normalize	= "normalize";
	char	*c_ellipse	= "ellipse";
	char	*c_rectangle	= "rectangle";
	char	*c_to		= "to";
	char	*c_from		= "from";
	char	*nex_self 	= "self";
	char 	*opt_default	= "w60";

	number_networks = 0;
	number_weight_by_file = 0;
	yyin = fpin;		/* redirect i/o */
	yyout = fpout;


[/][*].*[*][/] {

  /* single line comments: do nothing */

}





[/][*][- \t\n\[\]\\.,_=+@~"';:?<>!#$&()^/`|{}%A-Za-z0-9]*[*][/]  {
		/* multi-line comments , DO NOT TAKE *  */
		}





[Nn]exus{blanks}+{any_char}+	{

  number_nexus++;

  if (number_nexus == 1 ) { 
    nexuses = nexus_head;
    nexuses->next = NULL;
  }

  if (number_nexus > 1) {
    nexuses->next = (NEXUS) nxMalloc( SIZE_NEXUS );
    nexuses = nexuses->next;
    nexuses->next = NULL;
  }

  nexuses->id = number_nexus;
  if ( ( check=sscanf( yytext,"%*s %s",nexuses->name) ) != 1 )
    fprintf (stderr," ERROR in NX file: nexus name \n");
}





[{]*file{blanks}*write{blanks}*dir{char}*{blanks}*[=]{blanks}*{any_char}+	{
  if ((check = sscanf( yytext, "%*[^=] %*[=] %s",
		      nexuses->file_write_dir )) != 1)
    fprintf (stderr," ERROR in NX file: nexus directory \n");
}





[Nn]etwork{blanks}+{any_char}+  	{

  /* network,  network_name */

  /* reset; increment by to/from, used by projection, etc */
  number_connections = 0;

  number_networks++;
  net_check[number_networks][0]=ON;
  if (number_networks == 1) {
    pos_x = DEFAULT_X_POSITION;
    network = network_head;
    network->next = NULL;
  }
  if (number_networks > 1) {
    pos_x = (network->pos_x + DEFAULT_X_GUTTER
	     + (network->cell_size * (float) network->dim_x));
    network->next = (NETWORK) nxMalloc( SIZE_NETWORK );
    network = network->next;
    network->next = NULL;
  }

  /* always make and reset extern info 	*/
  network->extern_connections =
    (EXTERN_CONNECTION) nxMalloc( SIZE_EXTERN_CONNECTION );
  (network->extern_connections)->extern_to = FALSE; /* if ON, set at to/from */
  (network->extern_connections)->extern_from = FALSE;
  (network->extern_connections)->extern_to_times = 0;
  (network->extern_connections)->extern_from_times = 0;

  if ( ( check=sscanf( yytext,"%*s %s", network->name ) ) != 1 )
    fprintf (stderr," ERROR in NX file: network name \n");

  network->id = number_networks;
  network->cell_size = SIZE_FACTOR;
  network->pos_x = pos_x;
  network->pos_y = 0.0;

  /* changed to reduce unnecessary field in setting structure DW 94.07.07 */
  network->name_size_h = 1.0;
  network->name_size_w = 1.0;

  network->scale = 1.0;	/* optional arguments */
  network->offset = 0.0;
  network->function_slope = 1.0;
  network->decay = 1.0;

  for ( i=1; i<=9; i++ ) 
    net_check[number_networks][i] = OFF;
}



[{]*(#|number){blanks}*of{blanks}*units{blanks}*[=]{blanks}*{digit}+ 	{
		/* # of cells  is eliminated 10/12/92						*/
		/* when both x dim and y dim are set,						*/
		/* compute # of units and allocate memory for all CELLs. 			*/
		/* there is a checking to avoid multiple allocations.				*/
}

x{blanks}*dim{char}*{blanks}*[=]{blanks}*{digit}+ {
  net_check[number_networks][2]=ON;

  /* x dimension 	*/
  if ( ( check = sscanf( yytext,"%*[^=] %*[=] %d", &network->dim_x) ) != 1 )
    fprintf (stderr," ERROR in NX file: x dimension \n");

  /* # of cells (added 10/12/92) */
  if ((net_check[number_networks][2] == ON)
      && (net_check[number_networks][3] == ON)) {
    /* if x & y dim are set */
    if (net_check[number_networks][1]==ON) {
      /* memory already allocated 	*/
      fprintf (stderr," ERROR in NX file: x or y dimension duplicated.\n");
      net_check[number_networks][1]==OFF;
    } else {
      /* allocate memory to CELLs	*/
      net_check[number_networks][1]=ON;

      network->number_cells = network->dim_x * network->dim_y;
      cell_head = (CELL) nxCalloc( network->number_cells, SIZE_CELL );
      network->cells = cell_head;

      /* set cell id and net id to ALL CELLS */
      for (count=0; count < network->number_cells; count++) {
	(cell_head + count)->id = count + 1;   /* cell id begins from 1 */
	(cell_head + count)->net_id = network->id;	
      }
    }
  }
}

y{blanks}*dim{char}*{blanks}*[=]{blanks}*{digit}+ {
  net_check[number_networks][3]=ON;

  /* y dimension 	*/
  if ( ( check = sscanf( yytext,"%*[^=] %*[=] %d", &network->dim_y) ) != 1 )
    fprintf (stderr," ERROR in NX file: y dimension \n");

  /* # of cells (added 10/12/92) */
  if ((net_check[number_networks][2] == ON)
      && (net_check[number_networks][3] == ON)) {
    /* if x & y dim are set */
    if (net_check[number_networks][1]==ON) {
      /* memory already allocated 	*/
      fprintf (stderr," ERROR in NX file: x or y dimension duplicated.\n");
      net_check[number_networks][1]==OFF;
    } else {
      /* allocate memory to CELLs	*/
      net_check[number_networks][1]=ON;

      network->number_cells = network->dim_x * network->dim_y;
      cell_head = (CELL) nxCalloc( network->number_cells, SIZE_CELL );
      network->cells = cell_head;

      /* set cell id and net id to ALL CELLS */
      for (count=0; count < network->number_cells; count++) {
	(cell_head + count)->id = count + 1;    /* cell id begins from 1 */
	(cell_head + count)->net_id = network->id;	
      }
    }
  }		
}

x{blanks}*pos{char}*{blanks}*[=]{blanks}*{digit}+ {
		/* optional, thus no check */
		/* display position */
		if ( ( check = sscanf( yytext,"%*[^=] %*[=] %f", &network->pos_x) ) != 1 )
			fprintf (stderr," ERROR in NX file: x position \n");
 }

y{blanks}*pos{char}*{blanks}*[=]{blanks}*{digit}+ {
		/* optional, thus no check */
		/* display position */
		if ( ( check = sscanf( yytext,"%*[^=] %*[=] %f", &network->pos_y) ) != 1 )
			fprintf (stderr," ERROR in NX file: y position \n");
}

net{char}*{blanks}*size{blanks}*[=]{blanks}*{digit}+ {
		/* optional, thus no check */
		/* display size */
		if ( ( check = sscanf( yytext,"%*[^=] %*[=] %f", &net_size_scale) ) != 1 )
			fprintf (stderr," ERROR in NX file: network display size \n");
		network->cell_size *= net_size_scale;
}
text{blanks}*h[eight]*{blanks}*[=]{blanks}*{digit}+ {
		/* optional, thus no check */
		/* text display size */
		if ( ( check = sscanf( yytext,"%*[^=] %*[=] %f", &name_h_scale) ) != 1 )
			fprintf (stderr," ERROR in NX file: network text size \n");
		else
		  network->name_size_h = name_h_scale;
}
text{blanks}*w[idth]*{blanks}*[=]{blanks}*{digit}+ {
		/* optional, thus no check */
		/* text display size */
		if ( ( check = sscanf( yytext,"%*[^=] %*[=] %f", &name_w_scale) ) != 1 )
			fprintf (stderr," ERROR in NX file: network text size \n");
		else
		  network->name_size_w = name_w_scale;
}

transfer{blanks}*func{char}*{blanks}*[=]{blanks}*{any_char}+ {

  net_check[number_networks][4]=ON;

  /* class (transfer_function) 	*/
  if ( ( check=sscanf( yytext,"%*[^=] %*[=] %s", transfer_func) ) != 1 )
    fprintf (stderr," ERROR in NX file: transfer function \n");

#if 0
  fprintf( stderr, "# network <%s>, transfer func <%s>\n",
	  network->name, transfer_func );
#endif

  /* no argument , thus set default min , max ,slope */
  network->function_min = DEFAULT_ACTIVITY_MIN;
  network->function_max = DEFAULT_ACTIVITY_MAX;
  network->function_slope = DEFAULT_ACTIVITY_SLOPE;

  /* to convert simnoid to simple, step to binary */
  if ( strcmp( transfer_func, c_sigmoid ) == 0 ) {
    sprintf( network->class, "simple");
    break;
  }
  if ( strcmp( transfer_func, c_step ) == 0 ) {
    sprintf( network->class, "binary");
    break;
  }

  /* linear */
  if ( strcmp( transfer_func, "linear" ) == 0 ) {	
    sprintf( network->class,"%s",transfer_func );
    break;
  }

  /* allow other funcs for connect() 				*/
  sscanf( transfer_func,"%s",network->class );

#if 0
  fprintf( stderr, "# network <%s>, transfer func <%s>\n",
	  network->name, network->class );
#endif
}





transfer{blanks}*func{char}*{blanks}*[=]{blanks}*{any_char}+{blanks}*[(][- \t\n,._/A-Za-z0-9]+[)] {

  net_check[number_networks][4]=ON;

  /* class (transfer_function) , pgn 		*/
  /* allow other funcs for connect() 		*/
  check=sscanf( yytext,"%*[^=] %*[=] %[^(] %*[(] %[^)] %*[)]", transfer_func_type, transfer_func_content ); 
  if ( check != 2 ) 
    fprintf (stderr," ERROR in NX file: transfer function \n");
  sscanf( transfer_func_type,"%s",transfer_func_type);		/* to remove spaces */

  /* fprintf( stderr, "# network <%s>, transfer func content <%s>\n",network->name,transfer_func_content ); */ 
  /* set defaults of min, max, slope */ 		
  network->function_min = DEFAULT_ACTIVITY_MIN;
  network->function_max = DEFAULT_ACTIVITY_MAX;
  network->function_slope = DEFAULT_ACTIVITY_SLOPE;

  /* sigmoid, 3 arguments, convert simnoid to simple */
  if ( strcmp( transfer_func_type, c_sigmoid ) == 0 ) {	
    sprintf( network->class, "simple");
    if ( sscanf(transfer_func_content, "%f %*[,] %f %*[,] %f",
		&network->function_min, &network->function_max, &network->function_slope ) != 3 )
      fprintf( stderr," ERROR in sigmoid function in network<%s>, defaults used.\n",network->name );
    /* fprintf( stderr,"# slope %f \n",network->function_slope); */
    break;
  }

  /* step, 2 arguments, convert to binary */
  if ( strcmp( transfer_func_type, c_step ) == 0 ) {
    sprintf( network->class, "binary");
    if ( sscanf(transfer_func_content, "%f %*[,] %f",
		&network->function_min, &network->function_max ) != 2 )
      fprintf( stderr," ERROR in step function in network<%s>, defaults used.\n",network->name );
    break;
  }

  /* linear 3 argument */
  if ( strcmp( transfer_func_type, "linear" ) == 0 ) {	
    sprintf( network->class,"%s",transfer_func_type );
    if (sscanf( transfer_func_content,
	       "%f %*[,] %f %*[,] %f",
	       &network->function_min,
	       &network->function_max,
	       &network->function_slope ) != 3)
      fprintf( stderr,
	      " ERROR in linear function in network<%s>, defaults used.\n",
	      network->name );
    break;
  }

  /* ln 1 argument  SY 94.11.08 */
  if (strcmp( transfer_func_type, "ln" ) == 0) {
    sprintf( network->class, "%s", transfer_func_type );
    if (sscanf( transfer_func_content, "%f",
	       &network->function_slope ) != 1)
      fprintf( stderr,
	      " ERROR in ln function in network<%s>, defaults used.\n",
	      network->name );
    break;
  }

  /* exp 1 argument  SY 94.11.08 */
  if (strcmp( transfer_func_type, "exp" ) == 0) {
    sprintf( network->class, "%s", transfer_func_type );
    if (sscanf( transfer_func_content, "%f",
	       &network->function_slope ) != 1)
      fprintf( stderr,
	      " ERROR in exp function in network<%s>, defaults used.\n",
	      network->name );
    break;
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
    comma[j] = yyleng;
    pgn_arc = j;

    /* set nexts_in_nex: name, extern_from/to = FALSE */

    for ( i = 0; i < pgn_arc; i++ ) {
      for ( j = 0; j < FUNCTION_ARG_SIZE; j++ )	/* initialize to clean up  */ 
	pgn_arguments[j] = ' ';
      k = 0;
      for ( j = (comma[i] + 1); j < comma[i+1]; j++ ) {
	pgn_arguments[k] = pgn_arg[j];
	k++;
      }
      sscanf( pgn_arguments,"%s",pgn_arguments );	/* remove spaces */
      if (i == 0)
	sprintf( transfer_func_content, "%s",
		pgn_arguments );
      else 
	sprintf( transfer_func_content, "%s,%s",
		transfer_func_content, pgn_arguments );
    }
    sprintf( network->class, "%s-%s",
	    transfer_func_type, transfer_func_content );
    /* fprintf(stderr,"pgn function: %s\n",network->class); */ 
  }
  else {

    /* allow other functions */
    /* (expect one argument) */
    sscanf( transfer_func_content,"%s",transfer_func_content);
    sprintf( network->class,"%s_%s",
	    transfer_func_type, transfer_func_content );
  }

#if 0
  fprintf( stderr,
	  "# network <%s>, transfer func <%s>\n",
	  network->name, network->class );
#endif
}

threshold{blanks}*[=]{blanks}*{any_char}+{blanks}*[(][- \t\n,._A-Za-z0-9]+[)] {
		net_check[number_networks][5]=ON;
		if ( net_check[number_networks][1] == OFF ) {
			fprintf( stderr," ERROR in NX file: threshold defined earlier than x,y dimensions in network <%s>.\n",network->name);
			return(ERROR);
		}

		check=sscanf( yytext, "%*[^=] %*[=] %[^(] %*[(] %[^)] %*[)]", thrshd_type, thrshd_range);
		sscanf( thrshd_type,"%s",thrshd_type);	/* to remove spaces */
		
		if ( check != 2 )
			fprintf( stderr," ERROR in NX file: threshold \n");

		/* set threshold to ALL CELL */
		if ( strcmp( thrshd_type, c_const) == 0 ) {
			cell_head->threshold = (float) atof( thrshd_range );
			for (count=1; count < network->number_cells; count++) 
				(cell_head + count)->threshold = cell_head->threshold;
		} else {
			if ( strcmp( thrshd_type, c_random) == 0 ) {
				if ( (check = sscanf (thrshd_range, "%f %*[,] %f", &min, &max)) != 2 )
					fprintf( stderr," ERROR in NX file: threshold \n");
				for (count=0; count < network->number_cells; count++) 
					(cell_head + count)->threshold = get_random(min, max);
			} else {
				fprintf( stderr," ERROR in NX file: threshold \n");
		}	}
}

scale{blanks}*[=]{blanks}*{digit}+ {
		/* optional, no net check. */
		/* scale */
		if ( ( check=sscanf( yytext, "%*[^=] %*[=] %f", &network->scale) ) != 1 )
			fprintf( stderr," ERROR in NX file: scale \n");
}

offset{blanks}*[=]{blanks}*{digit}+ {
		/* optional, no net check. */
		/* offset */
		if ( ( check=sscanf( yytext, "%*[^=] %*[=] %f", &network->offset) ) != 1 )
			fprintf( stderr," ERROR in NX file: offset \n");
}


decay{blanks}*[=]{blanks}*{digit}+ {
		net_check[number_networks][6]=ON;
		if ( ( check=sscanf( yytext, "%*[^=] %*[=] %f", &network->decay) ) != 1 )
			fprintf( stderr," ERROR in NX file: decay \n");
}

clamp{blanks}*[=]{blanks}*{any_char}+ {
		net_check[number_networks][7]=ON;
		if ( net_check[number_networks][1] == OFF ) {
			fprintf( stderr," ERROR in NX file: clamp defined earlier than x,y dimensions in network <%s>.\n",network->name);
			return(ERROR);
		}

		if ( ( check=sscanf( yytext, "%*[^=] %*[=] %s",clamp_value) ) != 1 )
			fprintf( stderr," ERROR in NX file: clamp \n");
		if ( strcmp( clamp_value, c_on) == 0 ) {
			cell_head->clamp = CLAMP_ON;
		} else {
			if ( strcmp( clamp_value, c_off) == 0 ) {
				cell_head->clamp = CLAMP_OFF;
			} else {
				fprintf( stderr," ERROR in NX file: clamp \n");
				break;
			}
		}
		/* set clamp to ALL CELLS */
		for (count=1; count < network->number_cells; count++) 
			(cell_head + count)->clamp = cell_head->clamp;
}

initial{blanks}*firing{blanks}*rate{blanks}*[=]{blanks}*{any_char}+{blanks}*[(][- \t\n,._A-Za-z0-9/]+[)] {

  net_check[number_networks][8] = ON;

  if ( net_check[number_networks][1] == OFF ) {
    fprintf( stderr,
	    " ERROR in NX file: initial firing rate defined earlier than x,y dimensions of network <%s>.\n",
	    network->name );
    return ERROR;
  }

  check = sscanf( yytext, "%*[^=] %*[=] %[^(] %*[(] %[^)] %*[)]",
	       init_firing_type, init_firing_range );
  sscanf( init_firing_type, "%s", init_firing_type ); /* to remove spaces */

  if (check != 2)
    fprintf( stderr," ERROR in NX file: initial firing rate \n");

  /* set initial firing rates to all cells in a network */
  if ( strcmp( init_firing_type, c_const) == 0 ) {

    cell_head->firing_rate = (float) atof( init_firing_range );
    for (count=1; count < network->number_cells; count++) 
      (cell_head + count)->firing_rate = cell_head->firing_rate;

  }
  else {
    if ((strcmp( init_firing_type, c_random ) == 0)
	|| (strcmp( init_firing_type, "rand" ) == 0)) {

      if ((check = sscanf( init_firing_range,
			  "%f %*[,] %f", &min, &max )) != 2)
	fprintf( stderr," ERROR in NX file: initital firing rate \n");
      for (count=0; count < network->number_cells; count++) 
	(cell_head + count)->firing_rate = get_random(min, max);

    }
    else {

      if (strcmp( init_firing_type, c_file ) == 0) {
	sscanf( init_firing_range, "%s", init_firing_range );
	set_firing_rate_by_file( network, init_firing_range );
      }
      else
	fprintf( stderr," ERROR in NX file: init firing rate \n");

    }
  }
}

evaluation[s]*{blanks}*per{blanks}*cycle{blanks}*[=]{blanks}*{digit}+ {
		net_check[number_networks][9]=ON;
		if ( (check=sscanf( yytext,"%*[^=] %*[=] %d", &network->evaluations_per_cycle) ) != 1 )
		  fprintf( stderr, " ERROR in NX file: evaluation per cycle \n" );

		/* BUG FIX: Don't allow neg/zero evals per cycle.  DW 940803 */
		if (network->evaluations_per_cycle <= 0) {
		  fprintf( stderr, " ERROR in NX file: evaluations per cycle must be greater than zero,\n" );
		  fprintf( stderr, "  in network %s.  Setting to 1 (default).\n", network->name );
		  network->evaluations_per_cycle = 1;
		}
}


connection	{
}

[{]*(from|to|pgn-to){blanks}*[-@./_A-Za-z0-9]+  {
		cp_check[0]=ON;
		if ( ( check=sscanf( yytext,"%s %s", connect_type, connect_names) ) != 2 )
			fprintf( stderr," ERROR in NX file: connect from/to \n");

		
		check = sscanf( connect_names,"%[^@] %*[@] %s",connect_net_name, connect_nex_name);
		if (check == 2) {
			/* if @ exist	*/
			extern_flag = ON;	/* initially OFF, reset at { */
			if ( strcmp( connect_type, c_to) == 0 ) {
				(network->extern_connections)->extern_to = TRUE;
				(network->extern_connections)->extern_to_times++;
				sprintf( (network->extern_connections)->extern_to_dir,"%s", get_extern_dir(nex_self, nexus_head)); 
			} else {
				if ( strcmp( connect_type, c_from) == 0 ) {
					(network->extern_connections)->extern_from = TRUE;
					if ( ( ((network->extern_connections)->extern_from_times)++ ) >= 2 ) 
						fprintf( stderr," ERROR in NX file: extern from connection \n");
					sscanf( connect_net_name,"%s",(network->extern_connections)->extern_from_net );
					sscanf( connect_nex_name,"%s",(network->extern_connections)->extern_from_nex );
					sprintf( (network->extern_connections)->extern_from_dir,"%s"
									, get_extern_dir(connect_nex_name, nexus_head));
					/* set default option value */ 
					sprintf( (network->extern_connections)->extern_from_opt,"%s",opt_default );
				} else {
					fprintf( stderr," ERROR in NX file: extern to/from connection \n");
			}	}
		} else {
			extern_flag = OFF;
			number_connections++;   	/* reset at next network */
			/* 
			 * after participating cells are defined in "projection =", set net_name and to/from there.
			 */ 
		}
		for ( i=1; i<=10; i++ )
			cp_check[i] = OFF;
}





[{]*projection[s]*{blanks}*[=]{blanks}*(full|FULL|Full) {

  /* projection FULL */

  cp_check[1]=ON;

  /*
   *  set all parameters to first_cell->connect_parameters(connect_params).
   *  when reach }, copy them to ALL AOI cells.
   *  NOTE: 1) AOIs cannot overlap.
   *        2) FULL statement must locate before any AOI 
   */


  if ( extern_flag == ON ) break;
  if ( parameters_store_flag == ON ) {
    /* previous connect to/from { is not ended by } yet.	*/
    fprintf( stderr," ERROR in NX file: projection \n");
  }
  parameters_store_flag = ON;
  projection_type = FULL;

  connect_params = (PARAMETERS) nxMalloc( SIZE_PARAMETERS );

  /* set connected net name, # connections and type to first_cell  */
  sprintf( connect_params->name, "%s", connect_net_name );

  if ( strcmp( connect_type, c_to) == 0 ) {
    connect_params->type = ANTEROGRADE ;
  }
  else {
    if ( strcmp( connect_type, c_from) == 0 ) {
      connect_params->type = RETROGRADE;
    }
    else {
      if ( strcmp( connect_type, "pgn-to") == 0 ) 
	connect_params->type = PGN_ANTEROGRADE;
      else 
	fprintf( stderr," ERROR in NX file: to/from connection.\n");
    }	
  }

  connect_params->next = NULL;
}





[{]*projection[s]*{blanks}*[=]{blanks}*aoi{blanks}*[(]{blanks}*{digit}+[ \t\n,]*{digit}+[ \t\n,]*{digit}+[ \t\n,]*{digit}+[ \t\n,]*{blanks}*[)] {

  /* projection AOI */

  cp_check[1]=ON;

  /*
   *  NOTE: 1) AOIs cannot overlap.
   *        2) FULL statement must locate before any AOI
   */


  if ( extern_flag == ON ) break;
  if ( parameters_store_flag == ON ) {
    /* previous connect to/from { is not ended by } yet.	*/
    fprintf( stderr," ERROR in NX file: projection \n");
  }	
  parameters_store_flag = ON;	/* reset to OFF at next closing brase */
  projection_type = AOI;	

  if ( sscanf(yytext, "%*[^(] %*[(] %d %*[,] %d %*[,] %d %*[,] %d",
	      &aoi_range[0],&aoi_range[1],&aoi_range[2],&aoi_range[3]) != 4 )
    fprintf( stderr," ERROR in NX file: projection \n");

  connect_params = (PARAMETERS) nxMalloc( SIZE_PARAMETERS );

  /* set connected net name, # connections and type to first_cell  */
  sprintf( connect_params->name, "%s", connect_net_name );

  if ( strcmp( connect_type, c_to) == 0 ) {
    connect_params->type = ANTEROGRADE ;
  }
  else {
    if ( strcmp( connect_type, c_from) == 0 ) {
      connect_params->type = RETROGRADE;
    }
    else {
      if ( strcmp( connect_type, "pgn-to") == 0 ) 
	connect_params->type = PGN_ANTEROGRADE;
      else 
	fprintf( stderr," ERROR in NX file: to/from connection \n");
    }	
  }

  connect_params->next = NULL;
}





mapping{blanks}*type{blanks}*[=]{blanks}*{any_char}+ {

  cp_check[2]=ON;

  if ( cp_check[1] != ON ) {
    fprintf( stderr,
	    " ERROR in NX file: mapping type defined earlier than projection\n" );
    fprintf( stderr,
	    "                   in network <%s>, connection with <%s> \n",
	    network->name, connect_net_name );
    return ERROR;
  }

  if ( ( check=sscanf( yytext,"%*[^=] %*[=] %s", mapping_type) ) != 1 )
    fprintf (stderr," ERROR in NX file: mapping type \n");
  if ( strcmp( mapping_type, c_direct) == 0 ) {
    connect_params->projection_type = DIRECT;
  } else {
    if ( strcmp( mapping_type, c_normalize) == 0 ) 
      connect_params->projection_type = NORMALIZE;
    else 
      fprintf( stderr," ERROR in NX file: mapping type  \n");
  }
}





connection{blanks}*field{blanks}*shape{blanks}*[=]{blanks}*{any_char}+ {

  cp_check[3]=ON;

  if ( cp_check[1] != ON ) {
    fprintf( stderr,
	    " ERROR in NX file: connection field shape defined earlier than projection \n" );
    fprintf( stderr,
	    "                   in network <%s>, connection with <%s> \n",
	    network->name, connect_net_name );
    return ERROR;
  }

  if ( ( check=sscanf( yytext,"%*[^=] %*[=] %s", rf_shape) ) != 1 )
    fprintf (stderr," ERROR in NX file: connection field shape \n");
  if ( strcmp( rf_shape, c_ellipse) == 0 ) {
    connect_params->mask_type = ELLIPSE ;
  } else {
    if ( strcmp( rf_shape, c_rectangle) == 0 ) 
      connect_params->mask_type = RECTANGLE;
    else 
      fprintf( stderr," ERROR in NX file: connection field shape  \n");
  }
}





length{blanks}*[=]{blanks}*{any_char}+ {

  cp_check[4]=ON;

  if ( cp_check[1] != ON ) {
    fprintf( stderr,
	    " ERROR in NX file: length defined earlier than projection \n" );
    fprintf( stderr,
	    "                   in network <%s>, connection with <%s> \n",
	    network->name, connect_net_name );
    return ERROR;
  }

  if ( ( check=sscanf( yytext,"%*[^=] %*[=] %d", &connect_params->minor_dim) ) != 1 )
    fprintf (stderr," ERROR in NX file: connection length \n");	/* minor & major shifted 5/1/92 */
}





width{blanks}*[=]{blanks}*{any_char}+ {

  cp_check[5]=ON;

  if ( cp_check[1] != ON ) {
    fprintf( stderr,
	    " ERROR in NX file: width defined earlier than projection \n" );
    fprintf( stderr,
	    "                   in network <%s>, connection with <%s> \n",
	    network->name, connect_net_name );
    return ERROR;
  }

  if ((check = sscanf( yytext, "%*[^=] %*[=] %d",
		      &connect_params->major_dim)) != 1)
    fprintf (stderr," ERROR in NX file: connection width \n");
}





rotation{blanks}*angle{blanks}*[=]{blanks}*{any_char}+ {

  cp_check[6]=ON;

  if ( cp_check[1] != ON ) {
    fprintf( stderr,
	    " ERROR in NX file: rotation defined earlier than projection \n" );
    fprintf( stderr,
	    "                   in network <%s>, connection with <%s> \n",
	    network->name,connect_net_name );
    return ERROR;
  }

  if ((check = sscanf( yytext, "%*[^=] %*[=] %d",
		      &connect_params->angle )) != 1)
    fprintf (stderr," ERROR in NX file: angle \n");
}





shift{blanks}*x{blanks}*[=]{blanks}*{any_char}+ {

  cp_check[7]=ON;

  if ( cp_check[1] != ON ) {
    fprintf ( stderr,
	     " ERROR in NX file: shift x defined earlier than projection \n" );
    fprintf ( stderr,
	     "                   in network <%s>, connection with <%s> \n",
	     network->name, connect_net_name );
    return ERROR;
  }

  if ((check=sscanf(yytext,"%*[^=] %*[=] %d",&connect_params->shift_x))!=1)
    fprintf (stderr," ERROR in NX file: shift x \n");
}





shift{blanks}*y{blanks}*[=]{blanks}*{any_char}+ {

  cp_check[8]=ON;

  if ( cp_check[1] != ON ) {
    fprintf ( stderr,
	     " ERROR in NX file: shift y defined earlier than projection \n" );
    fprintf ( stderr,
	     "                   in network <%s>, connection with <%s> \n",
	     network->name, connect_net_name );
    return ERROR;
  }

  if ((check=sscanf(yytext,"%*[^=] %*[=] %d",&connect_params->shift_y))!=1)
    fprintf (stderr," ERROR in NX file: shift y \n");
}





weight{blanks}*function{blanks}*[=]{blanks}*{any_char}+{blanks}*[(][- \t\n,._A-Za-z0-9/]+[)] {

  cp_check[9]=ON;

  if ( cp_check[1] != ON ) {
    fprintf ( stderr,
	     " ERROR in NX file: weight function defined earlier than projection \n");
    fprintf ( stderr,
	     "                   in network <%s>, connection with <%s> \n",
	     network->name, connect_net_name );
    return(ERROR);
  }

  check = sscanf( yytext, "%*[^=] %*[=] %[^(] %*[(] %[^)] %*[)]",
		 weight_type, weight_content );
  sscanf( weight_type, "%s" ,weight_type ); /* to remove spaces */

  if ( check != 2 )
    fprintf( stderr," ERROR in NX file: weight function \n");

  /* file function */
  if ( strcmp( weight_type, c_file ) == 0 ) {
    number_weight_by_file++;		/* reset only at start */
    if ( number_weight_by_file == 1 ) {
      weights_files = weights_from_files; 
      weights_files->next = NULL;
#if 0
      printf("# %d st weight file: %x\n",number_weight_by_file,weights_files);
#endif
    }
    if ( number_weight_by_file > 1 ) {
      weights_files->next = (WEIGHT_ARRAY) nxMalloc( SIZE_WEIGHT_ARRAY );
      weights_files = weights_files->next;
      weights_files->next = NULL;
#if 0
      printf("#  %d th weight file: %x\n",number_weight_by_file,weights_files);
#endif
    }

    sscanf( weight_content,"%s",weight_content );

    /*
     *  DW 95.01.20  Bug fix: if we can't find the file, stop right now!
     *    Otherwise we'll crash later on when trying to connect this network.
     */
    if (set_weights_by_file( weights_files,
			    weight_content,
			    connect_params->major_dim,
			    connect_params->minor_dim ) == ERROR) {
      fprintf( stderr, "FATAL: Exiting program.\n" );
      exit( ERROR );
    }

    sprintf ( connect_params->function, "%s_%s",weight_type, weight_content );
  }

  /* const function */
  if ( strcmp( weight_type, c_const ) == 0 ) { 
    check = sscanf( weight_content, "%f", &contents_value[1] );
    if ( check == 1 )
      sprintf ( connect_params->function, "%s_%f",
	       weight_type, contents_value[1] );
    else
      fprintf( stderr,
	      " Error in NX file: arguments of weight function <%s> in network<%s>\n",
	      network->name, weight_type );
  }

  /* line function */	
  if ( strcmp( weight_type, "line" ) == 0 ) {
    check = sscanf( weight_content, "%f , %f , %f , %f",
		   &contents_value[1], &contents_value[2],
		   &contents_value[3], &contents_value[4] );
    if ( check == 4 )
      sprintf ( connect_params->function,
	       "%s_sige_%f_scalee_%f_sigi_%f_scalei_%f",
	       weight_type, contents_value[1], contents_value[2],
	       contents_value[3], contents_value[4] ); 
    else
      fprintf( stderr,
	      " Error in NX file: arguments of weight function <%s> in network<%s>\n",
	      network->name, weight_type );
  }

  /* dog function */
  if ( strcmp( weight_type, "dog" ) == 0 ) {
    check = sscanf( weight_content, "%f , %f , %f , %f",
		   &contents_value[1], &contents_value[2], &contents_value[3], &contents_value[4] );
    if ( check == 4 )
      sprintf ( connect_params->function, "%s_sige_%f_scalee_%f_sigi_%f_scalei_%f",
	       weight_type, contents_value[1], contents_value[2], contents_value[3], contents_value[4] );
    else
      fprintf(stderr," Error in NX file: arguments of weight function <%s> in network<%s>\n",
	      network->name,weight_type);
  }

  /* rand function */
  if ( strcmp( weight_type, "random") == 0
      || strcmp( weight_type, "rand" ) == 0 ) {
    check = sscanf( weight_content, "%f , %f",
		   &contents_value[1], &contents_value[2] );
    if ( check == 2 )
      sprintf ( connect_params->function, "rand_min_%f_max_%f",
	       contents_value[1], contents_value[2] );
    else
      fprintf(stderr," Error in NX file: arguments of weight function <%s> in network<%s>\n",
	      network->name,weight_type);
  }

  /* exp function */
  if ( strcmp( weight_type, "exp" ) == 0 ) {
    check = sscanf( weight_content, "%f , %f , %f",
		   &contents_value[1],
		   &contents_value[2],
		   &contents_value[3] );
    if ( check == 3 ) 
      sprintf ( connect_params->function, "%s_max_%f_min_%f_tau_%f",
	       weight_type, contents_value[1],
	       contents_value[2], contents_value[3] );
    else
      fprintf(stderr," Error in NX file: arguments of weight function <%s> in network<%s>\n",
	      network->name,weight_type);
  }

  /* Gabor function */
  if((strcmp( weight_type, "gaborEven" ) == 0) || (strcmp( weight_type, "gaborOdd" ) == 0)) {
    check = sscanf( weight_content, "%f,%f,%f,%f,%f,%f",
		   &contents_value[1], &contents_value[2],
		   &contents_value[3], &contents_value[4],
		   &contents_value[5], &contents_value[6]);
    if ( check == 6 ) 
      sprintf ( connect_params->function,
	       "%s_p_%f_o_%f_r_%f_t_%f_r_%f_s_%f",
	       weight_type, contents_value[1], contents_value[2],
	       contents_value[3], contents_value[4],
	       contents_value[5], contents_value[6]);
    else
      fprintf( stderr,
	      " Error in NX file: arguments of weight function <%s> in network<%s>\n",
	      network->name, weight_type );
  }

  /* fprintf (stderr, "# network <%s> weight_func: <%s>\n",network->name,connect_params->function); */
}





feed{blanks}*back{blanks}*[=]{blanks}*{any_char}+ {
		cp_check[10]=ON;
		if ( cp_check[1] != ON ) {
			fprintf (stderr," ERROR in NX file: feedback defined earlier than projection \n");
			fprintf (stderr,"                   in network <%s>, connection with <%s> \n",network->name,connect_net_name);
			return(ERROR);
		}

		if ( ( check=sscanf( yytext,"%*[^=] %*[=] %s", feed_back) ) != 1 )
			fprintf (stderr," ERROR in NX file: feed back \n");
		if ( strcmp( feed_back, c_on) == 0 ) {
			connect_params->feedback = ON;
		} else {
			if ( strcmp( feed_back, c_off) == 0 ) 
				connect_params->feedback = OFF;
			else 
				fprintf( stderr," ERROR in NX file: feed back \n");
		}
}





extern{blanks}*from{blanks}*(option|opt){blanks}*[=]{blanks}*{any_char}+ {

  if ((check = sscanf( yytext,"%*[^=] %*[=] %s",
		      (network->extern_connections)->extern_from_opt )) != 1)
    fprintf (stderr," ERROR in NX file:  extern from option \n");

  /* Check for w or p w/out a timeout; if so, force value of DEFAULT_TIMEOUT */
  /* DW 94.07.07 */

  if( *((network->extern_connections)->extern_from_opt) != 'g'
     && *((network->extern_connections)->extern_from_opt + 1) == '\0' ) {
    strcat( (network->extern_connections)->extern_from_opt, DEFAULT_TIMEOUT );
    fprintf( stderr,
	    " ERROR in NX file:  extern from option, needs timeout value \n" );
  }
}





[}]		{

  /* case {  */

  if ( (extern_flag == ON) || (parameters_store_flag != ON) )
     break;  /* not always error */

  parameters_store_flag = OFF;
  extern_flag = OFF;

  /* copy all first_cell info to ALL CELLS in FULL or AOI
   * NOTE: 1) AOIs cannot partly overlap.
   *   2) FULL statement must locate before any AOI 
   *   3) larger AOI should be defined earlier than smaller one */

  if ( projection_type == FULL ) {
    /*  check first cell to see if connect_params is specified yet.
     *    since full connections HAVE to be specified before AOI's, this
     *    should not cause problems.
     */

    if (cell_head->net_connections == 0) {

      /* first connection for all cells */
      for ( i = 0; i < network->number_cells; i++ ) {
	new_cell = cell_head + i;
	new_cell->net_connections++;
	new_cell->connect_parameters = connect_params;
	new_cell->connect_list_un = NULL;
	new_cell->connect_list = NULL;
	new_cell->output_list_un = NULL;
	new_cell->output_list = NULL;
	new_cell->specs = NULL;
      }
    }
    else {
      /*  append connect_params to end of cell_head's connect_parameters.  */
      params = cell_head->connect_parameters;
      while (params->next != NULL)
	params = params->next;
      params->next = connect_params;
      for (i = 0; i < network->number_cells; i++) {
	/*
	 *  Need only to increase net_connections since connect_params
	 *    is already pointing to cell_head's connections, which is
	 *    the same for all cells.
	 */
	new_cell = cell_head + i;
	new_cell->net_connections++;
      }
    }
  }

  else {
    /* for AOI */			
    xs = aoi_range[0];   xe = aoi_range[2];
    ys = aoi_range[1];   ye = aoi_range[3];

    first_cell = cell_head + ((ys - 1) * network->dim_x + xs);

    if (first_cell->net_connections == 0) {
      /*
       *  First AOI cell has no connections, therefore assume no FULL
       *    connections and no other AOI connections in this region, since
       *    larger AOI has to be defined before.
       */

      for ( y = ys; y <= ye; y++) {
	for ( x= xs; x <= xe; x++) {
	  seq_cell_number = (network->dim_x * (y - 1)) + x ;
	  /* -1 for the first_cell */
	  new_cell = cell_head + seq_cell_number - 1;
	  new_cell->net_connections++;
	  new_cell->connect_parameters = connect_params;
	  new_cell->connect_list_un = NULL;
	  new_cell->connect_list = NULL;
	  new_cell->output_list_un = NULL;
	  new_cell->output_list = NULL;
	  new_cell->specs = NULL;
	}
      }
    }

    else {
      /* append connect_params to end of cell's connect_parameters */
      params = first_cell->connect_parameters;
      while (params->next != NULL)
	params = params->next;
      params->next = connect_params;
      for (y = ys; y <= ye; y++)
	for (x = xs; x <= xe; x++) {
	  seq_cell_number = (network->dim_x * (y - 1)) + x;
	  new_cell = cell_head + seq_cell_number - 1;
	  new_cell->net_connections++;
	}
    }
  }

  /* check existance of all statements  (not extern) */
  for ( i=0; i<=10; i++ ) {
    if ( cp_check[i] != ON ) {

      switch (i) {
      case 0:
	sprintf( errorstr, "from/to" );
	break;
      case 1:
	sprintf( errorstr, "projections" );
	break;
      case 2:
	sprintf( errorstr, "mapping" );
	break;
      case 3:
	sprintf( errorstr, "rf_shape" );
	break;
      case 4:
	sprintf( errorstr, "length" );
	break;
      case 5:
	sprintf( errorstr, "width" );
	break;
      case 6:
	sprintf( errorstr, "rotation" );
	break;
      case 7:
	sprintf( errorstr, "shift_x" );
	break;
      case 8:
	sprintf( errorstr, "shift_y" );
	break;
      case 9:
	sprintf( errorstr, "weight_fn" );
	break;
      case 10:
	sprintf( errorstr, "feedback" );
	break;
      }
      fprintf( stderr,
	      " WARNING in NX file: undefined connection parameter %s\n",
	      errorstr );
      fprintf( stderr,
	      "         in network <%s>, connection <%s %s>\n",
	      network->name, connect_type, connect_names );
    }

    cp_check[i] = OFF;	
  }
}

%%



/*
 *  Load initial firing rate of a network, from an assigned file.
 *  Return values: ERROR 	File could not be opened.
 *                 OK           File was successfully loaded.
 */

int
set_firing_rate_by_file( network, file_load )
    NETWORK	network;
    char 	*file_load;
{
  FILE		*fpload;	



  fpload = nxFopen( file_load );
  if (fpload == NULL) {
    printf( " ERROR in building: initial firing rate - no such file %s \n",
	   file_load );
    return ERROR;
  } else {
    printf( "        Loading Activity file into <%s>\n", network->name );
    load_activity_file( fpload, network );
  }
  return OK;
}


/* set a weights array from an assigned file	*/

int set_weights_by_file(  weights_files, file_load, major_dim, minor_dim )
	WEIGHT_ARRAY 	weights_files;
	char		*file_load;
	int 		major_dim;
	int		minor_dim;
{
  /*
   * return values: 	ERROR   file cannot open
   *			OK
   */
  FILE           *fpweights;
  int             count_weights, x, y;



  fpweights = nxFopen( file_load );
  if (fpweights == NULL) {
    printf( " ERROR in building: weight function - no such file %s  \n",
	   file_load );
    return (ERROR);
  } else {
    printf("\tLoading Connection file for <%s>\n", file_load);
    strcpy(weights_files->name, file_load);
    weights_files->list_of_weights =
      (float *) nxCalloc( major_dim * minor_dim, sizeof( float ) );

    for (y = minor_dim - 1; y >= 0; y--)
      for (x = 0; x < major_dim; x++) {
	count_weights = y * major_dim + x;

	fscanf( fpweights, "%f",
	       (weights_files->list_of_weights + count_weights) );
      }
  }

  fclose( fpweights );

  return OK;
}

int set_weights_by_const( network, aoi_range, weight_content )
	NETWORK	network;
	int	*aoi_range;
	char	*weight_content;
{
  /* 
   *	return values: 	OK
   */
	return(OK);
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
