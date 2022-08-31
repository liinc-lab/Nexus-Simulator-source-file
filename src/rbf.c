/**************************************************************
 *                                                            *
 *                           NEXUS                            *
 *                                                            *
 *             (c) 1990 Paul Sajda and Leif Finkel            *
 *                                                            *
 *                       (nexus_rbf.c)                        *
 *                                                            *
 **************************************************************/

#include "nexus.h"
#include "rbf.h"
#include "batch.h"
#include "functions.h"
#include "graphics.h"
#include "tcl_main.h"
#include "nexus_opengl.h"

#ifdef NEXUS_SGI
#include "nexus_tk.h"
#endif

#ifdef NEXUS_LINUX
#include "nexus_tk.h"
#endif

#ifdef NEXUS_SUN
#include <nexus_xview.h>
#include <nexus_xview_functions.h>
#endif



RBF_NETWORK		rbf_network_head,
			rh_head, ro_head;
int			rbf_parameters_allocate = FALSE;
float			rbfInitBias		= 0.002,
			rbfInitSmooth		= 0.02,
			rbfParaCenter		= 0.2,
			rbfParaBias		= 0.0,
			rbfParaWeight		= 0.2,
			rbfParaMinChange	= 0.0,
			rbfParaMomentum		= 0.0;
char			rbfBatchFilename[NAME_SIZE],
			rbfTrainFilename[NAME_SIZE],
  			rbfErrorFilename[NAME_SIZE],
			rbfInputNetwork[NAME_SIZE],
			rbfHiddenNetwork[NAME_SIZE],
			rbfOutputNetwork[NAME_SIZE];


static NETWORK		ihead, hhead, ohead;
static RBF_NETWORK	rbfAllocNetwork _ANSI_ARGS_(( void ));
static RBF_CELL		rbfAllocCells _ANSI_ARGS_(( unsigned number ));
static RBF_CONNECT	rbfAllocConnections _ANSI_ARGS_(( unsigned number ));
static FILE		*rbf_save_error_fp;


#ifdef NEXUS_SUN
Frame		rbf_frame;
Panel_item	chbox_rbf,
		chbox_save_err_rbf,
		rbf_network_input_list,
		rbf_network_hidden_list,
		rbf_network_output_list;

static Panel	rbf_panel;
#endif







static RBF_NETWORK
rbfAllocNetwork( )
{
  RBF_NETWORK	head;


  if ((head = (RBF_NETWORK) malloc( sizeof( ELEMENT_RBF_NETWORK ) )))
    return head;

  printf( "FATAL: Memory allocation failure, rbfAllocNetwork.\n" );
  exit(1);
}  /* rbfAllocNetwork( ) */



/* create cells of network */
/* Note - calloc() is used to increase searching efficiency */

static RBF_CELL
rbfAllocCells( number )
    unsigned number;
{
  RBF_CELL	head;


  if ((head = (RBF_CELL) calloc( number, sizeof( ELEMENT_RBF_CELL ) )))
    return head;

  printf( "FATAL: Memory allocation failure, rbfAllocCells.\n" );
  exit(1);
}  /* rbfAllocCells( ) */



static RBF_CONNECT
rbfAllocConnections( number )
    unsigned number;
{
  RBF_CONNECT	head;


  if (number == 0)
    return NULL;

  if ((head = (RBF_CONNECT) calloc( number, sizeof( ELEMENT_RBF_CONNECT ) )))
    return head;

  printf( "FATAL: Memory allocation failure, rbfAllocConnections.\n");
  exit(1);
}  /* rbfAllocConnections( ) */



/* finds the hidden network as well as the rbf network associated */
/* updates the activity using the cell bias of the rbf network    */
/* cell input = input_cell->activity - conductance                */
/* cell activity = exp( -bias * sum_input * sum_input)            */

int
update_cell_rbf_gaussian( cell, head )
    CELL cell;
    NETWORK head;
{
  RBF_NETWORK		rhead;
  register int		count;
  register float	input, norm_input;
  float			bias;


  rhead = rbf_network_head;
  while (rhead != NULL && rhead->id != head->id)
    rhead = rhead->next;
  
  if (rhead == NULL) {
    printf("RBF Network for %s layer not created!\n",head->name);
    return ERROR;
  }

  for (count = 0, norm_input = 0;
       count < (cell->number_connections);
       count++) {
    input = ((cell->connect_list + count)->input_cell->firing_rate 
	     - (cell->connect_list + count)->conductance);
    norm_input += input * input;
  }

  cell->voltage = norm_input;
  bias  = (rhead->rbf_cells + (cell->id - 1))->bias;
  cell->firing_rate = (float) exp(-bias*norm_input);

  return OK;
}



/* finds the output network and the rbf network associated           */
/* updates the activity of each cell using the bias of the rbf cells */
/* cell input = input_cell->firing * conductance                     */
/* cell activity = (1.0/(1.0 + exp(-bias-sum_input)))                */

int
update_cell_rbf_logistic( cell, head )
    CELL cell;
    NETWORK head;
{
  RBF_NETWORK	rhead;
  float		bias;


  rhead = rbf_network_head;
  while (rhead != NULL && rhead->id != head->id)
    rhead = rhead->next;

  if (rhead == NULL) {
    printf( "RBF Network for %s layer not created!\n", head->name );
    return ERROR;
  }

  bias  = (rhead->rbf_cells + (cell->id - 1))->bias;
  cell->firing_rate = (float) (1.0 / (1.0 + exp( -bias - (cell->voltage) )));

  return OK;
}



void
save_rbf_networks( fp )
    FILE *fp;
{
  RBF_NETWORK rbf_head;
  RBF_CELL rbf_cell;
  int count_cells;

  if(rbf_network_head!=NULL) {
    for(rbf_head=rbf_network_head;rbf_head!=NULL;rbf_head=rbf_head->next) {
      fprintf( fp, "R_%d\n", rbf_head->id );
      for (count_cells = 0;
	   count_cells < rbf_head->net_ptr->number_cells;
	   count_cells++) {
	rbf_cell = rbf_head->rbf_cells + count_cells;
	fprintf( fp, "C_%d %f\n", rbf_cell->id, rbf_cell->bias );
      }
      printf("Finished Saving RBF Network %s\n",rbf_head->net_ptr->name);
    }
  }
}



