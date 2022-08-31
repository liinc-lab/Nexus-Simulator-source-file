/**************************************************************
 *                                                            *
 *                            NEXUS                           *
 *                                                            *
 *              (c) 1990 Paul Sajda and Leif Finkel           *
 *                                                            *
 *                       (nexus_run.c)                        *
 *                                                            *
 *      modified for Parallel Nexus.  10/28/91 by K.Sakai     *
 **************************************************************/

#include "nexus.h"
#include "run.h"
#include "bp.h"
#include "inversion.h"
#include "rbf.h"
#include "extern_functions.h"
#include "graphics.h"
#include "functions.h"
#include "hebb.h"
#include "batch.h"
#include "main.h"

static int		count_cycles;



int
get_current_cycle( )
{
  return count_cycles;
}



/*
 *  RECURSIVE ROUTINE:  swap_firing_rate().
 */

void
swap_firing_rate( network )
    register NETWORK	network;
{
  register int		count;
  register CELL		cell;

  /*
   *  swap_cells() removed, replaced with in-line code.  DW 94.08.17
   */

  if (network) {
    cell = network->cells;

    for (count = 0; count < network->number_cells; count++, cell++)
      if (cell->clamp == CLAMP_OFF)
	cell->firing_rate_old = cell->firing_rate;

    swap_firing_rate( network->next );
  }
}



/*
 *  Clean up after ourselves.
 *  ifdefs; DW 94.10.29;  routines moved to their respective files, DW 94.11.06
 */

void
close_error_files( )
{
  /*  extern FILE *cycle_plot_fp; */


#ifdef NEXUS_BP
  (void) closeErrorFileBP( );
#endif /* NEXUS_BP */

#ifdef NEXUS_INVERSION
  (void) closeErrorFileInv( );
#endif /* NEXUS_INVERSION */

#ifdef NEXUS_RBF
  (void) closeErrorFileRBF( );
#endif /* NEXUS_RBF */

  if (activity_output_fp)
    fclose( activity_output_fp );

/*  if (cycle_plot_fp)
    fclose( cycle_plot_fp ); */
}  /* close_error_files() */



/***********************************************************************
 *  Simulation cycle routines
 ***********************************************************************/

/**
 **  Do one simulation cycle, random updating.
 **/

