/*************************************************************
*                                                            *
*                            NEXUS                           *
*                                                            *
*                    (nexus_extern_functions.c)              *
*                                                            *
*     		     10/28/91 by K.Sakai                     *
**************************************************************/

/* extern_functions.c includes:
 *				extern_get_file(),
 *				time_out_error(),
 *				extern_put_file(),
 *				file_complete_check(),
 *				file_update_check(),
 *				get_last_file(),
 *				time_now(),
 *				init_extern_connections().
 */

/*	extern_get_file()
 *
 *	RETURN VALUE: 	option	return
 *			OK	-w,p file (new or used) read completed.
 *				-g new file read, or not read. 
 *			ERROR	-w time out; new file not ready or not exist.
 *				-p old file(s) not ready or not exist, or
 *				   time out of the first file. 
 *				
 *	NOTE: 	-wN ,  1<= N <=600  otherwise N is set to N.
 *			default is wDEFAULT_TIMEOUT, set in nexus_lex_build.l,
 *			and is used if option is not apparently defined.
 */

#include	"nexus.h"
#include	"extern_functions.h"
#include	<fcntl.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<unistd.h>
#include	<stdlib.h>

int file_complete_check(char *file_name, char *directory);
int time_out_error(NETWORK net_head, char *file_name);
int file_update_check(NETWORK net_head, char *file_name);