void
load_rbf_networks( fp )
    FILE *fp;
{
  RBF_NETWORK rbf_head,old_rbf_head;
  NETWORK head;
  RBF_CELL rcell;
  int count_cells;
  char input[FILE_BUFSIZE];

  
  rbf_head = rbfAllocNetwork();
  rbf_network_head = rbf_head;
  
  while (fgets( input, FILE_BUFSIZE, fp )) {
    sscanf( input, "R_%d", &rbf_head->id );
    head = network_head;
    while(head->id!=rbf_head->id)
      head = head->next;
    
    rbf_head->net_ptr = head;
    rbf_head->rbf_cells = rbfAllocCells(head->number_cells);
    
    for(count_cells = 0;count_cells<head->number_cells;count_cells++) {
      rcell = rbf_head->rbf_cells + count_cells;
      rcell->id = (head->cells + count_cells)->id;
      fgets( input, FILE_BUFSIZE, fp );
      sscanf( input, "C_%d %f", &rcell->id, &rcell->bias );
      rcell->connect_list =
	rbfAllocConnections((head->cells + count_cells)->number_connections);
      rcell->cell_ptr = (head->cells + count_cells);
    }
    printf("Finished Loading RBF Network %s\n",head->name);
    rbf_head->next = rbfAllocNetwork();
    old_rbf_head = rbf_head;
    rbf_head = rbf_head->next;
  };
  
  old_rbf_head->next = NULL;
  ro_head = rbf_network_head;
  ohead = rbf_network_head->net_ptr;
  rh_head = rbf_network_head->next;
  hhead = rbf_network_head->next->net_ptr;

  rbf_parameters_allocate = TRUE;
}



void
rbf_print_bias( net_id, cell_id )
    int net_id, cell_id;
{
  RBF_NETWORK rbf_head;

  if (rbf_network_head != NULL) {
    for (rbf_head = rbf_network_head;
	 (rbf_head != NULL) && (rbf_head->id != net_id);
	 rbf_head = rbf_head->next);
    if (rbf_head != NULL)
      printf( "\t bias         = %f\n\n",
	     (rbf_head->rbf_cells + (cell_id - 1))->bias );
    else
      printf("\n");
  }
}



/*
 *  DW 95.01.27  Free RBF-specific storage.
 */

void
rbfFreeParam( )
{
  register RBF_NETWORK	netPtr, oldNetPtr;
  register RBF_CELL	cellPtr;
  register int		cellCount,
  			numCells;


  if (!rbf_parameters_allocate)
    return;

  netPtr = rbf_network_head;
  while (netPtr) {

    oldNetPtr = netPtr;
    cellPtr = netPtr->rbf_cells;
    numCells = netPtr->net_ptr->number_cells;

    for (cellCount = 0; cellCount < numCells; cellCount++) {
      free( (cellPtr + cellCount)->connect_list );
      free( cellPtr + cellCount );
    }

    free( oldNetPtr );
    netPtr = netPtr->next;
  }

  rbf_network_head		= NULL;
  rbf_parameters_allocate	= FALSE;
}



/* makeMatrix - allocate space for 2 dimensional array */

MATRIX
makeMatrix( dim1, dim2 )
    int     dim1, dim2;
{
  int		i;
  float		**ptr;
  MATRIX	matrix;

  matrix = (MATRIX) malloc(sizeof(ELEMENT_MATRIX));
  matrix->rows = dim1;
  matrix->cols = dim2;
  if((ptr = (float **) calloc(dim1+1,sizeof(float *)))==NULL) {
    printf("calloc failed in makeMatrix() - ABORT RBF INITIALIZATION\n");
    exit(1);
  }
  for (i=0;i<dim1;i++)
    if((ptr[i] = (float *)calloc(dim2,sizeof(float)))==NULL) {
      printf("calloc failed in makeMatrix() - ABORT RBF INITIALIZATION\n");
      exit(1);
    }
  ptr[dim1]=NULL;
  matrix->data = ptr;
  return(matrix);
}



MATRIX
MMultiply( M1, M2 )
    MATRIX M1,M2;
{

  MATRIX M;
  int r,c,rows,cols,index,count;

  rows=M1->rows;
  cols=M2->cols;
  index=M1->cols;

  M = (MATRIX) makeMatrix(rows,cols);
  for(r=0;r<rows;r++)
    for(c=0;c<cols;c++)
      for(count=0;count<index;count++)
	M->data[r][c] += (float) (M1->data[r][count] * M2->data[count][c]);

  return(M);
}



MATRIX
MAdd( M1, M2 )
    MATRIX M1,M2;
{

  MATRIX M;
  int r,c,rows,cols;

  rows=M1->rows;
  cols=M1->cols;

  M = (MATRIX) makeMatrix(rows,cols);
  for(r=0;r<rows;r++)
    for(c=0;c<cols;c++)
      M->data[r][c] = (float) (M1->data[r][c] + M2->data[r][c]);

  return(M);
}



MATRIX
MTranspose( M1 )
    MATRIX M1;
{

  MATRIX M;
  int r,c,rows,cols;

  cols=M1->rows;
  rows=M1->cols;

  M = (MATRIX) makeMatrix(rows,cols);
  for(r=0;r<rows;r++)
    for(c=0;c<cols;c++)
      M->data[r][c] = (float) M1->data[c][r];

  return(M);
}



