/*
 *  Pops up the frame pointed to by item's PANEL_CLIENT_DATA.  Set this as
 *    the callback procedure for a button that should open up a sub-frame.
 *    Set the PANEL_CLIENT_DATA of that button to the sub-frame's handle.
 */
extern void		proc_btn_subframe( );

/*
 *  Closes the frame pointed to by item's PANEL_CLIENT_DATA.  Set this as
 *    the callback procedure for a button that should close a sub-frame.
 *    Set the PANEL_CLIENT_DATA of that button to the sub-frame's handle.
 */
extern void		proc_btn_cancel( );

/*
 *  Returns NXI_STOP if the Stop button in panel_simulate was pressed.
 *  Otherwise, returns NXI_IGNORE.
 */
extern int		check_interrupt( );

/*
 *  Set this as the callback procedure for a PANEL_TEXT item which must
 *    hold a name (such as a network name, or a filename).  It will store
 *    the value entered (by pressing RETURN) into the (char *) variable
 *    pointed to by PANEL_CLIENT_DATA.  Set the latter when creating the
 *    PANEL_TEXT item.  panel_filename gets special treatment.
 */
extern Panel_setting	proc_enter_name( );

/*
 */
extern Panel_setting	proc_enter_float( );
extern Panel_setting	proc_enter_int( );
extern Panel_setting	proc_enter_pos_int( );

/*
 *  Updates associated panel_list as well as variable.
 */
extern Panel_setting	proc_enter_network( );

extern int		proc_selection( );
extern void		proc_chbox_generic( );
extern void		proc_chbox_binary( ); /* for (re)setting nexus_flags. */

extern void		proc_percentage_slider( );


extern Server_image	carriage_return_image;
