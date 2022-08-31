/* 
 * tkGLx.c --
 *
 *	This module implements "GLxwin" widgets. This is the TCL
 *	interface to OpenGL.
 *
 * Copyright (c) 1991-1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */


#define HAVE_LIMITS_H
#include <tkInt.h>
/* #include "tkInt.h" */

#include <GL/glx.h>
#include <GL/gl.h>
#include "nexus_tk.h"

#include "tkGLx.h"

static int	attributeList[] = {
    GLX_RGBA,
    GLX_STENCIL_SIZE, 0,
    GLX_ACCUM_RED_SIZE, 0,
    GLX_ACCUM_GREEN_SIZE, 0,
    GLX_ACCUM_BLUE_SIZE, 0,
    GLX_ACCUM_ALPHA_SIZE, 0,
    GLX_AUX_BUFFERS, 0,
    GLX_DEPTH_SIZE, 0,
    GLX_RED_SIZE, 1,
    GLX_GREEN_SIZE, 1,
    GLX_BLUE_SIZE, 1,
    None
};


/*
 * A data structure of the following type is kept for each glxwin
 * widget managed by this file:
 */

typedef struct {
    Tk_Window tkwin;		/* Window that embodies the glxwin.  NULL
				 * means window has been deleted but
				 * widget record hasn't been cleaned up yet. */
    Display *display;		/* X's token for the window's display. */
    Tcl_Interp *interp;		/* Interpreter associated with widget. */

    int width;			/* Width to request for window.  <= 0 means
				 * don't request any size. */
    int height;			/* Height to request for window.  <= 0 means
				 * don't request any size. */

    int flags;			/* Various flags;  see below for
				 * definitions. */

    int initialized;

    /*  Backing Store record: DW 95.01.04  */
    GLXPixmap glxPixmap;
    Pixmap pixmap;

    /*  Additional information needed: IKL 95.02.02  */
    XVisualInfo *visualInfo;
    Colormap cmap;
    GLXContext glxCxDirect;	/* Direct rendering */
    GLXContext glxCxXServer;	/* Rendering through the X Server */
    GC copyingGC;

} Glxwin;

/*
 *  Information used for argv parsing.
 */

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_PIXELS, "-height", "height", "Height",
       0, Tk_Offset(Glxwin, height), 0},
  
    {TK_CONFIG_PIXELS, "-width", "width", "Width",
       0, Tk_Offset(Glxwin, width), 0},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};

/*
 *  Forward declarations for procedures defined later in this file:
 */

static int		GlxwinConfigure _ANSI_ARGS_((Tcl_Interp *interp,
			    Glxwin *glxwinPtr, int argc, char **argv,
			    int flags));
static void		GlxwinDestroy _ANSI_ARGS_((char *clientData));
static void		GlxwinEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static int		GlxwinWidgetCmd _ANSI_ARGS_((ClientData clientData,
				Tcl_Interp *, int argc, char **argv));
/*
 *  For C Backing store routines.  DW 95.02.04
 */

static Glxwin *		currGlxwin;
extern int		backing_store;


/*
 *  GetVisual  --  get a required visual, try to find one even over
 *    a remote connection to a foreign Display
 *  IKL 95.03.07
 */

static XVisualInfo *
GetVisual( Glxwin * glxwinPtr )
{
  XVisualInfo *list, template;
  int mask =	VisualScreenMask |
		VisualBitsPerRGBMask |
		VisualClassMask;
/*		VisualDepthMask; */
  int nitems;

/*  template.depth 		= 24; */
  template.screen		= Tk_ScreenNumber(glxwinPtr->tkwin);
  template.bits_per_rgb		= 8;
  template.class		= TrueColor;

  list = XGetVisualInfo(glxwinPtr->display, mask, &template, &nitems);

  if (nitems > 1) {
    return &list[0];
  }
  else {
    return NULL;
  }
}


