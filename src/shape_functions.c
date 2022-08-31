/* shape_functions.c

* 	cell->voltage is updated before this file.
* 	network->scale and network->offset are applied to cell->firing_rate after this file. 

*  pgn functions for shape from texture and shading.
*  pgn functions: 	energy, 	square
			inv_shift
			normal_2/1	normal_2o1p1
			half_sample
			"ave", 		aniso_ave
			sobel, 		grad
			local_max, 	inh_by_abs
			FMM
			depth_sq_op, 	depth_df_op
			convol]ution
			multiply,  	multiply_VectAve
			phase_couple	
			delta,		set_volt_fire

		for freq. model( => _dp30 )
			LI_scale,	clamp
			"region",	long_range_slope	
			integrate,	"diff"
			global,		wipe
			"switch",	check_corr
			total_conductance
			"SRC", "SRC_fb", "LRC" , "BPC"
			auto_s, auto_ss, auto_sss
			auto_expand	= dilation by distance
			nozero_ave,	inside_ave,	region_ave
			get_voltage,	dil_freq_energy,erosion
			freq_by_thr_egy,change_orient
			2D_tune,  	Vect_Spac_Ave
			surface,	kernel_in
			1D_plot,	NakaRush
			strongest_net,	"write"
			 
			
   Some old functions are saved in shape_function_8_92.c			
*/

#include <nexus.h>
#include <nexus_pgn_includes.h>
/* #include "nexus_graphics.h"  For ver 0.8 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


/* for diameter 11 */
#define 	RANGE1_P	5
#define 	RANGE1_N	-5
#define	 	RANGE2_P	5
#define	 	RANGE2_N	-5
#define		DISTANCE5	5

/* for diameter 17 */
#define 	RANGE3_P	8
#define 	RANGE3_N	-8
#define	 	RANGE4_P	8
#define	 	RANGE4_N	-8
#define		DISTANCE8	8

/* for diameter 25 */
#define 	RANGE7_P	12
#define 	RANGE7_N	-12
#define		DISTANCE12	12

/* for diameter 33 */
#define 	RANGE5_P	16
#define 	RANGE5_N	-16
#define	 	RANGE6_P	16
#define	 	RANGE6_N	-16
#define		DISTANCE16	16

/* for diameter 41 */
#define	 	RANGE8_P	20
#define	 	RANGE8_N	-20
#define		DISTANCE20	20


#define 	OUT_OF_RANGE	-1
#define		SQRT2		1.41421356
#define		PAI		3.14159265
#define		PI		3.14159265
#define		NUM_MASKS	18		/* # of mask read from fileset, 9(18)	*/
#define		MAX_MASK_COEF	9025		/* max. # of coefficient of mask, 95x95 */
#define		MAX_MASK_WD	95		/* max. column(or row) of mask, 95	*/	
#define		MAX_WAVE	16.0		/* max. wave length(freq)		*/
#define 	MAX_ARG_LEN 	70		/* length of arguments			*/

extern  SETTINGS setting;

/* external variables valid in this file */
double	mask_coef2[NUM_MASKS][MAX_MASK_COEF];	/* mask coeficients from files  */
double	mask_scale[NUM_MASKS];
double	mask_wl[NUM_MASKS];			
int	mask_length[NUM_MASKS];
int	mask_width[NUM_MASKS];
int	num_masks;
int	c_cycle;

float beki(int number)
{
  int i;
  float result = 1;

  for(i = 0; i < number; i++)
    result *= 0.1;
  
  return result;
}

float char_to_float(char tmp_arg[70])
{
  char end[] = "\0";
  char result[10];
  char tmp[2];
  int length = 0;
  int element = 0;
  int add1 = 0;
  int add2 = 0;
  int start = 0;
  int i, j;
  float RESULT;
  
  tmp[1] = '\0';
  tmp[0] = tmp_arg[0];
  if(strcmp(tmp, "-") == 0)
    {
      element = 1;
      start = 1;
    }
  
  for(i = start; i < strlen(tmp_arg); i++){
    tmp[0] = tmp_arg[i];
    if(strcmp(tmp, ".") != 0){
      result[add1] = tmp[0];     
      add1++;
    }

    tmp[0] = tmp_arg[strlen(tmp_arg) - 1 - add2];
    if(strcmp(tmp, ".") == 0){  
      length = add2;
    }
    add2++;  
  }
  
  result[add1] = end[0];  

  if(element == 0)
    RESULT = ((float) atoi(result)) * beki(length);
  else
    RESULT = -1 * ((float) atoi(result)) * beki(length); 

  return RESULT;
}  

do_pgn_functions(cell, pgn_func, network)
	CELL            cell;
	char           *pgn_func;
	NETWORK 	network;
{

	extern SETTINGS setting;
	CONNECTION      connection, connection2, connection3;
	CONNECTION	init_connection;
	CONNECTION      get_net_connection();
	SPECS           specs;
	NETWORK         net_head, get_network_name(), get_network_id();
	NETWORK		connect_net, connect_net2, connect_net3;
	CELL            get_nearest_neighbor();
	CELL		get_neighboor();
	CELL		in_cell, in_cell2, in_cell3, in_cell_head;
	CELL		neighbor_cell, neighbor_cell2, neighbor_cell3;
	CELL		max_cell, max_cell2;
	FILE		*fp;
		
	extern double	mask_coef2[][MAX_MASK_COEF];
	extern double	mask_scale[];
	extern double	mask_wl[];				/* wave length = 1/freq */
	extern int	mask_length[];
	extern int	mask_width[];
	extern int	num_masks;
	static char	prev_file[MAX_ARG_LEN];
	static double	mask_coef[MAX_MASK_COEF];
	
	double		gaussian(), convolution(), gauss_dist(),gauss_colin(), gauss_angl(), gauss_one_dim();
	double          convolution_z();  /* new 06/20/2002 */
	double		voltage_in, energy, freq;
	double		weight, weight_sum, weight_volt;	/* scaling factor of constant from nx file */
	double		sum, sum_1, sum_2;
	double		value_x, value_y, value[MAX_MASK_WD];
	double		value2[MAX_MASK_WD][MAX_MASK_WD];	
	double		value_0, value_1, value_2;
	double		value_p, value_m;
	double 		sig, scale;
	double		gauss_coef;
	double		max_firing_rate;
	double 		in_value, in_value_2;
	double		value_p2, value_m2;
	double		dist_shift;
	double		min, max;
	double		abs_max, abs_min, max_wl;
	double		mask_out[NUM_MASKS];
	double		max_freq, max_energy, max_distance;		/* for dil_freq_energy */
	double		neighbor_value[1600], neighbor_dist[1600];	/* upto 40x40 */
	double		min_dist, min_value;
	double		ftmp;
	double		corr, shift;
	double		thrshd_shift, thrshd_conv, thrshd_go;
	double		max_conduct, src, lrc, bipc;
	double		edge;
	double		component, angle;
	double		ave_region_1, ave_region_2;
	double 		ratio;
	
	int		x_limit_low(), x_limit_high();
	int		y_limit_low(), y_limit_high();
	int		maskset_read(), distribute();
	int		number_element;
	int		i, j, k;
	int		number_connect;
	int		net_dim_x, net_dim_y;
	int		cell_pos_x, cell_pos_y;
	int		in_cell_id;
	int		position;
	int		flag_edge, flag_zero;
	int		x_low, x_high, y_low, y_high;
	
	double 		fdistance;
	int 		distance, sq_distance;
	int		range1_n, range1_p, range, range2;
	int		range2_n, range2_p;
	int		cell_count;
	int 		x_high_limit, x_low_limit;
	int		y_high_limit, y_low_limit;
	int		bias_x, bias_y;
	int		bias;
	int 		length, width;
	int		half_length, half_width;
	int		width_y;
	int		mark_count = 0;
	int		check;
	int		loop_mask, max_mask;
	int		radius, diamet;
	int		max_net;
	int		max_dist;
	int		size;
	int 		cell_count_1, cell_count_2;
	
  	char 		arg1[MAX_ARG_LEN], arg2[MAX_ARG_LEN], arg3[MAX_ARG_LEN], arg4[MAX_ARG_LEN],
       			arg5[MAX_ARG_LEN], arg6[MAX_ARG_LEN], arg7[MAX_ARG_LEN], arg8[MAX_ARG_LEN],
       			arg9[MAX_ARG_LEN], arg10[MAX_ARG_LEN], arg11[MAX_ARG_LEN], arg12[MAX_ARG_LEN],
			arg13[MAX_ARG_LEN];
	char 		pgn_func_three[4];
	char		pgn_func_range[3];
	char 		pgn_func_type[2];
	char		filename[MAX_ARG_LEN];
	char		mask_file[NUM_MASKS][MAX_ARG_LEN];
	char		flag_fw;


/* test pgn */
	if ( determine_pgn_function(pgn_func, "test") ) {
		get_argument(pgn_func, 1, arg1);
		get_argument(pgn_func, 2, arg2);
		
		cell->voltage = char_to_float(arg1);
		cell->firing_rate = char_to_float(arg2);
		
		printf(" %f \n",cell->firing_rate);
		return(OK);
	}

/* Naka-Ruchton function (sigmoid) 	*/
/* (Rmax X x**n)/(m**n + x**n) 		*/
/* Three arguments */
	if ( determine_pgn_function(pgn_func, "NakaRush") ) {
		double slope, r_max, mue;
		
		connection = cell->connect_list;
		voltage_in = (connection->input_cell)->firing_rate;
		cell->voltage = voltage_in;			/* set input value */
		
		if ( voltage_in < 0.0 ) {
			cell->firing_rate = 0.0;
			return(OK);
		}
		
		get_argument(pgn_func, 1, arg1);  /* Rmax N  	*/
		get_argument(pgn_func, 2, arg2);  /* N (=2.5) 	*/
		get_argument(pgn_func, 3, arg3);  /* mue	*/
		r_max = char_to_float(arg1);
		slope = char_to_float(arg2);
		mue = char_to_float(arg3);
		
		cell->firing_rate = (connection->conductance)*
		                    ( r_max * pow(voltage_in, slope) )/( pow(mue, slope) + pow(voltage_in, slope) );
		return(OK);
	}

/* pgn function to get one input from a connection and calculate *2 */

	if ( determine_pgn_function(pgn_func, "square") ) {
		connection = cell->connect_list;
		voltage_in = (connection->input_cell)->firing_rate;
		cell->voltage = voltage_in;			/* set input value */
		
		energy = voltage_in * voltage_in;
		weight = connection->conductance;		/* take weight from nx file */
		
		cell->firing_rate_old = cell->firing_rate;
		cell->firing_rate = energy * weight;
		
		/* printf( "# in: %lf     fire: %lf \n",voltage_in,energy); */
		return(OK);
	}

/* Delta function, output=arg3, if arg1<in<arg2 	*/

	if ( determine_pgn_function(pgn_func, "delta") ) {
		get_argument(pgn_func, 1, arg1);  /* min */
		get_argument(pgn_func, 2, arg2);  /* max */
		get_argument(pgn_func, 3, arg3);  /* value */
		
		min = char_to_float(arg1); 	max = char_to_float(arg2);	
		value_0 = char_to_float(arg3);
		
		connection = cell->connect_list;
		cell->voltage = (connection->input_cell)->firing_rate;	/* set input value */
		
		if ( ((cell->voltage) >= min)&&((cell->voltage) <= max) ) 
			cell->firing_rate = value_0;
		else
			cell->firing_rate = 0.0;
			
		return(OK);
	}
	
/* Double Delta function, out=arg3, if arg1<in<arg2; out=arg6, if arg4<=in<=arg5; otherwise out=arg7 	*/

	if ( determine_pgn_function(pgn_func, "dbl_delta") ) {
		double min_1, min_2, max_1, max_2, out_1, out_2;
		
		get_argument(pgn_func, 1, arg1);  /* min */
		get_argument(pgn_func, 2, arg2);  /* max */
		get_argument(pgn_func, 3, arg3);  /* out */
		get_argument(pgn_func, 4, arg4);  /* min */
		get_argument(pgn_func, 5, arg5);  /* max */
		get_argument(pgn_func, 6, arg6);  /* out */
		get_argument(pgn_func, 7, arg7);  /* others */
		
		min_1 = char_to_float(arg1); 	max_1 = char_to_float(arg2);
		out_1 = char_to_float(arg3);
		
		min_2 = char_to_float(arg4); 	max_2 = char_to_float(arg5);	
		out_2 = char_to_float(arg6);
		value_0 = char_to_float(arg7);
		
		connection = cell->connect_list;
		cell->voltage = (connection->input_cell)->firing_rate;	/* set input value */
		
		if ( ((cell->voltage) >= min_1)&&((cell->voltage) <= max_1) ) {
			cell->firing_rate = out_1;
		} else if (((cell->voltage) >= min_2)&&((cell->voltage) <= max_2) ) {
			cell->firing_rate = out_2;
		} else {
			cell->firing_rate = value_0;
		}
			
		return(OK);
	}

/* Inverse Linear function for 50 shifted input		*/
/* if input > 50, out =0, otherwise: out = -1x(in - 50)	*/

	if( determine_pgn_function(pgn_func,"inv_shift") ) {
		connection = cell->connect_list;
		voltage_in = (connection->input_cell)->firing_rate;
		cell->voltage = voltage_in;			/* set input value */		
		cell->firing_rate_old = cell->firing_rate;
		
		if ( voltage_in >= 50 ) {
			/* input is positive value */
			cell->firing_rate = 0.0;
		} else {
			cell->firing_rate = (connection->conductance)*(-1)*(voltage_in - 50);
		}	
		return(OK);
	}

/* Set voltage and firing_rate (for use with local_max) 	*/
/* First_net->firing to firing, second_net->firing to voltage 	*/

	if ( determine_pgn_function(pgn_func, "set_volt_fire") ) {
		connection = cell->connect_list;	/* the second connection, value to voltage 	*/
		connection2 = connection + 1;		/* the first connection, value to firing_rate   */

		weight_volt = connection->conductance;	/* take weights from nx file */
		weight      = connection2->conductance;	

		cell->firing_rate =  weight*((connection2->input_cell)->firing_rate);
		cell->voltage = weight_volt*((connection->input_cell)->firing_rate);
		
		return(OK);
	}

/* Write to display/file */
/* Default to write to terminal.  If arg1 is 'f', write/append to file in addition to terminal. */
	if ( determine_pgn_function(pgn_func, "write") ) {
		get_argument(pgn_func,1,arg1);
		get_argument(pgn_func,2,arg2);

		connection = cell->connect_list;
		cell->firing_rate = (connection->input_cell)->firing_rate;
		value_0 = (network->scale)*(cell->firing_rate) + (network->offset);	
		/* scale and offset are valid for display and file (as well as firing rate) */
		printf("                      %f \n", value_0 );	/* to display */
		
		/* write to file */	
		if ( arg1[0] == 'f' ) {
			sprintf(filename,"%s",arg2);
			if ( (fp = fopen(filename,"a+")) == NULL ) {
				fprintf(stderr," file <%s> create/append error. \n",filename );
				fclose(fp);
				return(OK);
			}
			fprintf(fp," %f \n",value_0);
			fclose(fp);
			return(OK);
		}	
		/* No write to file */
		return(OK);
	}

/* Show which net is strongest among three nets. 	*/
/* connection conductance and threshold are valid. 	*/
	if ( determine_pgn_function(pgn_func,"strongest_net") ) {
		double	disp_increment, disp_offset;
		
		get_argument(pgn_func,1,arg1);	/* diaplay increment 	*/
		get_argument(pgn_func,2,arg2);	/* display offset  	*/
		disp_increment = char_to_float(arg1);
		disp_offset = char_to_float(arg2);

		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		
		max_net = -1;		/* if no max OR max<threshold, firing_rate=0. */
		max_firing_rate = 0.0;
		
		/* a loop to change map */
		for ( i = 0; i < number_connect; i++ ) {
			connection = init_connection + i;				
			in_cell = connection->input_cell;
			in_value = (connection->conductance)*(in_cell->firing_rate);

			if ( (in_value > max_firing_rate)&&(in_value >= cell->threshold) ) {
				max_firing_rate = in_value;
				max_net = number_connect - i - 1; /* i loops from the last connection */
			}
		}
		
		if ( max_net != -1) {
			cell->voltage = max_firing_rate;
			cell->firing_rate = (disp_increment*(double)max_net) + disp_offset;
		} else {
			cell->voltage = max_firing_rate;
			cell->firing_rate = -100.0;
		}			
		return(OK);
	}
	
/* pgn function to get H-V of L-R with normalization */

	if ( determine_pgn_function(pgn_func, "normal_2/1") ) {
		connection = cell->connect_list;	/* the second connection, value to be devided 	*/
		connection2 = connection + 1;		/* the first connection, value to devide   	*/
		
		sum = (connection2->input_cell)->firing_rate;
		voltage_in =  (connection->input_cell)->firing_rate;
		cell->voltage = voltage_in ;					/* set input value */
		weight_sum = connection2->conductance;				/* take weights from nx file */
		weight_volt = connection->conductance;	
		
		cell->firing_rate_old = cell->firing_rate;
		if ( (weight_sum * sum) != 0.0 )
			cell->firing_rate = (weight_volt * voltage_in) / (weight_sum * sum);
		else	
			cell->firing_rate = 0.0;		/* if local energy sum is 0 */
		
		/* printf( "# in: %lf     fire: %lf \n",voltage_in,energy); */
		return(OK);
	}
	
/*  net2/(net1 + 1) normalization, never goes above net2 */
/* 11/30/96 */

	if ( (determine_pgn_function(pgn_func, "normal_2/1+1"))||(determine_pgn_function(pgn_func, "normal_2o1p1")) ) {
		connection = cell->connect_list;	/* the second connection, value to be devided 	*/
		connection2 = connection + 1;		/* the first connection, value to devide   	*/
		
		sum = (connection2->input_cell)->firing_rate;
		voltage_in =  (connection->input_cell)->firing_rate;
		cell->voltage = voltage_in ;					/* set input value */
		weight_sum = connection2->conductance;				/* take weights from nx file */
		weight_volt = connection->conductance;	
		
		cell->firing_rate_old = cell->firing_rate;
		cell->firing_rate = (weight_volt * voltage_in) / (weight_sum * sum + 1.0);
		/* printf( "# in: %lf     fire: %lf \n",voltage_in,energy); */
		return(OK);
	}
	
/* pgn function to make a shrinked(1/2) image by sampling a pixel located at (2x,2y) */
/* "connect from" network should have 2x size */

	if ( determine_pgn_function(pgn_func, "half_sample") ) {
		connection = cell->connect_list;
		connect_net = get_network_id( (connection->input_cell)->net_id );	/* connected network head */
		
		network = get_network_id( cell->net_id );				/* network itself */
		net_dim_x = network->dim_x;			
		net_dim_y = network->dim_y;
		
		/* get current cell position (cell_pos_x,cell_pos_y) , then calc connected in_cell_id 	*/
		
		cell_pos_x = (int)fmod( (double)cell->id , (double)net_dim_x );
		if ( cell_pos_x == 0 ) 				
			cell_pos_x = net_dim_x;			/*  x(also y) = 1,2,3, ... ,net_dim_? 	*/
		cell_pos_y = ((cell->id - 1) / net_dim_x) + 1;	
		in_cell_id = (cell_pos_x * 2) + ((cell_pos_y * 2) - 1)*(net_dim_x * 2);
	
		in_cell_head = connect_net->cells;
		voltage_in = (in_cell_head + in_cell_id - 1)->firing_rate;
					/* same as : voltage_in = ((connect_net->cells) + in_cell_id - 1 )->firing_rate; */
		cell->voltage = voltage_in ;			/* set input value 	*/
		weight_volt = connection->conductance;	
		
		cell->firing_rate_old = cell->firing_rate;
		cell->firing_rate = weight_volt * voltage_in;
		
		/*printf( "# cell->id, net_dim_x: %d, %d,   cell_pos_x,y: %d, %d,    in_cell_id: %d\n", 
			cell->id, net_dim_x, cell_pos_x, cell_pos_y, in_cell_id); */
		return(OK);
	}
	
/* pgn function to operat Sobel's filter 	*/
/* pgn function to operat Sobel's filter 5x5 	*/
/* pgn function to operat Sobel's filter 7x7 	*/
/* pgn function to average 			*/

/* multiplication */
	if ( determine_pgn_function(pgn_func, "multiply") ) {
		
		/* if normalized from connection, first row & column cells may not have 2 connections.(Nexus bug) */
		number_connect = cell->number_connections;
		if ( number_connect < 2 ) {
			cell->voltage = 0.0;
			cell->firing_rate =0.0;
			return(OK);
		}
		
		connection2 = cell->connect_list;		/* second connection	 */
		in_cell2 = connection2->input_cell;	
		connect_net2 = get_network_id(in_cell2->net_id);
		
		connection = connection2 + 1;			/* first connection	 */
		in_cell = connection->input_cell;	
		connect_net = get_network_id(in_cell->net_id);
		
		cell->voltage = (connection->conductance)*(in_cell->firing_rate) * 
				(connection2->conductance)*(in_cell2->firing_rate);
		cell->firing_rate = cell->voltage;
		
		return(OK);
	}
/* multiplication */
	if ( determine_pgn_function(pgn_func, "multiply_p") ) {
		
		/* if normalized from connection, first row & column cells may not have 2 connections.(Nexus bug) */
		number_connect = cell->number_connections;
		if ( number_connect < 2 ) {
			cell->voltage = 0.0;
			cell->firing_rate =0.0;
			return(OK);
		}
		
		connection2 = cell->connect_list;		/* second connection	 */
		in_cell2 = connection2->input_cell;	
		connect_net2 = get_network_id(in_cell2->net_id);
		
		connection = connection2 + 1;			/* first connection	 */
		in_cell = connection->input_cell;	
		connect_net = get_network_id(in_cell->net_id);
		
		cell->voltage = (connection->conductance)*(in_cell->firing_rate) * 
				(connection2->conductance)*(in_cell2->firing_rate);
		cell->firing_rate = cell->voltage;
		
		return(OK);
	}
/* multiplication for VectSpacAve */
/* if input angle (1st connection in nx) < threshold, OR region(2nd) <= 0.0, then output -999 */
	if ( determine_pgn_function(pgn_func, "multiply_VectAve") ) {
		
		/* if normalized from connection, first row & column cells may not have 2 connections.(Nexus bug) */
		number_connect = cell->number_connections;
		if ( number_connect < 2 ) {
			cell->voltage = 0.0;
			cell->firing_rate =0.0;
			return(OK);
		}
		
		connection2 = cell->connect_list;		/* second connection	 */
		in_cell2 = connection2->input_cell;	
		connect_net2 = get_network_id(in_cell2->net_id);
		
		connection = connection2 + 1;			/* first connection	 */
		in_cell = connection->input_cell;	
		connect_net = get_network_id(in_cell->net_id);
		
		if ( ((in_cell->firing_rate) > (cell->threshold))&&((in_cell2->firing_rate) > 0.0) ) {
			/* if angle(1) > threshold, AND region(2) > 0, multiply. */
			cell->voltage = (connection->conductance)*(in_cell->firing_rate) * 
					(connection2->conductance)*(in_cell2->firing_rate);
			cell->firing_rate = cell->voltage;		
		} else {
			/* if angle(1) < threshold, OR region(2) <= 0,  output -999. */
			cell->voltage = 0.0;
			cell->firing_rate = -999.9/(network->scale);	
		}
		return(OK);
	}

/*  Compute scaling factor for LI.  Use (own - opp.)/own for input for scaling function.	*/
/* If own_input < threshold, output 0.0. 	*/
/* 1st connection: own 			*/
/* 2nd connection: other 		*/

	if ( determine_pgn_function(pgn_func, "LI_scale") ) {
		double	mean_value_, mean_value_2;
		double	input, output;
		double	mean_own, mean_opp;
		
		get_argument(pgn_func, 1, arg1);		/*  min scaling factor			*/
		get_argument(pgn_func, 2, arg2);		/*  max. scaling factor			*/
		get_argument(pgn_func, 3, arg3);		/*  slope of scaling function		*/
		get_argument(pgn_func, 4, arg4);		/*  y interception of scaling function	*/
		get_argument(pgn_func, 5, arg5);		/*  diameter for AOI			*/
		
		/* if normalized from connection, first row & column cells may not have 2 connections.(Nexus bug) */
		number_connect = cell->number_connections;
		if ( number_connect < 2 ) {
			cell->voltage = 0.0;
			cell->firing_rate =0.0;
			return(OK);
		}
		connection2 = cell->connect_list;		/* second connection (other)	 */
		in_cell2 = connection2->input_cell;	
		connect_net2 = get_network_id(in_cell2->net_id);
		
		connection = connection2 + 1;			/* first connection  (own)	 */
		in_cell = connection->input_cell;	
		connect_net = get_network_id(in_cell->net_id);
		
		if ( in_cell->firing_rate < cell->threshold ) {	/* if own < thrshd, out 0.0; to eliminate outside */
			cell->voltage = 0.0;
			cell->firing_rate =0.0;
			return(OK);
		}
		
		range1_p = atoi(arg5)/2;
		range1_n = -range1_p;
		range2_p = range1_p;
		range2_n = -range1_p;
		fdistance = range1_p;
		
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* the cell of interest is not at edge */
			/* x,y low and high limit due to edge */
			x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
			x_high_limit = x_limit_high( connect_net, in_cell, range1_p );
			y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
			y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
			
			sum_1 = 0.0;   sum_2 = 0.0;    cell_count = 0;
			for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
			for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
				if (((bias_x * bias_x) + (bias_y * bias_y)) <= (fdistance * fdistance)) {
					/* within distance */
					neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
					neighbor_cell2 = in_cell2 + (bias_y * connect_net2->dim_x + bias_x);
					sum_1 += (connection->conductance)*(neighbor_cell->firing_rate);
					sum_2 += (connection2->conductance)*(neighbor_cell2->firing_rate);
					cell_count ++;
				}
			}}
			mean_own = sum_1 / (double)cell_count;
			mean_opp = sum_2 / (double)cell_count;
			
			if ( mean_own != 0.0 ) {
				input = (mean_own - mean_opp)/mean_own;
				output = char_to_float(arg3)*input + char_to_float(arg4);
				if ( output > char_to_float(arg2) )		/* clamp to max */
					output = char_to_float(arg2);
				if ( output < char_to_float(arg1) )		/* clamp to min */
				  output = char_to_float(arg1);
				cell->voltage = input;
				cell->firing_rate = output;
			} else {
				cell->voltage = 0.0;
				cell->firing_rate = 0.0;
			}		
			
		} else {
			/* if cell is on a edge */
		     	cell->voltage = 0.0;
		     	cell->firing_rate = 0.0;
		}
		return(OK);
	}

/* if input</>threshold, change firing_rate of cells into assgined value(arg[2]). */
/* arg[1]: more or less								*/
	if ( determine_pgn_function(pgn_func, "clamp") ) {
		get_argument(pgn_func, 1, arg1);	/* < or > 	*/
		get_argument(pgn_func, 2, arg2);	/* clamp value 	*/

		connection = cell->connect_list;
		in_cell = connection->input_cell;

		if (arg1[0] == 'm') {
			if (in_cell->firing_rate > cell->threshold) {		/* if input > threshold, clamp */
				cell->voltage = in_cell->firing_rate;
				cell->firing_rate = char_to_float(arg2);
			} else {
				cell->voltage = in_cell->firing_rate;
				cell->firing_rate = in_cell->firing_rate;
			}
		} else {
			if (in_cell->firing_rate < cell->threshold) {		/* default: if input < threshold, clamp */
				cell->voltage = in_cell->firing_rate;
				cell->firing_rate = char_to_float(arg2);
			} else {
				cell->voltage = in_cell->firing_rate;
				cell->firing_rate = in_cell->firing_rate;
			}
		}
		return (OK);
	}

/*  mark the region of 2nd connection having close value to 1st connection. (for Fo_Region)	*/
/*  outside shunt by 3rd connection.		*/
	if ( determine_pgn_function(pgn_func, "region") ) {
		double	mean_value;
		
		get_argument(pgn_func, 1, arg1);		/* considering region; range(width=height)		*/
		get_argument(pgn_func, 2, arg2);		/* +-range to be considered as same(as min or max.)   	*/
		
		/* if normalized from connection, first row & column cells may not have 2 connections.(Nexus bug) */
		number_connect = cell->number_connections;
		if ( number_connect < 3 ) {
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
			return(OK);
		}
		connection3 = cell->connect_list;		/* third connection (Inside)	 */
		in_cell3 = connection3->input_cell;	
		connect_net3 = get_network_id(in_cell3->net_id);
		
		connection2 = connection3 + 1;			/* second connection (sum(LAS_H,V,L,R))	 */
		in_cell2 = connection2->input_cell;	
		connect_net2 = get_network_id(in_cell2->net_id);
		
		connection = connection2 + 1;			/* first connection  (Global Min)	 */
		in_cell = connection->input_cell;	
		connect_net = get_network_id(in_cell->net_id);
		
		if ( in_cell3->firing_rate > 0.0 ) {		/* Outside */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
			return(OK);
		}
		
		/* if (second connection) > (first connection +- arg3), assgin arg4 */
		range1_p = atoi(arg1);
		range1_n = -range1_p;
		range2_p = range1_p;
		range2_n = -range1_p;
		fdistance = range1_p;
		
		if (not_at_edge(in_cell2->id, in_cell2->net_id, connect_net2->name) == TRUE) {
			/* a cell of interest is not at edge */
			/* x,y low and high limit due to edge */
			x_low_limit = x_limit_low( connect_net2, in_cell2, range1_n );
			x_high_limit = x_limit_high( connect_net2, in_cell2, range1_p );
			y_low_limit  = y_limit_low( connect_net2, in_cell2, range2_n );
			y_high_limit = y_limit_high( connect_net2, in_cell2, range2_p );
			
			sum = 0.0;   cell_count = 0;
			for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
			for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
				if (((bias_x * bias_x) + (bias_y * bias_y)) <= (fdistance * fdistance)) {
					/* within distance */
					neighbor_cell3 = in_cell3 + (bias_y * connect_net3->dim_x + bias_x);
					if ( neighbor_cell3->firing_rate == 0.0 ) {
						/* Inside */
						neighbor_cell = in_cell2 + (bias_y * connect_net2->dim_x + bias_x);
						sum += (connection2->conductance)*(neighbor_cell->firing_rate);
						cell_count ++;
					}
				}
			}}
			mean_value = sum / (double)cell_count;
			if ( (mean_value > ((in_cell->firing_rate) - char_to_float(arg2))) &&
			     (mean_value < ((in_cell->firing_rate) + char_to_float(arg2))) ) {
			     	cell->voltage = mean_value;
			     	cell->firing_rate = mean_value;		/* if close to min, out mean(~min) value */
			} else {
			     	cell->voltage = mean_value;
			     	cell->firing_rate = 0.0;		/* if not close to min, out 0.0 */
			}
		} else {
			/* if cell is on a edge */
		     	cell->voltage = 0.0;
		     	cell->firing_rate = 0.0;
		}
		return(OK);
	}

