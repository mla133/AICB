//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//   Add - Pak Diagnostics Here 
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#include "database.h"
#include "stamgr.h"
//#include "menumgr.h"
//#include "literals.h"
//#include "fcdi_key.h"
#include "comdrv.h"
#include "sai_cmds.h"
#include "bool.h"
#include "prv_inj.h"

#include <stdio.h>
//TODO: #pragma sep_on
extern float temp_SAI_totals [MAX_SMART_INJECTORS]; 
//TODO: #pragma sep_off
void *addpack_menu(byte key, byte arm, byte init);
void *addpack_version(byte key, byte arm, byte init);
void *select_addpack_menu(byte key, byte arm, byte init);
void *select_addpack_inject_menu(byte key, byte arm, byte init);
void *inj_io_diag_menu(byte key, byte arm, byte init);
void *inj_test_menu(byte key, byte arm, byte init);
void  *AICBTestMenu2(byte key, byte arm, byte init);
void complete_aicb_prove_inject(byte *injInProgress, byte *authorizedToInject, byte *injectCounts);

void SAI_Init(unsigned char inj_no);
void SAI_ResetCount(unsigned char inj_no);
void SAI_ReadCount(unsigned char inj_no);
void SAI_SwVersion(unsigned char inj_no);
void SAI_SetState(unsigned char inj_no, unsigned char output, unsigned char state);
void SAI_GetState(unsigned char inj_no, unsigned char output);
void SAI_VolPerCycle (unsigned char inj_no);
void SAI_II (register unsigned char inj_no);


/*-------------------------------------------------------------------+
|                                                                    |
| 		AddPak General Diagnostic Main Menu - Program Mode 			 |
|                                                                    |
+-------------------------------------------------------------------*/
enum {	VERSION,
		IO_TEST,
		TEST_INJECT,
		SAI_INITIALIZE,
		NUM_ADDPACK_DIAGS };

void  *addpack_menu(byte key, byte arm, byte init)
{
	//---------------------------------------------------------------------------
	byte i;
	static byte select_index;
	BOOL addPakInited = BOOL_FALSE;

	switch (select_index)
	{
		case VERSION:
			for (i=MAX_INJECTORS-1; i>=MAX_DUMB_INJECTORS; i--)
			{
				if ((rDB.add_inj.inj[i].type == ADD_PAK_INJ) || (rDB.add_inj.inj[i].type == ADD_PAK_2_STROKE))
					/* request version and crc information */
	 			SAI_SwVersion(i);
			}
			break;

		case IO_TEST:
			printf("IO_TEST\n");
			break;

		case TEST_INJECT:
			printf("TEST_INJECT\n");
			break;

		case SAI_INITIALIZE:
		// We now need to init two boards if we have them.
		// Board #1
			for (i=MAX_DUMB_INJECTORS; i<(MAX_DUMB_INJECTORS+10); i++)
				if ((pDB_edit_buf.add_inj.inj[i].type == ADD_PAK_INJ) || (pDB_edit_buf.add_inj.inj[i].type == ADD_PAK_2_STROKE))
				{
					//rDB.cur_injector = i;
					SAI_Init(i);
					addPakInited = BOOL_TRUE;
					break;
				}

		// Board #2
			for (i=(MAX_DUMB_INJECTORS+10); i<MAX_INJECTORS; i++)
				if ((pDB_edit_buf.add_inj.inj[i].type == ADD_PAK_INJ) || (pDB_edit_buf.add_inj.inj[i].type == ADD_PAK_2_STROKE))
				{
					//rDB.cur_injector = i;
					SAI_Init(i);
					addPakInited = BOOL_TRUE;
					break;
				}

			break;

		default:
			break;
		}

	//return;
}




/*---------------------------------------------------------------------------+
|                                                                            |
| 			AddPak Version / CRC Screen - Program Mode						 |
|                                                                            |
+---------------------------------------------------------------------------*/
void  *addpack_version(byte key, byte arm, byte init)
{
	//----------------------------------------------------------------
	byte i, j;														  
	char addpack_version[100];
	//----------------------------------------------------------------

	for(i=0, j=MAX_DUMB_INJECTORS; j<MAX_INJECTORS; i++, j++)
 	{
 		if ((pDB_edit_buf.add_inj.inj[j].type == ADD_PAK_INJ) || (pDB_edit_buf.add_inj.inj[j].type == ADD_PAK_2_STROKE))
 			sprintf(addpack_version,"%02d %8s", rDB.addpack[j].version, rDB.addpack[j].crc);
 	}
	return(addpack_version);
}


// Attempt to find a port for the Add-Pak
unsigned char AddPakBoard_1_Available (void)
{
	//----------------
	int i;
	//----------------

	for (i=MAX_DUMB_INJECTORS; i<(MAX_DUMB_INJECTORS+10); i++)
		{
		if ((rDB.add_inj.inj[i].type == ADD_PAK_INJ) || (rDB.add_inj.inj[i].type == ADD_PAK_2_STROKE))
			return (rDB.add_inj.inj[i].port);
		}

	return (0);
}


// Attempt to find a port for the Add-Pak
unsigned char AddPakBoard_2_Available (void)
{
	//----------------
	int i;
	//----------------

	for (i=(MAX_DUMB_INJECTORS+10); i<MAX_INJECTORS; i++)
		{
		if ((rDB.add_inj.inj[i].type == ADD_PAK_INJ) || (rDB.add_inj.inj[i].type == ADD_PAK_2_STROKE)) 
			return (rDB.add_inj.inj[i].port);
		}

	return (0);
}





