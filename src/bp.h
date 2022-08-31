#ifndef NEXUS_BP_H
#define NEXUS_BP_H

/*****************************************************************************
 *  Back Propagation learning method header file.                            *
 *****************************************************************************/



#define BP_HIDDEN 0
#define BP_OUTPUT 1



struct bp_network {
  int id;
  struct network *net_ptr;
  struct bp_cell *bp_cells;
  struct bp_network *next;
};
typedef struct bp_network ELEMENT_BP_NETWORK;
typedef ELEMENT_BP_NETWORK *BP_NETWORK;



struct bp_cell {
  int id;
  float dEx;           /* derivative for bp */
  struct bp_connect *connect_list;
  struct cell *cell_ptr;
};
typedef struct bp_cell ELEMENT_BP_CELL;
typedef ELEMENT_BP_CELL *BP_CELL;



struct bp_connect {
  float momentum;
  float batch_conductance;
};
typedef struct bp_connect ELEMENT_BP_CONNECT;
typedef ELEMENT_BP_CONNECT *BP_CONNECT;



extern NETWORK		network_head;
extern BP_NETWORK	bp_network_head;
extern int		bp_parameters_allocate;
extern float		bpLearningRate,
			bpParaMomentum;
extern char		bpBatchFilename[],
  			bpErrorFilename[],
			bpHiddenNetwork[],
			bpOutputNetwork[];

extern void		bpErrorMsg _ANSI_ARGS_(( char * ));
extern int		bpErrorQuery _ANSI_ARGS_(( char * ));
extern void		closeErrorFileBP _ANSI_ARGS_(( void ));
extern char *		bpAllocParam _ANSI_ARGS_(( void ));
extern void		bpFreeParam _ANSI_ARGS_(( void ));
extern int		run_bp _ANSI_ARGS_((int count_cycles, int cycles));

#endif /* NEXUS_BP_H */
