#include "nexus.h"
#include "nexus_tk.h"
#include "batch.h"

#include "tcl_def.h"

#include "graphics.h"

#ifdef NEXUS_RBF
#include "rbf.h"
#endif

#ifdef NEXUS_BP
#include "bp.h"
#endif

#ifdef NEXUS_INVERSION
#include "inversion.h"
#endif


IndexValueStruct indexValues[] = {
    /****************************/
    /* Simulate dialog		*/
    /****************************/
    {"simulate.updating", 2, {"sequential","random"},    &simulate_type, 0},
    {"simulate.realtime", 3, {"off", "cell", "network"}, &display_type, 0},

    /****************************/
    /* Set Parameters		*/
    /****************************/
    {".setparam.setnet", 9, {"clamped", "unclamped", "activity", "transfunc", 
			     "threshold", "scale", "offset", "updates",
			     "decay"}, &param_choice, 0},

    /****************************************************/
    /* electrodes                                       */
    /****************************************************/
    {"view.electrode",     2,  {"off", "on"}, &view_elect,     0},
    {"connect.electrode",  2,  {"off", "on"}, &connect_elect,  0},
    {"activity.electrode", 2,  {"off", "on"}, &activity_elect, 0},
    {"activity.precis",  3, {"1", "3", "6"}, 	&activity_output_precision, 0},
    {"activity.colour",  2, {"0", "1"}, &activity_display_depth, 0},
    {"conduct.colour", 2,   {"0", "1"}, &conduct_display_depth, 0},

#ifdef NEXUS_HEBB
    /****************************/
    /* Hebb			*/
    /****************************/
    {".hebb.scale",  4, {"1", "2", "3", "4", }, &hebbFactorChoice, 0},
#endif

    {"script.explore",	2,	{"off", "on"},	&explore_variables, 0},

    /****************************/
    /* end			*/
    /****************************/
    { 0, 0, {0, 0}, 0, 0 }
};

FlagValueStruct flagValues[] = {
    /****************************/
    /* Simulate dialog		*/
    /****************************/
    {"simulate.swap",      {"off", "on"},   ACTIVITY_SWAP, 0},
    {"simulate.batch",	   {"off", "on"},   LEARN_BATCH, 0},

    /****************************/
    /* Load Map dialog		*/
    /****************************/
    {".loadmap.load",	{"off", "on"}, 		ACTIVITY_LOAD, 0},
    {".loadmap.order",	{"off", "on"}, 		LOAD_RANDOM, 0},

    /****************************/
    /* Connection dialog	*/
    /****************************/
    {"connect.show",	 {"ante", "retro"},	CONNECT_SHOW_TYPE, 0},
    {"connect.toscreen", {"off", "on"},		CONNECT_PRINT, 0},
    {"connect.tofile",   {"off", "on"},		CONNECT_SAVE, 0},

    /****************************/
    /* Modify Cell Activity	*/
    /****************************/
    {"activity.change",{"change", "examine"},	ACTIVITY_STATE, 0},

    /****************************/
    /* Learning Menus           */
    /****************************/
#ifdef NEXUS_BP
    {".backprop.learning", 	{"off", "on"},  LEARN_BP, 	0},
    {".backprop.saveerr", 	{"off", "on"},   SAVE_ERR_BP, 	  0},
#endif

#ifdef NEXUS_HEBB
    {".hebb.learning",	 	{"off", "on"},  LEARN_HEBB, 	0},
    {".hebb.type", 		{"rate", "voltage"},   LEARN_HEBB_TYPE, 0},
#endif

#ifdef NEXUS_INVERSION
    {".inver.learning", 	{"off", "on"},  LEARN_INVERS, 	0},
    {".inver.saveerr", 		{"off", "on"},   SAVE_ERR_INVERS, 0},
#endif

#ifdef NEXUS_RBF
    {".rbf.learning", 		{"off", "on"},  LEARN_RBF, 	0},
    {".rbf.saveerr", 		{"off", "on"},   SAVE_ERR_RBF, 	  0},
#endif

    {"activity.cycle", 		{"off","on"}, 	ACTIVITY_CYCLE ,0},


    /*  MISSING: FILE_COMPRESSED flag */

    /****************************/
    /* end			*/
    /****************************/
    { 0, {0, 0}, 0, 0 }
};