/* Slant for shape						*/
/* Computes (In1-In2)/(In1 or 2)  ;(Fx-Fo)/Fo			*/
/* arg1: deviator: 1st or 2nd connection			*/	

	if ( determine_pgn_function(pgn_func, "diff") ) {
		double          in_fire;
		double		in_1, in_2;
		int             flag_norm = 0;

		get_argument(pgn_func, 1, arg1);		/* denomiator connection 1 or 2  	 */
		get_argument(pgn_func, 2, arg2);		/* for compensating bug(don't get connections)   */
								/* assign denominator.			*/

		/* if normalized-from connection, first row & column cells may not have 2 connections. */
		number_connect = cell->number_connections;
		if ( number_connect < 1 ) {
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
			return(OK);
		}
		if ( number_connect == 1 ) {			/* BUG COUNTER-MEASURE: normalized connection  */
			connection = cell->connect_list;	/*  connection Fx	 */
			in_cell = connection->input_cell;
			connect_net = get_network_id(in_cell->net_id);

			cell->voltage = in_cell->firing_rate - char_to_float( arg2 );	/* Fx - Fo 	 */
			in_1 = in_cell->firing_rate;
			in_2 = char_to_float( arg2 );			/* use arg2 for Fo, if single connection */
		
		}
		
		if (number_connect == 2) {
			connection2 = cell->connect_list;	/* second connection Fo	 */
			in_cell2 = connection2->input_cell;
			connect_net2 = get_network_id(in_cell2->net_id);

			connection = connection2 + 1;		/* first connection  Fx	 */
			in_cell = connection->input_cell;
			connect_net = get_network_id(in_cell->net_id);

			cell->voltage = in_cell->firing_rate - in_cell2->firing_rate;	/* Fx - Fo 	 */
			in_1 = in_cell->firing_rate;
			in_2 = in_cell2->firing_rate;
		}
		
		if (arg1[0] == '1') {
			if (in_1 != 0.0)
				cell->firing_rate = (cell->voltage) / in_1;	/* (In1 -In2)/In1 */
			else
				cell->firing_rate = 1000.0;
		} 
		if (arg1[0] == '2') { 
			if (in_2 != 0.0)
				cell->firing_rate = (cell->voltage) / in_2;	/* (In1 - In2)/In2 */
			else							/* (Fx-Fo)/Fo	   */
				cell->firing_rate = 1000.0;
		}										
		return(OK);
	}
	
	
/* Determines the orientation(8) of steepest descent by searching large region	*/
/* (currently, direction of smallest freq.) 			*/
/* IGNORE if input j< threshold (get rid of outside effects)	*/
	if (determine_pgn_function(pgn_func, "long_range_slope")) {
		double	in_fire;
		double	mean_value;
		int	min_direction;
		int	count[8];
		
		get_argument(pgn_func, 1, arg1);	/* a range to be search  	 		 */
		get_argument(pgn_func, 2, arg2);	/* length limit; if shorter than this, ignore  	 */
		range = atoi(arg1);
		range2 = range*range;
		range1_p = range;
		range1_n = -range1_p;
		range2_p = range1_p;
		range2_n = -range1_p;
		fdistance = range1_p;
		
		connection = cell->connect_list;
		in_cell = connection->input_cell;
		connect_net = get_network_id(in_cell->net_id);
		
		/* if direct input < threshold, out=0.0 */
		if ( in_cell->firing_rate < cell->threshold ) {
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
			return(OK);
		}
		
		/* grow dendrites to 8 directions for length of arg[1] or hit an edge.	*/
		/* store total values along each direction. 				*/
		
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* a cell of interest is not at edge */
			/* x,y low and high limit due to edge */
			x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
			x_high_limit = x_limit_high( connect_net, in_cell, range1_p );
			y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
			y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
			/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n",
					in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit); */
					
			for ( i = 0; i < 8; i++ ){
				neighbor_value[i] = 0.0;
				count[i] = 0;
				switch (i) {
				case 0:	/* V up */
			  		for (bias_y = 1; bias_y <= y_high_limit; bias_y++) {
						neighbor_cell = in_cell + (bias_y * connect_net->dim_x);
						if ( neighbor_cell->firing_rate >= cell->threshold ) {
						    neighbor_value[i] += (connection->conductance)*(neighbor_cell->firing_rate);
						    count[i]++;
					}	}
					break;
					
				case 1: /* V down */
			  		for (bias_y = y_low_limit; bias_y < 0; bias_y++) {
						neighbor_cell = in_cell + (bias_y * connect_net->dim_x);
						if ( neighbor_cell->firing_rate >= cell->threshold ) {
							neighbor_value[i] +=  (connection->conductance)*(neighbor_cell->firing_rate);
							count[i]++;
					}	}
					break;
					
				case 2:	/* R up	*/
			  		for (bias_x = 1; bias_x <= x_high_limit; bias_x++) {
			  			if ( (bias_y = bias_x) <= y_high_limit ){
			  			    if ( (bias_x*bias_x + bias_y*bias_y)<=range2 ) {
						      neighbor_cell = in_cell + (bias_y * connect_net->dim_x) + bias_x;
						      if ( neighbor_cell->firing_rate >= cell->threshold ) {
							  neighbor_value[i] += (connection->conductance)*(neighbor_cell->firing_rate);
						    	  count[i]++;
					}	}  }  }
					break;
					
				case 3: /* R down */
			  		for (bias_x = x_low_limit; bias_x < 0; bias_x++) {
			  			if ( (bias_y = bias_x) >= y_low_limit ){
			  			    if ( (bias_x*bias_x + bias_y*bias_y)<=range2 ) {
						      neighbor_cell = in_cell + (bias_y * connect_net->dim_x) + bias_x;
						      if ( neighbor_cell->firing_rate >= cell->threshold ) {
						    	  neighbor_value[i] += (connection->conductance)*(neighbor_cell->firing_rate);
						    	  count[i]++;
					}	}  }  }
					break;
					
				case 4: /* H right */
			  		for (bias_x = 1; bias_x <= x_high_limit; bias_x++) {
						neighbor_cell = in_cell + bias_x;
						if ( neighbor_cell->firing_rate >= cell->threshold ) {
							neighbor_value[i] += (connection->conductance)*(neighbor_cell->firing_rate);
							count[i]++;
					}	}
					break;
					
				case 5: /* H left */
			  		for (bias_x = x_low_limit; bias_x < 0; bias_x++) {
						neighbor_cell = in_cell + bias_x;
						if ( neighbor_cell->firing_rate >= cell->threshold ) {
							neighbor_value[i] += (connection->conductance)*(neighbor_cell->firing_rate);
							count[i]++;
					}	}
					break;

				case 6: /* L up */
			  		for (bias_x = x_low_limit; bias_x < 0 ; bias_x++) {
			  			if ( (bias_y = -bias_x) <= y_high_limit ){
			  			   if ( (bias_x*bias_x + bias_y*bias_y)<=range2 ) {
						      neighbor_cell = in_cell + (bias_y * connect_net->dim_x) + bias_x;
						      if ( neighbor_cell->firing_rate >= cell->threshold ) {
						    	  neighbor_value[i] += (connection->conductance)*(neighbor_cell->firing_rate);
						    	  count[i]++;
					}	}  }  }
					break;
				case 7: /* L down */
			  		for (bias_x = 1; bias_x <= x_high_limit; bias_x++) {
			  			if ( (bias_y = -bias_x) >= y_low_limit ){
			  			   if ( (bias_x*bias_x + bias_y*bias_y)<=range2 ) {
						      neighbor_cell = in_cell + (bias_y * connect_net->dim_x) + bias_x;
						      if ( neighbor_cell->firing_rate >= cell->threshold ) {
						    	  neighbor_value[i] += (connection->conductance)*(neighbor_cell->firing_rate);
						    	  count[i]++;
					}	}  }  }
					break;
				}
			}
			
			/* find steepest descent(currently, direction of smallest freq.) from stored 8 values.	*/
			min = 10000.0;
			min_direction = -1;
			for ( j = 0; j < 8; j++ ) {
				if ( count[j] != 0 ) {
					if ( count[j] >= atoi(arg2) ) {
						if ( ( mean_value=(neighbor_value[j] / count[j]) ) < min ) {
							min = mean_value;		/* min: direction of lowest freq. 	*/
							min_direction = j;		/*      probably more sophisticated.   	*/
						}
					}
					/* if length too short, ignored. */
				}
				/* if count=0 (eg. outside), ignored. */
			}
		} else {
			/* self is on an edge */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
			return(OK);
		}
		

		/* output the direction of min. freq.  This table is as same as pgn(change_orient) */
		switch (min_direction) {
			case 0 :  cell->firing_rate = 20;	/* Von  */
				  cell->voltage = min;
				  break;
				    
			case 1 :  cell->firing_rate = 25;	/* Voff */
				  cell->voltage = min;
				  break;
				    
			case 2 :  cell->firing_rate = 30;	/* Ron  */
				  cell->voltage = min;
				  break;
				    
			case 3 :  cell->firing_rate = 35;	/* Roff */
				  cell->voltage = min;
				  break;
				    
			case 4 :  cell->firing_rate = 50;	/* Hon  */
				  cell->voltage = min;
				  break;
				    
			case 5 :  cell->firing_rate = 55;	/* Hoff */
				  cell->voltage = min;
				  break;
				    
			case 6 :  cell->firing_rate = 60;	/* Lon  */
				  cell->voltage = min;
				  break;
				    
			case 7 :  cell->firing_rate = 65;	/* Loff */
				  cell->voltage = min;
				  break;
				    
			default:  cell->firing_rate = 0.0;    /* may be 8; all input was 0.0 */
				  cell->voltage = 0.0;
				  break;
		}

		return (OK);
	}

/* Integration from Fo, in the direction of steepest descend						*/	
/* Bo = Ao + B(min_Ai) , where Bo is own, Ao is direct cell in LM_Freq. map, Bi is surrounding cells. 	*/
/* along max slant(steepest descent) 							*/
/* output arg[2] if input < threshold (get rid of outside effects), or on edges.	*/
/* clamp value is insensitive to network scaling factor.				*/

	if (determine_pgn_function(pgn_func, "integrate")) {
		double	in_fire;
		double	clamp_value;
		
		get_argument(pgn_func, 1, arg1);	/* clamp value for outside		 */
		
		clamp_value = char_to_float(arg1);
		if ( (cell->number_connections) != 3 ) {
			if ( (cell->id) <= 5 )
				printf(" ERROR in pgn(integrate): # of connections != 3\n ");
			return(OK);
		}

		connection3 = cell->connect_list;		/* third connection (Fo_region) */
		in_cell3 = connection3->input_cell;	
		connect_net3 = get_network_id(in_cell3->net_id);
		
		connection2 = connection3 + 1;			/* second connection (LngRng_Slope_Ori)	 */
		in_cell2 = connection2->input_cell;	
		connect_net2 = get_network_id(in_cell2->net_id);
		
		connection = connection2 + 1;			/* first connection  (LM_Freq)	 */
		in_cell = connection->input_cell;	
		connect_net = get_network_id(in_cell->net_id);
		
		/* if at the edge, clamp to arg[1]. */
		if (not_at_edge(cell->id, cell->net_id, network->name) == FALSE) {
			cell->voltage = 0.0;
			cell->firing_rate = clamp_value * (1.0/network->scale);
			return(OK);
		}

		/* if input(LM_Freq) < threshold, clamp to arg[1].  It may be outside. */
		if ( in_cell2->firing_rate < cell->threshold ) {
			cell->voltage = 0.0;		
			cell->firing_rate = clamp_value * (1.0/network->scale);
			return(OK);
		}
		
		/* Fo region, No integration */
		if ( (in_cell3->firing_rate) > 0.0) {	
			cell->voltage = in_cell3->firing_rate;
			cell->firing_rate = (connection3->conductance)*(in_cell3->firing_rate);  /* should be Fo, not 0.0 */
			return(OK);
		}
		
		/* find direction of integration from LngRng_Slope_Ori(pgn-long_range_slope) 	*/
		/* and Bo=Ao(LM_Freq) + B(min_Ai) 						*/
		if ( (in_cell2->firing_rate) == 20.0 ) { 	/* V on */
			cell->voltage = in_cell->firing_rate;
			cell->firing_rate = in_cell->firing_rate + get_nearest_neighbor(cell, network, 6)->firing_rate;
			return(OK);
		}
		if ( (in_cell2->firing_rate) == 25.0 ) {	/* V off */
			cell->voltage = in_cell->firing_rate;
			cell->firing_rate = in_cell->firing_rate + get_nearest_neighbor(cell, network, 1)->firing_rate;
			return(OK);
		}
		if ( (in_cell2->firing_rate) == 30.0 ) {	/* R on */
			cell->voltage = in_cell->firing_rate;
			cell->firing_rate = SQRT2*in_cell->firing_rate + get_nearest_neighbor(cell, network, 7)->firing_rate;
			return(OK);
		}
		if ( (in_cell2->firing_rate) == 35.0 ) {	/* R off */
			cell->voltage = in_cell->firing_rate;
			cell->firing_rate = SQRT2*in_cell->firing_rate + get_nearest_neighbor(cell, network, 0)->firing_rate;
			return(OK);
		}
		if ( (in_cell2->firing_rate) == 50.0 ) {	/* H on */
			cell->voltage = in_cell->firing_rate;
			cell->firing_rate = in_cell->firing_rate + get_nearest_neighbor(cell, network, 4)->firing_rate;
			return(OK);
		}
		if ( (in_cell2->firing_rate) == 55.0 ) {	/* H off */
			cell->voltage = in_cell->firing_rate;
			cell->firing_rate = in_cell->firing_rate + get_nearest_neighbor(cell, network, 3)->firing_rate;
			return(OK);
		}
		if ( (in_cell2->firing_rate) == 60.0 ) {	/* L on */
			cell->voltage = in_cell->firing_rate;
			cell->firing_rate = SQRT2*in_cell->firing_rate + get_nearest_neighbor(cell, network, 5)->firing_rate;
			return(OK);
		}
		if ( (in_cell2->firing_rate) == 65.0 ) {	/* L off */
			cell->voltage = in_cell->firing_rate;
			cell->firing_rate = SQRT2*in_cell->firing_rate + get_nearest_neighbor(cell, network, 2)->firing_rate;
			return(OK);
		}
		cell->voltage = 0.0;		/* if LngRng_SO is undetermined, out = 0 */
		cell->firing_rate = 0.0;
		return(OK);
	}

/* Global computation 						*/	
/* compute global min or max of all connected networks. 	*/
/* NOTE: Currently, simply taking min or max of all connected cell at cell #1, copy it for rest. */
/*       connection type, size, etc are all ignored.						*/
/*       INPUT NET MUST BE THE SAME or LARGER SIZE.  If input is smaller and connection is normalized, */
/*       the 1st cell will not be connected(NEXUS Bug.)						       */
/* Threshold: if < thrshd, ignore.  Thus, firing_rate >= threshold				*/
	
	if ( determine_pgn_function(pgn_func, "global") ) {
		double  in_fire;
		float 	test_fire;
		double	test_fire2;

		get_argument(pgn_func, 1, arg1);	/* global min or max  			 */
		get_argument(pgn_func, 2, arg2);	/* size of RF for each cell		 */

		if (cell->id == 1) {
			min = 10000.0;
			max = -10000.0;
			init_connection = cell->connect_list;		/* get 2 info from 1st cell */
			number_connect = cell->number_connections;	

			/* a loop to change maps */
			for (i = 0; i < number_connect; i++) {
				/* if not at edge */
				connection = init_connection + i;
				connect_net = get_network_id( (connection->input_cell)->net_id );
				in_cell = connect_net->cells;	/* in_cell is the 1st cell of a connected net. */

				/* a loop to search all cells in the network */
				for (j = 0; j < (connect_net->number_cells); j++) {
					in_fire = (in_cell + j)->firing_rate;
					test_fire = (in_cell + j)->firing_rate;
					test_fire2 = (in_cell + j)->firing_rate;
					/*  (connection->conductance) *  */
					if ( in_fire >= (cell->threshold) ) {	 /* ignore  < threshold */
						if (in_fire > max)
							max = in_fire;
						if (in_fire < min)
							min = in_fire;
				}	}
			}
			if (arg1[2] == 'n') {
				cell->voltage = min;
				if (min > cell->threshold)
					cell->firing_rate = min;
				else
					cell->firing_rate = cell->threshold;	/* must be > threshold */

			} else { 	/* default is max */
				cell->voltage = max;
				if (max > cell->threshold)
					cell->firing_rate = max;
				else
					cell->firing_rate = cell->threshold;	/* must be > threshold */
			}
		}
		if (cell->id > 1) {
			cell->voltage = (network->cells)->voltage;
			cell->firing_rate = (network->cells)->firing_rate;
		}
		return(OK);
	}
	
		
/* General Local Max. */	
/* local maximum of all connected maps within assigned region 					*/
/* negative input is neglected( set to 0 ) , but weight function(const) is valid for conversion */

	if ( determine_pgn_function(pgn_func,"local_max") ) {
		double	input_scale, input_shift;
		double	in_fire, in_volt;
		double 	max_fire, abs_max_fire;
		double 	max_volt, abs_max_volt;
		
		get_argument(pgn_func,1,arg1);		/* size  			*/
		get_argument(pgn_func,2,arg2);		/* scale input			*/
		get_argument(pgn_func,3,arg3);		/* shift input			*/
		get_argument(pgn_func,4,arg4);		/* use sign / abs for comparison	    	*/
		get_argument(pgn_func,5,arg5);		/* use firing_rate / voltage for comparison 	*/
		get_argument(pgn_func,6,arg6);		/* use sign / abs for output 			*/
		get_argument(pgn_func,7,arg7);		/* use firing_rate / voltage for output	    	*/
		
		width = atoi( arg1 );
		half_length = width / 2;
		half_width = width / 2;

		range1_n = -half_width;
		range1_p = half_width;
		range2_n = -half_length;
		range2_p = half_length;
		fdistance = half_width; 
		
		input_scale = char_to_float(arg2);
		input_shift = char_to_float(arg3);
		
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		in_cell = init_connection->input_cell;
		connect_net = get_network_id(in_cell->net_id);		
		/* fprintf( stderr,"init # connect %d\n",number_connect); */ 	
		
		max_fire = 0.0;
		abs_max_fire = 0.0;
		max_volt = 0.0;
		abs_max_volt = 0.0;
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* a cell of interest is not at edge */
			/* a loop to change map */
			for ( i = 0; i < number_connect; i++ ) {
				/* if not at edge */
				connection = init_connection + i;				
				in_cell = connection->input_cell;
				connect_net = get_network_id(in_cell->net_id);	
					
				/* x,y low and high limit due to edge */
				x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
				x_high_limit = x_limit_high( connect_net, in_cell, range1_p );
				y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
				y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
				/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n",
						in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit); */

				for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
				for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
					if (((bias_x * bias_x) + (bias_y * bias_y)) <= (fdistance * fdistance)) {
						  /* within distance */
						neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
						in_fire = input_scale*(connection->conductance)*(neighbor_cell->firing_rate) 
							  - input_shift;
						in_volt = input_scale*(connection->conductance)*(neighbor_cell->voltage) - input_shift;
					
						if ( arg5[0] != 'v' ) {				/* comparison by firing_rate 	*/
							if ( arg4[0] != 'a' ) {
								if ( in_fire > max_fire ) {	/* comparison by signed 	*/
									max_fire = in_fire;
									max_volt = in_volt;
								}
							} else {
								if ( fabs(in_fire) > abs_max_fire ) { /* comparison by abs    	*/
						   			abs_max_fire =  fabs(in_fire);
						   			max_fire = in_fire;
						   			max_volt = in_volt;
						   	}	}
						 } else {					/* comparison by voltage	*/
						 	if ( arg4[0] != 'a' ) {
								if ( in_volt > max_volt ) {	/* comparison by signed 	*/
									max_fire = in_fire;
									max_volt = in_volt;
								}
							} else {
								if ( fabs(in_volt) > abs_max_volt ) { /* comparison by abs    	*/
						   			abs_max_volt =  fabs(in_volt);
									max_fire = in_fire;
						   			max_volt = in_volt;
						 }  	}	}
					}
				}}
			}
			/* output is always >= 0 , because max_fire never go below 0 	*/
			if ( arg7[0] != 'v' ) {				/* output  firing_rate */
				if ( arg6[0] != 'a' ) {			/* by signed 	*/
					cell->voltage = max_volt;
					cell->firing_rate = max_fire;
				} else {				/* by abs 	*/
					cell->voltage = max_volt;
					cell->firing_rate = fabs( max_fire );
				}
			} else {					/* output  voltage 	*/
				if ( arg6[0] != 'a' ) {			/* by signed 	*/
					cell->voltage = max_fire;
					cell->firing_rate = max_volt;
				} else {				/* by abs 	*/
					cell->voltage = max_fire;
					cell->firing_rate = fabs( max_volt );
			}	}	
		} else {
			/* all 4 peripheral edges */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}
/* local maximum of all connected maps of the exact location (1x1)			*/
/* negative input is neglected(set to 0) , but weight function(const) is valid		*/

	if ( strcmp(pgn_func, "local_max_1x1") == 0 ) {		
		
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		/* fprintf( stderr,"init # connect %d\n",number_connect); */ 	
		
		max_firing_rate = 0.0;
		/* a loop to change map */
		for ( i = 0; i < number_connect; i++ ) {
			connection = init_connection + i;				
			in_cell = connection->input_cell;
					
			in_value = (connection->conductance)*(in_cell->firing_rate);
			if ( in_value > max_firing_rate )
				max_firing_rate = in_value;
		}
		/* output is always >= 0 , because max_firing_rate never go below 0 */
		cell->voltage = max_firing_rate;
		cell->firing_rate = max_firing_rate;
		
		return(OK);
	}

/* local maximum of all connected maps within region 3x3 					*/
/* negative input is neglected( set to 0 ) , but weight function(const) is valid for conversion */
/* for f1, distance=1.5; f2, distance=2.25; f3, distance=3					*/

	if ( strcmp(pgn_func, "local_max_3x3") == 0 ) {		
		/* determine region , 3x3 */
		range1_n = -1;
		range1_p = 1;
		range2_n = -1;
		range2_p = 1;
		fdistance = 1.5; 
		
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		in_cell = init_connection->input_cell;
		connect_net = get_network_id(in_cell->net_id);		
		/* fprintf( stderr,"init # connect %d\n",number_connect); */ 	
		
		max_firing_rate = 0.0;
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* a cell of interest is not at edge */
			/* a loop to change map */
			for ( i = 0; i < number_connect; i++ ) {
				/* if not at edge */
				connection = init_connection + i;				
				in_cell = connection->input_cell;
				connect_net = get_network_id(in_cell->net_id);	
					
				/* x,y low and high limit due to edge */
				x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
				x_high_limit = x_limit_high( connect_net, in_cell, range1_p );
				y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
				y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
				/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n",
						in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit); */

				for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
					for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
						if (((bias_x * bias_x) + (bias_y * bias_y)) <= (fdistance * fdistance)) {
						   /* within distance */
						   neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
						   in_value = ((connection->conductance)*(neighbor_cell->firing_rate));
						   if ( in_value > max_firing_rate ) 
							max_firing_rate = in_value;
						}
					}
				}
			}
			/* output is always >= 0 , because max_firing_rate never go below 0 */
			cell->voltage = max_firing_rate;
			cell->firing_rate = max_firing_rate;
		} else {
			/* all 4 peripheral edges */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}
	
/* local maximum of all connected maps within region 5x5 (for f1.5)				*/
/* negative input is neglected( set to 0 ) , but weight function(const) is valid for conversion */

	if ( strcmp(pgn_func, "local_max_5x5") == 0 ) {		
		/* determine region , 5x5 */
		range1_n = -2;
		range1_p = 2;
		range2_n = -2;
		range2_p = 2;
		fdistance = 2.25; 
		
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		in_cell = init_connection->input_cell;
		connect_net = get_network_id(in_cell->net_id);		
		/* fprintf( stderr,"init # connect %d\n",number_connect); */ 	
		
		max_firing_rate = 0.0;
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* a cell of interest is not at edge */
			/* a loop to change map */
			for ( i = 0; i < number_connect; i++ ) {
				/* if not at edge */
				connection = init_connection + i;				
				in_cell = connection->input_cell;
				connect_net = get_network_id(in_cell->net_id);	
					
				/* x,y low and high limit due to edge */
				x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
				x_high_limit = x_limit_high( connect_net, in_cell, range1_p );
				y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
				y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
				/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n",
						in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit); */

				for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
					for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
						if (((bias_x * bias_x) + (bias_y * bias_y)) <= (fdistance * fdistance)) {
						   /* within distance */
						   neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
						   in_value = ((connection->conductance)*(neighbor_cell->firing_rate));
						   if ( in_value > max_firing_rate ) 
							max_firing_rate = in_value;
						}
					}
				}
			}
			/* output is always >= 0 , because max_firing_rate never go below 0 */
			cell->voltage = max_firing_rate;
			cell->firing_rate = max_firing_rate;
		} else {
			/* all 4 peripheral edges */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}
	

/* local maximum of all connected maps within region 1x1.					*/
/* assuming inputs are shifted 50, thus minus 50 at first.					*/
/* negative input is set to 0, but weight function(const) is valid for conversion. 		*/
/* this conversion is made after the shift.							*/
/* output = max( weight * (input - 50) )  							*/

	if ( strcmp(pgn_func, "local_max_shift_1x1") == 0 ) {		
		
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		/* fprintf( stderr,"init # connect %d\n",number_connect); */ 	
		
		max_firing_rate = 0.0;
		/* a loop to change map */
		for ( i = 0; i < number_connect; i++ ) {
			connection = init_connection + i;				
			in_cell = connection->input_cell;
					
			in_value = (connection->conductance)*(in_cell->firing_rate - 50.0);
			if ( in_value > max_firing_rate )
				max_firing_rate = in_value;
		}
		/* output is always >= 0 , because max_firing_rate never go below 0 */
		cell->voltage = max_firing_rate;
		cell->firing_rate = max_firing_rate;
		
		return(OK);
	}

/* local maximum of all connected maps within region 3x3.					*/
/* assuming inputs are shifted 50, thus minus 50 at first.					*/
/* negative input is set to 0, but weight function(const) is valid for conversion. 		*/
/* this conversion is made after the shift.							*/
/* output = max( weight * (input - 50) )  							*/
	if ( strcmp(pgn_func, "local_max_shift_3x3") == 0 ) {		
		/* determine region , 3x3 */
		range1_n = -1;
		range1_p = 1;
		range2_n = -1;
		range2_p = 1;
		fdistance = 1.5; 
		
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		in_cell = init_connection->input_cell;
		connect_net = get_network_id(in_cell->net_id);		
		/* fprintf( stderr,"init # connect %d\n",number_connect); */ 	
		
		max_firing_rate = 0.0;
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* a cell of interest is not at edge */
			/* a loop to change map */
			for ( i = 0; i < number_connect; i++ ) {
				/* if not at edge */
				connection = init_connection + i;				
				in_cell = connection->input_cell;
				connect_net = get_network_id(in_cell->net_id);	
					
				/* x,y low and high limit due to edge */
				x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
				x_high_limit = x_limit_high( connect_net, in_cell, range1_p );
				y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
				y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
				/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n",
						in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit); */

				for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
					for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
						if (((bias_x * bias_x) + (bias_y * bias_y)) <= (fdistance * fdistance)) {
						   /* within distance */
						   neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
						   in_value = (connection->conductance)*(neighbor_cell->firing_rate - 50.0);
						   if ( in_value > max_firing_rate )
							max_firing_rate = in_value;
						}
					}
				}
			}
			/* output is always >= 0 , because max_firing_rate never go below 0 */
			cell->voltage = max_firing_rate;
			cell->firing_rate = max_firing_rate;
		} else {
			/* all 4 peripheral edges */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}
	