static void
Tk_MakeGLXWindowExist( Glxwin * glxwinPtr )
{
  TkWindow *	winPtr	= (TkWindow *) glxwinPtr->tkwin;
  TkWindow *	winPtr2;

  Tk_Window	tkwinPtr = glxwinPtr->tkwin;
  Window	parent;
  Display *	dpy	 = Tk_Display( tkwinPtr );
  XVisualInfo * glxVi;
  int dummy;
  Tcl_HashEntry * hPtr;

  if (Tk_WindowId( tkwinPtr ) != None) {
    return;
  }

  if (Tk_IsTopLevel( tkwinPtr )) {
    parent = XRootWindow(dpy, Tk_ScreenNumber( tkwinPtr ));
  } else {
    if (winPtr->parentPtr->window == None) {
      Tk_MakeWindowExist((Tk_Window) (Tk_Parent( tkwinPtr )));
    }
    parent = winPtr->parentPtr->window;
  }

  /*
   *  Initialize the fields of a GLX widget.
   */

  /* get an appropriate visual */
  glxwinPtr->visualInfo = glxVi = glXChooseVisual( dpy,
						  DefaultScreen( dpy ),
						  attributeList );

  /*  glXChooseVisual failed; try a different approach  IKL 95.03.07  */
  if (!glxVi)
    glxwinPtr->visualInfo = glxVi = GetVisual( glxwinPtr );

  /*
   *  create a GLX context:
   *  This must be glXCreateContext( ..., GL_FALSE ); when
   *    a pixmap is used for backing store.  DW 95.01.11
   *  For the remote nexus (rnexus), all contexts MUST be through the
   *    X server (see GLX documentation).  DW 95.02.27
   */

  glxwinPtr->glxCxXServer = glXCreateContext( dpy, glxVi, None, GL_FALSE );

#ifdef NEXUS_REMOTE
  glxwinPtr->glxCxDirect = glXCreateContext( dpy, glxVi,
					    glxwinPtr->glxCxXServer,
					    GL_FALSE );
#else
  glxwinPtr->glxCxDirect = glXCreateContext(dpy, glxVi,
					    None,
					    GL_TRUE );
#endif /* NEXUS_REMOTE */

  /* create a color map */
  glxwinPtr->cmap = XCreateColormap( dpy, RootWindow( dpy, glxVi->screen ),
				    glxVi->visual, AllocNone );

  /* Set the attributes */
  Tk_SetWindowBorder( tkwinPtr, 0 );
  Tk_SetWindowColormap( tkwinPtr, glxwinPtr->cmap );

  /* Now create the window from the X server */
  winPtr->window = XCreateWindow( dpy, parent,
				 winPtr->changes.x, winPtr->changes.y,
				 winPtr->changes.width,
				 winPtr->changes.height,
				 winPtr->changes.border_width,
				 glxVi->depth, InputOutput, glxVi->visual,
				 CWBorderPixel|CWColormap|winPtr->dirtyAtts,
				 &winPtr->atts );
  hPtr = Tcl_CreateHashEntry(&winPtr->dispPtr->winTable,
	(char *) winPtr->window, &dummy);
  Tcl_SetHashValue(hPtr, winPtr);

  winPtr->dirtyAtts = 0;
  winPtr->dirtyChanges = 0;

  /*
   * If any siblings higher up in the stacking order have already
   * been created then move this window to its rightful position
   * in the stacking order.
   *
   * NOTE: this code ignores any changes anyone might have made
   * to the sibling and stack_mode field of the window's attributes,
   * so it really isn't safe for these to be manipulated except
   * by calling Tk_RestackWindow.
   */

  if (!(winPtr->flags & TK_TOP_LEVEL)) {
    for (winPtr2 = winPtr->nextPtr; winPtr2 != NULL;
	 winPtr2 = winPtr2->nextPtr) {
      if ((winPtr2->window != None) && !(winPtr2->flags & TK_TOP_LEVEL)) {
	XWindowChanges changes;
	changes.sibling = winPtr2->window;
	changes.stack_mode = Below;
	XConfigureWindow( dpy, winPtr->window,
			 CWSibling|CWStackMode, &changes);
	break;
      }
    }
  }

}


/*
 *--------------------------------------------------------------
 *
 * GlxwinCmd --
 *
 *	This procedure is invoked to process the "glxwin" Tcl
 *	command.  It creates a new "glxwin" widget.
 *
 *      IKL 95.02.02  Added Backing Store.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	A new widget is created and configured.
 *
 *--------------------------------------------------------------
 */