int
extern_get_file( net_head )
    NETWORK net_head;
{
  int		fdr;		/* file discripter for open() */
  int		file_buff_size; /* file buffer size in byte */
/*  int		file_buff_numb;	*/ /*   # of data in file buffer. */
  float		file_buf[FILE_BUFSIZE];
  char		file_name[NAME_SIZE];
/*  int 		nbyte; */
  int		int_size;	/* integer size in byte */
  int 		number_data;
  int		read_repeat;	/* # of times to be repeated for read */
  int		wait_time=60;	/* -wN , wait time N */
  float		time_now(),
		begin_time,
		now_time;
/*  int		file_compl_status; */
  int		cell_count;
/*  int		number_rest; */
  char		curr_dir[NAME_SIZE];
  int 		i;
  
  char 		option;		/* option for file not updated.		*/
  char 		*ext_net_name;
  char 		*directory;
  int		extern_read_times;
  CELL		cell_head;
  CELL		cell_write_head;	
	


  file_buff_size = FILE_BUFSIZE * FLOAT_SIZE;	/* 256 * 4 = 1024	 */
/*  file_buff_numb = FILE_BUFSIZE; */
  int_size = INT_SIZE;

  /* open the file and check completion.	*/
  ext_net_name = (net_head->extern_connections)->extern_from_net;
  directory = (net_head->extern_connections)->extern_from_dir;
  extern_read_times = (net_head->extern_connections)->extern_from_times;
  cell_head = net_head->cells;
  sprintf (file_name,"%s.%d", ext_net_name, (extern_read_times));   

  if (getcwd (curr_dir, sizeof(curr_dir)) == NULL) {
    fprintf(stderr, "getcwd failed.\n");
    exit(1);
  }
  chdir (directory);

  if ( (fdr = open(file_name, O_RDONLY)) != -1) {
    /* file EXIST.	*/
    if (file_complete_check( file_name, directory ) == TRUE) {
      /* file updated, READY for read	*/
      /* get length of the data, by reading the first 4 byte of int */
      if ( read( fdr, &number_data, int_size ) != 4 )
	fprintf(stderr,"extern_get_file(): %s read error. \n", file_name);
      read_repeat = number_data / FILE_BUFSIZE;

      /* READ data from the file 		*/
      for (i=0; i<read_repeat; i++) {
	read(fdr, file_buf, file_buff_size);
	/* write to firing_rate	*/
	cell_write_head = cell_head + i*FILE_BUFSIZE;
	for (cell_count = 0; cell_count < FILE_BUFSIZE; cell_count++) 
	  (cell_write_head + cell_count)->firing_rate = *(file_buf + cell_count);
      }
      /* read out the rest	*/
      read( fdr, file_buf, file_buff_size );		
      cell_write_head = cell_head + read_repeat*FILE_BUFSIZE;
      for (cell_count = 0;
	   cell_count < number_data - read_repeat * FILE_BUFSIZE;
	   cell_count++)
	(cell_write_head + cell_count)->firing_rate = *(file_buf + cell_count);
      close(fdr);
      chdir (curr_dir);
      return(OK);
    }
    close(fdr);
  }

  /* file NOT ready 			*/
  /* fprintf(stderr," file NOT ready\n"); */
  begin_time = time_now();

  /*
   *  we are now guaranteed an option (which may be incorrect) and a wait_time:
   *  see nexus_lex_build.l for details (search for extern_from_opt).
   *  DW 94.07.07
   */
  sscanf ((net_head->extern_connections)->extern_from_opt, "%c %d", &option, &wait_time);

  /*
   * FIXED: order of cases.  default moved to bottom like it should be.
   * DW 94.07.07
   */
  switch (option) {
  case 'p':
    if ( extern_read_times > 1) {
      /* look for previously used file , change file_name	 */
      sprintf(file_name, "%s.%d",ext_net_name, (extern_read_times - 1));

      /* try to open used file, decrement extension until success  */
      while( (fdr = open(file_name, O_RDONLY)) == -1) {

	/* decrement file_name extension */
	if ( (extern_read_times-- ) == 0 ) {
	  fprintf(stderr," network %s: old file %s not found.\n", net_head->name, file_name);
	  chdir (curr_dir);
	  return(ERROR);
	}
	sprintf(file_name, "%s.%d",ext_net_name, extern_read_times);
      }
      close( fdr );
    }

    /* used file EXIST or looking for FIRST file, loop until check file completion is OK or timeout	*/
    while (file_complete_check( file_name, directory ) <= 0) {
      if (((now_time = time_now()) - begin_time) > wait_time) {
	/* time out ; check status , issue & print error and return. */
	time_out_error( net_head, file_name );
	chdir( curr_dir );
	return (ERROR);
      }	}
    /* previously used file ready, file_name was changed to used one.	*/
    fprintf( stderr," Older(or first) file is used: external comm. file %s (-p)\n",file_name);
    break;

  case 'g':
    /* return w/o read	*/
    fprintf( stderr," Skiped extern comm. file: %s  (-g)\n", file_name );
    chdir( curr_dir );
    return( OK );

  default:
    /* assuming w, illegal option, or no option found in nx file */
    /* try to open until success or timeout	 */
    while ( (fdr = open(file_name, O_RDONLY)) == -1) {
      /* check time out	 */
      if (((now_time = time_now()) - begin_time) > wait_time) {
	/* time out ; check status , issue & print error and return. */
	time_out_error(net_head, file_name);
	chdir(curr_dir);
	return (ERROR);
      }
    }
    close(fdr);

    /* new file EXIST , check file completion until OK or timeout	*/
    while (file_complete_check( file_name, directory ) <= 0) {
      if (((now_time = time_now()) - begin_time) > wait_time) {
	/* time out ; check status , issue & print error and return. */
	time_out_error(net_head, file_name);
	chdir(curr_dir);
	return (ERROR);
      }	}
    fprintf( stderr," Waited for external comm. file %s (-w) for %fsec\n",file_name,(now_time - begin_time));
    /* new file ready	*/
    break;

  }	

  /*  file READY, read			*/		
  if ((fdr = open(file_name, O_RDONLY)) == -1) {
    fprintf(stderr," network %s: ERROR %s can not open.\n",net_head->name, file_name);
    chdir(curr_dir);
    return (ERROR);
  }
  /* get length of the data, by reading the first 4 byte of int */
  read(fdr, &number_data, int_size);
  read_repeat = number_data / FILE_BUFSIZE;

  /* READ data from the file 		 */
  for (i = 0; i < read_repeat; i++) {
    read(fdr, file_buf, file_buff_size);
    cell_write_head = cell_head + i*FILE_BUFSIZE;
    for (cell_count = 0; cell_count < FILE_BUFSIZE; cell_count++)
      (cell_write_head + cell_count)->firing_rate = *(file_buf + cell_count);
  }
  read(fdr, file_buf, file_buff_size);
  cell_write_head = cell_head + read_repeat*FILE_BUFSIZE;
  for (cell_count = 0;
       cell_count < number_data - read_repeat * FILE_BUFSIZE;
       cell_count++)
    (cell_write_head + cell_count)->firing_rate = *(file_buf + cell_count);

  close(fdr);
  chdir(curr_dir);
  return (OK);
}