/* local maximum of all connected maps within region 1x1 by absolute value, output is also abs. */
/* assuming input is shifted by 50 */

	if ( strcmp(pgn_func, "abs_local_max_1x1") == 0 ) {
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		/* fprintf( stderr,"init # connect %d\n",number_connect); */ 	
		
		max_firing_rate = 0.0;
		/* a loop to change map */
		for ( i = 0; i < number_connect; i++ ) {
			connection = init_connection + i;				
			in_cell = connection->input_cell;
			
			if ( (in_value = fabs((in_cell->firing_rate) - 50.0 )) > max_firing_rate )
				max_firing_rate = in_value;
		}
		/* output is always >= 0 , because max_firing_rate never go below 0 */
		cell->voltage = max_firing_rate;
		cell->firing_rate = max_firing_rate;
		
		return(OK);
	}
	
/* local maximum of all connected maps within region 3x3 by absolute value, output is also abs. */
/* assuming input is shifted by 50 */

	if ( strcmp(pgn_func, "abs_local_max_3x3") == 0 ) {
		
		/* determine region , 3x3 */
		range1_n = -1;
		range1_p = 1;
		range2_n = -1;
		range2_p = 1;
		fdistance = 1.5; 
		
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		in_cell = init_connection->input_cell;
		connect_net = get_network_id(in_cell->net_id);		
		/* fprintf( stderr,"init # connect %d\n",number_connect); */ 	
		
		max_firing_rate = 0.0;
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* a cell of interest is not at edge */
			/* a loop to change map */
			for ( i = 0; i < number_connect; i++ ) {
				/* if not at edge */
				connection = init_connection + i;				
				in_cell = connection->input_cell;
				connect_net = get_network_id(in_cell->net_id);	
					
				/* x,y low and high limit due to edge */
				x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
				x_high_limit = x_limit_high( connect_net, in_cell, range1_p );
				y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
				y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
				/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n",
						in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit); */

				for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
					for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
						if (((bias_x * bias_x) + (bias_y * bias_y)) <= (fdistance * fdistance)) {
							/* within distance */
							neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
							if ( fabs((neighbor_cell->firing_rate) - 50.0) > max_firing_rate ) 
								max_firing_rate = fabs((neighbor_cell->firing_rate) - 50.0);
						}
					}
				}
			}
			cell->voltage = max_firing_rate; 		/* you might need to set net->offset = 50 */
			cell->firing_rate = connection->conductance * max_firing_rate;
		} else {
			/* all 4 peripheral edges */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}

/* local maximum of all connected maps within region 1x1 by absolute value, output is SIGNED. */
/* assuming input is shifted by 50, output is also shifted 50. */

	if ( strcmp(pgn_func, "local_max_by_abs_1x1") == 0 ) {
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		/* fprintf( stderr,"init # connect %d\n",number_connect); */ 	
		
		max_firing_rate = 0.0;
		/* a loop to change map */
		for ( i = 0; i < number_connect; i++ ) {
			connection = init_connection + i;				
			in_cell = connection->input_cell;
			
			if ( fabs((in_cell->firing_rate) - 50.0) > fabs(max_firing_rate - 50.0) ) 
								max_firing_rate = in_cell->firing_rate;
		}
		/* output is SIGNED */
		cell->voltage = max_firing_rate;
		cell->firing_rate = max_firing_rate;
		
		return(OK);
	}

/* local maximum of all connected maps within region 3x3 by absolute value, but output is SIGNED. */
/* assuming input is shifted by 50 */
	if ( strcmp(pgn_func, "local_max_by_abs_3x3") == 0 ) {
		
		/* determine region , 3x3 */
		range1_n = -1;
		range1_p = 1;
		range2_n = -1;
		range2_p = 1;
		fdistance = 1.5; 
		
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		in_cell = init_connection->input_cell;
		connect_net = get_network_id(in_cell->net_id);		
		/* fprintf( stderr,"init # connect %d\n",number_connect); */ 	
		
		max_firing_rate = 0.0;
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* a cell of interest is not at edge */
			/* a loop to change map */
			for ( i = 0; i < number_connect; i++ ) {
				/* if not at edge */
				connection = init_connection + i;				
				in_cell = connection->input_cell;
				connect_net = get_network_id(in_cell->net_id);	
					
				/* x,y low and high limit due to edge */
				x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
				x_high_limit = x_limit_high( connect_net, in_cell, range1_p );
				y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
				y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
				/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n",
						in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit); */

				for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
					for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
						if (((bias_x * bias_x) + (bias_y * bias_y)) <= (fdistance * fdistance)) {
							/* within distance */
							neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
							if ( fabs((neighbor_cell->firing_rate) - 50.0) > fabs(max_firing_rate) ) 
								max_firing_rate = neighbor_cell->firing_rate - 50.0;
						}
					}
				}
			}
			cell->voltage = max_firing_rate;
			cell->firing_rate = connection->conductance * max_firing_rate;
		} else {
			/* all 4 peripheral edges */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}
	
/* gradient , GENERAL version */
/* out = ( sum_of_+_region - sum_of_-_region ) / ( #_units * distance_between_two_regions )		*/
/* for L & R, the distance(dx) is considered as a diagonal, thus 1.41421.				*/
/* Arguments:
	argv[1]: length(take gradient respect to this axis), see NOTE.
   	argv[2]: width(average along this axis),e.g. H: vertical, V:horizontal, L,R:vertical( = horizontal)	
   		 for L and R, thus, width(perpendicular to length) become actual(x1.1415) width. 
   	argv[3]: H,V,L or R 	
   	argv[4]: NZ or others. For NZ, if ANY incoming data <= threshold, out 0.0.
   NOTE: length(argv[1]) is the number of pixels, NOT actual length.  
	 Thus grad(7,3,H) is equivalent of grad(5,3,L) in terms of receptive field size.
*/
	if ( determine_pgn_function(pgn_func,"grad") ) {
		get_argument(pgn_func,1,arg1);		/* length  */
		get_argument(pgn_func,2,arg2);		/* width   */
		get_argument(pgn_func,3,arg3);		/* H,V,L,R */
		get_argument(pgn_func,4,arg4);		/* NZ	   */
		length = atoi( arg1 );
		width = atoi( arg2 );
		half_length = length / 2;
		half_width = width / 2;
		
		switch ( arg3[0] ) {
			case 'H':						/* positively sensitive to -0+ */
				x_low = -1*half_length; x_high = half_length; 
				y_low = -1*half_width; y_high = half_width; break;
			case 'V':								/* - */
				x_low = -1*half_width; x_high = half_width; 			/* 0 */
				y_low = -1*half_length; y_high = half_length; break;		/* + */
			case 'L':
				x_low = -1*half_length; x_high = half_length; 			/* -	  */
				y_low = -1*half_length; y_high = half_length; break;		/*   0    */
			case 'R':								/*     +  */
			 	x_low = -1*half_length; x_high = half_length; 		/*     -  */
				y_low = -1*half_length; y_high = half_length; break;	/*   0    */
			default:							/* +      */
				fprintf(stderr," pgn function <%s> ARGUMENT ERROR \n",pgn_func );
				return(ERROR);
		}
		 
		connection = cell->connect_list;
		in_cell_head = connection->input_cell;			/* connected cell    	*/
		connect_net = get_network_id( in_cell_head->net_id );	/* connected network 	*/
		
		network = get_network_id( cell->net_id );		/* self */
		
		flag_edge = OFF;
		if ( not_at_edge(cell->id, cell->net_id, network->name) == TRUE ) {	/* check self */
			/*  not at edge */
			for ( bias_x = x_low; bias_x <= x_high; bias_x++ ) {
			for ( bias_y = y_low; bias_y <= y_high; bias_y++ ) {
				/* get values from connected cells */
				neighbor_cell = get_neighboor( in_cell_head, connect_net, bias_x, bias_y );
				if ( not_at_edge(neighbor_cell->id, cell->net_id, network->name) == FALSE ) {
					/*  at edge */
					flag_edge = ON;
					break;
				} else {
					/* not at edge */
					value2[bias_x + x_high][bias_y + y_high] = neighbor_cell->firing_rate;	
					/* printf("index_x = %d, index__y = %d    value2 = %lf\n"
					   ,bias_x,bias_y,value2[bias_x + x_high][bias_y + y_high]); */
				}
			}}
			if ( flag_edge == OFF ) {
			  number_element = 0;
			  value_m = 0; value_p = 0;
			  flag_zero = OFF;
			  switch ( arg3[0] ) {
				case 'H':
					for ( bias_x = 0; bias_x < half_length; bias_x++ ) {
					for ( bias_y = 0; bias_y < width; bias_y++ ) {
						if (value2[bias_x][bias_y] <= cell->threshold)
							flag_zero = ON;
						value_m += value2[bias_x][bias_y];
						number_element++;
						/* printf(" minus: bias_x = %d, bias_y = %d, value2 = %lf\n"
						   ,bias_x,bias_y,value2[bias_x][bias_y]); */
					}}
					for ( bias_x = (half_length + 1); bias_x < length; bias_x++ ) {
					for ( bias_y = 0; bias_y < width; bias_y++ ) {
						if (value2[bias_x][bias_y] <= cell->threshold)
							flag_zero = ON;
						value_p += value2[bias_x][bias_y];
					}}
					/* distance between centers of positive and negative groups.  	*/  
					fdistance = (double)half_length + 1.0; 			/* 2x(half/2)+1 */
					/* printf("H  %6.2f,%6.2f : %6.2f,%6.2f  : %6.2f,%d,%d\n", 
					   value_m,value_m2,value_p,value_p2,fdistance,number_element,half_length); */	   
					break;
				case 'V':
					for ( bias_y = 0; bias_y < half_length; bias_y++ ) {
					for ( bias_x = 0; bias_x < width; bias_x++ ) {
						if (value2[bias_x][bias_y] <= cell->threshold)
							flag_zero = ON;
						value_p += value2[bias_x][bias_y];
						number_element++;
					}}
					for ( bias_y = (half_length + 1); bias_y < length; bias_y++ ) {
					for ( bias_x = 0; bias_x < width; bias_x++ ) {
						if (value2[bias_x][bias_y] <= cell->threshold)
							flag_zero = ON;
						value_m += value2[bias_x][bias_y];
					}}
						
					fdistance = (double)half_length + 1.0; 			/* 2x(half/2)+1 */
					break;
				case 'L':
					/* left columns (minus) */
					for ( bias_x = 0; bias_x < half_length; bias_x++ ) {
					for ( bias_y = (half_length + 1); bias_y < length; bias_y++ ) {
						if ( (bias_x + bias_y) == (length - 1) ) {
						/* left up diagonal */
							for ( width_y = -1*half_width; width_y <= half_width; width_y++ ) {
								if ( (bias_y + width_y) >= 0 && (bias_y + width_y) < length ) {
									if (value2[bias_x][bias_y + width_y] <= cell->threshold)
										flag_zero = ON;
									value_m += value2[bias_x][bias_y + width_y];
									number_element++; 	/* count minus only */
						}	}	}
					}}
					/* right columns (plus) */
					for ( bias_x = (half_length + 1); bias_x < length; bias_x++ ) {
					for ( bias_y = 0; bias_y < half_length; bias_y++ ) {
						if ( (bias_x + bias_y) == (length - 1) ) {
						/* left up diagonal */
							for ( width_y = (-1*half_width); width_y <= half_width; width_y++ ) {
							     if ( ((bias_y + width_y) >= 0) && ((bias_y + width_y) < length )) {
									if (value2[bias_x][bias_y + width_y] <= cell->threshold)
										flag_zero = ON;
									value_p += value2[bias_x][bias_y + width_y];
						}	}    }
					}}
					/* center column */
					for ( width_y = -1*half_width; width_y <= half_width; width_y++ ) {
						if ( ((half_length + width_y) >= 0)&&((half_length + width_y) < half_length) ) {
							if (value2[half_length][half_length + width_y] <= cell->threshold)
								flag_zero = ON;
							value_p += value2[half_length][half_length + width_y];
						}
						if (((half_length + width_y) < length)&&((half_length + width_y) > half_length)) {
							if ( value2[half_length][half_length + width_y]<= cell->threshold)
								flag_zero = ON;
							number_element++;
							value_m += value2[half_length][half_length + width_y];
					}	}
					
					/* mean distance between plus and minus regions */
					if ( fmod( (double)width,2.0 ) != 0 ) {
						/* width odd */
						dist_shift = (double)(((width - 1) / 4) + 1) / (double)width;
					} else {
						/* width even */
						dist_shift = 0.0; 
					}
					fdistance = ( 2.0*SQRT2*((double)half_length/2.0) ) + SQRT2*dist_shift;
					break;			/* dist_shift: 1/3 for width=3, 2/5 for width=5 */
				case 'R':
					/* left columns (plus) */
					for ( bias_x = 0; bias_x < half_length; bias_x++ ) {
					for ( bias_y = 0; bias_y < half_length; bias_y++ ) {
						if ( bias_x == bias_y ) {
						/* right up diagonal */
							for ( width_y = -1*half_width; width_y <= half_width; width_y++ ) {
								if ( (bias_y + width_y) >= 0 && (bias_y + width_y) < length ) {
								   	if (value2[bias_x][bias_y + width_y]<= cell->threshold)
								   		flag_zero = ON;
									value_p += value2[bias_x][bias_y + width_y];
									number_element++; 	/* count plus only */
						}	}	}
					}}
					/* right columns (minus) */
					for ( bias_x = (half_length + 1); bias_x < length; bias_x++ ) {
					for ( bias_y = (half_length + 1); bias_y < length; bias_y++ ) {
						if ( bias_x == bias_y ) {
						/* right up diagonal */
							for ( width_y = (-1*half_width); width_y <= half_width; width_y++ ) {
							     if ( ((bias_y + width_y) >= 0) && ((bias_y + width_y) < length )) {
							     		if (value2[bias_x][bias_y + width_y]<= cell->threshold)
							     			flag_zero = ON;
									value_m += value2[bias_x][bias_y + width_y];
						}	}    }
					}}
					/* center column */
					for ( width_y = -1*half_width; width_y <= half_width; width_y++ ) {
						if ( ((half_length + width_y) >= 0)&&((half_length + width_y) < half_length) ) {
							if (value2[half_length][half_length + width_y]<= cell->threshold)
								flag_zero = ON;
							value_p += value2[half_length][half_length + width_y];
							number_element++;
						}
						if(((half_length + width_y) < length)&&((half_length + width_y) > half_length)) {
							if (value2[half_length][half_length + width_y]<= cell->threshold)
								flag_zero = ON;
							value_m += value2[half_length][half_length + width_y];
					}	}
					
					/* mean distance between plus and minus regions */
					if ( fmod( (double)width,2.0 ) != 0 ) {
						/* width odd */
						dist_shift = (double)(((width - 1) / 4) + 1) / (double)width;
					} else {
						/* width even */
						dist_shift = 0.0; 
					}
					fdistance = ( 2.0*SQRT2*((double)half_length/2.0) ) + SQRT2*dist_shift;  
					break;
				default:
					fprintf(stderr," pgn function <%s> ARGUMENT ERROR \n",pgn_func );
					return(ERROR);
			}
			/* printf("%c %6.2f,%6.2f : %6.2f,%6.2f  : %6.2f,%d,%d\n", 
			   arg3[0],value_m,value_m2,value_p,value_p2,fdistance,number_element,half_length);
			*/		  
			cell->voltage = (value_p - value_m) / (number_element * fdistance);
			cell->firing_rate = connection->conductance * cell->voltage;
		   } 	/* end of switch */
		} else {
			/* at edge */
			flag_edge = ON;
		}
		if ( flag_edge == ON  ) {				/* if edge */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		} 	
		if ( (strcmp(arg4,"NZ") == 0)&&(flag_zero == ON) ) {  	
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		} 
		return(OK);
	
	}
	
/* OLDER FUNCTIONS saved in shape_function_8_92.c */
/* gradient of 5x3 area */
/* for L & R, dx is considered as diagonal, thus 1.41421.	*/
	
/* gradient of 7x3 area , NEW version */
/* for L & R, dx is considered as diagonal, thus 1.41421.	*/
	
/* gradient of 11x3 area , NEW version */
/* for L & R, dx is considered as diagonal, thus 1.41421.	*/
	
/* gradient of 3x7 area, along H */
	
/* gradient of 3x7 area, along V */
	
/* gradient of 3x7 area, along L */	
/* gradient of 3x7 area, along R */
	
/* sqrt  for depth */
	if ( determine_pgn_function(pgn_func, "sqrt") ) {
		connection = cell->connect_list;		/* the second connection */
		in_cell = connection->input_cell;		/* connected cell    	 */
		connect_net = get_network_id(in_cell->net_id);	/* connected network 	 */
		
		connection2 = connection + 1;			/* the first connection   */
		in_cell2 = connection2->input_cell;
		connect_net2 = get_network_id(in_cell2->net_id);	

		network = get_network_id(cell->net_id);		/* self */

		cell->voltage = (connection->conductance * in_cell->firing_rate) * 
				(connection2->conductance * in_cell2->firing_rate);
		if ( cell->voltage > 0 ) 
			cell->firing_rate =  sqrt( cell->voltage );
		else
			cell->firing_rate = 0.0;
		return(OK);
	}