MATRIX
MInverse( M1 )
    MATRIX M1;
{
  MATRIX M;
  float **a, *col;
  int i, j, *indx, N;
  void ludcmp(),lubksb();

  N=M1->rows;
  a = M1->data;
  if ((indx = (int *) malloc(N*sizeof(int))) == NULL
      || (col = (float *) malloc(N*sizeof(float))) == NULL) {
    printf("malloc failed in MInverse() - ABORT RBF INITIALIZATION\n");
    exit(1);
  }

  M = (MATRIX) makeMatrix(N,N);
  M->rows = N;
  M->cols = N;
  ludcmp(a,N,indx);

/*  printf("a:\n");
  for(i=0;i<N;i++)
    {
      for(j=0;j<N;j++)
	printf("%f, ",a[i][j]);
      printf("\n");
    }
  for(i=0;i<N;i++)
    printf("indx[%i] : %i\n",i,indx[i]); */
  
  for(j=0;j<N;j++)
    {
      for(i=0;i<N;i++)
	col[i]= (float) 0;
      col[j] = (float) 1;
      lubksb(a,N,indx,col);
      for(i=0;i<N;i++)
	M->data[i][j] = col[i];
    }

  return(M);
}



void
ludcmp(a,n,indx)
    int n,*indx;
    float **a;
{
  int i,imax,j,k;
  float big,dum,sum,temp;
  float *vv,*vector();
  void nrerror(),free_vector();

  vv = vector(n);
  for (i = 0; i < n; i++) {
    big = 0.0;
    for (j = 0; j < n; j++)
      if ((temp=fabs(a[i][j])) > big) big=temp;
    if (big == 0.0) {
      fprintf( stderr, "Singular matrix in routine ludcmp().\n" );
      fprintf( stderr, "ABORTing RBF initialization.\n" );
      exit(1);
    }
    vv[i] = 1.0 / big;
  }
  for (j = 0; j < n; j++) {
    for (i = 0; i < j; i++) {
      sum = a[i][j];
      for (k = 0; k < i; k++)
	sum -= a[i][k] * a[k][j];
      a[i][j] = sum;
    }
    big = 0.0;
    for (i = j; i < n; i++) {
      sum = a[i][j];
      for (k = 0; k < j; k++)
	sum -= a[i][k] * a[k][j];
      a[i][j] = sum;
      if ((dum = vv[i] * fabs(sum)) >= big) {
	big = dum;
	imax = i;
      }
    }
    if (j != imax) {
      for (k=0;k<n;k++) {
	dum=a[imax][k];
	a[imax][k]=a[j][k];
	a[j][k]=dum;
      }
      dum = vv[imax];
      vv[imax]=vv[j];
      vv[j] = dum;
    }
    indx[j]=imax;
    if (a[j][j] == 0.0) {
      printf("Singular Matrix in MInverse() - ABORT RBF INITIALIZATION\n");
      exit(1);
    }
    if (j != (n-1)) {
      dum = 1.0 / (a[j][j]);
      for (i=j+1;i<n;i++) a[i][j] *= dum;
    }
  }
  free(vv);
}



void
lubksb(a,n,indx,b)
    float **a,b[];
    int n,*indx;
{
	int i,ii=0,ip,j;
	float sum;

	for (i=0;i<n;i++) {
		ip=indx[i];
		sum=b[ip];
		b[ip]=b[i];
		if (ii)
			for (j=ii-1;j<i;j++) sum -= a[i][j]*b[j];
		else if (sum != 0.0) ii=i+1;
		b[i]=sum;
	}
	for (i=n-1;i>=0;i--) {
		sum=b[i];
		for (j=i+1;j<n;j++) sum -= a[i][j]*b[j];
		b[i]=sum/a[i][i];
	}
}


float *
vector( n )
    int n;
{
  float *v;

  v = (float *) malloc( n * sizeof( float ) );
  if (!v) {
    printf( "ERROR: Memory allocation failure, vector( ).\n" );
    printf( "ABORTing RBF initialization.\n\n" );
    exit (1);
  }
  return (v);
}



/*
 *  Used for debugging.
 */

#if 0
void
printM( M )
    MATRIX M;
{
  int r, c, rows, cols;

  rows = M->rows;
  cols = M->cols;
  
  for (r = 0; r < rows; r++) {
    for (c = 0; c < cols; c++)
      printf( "%f ", M->data[r][c] );
    printf( "\n" );
  }
}
#endif /* 0 */


void
closeErrorFileRBF( )
{
  if (rbf_save_error_fp)
    fclose( rbf_save_error_fp );
}

/*
 *  JB 94.06.11
 *    Can save error without turning learning on.
 *  DW 94.11.06
 *    Added return value to stop simulation if fatal error occurred.  Keeps
 *      NEXUS from crashing if it can't find training file, for instance.
 *      "filename" was overloaded: replaced with better names.
 *      'rfp' is now static local variable 'fp_output'; NO NEED for a global.
 *      Also, renamed "fp" to "fp_train" so that we know what it means.
 */