/*-------------------------------------------------------+
| 		Select AddPak Menu For IO DIagnostics			 |
| 			- Program Mode                               |
+-------------------------------------------------------*/
enum    {  INJ5  ,
		   INJ6  , 
		   INJ7  , 
		   INJ8  , 
		   INJ9  , 
		   INJ10 , 
		   INJ11 , 
		   INJ12 , 
		   INJ13 , 
		   INJ14 , 
		   INJ15 , 
		   INJ16 , 
		   INJ17 , 
		   INJ18 , 
		   INJ19 , 
		   INJ20 , 
		   INJ21 , 
		   INJ22 , 
		   INJ23 , 
		   INJ24 , 

NUM_INJ };

void  *select_addpack_menu(byte key, byte arm, byte init)
{
	//------------------------------------------------------------------------------
	static byte select_index;
	byte i, j;
	//------------------------------------------------------------------------------


				switch (select_index)
					{
					// AICB Board #1
					case INJ5: 
				   	case INJ6: 
					case INJ7: 
				   	case INJ8: 
					case INJ9: 
				   	case INJ10:
					case INJ11:
				   	case INJ12:
					case INJ13:
					case INJ14:
						i = select_index + MAX_DUMB_INJECTORS;

						if ( (rDB.add_inj.inj[i].type == ADD_PAK_INJ) ||
							 (rDB.add_inj.inj[i].type == ADD_PAK_2_STROKE)
							|| AddPakBoard_1_Available())
							return(inj_io_diag_menu);
						else
							{
							printf("Addpak1 Not Available\n");
							//general_information(&litptr[addpak_1_not_available_lit]);
							break;
							}

					// AICB Board #2
					case INJ15:
				   	case INJ16:
					case INJ17:
				   	case INJ18:
					case INJ19:
				   	case INJ20:
					case INJ21:
				   	case INJ22:
					case INJ23:
					case INJ24:
						j = select_index + MAX_DUMB_INJECTORS;

						if ( (rDB.add_inj.inj[j].type == ADD_PAK_INJ) ||
							(rDB.add_inj.inj[j].type == ADD_PAK_2_STROKE)
							|| AddPakBoard_2_Available())
							return(inj_io_diag_menu);
						else
							{
							printf("Addpak2 Not Available\n");
							//general_information(&litptr[addpak_2_not_available_lit]);
							break;
							}

					default:
						break;


	 	}

	return(select_addpack_menu);
};


/*-------------------------------------------------------+
| 		AddPak IO Diagnostic - Program Mode				 |
+-------------------------------------------------------*/
enum {	TOGGLE_PUMP,
		TOGGLE_SOLENOID,
		RESET_COUNTS,
		NUM_IO_DIAGS };