/* depth from square of opponents */
/* if ( (fabs(net1) >= fabs(net2)) && (net1*net2>0) ) then sqrt(net1*net2)	*/
/* if net1*net2 <0 , then output is set to 0					*/
/* weight is valid								*/

	if ( determine_pgn_function(pgn_func, "depth_sq_op") ) {
		connection = cell->connect_list;		/* the second connection */
		in_cell = connection->input_cell;		/* connected cell    	 */
		connect_net = get_network_id(in_cell->net_id);	/* connected network 	 */
		
		connection2 = connection + 1;			/* the first connection   */
		in_cell2 = connection2->input_cell;
		connect_net2 = get_network_id(in_cell2->net_id);	

		if ( fabs(in_cell->firing_rate) <= fabs(in_cell2->firing_rate) ) {
		/* net1 >= net2 */
			cell->voltage = (connection->conductance * in_cell->firing_rate) * 
					(connection2->conductance * in_cell2->firing_rate);
			if ( cell->voltage > 0 ) 
				cell->firing_rate =  sqrt( cell->voltage );
			else
				cell->firing_rate = 0.0;
		} else {
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}

/* depth from square of opponents */
/* if ( (fabs(net1) >= fabs(net2)) && (net1*net2>0) ) then sqrt(net1*net2)	*/
/* if net1*net2 <0 , then output is set to 0					*/
/* weight is valid								*/

	if ( strcmp(pgn_func, "depth_sq_op2") == 0 ) {
		connection = cell->connect_list;		/* the second connection */
		in_cell = connection->input_cell;		/* connected cell    	 */
		connect_net = get_network_id(in_cell->net_id);	/* connected network 	 */
		
		connection2 = connection + 1;			/* the first connection   */
		in_cell2 = connection2->input_cell;
		connect_net2 = get_network_id(in_cell2->net_id);	

		/* NO net1 >= net2 CONSTRAIN */
		cell->voltage = (connection->conductance * in_cell->firing_rate) * 
				(connection2->conductance * in_cell2->firing_rate);
		if ( cell->voltage > 0 ) 
			cell->firing_rate =  sqrt( cell->voltage );
		else
			cell->firing_rate = 0.0;
		return(OK);
	}
	
/* depth from difference of opponents; net1 - net2		*/
/* if net1*net2 = 0(either one <= 0) , then output is set to 0	*/
/* weight is valid						*/

	if ( strcmp(pgn_func, "depth_df_op") == 0 ) {
		connection = cell->connect_list;		/* the second connection */
		in_cell = connection->input_cell;		/* connected cell    	 */
		connect_net = get_network_id(in_cell->net_id);	/* connected network 	 */
		
		connection2 = connection + 1;			/* the first connection   */
		in_cell2 = connection2->input_cell;
		connect_net2 = get_network_id(in_cell2->net_id);	

		if ( (in_cell2->firing_rate)*(in_cell->firing_rate) > 0 ) {
			/* net1 - net2 */
			cell->voltage = (connection2->conductance * in_cell2->firing_rate) - 
					(connection->conductance * in_cell->firing_rate);	
			if ( cell->voltage > 0 ) {
				/* if net1 > net2 */ 
				cell->firing_rate = cell->voltage;
			} else {
				cell->firing_rate = 0.0;
			}
		} else {
			/* either one <= 0 */
			cell->firing_rate = 0.0;
		}
		return(OK);
	}
/* division: net1 / net2	*/
/* weight is valid		*/
	if ( strcmp(pgn_func, "division") == 0 ) {
		connection = cell->connect_list;		/* the second connection */
		in_cell = connection->input_cell;		/* connected cell    	 */
		connect_net = get_network_id(in_cell->net_id);	/* connected network 	 */
		
		connection2 = connection + 1;			/* the first connection   */
		in_cell2 = connection2->input_cell;
		connect_net2 = get_network_id(in_cell2->net_id);	

		if ( in_cell->firing_rate != 0 ) {
			/* net1 / net2 */
			cell->voltage = (connection2->conductance * in_cell2->firing_rate) / 
					(connection->conductance * in_cell->firing_rate);	
			cell->firing_rate = cell->voltage;
		} else {
			cell->firing_rate = 1000.0;
		}
		return(OK);
	}

/* lateral inhibition by absolute value 					*/
/* 1st connection: abs local max,  second: each map, assuming shift of 50 	*/
/* weight functions ( const. ) are valid. for inhibition, 1st connection(local max) must be negative  	*/
/* assuming net->offset = 50.0 for display					*/
/* if weight for 1st connection(local max) is 0.5, then net->scale should be 2.0 */

	if ( strcmp(pgn_func, "inh_by_abs") == 0 ) {
		connection = cell->connect_list;		/* the second connection, value to be subtracted */
		in_cell = connection->input_cell;		/* connected cell       , assuming shift of 50 	 */
		connect_net = get_network_id(in_cell->net_id);	
		
		connection2 = connection + 1;			/* the first connection, value to subtract   */
		in_cell2 = connection2->input_cell;		/*                     , assuming no shift   */
		connect_net2 = get_network_id(in_cell2->net_id);	

		network = get_network_id(cell->net_id);		/* self */

		cell->voltage = (in_cell->firing_rate) - 50.0;
		if ( cell->voltage >= 0 ) {
			cell->firing_rate = (connection->conductance * (in_cell->firing_rate - 50.0)) 
					+ (connection2->conductance * in_cell2->firing_rate) ;
			if ( cell->firing_rate < 0 )
				cell->firing_rate = 0;
		} else {
			cell->firing_rate = (connection->conductance * (in_cell->firing_rate - 50.0)) 
					- (connection2->conductance * in_cell2->firing_rate) ;
			if ( cell->firing_rate > 0 )
				cell->firing_rate = 0;
		}
		return(OK);
	}

/* FMM 	:  Frequency Modulated Magnitude	*/
/* take conductances for weight			*/
	if ( strcmp(pgn_func, "FMM") == 0 ) {
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		/* fprintf( stderr,"init # connect %d\n",number_connect); */ 	
		
		
		in_value = 0.0;
		/* a loop to change map */
		for ( i = 0; i < number_connect; i++ ) {
			connection = init_connection + i;				
			in_cell = connection->input_cell;
					
			in_value += (connection->conductance)*(in_cell->firing_rate);
		}
		cell->voltage = in_value;
		cell->firing_rate = in_value / (double)number_connect ;
		
		return(OK);
	}

/* convolution by file mask	*/
/* Arguments:
	argv[1]: x dim. of the mask(length)
   	argv[2]: y dim. of the mask(width)
   	argv[3]: file name
	argv[4]: min. of output
	argv[5]: max. of output
	argv[6]: transfer function (optional)
	argv[7]: slope for sigmoid, OR switch for linear (optional)
        
*/
/* (1) weight is valid before evaluation. 
   (2) If linear trasfer function, subtract threshold.

   if ( convolution x weight - threshold ) > max, out = max
   if ( convolution x weight - threshold ) < min, out = min
   otherwise,    		      out = convolution x weight - threshold

   New 06/18/2002: Mimic built-in "Linear" transfer function.   
	( (convolution X weight) - threshold ) before operating on max & min.

   New 06/20/2002: If linear & arg[7]=1, then partial convolution is computed. If a mask covers the region
        outside the connected net, the activities of the outside is set to zero. This operation is compatible
        with ordinary connections without pgn.
                   
*/
/* a mask file is read only if the file is different from the previously used file.  
*/
	if ( determine_pgn_function(pgn_func,"conv") ) {
		char funcname[MAX_ARG_LEN];
		double sig_slope, sig_val;
		get_argument(pgn_func,1,arg1);
		get_argument(pgn_func,2,arg2);
		get_argument(pgn_func,3,arg3);
		get_argument(pgn_func,4,arg4);
		get_argument(pgn_func,5,arg5);
		get_argument(pgn_func,6,arg6);
		get_argument(pgn_func,7,arg7);
		
		min = char_to_float( arg4 );
		max = char_to_float( arg5 );
		length = atoi( arg1 );
		width  = atoi( arg2 );
		number_element = length * width;
		
		/* Determine output function after the convolution */
		sprintf(funcname,"%s",arg6);
		if ( strlen( funcname ) <= 3 ) {		/* DOES NOT WORK */
			/* no arg6, default is linear */
			sprintf (funcname, "linear");
		}
		
		/* Load mask coeficients */
		sprintf(filename,"%s",arg3);
		if ( strcmp( filename, prev_file ) != 0 ) {
			/* new file name( different from the right before) */
			strcpy( prev_file, filename );
			if ( (fp = fopen(filename,"r")) == NULL ) {
				fprintf(stderr," file <%s> open error. (use previous file)\n",filename );
				fclose(fp);
				return(ERROR);
			}
			for ( i = 0; i < number_element; i++ ) {
				if ( (check = fscanf(fp,"%lf",&mask_coef[i])) != 1 ) {
					fprintf(stderr," file <%s> read error. (use previous file)\n",filename );
					fclose(fp);
					return(ERROR);
				}	
				/* printf(" %lf",mask_coef[i]); */
			}
			fprintf(stderr," mask file <%s> loaded.\n",filename);
			fclose(fp);
		}
				
		connection = cell->connect_list;
		in_cell_head = connection->input_cell;			/* connected cell    	*/
		network = get_network_id( cell->net_id );		/* self */

		/* choose whether (1) do nothing, or (2) compute partial convolution, if the mask covers outside. */
		/* if (  char_to_float( arg7 ) != 1.0) { */
		if (  atof( arg7 ) != 1.0 ) {
		  /* do nothing if covers outside */
		  cell->voltage = convolution(cell, in_cell_head, length, width, mask_coef, 0.0);
		} else {
		  /* compute partial covolution */
		  cell->voltage = convolution_z(cell, in_cell_head, length, width, mask_coef, 0.0);
		}
		cell->firing_rate = connection->conductance * cell->voltage; 
		
		/* transfer function */
		if ( strcmp( funcname, "sigmoid" ) != 0 ) {
		  /* linear */
			
                        /* threshold changed 06/18/2002 */
                        cell->firing_rate = (cell->firing_rate) - (cell->threshold);
                        
                        /* max, min clamp */
			if ( cell->firing_rate > max )
				cell->firing_rate = max;
			if ( cell->firing_rate < min )
				cell->firing_rate = min;
			
		}
		if ( strcmp( funcname, "sigmoid" ) == 0 ) {
		  /* sigmoid */
			sig_slope = char_to_float( arg7 );
			sig_val = 1.0/( 1.0 + exp( ((cell->threshold)-(cell->voltage))*sig_slope ) );
			cell->firing_rate = ( (max - min)*sig_val + min )*(connection->conductance);
		}
		return(OK);
	}

/* sobel(3x3) filtering */
	if ( determine_pgn_function(pgn_func,"sobel") ) {
		connection = cell->connect_list;
		in_cell_head = connection->input_cell;			/* connected cell    	*/
		connect_net = get_network_id( in_cell_head->net_id );	/* connected network 	*/

		network = get_network_id( cell->net_id );		/* self */
		
		if ( not_at_edge(cell->id, cell->net_id, network->name) == TRUE ) {	/* check self */
			/* not at edge */
			for ( position = 0; position <= 7; position++ ) {
				/* get values from connected cells */
				neighbor_cell = get_nearest_neighbor( in_cell_head, connect_net, position );
				value[position] = neighbor_cell->firing_rate;	
			}
			value_x = value[7] + 2.0*value[4] + value[2] - value[5] - 2.0*value[3] - value[0];
			value_y = value[5] + 2.0*value[6] + value[7] - value[0] - 2.0*value[1] - value[2];
			cell->voltage = (double)sqrt( pow((double)value_x,(double)2.0) + pow((double)value_y,(double)2.0) );
			cell->firing_rate = connection->conductance * cell->voltage;
		} else {
			/* at edge */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}

/* Sobel(5x5) filtering */
	if ( strcmp(pgn_func, "sobel5") == 0 ) {
		connection = cell->connect_list;
		in_cell_head = connection->input_cell;			/* connected cell    	*/
		connect_net = get_network_id( in_cell_head->net_id );	/* connected network 	*/

		network = get_network_id( cell->net_id );		/* self */
		
		flag_edge = OFF;
		if ( not_at_edge(cell->id, cell->net_id, network->name) == TRUE ) {	/* check self */
			/*  not at edge */
			for ( position = 0; position <= 23; position++ ) {
				/* get values from connected cells */
				neighbor_cell = get_nearest_neighbor( in_cell_head, connect_net, position );
				if ( not_at_edge(neighbor_cell->id, cell->net_id, network->name) == FALSE ) {
					/*  at edge */
					flag_edge = ON;
					break;
				} else {
					/* not at edge */
					value[position] = neighbor_cell->firing_rate;	
				}
			}
			if ( flag_edge == OFF ) {
				value_x = 2.0*value[12] + 4.0*value[14] + 4.0*value[16] + 4.0*value[18] + 2.0*value[23]
				         - 2.0*value[8] - 4.0*value[13] - 4.0*value[15] - 4.0*value[17] - 2.0*value[19]
				         + value[2] + 2.0*value[4] + value[7]
				         - value[0] - 2.0*value[3] - value[5] ;
				value_y = 2.0*value[19] + 4.0*value[20] + 4.0*value[21] + 4.0*value[22] + 2.0*value[23] 
				         - 2.0*value[8] - 4.0*value[9] - 4.0*value[10] - 4.0*value[11] - 2.0*value[12]
				         + value[5] + 2.0*value[6] + value[7]
				         - value[0] - 2.0*value[1] - value[2];
				cell->voltage = (double)sqrt( pow((double)value_x,(double)2.0) + pow((double)value_y, (double)2.0) );
				cell->firing_rate = connection->conductance * cell->voltage;
			}
		} else {
			/* at edge */
			flag_edge = ON;
		}
		if ( flag_edge == ON ) {
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		} 	
		return(OK);
	}


/* wipe 			*/
/* regional conditioning 	*/
/* trick for fg/bg 		*/
	if ( determine_pgn_function(pgn_func,"wipe") ) {
		int 	dw_l, up_r;
		int	trunc_edge;
		int	cx, cy;
		double	edge_value;
		double  fradius;
		
		get_argument(pgn_func,1,arg1);
		get_argument(pgn_func,2,arg2);
		get_argument(pgn_func,3,arg3);
		get_argument(pgn_func,4,arg4);
		get_argument(pgn_func,5,arg5);
		connection = cell->connect_list;				
		in_cell = connection->input_cell;
		connect_net = get_network_id(in_cell->net_id);
		
		cell_pos_x = cell_position_x( in_cell->id, connect_net->dim_x, connect_net->dim_y );
		cell_pos_y = cell_position_y( in_cell->id, connect_net->dim_x, connect_net->dim_y );
		
		
		switch ( arg1[0] ) {
		case 's':
			/* for sph_tex180_2 , center at (90,90), r=65(62+3) */
			if ( sqrt(pow((90.0 - (double)cell_pos_x),2.0) + pow((90.0 - (double)cell_pos_y),2.0)) <= 65.0 ) {
				cell->voltage = 100.0;	
				cell->firing_rate = 100.0;
			} else {
				cell->voltage = 0.0;	
				cell->firing_rate = 0.0;
			}
			break;
		case 'g':
			/* for sph_tex180_2 , center at (86,98), r=74 */
			cx = char_to_float( arg2 );
			cy = char_to_float( arg3 );
			fradius = char_to_float( arg4 );
			
			if ( sqrt(pow((cx - (double)cell_pos_x),2.0) + pow((cy - (double)cell_pos_y),2.0)) <= fradius ) {
				cell->voltage = 100.0;	
				cell->firing_rate = 100.0;
			} else {
				cell->voltage = 0.0;	
				cell->firing_rate = 0.0;
			}
			break;
		case 'm':
			/* for melon4 */
			if ( (cell_pos_x >= 53) || ((cell_pos_x >= 38)&&( cell_pos_y >= 28)) ) {
				cell->voltage = 100.0;		
				cell->firing_rate = 100.0;
			} else {
				cell->voltage = in_cell->firing_rate;
				cell->firing_rate = in_cell->firing_rate;
			}
			break;
		case 'a':
			/* for avocado10 */
			if ( cell_pos_x <= 37 ) {
				cell->voltage = 0.0;		
				cell->firing_rate = 0.0;
			} else {
				cell->voltage = in_cell->firing_rate;
				cell->firing_rate = in_cell->firing_rate;
			}
			break;
		case 'b':
			/* for b3plane 90x90 */
			if ( (cell_pos_x <= 7)||(cell_pos_y <= 7)||(cell_pos_x > 83)||(cell_pos_y > 83) ) {
				cell->voltage = 100.0;		
				cell->firing_rate = 100.0;
			} else {
				cell->voltage = in_cell->firing_rate;
				cell->firing_rate = in_cell->firing_rate;
			}
			break;
		case 'f':
			/* for fft circle , center at (arg2,arg2), r=arg3 */
			if ( sqrt(pow((char_to_float(arg2)-(double)cell_pos_x),2.0) + pow((char_to_float(arg2)-(double)cell_pos_y),2.0)) <= atoi(arg3) ) {
				cell->voltage = 100.0;	
				cell->firing_rate = 100.0;
			} else {
				cell->voltage = 0.0;	
				cell->firing_rate = 0.0;
			}
			break;
		case 'r':
			/* for fft circle , center at (arg2,arg2), r=arg3 */
			if ( sqrt(pow((char_to_float(arg2)-(double)cell_pos_x),2.0) + pow((char_to_float(arg2)-(double)cell_pos_y),2.0)) <= atoi(arg3) ) {
				cell->voltage = in_cell->firing_rate;	
				cell->firing_rate = in_cell->firing_rate;
			} else {
				cell->voltage = 0.0;	
				cell->firing_rate = 0.0;
			}
			break;
		case 'q':
			/* for fft rect , center at (arg2,arg2), dist_x= arg3, dist_y=arg4 */
			if ( ( pow((char_to_float(arg2) - (double)cell_pos_x),2.0) <= pow(char_to_float(arg3),2.0) ) &&
			     ( pow((char_to_float(arg2) - (double)cell_pos_y),2.0) <= pow(char_to_float(arg4),2.0) ) ) {
				cell->voltage = in_cell->firing_rate;	
				cell->firing_rate = in_cell->firing_rate;
			} else {
			  cell->voltage = char_to_float(arg5);	
				cell->firing_rate = char_to_float(arg5);
			}
			break;
		case 'w':
			/* for wraping_lrg, up-right and down-left triangle */
			dw_l = atoi( arg2 );	
			up_r = atoi( arg3 );
			edge_value = char_to_float( arg4 );
			
			if ( ((cell_pos_x + cell_pos_y)>=dw_l) && ((cell_pos_x + cell_pos_y)<=up_r) ) {
				cell->voltage = in_cell->firing_rate;	
				cell->firing_rate = in_cell->firing_rate;
			} else {
				cell->voltage = 0.0;	
				cell->firing_rate = edge_value;
			}
			break;
		case 'e':
			/* trancate edges */
			trunc_edge = atoi(arg2);
			edge_value = char_to_float(arg3);
			
			if ( (cell_pos_x > trunc_edge) && (cell_pos_x < (network->dim_x - trunc_edge)) &&
			     (cell_pos_y > trunc_edge) && (cell_pos_y < (network->dim_y - trunc_edge)) ) {
				cell->voltage = in_cell->firing_rate;	
				cell->firing_rate = in_cell->firing_rate;
			} else {
				cell->voltage = 0.0;	
				cell->firing_rate = edge_value;
			}
			break;
		case 'u':
			/* trancate upper edge , for perspective */
			trunc_edge = atoi(arg2);
			edge_value = char_to_float(arg3);
			
			if ( cell_pos_y < (network->dim_y - trunc_edge) ) {
				cell->voltage = in_cell->firing_rate;	
				cell->firing_rate = in_cell->firing_rate;
			} else {
				cell->voltage = 0.0;	
				cell->firing_rate = edge_value;
			}
			break;
		case 't':
			/* for b&w torus , center at (arg2,arg2), out_r=arg3, in_r=arg4 */
		  if ( (sqrt(pow((char_to_float(arg2)-(double)cell_pos_x),2.0) + pow((char_to_float(arg2)-(double)cell_pos_y),2.0)) <= atoi(arg3) )&&(sqrt(pow((char_to_float(arg2)-(double)cell_pos_x),2.0) + pow((char_to_float(arg2)-(double)cell_pos_y),2.0)) >= atoi(arg4) ) ) {
				cell->voltage = 100.0;	
				cell->firing_rate = 100.0;
			} else {
				cell->voltage = 0.0;	
				cell->firing_rate = 0.0;
			}
			break;
		}
			
		return(OK);
	}

/* switch 									*/
/* choose one net from two, cancel shift, and display a limited band ( + or - ) */
	if ( determine_pgn_function(pgn_func,"switch") ) {
		int switch_on = 1;

		get_argument(pgn_func,1,arg1);				/* connection switch 	*/
		get_argument(pgn_func,2,arg2);				/* shift (50)		*/
		get_argument(pgn_func,3,arg3);				/* min out (0 or -3000)	*/
		get_argument(pgn_func,4,arg4);				/* max out (3000 or  0)	*/
		switch_on = atoi( arg1 );			
		shift = char_to_float( arg2 );
		min = char_to_float( arg3 );
		max = char_to_float( arg4 );
		
		connection2 = cell->connect_list;			/* the second connection, Ori. Energy 	*/
		in_cell2 = connection2->input_cell;			/* connected cell    	 		*/
		connect_net2 = get_network_id(in_cell2->net_id);	/* connected network 	 		*/

		connection = connection2 + 1;				/* the first connection, 	 	*/
		in_cell = connection->input_cell;
		connect_net = get_network_id(in_cell->net_id);
		
		if ( switch_on == 1 ) {
			cell->voltage = in_cell->firing_rate - shift;
			cell->firing_rate = (connection->conductance) * (cell->voltage);
		} else {
			cell->voltage = in_cell2->firing_rate - shift;
			cell->firing_rate = (connection2->conductance) * (cell->voltage);
		}	
		
		if ( (cell->firing_rate) < min )
			cell->firing_rate = min;
		if ( (cell->firing_rate) > max )
			cell->firing_rate = max;
		
		return(OK);
	}
			
/* check correlation.  											*/
/* If in_1*in_2 >= threshold, then out abs_max(in_1,in_2) Otherwise, out 0.0.				*/
/* arg[1]: shift , subtract from incoming data at first.						*/
/* arg[2,][3]: min and max output. For on, off channels. Weight is valid before min., max.		*/
/* Off nets may be scaled by -1										*/

	if ( determine_pgn_function(pgn_func,"check_corr_max") ) {
		get_argument(pgn_func,1,arg1);				/* shift 		*/
		shift = char_to_float( arg1 );
		
		get_argument(pgn_func,2,arg2);				/* min out 		*/
		get_argument(pgn_func,3,arg3);				/* max out		*/
		min = char_to_float( arg2 );
		max = char_to_float( arg3 );
		
		connection2 = cell->connect_list;			/* the second connection, Ori. Energy 	*/
		in_cell2 = connection2->input_cell;			/* connected cell    	 		*/
		connect_net2 = get_network_id(in_cell2->net_id);	/* connected network 	 		*/

		connection = connection2 + 1;				/* the first connection, 	 	*/
		in_cell = connection->input_cell;
		connect_net = get_network_id(in_cell->net_id);
		
		if ( (corr=(in_cell->firing_rate - shift)*(in_cell2->firing_rate - shift)) >= cell->threshold ) {
			/* both have same sign, if threshold = 0 */
			if ( fabs(in_cell->firing_rate - shift) > fabs(in_cell2->firing_rate - shift) )
				cell->voltage = in_cell->firing_rate - shift;
			else
				cell->voltage = in_cell2->firing_rate - shift;
				
			cell->firing_rate = (connection->conductance) * (cell->voltage);
			if ( (cell->firing_rate) < min )
				cell->firing_rate = min;
			if ( (cell->firing_rate) > max )
				cell->firing_rate = max;
		} else {
			/* different sign */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}

/* check correlation in 3x3 region.  Check all cells in 3x3 have same sign. 			*/
/* If in_1*in_2 >= threshold, then out abs_max(in_1,in_2) of 1x1 region . Otherwise, out 0.0.	*/
/* arg[1]: shift , subtract from incoming data at first.					*/

	if ( determine_pgn_function(pgn_func,"check_corr_3x3") ) {
		get_argument(pgn_func,1,arg1);				/* shift 		*/
		shift = char_to_float( arg1 );
		
		connection2 = cell->connect_list;			/* the second connection, Ori. Energy 	*/
		in_cell2 = connection2->input_cell;			/* connected cell    	 		*/
		connect_net2 = get_network_id(in_cell2->net_id);	/* connected network 	 		*/

		connection = connection2 + 1;				/* the first connection, 	 	*/
		in_cell = connection->input_cell;
		connect_net = get_network_id(in_cell->net_id);
		
		sum_1 = 0.0;  sum_2 = 0.0;
		if (not_at_edge(cell->id, cell->net_id, network->name) == TRUE) {	/* check self */
			for ( i = 0; i <= 7; i++ ) {
				sum_1 = sum_1 + (get_nearest_neighbor(in_cell,connect_net,i))->firing_rate - shift;
				sum_2 = sum_2 + (get_nearest_neighbor(in_cell2,connect_net2,i))->firing_rate - shift;
			}
			
			if ( (sum_1*sum_2) >= cell->threshold ) {
				/* both have same sign, if threshold = 0 */
				if ( fabs(in_cell->firing_rate - shift) > fabs(in_cell2->firing_rate - shift) )
					cell->voltage = fabs(in_cell->firing_rate - shift);
				else
					cell->voltage = fabs(in_cell2->firing_rate - shift);
				
				cell->firing_rate = (connection->conductance) * (cell->voltage);
			} else {
				/* different sign */
				cell->voltage = 0.0;
				cell->firing_rate = 0.0;
			}
		} else {
			/* on edge */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}

/* check correlation.  	General version.								*/
/* Sum values in a circle w/ dia=arg[5], compare the sum of all connected nets(upto 10,same dimensions) */
/* If min(all)/max(all) >= threshold, then out. Otherwise, out 0.0.					*/
/* The threshold may be 0.0 for checking same sign, may be -0.5 for weak correlation check. 		*/
/* Output depends on arg[4]: all, max, ratio of max/min, or sd.					*/
/* arg[1]: 	shift , subtract from incoming data at first.					*/
/* arg[2][3]: 	min and max output. For on, off channels. Weight is valid before min., max.	*/
/* arg[4]:     	out a(ll), m(ax), r(atio of abs_man/abs_max), s(d), or d(eviation of max).	*/
/* arg[5]:	size(width = height) 								*/
/* Off nets may be scaled by -1				*/
/* May be  network scale = 1/(area of region) 		*/

	if ( determine_pgn_function(pgn_func,"check_corr") ) {
		double	sum_region[10];
		double	sum_all, ave_all;
		double  dev_max, dev_min;
		double	deviation, sum_dev, max_dev, sd;
		
		get_argument(pgn_func,1,arg1);				/* shift 		*/
		get_argument(pgn_func,2,arg2);				/* min out 		*/
		get_argument(pgn_func,3,arg3);				/* max out		*/
		get_argument(pgn_func,4,arg4);				/* choose out		*/
		get_argument(pgn_func,5,arg5);				/* length(=width) of region */
		shift = char_to_float( arg1 );
		min = char_to_float( arg2 );
		max = char_to_float( arg3 );
		size = atoi( arg5 );
		
		if ( (size <= 0)||(size >33) )				/* for no argument */
			size = 1;
			
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;			/* the last connection, Ori. Energy 	*/
		in_cell = init_connection->input_cell;			/* connected cell    	 		*/
		connect_net = get_network_id(in_cell->net_id);		/* connected network 	 		*/

		
		range1_n = (int)(-size / 2.0);
		range1_p = (int)(size / 2.0);
		range2_n = (int)(-size / 2.0);
		range2_p = (int)(size / 2.0);
		distance = (int)(size / 2.0);
		/* printf("range: %d %d %d %d\n",range1_n,range1_p,range2_n,range2_p);  all get same */
		
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* if not at edge */
			/* x,y low and high limit due to edge */
			x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
			x_high_limit = x_limit_high( connect_net, in_cell, range1_p );
			y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
			y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
			/* fprintf(stderr,"id: %d, xl %d, xh %d, yl %d, yd %d\n",
						in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit);  */

			/* a loop to change maps */
			for (i = 0; i < number_connect; i++) {
				connection = init_connection + i;
				in_cell = connection->input_cell;
				sum_region[i] = 0.0;
				
				for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
				for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
					if (((bias_x * bias_x) + (bias_y * bias_y)) <= (distance * distance)) { 
						/* within distance */
						neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
						sum_region[i] += neighbor_cell->firing_rate - shift;
						/* printf("conn: %d bias_x: %d _y: %d distance: %d input_sum: %lf \n"
									,i, bias_x, bias_y, distance ,sum_region[i]); */
					} 
				}}
			}	
		} else {
			/* peripheral edges */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
			return(OK);
		}
		
		/* compute ave, sd, abs_min, & abs_max */
		dev_max = -10000.0;
		dev_min = 10000.0;
		sum_all = 0.0;
		for (i = 0; i < number_connect; i++) {	/* i : map */
			sum_all += sum_region[i];			/* sum of all maps */
			if ( sum_region[i] > dev_max ) 			/* sum of region is computed. 			*/
				dev_max = sum_region[i];	
			if ( sum_region[i] < dev_min ) 
				dev_min = sum_region[i];
		}
		ave_all = sum_all / number_connect;
		max_dev = 0.0;
		sum_dev = 0.0;
		for (i = 0; i < number_connect; i++) {
			deviation = fabs( (ave_all - sum_region[i])/ave_all );		/* normalized by ave */
			sum_dev += deviation;
			if ( deviation >= max_dev )
				max_dev = deviation;
		}
		sd = sum_dev / number_connect;
		/* printf("conn= %d sum= %lf  ave= %lf sum_dev= %lf max_dev= %lf\n"
					,number_connect,sum_all,ave_all,sum_dev,max_dev); */ 

		/* output */
		if ( (ratio = dev_min / dev_max) >= (cell->threshold) ) {
			/* both have same sign, if threshold = 0 */
			
			switch( arg4[0] ) {
			case 'a':						/* sum of all region */
				cell->firing_rate = (connection->conductance) * sum_all;	
				break;							
			case 'm':						/* (abs)max of all region */
				cell->firing_rate = (connection->conductance) * max;
				break;
			case 'r':						/* ratio of (abs)min/(abs)max */
				cell->firing_rate = (connection->conductance) * ratio;	
				break;
			case 's':						/* sd of all */
				cell->firing_rate = (connection->conductance) * sd;
				break;
			case 'd':						/* max. deviation among connected maps */
				cell->firing_rate = (connection->conductance) * max_dev;
				break;
			default:
				cell->firing_rate = (connection->conductance) * sd;
				break;
			}
			
			cell->voltage = ratio;
			if ( (cell->firing_rate) < min )
				cell->firing_rate = min;
			if ( (cell->firing_rate) > max )
				cell->firing_rate = max;
		} else {
			/* < thrshd */
			cell->voltage = ratio;
			cell->firing_rate = 0.0;
			/* printf("< thrshd \n"); */
		}
		return(OK);
	}

/* UNDER DEVELOPMENT */	
/* conductance, short and long range.  short, long(w/ bipolar)					*/
/* 	SRC:	0 ~ 100	   narrower, stronger (need for gray scale)				*/
/* 		from 1st connection, intensity							*/
/*   	LRC:	0 ~ 30	   wider, weaker							*/	
/*  		from 2nd connection, freq.							*/
/*	Conductance = SRC + LRC,    Min SRC < Max LRC: in order to inhibit over-blob shift. 	*/
/*	 Weight is valid for convolution.							*/

/*   A file "fileset" is read only once if the file is different from the previously used file. */ 
/*   Format of fileset: filename     x dim(length)  y dim(width)  scale	    wave length of masks.	*/
/*   	  ,stored to: mask_file[], mask_length[], mask_width[], mask_scale[],mask_wl[]			*/

	if ( determine_pgn_function(pgn_func,"total_conductance") ) {
		get_argument(pgn_func,1,arg1);		/* filename for src		*/
		get_argument(pgn_func,2,arg2);		/* filename for lrc, 	includes masks for multi-freq.	*/
		get_argument(pgn_func,3,arg3);		/* filename for bipolar,includes masks for multi-freq.	*/
		get_argument(pgn_func,4,arg4);		/* min.				*/
		get_argument(pgn_func,5,arg5);		/* max.				*/
		get_argument(pgn_func,6,arg6);		/* firing rate: S,R,B or Total	*/
		get_argument(pgn_func,7,arg7);		/* voltage:	S,R,B or Total	*/
		
		sprintf(filename,"%s",arg1);
		min = char_to_float( arg4 );
		max = char_to_float( arg5 );
		
	/* read a file "fileset" */
		if ( strcmp( filename, prev_file ) != 0 ) {
			/* new file name( different from the right before ) */
			strcpy( prev_file, filename );
			if ( maskset_read(filename) == ERROR )		/* returning values stored in externs */
				return(ERROR);
		}
		
	/* compute conductance */
		network = get_network_id(cell->net_id);			/* self */
		connection2 = cell->connect_list;			/* the second connection, Freq 	*/
		in_cell2 = connection2->input_cell;			/* connected cell    	 	*/
		
		connection = connection2 + 1;				/* the first connection, Intensity */
		in_cell = connection->input_cell;

		/* src , Intensity driven */
		src = 0.0; 	
		
		/* lrc */
		lrc = 0.0;
		
		/* bipolar */
		bipc = 0.0;
		
		
	/* output */
		switch (arg6[0]) {
			case 'S': 	cell->firing_rate = src;	break;
			case 'L':	cell->firing_rate = lrc;	break;
			case 'B':	cell->firing_rate = bipc;	break;
			default: 
		      /*ceconvolutionll->firing_rate = src + lrc + bipc;	break;*/
			  cell->firing_rate = src + lrc + bipc;	break;    /* 6/20/2002 H.N.*/
		}
		switch (arg7[0]) {
			case 'S': 	cell->voltage = src;	break;
			case 'L':	cell->voltage = lrc;	break;
			case 'B':	cell->voltage = bipc;	break;
			default: 
				cell->voltage = src + lrc + bipc;	break;
		}
		
		/* max, min clamp */
		if ( cell->firing_rate > max )
			cell->firing_rate = max;
		if ( cell->firing_rate < min )
			cell->firing_rate = min;
		return(OK);
	}
	
/* SRC: look self and neighborhood cells , and determines his SRC. 	
 	SRC:	0 ~ 100	   narrower, stronger (need for gray scale)
 		from 1st connection, intensity
   convolution using parameters in fileset 
*/
/* Weight is valid for convolution.
   Scaling for reducing SRC when no direct input exist.   
   	SRC=(abs(direct_in) + 100)/200,  assuming max=100.  For in=100: Scale=1.0,   for in=0, scale=0.5
   A file "fileset" is read only once if the file is different from the previously used file.  
   	Format of fileset: filename     x dim(length)  y dim(width)  scale	   wave length of masks.	
   	       ,stored to: mask_file[], mask_length[], mask_width[], mask_scale[], mask_wl[]			
*/
	if ( determine_pgn_function(pgn_func,"SRC") ) {
		
		get_argument(pgn_func,1,arg1);		/* filename for src		*/
		get_argument(pgn_func,2,arg2);		/* min.				*/
		get_argument(pgn_func,3,arg3);		/* max.				*/
		get_argument(pgn_func,4,arg4);		/* scale or no: scale by direct intensity or not.	*/
		
		sprintf(filename,"%s",arg1);
		min = char_to_float( arg2 );
		max = char_to_float( arg3 );

	/* read a file "fileset" */
		if ( strcmp( filename, prev_file ) != 0 ) {
			/* new file name( different from the right before ) */
			strcpy( prev_file, filename );
			if ( maskset_read(filename) == ERROR ) 	/* returning values stored in externs */
				return(ERROR);
		}
		
	/* SRC, no feedback 	*/
		/* compute conductance , MUST have only ONE connection */
		connection = cell->connect_list;
		in_cell = connection->input_cell;	/* connected cell    	*/
		if ( arg4[0] != 'n' )						/* scale to reduce SRC when no direct input */
			scale = (fabs(in_cell->firing_rate) + 100.0)/200.0; 	/* assuming max = +/-100 		    */
		else
			scale = 1.0;
								
		/* src , Intensity driven */				
		if ( connection->conductance >= 0.0 )		/* set edge clamp; edges get smaller. 	*/
			edge = min;
		else 
			edge = -min;				/* for off channel 	*/
								/* one mask only 	*/
		cell->voltage = convolution(cell, in_cell, mask_length[0], mask_width[0], mask_coef2[0], edge);
		cell->firing_rate = scale * mask_scale[0] * (connection->conductance) * cell->voltage;		     
			
		/* max, min clamp 	*/
		if ( cell->firing_rate > max )
			cell->firing_rate = max;
		if ( cell->firing_rate < min )
			cell->firing_rate = min;
		return(OK);
	}
		 			
/* SRC  with suppressive feedback from freq. , MUST have TWO connections				*/
/* Turn OFF Real Time Display; Correct SRC is obtained only after all cells in this network is excuted, */
/* if distribution is chosen instead of convolution. Reset all cells to 0 when in turn of cell #0 (but  */
/* this can not be seen until updated).									*/
/* 	SRC:	0 ~ 100	   narrower, stronger (need for gray scale)
 		from 1st connection, intensity
 		from 2nd connection, freq 
   ditribution using parameters in fileset : network scaling in invalid.
*/
/* Weight is valid for distribution.
   Scaling for reducing SRC when no direct input exist.   
   	SRC=(abs(direct_in) + 100)/200,  assuming max=100.  For in=100: Scale=1.0,   for in=0, scale=0.5
   A file "fileset" is read only once if the file is different from the previously used file.  
   	Format of fileset: filename     x dim(length)  y dim(width)  scale	   wave length of masks.	
   	       ,stored to: mask_file[], mask_length[], mask_width[], mask_scale[], mask_wl[]			
*/
/* Feedback from freq. */
/* If freq. < threshold(0.0), SRC=0.  This is to suppress shift of auto-RF to the location where output */
/* of masks are very small.   This helps cells to respond to more optimal location, and to prevent 	*/
/* producing no output for freq.									*/
   
	if ( determine_pgn_function(pgn_func,"SRC_fb") ) {
		double 	distr_scale = 1.0;
		double	suppress_thrshd = 0.0;
		
		get_argument(pgn_func,1,arg1);		/* filename for src				*/
		get_argument(pgn_func,2,arg2);		/* scale or no: scaling by direct intensity	*/
		get_argument(pgn_func,3,arg3);		/* suppression threshold ( 0.0 )		*/
		get_argument(pgn_func,4,arg4);		/* convolution or distribution			*/
		get_argument(pgn_func,5,arg5);		/* min	(conv)					*/
		get_argument(pgn_func,6,arg6);		/* max	(conv)					*/ 
		get_argument(pgn_func,7,arg7);		/* scale , for ditribution.			*/ 
		
		sprintf(filename,"%s",arg1);
		suppress_thrshd = char_to_float(arg3);
		min = char_to_float(arg5);
		max = char_to_float(arg6);
		distr_scale = char_to_float( arg7 );

	/* read a file "fileset" */
		if ( strcmp( filename, prev_file ) != 0 ) {
			/* new file name( different from the right before ) */
			strcpy( prev_file, filename );
			if ( maskset_read(filename) == ERROR ) 	/* returning values stored in externs */
				return(ERROR);
		}
   		
		/* if cell->id = 1, then clear all cells */
			network = get_network_id(cell->net_id);			/* self */
			if ( cell->id == 1 ) {
				for ( i = 0; i < network->number_cells; i++ ) 
					(cell + i)->firing_rate = 0.0;
				/* printf(" All cells cleared to 0.\n"); */
			}
			
		/* compute conductance 	*/
		connection2 = cell->connect_list;			/* 2nd connection, freq.	*/
		in_cell2 = connection2->input_cell;			/* connected cell    	 	*/
		connect_net2 = get_network_id(in_cell2->net_id);	/* connected network 	 	*/
		
		connection = connection2 + 1;				/* 1st connection, intensity	*/
		in_cell = connection->input_cell;			/* connected cell    	 	*/
		connect_net = get_network_id(in_cell->net_id);		/* connected network 	 	*/

		/* scale by direct intensity */
		if ( arg2[0] != 'n' )						/* scale to reduce SRC when no direct input */
			scale = (fabs(in_cell->firing_rate) + 100.0)/200.0; 	/* assuming max = +/-100 		    */
		else
			scale = 1.0;
			
			
		if ( arg4[0] == 'd' ) {
			/* by distribution */
			/* if cell->id = 1, then clear all cells */
			if ( cell->id == 1 ) {
				for ( i = 0; i < network->number_cells; i++ ) 
					(cell + i)->firing_rate = 0.0;
				printf(" SRC by distribution. \n");
				/* printf(" All cells cleared to 0.\n"); */
			}
			
			/* if feedback == 0, SRC = 0  */
			if ( (freq = in_cell2->firing_rate * connection2->conductance) > suppress_thrshd ) 
				distribute(cell, (in_cell->firing_rate)*scale*(connection->conductance)*distr_scale*mask_scale[0]
				   	, mask_length[0], mask_width[0], mask_coef2[0]);
		} else {
			/* by convolution */
			if ( connection->conductance >= 0.0 )		/* set edge clamp; edges get smaller. 	*/
				edge = min;
			else 
				edge = -min;				/* for off channel 	*/
									/* one mask only 	*/
			cell->voltage = convolution(cell, in_cell, mask_length[0], mask_width[0], mask_coef2[0], edge);
			cell->firing_rate = distr_scale * scale * mask_scale[0] * (connection->conductance) * cell->voltage;
			
			if ( in_cell2->firing_rate <= suppress_thrshd )	/* if feedback == 0, SRC = 0 	*/
				cell->firing_rate = 0.0;	     
			
			/* max, min clamp 	*/
			if ( cell->firing_rate > max )
				cell->firing_rate = max;
			if ( cell->firing_rate < min )
				cell->firing_rate = min;
		}
		return(OK);
	}

/* LRC: get directly connected cell's freq., write conductances due to this cell to all perticipant cells.
	Turn OFF Real Time Display; Correct LRC is obtained only after all cells in this network is excuted.  	
   	Reset all cells to 0 when in turn of cell #0 (but this can not be seen until updated).
 	LRC:	0 ~ 30	   wider, weaker (need for gray scale)
 		from 1st connection, freq.
*/
/* Weight is valid for convolution.
   A file "fileset" is read only once if the file is different from the previously used file.  
   Format of fileset: filename     x dim(length)  y dim(width)  scale	    wave length of masks.	
   	  ,stored to: mask_file[], mask_length[], mask_width[], mask_scale[],mask_wl[]			
*/
	if ( determine_pgn_function(pgn_func,"LRC") ) {
		get_argument(pgn_func,1,arg1);		/* filename for lrc		*/
		get_argument(pgn_func,2,arg2);		/* output scaling		*/
		
		sprintf(filename,"%s",arg1);
		scale = char_to_float( arg2 );
		
		/* if cell->id = 1, then clear all cells */
		if ( cell->id == 1 ) {
			for ( i = 0; i < network->number_cells; i++ ) 
				(cell + i)->firing_rate = 0.0;
			/* printf(" All cells cleared to 0.\n"); */
		}
				
		/* read a file "fileset" */
		if ( strcmp( filename, prev_file ) != 0 ) {
			/* new file name( different from the right before ) */
			strcpy( prev_file, filename );
			if ( maskset_read(filename) == ERROR )		/* returning values stored in externs */
				return(ERROR);
		}
		
		/* compute conductance */
		connection = cell->connect_list;
		in_cell = connection->input_cell;		/* connected cell  	*/
		freq = in_cell->firing_rate * connection->conductance;
								/* lrc , Freq. driven 	*/
		if ( freq > 0.8 ) 				/* freq=1,     wl=1  	*/
			distribute(cell, scale*mask_scale[0], mask_length[0], mask_width[0], mask_coef2[0]);
		if ( (freq > 0.6) && (freq <= 0.8) ) 		/* freq=0.666, wl=1.5 	*/
			distribute(cell, scale*mask_scale[1], mask_length[1], mask_width[1], mask_coef2[1]);
		if ( (freq > 0.4) && (freq <= 0.6) ) 		/* freq=0.5,   wl=2	*/
			distribute(cell, scale*mask_scale[2], mask_length[2], mask_width[2], mask_coef2[2]);
		if ( (freq > 0.3) && (freq <= 0.4) ) 		/* freq=0.333, wl=3	*/
			distribute(cell, scale*mask_scale[3], mask_length[3], mask_width[3], mask_coef2[3]);
		if ( (freq > 0.2) && (freq <= 0.3) ) 		/* freq=0.25 , wl=4	*/
			distribute(cell, scale*mask_scale[4], mask_length[4], mask_width[4], mask_coef2[4]);
		if ( (freq > 0.16) && (freq <= 0.2) ) 		/* freq=0.1818,wl=5.5	*/
			distribute(cell, scale*mask_scale[5], mask_length[5], mask_width[5], mask_coef2[5]);
		if ( (freq > 0.11) && (freq <= 0.16) ) 		/* freq=0.125, wl=8.0	*/
			distribute(cell, scale*mask_scale[5], mask_length[6], mask_width[6], mask_coef2[6]);
		if ( (freq > 0.0001) && (freq <= 0.11) ) 	/* freq=0.0909,wl=11.0	*/
			distribute(cell, scale*mask_scale[5], mask_length[7], mask_width[7], mask_coef2[7]);
			/* 0.001 : discard input < 0.01 (if weight 0.01)	*/ 
		return(OK);
	}

/* UNDER DEVELOPMENT */
/* Bipolar: look self and neighborhood cells, and determines his BPC. 	
 	BPC:	0 ~ 50	   
 		from 1st connection, freq. driven
*/
/* Weight is valid for convolution.
   A file "fileset" is read only once if the file is different from the previously used file.  
   Format of fileset: filename     x dim(length)  y dim(width)  scale	    wave length of masks.	
   	  ,stored to: mask_file[], mask_length[], mask_width[], mask_scale[],mask_wl[]			
*/
	if ( determine_pgn_function(pgn_func,"BPC") ) {
		get_argument(pgn_func,1,arg1);		/* filename for bpc		*/
		get_argument(pgn_func,2,arg2);		/* min.				*/
		get_argument(pgn_func,3,arg3);		/* max.				*/
		
		return(OK);  /* UNDER DEVELOPMENT */
		sprintf(filename,"%s",arg1);
		min = char_to_float( arg2 );
		max = char_to_float( arg3 );
		
	/* read a file "fileset" */
		if ( strcmp( filename, prev_file ) != 0 ) {
			/* new file name( different from the right before ) */
			strcpy( prev_file, filename );
			if ( maskset_read(filename) == ERROR )		/* returning values stored in externs */
				return(ERROR);
		}
		
	/* compute conductance */
		connection = cell->connect_list;
		in_cell = connection->input_cell;			/* connected cell    	 */
		connect_net = get_network_id(in_cell->net_id);		/* connected network 	 */
		network = get_network_id(cell->net_id);			/* self */

		/* bpc , freq driven */
		
		
		
		cell->voltage = 0.0;
		cell->firing_rate = mask_scale[i] * (connection->conductance) * cell->voltage;
		 
		/* max, min clamp */
		if ( cell->firing_rate > max )
			cell->firing_rate = max;
		if ( cell->firing_rate < min )
			cell->firing_rate = min;
		return(OK);
	}


/* auto_s , Auto-Tuning in size	*/
/* 
   1st connection : intensity(input),   2nd connection: inside(if inside firing_rate=0)
   Weight is valid before evaluation. 
   if ( convolution x weight ) > max, out = max
   if ( convolution x weight ) < min, out = min
   otherwise,    		      out = convolution x weight
   Then find the mask having max. absolute value.
   Store convolution value(Energy) in firing_rate, and wavelength(order of mask appearance in fileset,Freq) in voltage.
   
   Weight is valid for convolution.
   A file "fileset" is read only once if the file is different from the previously used file.  
   Format of fileset: filename     x dim(length)  y dim(width)  scale	    wave length of masks.	
   	  ,stored to: mask_file[], mask_length[], mask_width[], mask_scale[],mask_wl[]			
   	  
   Shunt by Contour:
   	if (direct input from "inside" != 0) then output 0.   : Prohibit outer cells to respond.(redundant but efficient)
   	if (any potion of RF is "outside") then mask output 0.: Prohibit inner cells to take into account outer region.
  
   If abs(energy) < threshold(arg7), output 0.
   OUTPUT  firing_rate: Ori. Energy,    voltage: Freq.
*/
	if ( determine_pgn_function(pgn_func,"auto_s") ) {
		static FILE	*fp;			/* this file is valid only in this function */
		int		cell_pos_x_ori;
		int		cell_pos_y_ori;
		char		file_rec[MAX_ARG_LEN];
		static int	percent_done;
		
		get_argument(pgn_func,1,arg1);		/* filename 				*/
		get_argument(pgn_func,2,arg2);		/* min. (energy out)			*/
		get_argument(pgn_func,3,arg3);		/* max.	(energy out)			*/
		get_argument(pgn_func,4,arg4);		/* voltage: 	energy, freq. or wave length	*/
		get_argument(pgn_func,5,arg5);		/* firing rate: energy, freq. or wave length	*/
		get_argument(pgn_func,6,arg6);		/* record file, f or n				*/
		get_argument(pgn_func,7,arg7);		/* threshold for voltage to output freq.			   */
							/* e.g. if abs_energy(volt) < thrshd, output freq(firing_rate) = 0 */
		
		sprintf(filename,"%s",arg1);
		min = char_to_float( arg2 );
		max = char_to_float( arg3 );
		
		/* make a record file */
		if ( arg6[0] == 'f' ) {	
			if ( cell->id == 1 ) {
				sprintf( file_rec,"%s_auto.rec",network->name);
				if ( (fp = fopen(file_rec,"w"))==NULL )	{	/* for debug purpose file generation 	*/
					fprintf(stderr," ERROR: auto_s recording file write.  file name=%s\n",file_rec);
					printf("file name: %s\n",file_rec);
					return(OK);
				} else {
					fprintf(stderr," auto_s recording file <%s> created.\n",file_rec);
				}
			}
			if ( cell->id == network->number_cells ) {	/* the last cell is not processed 	*/
				fclose(fp);
				fprintf(stderr," auto_s recording file <%s> completed.\n",file_rec);
				return(OK);
			}
		} 
		
		/* read a file "fileset" */
		if ( strcmp( filename, prev_file ) != 0 ) {
			/* new file name( different from the right before ) */
			strcpy( prev_file, filename );
			if ( maskset_read(filename) == ERROR )		/* returning values stored in externs */
				return(ERROR);
		}
		
		/* % done */
		if ( cell->id == 1 ) 
			percent_done = 1;
		if ( (float)(cell->id) > ((float)(network->number_cells)/10.0)*(float)percent_done ) {
			fprintf(stderr,"     auto_s:  %d %% done.\n",percent_done*10 );
			percent_done++;
		}
		if ( (cell->id) == (network->number_cells) ) {
		}
			
		
		/* connections */
		network = get_network_id(cell->net_id);			/* self 			*/

		connection2 = cell->connect_list;			/* 2nd connection, contour(outside)*/
		in_cell2 = connection2->input_cell;			/* connected cell    	 	*/
		connect_net2 = get_network_id(in_cell2->net_id);	/* connected network 	 	*/
		
		connection = connection2 + 1;				/* 1st connection, intensity	*/
		in_cell = connection->input_cell;			/* connected cell    	 	*/
		connect_net = get_network_id(in_cell->net_id);		/* connected network 	 	*/
		
		/* shunt by contour  	*/
		if ( (in_cell2->firing_rate) != 0.0 ) {		/* if outside of contour, produce 0. 	*/
			cell->firing_rate = 0.0;
			cell->voltage = 0.0;
			return(OK);
		}
		
		/* loop masks  */
		for (loop_mask = 0; loop_mask < num_masks; loop_mask++) {
			length = mask_length[loop_mask];
			width = mask_width[loop_mask];
			
			half_length = length / 2.0;
			half_width = width / 2.0;
			x_low = -1 * half_length;
			x_high = half_length;
			y_low = -1 * half_width;
			y_high = half_width;


			flag_edge = OFF;
			if (not_at_edge(cell->id, cell->net_id, network->name) == TRUE) {	/* check self */
				/* self not at edge */
				for (bias_x = x_low; bias_x <= x_high; bias_x++) {
				for (bias_y = y_low; bias_y <= y_high; bias_y++) {
					/* get values from connected cells */
					neighbor_cell = get_neighboor(in_cell, connect_net, bias_x, bias_y);
					neighbor_cell2 = get_neighboor(in_cell2, connect_net2, bias_x, bias_y);  /* outside */
					if ( (not_at_edge(neighbor_cell->id, cell->net_id, network->name) == FALSE) || 
					     (neighbor_cell2->firing_rate != 0.0) ) {
						/* any cell in RF is at edge OR outside of contour */
						flag_edge = ON;
						break;
					} else {
						/* not at edge OR at outside, store connected cell's firing rates in value2[][] */
						value2[bias_x + x_high][bias_y + y_high] = neighbor_cell->firing_rate;
					}
				}}
				if (flag_edge == OFF) {	
				/* compute convolution */
					value_m = 0;
					number_element = 0;
					for (bias_y = 0; bias_y < width; bias_y++) {
					for (bias_x = 0; bias_x < length; bias_x++) {
						value_m += value2[bias_x][bias_y] * mask_coef2[loop_mask][number_element];
						number_element++;
					}}
					mask_out[loop_mask] = mask_scale[loop_mask] * connection->conductance * value_m;
				}
			} else {
				/* self at edge */
				flag_edge = ON;
			}
			
			if (flag_edge == ON) {
				mask_out[loop_mask] = 0.0;
			}
		} /* end mask loop */
		
		/* find abs max.   on, off are determined by min, max. */
		if ( arg6[0] == 'f' ) {
			cell_pos_x_ori = cell_position_x( cell->id, network->dim_x, network->dim_y );
			cell_pos_y_ori = cell_position_y( cell->id, network->dim_x, network->dim_y );
			fprintf( fp,"%6d   %3d %3d ",cell->id,cell_pos_x_ori,cell_pos_y_ori);
		}
		abs_max = 0.0;
		max_wl = 0.0;
		cell->firing_rate = 0.0;
		cell->voltage = 0.0;
		for ( loop_mask = 0; loop_mask < num_masks; loop_mask++ ) {
			/* max, min clamp before comparison */
			if ( mask_out[loop_mask] < min )
				mask_out[loop_mask] = min;
			if ( mask_out[loop_mask] > max )
				mask_out[loop_mask] = max;
				
			if ( fabs(mask_out[loop_mask]) > abs_max ) {
				abs_max = fabs(mask_out[loop_mask]);
				max_wl = mask_wl[loop_mask];
				switch ( arg5[0] ) {
					case 'e': cell->firing_rate = mask_out[loop_mask];  /* not abs */
						  break;
					case 'f': cell->firing_rate = 1.0 / mask_wl[loop_mask];
						  break;
					case 'w': cell->firing_rate = mask_wl[loop_mask];
						  break;
				}
				switch ( arg4[0] ) {
					case 'e': cell->voltage = mask_out[loop_mask];      /* not abs */
						  break;
					case 'f': cell->voltage = 1.0 / mask_wl[loop_mask];
						  break;
					case 'w': cell->voltage = mask_wl[loop_mask];
						  break;
				}
			}

			if ( arg6[0] == 'f' ) 
				fprintf( fp,"%6.2f ",mask_out[loop_mask]);
		}
		if ( arg6[0] == 'f' ) 
			fprintf( fp,"  %6.2f\n",max_wl);
			
		if ( fabs(cell->voltage) < char_to_float(arg7) ) 	/* thresholding by abs voltage */
			cell->firing_rate = 0.0;

		return(OK);
	}

/* auto_ss , Auto-Tuning in Size(by Local Max) and Shift	*/
/* Shift:
   	If direct Conductance > threshold, look nearest neighbor(8) cells.
   	If (the neighbor cell > threshold for shift), move to the highest location.  Continue until get connected max. 
   	If (the connected max. > threshold for convolution), compute masks at the location as follows.
   	
   Size by Local max:
   	Weight is valid before evaluation. 
   	if ( convolution x weight ) > max, out = max
   	if ( convolution x weight ) < min, out = min
   	otherwise,    		      out = convolution x weight
   	Then find the mask having max. absolute value.
  	 Store convolution value(Energy) in firing_rate, and wavelength(order of mask appearance in fileset,Freq) in voltage.
  	 
   Shunt by Contour:
   	if (direct input from "outside" net= 0.0) then output 0.   : Prohibit outer cells to respond.
   	if (shifted position of "outside" net= 0.0) then output 0. : Prohibit inner cells to respond outside.
   	(mask RF may includes "outside".)
   	
   A file "fileset" is read only once if the file is different from the previously used file.  
   Format of fileset: filename     x dim(length)  y dim(width)  scale	    wave length of masks.	
   	  ,stored to: mask_file[], mask_length[], mask_width[], mask_scale[],mask_wl[]			
   	  
   1st connection: intensity,   2nd connection: conductance   Dimensions of 1st and 2nd nets MUST be SAME.
   3rd connection: inside/outside of the object.  
   OUTPUT  Ori. Energy(abs), Freq. or wave length
*/
	if ( determine_pgn_function(pgn_func,"auto_ss") ) {	
		static FILE	*fp;			/* this file is valid only in this function */
		static int	percent_done;
		int		cell_pos_x_ori;
		int		cell_pos_y_ori;
		float		direct_conduct;
		char		file_rec[MAX_ARG_LEN];
		
		get_argument(pgn_func,1,arg1);		/* fileset name 		*/
		get_argument(pgn_func,2,arg2);		/* min.	(mask out)		*/
		get_argument(pgn_func,3,arg3);		/* max.	(mask out)		*/
		get_argument(pgn_func,4,arg4);		/* voltage:    freq, wave length, abs(energy)			*/
		get_argument(pgn_func,5,arg5);		/* firing_rate:freq, wave length, abs(energy)			*/
		get_argument(pgn_func,6,arg6);		/* threshold for shift(difference of conductance)		*/
		get_argument(pgn_func,7,arg7);		/* threshold for convolution(value of conductance) 		*/
		get_argument(pgn_func,8,arg8);		/* max. distance for shift 					*/
		get_argument(pgn_func,9,arg9);		/* threshold of min. abs. energy to produce firing_rate(freq) 	*/
		get_argument(pgn_func,10,arg10);	/* generate a record file					*/
		
		sprintf(filename,"%s",arg1);
		min = char_to_float( arg2 );
		max = char_to_float( arg3 );
		thrshd_shift = char_to_float(arg6);
		thrshd_conv  = char_to_float(arg7);
		max_dist     = atoi(arg8);
		thrshd_go    = char_to_float(arg9);
		
		network = get_network_id(cell->net_id);			/* self 			*/

		connection3 = cell->connect_list;			/* 3rd connection, contour(outside)*/
		in_cell3 = connection3->input_cell;			/* connected cell    	 	*/
		connect_net3 = get_network_id(in_cell3->net_id);	/* connected network 	 	*/

		connection2 = connection3 + 1;				/* 2nd connection, conductance  */
		in_cell2 = connection2->input_cell;			/* connected cell    	 	*/
		connect_net2 = get_network_id(in_cell2->net_id);	/* connected network 	 	*/
		direct_conduct = in_cell2->firing_rate;
		
		connection = connection2 + 1;				/* 1st connection, intensity	*/
		in_cell = connection->input_cell;			/* connected cell    	 	*/
		connect_net = get_network_id(in_cell->net_id);		/* connected network 	 	*/


		if ( arg10[0] == 'f' ) {	/* make a record file */
			if ( cell->id == 1 ) {
				sprintf( file_rec,"%s_auto.rec",network->name);
				if ( (fp = fopen(file_rec,"w"))==NULL )	{	/* for debug purpose file generation 	*/
					fprintf(stderr," ERROR: auto_ss recording file write.  file name=%s\n",file_rec);
					printf("file name: %s\n",file_rec);
					return(OK);
				}
			}
			if ( cell->id == network->number_cells ) {	/* the last cell is not processed 	*/
				fclose(fp);
				fprintf(stderr," auto_ss recording file <%s> completed.\n",file_rec);
				return(OK);
			}
		} 
		
		if ( cell->id == 1 ) 
			percent_done = 1;
		if ( (float)(cell->id) > ((float)(network->number_cells)/10.0)*(float)percent_done ) {
			fprintf(stderr,"     auto_ss:  %d %% done.\n",percent_done*10 );
			percent_done++;
		}
		
	/* shunt by contour  	*/
		if ( (in_cell3->firing_rate) == 0.0 ) {		/* if outside of contour, produce 0. 	*/
			cell->firing_rate = 0.0;
			cell->voltage = 0.0;
			return(OK);
		}
						
	/* find Connected max.  */
		if (not_at_edge(cell->id, cell->net_id, network->name) == FALSE) { 	/* check self */
			cell->firing_rate = 0.0;
			cell->voltage = 0.0;
			return(OK);
		}
		if ( in_cell2->firing_rate > cell->threshold ) {
			for( j = 0; j < max_dist; j++ ) {
				/* find max nearest neighbor */
				max_conduct = in_cell2->firing_rate;
				max_cell = in_cell2;					/* for safety */
				/* look for direct neighbors */
				for ( i = 0; i <= 7; i++ ) {
					neighbor_cell = get_nearest_neighbor(in_cell2, connect_net2, i);
					if (not_at_edge(neighbor_cell->id, in_cell2->net_id, connect_net2->name) == TRUE) {
						/* do not move to edge. */
						if ( neighbor_cell->firing_rate > max_conduct ) {
							max_cell = neighbor_cell;
							max_conduct = neighbor_cell->firing_rate;
					}	}
				}
				
				/* shift to max. place */
				if ( (max_conduct - in_cell2->firing_rate) > thrshd_shift ) 
					in_cell2 = max_cell;
				else 
					break;					/* max can be itself(no shift) */
			} 
			
			if ( (in_cell2->firing_rate < thrshd_conv) || (j >= (max_dist - 1)) ) {
				/* max. conductance too small  OR  max. too far */
				cell->firing_rate = 0.0;
				cell->voltage = 0.0;
				return(OK);
			}
			
		} else {
			/* direct conductance < threshold */
			cell->firing_rate = 0.0;
			cell->voltage = 0.0;
			return(OK);
		}

	/* shift intensity cell */
		cell_pos_x = cell_position_x( in_cell2->id, connect_net2->dim_x, connect_net2->dim_y );
		cell_pos_y = cell_position_y( in_cell2->id, connect_net2->dim_x, connect_net2->dim_y );
		in_cell = get_cell_at_position( connect_net->name, cell_pos_x, cell_pos_y );
		
		/* shunt by contour */
		in_cell3 = get_cell_at_position( connect_net3->name, cell_pos_x, cell_pos_y );
		if ( (in_cell3->firing_rate) == 0.0 ) {	   /* if the shifted position is outside, produce 0. */
			cell->firing_rate = 0.0;
			cell->voltage = 0.0;
			return(OK);
		}
		
		/* printf(" mask center(shifted or not): %d %d \n",cell_pos_x, cell_pos_y); */
		if ( arg10[0] == 'f' ) {
			cell_pos_x_ori = cell_position_x( cell->id, network->dim_x, network->dim_y );
			cell_pos_y_ori = cell_position_y( cell->id, network->dim_x, network->dim_y );
			fprintf( fp,"%6d   %3d %3d %7.3f    %3d %3d %7.3f    "
			   ,cell->id,cell_pos_x_ori,cell_pos_y_ori,direct_conduct, cell_pos_x,cell_pos_y,in_cell2->firing_rate);
		}
			
	/* read a file "fileset" */
		if ( strcmp( filename, prev_file ) != 0 ) {
			/* new file name( different from the right before ) */
			strcpy( prev_file, filename );
			if ( maskset_read(filename) == ERROR )		/* returning values stored in externs */
				return(ERROR);
		}
		
	/* loop masks  */
		for (loop_mask = 0; loop_mask < num_masks; loop_mask++) {
			length = mask_length[loop_mask];
			width = mask_width[loop_mask];
			
			half_length = length / 2.0;
			half_width = width / 2.0;
			x_low = -1 * half_length;
			x_high = half_length;
			y_low = -1 * half_width;
			y_high = half_width;

			flag_edge = OFF;
			if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {	/* check self */
				/* not at edge */
				for (bias_x = x_low; bias_x <= x_high; bias_x++) {
				for (bias_y = y_low; bias_y <= y_high; bias_y++) {
					/* get values from connected cells */
					neighbor_cell = get_neighboor(in_cell, connect_net, bias_x, bias_y);
					if (not_at_edge(neighbor_cell->id, in_cell->net_id, connect_net->name) == FALSE) {
						/* at edge */
						flag_edge = ON;
						break;
					} else {
						/* not at edge, store connected cell's firing rates in value2[][] */
						value2[bias_x + x_high][bias_y + y_high] = neighbor_cell->firing_rate;
					}
				}}
				if (flag_edge == OFF) {	
				/* compute convolution */
					value_m = 0;
					number_element = 0;
					for (bias_y = 0; bias_y < width; bias_y++) {
					for (bias_x = 0; bias_x < length; bias_x++) {
						value_m += value2[bias_x][bias_y] * mask_coef2[loop_mask][number_element];
						number_element++;
					}}
					mask_out[loop_mask] = mask_scale[loop_mask] * connection->conductance * value_m;
				}
			} else {
				/* at edge */
				flag_edge = ON;
			}
			
			if (flag_edge == ON) {
				mask_out[loop_mask] = 0.0;
			}
		} /* end mask loop */
		
	/* find abs max.   on, off are determined by min, max. */
		abs_max = 0.0;
		max_freq = 0.0;
		max_wl = 0.0;
		for ( loop_mask = 0; loop_mask < num_masks; loop_mask++ ) {
			/* max, min clamp before comparison, for +/- separation */
			if ( mask_out[loop_mask] < min )
				mask_out[loop_mask] = min;
			if ( mask_out[loop_mask] > max )
				mask_out[loop_mask] = max;
				
			if ( fabs(mask_out[loop_mask]) > abs_max ) {	/* abs for - channels */
				abs_max = fabs(mask_out[loop_mask]);
				max_freq = 1.0 / mask_wl[loop_mask];
				max_wl = mask_wl[loop_mask];
			}
			if ( arg10[0] == 'f' )
				fprintf( fp,"%7.3lf ",mask_out[loop_mask]);
		}
		if ( arg10[0] == 'f' )
			fprintf( fp,"  %4.1lf\n",max_wl);
		
		if ( abs_max >= thrshd_go ) {		/* if the energy is smaller than the threshold, firing_rate = 0 */
			switch ( arg5[0] ) {
				case 'e': cell->firing_rate = abs_max;	/* abs energy 	*/
				 	 break;
				case 'w': cell->firing_rate = max_wl;	/* wave length */
					  break;
				case 'f':
				default : cell->firing_rate = max_freq;	/* freq		*/
				  	break;
			}
		} else {
			cell->firing_rate = 0.0;
		}	
		switch ( arg4[0] ) {
			case 'f': cell->voltage = max_freq;
				  break;
			case 'w': cell->voltage = max_wl;
				  break;
			case 'e':
			default : cell->voltage = abs_max;		/* abs energy 	*/
				  break;
		}
	
		return(OK);
	}

/* auto_sss , Auto-Tuning in Size(by Local Max) and Shift by DIRECT connections from SRC & LRC	*/
/* Shift:
   	If (direct Conductance > cell->threshold), look nearest neighbor(8) cells.
   	If (the neighbor cell > threshold for shift), move to the highest location.  Continue until get connected max. 
   	If (the connected max. > threshold for convolution), compute masks at the location as follows.
   	
   Size by Local max:
   	Weight is valid before evaluation. 
   	if ( convolution x weight ) > max, out = max
   	if ( convolution x weight ) < min, out = min
   	otherwise,    		      out = convolution x weight
   	Then find the mask having max. absolute value.
  	 Store convolution value(Energy) in firing_rate, and wavelength(order of mask appearance in fileset,Freq) in voltage.
   
   A file "fileset" is read only once if the file is different from the previously used file.  
   Format of fileset: filename     x dim(length)  y dim(width)  scale	    wave length of masks.	
   	  ,stored to: mask_file[], mask_length[], mask_width[], mask_scale[],mask_wl[]			
   	  
   Connections:  1st: intensity,   2nd: SRC,    3rd: LRC  	;Dimensions of ALL nets MUST be SAME.
   OUTPUT  Ori. Energy(abs), Freq. or wave length
*/
	if ( determine_pgn_function(pgn_func,"auto_sss") ) {
		get_argument(pgn_func,1,arg1);		/* fileset name 		*/
		get_argument(pgn_func,2,arg2);		/* min.	(energy out)		*/
		get_argument(pgn_func,3,arg3);		/* max.	(energy out)		*/
		get_argument(pgn_func,4,arg4);		/* voltage:    freq, wave length, abs(energy)			*/
		get_argument(pgn_func,5,arg5);		/* firing_rate:freq, wave length, abs(energy)			*/
		get_argument(pgn_func,6,arg6);		/* threshold for shift(difference of conductance)		*/
		get_argument(pgn_func,7,arg7);		/* threshold for convolution(value of conductance) 		*/
		get_argument(pgn_func,8,arg8);		/* max. distance for shift 					*/
		get_argument(pgn_func,9,arg9);		/* threshold of min. abs. energy to produce firing_rate(freq) 	*/
		
		sprintf(filename,"%s",arg1);
		min = char_to_float( arg2 );
		max = char_to_float( arg3 );
		thrshd_shift = char_to_float(arg6);
		thrshd_conv  = char_to_float(arg7);
		max_dist     = atoi(arg8);
		thrshd_go    = char_to_float(arg9);
		
		network = get_network_id(cell->net_id);			/* self 			*/
		
		connection3 = cell->connect_list;			/* 3rd connection, LRC		*/
		in_cell3 = connection3->input_cell;			/* connected cell    	 	*/
		connect_net3 = get_network_id(in_cell3->net_id);	/* connected network 	 	*/

		connection2 = connection3 + 1;				/* 2nd connection, SRC 		*/
		in_cell2 = connection2->input_cell;			/* connected cell    	 	*/
		connect_net2 = get_network_id(in_cell2->net_id);	/* connected network 	 	*/
		
		connection = connection2 + 1;				/* 1st connection, intensity	*/
		in_cell = connection->input_cell;			/* connected cell    	 	*/
		connect_net = get_network_id(in_cell->net_id);		/* connected network 	 	*/
		
	/* find Connected max.   */
		if (not_at_edge(cell->id, cell->net_id, network->name) == FALSE) { 	/* check self */
			cell->firing_rate = 0.0;
			cell->voltage = 0.0;
			return(OK);
		}
		if ( (in_cell2->firing_rate + in_cell3->firing_rate) > cell->threshold ) {
			for( j = 0; j < max_dist; j++ ) {
				/* find max nearest neighbor */
				max_conduct = in_cell2->firing_rate + in_cell3->firing_rate;	/* SRC + LRC  */
				max_cell  = in_cell2;						/* for safety */
				max_cell2 = in_cell3;						/* for safety */
				for ( i = 0; i <= 7; i++ ) {
					neighbor_cell = get_nearest_neighbor(in_cell2, connect_net2, i);
					neighbor_cell2 = get_nearest_neighbor(in_cell3, connect_net3, i);
					if (not_at_edge(neighbor_cell->id, in_cell2->net_id, connect_net2->name) == TRUE) {
						/* do not move to edge. */
						if ( (neighbor_cell->firing_rate + neighbor_cell2->firing_rate) > max_conduct ) {
							max_cell = neighbor_cell;
							max_cell2 = neighbor_cell2;
							max_conduct = neighbor_cell->firing_rate + neighbor_cell2->firing_rate;
					}	}
				}
				
				/* shift to max. place */
				if ( (max_conduct - (in_cell2->firing_rate + in_cell3->firing_rate)) > thrshd_shift ) {
					in_cell2 = max_cell;
					in_cell3 = max_cell2;
				} else {
					break;					/* max can be itself(no shift) */
				}
			} 
			
			if ( ((in_cell2->firing_rate + in_cell3->firing_rate) < thrshd_conv) || (j >= (max_dist - 1)) ) {
				/* max. conductance too small  OR  max. too far */
				cell->firing_rate = 0.0;
				cell->voltage = 0.0;
				return(OK);
			}
			
		} else {
			/* direct conductance < threshold */
			cell->firing_rate = 0.0;
			cell->voltage = 0.0;
			return(OK);
		}

	/* shift intensity cell */
		cell_pos_x = cell_position_x( in_cell2->id, connect_net2->dim_x, connect_net2->dim_y );
		cell_pos_y = cell_position_y( in_cell2->id, connect_net2->dim_x, connect_net2->dim_y );
		in_cell = get_cell_at_position( connect_net->name, cell_pos_x, cell_pos_y );
		/* printf(" mask center(shifted or not): %d %d \n",cell_pos_x, cell_pos_y); */

	/* read a file "fileset" */
		if ( strcmp( filename, prev_file ) != 0 ) {
			/* new file name( different from the right before ) */
			strcpy( prev_file, filename );
			if ( maskset_read(filename) == ERROR )		/* returning values stored in externs */
				return(ERROR);
		}
		
	/* loop masks  */
		for (loop_mask = 0; loop_mask < num_masks; loop_mask++) {
			length = mask_length[loop_mask];
			width = mask_width[loop_mask];
			
			half_length = length / 2.0;
			half_width = width / 2.0;
			x_low = -1 * half_length;
			x_high = half_length;
			y_low = -1 * half_width;
			y_high = half_width;

			flag_edge = OFF;
			if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {	/* check self */
				/* not at edge */
				for (bias_x = x_low; bias_x <= x_high; bias_x++) {
				for (bias_y = y_low; bias_y <= y_high; bias_y++) {
					/* get values from connected cells */
					neighbor_cell = get_neighboor(in_cell, connect_net, bias_x, bias_y);
					if (not_at_edge(neighbor_cell->id, in_cell->net_id, connect_net->name) == FALSE) {
						/* at edge */
						flag_edge = ON;
						break;
					} else {
						/* not at edge, store connected cell's firing rates in value2[][] */
						value2[bias_x + x_high][bias_y + y_high] = neighbor_cell->firing_rate;
					}
				}}
				if (flag_edge == OFF) {	
				/* compute convolution */
					value_m = 0;
					number_element = 0;
					for (bias_y = 0; bias_y < width; bias_y++) {
					for (bias_x = 0; bias_x < length; bias_x++) {
						value_m += value2[bias_x][bias_y] * mask_coef2[loop_mask][number_element];
						number_element++;
					}}
					mask_out[loop_mask] = mask_scale[loop_mask] * connection->conductance * value_m;
				}
			} else {
				/* at edge */
				flag_edge = ON;
			}
			
			if (flag_edge == ON) {
				mask_out[loop_mask] = 0.0;
			}
		} /* end mask loop */
		
	/* find abs max.   on, off are determined by min, max. */
		abs_max = 0.0;
		max_freq = 0.0;
		max_wl = 0.0;
		for ( loop_mask = 0; loop_mask < num_masks; loop_mask++ ) {
			/* max, min clamp before comparison, for +/- separation */
			if ( mask_out[loop_mask] < min )
				mask_out[loop_mask] = min;
			if ( mask_out[loop_mask] > max )
				mask_out[loop_mask] = max;
				
			if ( fabs(mask_out[loop_mask]) > abs_max ) {
				abs_max = fabs(mask_out[loop_mask]);
				max_freq = 1.0 / mask_wl[loop_mask];
				max_wl = mask_wl[loop_mask];
			}
		}
		
		if ( abs_max >= thrshd_go ) {		/* if the energy is smaller than the threshold, firing_rate = 0 */
			switch ( arg5[0] ) {
				case 'e': cell->firing_rate = abs_max;
				 	 break;
				case 'w': cell->firing_rate = max_wl;
					  break;
				case 'f':
				default : cell->firing_rate = max_freq;
				  	break;
			}
		} else {
			cell->firing_rate = 0.0;
		}	
		switch ( arg4[0] ) {
			case 'f': cell->voltage = max_freq;
				  break;
			case 'w': cell->voltage = max_wl;
				  break;
			case 'e':
			default : cell->voltage = abs_max;
				  break;
		}
		return(OK);
	}

/* get voltage of teh connected cell. */	
	if (strcmp(pgn_func, "get_voltage") == 0) {
		connection = cell->connect_list;
		in_cell = connection->input_cell;	
			
		/* get the voltage of the connected cell	*/	
		cell->voltage = in_cell->voltage;		
		cell->firing_rate = connection->conductance * cell->voltage;
		return(OK);
	}

/* dilation by freq. and ori. energy.	*/
/* RF candidate is determined by the freq.(propotional to wavelength).  Actual RF is determined by the competition of 	  */
/* the cells which claim the location as their RF candidatedidates.  The cell having strongest energy takes the location. */
/* 	The cell looks for non-zero Freq. in possible largest region and check whether the non-zero cell claims the 	*/
/*	position or not.  If he claims, store his Freq. and Ori.E.  After the inspection, find the connected cell 	*/
/* 	having the maximum Ori.E., and take his Freq. as firing rate.							*/
/* Freq. is given by the first connection.   Ori.Energy is given by the second connection. 				*/
/* arg[1] = scale for candidate RF; distance = scale x wavelength(=1/freq)						*/
/* OUTPUT is freq. 													*/

	if ( determine_pgn_function(pgn_func,"dil_freq_energy") ) {
		get_argument(pgn_func, 1, arg1);	/* scaling for RF size 			*/
		get_argument(pgn_func, 2, arg2);	/* if v , get freq from voltage		*/
		scale = char_to_float(arg1);			/* scale x wavelength = RF candidate 	*/
		
		/* determine possible largest region */
		max_distance = MAX_WAVE * scale;
		range1_n = -max_distance;
		range1_p = max_distance;
		range2_n = -max_distance;
		range2_p = max_distance;

		connection2 = cell->connect_list;			/* the second connection, Ori. Energy 	*/
		in_cell2 = connection2->input_cell;			/* connected cell    	 		*/
		connect_net2 = get_network_id(in_cell2->net_id);	/* connected network 	 		*/

		connection = connection2 + 1;				/* the first connection, Freq(NOT wave length) */
		in_cell = connection->input_cell;
		connect_net = get_network_id(in_cell->net_id);

		/* x,y low and high limit due to edge */
		x_low_limit = x_limit_low(connect_net, in_cell, range1_n);
		x_high_limit = x_limit_high(connect_net, in_cell, range1_p);
		y_low_limit = y_limit_low(connect_net, in_cell, range2_n);
		y_high_limit = y_limit_high(connect_net, in_cell, range2_p);
		/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n"
				, in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit); */

		/* look for claiming cell */
		max_energy = 0.0; 
		max_freq = 0.0;
		for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
		for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
			if ( (fdistance = (double)((bias_x * bias_x) + (bias_y * bias_y))) <= ( max_distance*max_distance )) {
				/* within distance */
				neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
				if ( arg2[0]=='v' )
					freq = ((connection->conductance) * (neighbor_cell->voltage));
				else
					freq = ((connection->conductance) * (neighbor_cell->firing_rate));
				if ( ((1.0/freq) * scale) > sqrt(fdistance) ) { 			/* must be freq. */
					/* the position is claimed by the connected cell */
					neighbor_cell = in_cell2 + (bias_y * connect_net2->dim_x + bias_x);
					energy = ((connection2->conductance) * (neighbor_cell->firing_rate));
					if ( energy > max_energy ) {
						max_energy = energy;
						max_freq = freq;
					}
				}
			}
		}}

		/* output is always >= 0 , because max_freq never go below 0 */
		cell->voltage = max_energy;
		cell->firing_rate = max_freq;
		return(OK);
	}