/* time out ; check file status , issue & print error. */
int
time_out_error( net_head, file_name )
    NETWORK net_head;
    char * file_name;
{
  int 	file_update_status;

  file_update_status = file_update_check(net_head, file_name);
  fprintf (stderr, " network %s: file %s read timeout,", net_head->name, file_name);
  if ( file_update_status == NOT_EXIST ) fprintf(stderr," not exist.\n");
  if ( file_update_status == FALSE ) fprintf(stderr, " file size inconsistent. (not completed?)\n");

  return(file_update_status);
}
 
 
 
/* extern_put_file.c	
*	RETURN VALUE:
*	
*	
*
*
* #include	<fcntl.h>
* #include	<sys/types.h>
* #include	<sys/stat.h>
* #include	<sys/param.h>
*
* #define 	FILE_BUFSIZE	256
*
* #define	INT_SIZE	4
* #define	FLOAT_SIZE	4
*/

void
extern_put_file( net_head )
    NETWORK net_head;
{
  char 	ext_file_name[NAME_SIZE];		/* file name to be write */
  int 	fdw;					/* file descripter for r,w */
/*  char 	buf[FILE_BUFSIZE];	*/ /*	buffer size for write */
  char	curr_dir[100];
/*  float 	time_now(),
		open_time; */
  float	file_buff[FILE_BUFSIZE];
  int	file_buff_size;
  int 	buff_count;
/*  int	nbyte; */
  int 	cell_count;
  CELL	cell_head;

  file_buff_size = FILE_BUFSIZE * FLOAT_SIZE;
	
  /* change directory */
/*  open_time = time_now( ); */
  if (getcwd (curr_dir, sizeof(curr_dir)) == NULL) {
    fprintf(stderr, "getcwd failed.\n");
    exit(1);
  }
  chdir( (net_head->extern_connections)->extern_to_dir );
	
  /* write a file	*/					/* filename to be write; netname.#	*/
  sprintf(ext_file_name,"%s.%d",net_head->name,(net_head->extern_connections)->extern_to_times); 	
  if ( (fdw = open( ext_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644 )) == -1) {    
    /* open failed		*/
    fprintf (stderr,"extern_put_file(): CANNOT open %s for write. \n",ext_file_name); 
  } else {	
    /* open succeded, write to the file.	*/
    /* fprintf(stderr,"OPEN %s successfully for writing. \n",ext_file_name); */
    write(fdw, &net_head->number_cells, 4);
    cell_head = net_head->cells;
    buff_count = 0;
    for (cell_count=0; cell_count < net_head->number_cells; cell_count++) {
      file_buff[buff_count]= (cell_head + cell_count)->firing_rate;
      buff_count++;
      if ( buff_count == FILE_BUFSIZE ) {	
	write( fdw, file_buff, file_buff_size);
	buff_count=0;
      }
    }
    write( fdw, file_buff, (buff_count * FLOAT_SIZE) );		/* wipe out the rest	*/
    write( fdw, "\n", 2 );
  }
  close(fdw);
  
  chdir(curr_dir);		
  /* fprintf(stderr, "Time took for extern_put_file: %f (sec) \n",
     (time_now() - open_time) ); */
}


/*	file_complete_check()	
 *
 *	RETURN VALUES:	TRUE 	actual file length = expected file length.
 *			FALSE	file exist but, actual file length is not
 *				expected file length.
 *			ERROR	failed to open the file or
 *				can not access file status.
 *
 *	Function: 	Open the file and read the expected length of the file.
 *			Compare the expected and actual length.
 *			
 * #include	<fcntl.h>
 * #include	<sys/types.h>
 * #include	<sys/stat.h>
 */