IntValueStruct intValues[] = {
    /****************************/
    /* Simulate dialog		*/
    /****************************/
    {"simulate.cycles", 	&number_cycles, 0},
    {"simulate.cells",		&random_cells, 0},


    {"activity.rate",		&setActivityPercentage, 	0},


    {".editcon.connection",	&edit_connect_current_id, 	0},

#ifdef NEXUS_HEBB
    /****************************/
    /* Hebb			*/
    /****************************/
    {".hebb.plus_plus",		&hebbPlusPlus, 		0},
    {".hebb.plus_minus",	&hebbPlusMinus, 	0},
    {".hebb.minus_plus",	&hebbMinusPlus, 	0},
    {".hebb.minus_minus",	&hebbMinusMinus, 	0},
#endif
		
    {"graphics_off",		&graphics_off, 		0},
		
    /****************************/
    /* end			*/
    /****************************/
    { 0, 0, 0 }
};

FloatValueStruct floatValues[] = {

    /****************************/
    /*Randomize Cell Activity	*/
    /****************************/
    {".randomize:minfire",	&random_cell_min_firing, 0},
    {".randomize:maxfire",	&random_cell_max_firing, 0},

    /****************************/
    /*Modify Cell Activity	*/
    /****************************/
    {"activity.min",		&activity_display_range_min, 0},
    {"activity.max",		&activity_display_range_max, 0},

    /****************************/
    /* Connectivity		*/
    /****************************/
    {"connect.dispmin",		&conduct_display_range_min, 0},
    {"connect.dispmax",		&conduct_display_range_max, 0},
    {"connect.compmin",		&conduct_compute_range_min, 0},
    {"connect.compmax",		&conduct_compute_range_max, 0},

    /****************************/
    /* Simulation View		*/
    /****************************/
    {"view.movement",		&translation_amount, 0},
    {"view.scale",		&change_dimension_amount, 0},

#ifdef NEXUS_BP
    {".backprop.learnrate_value",	&bpLearningRate, 0},
    {".backprop.momentum_value",	&bpParaMomentum, 0},
#endif

#ifdef NEXUS_INVERSION
    {".inver.rate_value",	&invLearningRate, 	0},
    {".inver.approx_value",	&invSecondRatio, 	0},
    {".inver.approx2_value",	&invSecondApprox,	0},
    {".inver.decay_value",	&invDecay,		0},
#endif

#ifdef NEXUS_HEBB
    {".hebb.thresh_value",	&hebbPreThreshold,	0},
    {".hebb.pre_plus_value",	&hebbPlusPostThreshold,	0},
    {".hebb.pre_minus_value",	&hebbMinusPostThreshold,0},
#endif

#ifdef NEXUS_RBF
    {".rbf.i_bias_value",	&rbfInitBias,		0},
    {".rbf.smooth_value",	&rbfInitSmooth,		0},
    {".rbf.centers_value",	&rbfParaCenter,		0},
    {".rbf.l_bias_value",	&rbfParaBias,		0},
    {".rbf.weights_value",	&rbfParaWeight,		0},
    {".rbf.moment_value",	&rbfParaMomentum,	0},
    {".rbf.min_value",		&rbfParaMinChange,	0},
#endif


    { 0, 0, 0 }
};