int
GlxwinCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
  Tk_Window	main_w = (Tk_Window) clientData;
  Glxwin *	glxwinPtr;
  Tk_Window	tkwin;


  if (argc < 2) {
    Tcl_AppendResult(interp, "wrong # args:  should be \"",
		     argv[0], " pathName ?options?\"", (char *) NULL);
    return TCL_ERROR;
  }

  tkwin = Tk_CreateWindowFromPath(interp, main_w, argv[1], (char *) NULL);
  if (tkwin == NULL) {
    return TCL_ERROR;
  }
  Tk_SetClass(tkwin, "GLxwin");

  /*
   * Allocate and initialize the widget record.
   */

  glxwinPtr = (Glxwin *) ckalloc(sizeof(Glxwin));
  glxwinPtr->tkwin = tkwin;
  glxwinPtr->display = Tk_Display(tkwin);
  glxwinPtr->interp = interp;
  glxwinPtr->width  = 0;
  glxwinPtr->height = 0;
  glxwinPtr->pixmap = None;
  glxwinPtr->glxPixmap = None;

  Tk_MakeGLXWindowExist( glxwinPtr );

  Tk_CreateEventHandler( glxwinPtr->tkwin, ExposureMask|StructureNotifyMask,
			GlxwinEventProc, (ClientData) glxwinPtr );
  Tcl_CreateCommand( interp, Tk_PathName(glxwinPtr->tkwin), GlxwinWidgetCmd,
		    (ClientData) glxwinPtr, (void (*)()) NULL );
  if (GlxwinConfigure( interp, glxwinPtr, argc-2, argv+2, 0 ) != TCL_OK) {
    Tk_DestroyWindow(glxwinPtr->tkwin);
    return TCL_ERROR;
  }

  /*
   *  Now create the GC for copying the pixmap to the GL window.
   */
  glxwinPtr->copyingGC = XCreateGC( glxwinPtr->display,
				   Tk_WindowId( glxwinPtr->tkwin ),
				   0, 0 );

  currGlxwin = glxwinPtr;

  interp->result = Tk_PathName( glxwinPtr->tkwin );
  return TCL_OK;
}