void  *inj_io_diag_menu(byte key, byte arm, byte init)
{
	//-------------------------------------------------------------------------
	static byte select_index;
	unsigned char i;
	static unsigned char temp_pdb_type, temp_rdb_type, temp_addr, tempPort;
	static unsigned long secTimer;
	static unsigned char exitInProgress;

	const static MENU_ITEM inj_io_diag_menu_list[]=
		{
		{ROW(1),COL(1),SYSTEM_FONT,WHITE,ENDPT(ROW(1),COL(31)),STRING,0,0,
			TITLE|CENTER,0,0,0,&select_litptr[0],0,0},

		{ROW(2),COL(3),SYSTEM_FONT,WHITE,"%s",STRING,0,0,
			SELECT1|LIST|NO_ENUM,TOTAL(NUM_IO_DIAGS)|DISP(NUM_IO_DIAGS),
			0,0,&litptr[addpack_pump_lit],0,0},

	   	{ROW(2),COL(25),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			VAR_LIT,0,0,0,&select_litptr[1],0,0},

		{ROW(3),COL(25),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			VAR_LIT,0,0,0,&select_litptr[2],0,0},

		{ROW(5),COL(1),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			VAR_LIT,0,0,0,&select_litptr[3],0,0},
		};

	const static MENU_PAGE menu_pages[]=
		{
		MENU_LIST(inj_io_diag_menu_list)
		};

	const static MENU_INFO menu_info=
		{
		inj_io_diag_menu,
		(MENU_PAGE *)menu_pages,
		NUM_PAGES(menu_pages)
		};
	//-------------------------------------------------------------------------



	if (init == TRUE)
		{
		/* save actual injector programming - so we can set up as typical addpack injector */
		temp_pdb_type 	= pDB.add_inj.inj[rDB.cur_injector].type;
		temp_rdb_type 	= rDB.add_inj.inj[rDB.cur_injector].type;
		temp_addr 		= pDB.add_inj.inj[rDB.cur_injector].inj_address;
		tempPort 		= rDB.add_inj.inj[rDB.cur_injector].port;


		// Set the address up first!
		if (rDB.cur_injector < (MAX_DUMB_INJECTORS+10))
			pDB.add_inj.inj[rDB.cur_injector].inj_address = 97 + rDB.cur_injector;
		else if (rDB.cur_injector < MAX_INJECTORS)
			pDB.add_inj.inj[rDB.cur_injector].inj_address = 187 + rDB.cur_injector;

		// Set the comm port!!!	(if we need to)
		if ((pDB.add_inj.inj[rDB.cur_injector].type != ADD_PAK_INJ) && 
			(pDB.add_inj.inj[rDB.cur_injector].type != ADD_PAK_2_STROKE))
			{
			if (rDB.cur_injector < (MAX_DUMB_INJECTORS+10))
				rDB.add_inj.inj[rDB.cur_injector].port = AddPakBoard_1_Available();
			else if (rDB.cur_injector < MAX_INJECTORS)
				rDB.add_inj.inj[rDB.cur_injector].port = AddPakBoard_2_Available();
			}

		// Now, set the Type up!  -- This MUST BE DONE AFTER THE PORT SET-UP !!!!!
		pDB.add_inj.inj[rDB.cur_injector].type = temp_pdb_type /*ADD_PAK_INJ*/;
		rDB.add_inj.inj[rDB.cur_injector].type = temp_rdb_type /*ADD_PAK_INJ*/;


		// Set the timer 
		secTimer = sec_ticks;

		/* request current information from Add-Pack */
		SAI_GetState(rDB.cur_injector, PUMP);		   
		SAI_GetState(rDB.cur_injector, SOLENOID);
		SAI_ReadCount(rDB.cur_injector);

		// This flag will cause us to Poll on this injector once.
		rDB.add_inj.inj[rDB.cur_injector].pollInPrgmMode = TRUE;
		
		// Format the data for the screen
		sprintf(var_lit[0],litptr[addpack_inj_1_lit + rDB.cur_injector]);
		sprintf(var_lit[1],"%3.3s",litptr[off_lit + rDB.addpack[rDB.cur_injector].pump_state]);
		sprintf(var_lit[2],"%3.3s",litptr[off_lit + rDB.addpack[rDB.cur_injector].solenoid_state]);
		
		sprintf(var_lit[3],"%-6.6s %06d %3.3s %011.3f", litptr[addpack_counts_lit], rDB.addpack[rDB.cur_injector].pulses, 
		litptr[addpack_vol_lit], temp_SAI_totals[rDB.cur_injector]);
		
		for (i=0; i<4; i++)
			select_litptr[i] = var_lit[i];
			
		exitInProgress = FALSE;

		build_display(&menu_info, arm, &select_index, NULL);
		}

	else
		{
		if ((exitInProgress == FALSE) && ((sec_ticks - secTimer) != 0))
			{
			// Set the timer again.
			secTimer = sec_ticks;
			/* request current information from Add-Pack */
			SAI_GetState(rDB.cur_injector, PUMP);		   
			SAI_GetState(rDB.cur_injector, SOLENOID);
			SAI_ReadCount(rDB.cur_injector);
			// This flag will cause us to Poll on this injector once.
			rDB.add_inj.inj[rDB.cur_injector].pollInPrgmMode = TRUE;
			}


		sprintf(var_lit[0],litptr[addpack_inj_1_lit + rDB.cur_injector]);
		sprintf(var_lit[1],"%3.3s",litptr[off_lit + rDB.addpack[rDB.cur_injector].pump_state]);
		sprintf(var_lit[2],"%3.3s",litptr[off_lit + rDB.addpack[rDB.cur_injector].solenoid_state]);
		
		sprintf(var_lit[3],"%-6.6s %06d %3.3s %011.3f", litptr[addpack_counts_lit], rDB.addpack[rDB.cur_injector].pulses, 
			litptr[addpack_vol_lit], temp_SAI_totals[rDB.cur_injector]);

		switch(key)
		{
 			case RE_INIT:                                                                                           
				if ((exitInProgress == TRUE) && ((sec_ticks - secTimer) > 1))
					{
					exitInProgress = FALSE;
					/* restore actual injector programming */
					pDB.add_inj.inj[rDB.cur_injector].type = temp_pdb_type;
					rDB.add_inj.inj[rDB.cur_injector].type = temp_rdb_type;
					pDB.add_inj.inj[rDB.cur_injector].inj_address = temp_addr;

					// Reset the comm port!!!
					if ((pDB.add_inj.inj[rDB.cur_injector].type != ADD_PAK_INJ)	&& (pDB.add_inj.inj[rDB.cur_injector].type != ADD_PAK_2_STROKE))
						rDB.add_inj.inj[rDB.cur_injector].port = tempPort;

					return(prev_display());
					}
				else
					stamgr_event(RE_INIT,0);
				break;

			case CLEAR:				
					// Make sure the IO point is off when we leave this screen!!
					SAI_SetState(rDB.cur_injector, PUMP, 		0);
					SAI_SetState(rDB.cur_injector, SOLENOID, 	0);

					// Set the timer again.
					secTimer = sec_ticks;
					// Now give the commands a second to go out!
					stamgr_event(RE_INIT,0);
					exitInProgress = TRUE;
					break;

			case NEXT:
				next_page(PAGE_DOWN);
				break;
													   
			case UP:
			case DOWN:
				next_selection(key);
				break;

			case ENTER:
				switch (select_index)
				{
					case TOGGLE_PUMP:
						SAI_SetState(rDB.cur_injector, PUMP, !rDB.addpack[rDB.cur_injector].pump_state);
						break;

					case TOGGLE_SOLENOID:
						SAI_SetState(rDB.cur_injector, SOLENOID, !rDB.addpack[rDB.cur_injector].solenoid_state);
						break;

					case RESET_COUNTS:
						SAI_ResetCount(rDB.cur_injector);
						general_information(&litptr[counts_have_been_reset_lit]);
						break;

					default:
						break;
				}

			default:
				break;

		}
	 }
	return(inj_io_diag_menu);
};