StringValueStruct stringValues[] = {
    /****************************/
    /* Activity			*/
    /****************************/
    {"activity.network",	activity_output_network_name, 0},
    {"activity.filename",	activity_output_filename, 0},

    /****************************/
    /* View			*/
    /****************************/
    {"view.network",		view_network, 0},

    /****************************/
    /* LoadMap			*/
    /****************************/
    {".loadmap.network",	network_load_activity, 0},
    {".loadmap.filename_b",	filename_load_activity, 0},
    {".loadmap.filename_a",	activity_filename, 0},

    /****************************/
    /* Edit Connection		*/
    /****************************/
    {".editcon.network",	edit_connect_network, 	0},
    {".editcon.function",	edit_connect_function, 	0},

    /****************************/
    /* Connectivity		*/
    /****************************/
    {"connect.filename",	send_connections_filename, 0},

    /****************************/
    /* Set Parameters		*/
    /****************************/
    {".setparam.network",	parameter_network, 0},
    {".setparam.value",		parameter_value, 0},

    /****************************/
    /* Learning Menus: files    */
    /****************************/

#ifdef NEXUS_BP
    {".backprop.outfile",	bpBatchFilename, 0},
    {".backprop.filename_err",	bpErrorFilename, 0},
    {".backprop.hidnet_name",	bpHiddenNetwork, 0},
    {".backprop.outnet_name",	bpOutputNetwork, 0},
#endif

#ifdef NEXUS_INVERSION
    {".inver.outfile",		invOutputFilename, 0},
    {".inver.filename_err",	invErrorFilename, 0},
    {".inver.innet_name",	invInputNetwork, 0},
    {".inver.outnet_name",	invOutputNetwork, 0},
#endif

#ifdef NEXUS_RBF
    {".rbf.outfile",		rbfBatchFilename, 0},
    {".rbf.filename_err",	rbfErrorFilename, 0},
    {".rbf.innet_name",		rbfInputNetwork, 0},
    {".rbf.outnet_name",	rbfOutputNetwork, 0},
    {".rbf.hidnet_name",	rbfHiddenNetwork, 0},
#endif


    {0,0,0}
};






/*----------------------------------------------------------------------
 *
 *			INDEX Values
 *
 *----------------------------------------------------------------------
 */

TRACE_PROC(IndexValueTraceProc)
{
  IndexValueStruct * valPtr = (IndexValueStruct*) clientData;
  char * val = GET_VAR(valPtr->tclVar);
  int i;

  if (explore_variables)
    fprintf(stderr, "Index var. = %s;\t val. = %s\n", valPtr->tclVar, val);

  if (val) {
    for (i = 0; i < valPtr->numValues; i++) {
      if (strcmp(val, valPtr->tclStrings[i]) == 0) {

	*valPtr->value = i;

	if (valPtr->value == &view_elect || 
	    valPtr->value == &connect_elect || 
	    valPtr->value == &activity_elect) {
	  SetElectrode(valPtr->value, i);
	}
	else if (valPtr->value == &activity_display_depth)
	  redrawGraphics( );
	else if (valPtr->value == &conduct_display_depth)
	  (void) draw_outline( TRUE );

	return 0;
      }
    }
  }

  /* This is the default */
  *valPtr->value = 0;

  if (explore_variables)
    fprintf(stderr, "unknown flag value.\n");

  return 0;
}

/*----------------------------------------------------------------------
 *
 *			FLAG Values
 *
 *----------------------------------------------------------------------
 */

TRACE_PROC(FlagValueTraceProc)
{
  FlagValueStruct * valPtr = (FlagValueStruct*) clientData;
  char * val = GET_VAR(valPtr->tclVar);
  int i;

  if (explore_variables)
    fprintf(stderr, "Flag var. = %s;\t val. = %s\n", valPtr->tclVar, val);

  if (val) {
    for (i=0; i < 2; i++) {
      if (strcmp(val, valPtr->tclStrings[i]) == 0) {
	SET_FLAG(valPtr->flag_index, i);
	return 0;
      }
    }
  }

  /* This is the default */
  SET_FLAG(valPtr->flag_index, 0);

  if (explore_variables)
    fprintf(stderr, "unknown flag value.\n");

  return 0;
}

