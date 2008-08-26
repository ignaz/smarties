/**
 * \file smarties2.c
 * 
 * Copyright (c) 2008 Simeon Felis
 * 
 * \brief Entry file for Smarties project
 * 
 * \version 0.1
 * 
 * \mainpage Smarties2 software
 * 
 * \section intro License
 * 
 * GPL2 Licence
 * 
 * \section install Description
 * 
 * This is the Source code's API and flow documentation  for Smarties2
 * 
 * \section arch Architecture
 * 
 * Goal of this application is a state machine controlled by status flags.
 * The main() function controls the program flow by reading and setting status
 * flags. These status flags are polled each millisecond in an timer interrupt 
 * routine. This timer interrupt routine reads and sets the IO ports and sets 
 * corresponding status flags.
 * 
 * Following image clarifies the structure of the software:
 * 
 * \image html api-layer.png "Layers of the software stack"
 * \image latex api-layer.png "Layers of the software stacke" width=15cm
 * 
 * The task of the differen layers, high level, abstraction layer and low level can be described
 * like in following image:
 * 
 * \image html state-machine.png "Executing the different layers"
 * \image latex state-machine.png "Executing the different layers" width=15cm
 * 
 * The state machine is devided into two sections, the mode and steps. Modes
 * are represented as an enum \ref system_mode_t and the steps as an struct \ref system_step_t
 * 
 * The modes are changed depending on the user inputs or after powering on/reseting. The 
 * next picture clarifies the modes of the state machine.
 * 
 * \image html state-machine-modes.png "State diagram of the smartie sorter" 
 * \image latex state-machine-modes.png "State diagram of the smartie sorter" width=10cm
 * 
 * The mode \ref SYS_MODE_RUNNING equals the automatic mode, where everything is 
 * controlled in several steps. The last step, step III, is a transition step to
 * begin from the start again. See next picture for the overview of the steps.
 * 
 * \image html state-machine-steps.png "Executing steps of the mode SYS_MODE_RUNNING"
 * \image latex state-machine-steps.png "Executing steps of the mode SYS_MODE_RUNNING" width=10cm
 * 
 * Each steps starts several tasks and waits until they are finished. Then the next
 * step will be entered. 
 * 
 * To see what is happening exactly in the different steps, please have a look at the 
 * sourcecode.
 * 
 * The modes, steps and all input/output related parts of the Smartie sorter are administrated
 * within struct. The structs are organized like in the following picture.
 * 
 * \image html objects-overview.png "Brief overview of several objects (elements) of the smartie sorter"
 * \image latex objects-overview.png "Brief overview of several objects (elements) of the smartie sorter" width=15cm
 * 
 * Note that this picture doesn't show all objects and all elements. It's just a draft overview.
 * For the detailed overview please refere to the code and documentation of \ref smartie_sorter_t. 
 * 
 * The application entry point is located is the \ref smarties2.c file.
 * - The main function first performs the initialization of inputs and outputs
 * - It handles the modes of the smartie sorter
 * - It handles the state machine
 * 
 * The LCD controlling is done with the \ref lcd_display.h
 * 
 * The Menu structure is described in \ref menu.h
 * 
 * The smartie color detection is done by calculating the smallest distance to a next smartie. 
 * 
 * For reference measures some values for each channel are recorded to gain a avarage value. They can 
 * be represented in an 3 Dimensional graph. For color detection the avarage value for each channel 
 * is used.
 * 
 * \image html plots_html.png "Smartie color RGB valus" width=15cm
 * \image latex plots.png "Smartie color RGB valus" width=\textwidth
 * 
 * If a smartie color's red, green and blue cannel are measured the distance to each reference 
 * smartie is worked out by following formula:
 * 
 * \f[
 * Distance = \sqrt{ ( {blue_{new} - blue_{ava} } )^2 + { (green_{new} - green_{ava} })^2 + { red_{new} - red_{ava} })^2 }
 * \f]
 * 
 * The color tables with avarage values can be found in \ref system.c (\ref col_ava_blu).
 * 
 * Only the blue, green and red channel is respected. A survey brought up that the brightness of the 
 * surrounding has no influence to the color measurent. The most important factor is temperature, 
 * as the smarties fat comes when they are getting to warm (above 24 Deg C). Then, the smartie's colors 
 * become brighter.
 * 
 * More methods for calculating the correct smartie color are prepared in the code. They can be enabled
 * by compiler switches. Enabling all methods could possible fill all data memory, as a lot of reference
 * data is necessary. 
 * 
 * 
 * The system related IO actions are all defined in \ref system.h There are controlled
 * - moving the revolver
 * - moving the catcher
 * - user input controlls
 * 
 * Minor configurations are made in \ref smarties2.h
 */