/* freq by thresholding ori. energy */
/* If Ori. Energy > Threshold , then take freq.   Otherwise, freq = 0; */

	if ( determine_pgn_function(pgn_func,"freq_by_thr_egy") ) {
	
		connection2 = cell->connect_list;			/* the second connection, Ori. Energy 	*/
		in_cell2 = connection2->input_cell;			/* connected cell    	 		*/

		connection = connection2 + 1;				/* the first connection, Freq(wave length) */
		in_cell = connection->input_cell;
		
		
		if ( in_cell2->firing_rate > cell->threshold ) 
			cell->firing_rate = (connection->conductance) * (in_cell->firing_rate); 
		else
			cell->firing_rate = 0.0;
			
		return(OK);
	}
	

/* New averaging  */
/* If the cell is on edge, =0.  Area outside of the edge is ignored. The # of cell is also decremented. */	
/* take average of a circular region of diameter x of ONE networks.  sigma of gaussian is radius 	*/

	if ( determine_pgn_function(pgn_func,"ave") ) {
		get_argument(pgn_func,1,arg1);			/* diameter */
		
		connection = cell->connect_list;
		in_cell = connection->input_cell;		/* connected cell    	 */
		connect_net = get_network_id(in_cell->net_id);	/* connected network 	 */
		network = get_network_id(cell->net_id);		/* self */

	/* If connected cell = 0, out = 0 		for non-zero ave. 
		if ( in_cell->firing_rate == 0.0 ) {	
			cell->firing_rate = 0.0;
			return(OK);
		} 
	*/	
		diamet = atoi( arg1 );			/* diamet,radius are integer */
		radius = diamet / 2.0;
		
		range1_n = -radius;
		range1_p = radius;
		range2_n = -radius;
		range2_p = radius;
		distance = radius;
		sig = (double)radius;
		if ( diamet == 5  ) scale = 2.56;
		if ( diamet == 7  ) scale = 5.71;
		if ( diamet == 11 ) scale = 16.0;	       /* max ~ 1.00, min ~ 0.37 */
		if ( diamet == 13 ) scale = 23.0;
		if ( diamet == 17 ) scale = 41.0;
		if ( diamet == 25 ) scale = 90.0;
		if ( diamet == 33 ) scale = 162.0;
		if ( diamet == 41 ) scale = 256.0;
		if ( diamet == 61 ) scale = 558.0;
		
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* if not at edge */
			/* x,y low and high limit due to edge */
			x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
			x_high_limit = x_limit_high( connect_net, in_cell, range1_p );

			y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
			y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
			/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n",
						in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit); */

			cell_count = 0;
			sum = 0.0;
			for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
			for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
				if (((bias_x * bias_x) + (bias_y * bias_y)) <= (distance * distance)) {
					/* within distance */
					neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
					sum = sum + gaussian( bias_x,bias_y,sig,scale)*(neighbor_cell->firing_rate) ;
					cell_count++;
				}
			}
			}
			cell->voltage = sum;
			cell->firing_rate = connection->conductance * (sum / (double)cell_count);

		} else {
			/* all 4 peripheral edges */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}