int
run_rbf( count_cycles, cycles )
     int count_cycles, cycles;
{
  FILE			*fp_train;
  static FILE		*fp_output;
  CELL			cell;
  RBF_CELL		rbf_cell;
  CONNECTION		temp_connect;
  RBF_CONNECT		rbf_connect;
  extern int		LOAD_MARKER, END_OF_BATCH_FILES;
  float			desired_out, out_act,
			error, sqerror, w_error, center_delta;
  int			count, ccount, index, icount;



  if (rbf_parameters_allocate == FALSE) {
    printf( "RBF parameters not allocated.  Turning off RBF learning.\n" );
    printf( "  ABORTing simulation.\n" );
    set_flag( LEARN_RBF, OFF );
    return ERROR;
  }

  if (count_cycles == 0) {

    fp_output = nxFopen( rbfBatchFilename );
    if (fp_output == NULL) {
      printf( "Can not open training batch file '%s';\n", rbfBatchFilename );
      printf( "  ABORTing simulation.  Please check 'Desired Output\n" );
      printf( "  File' parameter in RBF learning menu.\n" );
      return ERROR;
    }

    if (rh_head == NULL) {
      printf( "RBF Network for %s layer not created!", hhead->name );
      printf( "  Turning off RBF learning, ABORTing simulation.\n" );
      set_flag( LEARN_RBF, OFF );
      return ERROR;
    }
    if (ro_head == NULL) {
      printf( "RBF Network for %s layer not created!", ohead->name );
      printf( "  Turning off RBF learning, ABORTing simulation.\n" );
      set_flag( LEARN_RBF, OFF );
      return ERROR;
    }

    if (query_flag( SAVE_ERR_RBF )) {
      rbf_save_error_fp = fopen( rbfErrorFilename, "w" );
      if (rbf_save_error_fp == NULL) {
	printf( "Can not open RBF error file '%s'.\n", rbfErrorFilename );
	printf( "  Turning error saving off and continuing simulation.\n\n" );
	set_flag( SAVE_ERR_RBF, OFF );
      }
    }

  } /* count_cycle == 0 ... Pre-simulation initialization complete. */



  /* initializes the bp_error for the hidden network */

  for (count = 0; count < hhead->number_cells; count++)
    (rh_head->rbf_cells + count)->bp_error = 0.0;


  /*
   *  Read in the desired output cell activity and calculate the squared error.
   */

  printf( "RBF : Adjusting Network %s\n", ohead->name );
  if (query_flag( LOAD_RANDOM )) {
    rewind( fp_output );
    for (count = 0; count < LOAD_MARKER; count++)
      fscanf( fp_output, "%*[^\n]%*c" );
  }
  if (fscanf( fp_output, "%s", rbfTrainFilename ) == EOF) {
    rewind( fp_output );
    fscanf( fp_output, "%s", rbfTrainFilename );
  }

  fp_train = nxFopen( rbfTrainFilename );
  if (fp_train == NULL) {
    printf( "ERROR: Can not open training file '%s'.\n", rbfTrainFilename );
    printf( "  ABORTing simulation.\n\n" );
    return ERROR;
  }


  sqerror = 0.0;

  for(count=ohead->dim_y - 1;count>=0;count--)
    for(ccount=0;ccount<ohead->dim_x;ccount++) {
      index = count*ohead->dim_y + ccount;
      cell = ohead->cells + index;
      rbf_cell = ro_head->rbf_cells + index;
      fscanf(fp_train,"%f",&desired_out);
      out_act = cell->firing_rate;
      error = desired_out - out_act;
      sqerror += error * error;

      /* checks to see if the error exceeds the min change in the RBF menu */
      if((float)fabs(error)>rbfParaMinChange) {
	w_error = error * out_act * (1.0 - out_act);

	/*
	 *  Calculate the changes in the bias of the cells in the output
	 *    layer by using the weighted error as well as the weights
	 *    learning rate in the RBF menu
	 */

	if (query_flag( LEARN_BATCH )) {
	  rbf_cell->batch_bias += w_error;
	  if (END_OF_BATCH_FILES) {
	    rbf_cell->bias += rbfParaWeight * rbf_cell->batch_bias;
	    rbf_cell->batch_bias = (float) 0;
	  }
	}
	else
	  rbf_cell->bias += rbfParaWeight * w_error;

	for(icount=0;icount<cell->number_connections;icount++) {
	  temp_connect = cell->connect_list + icount;
	  rbf_connect = rbf_cell->connect_list + icount;

	  /*
	   *  Calculates the back propagated error for cells
	   *  in the hidden layer
	   */

	  if(temp_connect->input_cell->net_id == hhead->id)
	    (rh_head->rbf_cells + (temp_connect->input_cell->id - 1))->bp_error
	      += w_error * temp_connect->conductance;

	  /*
	   *  Change the weights leading to the output network using
	   *    the weights learning rate in the RBF menu as well as
	   *    a momentum term
	   */

	  if (query_flag( LEARN_BATCH )) {
	    rbf_connect->batch_conductance += w_error *
	      temp_connect->input_cell->firing_rate;
	    if (END_OF_BATCH_FILES) {
	      temp_connect->conductance +=
		(rbf_connect->momentum = rbfParaWeight *
		 rbf_connect->batch_conductance
		 + rbfParaMomentum * rbf_connect->momentum);
	      rbf_connect->batch_conductance = 0.0;
	    }
	  }
	  else
	    temp_connect->conductance += 
	      (rbf_connect->momentum =
	       rbfParaWeight * w_error * temp_connect->input_cell->firing_rate
	       + rbfParaMomentum * rbf_connect->momentum);
	} /* goes to the next connection in current cell */
      } /* end if error>rbfParaMinChange */
    } /* goes to the next cell */
  fclose(fp_train);


  /*
   *  Calculate the changes in the weights and bias
   *    of the cells in the hidden layer
   */

  printf("RBF : Adjusting Network %s\n",hhead->name);
  for(count=0;count<hhead->number_cells;count++) {
    rbf_cell = rh_head->rbf_cells + count;
    cell = hhead->cells + count;
    center_delta = rbf_cell->bp_error *
      (-rbf_cell->bias * exp(-rbf_cell->bias*cell->voltage));
    for(ccount=0;ccount<cell->number_connections;ccount++) {
      temp_connect = cell->connect_list + ccount;
      rbf_connect = rbf_cell->connect_list + ccount;

      if (query_flag( LEARN_BATCH )) {
	rbf_connect->batch_conductance +=
	  center_delta * (temp_connect->input_cell->firing_rate -
			  temp_connect->conductance);
	if (END_OF_BATCH_FILES) {
	  temp_connect->conductance +=
	    (rbf_connect->momentum =
	     (-rbfParaCenter) * rbf_connect->batch_conductance +
	     rbfParaMomentum * rbf_connect->momentum);
	  rbf_connect->batch_conductance = (float) 0;
	}
      }
      else
	temp_connect->conductance += 
	  (rbf_connect->momentum = (-rbfParaCenter) * center_delta *
	   (temp_connect->input_cell->firing_rate - temp_connect->conductance)
	   + rbfParaMomentum * rbf_connect->momentum);
    } /* goes to next connection */
    if (query_flag( LEARN_BATCH )) {
      rbf_cell->batch_bias +=
	rbf_cell->bp_error * (-cell->voltage *
			      exp(-rbf_cell->bias*cell->voltage));
      if (END_OF_BATCH_FILES) {
	rbf_cell->bias += rbfParaBias * rbf_cell->batch_bias;
	rbf_cell->batch_bias = (float) 0;
      }
    }
    else
      rbf_cell->bias +=
	rbfParaBias * rbf_cell->bp_error *
	  (-cell->voltage * exp(-rbf_cell->bias*cell->voltage));
  } /* goes to next hidden unit */

  if (END_OF_BATCH_FILES) {
    printf("Batch modifications made...\n");
    END_OF_BATCH_FILES = FALSE;
  }

  printf("Sum squared error = %f\n",sqerror);

  /*
   *  Write error to file.
   */
  if (query_flag( SAVE_ERR_RBF )) {
    fprintf( rbf_save_error_fp, "%f", sqerror );

    /* record load marker if load=random DW 94.08.24 */
    if (query_flag( LOAD_RANDOM ))
      fprintf( rbf_save_error_fp, " %d", LOAD_MARKER + 1 );

    fprintf( rbf_save_error_fp, "\n" );
  }

  printf("RBF : Done\n");


  /*
   *  After last cycle, clean up.
   */
  if (count_cycles == cycles - 1) {
    fclose( fp_output );
    if (query_flag( SAVE_ERR_RBF ))
      fclose( rbf_save_error_fp );
  }

  return OK;
}  /* run_rbf() */








