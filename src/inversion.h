#ifndef NEXUS_INVERSION_H
#define NEXUS_INVERSION_H

/* structures added when using inversion */

struct inv_network {
  int id;
  struct network *net_ptr;
  struct inv_cell *inv_cells;
  struct inv_network *next;
};
typedef struct inv_network ELEMENT_INV_NETWORK;
typedef ELEMENT_INV_NETWORK *INV_NETWORK;



struct inv_cell{
  int id;
  float dEx;           /* derivative for inversion */
  float batch_firing;
  struct cell *cell_ptr;
};
typedef struct inv_cell ELEMENT_INV_CELL;
typedef ELEMENT_INV_CELL *INV_CELL;



extern char		invOutputFilename[],
  			invErrorFilename[],
  			invInputNetwork[],
  			invOutputNetwork[];

extern float		invLearningRate,
  			invSecondRatio,
  			invSecondApprox,
  			invDecay;

extern void		invErrorMsg _ANSI_ARGS_(( char * ));
extern int		invErrorQuery _ANSI_ARGS_(( char * ));
extern void		closeErrorFileInv _ANSI_ARGS_(( void ));
extern char *		invAllocParam _ANSI_ARGS_(( void ));
extern void		invFreeParam _ANSI_ARGS_(( void ));
extern int		run_inversion _ANSI_ARGS_((int count_cycles,
						   int cycles ));

#endif /* NEXUS_INVERSION_H */