/*----------------------------------------------------------------------
 *
 *			INT Values
 *
 *----------------------------------------------------------------------
 */

TRACE_PROC(IntValueTraceProc)
{
  IntValueStruct * valPtr = (IntValueStruct*) clientData;
  char * val = GET_VAR(valPtr->tclVar);


  if (val) {
    *valPtr->value = atoi(val);
  }

  if (explore_variables)
    fprintf(stderr, "Integer var. = %s;\t val. = %s\n", valPtr->tclVar, val);

  return 0;
}



/*----------------------------------------------------------------------
 *
 *			FLOAT Values
 *
 *----------------------------------------------------------------------
 */

static void SetFloatValue(FloatValueStruct * valPtr)
{
  char buff[1024];

  sprintf(buff, "%f", *(valPtr->value));
  Tcl_SetVar(nxInterp, valPtr->tclVar, buff, TCL_GLOBAL_ONLY);
}

TRACE_PROC(FloatValueTraceProc)
{
  FloatValueStruct * valPtr = (FloatValueStruct*) clientData;
  char * val = GET_VAR(valPtr->tclVar);

  if (val) {
    *valPtr->value = atof(val);
  }

  if (explore_variables)
    fprintf(stderr, "Float var. = %s;\t val. = %f\n",
	    valPtr->tclVar, atof(val));

  /*--------------------------------------------------
   *	Min/Max values in "Randomize cell activity"
   *--------------------------------------------------
   */
  if (valPtr->value == &random_cell_min_firing) {
    if (random_cell_min_firing > random_cell_max_firing) {
      random_cell_min_firing = random_cell_max_firing;
      SetFloatValue(valPtr);
    }
  }
  else if (valPtr->value == &random_cell_max_firing) {
    if (random_cell_max_firing < random_cell_min_firing) {
      random_cell_max_firing = random_cell_min_firing;
      SetFloatValue(valPtr);
    }
  }
  /*--------------------------------------------------
   *	Min/Max values in "Activity"
   *--------------------------------------------------
   */
  else if (valPtr->value == &activity_display_range_min) {
    if (activity_display_range_min > activity_display_range_max) {
      activity_display_range_min = activity_display_range_max;
      SetFloatValue(valPtr);
    }
    redrawGraphics( );
  }
  else if (valPtr->value == &activity_display_range_max) {
    if (activity_display_range_max < activity_display_range_min) {
      activity_display_range_max = activity_display_range_min;
      SetFloatValue(valPtr);
    }
    redrawGraphics( );
  }
  /*--------------------------------------------------
   *	Min/Max values in "Connectivity"
   *--------------------------------------------------
   */
  else if (valPtr->value == &conduct_display_range_min) {
    if (conduct_display_range_min > conduct_display_range_max) {
      conduct_display_range_min = conduct_display_range_max;
      SetFloatValue(valPtr);
    }
    (void) drawlegends( );
  }
  else if (valPtr->value == &conduct_display_range_max) {
    if (conduct_display_range_max < conduct_display_range_min) {
      conduct_display_range_max = conduct_display_range_min;
      SetFloatValue(valPtr);
    }
    (void) drawlegends( );
  }
  else if (valPtr->value == &conduct_compute_range_min) {
    if (conduct_compute_range_min > conduct_compute_range_max) {
      conduct_compute_range_min = conduct_compute_range_max;
      SetFloatValue(valPtr);
    }
    /*
     *  Backward compatibility hack.  DW 94.08.31
     */
    setting->conduct_min = conduct_compute_range_min;
  }
  else if (valPtr->value == &conduct_compute_range_max) {
    if (conduct_compute_range_max < conduct_compute_range_min) {
      conduct_compute_range_max = conduct_compute_range_min;
      SetFloatValue(valPtr);
    }
    /*
     *  Backward compatibility hack.  DW 94.08.31
     */
    setting->conduct_max = conduct_compute_range_max;
  }

  return 0;
}

/*----------------------------------------------------------------------
 *
 *			STRING Values
 *
 *----------------------------------------------------------------------
 */
