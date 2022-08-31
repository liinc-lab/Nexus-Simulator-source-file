#ifndef _NEXUS_BATCH_H
#define _NEXUS_BATCH_H

/*
 *  Batch File internal list structure.  DW 94.11.05
 */
struct batchList {
  int	flagPicked;
  int	numArgs;
  char	filename[NAME_SIZE];
  char	networkName[NAME_SIZE];
};
typedef struct batchList ELEMENT_BATCHLIST;
typedef ELEMENT_BATCHLIST *BATCHLIST;


/*
 *  DW 95.02.07
 *  Added selection of which type of batch file is desired.  This allows
 *    the use of one routine to read a batch file, whether it be a
 *    Training Inputs or Training Outputs Batch File; and one routine to
 *    read a file as listed in the batch file; these may be done in an
 *    asynchronous manner since separate file pointers are kept.
 *
 *  This necessitated the following two definitions, and the alteration
 *    of most batch variables into arrays.
 */

typedef enum { NXBATCH_IN, NXBATCH_OUT } batch_t;

#define NUM_BAT_FILES	2

extern BATCHLIST	currBatchList[];
extern int		currBatchListLength[];

extern int		LOAD_MARKER;
extern int		END_OF_BATCH_FILES;
extern char		filename_load_activity[];
extern char		network_load_activity[];

extern int		readBatchFile _ANSI_ARGS_(( batch_t, char * ));
extern int		getBatchActivityFile _ANSI_ARGS_(( batch_t, int,
							  char *, char * ));
extern int		init_activity_cycle _ANSI_ARGS_(( void ));
extern int		load_activity_cycle _ANSI_ARGS_(( void ));

#endif /* _NEXUS_BATCH_H */