int
file_complete_check(file_name, directory) 
    char 	*file_name;
    char 	*directory;
{
  struct stat     buf;			/* for inode info. */
  int             fdr;			/* file discripter for open() */
  int             file_buff_size;	/* file buffer size in byte */
/*  float           file_buf[FILE_BUFSIZE]; */
  int             write_length;		/* length written */
  int             file_length;		/* current actual file length */
/*  int 		nbyte; */
  int 		number_data;
/*  char		curr_dir[NAME_SIZE]; */
  
  /* getwd (curr_dir);  This routine supports different directory,
     but not used. */
  /* chdir (directory); */
  
  if ((fdr = open(file_name, O_RDONLY)) == -1) {
    /* fprintf(stderr, "file_compl_check(): CANNOT open %s. \n", file_name); */
    /* chdir (curr_dir); */
    return(ERROR);
  } else {
    /* fprintf (stderr,"OPEN %s succesfully. \n",file_name); */
    if (fstat(fdr, &buf) == -1) {
      /* fprintf (stderr,"file_compl_check():  %s open but IRREGULAR contents.\n",file_name); */
      /* chdir (curr_dir); */
      return(ERROR);
    } else {
      file_length = buf.st_size;
      file_buff_size = INT_SIZE;
      read(fdr, &number_data, file_buff_size); /* read the first 4 byte of int */
      write_length = (number_data * 4) + 6;		 /* expected length of the file  */
    }
    close(fdr);
  }
  
  /* chdir (curr_dir); */	
  /* printf("%s: expected %d   actual %d  (byte)\n",file_name, write_length, file_length); */
  if (write_length == file_length) {
    return (TRUE);
  } else {
    return (FALSE);
  }
}


/*	file_update_check()	
*	
*	RETURN VALUES:	TRUE		file ready.
*			FALSE		file exist but not completed (writing?).
*			NOT_EXIST	file not exist, or
*					file not updated.
*	NOTE:	If file extension is 0, return(NOT_EXIST).
*
*	Subroutines: get_last_file(), file_complete_check() .
*	Function:    check existance and completion of the latest file.
*	
* #include	<fcntl.h>
* #include	<sys/types.h>
* #include	<sys/stat.h>
* #include	<sys/param.h>
*
* #define 	FILE_BUFSIZE	256
* #define		INT_SIZE	4
* #define		FLOAT_SIZE	4
*/

int
file_update_check(net_head,file_name) 
    NETWORK net_head;
    char 	*file_name;
{
  char	*directory;
  char	*ext_net_name;
  int	last_file_ext, 
  get_last_file();
  int 	file_compl_status,
  file_complete_check();
/*  int 	last_read; */
  /* char	directory[NAME_SIZE];   error */
  
  ext_net_name = (net_head->extern_connections)->extern_from_net;
  directory = (net_head->extern_connections)->extern_from_dir;
  
  if ( (last_file_ext = get_last_file(ext_net_name, directory)) <= 0 ) {
    /* fprintf(stderr, "file_update_check(): %s NOT exist.\n",ext_net_name); */
    return(NOT_EXIST);
  } 
  
  /* some file exist	*/
  if ( last_file_ext < (net_head->extern_connections)->extern_from_times ) {
    /* file not updated.	*/
    return(NOT_EXIST);
  } 
  
  /* updated file exist.	(assuming files are made sequencially as it's extension) */
  if ( (file_compl_status = file_complete_check(file_name, directory)) <= 0 ) {
    /* not completed, may be now writing, return(FALSE). 
     * or file extension order irregular, return(ERROR).
     */
    return file_compl_status;
  }
  
  /* file write already completed.	*/
  return(TRUE);
}



/*	get_last_file()	
*
*	RETURN VALUES: 	int last_file_ext	latest file extension number.
*			NOT_EXIST (= ERROR)	file not exist.
*			ERROR			pipeline to sh failed.
*	NOTE: if file extension is not number, return(0).
*
*	Function: Open shell pipe to "ls | grep" the file.
*		  Get extension of the file and compare it with expected #.
*/

#include	<sys/param.h>