/* anisotropic Gaussian */
/* originally from "ave" above.       			*/
/* arg1: x diameter, arg2: y diameter, arg3: rotation 	*/
/* UNDER DEVELOPMENT */

	if ( determine_pgn_function(pgn_func,"aniso_ave") ) {
		get_argument(pgn_func,1,arg1);			/* diameter */
		
		connection = cell->connect_list;
		in_cell = connection->input_cell;		/* connected cell    	 */
		connect_net = get_network_id(in_cell->net_id);	/* connected network 	 */
		network = get_network_id(cell->net_id);		/* self */

	/* If connected cell = 0, out = 0 		for non-zero ave. 
		if ( in_cell->firing_rate == 0.0 ) {	
			cell->firing_rate = 0.0;
			return(OK);
		} 
	*/	
		diamet = atoi( arg1 );			/* diamet,radius are integer */
		radius = diamet / 2.0;
		
		range1_n = -radius;
		range1_p = radius;
		range2_n = -radius;
		range2_p = radius;
		distance = radius;
		sig = (double)radius;
		if ( diamet == 5  ) scale = 2.56;
		if ( diamet == 7  ) scale = 5.71;
		if ( diamet == 11 ) scale = 16.0;	       /* max ~ 1.00, min ~ 0.37 */
		if ( diamet == 13 ) scale = 23.0;
		if ( diamet == 17 ) scale = 41.0;
		if ( diamet == 25 ) scale = 90.0;
		if ( diamet == 33 ) scale = 162.0;
		if ( diamet == 41 ) scale = 256.0;
		if ( diamet == 61 ) scale = 558.0;
		
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* if not at edge */
			/* x,y low and high limit due to edge */
			x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
			x_high_limit = x_limit_high( connect_net, in_cell, range1_p );

			y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
			y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
			/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n",
						in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit); */

			cell_count = 0;
			sum = 0.0;
			for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
			for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
				if (((bias_x * bias_x) + (bias_y * bias_y)) <= (distance * distance)) {
					/* within distance */
					neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
					sum = sum + gaussian( bias_x,bias_y,sig,scale)*(neighbor_cell->firing_rate) ;
					cell_count++;
				}
			}
			}
			cell->voltage = sum;
			cell->firing_rate = connection->conductance * (sum / (double)cell_count);

		} else {
			/* all 4 peripheral edges */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}

/* none zero averaging  	 					*/
/* If the cell is on edge, =0.  Area outside of the edge is ignored. 	*/
/* If value within the region is 0, then # of cell is also decremented. */	
/* take Gaussian average of a circular region of diameter x of ONE networks 	*/

	if ( determine_pgn_function(pgn_func,"nozero_ave") ) {
		get_argument(pgn_func,1,arg1);			/* diameter */
		
		connection = cell->connect_list;
		in_cell = connection->input_cell;		/* connected cell    	 */
		connect_net = get_network_id(in_cell->net_id);	/* connected network 	 */
		network = get_network_id(cell->net_id);		/* self */

		/* If connected cell = 0, out = 0 		for non-zero ave. */
		if ( in_cell->firing_rate == 0.0 ) {	
			cell->firing_rate = 0.0;
			return(OK);
		} 
			

		diamet = atoi( arg1 );			/* diamet,radius are integer */
		radius = diamet / 2.0;
		
		range1_n = -radius;
		range1_p = radius;
		range2_n = -radius;
		range2_p = radius;
		distance = radius;
		sig = (double)radius;				/* sigma = radius 		*/
		if ( diamet == 5  ) scale = 2.56;
		if ( diamet == 7  ) scale = 5.71;
		if ( diamet == 11 ) scale = 16.0;	       	/* max ~ 1.00, min ~ 0.37 	*/
		if ( diamet == 13 ) scale = 23.0;
		if ( diamet == 17 ) scale = 41.0;
		if ( diamet == 25 ) scale = 90.0;
		if ( diamet == 33 ) scale = 162.0;
		if ( diamet == 41 ) scale = 256.0;
		
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* if not at edge */
			/* x,y low and high limit due to edge */
			x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
			x_high_limit = x_limit_high( connect_net, in_cell, range1_p );

			y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
			y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
			/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n",
						in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit); */
			cell_count = 0;
			sum = 0.0;
			for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
			for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
				if (((bias_x * bias_x) + (bias_y * bias_y)) <= (distance * distance)) {
					/* within distance */
					neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
					if ( neighbor_cell->firing_rate != 0.0 ) {
						/* value is NON ZERO */
						sum = sum + gaussian( bias_x,bias_y,sig,scale)*(neighbor_cell->firing_rate) ;
						cell_count++;
				}	}
			}}
			if ( cell_count != 0 ) {
				cell->voltage = sum;
				cell->firing_rate = connection->conductance * (sum / (double)cell_count);
			} else {
				cell->firing_rate = 0.0;
			}
		} else {
			/* all 4 peripheral edges */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}

/* Averaging only in INSIDE.(similar to nozero_ave)  Instead of using zero value for exclusion, use Inside net. */
/* If inside_net within the RF is non-zero, then # of cell is also decremented. 				*/	
/* take Gaussian average of a circular region of diameter x of ONE networks 						*/

	if ( determine_pgn_function(pgn_func,"inside_ave") ) {
		get_argument(pgn_func,1,arg1);			/* diameter */
		
								/* 3rd connection, dummy_to is not on this list */
		connection2 = cell->connect_list;		/* 2nd connection: inside */
		in_cell2 = connection2->input_cell;		
		connect_net2 = get_network_id(in_cell2->net_id); /* inside net    	*/

		connection = connection2 + 1;			/* 1st connection: input */
		in_cell = connection->input_cell;		/* connected cell    	*/
		connect_net = get_network_id(in_cell->net_id);	/* connected network 	*/
		network = get_network_id(cell->net_id);		/* self 		*/

		/* If inside_net cell != 0, out = 0  */
		if ( in_cell2->firing_rate > 0.0 ) {
			/* self is in outside */
			cell->firing_rate = 0.0;
			return(OK);
		} 
			

		diamet = atoi( arg1 );			/* diamet,radius are integer */
		radius = diamet / 2.0;
		
		range1_n = -radius;
		range1_p = radius;
		range2_n = -radius;
		range2_p = radius;
		distance = radius;
		sig = (double)radius;				/* sigma = radius 		*/
		if ( diamet == 5  ) scale = 2.56;
		if ( diamet == 7  ) scale = 5.71;
		if ( diamet == 11 ) scale = 16.0;	       	/* max ~ 1.00, min ~ 0.37 	*/
		if ( diamet == 13 ) scale = 23.0;
		if ( diamet == 17 ) scale = 41.0;
		if ( diamet == 25 ) scale = 90.0;
		if ( diamet == 33 ) scale = 162.0;
		if ( diamet == 41 ) scale = 256.0;
		if ( diamet == 61 ) scale = 558.0;
		
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* if not at edge */
			/* x,y low and high limit due to edge */
			x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
			x_high_limit = x_limit_high( connect_net, in_cell, range1_p );

			y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
			y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
			/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n",
						in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit); */
			cell_count = 0;
			sum = 0.0;
			for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
			for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
				if (((bias_x * bias_x) + (bias_y * bias_y)) <= (distance * distance)) {
					/* within distance */
					neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
					neighbor_cell2 = in_cell2 + (bias_y * connect_net->dim_x + bias_x);
					if ( (neighbor_cell2->firing_rate) == 0.0 ) {
						/* Inside */
						sum = sum + gaussian( bias_x,bias_y,sig,scale)*(neighbor_cell->firing_rate) ;
						cell_count++;
				}	}
			}}
			if ( cell_count != 0 ) {
				cell->voltage = sum;
				cell->firing_rate = connection->conductance * (sum / (double)cell_count);
			} else {
				cell->firing_rate = 0.0;
			}
		} else {
			/* all 4 peripheral edges */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}

/* regional averaging   Uniform averaging, NOT gaussian   April '97  	*/
/* If the cell is on edge, =0.  Area outside of the edge is ignored. 	*/
/* If value within the region is 0, then # of cell is also decremented. */	
/* take average of a circular region of diameter x of ONE networks 	*/

	if ( determine_pgn_function(pgn_func,"region_ave") ) {
	  	double act_scale, act_offset;

		get_argument(pgn_func,1,arg1);			/* diameter */
		get_argument(pgn_func,2,arg2);			/* write activity */
		get_argument(pgn_func,3,arg3);			/* scale to write */
		get_argument(pgn_func,4,arg4);			/* offset to write */
		
		connection = cell->connect_list;
		in_cell = connection->input_cell;		/* connected cell    	 */
		connect_net = get_network_id(in_cell->net_id);	/* connected network 	 */
		network = get_network_id(cell->net_id);		/* self */
			

		diamet = atoi( arg1 );			/* diamet,radius are integer */
	        radius = diamet / 2.0;
		
	        range1_n = -radius;
	        range1_p = radius;
	        range2_n = -radius;
	        range2_p = radius;
	        distance = radius;
	        
	        if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
		  /* if not at edge */
		  /* x,y low and high limit due to edge */
		        x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
		        x_high_limit = x_limit_high( connect_net, in_cell, range1_p );

		        y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
		        y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
			/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n",
			        in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit); */
		        cell_count = 0;
		        sum = 0.0;
		        for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
		        for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
			        if (((bias_x * bias_x) + (bias_y * bias_y)) <= (distance * distance)) {
				  /* within distance */
				        neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
				         if (neighbor_cell->firing_rate != 0.0 ) {
					   /* value is NON ZERO */
					        sum = sum + neighbor_cell->firing_rate;
					        cell_count++;
			        }	}
		        }}
		        if ( cell_count != 0 ) {
		        	cell->voltage = sum;
		        	cell->firing_rate = connection->conductance * (sum / (double)cell_count);
		        } else {
		        	cell->firing_rate = 0.0;
		        }
		  
	        } else {
		/* all 4 peripheral edges */
		        cell->voltage = 0.0;
		        cell->firing_rate = 0.0;
		  
	        }
		
	        if (arg2[0]=='w') {
		        act_scale = char_to_float(arg3);
		        act_offset = char_to_float(arg4);
		        value_0 = act_scale*(cell->firing_rate) + act_offset;
		        printf("                      %f \n", value_0);
	        }
	  
		return(OK);
	}

/* ave of TWO networks (old version) */
/* takes 17x17 only */
	if ( determine_pgn_function(pgn_func,"ave17x17cir2") ) {
		connection = cell->connect_list;
		in_cell = connection->input_cell;		/* connected cell    	 */
		connect_net = get_network_id(in_cell->net_id);	/* connected network 	 */
		
		connection2 = connection + 1;			/* the first connection   */
		in_cell2 = connection2->input_cell;
		connect_net2 = get_network_id(in_cell2->net_id);	

		network = get_network_id(cell->net_id);	/* self */
		/* fprintf(stderr," enter ave 2 : %s %s %s \n",pgn_func_three, pgn_func_range, pgn_func_type);		*/
		/* fprintf(stderr," cell id: %d   net name:%s %s\n",in_cell->id,connect_net->name, connect_net2->name);	*/
		
		/* if ( strcmp(pgn_func, "ave11x11cir2") == 0 ) {
			range1_n = RANGE1_N;
			range1_p = RANGE1_P;
			range2_n = RANGE2_N;
			range2_p = RANGE2_P;
			distance = DISTANCE5;
			sig = 5.0;
			scale = 10.0;
		} */
		if ( strcmp(pgn_func, "ave17x17cir2") == 0 ) {
			range1_n = RANGE3_N;
			range1_p = RANGE3_P;
			range2_n = RANGE4_N;
			range2_p = RANGE4_P;
			distance = DISTANCE8;
			sig = 8.0;			/* max=1.0, min coef = 0.366 */
			scale = 20.0;			
		}
		/* if ( strcmp(pgn_func, "ave25x25cir2") == 0 ) {
			range1_n = RANGE7_N;
			range1_p = RANGE7_P;
			range2_n = RANGE7_N;
			range2_p = RANGE7_P;
			distance = DISTANCE12;
			sig = 12.0;
			scale = 90.0;			
		}
		if ( strcmp(pgn_func, "ave33x33cir2") == 0 ) {
			range1_n = RANGE5_N;
			range1_p = RANGE5_P;
			range2_n = RANGE6_N;
			range2_p = RANGE6_P;
			distance = DISTANCE16;
			sig = 16.0;
			scale = 40.0;			
		} */
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* if not at edge */
			/* x,y low and high limit due to edge */
			x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
			x_high_limit = x_limit_high( connect_net, in_cell, range1_p );

			y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
			y_high_limit = y_limit_high( connect_net, in_cell, range2_p );

			cell_count = 0;
			sum = 0.0;
			for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
				for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
					if (((bias_x * bias_x) + (bias_y * bias_y)) <= (distance * distance)) {
						/* within distance */
						neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
						neighbor_cell2 = in_cell2 + (bias_y * connect_net2->dim_x + bias_x);
						gauss_coef = gaussian( bias_x,bias_y,sig,scale);
						sum = sum + gauss_coef*(neighbor_cell->firing_rate) +
							    gauss_coef*(neighbor_cell2->firing_rate);
						cell_count++;
					/* if ( (cell_count >= 10 )&&( cell_count < 20) ) */
					/* fprintf(stderr,"%lf %lf\n",neighbor_cell->firing_rate, neighbor_cell2->firing_rate); */
					}
				}
			}
			cell->voltage = sum;
			cell->firing_rate = connection->conductance * (sum / (double) cell_count);

		} else {
			/* all 4 edges */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}

/* dilation by distance OR auto expansion */
/* If the directly connected cell has the value > threshold, then get the value.					*/
/* If the value <= threshold, then look for inside of the diameter(arg[1]).   Get non-zero closest cell's firing rate 	*/

	if ( determine_pgn_function(pgn_func,"auto_expand") ) {
		get_argument(pgn_func,1,arg1);			/* search diameter */
		
		connection = cell->connect_list;
		in_cell = connection->input_cell;		/* connected cell    	 */
		connect_net = get_network_id(in_cell->net_id);	/* connected network 	 */
		network = get_network_id(cell->net_id);		/* self */

		diamet = atoi( arg1 );			/* diamet,radius are integer */
		radius = diamet / 2.0;
		
		range1_n = -radius;
		range1_p = radius;
		range2_n = -radius;
		range2_p = radius;
		distance = radius;			/* distance is integer */
		
		if (in_cell->firing_rate <= cell->threshold) {
			/* directly connected cell has low value */
			if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
				/* if not at edge */
				/* x,y low and high limit due to edge */
				x_low_limit = x_limit_low(connect_net, in_cell, range1_n);
				x_high_limit = x_limit_high(connect_net, in_cell, range1_p);

				y_low_limit = y_limit_low(connect_net, in_cell, range2_n);
				y_high_limit = y_limit_high(connect_net, in_cell, range2_p);
				/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n", in_cell->id, x_low_limit,
				                   x_high_limit, y_low_limit, y_high_limit); */

				cell_count = 0;
				for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
				for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
					if (fdistance=((bias_x * bias_x) + (bias_y * bias_y)) <= (double)(distance*distance)) {
						/* within distance */
						neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
						if (neighbor_cell->firing_rate != 0.0) {
							/* value is NON ZERO */
							neighbor_value[cell_count] = neighbor_cell->firing_rate;
							neighbor_dist[cell_count] = fdistance;
							cell_count++;
						}
					}
				}}

				/* find closest non-zero */
				min_dist = 1000.0;
				min_value = 0.0;
				for (i = 0; i < cell_count; i++) {
					if (neighbor_dist[i] < min_dist) {
						min_dist = neighbor_dist[i];
						min_value = neighbor_value[i];
					}
				}
				cell->voltage = min_dist;
				cell->firing_rate = connection->conductance * min_value;
				/* out is firing rate of the closest cell */
			} else {
				/* all 4 peripheral edges */
				cell->voltage = 0.0;
				cell->firing_rate = 0.0;
			}
		} else {
			/* directly connected cell has high value */
			cell->firing_rate = in_cell->firing_rate;
		}
		return(OK);
	}

/* Erosion */
/* If anycell in distance(arg[1],diameter) <= threshold, then out = 0.	*/

	if ( determine_pgn_function(pgn_func,"erosion") ) {
		get_argument(pgn_func,1,arg1);			/* diameter */
		
		connection = cell->connect_list;
		in_cell = connection->input_cell;		/* connected cell    	 */
		connect_net = get_network_id(in_cell->net_id);	/* connected network 	 */
		network = get_network_id(cell->net_id);		/* self */

		diamet = atoi( arg1 );				/* diamet,radius are integer */
		radius = diamet / 2.0;

		range1_n = -radius;
		range1_p = radius;
		range2_n = -radius;
		range2_p = radius;
		distance = radius;
		
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* if self not at edge */
			/* x,y low and high limit due to edge */
			x_low_limit = x_limit_low( connect_net, in_cell, range1_n );
			x_high_limit = x_limit_high( connect_net, in_cell, range1_p );

			y_low_limit  = y_limit_low( connect_net, in_cell, range2_n );
			y_high_limit = y_limit_high( connect_net, in_cell, range2_p );
			/* fprintf(stderr,"%d, xl %d, xh %d, yl %d, yd %d\n",
						in_cell->id, x_low_limit, x_high_limit, y_low_limit, y_high_limit); */

			cell_count = 0;
			sum = 0.0;
			for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
			for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
				if (((bias_x * bias_x) + (bias_y * bias_y)) <= (distance * distance)) {
					/* within distance */
					neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
					if ( neighbor_cell->firing_rate <= cell->threshold ) {
						cell->voltage = 0.0;
						cell->firing_rate = 0.0;
						return(OK);
					}
				}
			}}
			cell->voltage = in_cell->firing_rate;
			cell->firing_rate = connection->conductance * in_cell->firing_rate;

		} else {
			/* all 4 peripheral edges */
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
		}
		return(OK);
	}