/* construct linked list for rbf */

char *
rbfAllocParam( )
{
  CELL			cell;
  FILE			*ofp, *outfp;
  MATRIX		G,Go,RBF_Matrix,c,y;
  NETWORK		head;
  RBF_CELL		rcell;
  RBF_NETWORK		rbf_head = NULL,
  			old_rbf_head;
  SPECS			specs;
  char			a_filename[NAME_SIZE],
  			out_file[NAME_SIZE];
  float			input, norm_input;
  int			count, icount, ccount, hcount,
  			num_hcells, num_icells, num_ocells,
  			file_length;

  extern NETWORK	get_network_id();



  if (rbf_parameters_allocate) {
    if (!rbfErrorQuery( "RBF networks already initialized.  Reinitialize?" ))
      return "Aborting reinitialization.";
    fprintf( stderr, "Reinitializing RBF networks..." );

    rbfFreeParam( );
  }
  else
    fprintf( stderr, "Initializing RBF parameters..." );

  if (strlen( rbfInputNetwork ) == 0)
    return "Input Network is not specified.";
  if (strlen( rbfHiddenNetwork ) == 0)
    return "Hidden Network is not specified.";
  if (strlen( rbfOutputNetwork ) == 0)
    return "Output Network is not specified.";
  if (strlen( filename_load_activity ) == 0)
    return "Activity File(s) is (are) not specified.";
  if (strlen( rbfBatchFilename ) == 0)
    return "Training Outputs Batch File is not specified.";

  /*
   *  Find input network.
   */

  if (!(ihead = network_head))
    return "No networks have been loaded.";
  while (ihead && strcmp( ihead->name, rbfInputNetwork ))
    ihead = ihead->next;
  if (!ihead)
    return "The Input Network specified does not exist.";
  num_icells = ihead->number_cells;

  /*
   *  Find hidden network.
   */

  hhead = network_head;
  while (hhead && strcmp( hhead->name, rbfHiddenNetwork ))
    hhead = hhead->next;
  if (!hhead)
    return "The Hidden Network specified does not exist.";

  old_rbf_head = rbf_head;
  rbf_head = rbfAllocNetwork();
  rbf_head->id = hhead->id;
  rbf_head->rbf_cells = rbfAllocCells(hhead->number_cells);
  for(count = 0;count < hhead->number_cells; count++) {
    rcell = rbf_head->rbf_cells + count;
    rcell->id = (hhead->cells + count)->id;
    rcell->connect_list =
      rbfAllocConnections( (hhead->cells + count)->number_connections );
    rcell->cell_ptr = (hhead->cells + count);
  }
  rbf_head->net_ptr = hhead;
  rbf_head->next = old_rbf_head;
  rh_head = rbf_head;
  num_hcells = hhead->number_cells;

  /*
   *  Find output network.
   */
  
  ohead = network_head;
  while (ohead && strcmp( ohead->name, rbfOutputNetwork ))
    ohead = ohead->next;
  if (!ohead)
    return "The Output Network specified does not exist.";

  old_rbf_head = rbf_head;
  rbf_head = rbfAllocNetwork();
  rbf_head->id = ohead->id;
  rbf_head->rbf_cells = rbfAllocCells(ohead->number_cells);
  for(count = 0;count < ohead->number_cells; count++) {
    rcell = rbf_head->rbf_cells + count;
    rcell->id = (ohead->cells + count)->id;
    rcell->connect_list =
      rbfAllocConnections( (ohead->cells + count)->number_connections );
    rcell->cell_ptr = (ohead->cells + count);
  }
  rbf_head->net_ptr = ohead;
  rbf_head->next = old_rbf_head;
  ro_head = rbf_head;
  num_ocells = ohead->number_cells;



  /*
   *  Check to make sure Input and Hidden networks are connected.
   */
  
  specs = ihead->cells->specs;
  while (specs && strcmp( rbfHiddenNetwork,
			 get_network_id( specs->id )->name ))
    specs = specs->next;
  if (!specs) {
    specs = hhead->cells->specs;
      while (specs && strcmp( rbfInputNetwork,
			     get_network_id( specs->id )->name ))
	specs = specs->next;
  }
  if (!specs)
    return "The Input and Hidden networks must be connected.";

  /*
   *  Check to make sure Hidden and Output networks are connected.
   */

  specs = ohead->cells->specs;
  while (specs && strcmp( rbfHiddenNetwork,
			 get_network_id( specs->id )->name ))
    specs = specs->next;
  if (!specs) {
    specs = hhead->cells->specs;
    while (specs && strcmp( rbfOutputNetwork,
			   get_network_id( specs->id )->name ))
      specs = specs->next;
  }
  if (!specs)
    return "The Hidden and Output networks must be connected.";




  /* compares number of training patterns to number of hidden units      */
  /* distributes training patterns evenly as centers among hidden units  */
  /* loads training pattern, propagates the activity to the hidden units */
  /* and stores it as a center by assigning the activity to the weights  */

  if (readBatchFile( NXBATCH_IN, filename_load_activity ))
    return "Can not open Training Inputs Batch File";
  file_length = currBatchListLength[NXBATCH_IN];

  if (file_length <= 0)
    return "Training file has length of 0.";
  if (file_length < num_hcells)
    return "# of Hidden Units is greater than # of training patterns.";

  printf( "Setting Centers...\n" );

  icount = 1;

  for (count = 0; count < num_hcells; count++) {

    if (getBatchActivityFile( NXBATCH_IN, icount - 1,
			     network_load_activity, a_filename ))
      return "Problem accessing activity file in RBF initialization.\n";

    printf( "   Center %i: %s\n", count, a_filename );

    /*
     *  Propagate input activities through network, up to hidden layer.
     */
    head = network_head;
    while (head != hhead) {

      for (ccount = 0; ccount < head->number_cells; ccount++)
	/* '!= CLAMP_SELECTIVE_ON' -> '== CLAMP_OFF'; DW 94.08.30 */
	if ((head->cells + ccount)->clamp == CLAMP_OFF)
	  update_cell_activity( head->cells + ccount, head->class, head, OFF );

      head = head->next;
    }

    /*
     *  Reached hidden layer.
     */
    for (ccount = 0; ccount < num_icells; ccount++)
      ((hhead->cells + count)->connect_list + ccount)->conductance =
	(ihead->cells + abs(num_icells - ccount - 1))->firing_rate;

    if (count != num_hcells - 1) {
      for (icount++;
	   icount < ((file_length - 1) * (count + 1) / (num_hcells - 1)) + 1;
	   icount++);
    }
  }
  
  /*  Initializes the y matrix to the desired activity of the output layer
   *    since transfer function of the output layer is rbf_logistic, the
   *    desired output is linearized between -4 and 4
   */
  
  printf( "Reading in desired output...\n" );
  y = (MATRIX) makeMatrix( file_length, num_ocells );
  ofp = nxFopen( rbfBatchFilename );
  if (ofp == NULL)
    return "Error opening output file.";

  for (count = 0; count < file_length; count++) {
    fscanf( ofp, "%s", out_file );
    outfp = nxFopen( out_file );
    if (outfp == NULL) {
      printf("Error in output file: %s\n",out_file);
      return "Error - No such file";
    }
    load_activity_file( outfp, ohead );
    for (icount = 0; icount < num_ocells; icount++)
      y->data[count][icount] = (ohead->cells + icount)->firing_rate * 8 - 4;
    fclose( outfp );
  }
  fclose(ofp);

  /* set bias of all hidden units to initialization bias */
  for (count = 0; count < num_hcells; count++)
    (rh_head->rbf_cells + count)->bias = rbfInitBias;

  /* create matrices for G and Go (actually rbfInitSmooth * Go) */
  /* extra space is created for calculation of the output bias */

  G = (MATRIX) makeMatrix( file_length, num_hcells + 1 );
  Go = (MATRIX) makeMatrix( num_hcells + 1, num_hcells + 1 );

  /* simulate for each of the learning patterns   */
  /* store output of hidden units in matrix G     */
  /* store weight_differences*rbfInitSmooth in matrix Go */
  
  printf( "Calculating weights...\n" );

  hcount = 0;
  for (count = 0; count < file_length; count++) {

    if (getBatchActivityFile( NXBATCH_IN, count,
			     network_load_activity, a_filename ))
      return "Problem accessing activity file in RBF initialization.\n";

    printf( "Loading %s...\n", a_filename );

    head = network_head;
    while (head != hhead) {
      printf( "Updating Network %s...\n", head->name );
      for (icount = 0; icount < head->number_cells; icount++)
	/* '!= CLAMP_SELECTIVE_ON' -> '== CLAMP_OFF'; DW 94.08.30 */
	if ((head->cells + icount)->clamp == CLAMP_OFF)
	  update_cell_activity( head->cells + icount,
			       head->class, head, OFF );
      head = head->next;
    }

    printf( "Updating Network %s...\n", head->name );
    for (icount = 0; icount < num_hcells; icount++) {
      cell = (hhead->cells) + icount;
      for (ccount = 0, norm_input = 0;
	   ccount < (cell->number_connections);
	   ccount++) {
	input = (cell->connect_list + ccount)->input_cell->firing_rate 
	  - (cell->connect_list + ccount)->conductance;
	norm_input += input * input;
      }
      cell->firing_rate = (float) exp(-rbfInitBias*norm_input);
      display_cell_activity( hhead, cell->id );
      G->data[count][icount] = cell->firing_rate;
      if(count==(int)(((file_length-1)*hcount)/(num_hcells-1))) {
	Go->data[hcount][icount] = G->data[count][icount]*rbfInitSmooth;
	if(icount==num_hcells-1) hcount++;
      }
    }
  }

  /* fill the rest of G and Go to calculate output bias */

  for(count=0;count<file_length;count++)
    G->data[count][num_hcells] = (float) 1;

  for(count=0;count<num_hcells+1;count++)
    Go->data[count][num_hcells] = rbfInitSmooth;
  for(count=0;count<num_hcells;count++)
    Go->data[num_hcells][count] = rbfInitSmooth;

  /* to set weights between hidden and output layer */
  /* calculate hidden-output weights through matrix calculations */
  /* c = Inv(Gt.G + Go).Gt.y */
  /* last row of matrix c contains the bias of the output cells */
  
  printf( "Calculating matrices...\n" );
  RBF_Matrix = MMultiply(
			 MInverse(
				  MAdd(
				       MMultiply(MTranspose(G),G),
				       Go)
				  ),
			 MTranspose(G)
			 );
  c = MMultiply(RBF_Matrix,y);

  printf( "Creating connections...\n" );
  for (ccount = 0; ccount < num_ocells; ccount++) {

    for (count = 0; count < num_hcells; count++)
      ((ohead->cells + ccount)->connect_list + count)->conductance = 
	c->data[(((ohead->cells + ccount)->connect_list
		  + count)->input_cell->id - 1)][ccount];

    (ro_head->rbf_cells + ccount)->bias = c->data[num_hcells][ccount];
    printf( "Set output bias to %f...\n",
	   (ro_head->rbf_cells + ccount)->bias );
  }

  free( c ); free( RBF_Matrix ); free( y ); free( G ); free( Go );

  /* assigns rbf network head to extern variable */
  rbf_network_head = rbf_head;
  rbf_parameters_allocate = TRUE; /* set rbf check flag */

  printf( "Done allocating RBF parameters.\n" );

  return NULL;
}  /* rbfAllocParam( ) */