int
do_random_cycle( whichCycle, cells_per_cycle )
    int		cells_per_cycle;
    int		whichCycle;
{
  register int		count_cells;
  register int		cell_id, count_num_eval;
  static NETWORK 	search_head;
  static CELL		cell;
  /* clamp status of network: CLAMP_ON, CLAMP_OFF, or CLAMP_SELECTIVE_ON */
  static int		flag_network_clamped;


  /* default option for extern_from read */
  char			push_opt[FUNCTION_ARG_SIZE];
  char			rand_default_opt[FUNCTION_ARG_SIZE];

  sprintf( rand_default_opt, "p10" );
  search_head = network_head;


  for (count_cells = 0; count_cells < cells_per_cycle; count_cells++) {

    if (check_interrupt( ) == NXI_STOP) {
      close_error_files( );
      return ERROR;
    }

    search_head = *(network_xref_list +
		    (int) (get_random( 1.0, (float) number_networks ) + 0.5));

    /*
     * first cell determines clamp status
     */
    flag_network_clamped = search_head->cells->clamp;

    /* 
     * In random updates,
     *
     *  If this is a DUMMY network and is not clamped,
     *    update its date from external network file and redisplay.
     *
     *  If this is NOT a DUMMY network, update evaluations_per_cycle
     *    number of random cells.
     */
    if ((search_head->extern_connections)->extern_from == TRUE) {

      if (flag_network_clamped != CLAMP_ON) {

	((search_head->extern_connections)->extern_from_times)++;	

	/*
	 * for random updating, use external option "p10" (use old data
	 * if new data hasn't arrived yet)
	 */
	sscanf( (search_head->extern_connections)->extern_from_opt,
	       "%s", push_opt );
	sprintf( (search_head->extern_connections)->extern_from_opt,
		"%s", rand_default_opt );

	if( (extern_get_file( search_head )) == ERROR ) {
	  /* couldn't read file according to the option */
	  printf( "Can't update dummy network %s.\n", search_head->name );
	  close_error_files( );
	  return ERROR;
	}
	sprintf( (search_head->extern_connections)->extern_from_opt,
		"%s", push_opt );

	/*
	 *  Redisplay entire network if requested.
	 *    Don't bother distinguishing cell-by-cell since we've already
	 *    updated the ENTIRE network at once.  DW 94.07.06
	 */
	if (display_type != DISP_OFF)
	  graph_network( search_head );

      } /* if (flag_network_clamped != CLAMP_ON) */

    } /* if DUMMY network */

    else {	/* This is NOT a DUMMY network. */

      /* do not allow 0 or negative  # of evals per cycle */
      if (search_head->evaluations_per_cycle > 0) {

	/* if net is not clamped, update cells & display */
	if (flag_network_clamped != CLAMP_ON) {

	  /*  DW 95.04.05  Take out Hebb learning from random updating.  */
	  /*    Just doesn't make sense!  */
	  /*	    || query_flag( LEARN_HEBB )) { */
	  /* each cell must be evaluated anyway if hebb is on, as */
	  /*   even a clamped cell may have a plastic connection. */

	  for (count_num_eval = 0;
	       count_num_eval < search_head->evaluations_per_cycle;
	       count_num_eval++) {

	    cell_id =
	      (int) (get_random( 1.0, (float) search_head->number_cells)
		     + 0.5);

	    cell = search_head->cells + cell_id - 1;

	    /* formerly update_cell_activity_rand(); 7/8/94 J.B. */
	    /* '!= CLAMP_SELECTIVE_ON' -> '== CLAMP_OFF'; DW 94.07.13 */
	    /* skip update & display if cell is clamped; 7/15/94 J.B. */

	    if (cell->clamp == CLAMP_OFF) {
	      update_cell_activity( cell, search_head->class,
				   search_head, OFF );

	      if (display_type == DISP_CELL)
		display_cell_activity( search_head, cell_id );
	    }

#if 0
#ifdef NEXUS_HEBB
	    /*  Taken out, DW 95.04.05, see above deletion of HEBB code.  */
	    if (query_flag( LEARN_HEBB ))
	      do_hebb_rule( cell, OFF );
#endif /* NEXUS_HEBB */
#endif /* 0 */

	  }  /* for (evaluations_per_cycle) */

	}  /* if (flag_network_clamped != CLAMP_ON) */

      }  /* if (search_head->evaluations_per_cycle > 0) { */
      else
	fprintf( stderr,
		"NEXUS: # evals per cycle must be > 0 (network %s).\n",
		search_head->name );
    }  /* else { (NOT a DUMMY network.) */

    if (((search_head->extern_connections)->extern_to == TRUE)) {
      /* the net has external to connection(s) */
      ((search_head->extern_connections)->extern_to_times)++;
      extern_put_file( search_head );
    }

  } /* for (count_cells) */

  if (display_type == DISP_NET)
    graph_activity( );

  printf("Finished cycle # %d\n", (count_cycles + 1));
  return OK;
}  /* do_random_cycle() */



/**
 **  Do one simulation cycle, sequential updating.
 **/