/* Represent depth change in orientation, compliment to depth in magnitude  					*/
/* Take local maximum of 1x1 (assuming all > 0).   firing_rate: max. orientation(8),display in 4 colors.   	*/
/*						   voltage:     magnitude 					*/
/* Output value: Von	0 deg. 	20 (Sky Blue)		Voff 	180 deg.  25
		 Ron	45	30 (Green)		Roff	225	  35
		 Hon	90	50 (Yellow)		Hoff	270	  55  
		 Lon	135	60 (Pink)		Loff	315	  65
   The order of connections MUST follow above order.
*/
	if ( determine_pgn_function(pgn_func,"change_orient") ) {
		get_argument(pgn_func,1,arg1);			/* diameter */
		range = char_to_float(arg1) / 2.0;
		sq_distance = pow((char_to_float(arg1)/2.0),2.0);
		
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		
		max_net = number_connect;
		max_firing_rate = 0.0;
		
		
		/* a loop to change map */
		for ( i = 0; i < number_connect; i++ ) {
			connection = init_connection + i;				
			in_cell = connection->input_cell;
			connect_net = get_network_id(in_cell->net_id);	

			if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
				/* if self not at edge */
				/* x,y low and high limit due to edge */
				x_low_limit = x_limit_low( connect_net, in_cell, -range );
				x_high_limit = x_limit_high( connect_net, in_cell, range );

				y_low_limit  = y_limit_low( connect_net, in_cell, -range );
				y_high_limit = y_limit_high( connect_net, in_cell, range );

				cell_count = 0;
				sum = 0.0;
				for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
				for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
					if (((bias_x * bias_x) + (bias_y * bias_y)) <= sq_distance) {
						neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
						in_value = (connection->conductance)*(neighbor_cell->firing_rate);
						if ( in_value > max_firing_rate ) {
							max_firing_rate = in_value;
							max_net = number_connect - i - 1; /* i loops from the last connection */
					}	}
				}}
		}	}
		/* output is always >= 0 , because max_firing_rate never go below 0 */
		cell->voltage = max_firing_rate;	/* max. value */
		
		switch (max_net) {
			case 0 :  cell->firing_rate = 20;	/* Von  */
				    break;
			case 1 :  cell->firing_rate = 25;	/* Voff */
				    break;
			case 2 :  cell->firing_rate = 30;	/* Ron  */
				    break;
			case 3 :  cell->firing_rate = 35;	/* Roff */
				    break;
			case 4 :  cell->firing_rate = 50;	/* Hon  */
				    break;
			case 5 :  cell->firing_rate = 55;	/* Hoff */
				    break;
			case 6 :  cell->firing_rate = 60;	/* Lon  */
				    break;
			case 7 :  cell->firing_rate = 65;	/* Loff */
				    break;
			default:    cell->firing_rate = 0.0;    /* may be 8; all input was 0.0 */
				    break;
		}
		return(OK);
	}

/* Vector summation over space from two maps: angle(1) and magnitude(2) */
	if ( determine_pgn_function(pgn_func,"Vect_Spac_Ave") ) {
		double  x, y;
		
		get_argument(pgn_func,1,arg1);	/* diameter */
		range = char_to_float(arg1) / 2.0;
		sq_distance = pow((char_to_float(arg1)/2.0),2.0);
		
		number_connect = cell->number_connections;
		connection = cell->connect_list;		/* magnitude */
		connection2 = connection + 1;			/* angle     */
				
		in_cell = connection->input_cell;
		in_cell2 = connection2->input_cell;
		
		connect_net = get_network_id(in_cell->net_id);	
		connect_net2 = get_network_id(in_cell2->net_id);
		
		if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
			/* if self not at edge */
			/* x,y low and high limit due to edge */
			x_low_limit = x_limit_low( connect_net, in_cell, -range );
			x_high_limit = x_limit_high( connect_net, in_cell, range );

			y_low_limit  = y_limit_low( connect_net, in_cell, -range );
			y_high_limit = y_limit_high( connect_net, in_cell, range );

			cell_count = 0;
			sum = 0.0;  sum_2 = 0.0;
			for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
			for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
				if (((bias_x * bias_x) + (bias_y * bias_y)) <= sq_distance) {
					neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
					neighbor_cell2 = in_cell2 + (bias_y * connect_net2->dim_x + bias_x);
					in_value = (connection->conductance)*(neighbor_cell->firing_rate);
					in_value_2 = (connection2->conductance)*(neighbor_cell2->firing_rate);
					if (in_value_2 > cell->threshold) {
						/* If input angle < threshold(-900),			*/
						/* it's response was less than threshold(2.0). 		*/
						/* Thus, no count.  See pgn(2D_tune).			*/				
						sum += in_value * cos( (in_value_2/180.0)*PAI );  	/* x component */
						sum_2 += in_value * sin( (in_value_2/180.0)*PAI );	/* y */
						cell_count++;
					}	
				}
			}}
			if (cell_count > 0) {				/* if no input, output 0. */
				x = sum / (double)cell_count ;
				y = sum_2 / (double)cell_count ;
				cell->voltage = sqrt( pow(x,2.0) + pow(y,2.0) );
				cell->firing_rate = ( atan( y/x ) / PAI )*180.0;
				return(OK);
			}		
		}
		cell->voltage = 0.0;		/* at edge or no input, output -999. */
		cell->firing_rate = -999.0;
		return(OK);
	}
	
/* 2D tuning for orient. 97.2.10., modified 97.4.6. */
/* Output is either Max, or Vector Sum(Angel or Intensity). */
/* For Max, output value: 10 to 130 with incriments of 10. (upto 13 connections)
		 if no max OR max<threshold, firing_rate=0.
   The order of connections MUST follow above order. Also see below. 
*/
	if ( determine_pgn_function(pgn_func,"2D_tune") ) {
		double  x, y;
		double  in_angle, out_angle;
		double  out_magnitude;
		int	count;
		
		get_argument(pgn_func,1,arg1);	/* diameter */
		get_argument(pgn_func,2,arg2);	/* m: max(default), a: angle, i: magnitude, */
						/* d: display angle the same way as max */
		range = char_to_float(arg1) / 2.0;
		sq_distance = pow((char_to_float(arg1)/2.0),2.0);
		
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		
		max_net = -1;		/* if no max OR max<threshold, firing_rate=0. */
		max_firing_rate = 0.0;
		x = 0.0;  y = 0.0;	
		count = 0;
		
		/* a loop to change map */
		for ( i = 0; i < number_connect; i++ ) {
			connection = init_connection + i;				
			in_cell = connection->input_cell;
			connect_net = get_network_id(in_cell->net_id);	

			if (not_at_edge(in_cell->id, in_cell->net_id, connect_net->name) == TRUE) {
				/* if self not at edge */
				/* x,y low and high limit due to edge */
				x_low_limit = x_limit_low( connect_net, in_cell, -range );
				x_high_limit = x_limit_high( connect_net, in_cell, range );

				y_low_limit  = y_limit_low( connect_net, in_cell, -range );
				y_high_limit = y_limit_high( connect_net, in_cell, range );

				cell_count = 0;
				sum = 0.0;
				for (bias_y = y_low_limit; bias_y <= y_high_limit; bias_y++) {
				for (bias_x = x_low_limit; bias_x <= x_high_limit; bias_x++) {
					if (((bias_x * bias_x) + (bias_y * bias_y)) <= sq_distance) {
						neighbor_cell = in_cell + (bias_y * connect_net->dim_x + bias_x);
						in_value = (connection->conductance)*(neighbor_cell->firing_rate);
						/* for LOCAL MAX */
						if ( (in_value > max_firing_rate)&&(in_value >= cell->threshold) ) {
							max_firing_rate = in_value;
							max_net = number_connect - i - 1; /* i loops from the last connection */
						}
						/* for Vector Sum */
						if ( in_value >= cell->threshold ) {
							sum += in_value;
							cell_count++;
						}
					}
				}}
			
				/* for VECTOR sum */
				/* connection MUST be in the right order */
				if ( cell_count > 0 ) {
					in_angle = PAI * ( (double)(-90 + i*5) / 180.0);	/* MAY 97 */
					/* OLD: in_angle = PAI * ( (double)(-30 + i*5) / 180.0) ; 		*/
						/* OLD: connection(i) begins from -30 with increments of 5.	*/
						/* OLD: (in nx file, it begins from 60)				*/
						/* NEW(May97): begins from -90 with increments of 5.	*/
						/* NEW(May97): (in nx file, it begins from 90)		*/
					x += (sum/(double)cell_count) * cos(in_angle);
					y += (sum/(double)cell_count) * sin(in_angle);
						/* uniform averaging over the range */
					count++;
				} 
		}	}
		
		/* Vector sum of all maps */
		out_magnitude = sqrt( pow(x,2.0) + pow(y,2.0) );
		if (count == 0) { 			/* if no input, set angle to -999.		 */
			out_angle = -999.9;		/* This will be a signal for pgn(Vect_Spac_Ave). */
		} else if (x == 0.0) {
			out_angle = 90.0;		
		} else {
			out_angle = ( atan( y/x ) / PAI )*180.0;
		}

		if (arg2[0] == 'a') {
			cell->voltage = out_magnitude;
			cell->firing_rate = out_angle;
			return(OK);
		} else if (arg2[0] == 'i') {
			cell->voltage = out_angle;
			cell->firing_rate = out_magnitude;
			return(OK);
		} else if (arg2[0] == 'd') {
			/* OLD: display angle with the same color as max: 60deg -> 10, -30deg -> 190 */
			/* NEW(May97): max: 90deg -> 10, -90deg -> 370 */
			cell->voltage = out_angle;
			if (count != 0)
				/* OLD: cell->firing_rate = -2.0*out_angle + 130.0; */
				cell->firing_rate = -2.0*out_angle + 190.0;  /* May 97 */
			else 
				cell->firing_rate = -999.9;	/* if no input, set angle to 999. */
			return(OK);
		} else if (arg2[0] == 'm') {
			/* for data, max. */
			cell->voltage = max_firing_rate;
			cell->firing_rate = 10.0*(double)max_net + 10.0;
			return(OK);
			
			/* OLD color code is nolonger in use from 97.5.6 */
			/* New code:					 */
			/* connect # 0  1  2  3   6  9   12  15  18  21  24  27  30  33  36 */
			/* net ang. 90 85 80 75  60  45  30  15   0 -15 -30 -45 -60 -75 -90 */
			/* Firing R.10 20 30 40  70 100 130 160 190 220 250 280 310 340 370 */

		} else if (arg2[0] == 'g') {
			/* magnitude of max. */
			cell->voltage = 10.0*(double)max_net + 10.0;		
			cell->firing_rate = max_firing_rate;
			return(OK);
		} else {			
			/* default: max */
			cell->voltage = max_firing_rate;	
			cell->firing_rate = 10.0*(double)max_net + 10.0;
		}
		return(OK);
	}
		
/* surface    Determines component(x or y) of steepest slope. 								*/
/* Get firing rates from 0 to 315deg nets, computes x or y component of the needle.  The length will represent depth	*/
/* and the orientation represents the steepest direction.								*/

	if ( determine_pgn_function(pgn_func,"surface") ) {
		get_argument(pgn_func,1,arg1);			/* x or y   */
		
		number_connect = cell->number_connections;
		init_connection = cell->connect_list;
		
		/* a loop to change map */
		component = 0.0;
		for ( i = 0; i < number_connect; i++ ) {
			connection = init_connection + i;				
			in_cell = connection->input_cell;
			connect_net = get_network_id(in_cell->net_id);	

			in_value = (connection->conductance)*(in_cell->firing_rate);
			if ( in_value > cell->threshold ) {
				angle = 45.0*(number_connect - i - 1); 
				/* i loops from the last connection; i = 0 ~ 7 				*/
				/* Order of connection: Hon, Loff, Voff, Roff, Hoff, Lon, Von, Ron 	*/
				if ( arg1[0] == 'x' )
					component += in_value * cos( PAI*(angle/ 180) );
				if ( arg1[0] == 'y' )
					component += in_value * sin( PAI*(angle/ 180) );
			}
		}
		cell->voltage = 0.0;
		cell->firing_rate = component;
		return(OK);
	}

/* kernel_in 			*/
/* OLD, use kernel_in_two 	*/
/* show two dots, one in the center of 65x65, and the other anywhere with the increment of (x+8,y+8) */
/* For computing 2nd order kernel */
	if ( determine_pgn_function(pgn_func, "kernel_in") ) {
		connection = cell->connect_list;		
		in_cell = connection->input_cell;	
		connect_net = get_network_id(in_cell->net_id);
		
		/* if this cell is in turn, set to 100, otherwise set to zero.  */
		/* c_cycle = get_current_cycle();  does not work */
		if (cell->id == network->number_cells) {
			c_cycle++;
			printf(" cycle %d \n",c_cycle ); 
		
			/* For each cell 
			if ((cell->id) == c_cycle ) {	
				cell->voltage = 100.0;
				cell->firing_rate = cell->voltage;
			} else {
				cell->voltage = 0.0;
				cell->firing_rate = cell->voltage;
			} 
			*/
		
			cell_pos_x = (int)fmod( (float)((c_cycle -1)*8 + 1 ),64.0 ); /* 1, 9, ...,57 */
			cell_pos_y = (int)(( (float)(c_cycle -1)*8.0 )/64.0)*8 + 1;
			get_cell_at_position(network->name,cell_pos_x,cell_pos_y)->firing_rate = 100.0;
		
		/*	(( get_cell_at_position(network->name,0,0) )+ (c_cycle - 1)*8)->voltage = 100.0;
			(( get_cell_at_position(network->name,0,0) )+ (c_cycle - 1)*8)->firing_rate = 100.0;
		*/
			/* the center (of 65x65, #2113) is always 100. */
			(( get_cell_at_position(network->name,0,0) )+ 2112)->voltage = 100.0;
			(( get_cell_at_position(network->name,0,0) )+ 2112)->firing_rate = 100.0;
		} else {
			cell->voltage = 0.0;
			cell->firing_rate = cell->voltage;
		}		
		return(OK);
	}

/* kernel_in_1 			*/
/* OLD, use kernel_in_one 	*/
/* CANNOT USE kernel_in AND kernel_in_1 at the SAME time. 	*/
	if ( determine_pgn_function(pgn_func, "kernel_in_1") ) {
		connection = cell->connect_list;		
		in_cell = connection->input_cell;	
		connect_net = get_network_id(in_cell->net_id);
		
		/* if this cell is in turn, set to 100, otherwise set to zero.  */
		/* c_cycle = get_current_cycle();  does not work */
		if (cell->id == network->number_cells) {
			c_cycle++;
			printf(" cycle %d \n",c_cycle ); 
		
			cell_pos_x = (int)fmod( (float)((c_cycle -1)*8 + 1 ),64.0 );   /* 1, 9, ...,57 */
			cell_pos_y = (int)(( (float)(c_cycle -1)*8.0 )/64.0)*8 + 1;
			get_cell_at_position(network->name,cell_pos_x,cell_pos_y)->firing_rate = 100.0;
		
			/* the center (of 65x65, #2113) is always 100.
			(( get_cell_at_position(network->name,0,0) )+ 2112)->voltage = 100.0;
			(( get_cell_at_position(network->name,0,0) )+ 2112)->firing_rate = 100.0;
			 */
		} else {
			cell->voltage = 0.0;
			cell->firing_rate = cell->voltage;
		}		
		return(OK);
	}

/* kernel_in_one 							*/
/* CANNOT USE kernel_in AND kernel_in_1 at the SAME time. 	*/
	if ( determine_pgn_function(pgn_func, "kernel_in_one") ) {
		double dot_value, interval;
		int   center_x, center_y, fixed_point;

		if (cell->id == network->number_cells) {
			/* Not in use for this pgn function, better non-exist for no connection.
			connection = (cell->connect_list);
			in_cell = connection->input_cell;
			connect_net = get_network_id(in_cell->net_id);
			*/

			get_argument(pgn_func,1,arg1);
			if ( arg1[0]=='p' ) dot_value = 100.0;
			if ( arg1[0]=='n' ) dot_value = -100.0;

			get_argument(pgn_func,2,arg2);
			interval = char_to_float( arg2 );	/* 8 for 8 points in 64x64  */
							/* 4 for 16 points in 64x64 */
			/* if this cell is in turn, set to 100, otherwise set to zero.  */
			/* c_cycle = get_current_cycle();  does not work */
			
			c_cycle++;
			printf(" cycle %d \n",c_cycle ); 
		
			cell_pos_x = (int)fmod( (float)((c_cycle -1)*interval ),(float)((network->dim_x) - 1) );    
									/* for 64x64 net, denomi=64,  0,8,...,56 */		
			cell_pos_y = (int)(( (float)(c_cycle -1)*interval )/(float)((network->dim_x) - 1) )*interval ;
			get_cell_at_position(network->name,cell_pos_x,cell_pos_y)->firing_rate = dot_value;
		
		} else {	
			cell->voltage = 0.0;
			cell->firing_rate = cell->voltage;
		}		
		return(OK);
	}

/* kernel_in_two */
/* For computing 2nd order kernel 	*/
/* Show two dots, one at the center (assigned in arg4 & arg5), and the other anywhere with the increment */
/* of (x+arg3,y+arg3).  Interval(arg3) MUST satisfy: net size = (multiple of interval) + 1		 */
/*			Center(arg4,arg5) MUST satisfy: multiple of interval				 */

	if ( determine_pgn_function(pgn_func, "kernel_in_two") ) {
		double dot_c_value, dot_2_value, interval;
		int   center_x, center_y, fixed_point;

		if (cell->id == network->number_cells) {
			/* Not in use in this pgn, better non-exist for no connection.
			connection = cell->connect_list;		
			in_cell = connection->input_cell;	
			connect_net = get_network_id(in_cell->net_id);
			*/

			get_argument(pgn_func,1,arg1);			/* center dot contrast */
			if ( arg1[0]=='p' ) dot_c_value = 100.0;
			if ( arg1[0]=='n' ) dot_c_value = -100.0;
			if ( (arg1[0]=='p')&&(arg1[1]=='m') ) dot_c_value = 200.0;
			if ( (arg1[0]=='n')&&(arg1[1]=='m') ) dot_c_value = -200.0;
			if ( (arg1[0]=='p')&&(arg1[1]=='l') ) dot_c_value = 50.0;
			if ( (arg1[0]=='n')&&(arg1[1]=='l') ) dot_c_value = -50.0;
			if ( (arg1[0]=='p')&&(arg1[1]=='s') ) dot_c_value = 25.0;
			if ( (arg1[0]=='n')&&(arg1[1]=='s') ) dot_c_value = -25.0;

			get_argument(pgn_func,2,arg2);			/* the 2nd dot */
			if ( arg2[0]=='p' ) dot_2_value = 100.0;
			if ( arg2[0]=='n' ) dot_2_value = -100.0;
			if ( (arg2[0]=='p')&&(arg2[1]=='m') ) dot_2_value = 200.0;
			if ( (arg2[0]=='n')&&(arg2[1]=='m') ) dot_2_value = -200.0;
			if ( (arg2[0]=='p')&&(arg2[1]=='l') ) dot_2_value = 50.0;
			if ( (arg2[0]=='n')&&(arg2[1]=='l') ) dot_2_value = -50.0;
			if ( (arg2[0]=='p')&&(arg2[1]=='s') ) dot_2_value = 25.0;
			if ( (arg2[0]=='n')&&(arg2[1]=='s') ) dot_2_value = -25.0;
			
			get_argument(pgn_func,3,arg3);
			interval = char_to_float( arg3 );	/* 8 for 8 points in 64x64  */
							/* 4 for 16 points in 64x64 */

			/* default fixed point is the center of  65x65 */
			get_argument(pgn_func,4,arg4);
			get_argument(pgn_func,5,arg5);
			if ( arg4 == NULL )		/* Doesn't work, it gets arg3 */
				center_x = (network->dim_x) - 1; 	/* 32 for 64x64 */
			else
				center_x = atoi( arg4 );
			if ( arg5 == NULL )
				center_y = (network->dim_y) - 1;
			else
				center_y = atoi( arg5 );
			

			/* if this cell is in turn, set to 100 or -100, otherwise set to zero.  */
			/* c_cycle = get_current_cycle();  does not work */
			
			c_cycle++;
			printf(" cycle %d \n",c_cycle ); 
		
			/* For each cell 
			if ((cell->id) == c_cycle ) {	
				cell->voltage = 100.0;
				cell->firing_rate = cell->voltage;
			} else {
				cell->voltage = 0.0;
				cell->firing_rate = cell->voltage;
			} 
			*/
		
			cell_pos_x = (int)fmod( (float)((c_cycle -1)*interval ),(float)((network->dim_x) - 1) );   
									/* for 64x64 net, denomi=64,  0,8,...,56 */
			cell_pos_y = (int)(( (float)(c_cycle -1)*interval )/(float)((network->dim_x) - 1) )*interval ;
			get_cell_at_position(network->name,cell_pos_x,cell_pos_y)->firing_rate = dot_2_value;
		
		/*	(( get_cell_at_position(network->name,0,0) )+ (c_cycle - 1)*8)->voltage = 100.0;
			(( get_cell_at_position(network->name,0,0) )+ (c_cycle - 1)*8)->firing_rate = 100.0;
		*/
			/* Fixed point given by arg4 & arg5, if not given, the default is	*/	
			/* the center (32,32) (of 65x65, #2113) . 				*/
			/* The center of the net must be even number to match!!! */
			fixed_point = (network->dim_x)*center_y + center_x ;    /* Not +1 */
			(( get_cell_at_position(network->name,0,0) )+ fixed_point)->voltage = dot_c_value;
			(( get_cell_at_position(network->name,0,0) )+ fixed_point)->firing_rate = dot_c_value;
		} else {
			cell->voltage = 0.0;
			cell->firing_rate = cell->voltage;
		}		
		return(OK);
	}


/* kernel_out */
/* computing kernel, arg1=interval(2, 4 or 8), arg2=potition of connected net(2112 for 64x64, 0 for 1x1 net) */
/* take the center cell in the connected map, and store at the position according to cycle */

	if ( determine_pgn_function(pgn_func, "kernel_out") ) {
		float interval;
		int   center_x, center_y, fixed_point;

		if (cell->id == 1) {
			connection = cell->connect_list;		
			in_cell = connection->input_cell;	
			connect_net = get_network_id(in_cell->net_id);

			get_argument(pgn_func,1,arg1);
			interval = char_to_float( arg1 );	/* 8 for 8 points in 64x64  MUST match with kernel_in* */
							/* 4 for 16 points in 64x64 */
			get_argument(pgn_func,2,arg2);
			get_argument(pgn_func,3,arg3);
			center_x = atoi( arg2 );
			center_y = atoi( arg3 );
			fixed_point = (network->dim_x)*center_y + center_x ;	 	/* Not +1 */

			cell_pos_x = (int)fmod( (float)((c_cycle -1)*interval ),(float)((network->dim_x)-1) );  
			cell_pos_y = (int)(( (float)(c_cycle -1)*interval )/(float)((network->dim_x)-1))*(int)interval ;
			get_cell_at_position(network->name,cell_pos_x,cell_pos_y)->firing_rate =
				(get_cell_at_position(connect_net->name,0,0) + fixed_point)->firing_rate;
			/* always get from the center */
			get_cell_at_position(network->name,cell_pos_x,cell_pos_y)->voltage = 0.0;				
		}
		return(OK);
	}
	
/* 1D_plot :  get a single point data from connected nets, and put in 1D array. */
	if ( determine_pgn_function(pgn_func,"1D_plot") ) {
		
		/* cell at the end of the net. */
		get_argument(pgn_func,1,arg1);		/* arg[1]&[2] x,y position to aquire */
		get_argument(pgn_func,2,arg2);
		bias_x = atoi( arg1 );
		bias_y = atoi( arg2 );	
	
		if ( cell->id > cell->number_connections ) {
			cell->voltage = 0.0;
			cell->firing_rate = 0.0;
			return(OK);
		}
		init_connection = cell->connect_list;
		connection = init_connection + (cell->id) - 1;   /* id begins from 1 */				
		in_cell = connection->input_cell;
		connect_net = get_network_id(in_cell->net_id);	

		neighbor_cell = (connect_net->cells) + (bias_y * connect_net->dim_x + bias_x);
		cell->firing_rate = network->scale*(connection->conductance)*(neighbor_cell->firing_rate);
		cell->voltage = network->scale*(connection->conductance)*(neighbor_cell->voltage);
		
		return(OK);
	}
		
/* Voltage dependent channels */
/* Computes sum = arg1*(i1 * i2 * ...)+arg2(i1 + i2 + i3 + ...) 	*/
/* If sum < arg3(min), then out=min.  If sum > arg4(max), then out=max.	*/

	if ( determine_pgn_function(pgn_func,"volt_depd") ) {
		get_argument(pgn_func,1,arg1);		
		get_argument(pgn_func,2,arg2);
		get_argument(pgn_func,3,arg3);		
		get_argument(pgn_func,4,arg4);		
		value_1 = char_to_float(arg1);	value_2 = char_to_float(arg2);
		min = char_to_float(arg3);	max = char_to_float(arg4);
		sum_1 = 1.0;	sum_2 = 0.0;
				
		init_connection = cell->connect_list;
		for (i = 0; i <(cell->number_connections); i++ ) {
			connection = init_connection + i;
			in_cell = connection->input_cell;
			if ( (in_cell->firing_rate) >= 0.0 ) {
				/* ignore if < 0 */
				sum_1 = sum_1 * (in_cell->firing_rate);
				sum_2 = sum_2 + (in_cell->firing_rate);
		}	}
		
		cell->firing_rate =  sum_1*value_1 + sum_2*value_2 ;
		cell->voltage = sum_2;

		if ( max < cell->firing_rate ) {
			cell->voltage = cell->firing_rate;
			cell->firing_rate = max;
		} else if ( min > cell->firing_rate ) {
			cell->voltage = cell->firing_rate;
			cell->firing_rate = min;			
		}
		return(OK);
	}	


/* Voltage dependent channels */
/* Computes sum = arg1*(i1 * i2 * ...)+arg2(i1 + i2 + i3 + ...) 	*/
/* If sum < threshold, then out=arg3(min).  If sum >= threshold, then out=arg4(max).	*/

	if ( determine_pgn_function(pgn_func,"volt_depd_step") ) {
		get_argument(pgn_func,1,arg1);		
		get_argument(pgn_func,2,arg2);
		get_argument(pgn_func,3,arg3);		
		get_argument(pgn_func,4,arg4);
		get_argument(pgn_func,5,arg5);
		value_1 = char_to_float(arg1);	value_2 = char_to_float(arg2);
		min = char_to_float(arg3);	max = char_to_float(arg4);
		sum_1 = 1.0;	sum_2 = 0.0;
				
		init_connection = cell->connect_list;
		for (i = 0; i <(cell->number_connections); i++ ) {
			connection = init_connection + i;
			in_cell = connection->input_cell;
			if ( (in_cell->firing_rate) >= 0.0 ) {
				/* ignore if < 0 */
				sum_1 = sum_1 * (in_cell->firing_rate);
				sum_2 = sum_2 + (in_cell->firing_rate);
		}	}
		
		cell->firing_rate = sum_1*value_1 + sum_2*value_2 ;

		if ( (cell->threshold) <= cell->firing_rate ) {
			cell->voltage = cell->firing_rate;
			cell->firing_rate = max;
		} else {
			cell->voltage = cell->firing_rate;
			cell->firing_rate = min;			
		}
		return(OK);
	}	