TRACE_PROC(StringValueTraceProc)
{
  StringValueStruct * valPtr = (StringValueStruct*) clientData;
  char * val = GET_VAR(valPtr->tclVar);

  if (val) {
    strcpy(valPtr->value, val);
  }

  if (explore_variables)
    fprintf(stderr, "String var. = %s;\t val. = %s\n", valPtr->tclVar, val);

  return 0;
}

/*----------------------------------------------------------------------
 *
 * TkNx_InitTraceVars -
 *
 *	Initialize the TCL "trace variable" routines.
 *
 *----------------------------------------------------------------------
 */

void
TkNx_InitTraceVars( Tcl_Interp *interp )
{
  IndexValueStruct  * indexPtr;
  FlagValueStruct   * flagPtr;
  IntValueStruct    * intPtr;
  FloatValueStruct  * floatPtr;
  StringValueStruct * stringPtr;
  int flags = TCL_GLOBAL_ONLY | TCL_TRACE_WRITES;
  char buff[100];

  /* Initialize the modules first */
  TkNx_ElecInit();
  TkNx_SimInit();
  TkNx_ModInit();

  /* Trace the index */
  for (indexPtr=indexValues; indexPtr->tclVar; indexPtr++) {
    Tcl_SetVar(interp, indexPtr->tclVar,
	       indexPtr->tclStrings[*(indexPtr->value)], TCL_GLOBAL_ONLY);
    Tcl_TraceVar(interp, indexPtr->tclVar, flags, IndexValueTraceProc,
		 (ClientData) indexPtr);
  }

  /* Trace the flag */
  for (flagPtr=flagValues; flagPtr->tclVar; flagPtr++) {
    Tcl_SetVar(interp, flagPtr->tclVar,
	       flagPtr->tclStrings[QUERY_FLAG(flagPtr->flag_index)],
	       TCL_GLOBAL_ONLY);
    Tcl_TraceVar(interp, flagPtr->tclVar, flags, FlagValueTraceProc,
		 (ClientData) flagPtr);
  }

  /* Trace the int */
  for (intPtr=intValues; intPtr->tclVar; intPtr++) {
    sprintf(buff, "%i", *(intPtr->value));
    Tcl_SetVar(interp, intPtr->tclVar, buff, TCL_GLOBAL_ONLY);
    Tcl_TraceVar(interp, intPtr->tclVar, flags, IntValueTraceProc,
		 (ClientData) intPtr);
  }

  /* Trace the float */
  for (floatPtr=floatValues; floatPtr->tclVar; floatPtr++) {
    sprintf(buff, "%f", *(floatPtr->value));
    Tcl_SetVar(interp, floatPtr->tclVar, buff, TCL_GLOBAL_ONLY);
    Tcl_TraceVar(interp, floatPtr->tclVar, flags, FloatValueTraceProc,
		 (ClientData) floatPtr);
  }

  /* Trace the strings */
  for (stringPtr=stringValues; stringPtr->tclVar; stringPtr++) {
    Tcl_SetVar(interp, stringPtr->tclVar,stringPtr->value,TCL_GLOBAL_ONLY);
    Tcl_TraceVar(interp, stringPtr->tclVar, flags, StringValueTraceProc,
		 (ClientData) stringPtr);
  }
}

/* set_flag --
 *
 * Set nexus_flags appropriately, and handle any TCL widgets.  DW 94.10.29
 *
 */
void
set_flag( which_flag, value )
    register flag_t	which_flag;
    register int	value;
{
    extern flag_t nexus_flags; /* used in the SET_FLAG macro. */
    FlagValueStruct * valPtr;

    SET_FLAG( which_flag, value );

    for (valPtr = flagValues; valPtr->tclVar; valPtr++) {
	if (valPtr->flag_index == which_flag) {
	    SET_VAR(valPtr->tclVar, valPtr->tclStrings[value]);
	    return;
	}
    }
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