/*-----------------------------------------------------------------------+
|                                                                        |
| 		Select AddPak Injector for Proving - Program Mode				 |
|                                                                        |
+-----------------------------------------------------------------------*/
void  *select_addpack_inject_menu(byte key, byte arm, byte init)
{
	//-------------------------------------------------------------------------
	static byte select_index;
	byte i;

	const static MENU_ITEM select_addpack_inject_menu_list1[]=
		{
		{ROW(1),COL(1),SYSTEM_FONT,WHITE,ENDPT(ROW(1),COL(31)),STRING,0,0,
			TITLE|CENTER,0,0,0,&litptr[addpack_diag_lit],0,0},

		{ROW(2),COL(3),SYSTEM_FONT,WHITE,"%s",STRING,0,0,
			SELECT1|LIST|NO_ENUM,TOTAL(NUM_INJ)|DISP(3)|ADD_MORE_LIT,
			0,0,&select_litptr[0],0,0},
		};

	const static MENU_PAGE menu_pages[]=
		{
		MENU_LIST(select_addpack_inject_menu_list1)
		};

	const static MENU_INFO menu_info=
		{
		select_addpack_inject_menu,
		(MENU_PAGE *)menu_pages,
		NUM_PAGES(menu_pages)
		};
	//-------------------------------------------------------------------------

	if (init == TRUE)
	{	
		for (i=MAX_DUMB_INJECTORS; i<MAX_INJECTORS; i++)
		{
			if (i >= pDB.num_inj_used)
				select_litptr[i-MAX_DUMB_INJECTORS] = NULL;
			else if (rDB.add_inj.inj[i].type == ADD_PAK_INJ)
				select_litptr[i-MAX_DUMB_INJECTORS] = litptr[inj_1_lit + i];
			else if (rDB.add_inj.inj[i].type == ADD_PAK_2_STROKE)
				select_litptr[i-MAX_DUMB_INJECTORS] = litptr[inj_1_lit + i];
			else
				select_litptr[i-MAX_DUMB_INJECTORS] = NULL;
		}

		build_display(&menu_info, arm, &select_index, NULL);
	}
	else
	{
		switch(key)
		{
			case CLEAR:
				return(prev_display());

			case NEXT:
				next_page(PAGE_DOWN);
				break;

			case UP:
			case DOWN:
				next_selection(key);
				break;

			case ENTER:
				switch (select_index)
				{
					case INJ5: 
				   	case INJ6: 
					case INJ7: 
				   	case INJ8: 
					case INJ9: 
				   	case INJ10:
					case INJ11:
				   	case INJ12:
					case INJ13:
					case INJ14:
					case INJ15:
				   	case INJ16:
					case INJ17:
				   	case INJ18:
					case INJ19:
				   	case INJ20:
					case INJ21:
				   	case INJ22:
					case INJ23:
					case INJ24:
						rDB.cur_injector = select_index + MAX_DUMB_INJECTORS;
						if ((rDB.add_inj.inj[rDB.cur_injector].type != ADD_PAK_INJ) && 
						(rDB.add_inj.inj[rDB.cur_injector].type != ADD_PAK_2_STROKE))
						{
							general_information(&litptr[inj_not_cfg_lit]);							
							break;
						}
						else
						{
							injPrvEntered = AICB_PROVING;
							return(inj_test_menu);
						}

					default:
						break;
				}
			default:
				break;
		}
	}

	return(select_addpack_inject_menu);
};


// Small fcn to let us know if there is an alarm active 
// on this injector, so we don't do anything with it.
unsigned char InjectorAlarmActive(unsigned char inj)
{
	unsigned char i;

	for (i=0; i<last_injector_alarm; i++)
		if (rDB.inj_alarm[inj][i] != 0)
			return(TRUE);

	return(FALSE);
}

/************************************************/
/*---------------------------------------------------+
| 		AddPak Prove Screen - Program Mode			 |
+---------------------------------------------------*/
#pragma sep_on
extern float tempVol;
extern float InjProvTotal;
extern unsigned char InjPerRequest;
extern byte authorizedToInject;
#pragma sep_off
extern struct data_base  fpMtrInjVolCalc[]; 
extern struct data_base UN_CH_inj_prv[]; 
        