/* Phase coupled oscilation model */
/* Determine coupling weights depending on distance, colinearity, and difference in angle.	*/
/* If this cell's max_orient=0 (cell->voltage), skip this cell.					*/
/* If considering cell's max_orient=0 (in_cell->voltage), skip this connection.			*/
/* If (orient_connect - max_orient)<thrshd_diff_ori(45deg), skip this connection.		*/
/* If location > min_rads(=RF length) i.e. outside the radius, skip this connection.		*/
/* Compute Gaussian coef. depending on distance, colinearity, and relative angle.		*/
/* */
/* ONE connection for taking orientation at the FIRST CYCLE only.  The orientation is stored in	*/
/* voltage which never changes.	<- ? 00.08.02							*/
/* Phase is stored in cell->firing_rate.  Use firing_rate_old to compute dp/dt.			*/
/* For the FIRST CELL, "firing_rate" is copied to "firing_rate_old".				*/

	if ( determine_pgn_function(pgn_func,"phase_couple") ) {
		double 	thrshd_diff_ori, min_rads;
		double	sigma_dist, sigma_colin, sigma_angl;
		double	scl_dist, scl_colin, scl_angl;		/* scale for Gaussian, coef=1 for x=mean */
		double  dp_dt;
		double  omega=0;	/* angular velocity = 2*PI 	*/
		double	max_step=22.5;	/* PI/8				*/
		double	coef_dist, coef_colin, coef_angl, coef, sum_coef;
		double	diff_orient, diff_coef, thrshd_couple;
		int	count_connect, fix_range=0, center;
		int	cell_x, cell_y, in_cell_x, in_cell_y;
		
		get_argument(pgn_func,1,arg1);		/* max differece in orientation  */
		get_argument(pgn_func,2,arg2);          /* min radius */
		get_argument(pgn_func,3,arg3);	        /* sigma for distance    */	
		get_argument(pgn_func,4,arg4);          /* sigma for colinearity */
		get_argument(pgn_func,5,arg5);          /* sigma for angle       */
		get_argument(pgn_func,6,arg6);          /* scale for distance    */
		get_argument(pgn_func,7,arg7);          /* scale for colinearity */
		get_argument(pgn_func,8,arg8);          /* scale for angle       */
		get_argument(pgn_func,9,arg9);          /* omega                 */
		get_argument(pgn_func,10,arg10);        /* max step              */
		get_argument(pgn_func,11,arg11);	/* f or none             */
		get_argument(pgn_func,12,arg12);	/* range from edges to fix phase */
		get_argument(pgn_func,13,arg13);	/* width of stimulus bar */
		
		thrshd_diff_ori = char_to_float(arg1);	min_rads = char_to_float(arg2);
		sigma_dist = char_to_float(arg3);	sigma_colin = char_to_float(arg4);
		sigma_angl = char_to_float(arg5);
		scl_dist = char_to_float(arg6);		scl_colin = char_to_float(arg7);
		scl_angl =  char_to_float(arg8);
		omega = char_to_float(arg9);		max_step = char_to_float(arg10);		
		if ( arg11[0] == 'f' ) {
			fix_range = atoi(arg12);
			half_width = ( atoi(arg13)/2 )+1;	/* half_width of stimulus (vertical bar) */	
		}

		/* If the FIRST CELL, move "firing_rate" of all cells to "firing_rate_old".	*/
		/* Do nothing other than this.							*/
		if ( cell->id == 1 ) {
			cell->firing_rate = -10.0;
			for ( i=0; i<(network->number_cells); i++ ) {
				in_cell = (network->cells) + i;
				in_cell->firing_rate_old = in_cell->firing_rate;
			}
			c_cycle++;
			fprintf(stderr,"cycle: %d\n",c_cycle);
			return(OK);
		}

		/* Since voltage is automatically updated before pgn, update explicitly.	*/
		/* Should orientation code (color code) has been converted to real angle (deg).	*/
		connection = cell->connect_list;
		in_cell = connection->input_cell;
		if ( (in_cell->firing_rate) > (cell->threshold) ) {	/* ignore if orient<= -370 	*/
			cell->voltage = in_cell->firing_rate;		/* -90< voltage <90 		*/
		} else {
			cell->voltage = -370.0;	
		}		
		
		/* From the second cell, compute coupling coef. for this cell.				*/
		/* 1. Skip if THIS CELL has no orientation, i.e. this orient has less magnitude.	*/
		/* 2. Skip if connected cell has no orientation, i.e. this orient has less magnitude.	*/
		/* 3. Skip if relative orientation between this cell and in_cell > thrshd_diff_ori.	*/
		/* 4. Skip if violate min. radius */
		
		if ( (cell->voltage) <= (cell->threshold) ) {
			cell->firing_rate = -10.0;		/* 1. Skip if no orientation 			*/
			return(OK);				/* This cell is not fired from the begining. 	*/
		}
		
		/* If at the EDGES, and arg11=f, FIX the phase to 0 or 180 within fix_range(arg12) */
		/* vertical line = 0-phase, otherwise 180 */
		cell_pos_x = cell_position_x(cell->id,network->dim_x,network->dim_y);
		cell_pos_y = cell_position_y(cell->id,network->dim_x,network->dim_y);
		center = (network->dim_x)/2 ;
		
		if ( (cell_pos_y < fix_range)||(cell_pos_y >= ((network->dim_y)-fix_range)) ) {
			/* top or bottom, within fix_range+1 (cell_pos_x starts from 0) */
		     	if ( (cell_pos_x > (center - half_width))&&(cell_pos_x <= (center + half_width)) ) {
				/* vertical line, 0-phase */
				cell->firing_rate = 0.0;
			} else {
				/* otherwise, 180-phase */
				cell->firing_rate = 180.0;
			}
			return(OK);
		}
		if ( (cell_pos_x < fix_range)||(cell_pos_x >= ((network->dim_x)-fix_range)) ) {
			/* leftest or rightest, phase=180 */
			cell->firing_rate = 180.0;
			return(OK);
		}
		
				
		count_connect=0;
		sum_coef = 0.0;
		for (i=0; i<(network->number_cells); i++ ) {	/* look for all cells 	*/
			in_cell = (network->cells) + i;
			if ( (cell->id) != (in_cell->id) ) {	/* Skip if it's own. 	*/
			if ( (in_cell->voltage) > (cell->threshold) ){	
								/* 2. Skip if no orientation 	*/
									/*   -90 < voltage <= 90 	*/
			diff_orient = fabs((in_cell->voltage) - (cell->voltage));
							/* ok for ++ & +-;     0 < diff_orient <=180 	*/
			if ( diff_orient > 90.0 ) 		   	
				diff_orient = 180.0 - diff_orient; 	/* set 0 <= diff_orient <= 90	*/
				/* diff_orient -= 90; 	OLD, revised 990316 */
			if ( diff_orient < thrshd_diff_ori ) {
								/* 3. Skip if relative ori > thrshd(45deg) 	*/
				cell_x = cell_position_x(cell->id,network->dim_x,network->dim_y);
				cell_y = cell_position_y(cell->id,network->dim_x,network->dim_y);
				in_cell_x = cell_position_x(in_cell->id,network->dim_x,network->dim_y);
				in_cell_y = cell_position_y(in_cell->id,network->dim_x,network->dim_y);
				
				if ( min_radius_test(cell_x, cell_y, cell->voltage, in_cell_x, in_cell_y, min_rads) ) {
								/* 4. Skip if violate min. radius */
				/* Computes coupling coef. */
				coef_dist = gauss_dist(cell_x, cell_y, in_cell_x, in_cell_y, sigma_dist, scl_dist);
				coef_colin = gauss_colin(cell_x, cell_y, cell->voltage, in_cell_x, in_cell_y, in_cell->voltage, sigma_colin, scl_colin);
				coef_angl = gauss_angl(diff_orient, sigma_angl, scl_angl);
				coef = coef_dist * coef_colin * coef_angl;	
				/* fprintf(stderr,"cell= %d,%d   connect_cell= %d,%d\n",cell_x,cell_y,in_cell_x,in_cell_y); */
					 
				/* Computes phase */
				sum_coef += coef*sin( (((in_cell->firing_rate_old)*PI)/180.0)-(((cell->firing_rate_old)*PI)/180.0) );
				count_connect++;
			}}}	}
		}
		if ( count_connect == 0 ) {		
			return(OK);	/* This cell is fired, but no other cell to couple. */
		} else {
			dp_dt = omega + max_step*(sum_coef);	/* omega=2PI=0, max_step=PI/16=11.25		    */
								/* Do not devided by count_connect. Exp(sum_coef)=1 */			
			cell->firing_rate = (cell->firing_rate_old) + dp_dt;
			if ( (cell->firing_rate) >= 360.0 ) {				/* 0 <= Phase < 2PI */
				cell->firing_rate = (cell->firing_rate)-360.0;
			} else if( (cell->firing_rate) < 0.0 ) {
				cell->firing_rate = (cell->firing_rate)+360.0;
			}
			return(OK);
		}	
	}	

/* Display phase within 0-180deg:  0 <= phase <= 180 	*/
/* Input is limited to 0-360     			*/
	if ( determine_pgn_function(pgn_func,"phase_display") ) {
	
		connection = cell->connect_list;
		in_cell = connection->input_cell;
		if ( ((in_cell->firing_rate) > 180.0)&&((in_cell->firing_rate) <= 360.0) ) { 	
			cell->firing_rate = 360.0 - (in_cell->firing_rate);
		} else {
			cell->firing_rate = in_cell->firing_rate;
		}
		return(OK);
	}

/* Converts orientation in colour code to real angle, if magnitude>threshold. */
/* May be used with phase_couple. */
	if ( determine_pgn_function(pgn_func,"convert_ori") ) {

		double	slant, offset;

		get_argument(pgn_func,1,arg1);		
		get_argument(pgn_func,2,arg2);	
		slant = char_to_float(arg1);
		offset = char_to_float(arg2);

		init_connection = cell->connect_list;	/* magnitude, the second connection.  */
		connection = init_connection + 1;	/* orientation, the first connection. */
		
		in_cell = init_connection->input_cell;	/* magnitude */
		in_cell2 = connection->input_cell;	/* orientation */
		
		if ( ( (in_cell->firing_rate) >= (cell->threshold) )&&
		     ( (in_cell2->firing_rate)>=10.0 )&&( (in_cell2->firing_rate)<=370.0 )  ) { /* -90 < orient(deg) < 90 */
				cell->firing_rate = slant*(in_cell2->firing_rate) + offset; 	/* convert to real orient */
		} else {
			/* if magnitude < threshold (300) */
			cell->firing_rate = -370.0;
		}
		return(OK);	
	}
	
/* */
	
/* no such a pgn function */
	if (cell->id <= 2 ) 
		fprintf( stderr," ERROR: No such pgn function <%s>.\n", pgn_func );
	return(OK);
	/* Changed from ERROR to OK.  2002/06/17 KS */
}




pgn_transform_outputs(cell, pgn_func)
     CELL            cell;
     char           *pgn_func;
{
 /*scale & offset valid by transform_outputs() in netwrok_functions.c */
 	return(OK);
}
 

CELL get_neighboor(input_cell, net_head2, bias_x, bias_y)  
     CELL            input_cell;
     NETWORK         net_head2;
     int             bias_x;
     int	     bias_y;
{
	int bias;
	
	bias = bias_y*(net_head2->dim_x) + bias_x;	
	return(input_cell + bias);
}

int  x_limit_low( connect_net, in_cell, range_xlow ) 
	NETWORK	connect_net;
	CELL	in_cell;
	int	range_xlow;
{
	int 	bias_x, bias;
	CELL	neighbor_cell;
	int 	not_at_edge();
	int	x_low_limit;
	
	x_low_limit = range_xlow;
	for (bias_x = (range_xlow + 1); bias_x <= 0; bias_x++) {
		bias = bias_x;
		if ((in_cell->id + bias) > 0) {
			neighbor_cell = in_cell + bias;
			if (not_at_edge(neighbor_cell->id, in_cell->net_id, connect_net->name) == FALSE) {
				/* at edge */
				x_low_limit = bias_x;
			}
		}
	}
	return( x_low_limit );
}

int  x_limit_high( connect_net, in_cell, range_xhigh ) 
	NETWORK	connect_net;
	CELL	in_cell;
	int	range_xhigh;
{
	int 	bias_x, bias;
	CELL	neighbor_cell;
	int 	not_at_edge();
	int	x_high_limit;
	
	x_high_limit = range_xhigh;
	for (bias_x = (range_xhigh - 1); bias_x >= 0; bias_x--) {
		bias = bias_x;
		if ((in_cell->id + bias) <= connect_net->number_cells) {
			neighbor_cell = in_cell + bias;
			if (not_at_edge(neighbor_cell->id, in_cell->net_id, connect_net->name) == FALSE) {
				/* at edge */
				x_high_limit = bias_x;
			}
		}
	}
	return( x_high_limit );
}

int  y_limit_low( connect_net, in_cell, range_ylow ) 
	NETWORK	connect_net;
	CELL	in_cell;
	int	range_ylow;
{
	int 	bias_y, bias;
	CELL	neighbor_cell;
	int 	not_at_edge();
	int	y_low_limit;

	y_low_limit = range_ylow;
	for (bias_y = (range_ylow + 1); bias_y <= 0; bias_y++) {
		bias = bias_y * (connect_net->dim_x);
		if ((in_cell->id + bias) > 0) {
			neighbor_cell = in_cell + bias;
			if (not_at_edge(neighbor_cell->id, in_cell->net_id, connect_net->name) == FALSE) {
				/* at edge */
				y_low_limit = bias_y;
			}
		}
	}
	return( y_low_limit );
}

int  y_limit_high( connect_net, in_cell, range_yhigh ) 
	NETWORK	connect_net;
	CELL	in_cell;
	int	range_yhigh;
{
	int 	bias_y, bias;
	CELL	neighbor_cell;
	int 	not_at_edge();
	int	y_high_limit;

	y_high_limit = range_yhigh;
	for (bias_y = (range_yhigh - 1); bias_y >= 0; bias_y--) {
		bias = bias_y * (connect_net->dim_x);
		if ((in_cell->id + bias) <= connect_net->number_cells) {
			neighbor_cell = in_cell + bias;
			if (not_at_edge(neighbor_cell->id, in_cell->net_id, connect_net->name) == FALSE) {
				/* at edge */
				y_high_limit = bias_y;
			}
		}
	}
	return( y_high_limit );
}

/* 2D circular symmetric gaussian (NOT DOG), also in mask.c*/
double gaussian(dx,dy,sig,scale) 
	int  	dx, dy;
	double 	sig, scale;
{
	double 	p1, p3;
	
	/* 2D Gaussian filter */
	p1 = 1.0 / sig * exp(-1.0 * pow((double) (dx), 2.0) / (2.0 * pow(sig, 2.0)));
	p3 = 1.0 / sig * exp(-1.0 * pow((double) (dy), 2.0) / (2.0 * pow(sig, 2.0)));

	return ( (1.0 / (2.0 * PI)) * (scale * p1 * p3) );	/* no sqrt */
}

/* 2D DOG, original in mask.c 
double dog2(x, y, sig_e_x, sig_e_y, sig_i_x, sig_i_y, scale_e_x, scale_e_y, scale_i_x, scale_i_y)
	double 	x, y;
	double	sig_e_x, sig_e_y, sig_i_x, sig_i_y;
	double 	scale_e_x, scale_e_y, scale_i_x, scale_i_y;
{
   	double	value;
   	
   	py1 = (1.0 / sig_e_y) * exp(-1.0 * pow((y + pos_e_y),2.0) / (2.0 * pow(sig_e_y,2.0))); 	
    	py2 = (1.0 / sig_i_y) * exp(-1.0 * pow((y + pos_i_y),2.0) / (2.0 * pow(sig_i_y,2.0))); 	
    			
    	px1 = (1.0 / sig_e_x) * exp(-1.0 * pow((x + pos_e_x),2.0) / (2.0 * pow(sig_e_x,2.0)));	
    	px2 = (1.0 / sig_i_x) * exp(-1.0 * pow((x + pos_i_x),2.0) / (2.0 * pow(sig_i_x,2.0)));	
    			
 	value = ((1.0 / 2.0 * PI) * (scale_e_x * px1 * scale_e_y * py1 - scale_i_x * px2 * scale_i_y * py2));
	return(value);
}
*/
/* 1D DOG , also in dog.c, mask.c
double dog(fx,sig_e,sig_i,scale_e,scale_i)
	double 	fx;
	double	sig_e, sig_i;
	double	scale_e, scale_i;
{
	double 	value;
   	p1 = (1.0 / sig_e) * exp(-1.0 * pow((fx + pos_e),2.0) / (2.0 * pow(sig_e,2.0)));
    	p2 = (1.0 / sig_i) * exp(-1.0 * pow((fx + pos_i),2.0) / (2.0 * pow(sig_i,2.0)));
    
    	value = (1.0 / sqrt(2.0 * PI)) * (scale_e * p1 - scale_i * p2);
	return(value);
}
*/

/* read in coeficients of masks from files.  file names, size, scale and wave length are given by the file of arg[1] 	*/
/* returning values are stored in externs.										*/

int maskset_read( filename )
	char   *filename;
{
	extern	double	mask_coef2[][MAX_MASK_COEF];
	extern	double	mask_scale[];
	extern	double	mask_wl[];
	extern  int	mask_length[];
	extern  int	mask_width[];
	extern  int	num_masks;
	FILE		*fp;
	int		i,j,k;
	int		check;
	char 		mask_file[NUM_MASKS][MAX_ARG_LEN];
	
	
	if ((fp = fopen(filename, "r")) == NULL) {
		fprintf(stderr, " file <%s> open error. \n", filename);
		fclose(fp);
		return (ERROR);
	}
	i = 0;
	while ((check = fscanf(fp, "%s %d %d %lf %lf",
			       mask_file[i],	/* name of mask file; 2D char array, a pointer to the first row */
			       &mask_length[i],	/* length	 */
			       &mask_width[i],	/* width	 */
			       &mask_scale[i],	/* scale	 */
			       &mask_wl[i])) == 5) {	/* wavelength	 */
		printf("    %s\t%d %d\t%lf %lf\n",
		       mask_file[i], mask_length[i], mask_width[i], mask_scale[i], mask_wl[i]);
		i++;
	}
	num_masks = i;
	fprintf(stderr, " <%d> mask file(s) assigned in <%s>.\n", num_masks, filename);
	if (num_masks > NUM_MASKS) {
		fprintf(stderr, "WARNING: Too many masks!\n");
		/* issue message but do nothing */
	}
	fclose(fp);

	/* read mask files */
	for (j = 0; j < num_masks; j++) {
		if ((fp = fopen(mask_file[j], "r")) == NULL) {
			fprintf(stderr, " file <%s> open error.\n", mask_file[j]);
			fclose(fp);
			return (ERROR);
		}
		/* fprintf(stderr,"mask file <%s> open.\n",mask_file[j]); */
		for (i = 0; i < (mask_length[j] * mask_width[j]); i++) {
			if ((check = fscanf(fp, "%lf", &mask_coef2[j][i])) != 1) {
				fprintf(stderr, " file <%s> read error.\n", mask_file[j]);
				fclose(fp);
				return (ERROR);
			}
			/* printf(" %lf",mask_coef2[j][i]); */
		}
		/* fprintf(stderr," mask file <%s> loaded.\n",mask_file[j]); */
		fclose(fp);
	}
	return (OK);
}


/* convolution by mask 			*/
/* a mask is teken from the array. 	*/
/* return convoluted value. 		*/

double  convolution(cell, in_cell_head, length, width, mask_coef, edge)
	CELL	cell, in_cell_head;
	int	length, width;
	double	mask_coef[];		/* pointer to the first value of the array, length of array is NUM_MASKS */
	double  edge;			/* clamp value for edges	*/
{
	NETWORK	connect_net, network;
	CELL	get_neighboor();
	CELL	neighbor_cell;
	
	int	not_at_edge();
	int	half_length, half_width;
	int	x_low, x_high, y_low, y_high;
	int 	flag_edge;
	int	bias_x, bias_y;
	int	number_element;
	
	double	value2[MAX_MASK_WD][MAX_MASK_WD];			/* upto AxA mask */
	double	value_m = 0.0;

		/* printf("in convolution(): mask_coef[1,2,3] = %lf %lf %lf\n",mask_coef[0],mask_coef[1],mask_coef[2]);	*/
		connect_net = get_network_id( in_cell_head->net_id );	/* connected network 	*/
		network = get_network_id( cell->net_id );		/* self */

		half_length = length / 2;
		half_width = width / 2;
		x_low = -1*half_length; x_high = half_length; 
		y_low = -1*half_width; y_high = half_width; 
		
		
		flag_edge = OFF;
		/* if ( not_at_edge(cell->id, cell->net_id, network->name) == TRUE ) {	*//* check self */
			/*  not at edge */
			for ( bias_x = x_low; bias_x <= x_high; bias_x++ ) {
			for ( bias_y = y_low; bias_y <= y_high; bias_y++ ) {
				/* get values from connected cells */
				neighbor_cell = get_neighboor( in_cell_head, connect_net, bias_x, bias_y );
				if ( not_at_edge(neighbor_cell->id, in_cell_head->net_id, connect_net->name) == FALSE ) {
					/*  at edge */
					flag_edge = ON;
					break;
				} else {
					/* not at edge, store connected cell's firing rates in value2[][] */
					value2[bias_x + x_high][bias_y + y_high] = neighbor_cell->firing_rate;	
					/* printf("index_x = %d, index__y = %d    value2 = %lf\n"
					   ,bias_x,bias_y,value2[bias_x + x_high][bias_y + y_high]); */
				}
			}}
			if ( flag_edge == OFF ) {	/* compute convolution */
			  	value_m = 0; 
			  	number_element = 0;
				for ( bias_y = 0; bias_y < width; bias_y++ ) {
				for ( bias_x = 0; bias_x < length; bias_x++ ) {
					value_m += value2[bias_x][bias_y] * mask_coef[number_element];
					/* printf(" x %d, y %d, value_in %lf, coef %lf\n",
					   bias_x,bias_y,value2[bias_x][bias_y],mask_coef[number_element]); */
					number_element++;
				}}
			}
		/* } else { 
			* at edge *
			flag_edge = ON; 
		} */
		if ( flag_edge == ON ) {
			/* if a edge in anywhere, store the assigned value. */
			value_m = edge;
		} 
		return(value_m);
}

/* convolution by mask: outside net is set to zero, and continue computation. Modified from convolution() 06/20/2002 KS  */
/* a mask is teken from the array. 	*/
/* return convoluted value. 		*/

double  convolution_z(cell, in_cell_head, length, width, mask_coef, edge)
	CELL	cell, in_cell_head;
	int	length, width;
	double	mask_coef[];		/* pointer to the first value of the array, length of array is NUM_MASKS */
	double  edge;			/* clamp value for edges	*/
{
	NETWORK	connect_net, network;
	CELL	get_neighboor();
	CELL	neighbor_cell;
	int	not_at_edge();

	int     con_net_dim_x, con_net_dim_y;
	int	half_length, half_width;
	int	x_low, x_high, y_low, y_high;
	int     number_outside;
	int	bias_x, bias_y;
	int     center_cell_x, center_cell_y;
	int	number_element;
	int     flag_outside;
	
	double	value2[MAX_MASK_WD][MAX_MASK_WD];			/* upto AxA mask */
	double	value_m = 0.0;

	/* printf("in convolution(): mask_coef[1,2,3] = %lf %lf %lf\n",mask_coef[0],mask_coef[1],mask_coef[2]);	*/
		connect_net = get_network_id( in_cell_head->net_id );	/* connected network 	*/
		network = get_network_id( cell->net_id );		/* self */

		half_length = length / 2;
		half_width = width / 2;
		x_low = -1*half_length; x_high = half_length; 
		y_low = -1*half_width; y_high = half_width; 
		
		center_cell_x =  ((in_cell_head->id) - 1) % (connect_net->dim_x) ;
		center_cell_y =  ((in_cell_head->id) - 1) / (connect_net->dim_x) ;
		
	    if ( (in_cell_head->id) == 50 ) 
	      		printf(" center_cell id,x,y %d %d %d x,y_low %d %d \n",in_cell_head->id,center_cell_x, center_cell_y, x_low, y_low) ; 

		/* check self, to avoid meaningless for loop */
		/* if ( not_at_edge(cell->id, cell->net_id, network->name) == TRUE ) {	*/
			/*  not at edge */
			for ( bias_x = x_low; bias_x <= x_high; bias_x++ ) {
			for ( bias_y = y_low; bias_y <= y_high; bias_y++ ) {
			        /* i(=bias_x + x_high) begins from zero. */
			        /* determine whether outside */
			      	if ( ((center_cell_x + bias_x) < 0) || ((center_cell_x + bias_x) >= (connect_net->dim_x)) ||
				     ((center_cell_y + bias_y) < 0) || ((center_cell_y + bias_y) >= (connect_net->dim_y)) ) {
				       /*  outside, store zero in value2[i][j]. */
				      value2[bias_x + x_high][bias_y + y_high] = 0.0;

				} else {
				      /* inside */
				      neighbor_cell = get_neighboor( in_cell_head, connect_net, bias_x, bias_y );
				      value2[bias_x + x_high][bias_y + y_high] = neighbor_cell->firing_rate;	
	
				      /*  if ( (in_cell_head->id) ==   50 ) {            
				          printf("  index_x,y  %d %d, cell id = %d,   value2 = %lf\n" 
					  ,bias_x,bias_y,neighbor_cell->id,value2[bias_x + x_high][bias_y + y_high]);
					}  */
			
				}
			}}

			/* compute convolution, simply ignores outside the net. */
			value_m = 0;  number_element = 0; 
			for ( bias_y = 0; bias_y < width; bias_y++ ) {
			for ( bias_x = 0; bias_x < length; bias_x++ ) {
				value_m += value2[bias_x][bias_y] * mask_coef[number_element];
				number_element++;
	                }}
		    
		/* } else { 
			* at edge *
			flag_edge = ON; 
		} */
				
		return(value_m);
}

/* distribution by a mask 								*/
/* Distribute conductance. The distribution of the conductance is taken from the array 	*/

        int distribute(cell, mask_scale, length, width, mask_coef)
	CELL	cell;
	double	mask_scale;
	int	length, width;
	double	mask_coef[];
{
	NETWORK         network;
	CELL            get_neighboor();
	CELL            neighbor_cell;

	int             not_at_edge();
	int             half_length, half_width;
	int             x_low, x_high, y_low, y_high;
	int             bias_x, bias_y;
	int             i;

	network = get_network_id(cell->net_id);		/* self */

	half_length = length / 2; 	half_width = width / 2;
	x_low = -1 * half_length;	x_high = half_length;
	y_low = -1 * half_width;	y_high = half_width;

	/* check edge in the region */
	for (bias_y = y_low; bias_y <= y_high; bias_y++) {
	for (bias_x = x_low; bias_x <= x_high; bias_x++) {
		neighbor_cell = get_neighboor(cell, network, bias_x, bias_y);
		if (not_at_edge(neighbor_cell->id, cell->net_id, network->name) == FALSE) {
			/* if edge anywhere, do nothing */
			return (OK);
		}
	}}
	
	/* no edge anywhere, distribute values */
	i = 0;
	for (bias_y = y_low; bias_y <= y_high; bias_y++) {
	for (bias_x = x_low; bias_x <= x_high; bias_x++) {
		neighbor_cell = get_neighboor(cell, network, bias_x, bias_y);
		neighbor_cell->firing_rate += mask_scale * mask_coef[i];
		i++;
	}}
	return (OK);
}

/* Following 5 functions are USED in phase_couple */
/* Used in phase_couple */
/* Test whether the cell and in_cell are outside circle.  	*/
/* Center1: X + min_rad * cos(orient)				*/
/*	    Y - min_rad * sin(orient); if a<0, sin(a)<0 	*/
/* Center2: X - min_rad * cos(orient)				*/
/*	    Y + min_rad * sin(orient); if a<0, sin(a)<0 	*/

 int min_radius_test(x, y, orient, in_x, in_y, min_rads)
	int	x, y;
	double	orient;		/* -90 < orient <= 90 */
	int	in_x, in_y;
	double	min_rads;
{
	double	center1_x, center1_y, center2_x, center2_y;
	double	d1, d2;
	
	center1_x = (double)x + min_rads*cos((orient*PI)/180.0);
	center1_y = (double)y - min_rads*sin((orient*PI)/180.0);
	center2_x = (double)x - min_rads*cos((orient*PI)/180.0);
	center2_y = (double)y + min_rads*sin((orient*PI)/180.0);
	
	if (( sqrt( pow(((double)in_x - center1_x),2.0) + pow(((double)in_y - center1_y),2.0) ) >= min_rads )&&
	    ( sqrt( pow(((double)in_x - center2_x),2.0) + pow(((double)in_y - center2_y),2.0) ) >= min_rads ) ) {
		return(1);
	} else {
		return(0);
	}
}

/* Used in phase_couple */
/* Determines coupling coeficient by distance. */
double gauss_dist(cell_x, cell_y, in_cell_x, in_cell_y, sigma_dist, scale)

	int	cell_x, cell_y, in_cell_x, in_cell_y;
	double	sigma_dist, scale;
{
	double	gauss_one_dim();
	double  distance, coef, dx, dy;
	double  mean = 0.0;
	
	dx = in_cell_x - cell_x;  	dy = in_cell_y - cell_y;
	distance = sqrt( pow(dx,2.0) + pow(dy,2.0) );
	coef = gauss_one_dim(mean, sigma_dist, scale, distance);
	return(coef);
}

/* Used in phase_couple */
/* Determines coupling coeficient by co-circularity. 	*/
/* Vertical = zero deg, right for + 			*/
double gauss_colin(cell_x, cell_y, cell_orient, in_cell_x, in_cell_y, in_cell_orient, sigma_colin, scale)

	int	cell_x, cell_y;
	double	cell_orient; 			/* -90 < cell_orient <= 90 */
	int	in_cell_x, in_cell_y;
	double	in_cell_orient, sigma_colin, scale;
{
	double	gauss_one_dim();
	double	ideal_angle, alpha, beta;
	double	coef, diff_orient;
	
	if (cell_x == in_cell_x) {
		alpha = 0.0;
	} else if (cell_y == in_cell_y) {
		alpha = PI/2.0;
	} else {
		alpha = atan( (double)(in_cell_x - cell_x)/(double)(in_cell_y - cell_y) );	
	}					/* -PI/2 < alpha=atan() <= PI/2;	*/
						/* zero-deg=vertical, clockwise for +	*/
	ideal_angle = (2.0*alpha*180.0)/PI - cell_orient; 	
						/*     -270 < ideal_angle <= 270 	*/
	while ( ideal_angle > 90.0 )		/* set  -90 < ideal_angle <= 90 	*/
		ideal_angle -= 180.0;		/* revised 990316 */
	while ( ideal_angle <= -90.0 )
		ideal_angle += 180.0;
	
	diff_orient = fabs(ideal_angle - in_cell_orient);  	/* 0 < diff_orient <= 180 */
	if ( diff_orient > 90.0 ) 
		diff_orient = 180.0 - diff_orient;	/* set  0 < diff_orient <= 90 */

	coef = gauss_one_dim(0.0,sigma_colin,scale,diff_orient);
		    /*     mean ,sigma_dist ,scale, x       */
	return(coef);
}

/* Used in phase_couple */
/* Determines coupling coeficient by relative angle. */
double gauss_angl(diff_orient, sigma_angl, scale)

	double	diff_orient;	/* 0 < diff_orient <= PI/2	*/
	double	sigma_angl, scale;
{
	double	gauss_one_dim();
	double	coef; 
		
	coef = gauss_one_dim(0.0,sigma_angl,scale,diff_orient);
		        /*  mean,sigma_angle,scale,distance	*/
	return(coef);
}

/* Used in phase_couple */
/* Computes gaussian coef as a function of distance. */
/* scale = 50.0: coef=1, if sigma=20 */
/* 	   75.2: coef=1, if sigma=30 */
/* 	   80.2: coef=1, if sigma=32 */
double gauss_one_dim(mean, sigma, scale, x)

	double mean, sigma, scale, x;
{
	double coef;
	
	coef = ( 1.0/(sqrt(2.0*PI)*sigma) )*exp(-0.5 * pow( ((x - mean)/sigma),2.0 ));
	return(scale*coef);
}



































