int
do_sequential_cycle( whichCycle, cycles )
    int		cycles;
    int		whichCycle;
{
  int			cell_id, count_num_eval;

  /*  Used to monitor cells' stability during a given cycle. */
  float			old_cell_activity;
  int			flag_network_stable;

  static NETWORK 	search_head;
  static CELL		cell;
  /* clamp status of network: CLAMP_ON, CLAMP_OFF, or CLAMP_SELECTIVE_ON */
  static int		flag_network_clamped;




  search_head = network_head;

  /* repeat for each network */
  while (search_head != NULL) {

    /* get clamp status */
    flag_network_clamped = search_head->cells->clamp;

    /*
     *  If this is a DUMMY network and is not clamped,
     *    update and redisplay.
     *  DW 94.07.12 - added graph_network() test.
     *  DW 94.07.25 - eliminated display_cell() loop.
     *    Since we're not updating cell by cell, but rather loading
     *    an entire network in at once, it makes sense to just display
     *    the entire network at once too.
     *    '== DISP_NET' -> '!= DISP_OFF' to accommodate this.
     */
    if ((search_head->extern_connections)->extern_from == TRUE) {

      if (flag_network_clamped != CLAMP_ON) {
	/* skip extern files if clamped */
	((search_head->extern_connections)->extern_from_times)++;	
	printf(" Updating network %s\n", search_head->name);

	if ((extern_get_file(search_head)) == ERROR) {
	  /* couldn't read file according to the option */
	  printf( "Can't update dummy network %s.\n", search_head->name );
	  close_error_files( );
	  return ERROR;
	}

	if (display_type != DISP_OFF || whichCycle == FIRST_CYCLE)
	  graph_network( search_head );
      }  /* if (flag_network_clamped != CLAMP_ON) { */

      else {

	if (whichCycle == FIRST_CYCLE)
	  graph_network( search_head );
      }
    }  /* if DUMMY network */



    /*
     *  Since this is NOT a DUMMY network, evaluate it and redisplay.
     */

    else {

      /*
       *  Do not allow 0 or negative  # of evals per cycle for
       *    SEQUENTIAL either.  Bug fix: DW 94.08.01
       */
      if (search_head->evaluations_per_cycle > 0) {

	if (flag_network_clamped != CLAMP_ON) {

	  for (count_num_eval = 0;
	       count_num_eval < search_head->evaluations_per_cycle;
	       count_num_eval++) {

	    flag_network_stable = ON;
	    printf("  Updating network %s\n", search_head->name);



	    /*
	     *  Main CELL loop.
	     */

	    for (cell_id = 1;
		 cell_id <= search_head->number_cells;
		 cell_id++) {

	      if (check_interrupt( ) == NXI_STOP) {
		close_error_files( );
		return ERROR;
	      }

	      cell = search_head->cells + cell_id - 1;
	      old_cell_activity = cell->firing_rate;

	      /* "!= CLAMP_SELECTIVE_ON" -> "== CLAMP_OFF"; DW 94.07.13 */
	      if (cell->clamp == CLAMP_OFF) {

		/* formerly update_cell_activity_seq(); 7/11/94 J.B. */
		update_cell_activity( cell,
				     search_head->class,
				     search_head,
				     query_flag( ACTIVITY_SWAP ) );

		if (cell->firing_rate != old_cell_activity) {
		  flag_network_stable = OFF;
		  if (display_type == DISP_CELL)
		    display_cell_activity( search_head, cell_id );
		}
		else if (whichCycle == FIRST_CYCLE &&
			 display_type == DISP_CELL)
		  display_cell_activity( search_head, cell_id );

	      }  /* if (cell->clamp == CLAMP_OFF) { */
	    }  /* for (number_cells) */

	    if ((display_type == DISP_NET &&
		 (whichCycle == FIRST_CYCLE || flag_network_stable == OFF)) ||
		(display_type == DISP_OFF && whichCycle == LAST_CYCLE)) {

		graph_network( search_head );

	      }

	  }  /* for (evaluations_per_cycle) */

	}  /* if (flag_network_clamped != CLAMP_ON) */

	else if (display_type != OFF)
	  if (query_flag( LEARN_BATCH ) || whichCycle == FIRST_CYCLE)
	    graph_network( search_head );

      }  /* if (evaluations_per_cycle > 0) { */

      else
	fprintf( stderr,
		"NEXUS: # evals per cycle must be > 0 (network %s).\n",
		search_head->name );

      /*
       *  DW 94.10.01, took out negative evals per cycle code.
       */

    }  /* else NOT a DUMMY network. */


    if ((search_head->extern_connections)->extern_to == TRUE) {
      /* the net has external to connection(s) */
      ((search_head->extern_connections)->extern_to_times)++;
      extern_put_file(search_head);
    }

    search_head = search_head->next;

  }	/* finished cycling through every network */



  search_head = network_head;



  /*
   *  Call individual learning modules and check their return values.
   *    Check added DW 94.08.19 (run_* routines now return a status flag)
   *  Call if save_error is enabled, even if learning is off.
   *    JB BP:94.06.09 / INV,RBF: 94.06.11
   *  ifdefs; DW 94.10.29
   *  Output activity: DW 94.11.10
   */


#ifdef NEXUS_HEBB
  if (query_flag( LEARN_HEBB ))
    if (run_hebb( ) == ERROR)
      return ERROR;
#endif /* NEXUS_HEBB */

#ifdef NEXUS_BP
  if (query_flag( LEARN_BP ) || query_flag( SAVE_ERR_BP ))
    if (run_bp( count_cycles, cycles ) == ERROR) {
      close_error_files( );
      return ERROR;
    }
#endif /* NEXUS_BP */

#ifdef NEXUS_INVERSION
  if (query_flag( LEARN_INVERS ) || query_flag( SAVE_ERR_INVERS ))
    if (run_inversion( count_cycles, cycles ) == ERROR) {
      close_error_files( );
      return ERROR;
    }
#endif /* NEXUS_INVERSION */

#ifdef NEXUS_RBF
  if (query_flag( LEARN_RBF ) || query_flag( SAVE_ERR_RBF ))
    if (run_rbf( count_cycles, cycles ) == ERROR) {
      close_error_files( );
      return ERROR;
    }
#endif /* NEXUS_RBF */



  if (query_flag( ACTIVITY_SWAP )) {
    printf("Swapping firing rates\n");
    swap_firing_rate( network_head );
  }


  printf( "Finished cycle # %d\n", (count_cycles + 1));
  return OK;
}  /* do_sequential_cycle() */










/*
 *  Main loop to update simulation state.
 *
 *  Extensively commented on and evaluated.  DW 94.07.07, JB ??.
 *  Clamping checks added 5/23/94, revised 7/15/94 J.B, fixed DW 94.08.25
 */

