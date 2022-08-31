#ifndef _NEXUS_TK_H_
#define _NEXUS_TK_H_

#include "nexus.h"
#include <stdlib.h>
#include "tk.h"

/************************/
/* nexus_tkmain.c	*/
/************************/
int TkNx_Init(Tcl_Interp * interp, Tk_Window  topLevel);

/************************/
/* nexus_tkdraw.c	*/
/************************/
int TkNx_RedrawGraphWin(ClientData c, Tcl_Interp *i, int  argc, char **argv);

/************************/
/* tkGlx.c		*/
/************************/
int TkGLx_Init(Tcl_Interp * interp, Tk_Window  topLevel);

/*
 * Convience macros for the tk based nexus package.
 */

#define CMD_CREATE3(interp, cmdname, funcname) \
        Tcl_CreateCommand(interp, cmdname, funcname, NULL, NULL)

#define CMD_CREATE4(interp, cmdname, funcname, clientData) \
        Tcl_CreateCommand(interp, cmdname, funcname, clientData, NULL)

/*
 *  This next line can be #ifdef DEBUG to turn on command tracing.
 */
#if 0

#define TCL_CHK(_name, _argc, _count, _arglist) \
  fprintf(stderr, "called tcl command %s\n", _name); \
 if (_argc != _count+1) { \
     Tcl_AppendResult(interp, "Wrong number of arguments for ",\
                      _name, ": should be ", _arglist, NULL); \
     return TCL_ERROR; \
 }

#else

#define TCL_CHK(_name, _argc, _count, _arglist) \
 if (_argc != _count+1) { \
     Tcl_AppendResult(interp, "Wrong number of arguments for ",\
                      _name, ": should be ", _arglist, NULL); \
     return TCL_ERROR; \
 }

#endif

extern Tcl_Interp * nxInterp;

#define COMMAND_PROC(proc) \
int proc(ClientData clientdata, Tcl_Interp *interp, int argc, char **argv )


/*----------------------------------------------------------------------
 * Macros for trace vars
 *----------------------------------------------------------------------
 */

#define TRACE_PROC(proc) \
char *proc(ClientData clientData, \
	Tcl_Interp *interp, char *name1, char *name2, int flags)

#define CMP_VAR(var, val) \
  (strcmp(Tcl_GetVar(interp, var, TCL_GLOBAL_ONLY),val)==0)

#define GET_VAR(var) \
  Tcl_GetVar(nxInterp, var, TCL_GLOBAL_ONLY)

#define SET_VAR(var,val) \
  (Tcl_SetVar(nxInterp, var, val, TCL_GLOBAL_ONLY))

typedef int (*NxVarCallback)(Tcl_Interp * interp, char * var, char * val);

typedef struct {
    char  * tclVar;
    int     numValues;
    char  * tclStrings[10];
    int   * value;
    NxVarCallback proc;
} IndexValueStruct;

typedef struct {
    char  * tclVar;
    char  * tclStrings[10];
    int     flag_index;
    NxVarCallback proc;
} FlagValueStruct;

typedef struct {
    char  * tclVar;
    int   * value;
    NxVarCallback proc;
} IntValueStruct;

typedef struct {
    char  * tclVar;
    float * value;
    NxVarCallback proc;
}  FloatValueStruct;

typedef struct {
    char * tclVar;
    char * value;
    NxVarCallback proc;
}  StringValueStruct;

/*----------------------------------------------------------------------
 *
 * 		Some globals in nexus used by TCL module
 *
 *----------------------------------------------------------------------
 */

extern int activity_elect;

/************************/
/* nexus_tcl_var.c	*/
/************************/
extern float random_cell_min_firing;
extern float random_cell_max_firing;

/************************/
/* nexus_tcl_mod.c	*/
/************************/
extern char parameter_network[NAME_SIZE];
extern char parameter_value[NAME_SIZE];
extern int param_choice;

/************************/
/* nexus_tcl_elec.c	*/
/************************/
extern int view_elect;
extern int connect_elect;
extern int activity_elect;

/************************/
/* Hebb			*/
/************************/
extern int	hebbFactorChoice,
		hebbPlusPlus,
		hebbPlusMinus,
		hebbMinusPlus,
		hebbMinusMinus;

extern float	hebbPreThreshold,
		hebbPlusPostThreshold,
		hebbMinusPostThreshold;

#define NX_GLXFRAMEBUF 0
#define NX_GLXPIXMAP 1

extern void		GlxwinLink _ANSI_ARGS_(( register int ));

#endif /* _NEXUS_TK_H_ */