#if defined(NEXUS_SGI) || defined(NEXUS_LINUX)
/*
 * TkNx_RBFInitialize
 */
COMMAND_PROC(TkNx_RBFAllocParam)
{
  extern int		display_type;
  int			temp_display_type;
  char			buff[1024];
  char			*result;


  TCL_CHK( "TkNx_RBFAllocParam", argc, 0, "" );

  temp_display_type = display_type;
  display_type = OFF;

  if ((result = rbfAllocParam( ))) {
    sprintf( buff, "error {in rbfAllocParam(): %s}", result );
    Tcl_AppendResult( interp, buff, NULL );
  }

  display_type = temp_display_type;

  return TCL_OK;
}



/*
 *  Returns TRUE if user answers YES to the query.  DW 94.11.09
 */

int
rbfErrorQuery( char *string )
{
  return queryDialog( string );
}
#endif /* defined(NEXUS_SGI) || defined(NEXUS_LINUX) */


#ifdef NEXUS_SUN
void
create_rbf( item, event )
    Panel_item	item;
    Event	*event;
{
  extern int		display_type;
  Frame			disabled_frames[1];
  Panel			panel = (Panel) xv_get( item, PANEL_PARENT_PANEL );
  char			*result;
  int			temp_display_type;

  extern char		*rbfAllocParam( );



  temp_display_type = display_type;
  display_type = OFF;
  disabled_frames[0] = NULL;
  disable_frames( disabled_frames );

  if (result = rbfAllocParam( )) {
    notice_prompt( rbf_panel, NULL,
		  NOTICE_MESSAGE_STRINGS,	result, NULL,
		  NULL );
    xv_set( item,
	   PANEL_NOTIFY_STATUS, XV_ERROR,
	   NULL );
  }

  display_type = temp_display_type;
  enable_frames( disabled_frames );
}