void  *inj_test_menu(byte key, byte arm, byte init)
{
	//-------------------------------------------------------------------------------------------------
	enum {	
		INJ_VOL,
		AUTH_DEAUTH,
		INJ_PER_REQUEST_PROVE,
		INJ_PROVE_CONTINUE,
		NUM_TEST_DIAGS };

	static byte select_index;
	static byte inject_counts;
	static byte injInProgress;
	unsigned char i;

	static unsigned long 	secTimer = 0;
	//static unsigned char 	reformatData;
	static float 			startInjVol;
	static float 			curTestInjVol;

	const static MENU_ITEM inj_test_menu_list[]=
		{
		{ROW(1),COL(1),SYSTEM_FONT,WHITE,ENDPT(ROW(1),COL(31)),STRING,0,0,
			TITLE|CENTER,0,0,0,&select_litptr[0],0,0},

		{ROW(2),COL(3),SYSTEM_FONT,WHITE,"%s",STRING,0,0,
			SELECT1|LIST|NO_ENUM,TOTAL(NUM_TEST_DIAGS)|DISP(NUM_TEST_DIAGS),
			0,0,&litptr[inj_vol_lit],0,0},

		{ROW(2),COL(18),SYSTEM_FONT,WHITE,"%8.3f",DYNAMIC|FLOAT,WHOLE(4),FRAC(3),INPUT,FIELD(INJ_VOL),
			0,&fpMtrInjVolCalc[inj_test_vol],0,&litptr[inj_prove_total_hlit],0},

		{ROW(2),COL(27),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			VAR_LIT,0,0,0,&select_litptr[1],0,0},

	   	{ROW(3),COL(16),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			VAR_LIT,0,0,0,&select_litptr[2],0,0},

		{ROW(4),COL(22),SYSTEM_FONT,WHITE,"%8.8s",STRING,0,0,
			VAR_LIT,FIELD(INJ_PER_REQUEST_PROVE),0,&UN_CH_inj_prv[inj_per_req],&litptr[inj_prove_single_inj_lit],0,0},
	   		   	
	   	/*{ROW(4),COL(22),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			VAR_LIT,0,0,0,&select_litptr[3],0,0},*/

		//{ROW(5),COL(1),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			//VAR_LIT,0,0,0,&select_litptr[4],0,0},
		};

	  
	  static const MENU_ITEM inj_req_select =
		{ROW(3),COL(3),SYSTEM_FONT,WHITE,"%s",STRING,0,0,
			SELECT1|LIST|NO_ENUM,TOTAL(NUM_INJ_PER_REQUEST)|DISP(NUM_INJ_PER_REQUEST),
			0,&UN_CH_inj_prv[inj_per_req],&litptr[inj_prove_single_inj_lit],0/*&litptr[inj_per_request_hlit]*/,0};


	const static MENU_PAGE menu_pages[]=
		{
		MENU_LIST(inj_test_menu_list)
		};

	const static MENU_INFO menu_info=
		{
		MtrInjTestMenu,
		(MENU_PAGE *)menu_pages,
		NUM_PAGES(menu_pages)
		};
	//-------------------------------------------------------------------------------------------------


	if (init == TRUE)
		{
		authorizedToInject 			= FALSE;
		injInProgress	= FALSE;
		inject_counts 	= 0;
		tempVol 		= 0;	
		startInjVol		= temp_SAI_totals[rDB.cur_injector];
		curTestInjVol	= 0;
		InjPerRequest  	= SINGLE_INJ_PER_REQUEST;


		// Format the screen
		sprintf(var_lit[0], "%14.14s #%d", 	litptr[prove_additive_lit], (rDB.cur_injector + 1));
		sprintf(var_lit[1],	"%s",			&pDB.add_inj.inj_units[0]);
		sprintf(var_lit[2],	"%14.14s",		litptr[deauth_inj_lit]);
		sprintf(var_lit[3], "%8.8s",		litptr[inj_prove_single_inj_lit]);
		
		for (i=0; i<4; i++)
			select_litptr[i] = var_lit[i];
		
		// Grab the current time, so we can periodically grab data.
		//secTimer = sec_ticks;
		// Don't need to reformat the data right now.
		//reformatData = FALSE;

		// reset the counts, as a nice'ity
		// UPDATE:  This isn't a nicety anymore!  We are going to use the counts to 
		//			facilitate proving calculations!!!!
		//SAI_ResetCount(rDB.cur_injector);
		// Deauthorize the injector, JUST...in case...
		rDB.addpack[rDB.cur_injector].diag_deauth_flag = TRUE;
		build_display(&menu_info, arm, &select_index, NULL);
		}

	else
		{

		if ((hun_ms_ticks - secTimer) >= 10)
			{
			// Set the timer again. Timer needed so aren't constantly sending
			//communication commands to addpak, that delays sending the command
			//to turn on additive pump DURING PROVING
			secTimer = hun_ms_ticks;
			// This flag will cause us to Poll on this injector once.
	   		rDB.add_inj.inj[rDB.cur_injector].pollInPrgmMode = TRUE;

			// Keep getting the solonoid and pump states, so we know 
			// just what we are doing.
	   		SAI_GetState(rDB.cur_injector, SOLENOID);
	   		SAI_GetState(rDB.cur_injector, PUMP);
	   	   }			   

	  
		switch(key)
			{
			case CLEAR:
				rDB.addpack[rDB.cur_injector].diag_deauth_flag = TRUE;

				if (curTestInjVol > 0)										// If we got volume, go on
					{
					// (float) = (unsigned int), so typecast it to a float!
					tempInjProvePulses = (float)rDB.addpack[rDB.cur_injector].pulses;
					return(MtrInjRealVolEntryMenu);
					}
				else
					{
					return(prev_display());									// Else, back to the main menu
					}

			case NEXT:
				next_page(PAGE_DOWN);
				break;
													   
			case UP:
			case DOWN:
				next_selection(key);
				break;

			//case REMOTE_MTR_INJ_INJECT_PROVE:
			//	complete_aicb_prove_inject(&injInProgress, &auth, &inject_counts);
			//break;
						
			case ENTER:
				switch (select_index)
					{
					case INJ_VOL:
						// If an alarm is set, Let us know about it, and don't allow us to program it.
						if (InjectorAlarmActive(rDB.cur_injector))
							general_information(&litptr[inj_alarm_present_lit]);

						else
							default_input_screen(NULL,DEFAULT_KEY_HANDLER);
						break;

					case AUTH_DEAUTH:
						// If an alarm is set, Let us know about it, and don't authorize!
						if (InjectorAlarmActive(rDB.cur_injector))
							general_information(&litptr[inj_alarm_present_lit]);

						// If we don't have a volume programed, let us know.
						else if (tempVol == 0)
							general_information(&litptr[enter_volume_lit]);

						// If we're authorized, then deauthorize
						else if ((authorizedToInject) && (rDB.addpack[rDB.cur_injector].pump_state != 0))
							{
							sprintf(var_lit[2],"%14.14s",litptr[deauth_inj_lit]);
							authorizedToInject = FALSE;
							rDB.addpack[rDB.cur_injector].diag_deauth_flag = TRUE;
							}

						// Else we're not authorized, so authorize the thing.
						// Make sure that we're not authorized in the first place
						else if ((!authorizedToInject) && (rDB.addpack[rDB.cur_injector].pump_state == 0))
							{
							// Need to stuff the temp value here, or it won't get xmitted
							rDB.add_inj.inj[rDB.cur_injector].vol_per_cycle = tempVol;
							sprintf(var_lit[2],"%14.14s",litptr[auth_inj_lit]);
							authorizedToInject = TRUE;
							inject_counts = 0;
							//sprintf(var_lit[2],"%s #%2d",litptr[injection_lit], inject_counts);
							SAI_ResetCount(rDB.cur_injector);
							SAI_VolPerCycle(rDB.cur_injector);
							rDB.addpack[rDB.cur_injector].diag_auth_flag = TRUE;
							}
						break;	
						
					
				   	case INJ_PER_REQUEST_PROVE:
						// If an alarm is set, Let us know about it, and don't allow us to program it.
						if (InjectorAlarmActive(rDB.cur_injector))
							general_information(&litptr[inj_alarm_present_lit]);
						else
							default_input_screen(&inj_req_select,DEFAULT_KEY_HANDLER);
					   	break;		
				   	
				   	
				   	
				   	case INJ_PROVE_CONTINUE:
						if (tempVol <= 0)
							general_information(&litptr[enter_volume_lit]);
						else if (!authorizedToInject)
							 general_information(&litptr[inj_must_be_auth_lit]);
						else 
							return(AICBTestMenu2);
						break;				   
						
					
					
					default:
						break;
					}

			default:
				break;
			}
		}

	return(inj_test_menu);
};