/*
 *--------------------------------------------------------------
 *
 * GlxwinWidgetCmd --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a widget managed by this module.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
GlxwinWidgetCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Information about glxwin widget. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
  Glxwin *glxwinPtr = (Glxwin *) clientData;
  int result = TCL_OK;
  int length;
  char c;


  if (argc < 2) {
    Tcl_AppendResult(interp, "wrong # args: should be \"",
		     argv[0], " option ?arg arg ...?\"", (char *) NULL);
    return TCL_ERROR;
  }
  Tk_Preserve((ClientData) glxwinPtr);
  c = argv[1][0];
  length = strlen(argv[1]);
  if ((c == 'c') && (strncmp(argv[1], "configure", length) == 0)) {

    if (argc == 2) {
      result = Tk_ConfigureInfo(interp, glxwinPtr->tkwin, configSpecs,
				(char *) glxwinPtr, (char *) NULL, 0);
    }
    else if (argc == 3) {
      result = Tk_ConfigureInfo(interp, glxwinPtr->tkwin, configSpecs,
				(char *) glxwinPtr, argv[2], 0);
    }
    else {
      result = GlxwinConfigure(interp, glxwinPtr, argc-2, argv+2,
			       TK_CONFIG_ARGV_ONLY);
    }
  }
  else if ((c == 'l') && (strncmp(argv[1], "link", length) == 0)) {

#ifdef DEBUG
    fprintf( stderr, "link\n" );
#endif

    /* connect the context to the window
     * All future OpenGL rendering will be drawn into this window.
     */

    glXMakeCurrent( glxwinPtr->display,
		   Tk_WindowId( glxwinPtr->tkwin ),
		   glxwinPtr->glxCxDirect );

  }
  else if ((c == 'p') && (strncmp(argv[1], "pmcreate", length) == 0)) {

    int width, height;


#ifdef DEBUG
    fprintf( stderr, "pmcreate %d %d\n", width, height );
#endif

    if (argc == 4) {
      width = atoi( argv[2] );
      height = atoi( argv[3] );
    }
    else {
      width = Tk_Width( glxwinPtr->tkwin );
      height = Tk_Height( glxwinPtr->tkwin );
    }

    if (glxwinPtr->pixmap != None) {
      glXDestroyGLXPixmap( glxwinPtr->display, glxwinPtr->glxPixmap );
      XFreePixmap( glxwinPtr->display, glxwinPtr->pixmap );
    }

    if (!(glxwinPtr->pixmap =
	  XCreatePixmap( glxwinPtr->display,
			Tk_WindowId( glxwinPtr->tkwin ),
			width, height,
			glxwinPtr->visualInfo->depth )))
      return TCL_ERROR;
    glXWaitX( );

    if (!(glxwinPtr->glxPixmap =
	  glXCreateGLXPixmap( glxwinPtr->display,
			     glxwinPtr->visualInfo,
			     glxwinPtr->pixmap )))
      return TCL_ERROR;
    glXWaitGL( );

  }
  else if ((c == 'p') && (strncmp(argv[1], "pmlink", length) == 0)) {

#ifdef DEBUG
    fprintf( stderr, "pmlink\n" );
#endif

    if (glxwinPtr->glxPixmap == None) {
      Tcl_AppendResult( interp, "Pixmap not yet created.", NULL );
      goto error;
    }

    /*
     *  DW 95.01.06
     *  Draw directly into backing store pixmap.  Each refresh will
     *    copy from pixmap into actual framebuffer.
     */
    glXMakeCurrent( glxwinPtr->display,
		   (GLXDrawable) glxwinPtr->glxPixmap,
		   glxwinPtr->glxCxXServer );
  }
  else if ((c == 'p') && (strncmp(argv[1], "pmcopy", length) == 0)) {

#ifdef DEBUG
    fprintf( stderr, "pmcopy %d %d %d %d\n",
	    atoi( argv[2] ), atoi( argv[3] ),
	    atoi( argv[4] ), atoi( argv[5] ) );
#endif

    if (glxwinPtr->glxPixmap == None) {
      Tcl_AppendResult( interp, "Pixmap not yet created.", NULL );
      goto error;
    }

    /*  XCopyArea from glxwinPtr->glxPixmap to framebuffer  */

    glXWaitGL( );
    XCopyArea( glxwinPtr->display,
	      glxwinPtr->pixmap,
	      Tk_WindowId( glxwinPtr->tkwin ),
	      glxwinPtr->copyingGC,
	      atoi( argv[2] ), atoi( argv[3] ),
	      atoi( argv[4] ), atoi( argv[5] ),
	      atoi( argv[2] ), atoi( argv[3] ) );

  }
  else {

    /*
     *  DW 95.01.06
     *  Changed error message from configure, position, or size to
     *    configure, link, or pmlink to conform to code.
     */

    Tcl_AppendResult( interp, "bad option \"", argv[1],
		     "\":  must be configure, link, or pm(create/link/copy)",
		     (char *) NULL );
    goto error;
  }

  Tk_Release((ClientData) glxwinPtr);
  return result;

 error:
  Tk_Release((ClientData) glxwinPtr);
  return TCL_ERROR;
}



/*
 *  GlxwinLink --
 *
 *    Link either the main window or the associated pixmap.
 */
void
GlxwinLink( register int whichDrawable )
{
  if (!currGlxwin)
    return;


  if (whichDrawable == NX_GLXPIXMAP && currGlxwin->glxPixmap != None) {

    /* 1 is NX_GLXPIXMAP */
    glXMakeCurrent( currGlxwin->display,
		   (GLXDrawable) currGlxwin->glxPixmap,
		   currGlxwin->glxCxXServer );

#ifdef DEBUG
    fprintf( stderr, "GlxwinLink( NX_GLXPIXMAP ), " );
#endif

  }
  else {

#ifdef DEBUG
    fprintf( stderr, "GlxwinLink( NX_GLXFRAMEBUF ): " );
#endif

    /* 0 is NX_GLXFRAMEBUF */
    glXMakeCurrent( currGlxwin->display,
		   Tk_WindowId( currGlxwin->tkwin ),
		   currGlxwin->glxCxDirect );

#ifdef DEBUG
    fprintf( stderr, "done.\n" );
#endif

  }

  return;
}



/*
 *  GlxwinUpdate --
 *
 *    Copy section from pixmap to framebuffer.
 */