/*****************************************************************************
 *  Set up the RBF Learning window.
 *****************************************************************************/

int
setup_menu_rbf( top_frame )
    Frame top_frame;
{
  extern char		rbfBatchFilename[],
  			rbfErrorFilename[],
			rbfInputNetwork[],
			rbfHiddenNetwork[],
			rbfOutputNetwork[];
  extern float		rbfInitBias,
			rbfInitSmooth,
			rbfParaCenter,
			rbfParaBias,
			rbfParaWeight,
			rbfParaMinChange,
			rbfParaMomentum;

  Panel_item		temp_text;
  char			*temp_str = "00000000000000000000";

  Panel			rbf_panel2;


  
  rbf_frame = (Frame)
    xv_create( top_frame,		FRAME,              
	      FRAME_LABEL,		"Radial Basis Functions",
	      XV_WIDTH,			1200,
	      XV_HEIGHT,		1000,
	      NULL );
  rbf_panel = (Panel)
    xv_create( rbf_frame,		PANEL,
	      PANEL_LAYOUT,		PANEL_HORIZONTAL,
	      OPENWIN_SHOW_BORDERS,	TRUE,
	      NULL);
  
  chbox_rbf = (Panel_item)
    xv_create( rbf_panel,		PANEL_CHECK_BOX,
	      PANEL_CHOOSE_ONE,		TRUE,
	      PANEL_LABEL_STRING,	"RBF Learning",
	      PANEL_CHOICE_STRINGS,	"OFF", "ON", NULL,
	      PANEL_NOTIFY_PROC,	proc_chbox_binary,
	      PANEL_CLIENT_DATA,	LEARN_RBF,
	      PANEL_VALUE,		query_flag( LEARN_RBF ),
	      NULL );

  temp_text = (Panel_item)
    xv_create( rbf_panel,
	      PANEL_NAME_TEXT( rbfInputNetwork ),
	      PANEL_NEWLINE,			20,
	      PANEL_LABEL_STRING,		"Input Network: ",
	      PANEL_VALUE,			"",
	      XV_KEY_DATA,			KEY_UPDATE_LIST, TRUE,
	      NULL );
  (void) xv_create( rbf_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );
  rbf_network_input_list = (Panel_item)
    xv_create( rbf_panel,
	      PANEL_NETWORK_LIST( temp_text, rbfInputNetwork ),
	      PANEL_NEWLINE,			10,
	      NULL );
  xv_set( temp_text,
	 XV_KEY_DATA, KEY_MY_LIST, rbf_network_input_list,
	 NULL );

  temp_text = (Panel_item)
    xv_create( rbf_panel,
	      PANEL_NAME_TEXT( rbfHiddenNetwork ),
	      PANEL_NEWLINE,			20,
	      PANEL_LABEL_STRING,		"Hidden Network: ",
	      PANEL_VALUE,			"",
	      XV_KEY_DATA,			KEY_UPDATE_LIST, TRUE,
	      NULL );
  (void) xv_create( rbf_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );
  rbf_network_hidden_list = (Panel_item)
    xv_create( rbf_panel,
	      PANEL_NETWORK_LIST( temp_text, rbfHiddenNetwork ),
	      PANEL_NEWLINE,			10,
	      NULL );
  xv_set( temp_text,
	 XV_KEY_DATA, KEY_MY_LIST, rbf_network_hidden_list,
	 NULL );

  temp_text = (Panel_item)
    xv_create( rbf_panel,
	      PANEL_NAME_TEXT( rbfOutputNetwork ),
	      PANEL_NEWLINE,			20,
	      PANEL_LABEL_STRING,		"Output Network: ",
	      PANEL_VALUE,			"",
	      XV_KEY_DATA,			KEY_UPDATE_LIST, TRUE,
	      NULL );
  (void) xv_create( rbf_panel,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );
  rbf_network_output_list = (Panel_item)
    xv_create( rbf_panel,
	      PANEL_NETWORK_LIST( temp_text, rbfOutputNetwork ),
	      PANEL_NEWLINE,			10,
	      NULL );
  xv_set( temp_text,
	 XV_KEY_DATA, KEY_MY_LIST, rbf_network_output_list,
	 NULL );

  window_fit( rbf_panel );

  rbf_panel2 = (Panel)
    xv_create( rbf_frame,		PANEL,
	      PANEL_LAYOUT,		PANEL_HORIZONTAL,
	      XV_X,			get_rect_right( rbf_panel ),
	      XV_Y,			get_rect_top( rbf_panel ),
	      OPENWIN_SHOW_BORDERS,	TRUE,
	      NULL);

  (void) xv_create( rbf_panel2,
		   PANEL_NAME_TEXT( rbfBatchFilename ),
		   XV_Y,			40,
		   PANEL_LABEL_STRING,		"Training Batch File: ",
		   PANEL_VALUE,			"",
		   NULL );
  (void) xv_create( rbf_panel2,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  sprintf( temp_str, "%9.3f", rbfInitBias );
  (void) xv_create( rbf_panel2,			PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Initialization Bias: ",
		   PANEL_NEWLINE,		20,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&rbfInitBias,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( rbf_panel2,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  sprintf( temp_str, "%9.3f", rbfInitSmooth );
  (void) xv_create( rbf_panel2,			PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Initialization Smoothness: ",
		   PANEL_NEWLINE,		10,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&rbfInitSmooth,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( rbf_panel2,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  (void) xv_create( rbf_panel2,			PANEL_MESSAGE,
		   PANEL_LABEL_STRING,		"Learning Rates...",
		   PANEL_NEWLINE,		50,
		   XV_X,			0,
		   PANEL_LABEL_BOLD,		TRUE,
		   NULL );

  sprintf( temp_str, "%9.3f", rbfParaCenter );
  (void) xv_create( rbf_panel2,			PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Centers: ",
		   PANEL_NEWLINE,		10,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&rbfParaCenter,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( rbf_panel2,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  sprintf( temp_str, "%9.3f", rbfParaBias );
  (void) xv_create( rbf_panel2,			PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Bias: ",
		   PANEL_NEWLINE,		10,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&rbfParaBias,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( rbf_panel2,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  sprintf( temp_str, "%9.3f", rbfParaWeight );
  (void) xv_create( rbf_panel2,			PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Weights: ",
		   PANEL_NEWLINE,		10,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&rbfParaWeight,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( rbf_panel2,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  sprintf( temp_str, "%9.3f", rbfParaMinChange );
  (void) xv_create( rbf_panel2,			PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Minimum Change: ",
		   PANEL_NEWLINE,		10,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&rbfParaMinChange,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( rbf_panel2,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  sprintf( temp_str, "%9.3f", rbfParaMomentum );
  (void) xv_create( rbf_panel2,			PANEL_TEXT,
		   PANEL_LABEL_STRING,		"Momentum: ",
		   PANEL_NEWLINE,		10,
		   PANEL_NOTIFY_PROC,		proc_enter_float,
		   PANEL_CLIENT_DATA,		&rbfParaMomentum,
		   PANEL_VALUE,			temp_str,
		   PANEL_VALUE_DISPLAY_LENGTH,	9,
		   NULL );
  (void) xv_create( rbf_panel2,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  chbox_save_err_rbf = (Panel_item)
    xv_create( rbf_panel2,			PANEL_CHECK_BOX,
	      PANEL_NEWLINE,			40,
	      XV_X,				0,
	      PANEL_CHOOSE_ONE,			TRUE,
	      PANEL_LABEL_STRING,		"Save Error: ",
	      PANEL_CHOICE_STRINGS,		"OFF", "ON", NULL,
	      PANEL_NOTIFY_PROC,		proc_chbox_binary,
	      PANEL_CLIENT_DATA,		SAVE_ERR_RBF,
	      PANEL_VALUE,			query_flag( SAVE_ERR_RBF ),
	      NULL );

  (void) xv_create( rbf_panel2,
		   PANEL_NAME_TEXT( rbfErrorFilename ),
		   PANEL_NEWLINE,		20,
		   PANEL_LABEL_STRING,		"Error Filename: ",
		   PANEL_VALUE,			"",
		   NULL );
  (void) xv_create( rbf_panel2,			PANEL_MESSAGE,
		   PANEL_LABEL_IMAGE,		carriage_return_image,
		   NULL );

  (void) xv_create( rbf_panel2,			PANEL_BUTTON,
		   PANEL_NEWLINE,		40,
		   PANEL_LABEL_STRING,		"Initialize",
		   PANEL_NOTIFY_PROC,		create_rbf,
		   NULL );

  (void) xv_create( rbf_panel2,			PANEL_BUTTON,
		   PANEL_LABEL_STRING,		"Cancel",
		   PANEL_NOTIFY_PROC,		proc_btn_cancel,
		   PANEL_CLIENT_DATA,		rbf_frame,
		   NULL );

  window_fit( rbf_panel2 );
  xv_set( rbf_panel2,
	 XV_HEIGHT, (int) xv_get( rbf_panel, XV_HEIGHT ),
	 NULL );
  window_fit( rbf_frame );
}  /* setup_menu_rbf() */



void
go_rbf( item )
    Panel_item item;
{
  extern get_network_names_list( );


  get_network_names_list( rbf_network_input_list );
  get_network_names_list( rbf_network_hidden_list );
  get_network_names_list( rbf_network_output_list );
  xv_set( rbf_frame,
	 XV_SHOW, TRUE,
	 NULL );
}



void
rbfErrorMsg( string )
    char *string;
{
  notice_prompt( rbf_panel, NULL,
		NOTICE_MESSAGE_STRINGS,	string,	NULL,
		NULL );
}



/*
 *  Returns TRUE if user answers YES to the query.  DW 94.11.07
 */

int
rbfErrorQuery( string )
    char *string;
{
  return ((notice_prompt( rbf_panel, NULL,
			NOTICE_MESSAGE_STRINGS, string,	NULL,
			NOTICE_BUTTON_YES,	"Yes",
			NOTICE_BUTTON_NO,	"No",
			NULL )) == NOTICE_YES);
}
#endif








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
