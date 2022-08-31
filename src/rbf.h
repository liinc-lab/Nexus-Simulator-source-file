#ifndef NEXUS_RBF_H
#define NEXUS_RBF_H

/*
 *  Use same data structure type for rbf as for backprop since all 
 *    that is need is a temp variable to hold the derivative 
 */

struct rbf_network {
  int id;
  struct network *net_ptr;
  struct rbf_cell *rbf_cells;
  struct rbf_network *next;
};
typedef struct rbf_network ELEMENT_RBF_NETWORK;
typedef ELEMENT_RBF_NETWORK *RBF_NETWORK;



struct rbf_cell {
  int id;
  float bias;           /* derivative for bp */
  float batch_bias;
  float bp_error;
  struct rbf_connect *connect_list;
  struct cell *cell_ptr;
};
typedef struct rbf_cell ELEMENT_RBF_CELL;
typedef ELEMENT_RBF_CELL *RBF_CELL;



struct rbf_connect {
  float batch_conductance;
  float momentum;
};
typedef struct rbf_connect ELEMENT_RBF_CONNECT;
typedef ELEMENT_RBF_CONNECT *RBF_CONNECT;



struct matrix {
  int rows;
  int cols;
  float **data;
};
typedef struct matrix ELEMENT_MATRIX;
typedef ELEMENT_MATRIX *MATRIX;



extern char		rbfBatchFilename[NAME_SIZE],
  			rbfErrorFilename[NAME_SIZE],
  			rbfInputNetwork[NAME_SIZE],
  			rbfOutputNetwork[NAME_SIZE],
  			rbfHiddenNetwork[NAME_SIZE];

extern float		rbfInitBias,
			rbfInitSmooth,
			rbfParaCenter,
			rbfParaBias,
			rbfParaWeight,
			rbfParaMomentum,
			rbfParaMinChange;

extern void		rbfErrorMsg _ANSI_ARGS_(( char * ));
extern int		rbfErrorQuery _ANSI_ARGS_(( char * ));
extern void		closeErrorFileRBF _ANSI_ARGS_(( void ));
extern char *		rbfAllocParam _ANSI_ARGS_(( void ));
extern void		rbfFreeParam _ANSI_ARGS_(( void ));

extern int		rbf_parameters_allocate;
extern int		update_cell_rbf_gaussian _ANSI_ARGS_(( CELL,
							      NETWORK ));
extern int		update_cell_rbf_logistic _ANSI_ARGS_(( CELL,
							      NETWORK ));

extern void		load_rbf_networks _ANSI_ARGS_((FILE *fp));
extern void		rbf_print_bias _ANSI_ARGS_((int net_id, int cell_id));

extern int		run_rbf _ANSI_ARGS_((int count_cycles,int cycles));

#endif /* NEXUS_RBF_H */










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