void
GlxwinUpdate( register int left, register int top,
	     register int width, register int height )
{
#ifdef DEBUG
  fprintf( stderr, "GlxwinUpdate( %d, %d, %d, %d )\n",
	  left, top, width, height );
#endif

  if (!currGlxwin || !backing_store) {
    glFlush( );
    return;
  }

  glXWaitGL( );
  XCopyArea( currGlxwin->display,
	    currGlxwin->pixmap,
	    Tk_WindowId( currGlxwin->tkwin ),
	    currGlxwin->copyingGC,
	    left, top,
	    width, height,
	    left, top );
  glXWaitX( );
  glFlush( );
}  



/*
 *----------------------------------------------------------------------
 *
 * GlxwinConfigure --
 *
 *	This procedure is called to process an argv/argc list in
 *	conjunction with the Tk option database to configure (or
 *	reconfigure) a glxwin widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width,
 *	etc. get set for glxwinPtr;  old resources get freed,
 *	if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
GlxwinConfigure(interp, glxwinPtr, argc, argv, flags)
    Tcl_Interp *interp;			/* Used for error reporting. */
    Glxwin *glxwinPtr;			/* Information about widget. */
    int argc;				/* Number of valid entries in argv. */
    char **argv;			/* Arguments. */
    int flags;				/* Flags to pass to
					 * Tk_ConfigureWidget. */
{
  if (Tk_ConfigureWidget(interp, glxwinPtr->tkwin, configSpecs,
			 argc, argv, (char *) glxwinPtr, flags) != TCL_OK) {
    return TCL_ERROR;
  }

  /*
   * Register the desired geometry for the window.  Then arrange for
   * the window to be redisplayed.
   */

  if ((glxwinPtr->width > 0) || (glxwinPtr->height > 0)) {
    Tk_GeometryRequest(glxwinPtr->tkwin, glxwinPtr->width,
		       glxwinPtr->height);
  }
  return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * GlxwinEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on glxwins.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.
 *
 *--------------------------------------------------------------
 */

static void
GlxwinEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    Glxwin *glxwinPtr = (Glxwin *) clientData;

    if (eventPtr->type == DestroyNotify) {
	Tcl_DeleteCommand(glxwinPtr->interp, Tk_PathName(glxwinPtr->tkwin));
	glxwinPtr->tkwin = NULL;
	Tk_EventuallyFree((ClientData) glxwinPtr, GlxwinDestroy);
    }

}

/*
 *----------------------------------------------------------------------
 *
 * GlxwinDestroy --
 *
 *	This procedure is invoked by Tk_EventuallyFree or Tk_Release
 *	to clean up the internal structure of a glxwin at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the glxwin is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
GlxwinDestroy(clientData)
    char *clientData;	/* Info about glxwin widget. */
{
    Glxwin *glxwinPtr = (Glxwin *) clientData;

    Tk_FreeOptions(configSpecs, (char *) glxwinPtr, glxwinPtr->display, 0);
    if (glxwinPtr->glxPixmap != None) {
      glXDestroyGLXPixmap( glxwinPtr->display, glxwinPtr->glxPixmap );
      XFreePixmap( glxwinPtr->display, glxwinPtr->pixmap );
      XFreeGC( glxwinPtr->display, glxwinPtr->copyingGC );
    }
    ckfree((char *) glxwinPtr);
}



/*----------------------------------------------------------------------
 * TkGLx_Init()
 *
 * TkGlx Initialization routine: Create the command that manipulates
 * OpenGL widgets.
 *
 *----------------------------------------------------------------------
 */

int
TkGLx_Init( Tcl_Interp *interp, Tk_Window  topLevel )
{
  Tcl_CreateCommand( interp, "GLxwin", GlxwinCmd, topLevel, NULL );

  return TCL_OK;
}



#if 0
/*----------------------------------------------------------------------
 *  TkGLx_SetOffScreenRender( )
 *
 *  TkGlx Initialization routine:  Create the command that manipulates
 *  OpenGL widgets.
 *
 *----------------------------------------------------------------------
 */
TkGLx_SetOffScreenRender( ClientData clientData )
{
  Glxwin * glxwinPtr = (Glxwin *) clientData;
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