int
get_last_file( net_name, directory )
    char	*net_name;
    char	*directory;
{
  FILE	*sh_pipe;		/* file pointer to sh pipe. */
/*  int	fdr; */			/* file discripter for open() */
/*  int	read_repeat; */		/* # of times to be repeated for read */
/*  int	file_buff_size; */	/* file buffer size in byte */
/*  float	file_buf[FILE_BUFSIZE]; */
/*  int	write_length; */		/* length written */
/*  int	file_length; */		/* current actual file length */
/*  int	nbyte; */
/*  int	number_data; */
  char	curr_dir[NAME_SIZE];
  char	com_last_file[NAME_SIZE];
  char	last_file[NAME_SIZE];
  char	last_file_name[NAME_SIZE];
  int	last_file_ext;
/*  char	working_dir[NAME_SIZE]; */
	
  if (getcwd (curr_dir, sizeof(curr_dir)) == NULL) {
    fprintf(stderr, "getcwd failed.\n");
    exit(1);
  }
  /*
   *  chdir(directory); This routine supports different directory, but
   *  is not used.
   */

  sprintf ( com_last_file, "ls -tr | grep %s | tail -1", net_name );
  fflush( stdout );
  if ((sh_pipe = popen( com_last_file, "r" )) == NULL) {	
    fprintf( stderr,
	    "get_last_file(): ERROR sh_pipe failed!     pwd = %s\n",
	    curr_dir ); 
    pclose(sh_pipe); 
    /* chdir (curr_dir); */
    return (ERROR);
  } 
  fscanf (sh_pipe,"%s",last_file);
  pclose(sh_pipe);

  /* chdir (curr_dir); */	
  sscanf( last_file,"%[^.] %*c %d",last_file_name , &last_file_ext );
  if ( strcmp(last_file_name, net_name) != 0 ) {
    /* file does not exist.		*/
    return(NOT_EXIST);
  } else {
    /* file exist			*/
    return(last_file_ext);
  }
}



/* time_now.c	    same as used in time.c (time.out)		
*	return sec in a resolution of 0.125 sec. 
*/

#include	<fcntl.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/time.h>


float
time_now()
{
  struct timeval tp;		/* for time info.	*/
  struct timezone tzp;
  float now, sec, msec;


  gettimeofday( &tp, &tzp );

  sec = (float) tp.tv_sec - 686414880;
  msec = 0.000001 * (float) tp.tv_usec;

  now = ( sec + msec );

  /* fprintf (stderr,"Time now %f %f %f \n",sec, msec,now);  */

  return now;
}


/* init_extern_connections.c 
* rm all external connection files, and reset extern_*_times to 0.
* this function will be called from main function and 
* run before run_simulation(),
*/

int
init_extern_connections(net_head)
    NETWORK net_head;
{
  char		curr_dir[NAME_SIZE];
  char		directory[NAME_SIZE];
  char		rm_files[NAME_SIZE];
  char		file_name[NAME_SIZE];
  NETWORK		network;
  int		last_ext;

  network = net_head;

  /* loop all networks  */
  while ( network != NULL ) {
    if ( (network->extern_connections)->extern_to == TRUE ) {
      /* extern_to exist */
      sprintf( file_name, "%s", network->name);
      sprintf( directory, "%s", (network->extern_connections)->extern_to_dir );
      if (getcwd (curr_dir, sizeof(curr_dir)) == NULL) {
	fprintf(stderr, "getcwd failed.\n");
	exit(1);
      }
      chdir (directory);
      last_ext = get_last_file(file_name, directory);
      if ( last_ext > 0 ) {
	/* the file(s) exist, rm network_name.*  */
	sprintf ( rm_files, "rm %s.*", file_name );
	system ( rm_files );
	printf(" Network  %s: reset external_to connection(s). (file(s) removed)\n", network->name );
      } else {
	printf(" Network  %s: reset external_to connection(s). (No file removed)\n", network->name );
      }
      (network->extern_connections)->extern_to_times = 0;
      chdir (curr_dir);
    }
		
		if ( (network->extern_connections)->extern_from == TRUE ) {
			(network->extern_connections)->extern_from_times = 0;
			printf(" Network  %s: reset external_from connection. \n", network->name );
		}
		network = network->next;
	}
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