void  *AICBTestMenu2(byte key, byte arm, byte init)
{
	//-------------------------------------------------------------------------------------------------
	enum {	
		PROVE_TOTAL_INJ_AMT,
		PROVE_INJECT,
		NUM_PROVE_SINGLE };

	
	static unsigned char selectIndex;
	static unsigned char injectCounts;
	static double last_prv_inj;
	static double avg_prv_inj;
	static double old_additive_total;
	static unsigned int num_multiple_prove_request;
	static unsigned long time_of_last_auto_inject;
	static unsigned char count_down_to_next_inject;
	static unsigned long sec_timer;
	static float start_inj_vol;
	static float cur_test_inj_vol;
	static unsigned char reformat_data;  

	//static BOOL authorizedToInject;
	static byte injInProgress;

	register unsigned char i;

	const static MENU_ITEM aicb_single_inj_test_menu_list[]=
	{
		{ROW(1),COL(1),SYSTEM_FONT,WHITE,ENDPT(ROW(1),COL(31)),STRING,0,0,
			TITLE|CENTER,0,0,0,&select_litptr[0],0,0},

		{ROW(2),COL(3),SYSTEM_FONT,WHITE,"%s",STRING,0,0,
			SELECT1|LIST|NO_ENUM,TOTAL(NUM_PROVE_SINGLE)|DISP(NUM_PROVE_SINGLE),
			0,0,&select_litptr[1],0,0},

		//select_litptr[2] could be a select item....do not use
		
		{ROW(2),COL(16),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			VAR_LIT,0,0,0,&select_litptr[4],0,0},

	   	/*{ROW(3),COL(16),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			VAR_LIT,0,0,0,&select_litptr[1],0,0},*/

	   	{ROW(4),COL(1),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			VAR_LIT,0,0,0,&select_litptr[5],0,0},

		{ROW(5),COL(1),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			VAR_LIT,0,0,0,&select_litptr[6],0,0},
	};

	const static MENU_ITEM aicb_multiple_inj_test_menu_list[]=
	{
		{ROW(1),COL(1),SYSTEM_FONT,WHITE,ENDPT(ROW(1),COL(31)),STRING,0,0,
			TITLE|CENTER,0,0,0,&select_litptr[0],0,0},

		{ROW(2),COL(3),SYSTEM_FONT,WHITE,"%s",STRING,0,0,
			SELECT1|LIST|NO_ENUM,TOTAL(NUM_PROVE_SINGLE)|DISP(NUM_PROVE_SINGLE),
			0,0,&select_litptr[1],0,0},

		//select_litptr[2] could be a select item....do not use
		
		{ROW(2),COL(16),SYSTEM_FONT,WHITE,"%10.3f",DYNAMIC|FLOAT,WHOLE(6),FRAC(3),INPUT,FIELD(PROVE_TOTAL_INJ_AMT),
			0,&fpMtrInjVolCalc[inj_prove_total_amt],0,&litptr[inj_prove_total_hlit],0},

		{ROW(2),COL(27),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			VAR_LIT,0,0,0,&select_litptr[3],0,0},
		
	   	{ROW(3),COL(16),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			VAR_LIT,0,0,0,&select_litptr[4],0,0},

	   	{ROW(4),COL(1),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			VAR_LIT,0,0,0,&select_litptr[5],0,0},

		{ROW(5),COL(1),SYSTEM_FONT,WHITE,"%s",DYNAMIC|TEXT,0,0,
			VAR_LIT,0,0,0,&select_litptr[6],0,0},
	};


		

	const static MENU_PAGE aicb_single_menu_pages[]=
		{
		MENU_LIST(aicb_single_inj_test_menu_list)
		};

	const static MENU_INFO aicb_single_menu_info=
		{
		AICBTestMenu2,
		(MENU_PAGE *)aicb_single_menu_pages,
		NUM_PAGES(aicb_single_menu_pages)
		};

	
	const static MENU_PAGE aicb_multiple_menu_pages[]=
		{
		MENU_LIST(aicb_multiple_inj_test_menu_list)
		};

	const static MENU_INFO aicb_multiple_menu_info=
		{
		AICBTestMenu2,
		(MENU_PAGE *)aicb_multiple_menu_pages,
		NUM_PAGES(aicb_multiple_menu_pages)
		};
		
	//-------------------------------------------------------------------------------------------------


	//-------------------------------------------------------------------------------------------------
	if (init == TRUE)
	{
		// Set-up the local parameters
		injInProgress		= BOOL_FALSE;
		injectCounts 		= 0;
		last_prv_inj 	   	= 0;
		avg_prv_inj			= 0;
		old_additive_total	= 0;
		num_multiple_prove_request = 0;
		count_down_to_next_inject = FALSE;
		InjProvTotal = 0;
		start_inj_vol = temp_SAI_totals[rDB.cur_injector];
		cur_test_inj_vol = 0;
		//tempVol 			= 0;	


		sprintf(var_lit[3],	"%s",			&pDB.add_inj.inj_units[0]);
		sprintf(var_lit[4], "%s #%2d",	litptr[injection_lit], 		injectCounts);
		sprintf(var_lit[5], "%4.4s %9.3f %3.3s %10.3f", litptr[inj_prove_last_lit], last_prv_inj  ,litptr[inj_prove_avg_lit], avg_prv_inj);
		sprintf(var_lit[6],	"%-6.6s %6d %3.3s %11.3f",litptr[addpack_counts_lit], rDB.addpack[rDB.cur_injector].pulses, 
														litptr[addpack_vol_lit], cur_test_inj_vol);		
		for (i=3; i<7; i++)
			select_litptr[i] = var_lit[i];
		
		// Grab the current time, so we can periodically grab data.
		sec_timer = sec_ticks;
		// Don't need to reformat the data right now.
		reformat_data = FALSE;

		// reset the counts, as a nice'ity
		// UPDATE:  This isn't a nicety anymore!  We are going to use the counts to 
		//			facilitate proving calculations!!!!
		SAI_ResetCount(rDB.cur_injector);
		
		if(InjPerRequest == SINGLE_INJ_PER_REQUEST)
		{	
			select_litptr[1] = NULL;
			select_litptr[2] = litptr[inject_lit];
			build_display(&aicb_single_menu_info, arm, &selectIndex, NULL);
		}
		else
		{
			//MULTIPLE_INJ_PER_REQUEST
			select_litptr[1] = litptr[inj_prove_total_lit];
			select_litptr[2] = litptr[inject_lit];
			build_display(&aicb_multiple_menu_info, arm, &selectIndex, NULL);
		}
	}
	//-------------------------------------------------------------------------------------------------


	//-------------------------------------------------------------------------------------------------
	else
   	{

   		// Set up the volume in the database, or we won't run!!
   		//if (tempVol > 0)
		//	rDB.add_inj.inj[rDB.cur_injector].vol_per_cycle = tempVol;
   		if((hun_ms_ticks - sec_timer) >= 10) //10 represents 1 second 
		{
			//get timer again
			sec_timer = hun_ms_ticks;
			reformat_data = TRUE;
			//request current information from add-pak
			SAI_ReadCount(rDB.cur_injector);
			//this  flag will cause us to poll on this injector once
			rDB.add_inj.inj[rDB.cur_injector].pollInPrgmMode = TRUE;

			// Keep getting the solonoid and pump states, so we know 
			// just what we are doing.
			SAI_GetState(rDB.cur_injector, SOLENOID);
			SAI_GetState(rDB.cur_injector, PUMP);		   

			cur_test_inj_vol = temp_SAI_totals[rDB.cur_injector] - start_inj_vol;
		}

		else if(reformat_data)
		{
			reformat_data = FALSE;
			
			last_prv_inj =  cur_test_inj_vol - old_additive_total;
			if(injectCounts > 0)
				avg_prv_inj = cur_test_inj_vol/ (double)injectCounts;
			
			// Re-format the dynamic stuff!
			sprintf(var_lit[3],	"%s",			&pDB.add_inj.inj_units[0]);
			sprintf(var_lit[4], "%s #%3d",	litptr[injection_lit], 		injectCounts);
			sprintf(var_lit[5], "%4.4s %9.3f %3.3s %10.3f", litptr[inj_prove_last_lit], last_prv_inj  ,litptr[inj_prove_avg_lit], avg_prv_inj);
			sprintf(var_lit[6],	"%-6.6s %6d %3.3s %11.3f",litptr[addpack_counts_lit], rDB.addpack[rDB.cur_injector].pulses, 
														litptr[addpack_vol_lit], cur_test_inj_vol);
			for (i=3; i<7; i++)
				select_litptr[i] = var_lit[i];

			// Update the screen
			//update_dbSelect_buffer(SELECT1);
		

			// If we were performing an injection, AND the solenoid closed, flag that the injection is done.
			if ((injInProgress == BOOL_TRUE) && (rDB.addpack[rDB.cur_injector].solenoid_state == 0))
			{
				injInProgress = BOOL_FALSE;
				if(InjPerRequest == MULTIPLE_INJ_PER_REQUEST)
					set_time_to_next_inject(num_multiple_prove_request,&time_of_last_auto_inject, &count_down_to_next_inject);
			}
			else if(count_down_to_next_inject &&
					 kickoff_next_auto_inject(&num_multiple_prove_request, &time_of_last_auto_inject, cur_test_inj_vol))
			{
				count_down_to_next_inject = FALSE;	
				num_multiple_prove_request--;
				old_additive_total = cur_test_inj_vol;
				complete_aicb_prove_inject(&injInProgress, &authorizedToInject, &injectCounts);
			}

	   	}
		//---------- KEY HANDLER -----------------------------------------------------
		switch(key)
			{
			case CLEAR:
				
				rDB.addpack[rDB.cur_injector].diag_deauth_flag = TRUE;
				authorizedToInject = FALSE;
				if (cur_test_inj_vol > 0)										// If we got volume, go on
					{
					// (float) = (unsigned int), so typecast it to a float!
					tempInjProvePulses = (float)rDB.addpack[rDB.cur_injector].pulses;
					return(MtrInjRealVolEntryMenu);
					}
				else
					return(prev_display());									// Else, back to the main menu
			
			case NEXT:
				next_page(PAGE_DOWN);
				break;
													   
			case UP:
			case DOWN:
				next_selection(key);
				break;

			case REMOTE_MTR_INJ_INJECT_PROVE:
				if((InjPerRequest == MULTIPLE_INJ_PER_REQUEST) && (InjProvTotal == 0.0))
				{
					general_information(&litptr[total_amt_greater_than_zero_lit]);
					break;	
				}
				if(num_multiple_prove_request > 0)
					break;//we are set up to inject automatically without pressing any keys or digital inputs
				if((InjPerRequest == MULTIPLE_INJ_PER_REQUEST) && (InjProvTotal > 0.0))
					set_up_multiple_proving(&num_multiple_prove_request, cur_test_inj_vol);
				old_additive_total = cur_test_inj_vol;
				complete_aicb_prove_inject(&injInProgress, &authorizedToInject, &injectCounts);
		   	break;
			
			case ENTER:
				switch (selectIndex)
				{
				   case PROVE_TOTAL_INJ_AMT: 
				   		// If an alarm is set, Let us know about it, and don't allow us to program it.
						if (InjectorAlarmActive(rDB.cur_injector))
							general_information(&litptr[inj_alarm_present_lit]);
						else
							default_input_screen(NULL,DEFAULT_KEY_HANDLER);
						break;
				   								
				   
				   
				   case PROVE_INJECT:
						if((InjPerRequest == MULTIPLE_INJ_PER_REQUEST) && (InjProvTotal == 0.0))
						{
							general_information(&litptr[total_amt_greater_than_zero_lit]);
							break;	
						}
						if(num_multiple_prove_request > 0)
							break;//we are set up to inject automatically without pressing any keys or digital inputs
						if((InjPerRequest == MULTIPLE_INJ_PER_REQUEST) && (InjProvTotal > 0.0))
							set_up_multiple_proving(&num_multiple_prove_request, cur_test_inj_vol);
						old_additive_total = cur_test_inj_vol;
						complete_aicb_prove_inject(&injInProgress, &authorizedToInject, &injectCounts);
						break;

					default:
						break;
				}

			default:
				break;
			}
		//---------- KEY HANDLER END -------------------------------------------------

		}

	return(AICBTestMenu2);
};

/***********************************************************************************/
void complete_aicb_prove_inject(byte *injInProgress, byte *auth, byte *injectCounts)
{
	// If an alarm is set, Let us know about it, and don't INJECT!
	if (InjectorAlarmActive(rDB.cur_injector))
		general_information(&litptr[inj_alarm_present_lit]);

	// If we are not authorized, don't do anything.
	else if (*auth == FALSE)
		general_information(&litptr[inj_must_be_auth_lit]);

	// if we are NOT performing an injection, AND the Solenoid is OFF...
	else if ((*injInProgress == FALSE) && (rDB.addpack[rDB.cur_injector].solenoid_state == 0))
	{
		*injInProgress = BOOL_TRUE;
		sprintf(var_lit[2],"%s #%2d",litptr[injection_lit], ++(*injectCounts));
	    SAI_II(rDB.cur_injector);
		SAI_GetState(rDB.cur_injector, SOLENOID);
	}
}



/***********************************************************************************/