int
run_simulation( cycles, cells_per_cycle, type )
    int		cycles, cells_per_cycle, type;
{
  /*  learn_flag is obsolete; removed.  DW 94.08.25 */


  /******************************************
   *  Pre-simulation:  DW 94.11.10
   ******************************************/

  if (query_flag( ACTIVITY_CYCLE ))
    if (!(activity_output_fp = fopen( activity_output_filename, "w" ))) {
      printf( "Can not open output file '%s'.\n", activity_output_filename );
      printf( "Turning off activity cycle record.\n" );
      set_flag( ACTIVITY_CYCLE, OFF );
    }

  if (query_flag( ACTIVITY_LOAD ))
    if (init_activity_cycle( ) == ERROR)
      return ERROR;


  /******************************************
   *  Simulation cycles:  DW 94.10.09- split up into three sections.
   ******************************************/

  count_cycles = 0;

  /*
   *  After first cycle, update display so we have a baseline view of the
   *    networks, unless DISPLAY_TYPE is DISP_OFF.  DW 94.10.09
   *  Also, force display update on last cycle.  DW 94.09.09
   */

  if (cycles > 1) {

    if (query_flag( ACTIVITY_LOAD ))
      if (load_activity_cycle( ) == ERROR)
	return ERROR;

    if (type == SEQUENTIAL) {
      if (do_sequential_cycle( FIRST_CYCLE, cycles ) == ERROR)
	return ERROR;
    }
    else {
      if (do_random_cycle( FIRST_CYCLE, cells_per_cycle ) == ERROR)
	return ERROR;
    }

    /*  if (query_flag( CYCLE_PLOT ))
	cycle_plot_network( count_cycles, cycles ); */

    if (query_flag( ACTIVITY_CYCLE ))
      output_activity( activity_output_fp );

    count_cycles = 1;
  }


  if (cycles > 2) {

    for (count_cycles = 1; count_cycles < cycles - 1; count_cycles++) {

      if (query_flag( ACTIVITY_LOAD ))
	if (load_activity_cycle( ) == ERROR)
	  return ERROR;

      if (type == SEQUENTIAL) {
	if (do_sequential_cycle( MIDDLE_CYCLE, cycles ) == ERROR)
	  return ERROR;
      }
      else {
	if (do_random_cycle( MIDDLE_CYCLE, cells_per_cycle ) == ERROR)
	  return ERROR;
      }

      /*  if (query_flag( CYCLE_PLOT ))
	  cycle_plot_network( count_cycles, cycles ); */

      if (query_flag( ACTIVITY_CYCLE ))
	output_activity( activity_output_fp );
    }
  }


  if (cycles > 0) {

    if (query_flag( ACTIVITY_LOAD ))
      if (load_activity_cycle( ) == ERROR)
	return ERROR;

    if (type == SEQUENTIAL) {
      if (do_sequential_cycle( (cycles == 1 ? FIRST_CYCLE : LAST_CYCLE),
			      cycles ) == ERROR)
	return ERROR;
    }
    else {
      if (do_random_cycle( (cycles == 1 ? FIRST_CYCLE : LAST_CYCLE),
			  cells_per_cycle ) == ERROR)
	return ERROR;
    }

    /*  if (query_flag( CYCLE_PLOT ))
	cycle_plot_network( count_cycles, cycles ); */

    if (query_flag( ACTIVITY_CYCLE ))
      output_activity( activity_output_fp );

    count_cycles++;
  }



  /******************************************
   *  Post-simulation
   ******************************************/

  if (activity_output_fp)
    fclose( activity_output_fp );

  return OK;

}  /* run_simulation() */



#if 0
void
swap_cells( number, head )
    int number;
    CELL head;
{
  register int count;


  for (count = 0; count < number; count++) {
    if (head->clamp == CLAMP_OFF)
      head->firing_rate_old = head->firing_rate;
    head++;
  }
}
#endif /* 0 */


/*
 *  These routines are obsolete.  See nexus_functions.c.  DW 94.07.13
 */

#if 0
int
update_cell_activity_seq( network, cell, function, decay, swap_flag )
    NETWORK		network;
    CELL            cell;
    char           *function;
    float           decay;
    int swap_flag;
{
  CONNECTION      connection;
  float           get_firing_rate();
  int             count;
  
  if (cell->clamp == CLAMP_OFF)
    update_cell_activity( cell, function, network, swap_flag );
}

int
update_cell_activity_rand( network, cell, function, decay )
    NETWORK		network;
    CELL            cell;
    char           *function;
    float           decay;
    
{
  CONNECTION      connection;
  float           get_firing_rate();
  int             count;
  
  if (cell->clamp == CLAMP_OFF)
    update_cell_activity( cell, function, network, OFF );
}
#endif /* 0 */
		









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