#include <avr/interrupt.h>
//#include <avr/io.h>
#include <stdlib.h>		// math stuff, itoa etc.
#include "smarties2.h"
#include "system.h"
#include "inits.h"
#include "menu.h"
#include "lcd_display.h" 	//display lib

#define DEBUG	1

smartie_sorter ss;

menu_entry *menu_current;
menu_entry men_initializing;
menu_entry men_running;
menu_entry men_lay_greeting[2];
//menu_entry men_lay_main[3]; /* deprecated */
menu_entry men_lay_main[4];
//menu_entry men_lay_sub_rotate[3]; /* deprecated */
//menu_entry men_lay_sub_color[3]; /* deprecated */


// Small helper functions 
void smartie_lcd_write_color (smartie_color color);


/**
  * \brief  Entry functin for smarties2
  *
  * This function controls the whole smarties machine by 
  * checking the current mode and performing the single 
  * steps required for the mode.
  * 
  * This function also handles the user inputs.
  * 
  * The subfunctions for driving the periphals are collected in \ref system.h. The 
  * LCD menu is controlled by functions collected in \ref lcd_display.h. 
  * 
  */
int main(void) {
	uint8_t temp = 1;
	
	int8_t index_temp;
	smartie_color col_temp;

	char s[7];

	ss.state.mode = SYS_MODE_INIT;
	ss.state.mode_last = SYS_MODE_INIT;

	init_all();
	sei	();

	/* Wait for some init functions to be finished */
	while (temp) {
#if 1 /* catcher motor status output */
		lcd_gotoxy(12,1); lcd_puts("c");
		switch (ss.mot_catcher.status) {
		case stat_idle:
			lcd_gotoxy(13,1); lcd_puts("cidl");
			break;
		case stat_start_working:
			lcd_gotoxy(13,1); lcd_puts("csta");
			break;
		case stat_working:
			lcd_gotoxy(13,1); lcd_puts("cwor");
			break;
		case stat_stop_working:
			lcd_gotoxy(13,1); lcd_puts("csto");
			break;
		case stat_finished:
			lcd_gotoxy(13,1); lcd_puts("cfin");
			break;
		default:
			break;
		}
		switch (ss.mot_catcher.status_last) {
		case stat_idle:
			lcd_gotoxy(19,1); lcd_puts("lidl");
			break;
		case stat_start_working:
			lcd_gotoxy(19,1); lcd_puts("lsta");
			break;
		case stat_working:
			lcd_gotoxy(19,1); lcd_puts("lwor");
			break;
		case stat_stop_working:
			lcd_gotoxy(19,1); lcd_puts("lsto");
			break;
		case stat_finished:
			lcd_gotoxy(19,1); lcd_puts("lfin");
			break;    				
		default:
			break;
		}
#endif
#if 1 /* revolver motor status output */
		lcd_gotoxy(0,1); lcd_puts("r");
		switch (ss.mot_revolver.status) {
		case stat_idle:
			lcd_gotoxy(1,1); lcd_puts("cidl");
			break;
		case stat_start_working:
			lcd_gotoxy(1,1); lcd_puts("csta");
			break;
		case stat_working:
			lcd_gotoxy(1,1); lcd_puts("cwor");
			break;
		case stat_stop_working:
			lcd_gotoxy(1,1); lcd_puts("csto");
			break;
		case stat_finished:
			lcd_gotoxy(1,1); lcd_puts("cfin");
			break;
		default:
			break;
		}
		switch (ss.mot_revolver.status_last) {
		case stat_idle:
			lcd_gotoxy(7,1); lcd_puts("lidl");
			break;
		case stat_start_working:
			lcd_gotoxy(7,1); lcd_puts("lsta");
			break;
		case stat_working:
			lcd_gotoxy(7,1); lcd_puts("lwor");
			break;
		case stat_stop_working:
			lcd_gotoxy(7,1); lcd_puts("lsto");
			break;
		case stat_finished:
			lcd_gotoxy(7,1); lcd_puts("lfin");
			break;    				
		default:
			break;
		}
#endif
		if ( (ss.mot_revolver.status == stat_idle)
				&& (ss.mot_revolver.status_last != stat_idle) ) /* Wait until position is found */
			ss.mot_revolver.status_last = stat_idle;

		if ( (ss.mot_catcher.status == stat_idle)
				&& (ss.mot_catcher.status_last != stat_idle) ) /* Wait until position is found */
			ss.mot_catcher.status_last = stat_idle;

		if ( (ss.mot_catcher.status_last == stat_idle) 
				&& (ss.mot_revolver.status_last == stat_idle) ) {
			ss.mot_revolver.current_pos = 0;
			ss.lb_revolver.passes = 0;

			temp = 0;
		}
	}

	/* initializing done, set state */
	ss.state.mode_last = ss.state.mode;
	ss.state.mode = SYS_MODE_PAUSE;

	lcd_clrscr();
		
	while (1) /* Main loop */
	{
    	switch (ss.state.mode) {
    	case SYS_MODE_INIT:
    		break;
    	case SYS_MODE_PAUSE:
    		if (ss.state.mode_last != SYS_MODE_PAUSE) { /* if we just entered that mode */
    			menu_current = &men_lay_greeting[0];
    			/* if we come from a reset */
    			if (ss.state.mode_last == SYS_MODE_INIT)
        			menu_current = &men_lay_greeting[1];    				
    			lcd_gotoxy(0,0); lcd_puts(menu_current->text[0]);
    			lcd_gotoxy(0,1); lcd_puts(menu_current->text[1]);
    			ss.state.mode_last = SYS_MODE_PAUSE;
    			ss.sens_tcs.status_last = stat_idle;
    		}
    		
    		/* color measure program tcs */
    		if (ss.prog == prog_color_tcs) {
    			if (ss.sens_tcs.status_last == stat_finished) {
    				ss.sens_tcs.status_last = stat_idle;
    				lcd_gotoxy(8,0); smartie_lcd_write_color(ss.sens_tcs.color);
    				lcd_gotoxy(15,0); lcd_puts("dis:  ");
    				lcd_gotoxy(19,0); lcd_puts(itoa(ss.sens_tcs.distance,s,10));
    				lcd_gotoxy(0,1); lcd_puts(MEN_TIT_EMPTY);
    				lcd_gotoxy(0,1); lcd_puts("Blu:");
    				if (ss.sens_tcs.filter_freq_blue<10) lcd_puts(" ");
    				lcd_puts(itoa(ss.sens_tcs.filter_freq_blue, s, 10));
    				lcd_puts("Gre:");
    				if (ss.sens_tcs.filter_freq_green<10) lcd_puts(" ");
    				lcd_puts(itoa(ss.sens_tcs.filter_freq_green, s, 10));
    				lcd_puts("Red:");
    				if (ss.sens_tcs.filter_freq_red<10) lcd_puts(" ");
    				lcd_puts(itoa(ss.sens_tcs.filter_freq_red, s, 10));
    				lcd_puts("Bri:");
    				if (ss.sens_tcs.filter_freq_none<10) lcd_puts(" ");
    				lcd_puts(itoa(ss.sens_tcs.filter_freq_none, s, 10));
    			}
    		}
    		
    		/* rotate program revolver */
    		if (ss.prog == prog_rotate_revolver) {
    			lcd_gotoxy(9,1); lcd_puts("Pos:");
				if (ss.mot_revolver.current_pos<10) lcd_puts(" ");
				lcd_puts(itoa(ss.mot_revolver.current_pos, s, 10));    			
    			if ( (ss.mot_revolver.status == stat_idle) && (ss.mot_revolver.status_last != stat_idle ) ) {
    				ss.mot_revolver.status_last = stat_idle;
    			}
    		}
    		
    		/* rotate program catcher */
    		if (ss.prog == prog_rotate_catcher) {
				lcd_gotoxy(0,1); lcd_puts("Now:");
				smartie_lcd_write_color(ss.mot_catcher.current_pos); 
				lcd_puts(itoa(ss.mot_catcher.current_pos, s, 10)); lcd_puts(";");
				lcd_puts("Next:"); 
				smartie_lcd_write_color(ss.mot_catcher.target_pos);
				lcd_puts(itoa(ss.mot_catcher.target_pos,s,10)); 
    			if ( (ss.mot_catcher.status == stat_idle) && (ss.mot_catcher.status_last != stat_idle) ) {
    				ss.mot_catcher.status_last = stat_idle;
    			}
    		}
    		break; /* SYS_MODE_PAUSE */

/******************************* SYS_MODE_RUNNING *******************************************/

		case SYS_MODE_RUNNING:
			if (ss.state.mode_last != SYS_MODE_RUNNING) { /* if we just started to run */
				ss.state.mode_last = SYS_MODE_RUNNING;
				menu_current = &men_running;
#if !DEBUG
				lcd_gotoxy(0,0); lcd_puts(menu_current->text[0]);
				lcd_gotoxy(0,1); lcd_puts(menu_current->text[1]);
#endif
#if DEBUG
				lcd_gotoxy(0,0); lcd_puts(MEN_TIT_EMPTY);
				lcd_gotoxy(0,1); lcd_puts(MEN_TIT_EMPTY);
#endif
				ss.state.step.I = stat_start_working;
				ss.state.step.II = stat_idle;
				ss.state.step.III = stat_idle;
			}

			if (ss.state.step.I == stat_start_working) { /* Initiate all work to be done: TCS */
				ss.state.step.I = stat_working;				
				sensor_tcs_get_color();
			}
			if (ss.state.step.I == stat_working) { /* Wait until is finished: TCS */
				if ( (ss.sens_tcs.status == stat_idle) && (ss.sens_tcs.status_last != stat_idle)) {
					ss.sens_tcs.status_last = stat_idle;
					ss.rev.smart[ss.mot_revolver.current_pos].color = ss.sens_tcs.color;
#if DEBUG
					lcd_gotoxy(0,0); lcd_puts("I:"); smartie_lcd_write_color(ss.sens_tcs.color);
					lcd_gotoxy(9,0); lcd_puts("O:");
					temp = sys_get_out_pos();
					smartie_lcd_write_color( ss.rev.smart[temp].color );
#endif
#if !DEBUG
					lcd_gotoxy(8,1); smartie_lcd_write_color(ss.sens_tcs.color);
#endif 
				}
				
				if ( (ss.sens_tcs.status_last == stat_idle) && (ss.sens_tcs.status == stat_idle) ) {
#if DEBUG
					lcd_gotoxy(0,1); 
					lcd_puts("C:"); lcd_puts(itoa(ss.mot_catcher.current_pos,s,10));
					lcd_puts("="); smartie_lcd_write_color(ss.mot_catcher.current_pos);
					lcd_puts("R:"); lcd_puts(itoa(ss.mot_revolver.current_pos,s,10));
					if (ss.mot_revolver.current_pos < 9) lcd_puts(" ");
#endif
					ss.state.step.I = stat_stop_working;
				}
			}
			if (ss.state.step.I == stat_stop_working) {
				ss.state.step.I = stat_idle;
				ss.state.step.II = stat_start_working;
			}
			if (ss.state.step.II == stat_start_working) { /* Initiate all work to be done: Catcher */
#if DEBUG
				lcd_gotoxy(0,1); 
				lcd_puts("C:"); lcd_puts(itoa(ss.mot_catcher.current_pos,s,10));
				lcd_puts("="); smartie_lcd_write_color(ss.mot_catcher.current_pos);
				lcd_puts("R:"); lcd_puts(itoa(ss.mot_revolver.current_pos,s,10));
				if (ss.mot_revolver.current_pos < 9) lcd_puts(" ");
#endif
				ss.state.step.II = stat_working;
				index_temp = sys_get_out_pos(); 
				col_temp = ss.rev.smart[index_temp].color;
				catcher_rotate_absolute(col_temp);

#if !DEBUG
				lcd_gotoxy(18,1); smartie_lcd_write_color(ss.rev.smart[index_temp].color);
#endif
#if DEBUG
				lcd_gotoxy(0,1); 
				lcd_puts("C:"); lcd_puts(itoa(ss.mot_catcher.current_pos,s,10));
				lcd_puts("="); smartie_lcd_write_color(ss.mot_catcher.current_pos);
				lcd_puts("R:"); lcd_puts(itoa(ss.mot_revolver.current_pos,s,10));
				if (ss.mot_revolver.current_pos < 9) lcd_puts(" ");
#endif
			}
			if (ss.state.step.II == stat_working) { /* Wait until is finished: Catcher */
				if ( (ss.mot_catcher.status == stat_idle) && (ss.mot_catcher.status_last != stat_idle)) {
					ss.mot_catcher.status_last = stat_idle;
#if DEBUG
					lcd_gotoxy(0,1); 
					lcd_puts("C:"); lcd_puts(itoa(ss.mot_catcher.current_pos,s,10));
					lcd_puts("="); smartie_lcd_write_color(ss.mot_catcher.current_pos);
					lcd_puts("R:"); lcd_puts(itoa(ss.mot_revolver.current_pos,s,10));
					if (ss.mot_revolver.current_pos < 9) lcd_puts(" ");
#endif
				}

				if (ss.mot_catcher.status_last == stat_idle) {
#if DEBUG
					lcd_gotoxy(0,1); 
					lcd_puts("C:"); lcd_puts(itoa(ss.mot_catcher.current_pos,s,10));
					lcd_puts("="); smartie_lcd_write_color(ss.mot_catcher.current_pos);
					lcd_puts("R:"); lcd_puts(itoa(ss.mot_revolver.current_pos,s,10));
					if (ss.mot_revolver.current_pos < 9) lcd_puts(" ");
#endif
					ss.state.step.II = stat_stop_working;
				}
			}
			if (ss.state.step.II == stat_stop_working) {
				ss.state.step.II = stat_idle;
				ss.state.step.III = stat_start_working;
#if DEBUG
				lcd_gotoxy(0,1); 
				lcd_puts("C:"); lcd_puts(itoa(ss.mot_catcher.current_pos,s,10));
				lcd_puts("="); smartie_lcd_write_color(ss.mot_catcher.current_pos);
				lcd_puts("R:"); lcd_puts(itoa(ss.mot_revolver.current_pos,s,10));
				if (ss.mot_revolver.current_pos < 9) lcd_puts(" ");
#endif
			}

			if (ss.state.step.III == stat_start_working) { /* Initiate all work to be done: Revolver & vibrator */
				ss.state.step.III = stat_working;
				vibrator_start();
				revolver_rotate_relative(1);
#if DEBUG
				lcd_gotoxy(0,1); 
				lcd_puts("C:"); lcd_puts(itoa(ss.mot_catcher.current_pos,s,10));
				lcd_puts("="); smartie_lcd_write_color(ss.mot_catcher.current_pos);
				lcd_puts("R:"); lcd_puts(itoa(ss.mot_revolver.current_pos,s,10));
				if (ss.mot_revolver.current_pos < 9) lcd_puts(" ");
#endif
			}
			if (ss.state.step.III == stat_working) { /* Wait until is finished: Revolver & vibrator */
				if ( (ss.mot_revolver.status == stat_idle) && (ss.mot_revolver.status_last != stat_idle) ) { 
					ss.mot_revolver.status_last = stat_idle;
#if DEBUG
					lcd_gotoxy(0,1); 
					lcd_puts("C:"); lcd_puts(itoa(ss.mot_catcher.current_pos,s,10));
					lcd_puts("="); smartie_lcd_write_color(ss.mot_catcher.current_pos);
					lcd_puts("R:"); lcd_puts(itoa(ss.mot_revolver.current_pos,s,10));
					if (ss.mot_revolver.current_pos < 9) lcd_puts(" ");
#endif
				}
				if ( (ss.vibr.status == stat_idle) && (ss.vibr.status_last != stat_idle) ) {
					ss.vibr.status_last = stat_idle;
				}
				if ( (ss.vibr.status_last == stat_idle) && (ss.mot_revolver.status_last == stat_idle) ) {
					ss.state.step.III = stat_stop_working;
#if DEBUG
					lcd_gotoxy(0,1); 
					lcd_puts("C:"); lcd_puts(itoa(ss.mot_catcher.current_pos,s,10));
					lcd_puts("="); smartie_lcd_write_color(ss.mot_catcher.current_pos);
					lcd_puts("R:"); lcd_puts(itoa(ss.mot_revolver.current_pos,s,10));
					if (ss.mot_revolver.current_pos < 9) lcd_puts(" ");
#endif
				}
			} /* if (ss.state.step.III == stat_working */

			if (ss.state.step.III == stat_stop_working) {
				ss.state.step.III = stat_idle;
				ss.state.step.I = stat_start_working;
			}

			break; /* SYS_MODE_RUNNING */
		default:
			break;
    	} /* switch (ss.mode.state) */

    	/* Poll for user inputs */
		if (ss.rotenc.push) {
			ss.rotenc.push = 0;
//			menu_action = (*menu_current).function;
//    		menu_action();
			(*menu_current).function();
			lcd_gotoxy(0,0); lcd_puts(menu_current->text[0]);
			lcd_gotoxy(0,1); lcd_puts(menu_current->text[1]);    			
		}
		if (ss.rotenc.left) {
			ss.rotenc.left = 0;
			menu_current = menu_current->prev;
			lcd_gotoxy(0,0); lcd_puts(menu_current->text[0]);
			lcd_gotoxy(0,1); lcd_puts(menu_current->text[1]);    			
		}
		if (ss.rotenc.right) {
			ss.rotenc.right = 0;
			menu_current = menu_current->next;
			lcd_gotoxy(0,0); lcd_puts(menu_current->text[0]);
			lcd_gotoxy(0,1); lcd_puts(menu_current->text[1]);    			
		}
	} /* Main loop end */
	return 0;
}

/**
 * \brief Writes the color to the current postion of the display. 
 */
void smartie_lcd_write_color (smartie_color color) {
	switch (color) {
	case col_yellow:
		lcd_puts(MEN_COL_YELLOW);
		break;
	case col_blue:
		lcd_puts(MEN_COL_BLUE);
		break;
	case col_brown:
		lcd_puts(MEN_COL_BROWN);
		break;
	case col_green:
		lcd_puts(MEN_COL_GREEN);
		break;
	case col_orange:
		lcd_puts(MEN_COL_ORANGE);
		break;
	case col_pink:
		lcd_puts(MEN_COL_PINK);
		break;
	case col_purple:
		lcd_puts(MEN_COL_PURPLE);
		break;
	case col_red:
		lcd_puts(MEN_COL_RED);
		break;
	default:
		lcd_puts(MEN_COL_UNKNOWN);
		break;
	}
}
