/**
 * fieldinit.cpp
 *
 *  @date: 	Jan 3, 2014
 *  @author: 	FMCNET\sutter
 *
 *  Database initializations useful for testing.
 *  	Includes straight, sequential and ratio blend configurations
 *
 *
 *  @section LICENSE
 *  
 *  Copyright 2014 FMC Technologies Measurement Solutions, Inc.
 *
     This file contains proprietary source code which is the sole property
    of FMC Technologies Measurement Solutions. Any duplication or distribution
    in whole or in part is strictly prohibited.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
 *	
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>

#include "sqlite3.h"
#include "database.h"
#include "../alarms.h"


const char *hm_class_1 = "Flammable Liquid! In case of s";
const char *hm_class_2 = "pillage contact DOT.";

/***********************************************************************************************************************************************************
 *
 * Database Field Test Initialization and Test Functions
 *
 ***********************************************************************************************************************************************************/
#ifdef zzz
int seq_blend_arm_init(sqlite3 *db, int arm)
{
	int i, rc, recipe_no;
	char sql[1200], arm_id[50];
	int arm_rcp[MAX_LOAD_ARMS] = {1,10,20,30,40,45};

	recipe_no = arm_rcp[arm];

	rc = SQLITE_OK;

	sqlite3_snprintf(sizeof(arm_id), arm_id, "Arm %d", arm+1);
}
#endif

int A4M_only = false;	// used to test 6 arms on A4M (all i/o will be configured using A4M only)

int std_field_test_init(sqlite3 *db, int num_arms, int units)
{
	int i, rc, arm, prd, dig_in, dig_out, pulse_out_rate, perm_start, a4b_available;
	float min_temp, max_temp, min_dens, max_dens, min_pres, max_pres;
	char sql[300];

	rc = SQLITE_OK;


	rc = sqlite3_exec(db, "BEGIN IMMEDIATE TRANSACTION", NULL, NULL, NULL);

	a4b_available = (!A4M_only && (num_arms > 3)) ? true:false;

	// clear out factory default recipe assignments
	rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used = 0, recipe_name = ''", NULL,NULL,NULL);

	// configure desired # of arms.  all arms are straight except arm 3 is a sequential blender
	sqlite3_snprintf(sizeof(sql), sql, "UPDATE load_arm_layout SET num_physical_arms=%d, arm1_config = 0, arm1_num_prds=1, arm2_config = 0, arm2_num_prds=1, "
			"arm3_config = 1, arm3_num_prds=2, arm4_config = 0, arm4_num_prds=1, arm5_config = 0, arm5_num_prds=1, arm6_config = 0, arm6_num_prds=1, "
			"a4b_available=%d", num_arms, a4b_available);
	rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);

	for (arm=0, dig_out=1; arm<num_arms; arm++)
	{
		// digital outputs 1-6 on A4M (1-3 are DC and 4-6 are AC) used for arms 1-3 up/downstream solenoids
		// digital outputs 31-36 on A4B (Bi-state DC outputs) used for arms 4-6 up/downstream solenoids
		if (a4b_available && (arm >= 3) && (dig_out <= FIRST_BIO_DIG_OUT))
			dig_out = FIRST_BIO_DIG_OUT+1;		// make it one based

		sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_func=%d, dig_out_arm=%d, dig_out_mtr=0 WHERE dig_out_config_id=%d", UPSTREAM_SOL, arm, dig_out++);
		rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_func=%d, dig_out_arm=%d, dig_out_mtr=0 WHERE dig_out_config_id=%d", DOWNSTREAM_SOL, arm, dig_out++);
		rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
	}

	// set dig_out to next available output on A4M
	if (a4b_available)
		dig_out = 7;

	// Use AC inputs (dig_in 7-11 on A4M and 12-15 on A4B)
	for (arm=0, dig_in=7; arm<num_arms; arm++)
	{
		if (a4b_available && (arm >= 3) && (dig_out <= FIRST_A4B_DIG_OUT))
			dig_out = FIRST_A4B_DIG_OUT+1;		// make it one based

		if (a4b_available && (arm >= 3) && (dig_in <= FIRST_A4B_DIG_IN))
			dig_in = FIRST_A4B_DIG_IN+1;		// make it one based

		if (a4b_available || (dig_in <= NUM_A4M_DIG_INPUTS))
		{
//			sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_in_config SET dig_in_func=%d, dig_in_arm=%d WHERE dig_in_config_id=%d", PERMISSIVE_1, arm, dig_in++);
//			rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
//			sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_in_config SET dig_in_func=%d, dig_in_arm=%d WHERE dig_in_config_id=%d", PERMISSIVE_2, arm, dig_in++);
//			rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
		}
#ifdef zzz
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_func=%d, dig_out_arm=%d, dig_out_mtr=0 WHERE dig_out_config_id=%d", UPSTREAM_SOL, arm, dig_out++);
		rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_func=%d, dig_out_arm=%d, dig_out_mtr=0 WHERE dig_out_config_id=%d", DOWNSTREAM_SOL, arm, dig_out++);
		rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
#endif

		// if A4B is not available, we may not have enough outputs for all arms to have a pump (ALSO LEAVE ROOM FOR BLOCK VALVE AND INJECTORS)
		if (a4b_available || (dig_out <= (NUM_A4M_DIG_OUTPUTS-4)))
		{
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_func=%d, dig_out_arm=%d, dig_out_mtr=0 WHERE dig_out_config_id=%d", PUMP_OUT, arm, dig_out++);
			rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
		}
	}

	if (num_arms >= 3)
	{
		if (a4b_available)      // arms 1-3 on A4M
			dig_out = 11;   //      1-6 up/down solenoids, 7-9 pumps, 11-12 block valves, 13-14 piston injector
		else if (dig_out <= 11) // all arms on A4M
			dig_out = 11;   //      if enough outputs remaining, leave space for injectors on 13-14
		else
			dig_out = 13;   // no room left for injectors

		for (prd=0, arm=ARM_3; prd<2; prd++)
		{
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_func=%d, dig_out_arm=%d, dig_out_prd=%d WHERE dig_out_config_id=%d", BLOCK_VALVE, arm, prd, dig_out++);
			rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
		}
	}

	if (num_arms >= 2)
	{
		dig_in = 10;

		// configure detect switch inputs for arm #2.  NOTE: dig_in_arm is 0 based
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_in_config SET dig_in_func=%d, dig_in_arm=1 WHERE dig_in_config_id=%d", SWING_ARM_DETECT_1, dig_in++);
		rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_in_config SET dig_in_func=%d, dig_in_arm=1 WHERE dig_in_config_id=%d", SWING_ARM_DETECT_2, dig_in++);
		rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
	}

//TODO: Remove these defined sections on production release MLA 050115
#define MATT
#define MTR_INJ
//#define TITAN
//#define MINIPAK
//#define AICB

	// if enough outputs left on A4M, configure a piston injector (and other smart injectors as needed for testing -- MLA 022015)
	if (dig_out <= 13)
	{
#ifdef MATT
		// set up some injectors and update recipe #1 --MLA
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE serial_port_config SET comm_protocol=6, baud_rate=1, data_parity=3 where serial_port_config_id=3");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE inj_sys_config SET inj_units_desc='cc', inj_totals_desc='gal', inj_conversion_factor=3785.412");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE injector_config SET inj_tag='Piston Inj 1', inj_type=1, inj_arm=0, inj_plumbing=1 WHERE injector_config_id=1");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_inj_config SET inj_vol=200, inj_rate=100, inj_prods=1 WHERE recipe_no=1 and inj_no=1");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_tag='PistonInj #1', dig_out_func=%d WHERE dig_out_config_id=21", PISTON_INJ_1);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_tag='AddPump #1', dig_out_func=%d WHERE dig_out_config_id=22", ADDITIVE_PUMP_1);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);

#ifdef MTR_INJ
		/********   METERED INJECTORS (#2 & #3 & #4)  ****************/
		for(int inj_no=2;inj_no<5;inj_no++)
		{
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE injector_config SET inj_tag='METER_INJ %d', inj_type=%d, inj_arm=0, inj_plumbing=1, inj_address=%d, inj_k_factor=5000, inj_meter_factor=1.0000, inj_high_tol=300, inj_low_tol=50, max_inj_tol_errors=5 WHERE injector_config_id=%d", inj_no, METER_INJ, inj_no+400, inj_no);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		}
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_inj_config SET inj_vol=50, inj_rate=50 WHERE recipe_no=1 and inj_no=2");
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_inj_config SET inj_vol=50, inj_rate=100 WHERE recipe_no=1 and inj_no=3");
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_inj_config SET inj_vol=100, inj_rate=50 WHERE recipe_no=1 and inj_no=4");
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
#endif

#ifdef AICB
		/********		AICB SETUP	(#1)  INJECTORS #5-10  *****************/
		int inj_no = 5; // 1-based start for injector configs
		int inj_add;
		for (inj_add=101; inj_add<106; inj_add++) // setting up first 5 injectors in AICB Simulator
		{
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE injector_config SET inj_tag='AICB %d', inj_type=8, inj_arm=0, inj_plumbing=1, inj_address=%d, inj_k_factor=5000, inj_meter_factor=1.0000, inj_high_tol=10, inj_low_tol=10, max_inj_tol_errors=0 WHERE injector_config_id=%d", inj_add, inj_add, inj_no);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			inj_no++;
		}

		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_inj_config SET inj_vol=200, inj_rate=100 WHERE recipe_no=1 and inj_no=5");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_inj_config SET inj_vol=50, inj_rate=50 WHERE recipe_no=1 and inj_no=6");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_inj_config SET inj_vol=50, inj_rate=100 WHERE recipe_no=1 and inj_no=7");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_inj_config SET inj_vol=100, inj_rate=50 WHERE recipe_no=1 and inj_no=8");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_inj_config SET inj_vol=100, inj_rate=200 WHERE recipe_no=1 and inj_no=9");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);

		for (int AICB_input=29; AICB_input<34; AICB_input++) //Last 5 inputs of the AICB #1 29-33
		{
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_in_config SET dig_in_tag='AICB_GEN_INPUT', dig_in_func=%d WHERE dig_in_config_id=%d", GENERAL_PURPOSE_IN, AICB_input);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		}
		for (int AICB_output=49; AICB_output<59; AICB_output++) //Last 10 outputs of AICB #1 49-58
		{
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_tag='AICB_GEN_OUTPUT', dig_out_func=%d WHERE dig_out_config_id=%d", GENERAL_PURPOSE_OUT, AICB_output);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		}
#endif

#ifdef TITAN
		/*********		TITAN SETUP		*******************/
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE injector_config SET inj_tag='Titan 009', inj_type=3, inj_arm=0, inj_plumbing=1, inj_address=9 WHERE injector_config_id=6");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_inj_config SET inj_vol=40, inj_rate=100 WHERE recipe_no=1 and inj_no=6");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
#endif

#ifdef MINIPAK
		/********		BLENDPAK SETUP		***************/
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE injector_config SET inj_tag='BlendPak 1', inj_type=4, inj_arm=0, inj_plumbing=1, inj_address=72 WHERE injector_config_id=3");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_inj_config SET inj_vol=10.0, inj_rate=20.0, inj_prods=1 WHERE recipe_no=1 and inj_no=3");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);


		/********		MINIPAK SETUP		***************/
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE injector_config SET inj_tag='MiniPak 1', inj_type=5, inj_arm=0, inj_plumbing=1, inj_address=83 WHERE injector_config_id=4");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_inj_config SET inj_vol=20.0, inj_rate=20.0, inj_prods=1 WHERE recipe_no=1 and inj_no=4");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
#endif
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE system_config SET flow_simulator=1");
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
#endif
	}

	// initialize bay hazardous material class to last arm available for bay B to prevent criticals
	sqlite3_snprintf(sizeof(sql), sql, "UPDATE bay_config SET rep_hm_bay=%d WHERE bay_config_id=2", num_arms-1);
	rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);


	if (units == US_CUSTOMARY_UNITS) {
		min_temp = 20; max_temp = 120; min_dens = 30; max_dens = 80, min_pres = 0, max_pres = 300;
	}
	else {
		min_temp = -30; max_temp = 50; min_dens = 500; max_dens = 1100, min_pres = 0, max_pres = 2000;
	}

	(void)min_dens; (void)max_dens;

	for (arm=0, i=0; arm<num_arms; arm++, i++)
	{
		if (arm == 0)
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE analog_io_config SET analog_io_tag='TP100%d', analog_io_func=%d, analog_io_arm=%d, analog_io_mtr=0, "
				" analog_io_type=%d WHERE analog_io_config_id=%d", i+1, TEMPERATURE_IN, arm, RTD, i+1);
		else
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE analog_io_config SET analog_io_tag='TP100%d', analog_io_func=%d, analog_io_arm=%d, analog_io_mtr=0, "
				" analog_io_type=%d, ana_io_min_value=%f, ana_io_max_value=%f WHERE analog_io_config_id=%d", i+1, TEMPERATURE_IN, arm, MA_IN, min_temp, max_temp, i+1);
		rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
	}
#ifdef zzz // live density only used by arms 5 and 6 below, so there would never be enough analog inputs
	// use any remaining analog i/o for density inputs
	for (arm=0; arm<num_arms; arm++, i++)
	{
		if (i >= MAX_ANALOG_IO)
			break;
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE analog_io_config SET analog_io_tag='DT100%d', analog_io_func=%d, analog_io_arm=%d, analog_io_mtr=0, "
				" analog_io_type=%d, ana_io_min_value=%f, ana_io_max_value=%f WHERE analog_io_config_id=%d", i+1, DENSITY_IN, arm, MA_IN, min_dens, max_dens, i+1);
		rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
	}
#endif
	// if available, assign an analog i/o as an output
	if (i < MAX_ANALOG_IO)
	{
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE analog_io_config SET analog_io_tag='Flow A1', analog_io_func=%d, analog_io_arm=0, analog_io_mtr=0, "
			" analog_io_type=%d, ana_io_min_value=%f, ana_io_max_value=%f WHERE analog_io_config_id=%d", FLOW_OUT, MA_OUT, 0.0, 600.0, i+1);
		rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
		i++;
	}

	// use any remaining analog i/o for pressure inputs
	for (arm=0; arm<num_arms; arm++, i++)
	{
		if (i >= MAX_ANALOG_IO)
			break;
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE analog_io_config SET analog_io_tag='PT100%d', analog_io_func=%d, analog_io_arm=%d, analog_io_mtr=0, "
				" analog_io_type=%d, ana_io_min_value=%f, ana_io_max_value=%f WHERE analog_io_config_id=%d", i+1, PRESS_IN, arm, MA_IN, min_pres, max_pres, i+1);
		rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
	}

//	// configure pulse input #8 for frequency densitometer (#8 allows higher accuracy frequency measurement over smaller frequency range for densitometers)
//	rc |= sqlite3_exec(db, "UPDATE pulse_in_config SET pulse_in_func=1, pulse_in_arm=0, pulse_in_mtr=0 WHERE pulse_in_config_id=8", NULL,NULL,NULL);

	for (arm=0; arm<num_arms; arm++)
	{
//		if (arm >= MAX_PULSE_OUT)
		if (arm >= 2)		// pulse outputs 3-4 use digital outputs 1-3
			break;

		pulse_out_rate = (arm < 2) ? 100:1;
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE pulse_out_config SET pul_out_arm=%d, pul_out_mtr=0, pul_out_units=3, pul_out_per_vol=%d "
				"WHERE pulse_out_config_id=%d", arm+1, pulse_out_rate, arm+1);
		rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
	}

	// preset_vol_type = GV, delivery_vol_type = GSV, recipes_per_tran = MULTIPLE, display resolution = hundredths
	rc |= sqlite3_exec(db,
		"UPDATE system_config SET flow_simulator=1, pulse_mode_type=1, preset_vol_type=1, delivery_vol_type=3, batch_per_tran=5, recipes_per_tran=1, display_resolution=2, "
			"auto_preset = 200, auto_preset_inc = 10", NULL, NULL, NULL);

	// vol_units = gal, temp_units = F, dens_units = lb/ft3, press_units = PSI
	if (units == US_CUSTOMARY_UNITS)
		rc |= sqlite3_exec(db,
			"UPDATE system_config SET vol_units=0, temp_units=1, ref_temp=60.0, dens_units=2, press_units=1 ", NULL, NULL, NULL);
	else // vol_units = L, temp_units = C, dens_units = kg/m3, press_units = kPa,
		rc |= sqlite3_exec(db,
			"UPDATE system_config SET vol_units = 3, temp_units=2, ref_temp=15.0, dens_units=3, press_units=4 ", NULL, NULL, NULL);


	rc |= sqlite3_exec(db, "UPDATE comm_config SET user_text_saved=1", NULL, NULL, NULL);

	rc |= sqlite3_exec(db, "UPDATE alarm_config SET clr_able_alarm=5", NULL, NULL, NULL);
	rc |= sqlite3_exec(db, "UPDATE prompt_config SET prompts_in_use=2, prompt_timeout=30, prompt_msg_1='Enter Driver ID', prompt_msg_2='Enter Pin #', "
					"prompt_length_1=9, prompt_length_2=4", NULL, NULL, NULL);

	// permissive_type = continuous. block valve position = CLOSE_BOTH (0), blend correction = SELF_CORR_COMPLETE(2).
	rc |= sqlite3_exec(db, "UPDATE arm_config SET ready_msg='AccuLoad IV Ready', permissive_1_type=2, permissive_2_type=2, "
			"valve_delay_on=5, pump_delay_off=4, start_stop_delay=5, low_flow_start_vol=50, low_flow_start_rate=100, "
			"arm_hi_flow_rate=500, valve_fault_timeout=10, clean_volume=10, bv_pos=0, clean_alarm_limit=4, blend_correction=2, blend_tol_pct=1.0, "
			"blend_tol_vol=2.0, overrun_alrm_limit=5, rpt_hm_class=0", NULL, NULL, NULL);

	for (i=0; i<num_arms; i++)
	{
		// set permissive_start to Manual Start for Arm 3. all other arms set to Auto Start.
		perm_start = (i==2) ? 0:1;
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE arm_config SET load_arm_id='Field Test - Load Arm %d', permissive_1_start=%d, "
				"permissive_1_msg='Connect Ground - Arm %d', permissive_2_start=%d, permissive_2_msg='Truck Overfill - Arm %d' "
				"WHERE arm_config_id=%d", i+1, perm_start, i+1, perm_start, i+1, i+1);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);

		sqlite3_snprintf(sizeof(sql), sql, "UPDATE meter_config SET k_factor = 100, meter_tag='M100%d' WHERE arm_no=%d AND mtr_no=1", i+1, i+1);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
	}

	int dens_type = (num_arms > 3) ? NO_LIVE_DENSITY : UNCORRECTED_DENS;

	for (prd=0; prd<MAX_PRODUCTS; prd++)
	{
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE product_config SET hm_class_1='%s', hm_class_2='%s (%d)', live_dens_type=%d "
				"WHERE prd_no=%d AND arm_no <=%d", hm_class_1, hm_class_2, prd+1, dens_type, prd+1, num_arms);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
	}

// Arm 1 and 2 - using reference density
	// API 2004 pg 42, Lube 810.555 kg/m3 (42.900 API, 0.81135 RD) @60F, meter @129.3F and 138.8 PSI --- CTPL=0.97091, CTL=0.96992, CPL=1.00102, dens=48.125 API@avgTemp
	rc |= sqlite3_exec(db, "UPDATE product_config SET product_id='Lube Oil', api_table=4, ref_dens=810.555, ref_dens_units=3, maint_temp=129.3, maint_press=138.8, "
			"flow_tol_percent=7, hi_flow_rate = 500, min_flow_rate=50, first_trip_vol=60, sec_trip_vol=0.1, mfac_1 = 1.03015 "
			"WHERE arm_no=1 AND prd_no=1", NULL, NULL, NULL);
	rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=1, recipe_name = 'Lube Oil (1)' WHERE recipe_config_id=1", NULL, NULL, NULL);

	if (num_arms >= 2)
	{
		// API 2004 calculator, Refined -10.0 API (1.16461 RD) @60F, meter @-13.3F and 265.7 PSI --- CTPL=1.02313, CTL=1.02249, CPL=1.00063, dens=-12.747 API@avgTemp
		rc |= sqlite3_exec(db, "UPDATE product_config SET product_id='Regular 87', api_table=2, ref_dens=-10.0, ref_dens_units=1, maint_temp=-13.3, maint_press=265.7, "
			"flow_tol_percent=7, hi_flow_rate = 500, min_flow_rate=50, first_trip_vol=60, sec_trip_vol=0.1, mfac_1 = 1.01015 "
			"WHERE arm_no=2 AND prd_no=1", NULL, NULL, NULL);

		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=2, recipe_name = 'Regular 87 (2)'  WHERE recipe_config_id=2",  NULL, NULL, NULL);
		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=2, recipe_name = 'Regular 87 (31)' WHERE recipe_config_id=31", NULL, NULL, NULL);
		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=2, recipe_name = 'Regular 87 (32)' WHERE recipe_config_id=32", NULL, NULL, NULL);
		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=2, recipe_name = 'Regular 87 (33)' WHERE recipe_config_id=33", NULL, NULL, NULL);
		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=2, recipe_name = 'Regular 87 (34)' WHERE recipe_config_id=34", NULL, NULL, NULL);
		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=2, recipe_name = 'Regular 87 (35)' WHERE recipe_config_id=35", NULL, NULL, NULL);
		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=2, recipe_name = 'Regular 87 (41)' WHERE recipe_config_id=41", NULL, NULL, NULL);
		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=2, recipe_name = 'Regular 87 (42)' WHERE recipe_config_id=42", NULL, NULL, NULL);
		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=2, recipe_name = 'Regular 87 (43)' WHERE recipe_config_id=43", NULL, NULL, NULL);
		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=2, recipe_name = 'Regular 87 (44)' WHERE recipe_config_id=44", NULL, NULL, NULL);
		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=2, recipe_name = 'Regular 87 (45)' WHERE recipe_config_id=45", NULL, NULL, NULL);

	}

// Arm 3 - Sequential Blender
	if (num_arms >= 3)
	{
		rc |= sqlite3_exec(db, "UPDATE arm_config SET rpt_hm_class=1 WHERE arm_config_id=3", NULL, NULL, NULL);

		// API 2004 pg 38, Refined 0.7943 (46.644 API) @60F, meter @85.0F and 247.3 PSI --- CTPL=0.98846, CTL=0.98683, CPL=1.00165, dens=48.725 API@avgTemp
		rc |= sqlite3_exec(db, "UPDATE product_config SET product_id='Premium 91', api_table=2, ref_dens=0.7943, ref_dens_units=4, maint_temp=85, maint_press=247.3, "
				"flow_tol_percent=7, hi_flow_rate = 500, min_flow_rate=50, first_trip_vol=60, sec_trip_vol=0.1, mfac_1 = 1.02015, bv_delay_open=4, bv_delay_close=6 "
				"WHERE arm_no=3 AND prd_no=1", NULL, NULL, NULL);
		// API 2004 calculator, Refined 20.2 API (0.93276 RD) @60F, meter @95.8F and 224.8 PSI --- CTPL=0.98626, CTL=0.98527, CPL=1.00100, dens=22.313 API@avgTemp
		rc |= sqlite3_exec(db, "UPDATE product_config SET product_id='Regular 87', api_table=2, ref_dens=20.2, ref_dens_units=1, maint_temp=95.8, maint_press=224.8, "
				"flow_tol_percent=7, hi_flow_rate = 500, min_flow_rate=50, first_trip_vol=60, sec_trip_vol=0.1, mfac_1 = 0.98885, bv_delay_open=4, bv_delay_close=6  "
				"WHERE arm_no=3 AND prd_no=2", NULL, NULL, NULL);

		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=3, recipe_name = '87 Octane', "
				"blend_pct1=100, blend_comp1=2, clean_deduct_prod=1, hm_class_prod=1 WHERE recipe_config_id=10", NULL, NULL, NULL);
		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=3, recipe_name = '88 Octane', "
				"blend_pct1=25, blend_comp1=2, blend_pct2=75, blend_comp2=1, clean_deduct_prod=0, hm_class_prod=0 WHERE recipe_config_id=11", NULL, NULL, NULL);
		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=3, recipe_name = '89 Octane', "
				"blend_pct1=25, blend_comp1=1, blend_pct2=25, blend_comp2=2, blend_pct3=25, blend_comp3=1, "
				"blend_pct4=25, blend_comp4=2, clean_deduct_prod=0, hm_class_prod=1 WHERE recipe_config_id=12", NULL, NULL, NULL);
		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=3, recipe_name = '90 Octane', "
				"blend_pct1=15, blend_comp1=2, blend_pct2=35, blend_comp2=1, blend_pct3=15, blend_comp3=2, "
				"blend_pct4=35, blend_comp4=1, clean_deduct_prod=0, hm_class_prod=0 WHERE recipe_config_id=13", NULL, NULL, NULL);
		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=3, recipe_name = '91 Octane', "
				"blend_pct1=100, blend_comp1=1, clean_deduct_prod=0, hm_class_prod=1 WHERE recipe_config_id=14", NULL, NULL, NULL);
	}

// Arm 4
	if (num_arms >= 4)
	{
		// API calculator, API C table, 0.000603/F @60F, 50.1 API (0.77919 RD), meter @32.53F and 185.32 PSI --- CTPL=1.01757, CTL=1.01648, CPL=1.00106,
		//		dens=46.965 API@avgTemp.  live_dens_type = NO_LIVE_DENSITY
		rc |= sqlite3_exec(db, "UPDATE product_config SET product_id='Ethanol', api_table=3, ref_dens=50.1, ref_dens_units=1, tcoeff_expansion=0.0603, live_dens_type=0, "
				"maint_temp=32.53, maint_press=185.32, flow_tol_percent=7, hi_flow_rate = 500, min_flow_rate=50, first_trip_vol=60, sec_trip_vol=0.1, mfac_1 = 1.04015 "
				"WHERE arm_no=4 AND prd_no=1", NULL, NULL, NULL);

		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=4, recipe_name = 'Ethanol (4)'        WHERE recipe_config_id=4", NULL, NULL, NULL);
	}

// Arm 5 and 6 - using live density
	if (num_arms >= 5)
	{
		// API calculator, Crude 64.58 lb/ft3, meter @118.5F and 353.4 PSI --- CTPL=0.98314, CTL=0.98188, CPL=1.00129, dens=2.846 API@60F (1.05325 RD).
		//	live_dens_type = UNCORRECTED_DENS.
		rc |= sqlite3_exec(db, "UPDATE product_config SET product_id='Crude', api_table=1, ref_dens=0, ref_dens_units=0, live_dens_type=1, maint_temp=118.5, maint_press=353.4, "
				"maint_dens=64.58, flow_tol_percent=7, hi_flow_rate = 500, min_flow_rate=50, first_trip_vol=60, sec_trip_vol=0.1, mfac_1 = 1.05015 "
				"WHERE arm_no=5 AND prd_no=1", NULL, NULL, NULL);

		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=5, recipe_name = 'Crude (5)'     WHERE recipe_config_id=5", NULL, NULL, NULL);
	}

	if (num_arms >= 6)
	{
		// API E LPG pg 51, 0.21056 (13.1319 lb/ft3) @60F, meter @87.46 and 350.0 PSI --- CTL=0.60137, RelDens@60&avgPres=0.350238, CPL=1.30774, CTPL=0.78644,
		//		dens=397.00 API@60F (0.26774 RD).  live_dens_type = UNCORRECTED_DENS.
		rc |= sqlite3_exec(db, "UPDATE product_config SET product_id='LPG', api_table=5, ref_dens=0, ref_dens_units=0, live_dens_type=1, maint_temp=87.46, maint_press=350, "
				"maint_dens=13.1319, flow_tol_percent=7, hi_flow_rate = 500, min_flow_rate=50, first_trip_vol=60, sec_trip_vol=0.1, mfac_1 = 1.06015 "
				"WHERE arm_no=6 AND prd_no=1", NULL, NULL, NULL);

		rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used=6, recipe_name = 'LPG (50)' WHERE recipe_config_id=50", NULL, NULL, NULL);
	}

	rc |= sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
	if (rc != SQLITE_OK) {

		set_diagnostic_alarm("Can't STD field test init database", __LINE__, __FILE__);
		sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
	}

	return(rc);
}


int ratio_blend_arm_init(sqlite3 *db, int arm, enum arm_config_types arm_config, int ratio_prds)
{
	int i, rc, recipe_no, num_prds, prd;
	char sql[1200], arm_id[50];
	int arm_rcp[MAX_LOAD_ARMS] = {1,10,20,30,0,0};

	// not enough meters left to have a 3 product blender on arms 5 and 6
	if (arm > ARM_4)
		return(SQLITE_ERROR);

	recipe_no = arm_rcp[arm];

	rc = SQLITE_OK;

	num_prds = 3;
	if (arm_config == HYBRID_BLENDER)
	{
		sqlite3_snprintf(sizeof(arm_id), arm_id, "Hybrid Arm %d", arm+1);
		// if hybrid, configure 2 sequential products + desired # ratio products
		num_prds = ratio_prds + 2;
	}
	else
		sqlite3_snprintf(sizeof(arm_id), arm_id, "Ratio Arm %d", arm+1);

	// permissive_type = continuous, permissive_start = auto start.
	sqlite3_snprintf(sizeof(sql), sql, "UPDATE arm_config SET ready_msg='AccuLoad IV Ready', load_arm_id='%s', permissive_1_type=2, "
			"permissive_1_msg='Connect Ground %d', permissive_1_start=1, permissive_2_type=2, permissive_2_msg='Truck Overfill %d', "
			"permissive_2_start=1, valve_delay_on=5, pump_delay_off=4, start_stop_delay=5, low_flow_start_vol=40, low_flow_start_rate=100, "
			"arm_hi_flow_rate=500, valve_fault_timeout=10, clean_product=1, clean_volume=10, clean_alarm_limit=5, blend_tol_pct=1.0, blend_tol_vol=3.0, "
			"blend_tol_timeout=15, blend_alarm_min_vol = 50, blend_correct_vol=2.0, blend_correct_time=5, overrun_alrm_limit=5, ratio_adjust_factor=10, "
			"ratio_time=1, rpt_hm_class=%d  WHERE arm_config_id=%d", arm_id, arm+1, arm+1, arm+1, arm+1);
	rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);

	for (i=0; i<num_prds; i++)
	{
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE meter_config SET meter_tag='%d00%d', turbine_mtr_blades=2, k_factor=100, valve_type=0, flow_adj_tol=4, "
			"flow_adj_timer=4, mtr_overrun_alrm_limit=5 "
			"WHERE arm_no=%d AND mtr_no=%d", arm+1, i+1, arm+1, i+1);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);

		sqlite3_snprintf(sizeof(sql), sql, "UPDATE product_config SET bp_percent_reduce=80, min_bp_flow_rate=50, "
				"min_bp_flow_rate_timer=10, bp_flow_recover_timer=1, hm_class_1='%s', hm_class_2='%s (%d)', live_dens_type=0 "
				"WHERE arm_no=%d AND prd_no=%d", hm_class_1, hm_class_2, i+1, arm+1, i+1);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);

	}


	prd = 1;

	// API 2004 calculator, C Table 0.000603/F, meter @91.4F and 154.4 PSI --- CTPL=0.98211, CTL=0.98096, CPL=1.00117, dens=55.9790 API@avgTemp
	//		dens@60F = 52.6249 API or 0.7685
	sqlite3_snprintf(sizeof(sql), sql, "UPDATE product_config SET product_id='Ethanol', api_table=3, ref_dens=0.7685, ref_dens_units=4, tcoeff_expansion=0.0603, maint_temp=91.4, "
			"maint_press=154.4, flow_tol_percent=7, hi_flow_rate = 300, min_flow_rate=25, first_trip_vol=30, sec_trip_vol=0.1, mfac_1 = 1.0305, min_batch_vol=10 "
			"WHERE arm_no=%d AND prd_no=%d", arm+1, prd++);
	rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
	if ((arm_config == HYBRID_BLENDER) && (ratio_prds > 1))
	{
		// API 2004 calculator, C Table 0.000603/F, meter @91.4F and 154.4 PSI --- CTPL=0.98211, CTL=0.98096, CPL=1.00117, dens=55.9790 API@avgTemp
		//		dens@60F = 52.6249 API or 0.7685
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE product_config SET product_id='Eth 2', api_table=3, ref_dens=0.7685, ref_dens_units=4, tcoeff_expansion=0.0603, maint_temp=91.4, "
				"maint_press=154.4, flow_tol_percent=7, hi_flow_rate = 500, min_flow_rate=20, first_trip_vol=30, sec_trip_vol=0.1, mfac_1 = 1.0305, min_batch_vol=10 "
				"WHERE arm_no=%d AND prd_no=%d", arm+1, prd++);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
	}
	// API 2004 calculator, Refined 784.3 kg/m3 @60F, meter @95.2F and 161.3 PSI --- CTPL=0.98158, CTL=0.98044, CPL=1.00116, dens=52.1203 API@avgTemp
	//		dens@60F = 48.7381 API or 0.78507
	sqlite3_snprintf(sizeof(sql), sql, "UPDATE product_config SET product_id='Premium 91', api_table=2, ref_dens=784.3, ref_dens_units=3, maint_temp=95.2, maint_press=161.3, "
			"flow_tol_percent=7, hi_flow_rate = 500, min_flow_rate=50, first_trip_vol=60, sec_trip_vol=0.1, mfac_1 = 1.0105, min_batch_vol=10 "
			"WHERE arm_no=%d AND prd_no=%d", arm+1, prd++);
	rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
	// API 2004 calculator, Refined 778.6 @60F, meter @85.6F and 145.8 PSI --- CTPL=0.98590, CTL=0.98488, CPL=1.00103, dens=52.6546 API@avgTemp
	//		dens@60F = 50.0576 API or 0.77937
	sqlite3_snprintf(sizeof(sql), sql, "UPDATE product_config SET product_id='Regular 87', api_table=2, ref_dens=778.6, ref_dens_units=3, maint_temp=85.6, maint_press=145.8, "
			"flow_tol_percent=7, hi_flow_rate = 500, min_flow_rate=50, first_trip_vol=60, sec_trip_vol=0.1, mfac_1 = 1.0205, min_batch_vol=10 "
			"WHERE arm_no=%d AND prd_no=%d", arm+1, prd++);
	rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);


	if (arm_config == HYBRID_BLENDER)
	{
		// intended for arms with 2 ratio products and 2 sequential products
		if (ratio_prds > 1)
		{
			// 90% P3, 10%P1
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = '10/0/90/0', "
				"blend_pct1=10, blend_comp3=3, blend_pct3=90, clean_deduct_prod=2, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			// 90% P3, 10%P2
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = '0/10/90/0', "
				"blend_pct2=10, blend_comp3=3, blend_pct3=90, clean_deduct_prod=2, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			// 40% P4, 30% P3, 20% P2, 10% P1 (seq order = p3, p4)
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = '10/20/30/40', "
				"blend_pct1=10, blend_pct2=20, blend_comp3=3, blend_pct3=30, blend_comp4=4, blend_pct4=40, clean_deduct_prod=3, "
				"hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			// 100% P1
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = 'Prd 1', "
				"blend_pct1=100, clean_deduct_prod=0, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			// 100% P2
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = 'Prd 2', "
				"blend_pct2=100, clean_deduct_prod=1, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			// 100% P3
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = 'Prd 3', "
				"blend_comp3=3, blend_pct3=100, clean_deduct_prod=2, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			// 100% P4
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = 'Prd 4', "
				"blend_comp3=4, blend_pct3=100, clean_deduct_prod=3, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			// 25% P4, 25% P3, 25% P2, 25% P1 (seq order = p4, p3)
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = '25/25/25/25', "
				"blend_pct1=25, blend_pct2=25, blend_comp3=4, blend_pct3=25, blend_comp4=3, blend_pct4=25, clean_deduct_prod=3, "
				"hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);

		}
		// intended for arms with 1 ratio product and 2 sequential products
		else
		{
			// 90% P3, 10%P1
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = '87 Octane', "
					"blend_pct1=10, blend_comp2=3, blend_pct2=90, clean_deduct_prod=2, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			// 70% P3, 20% P2, 10% P1 (seq order = p2, p3)
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = '88 Octane', "
					"blend_pct1=10, blend_comp2=2, blend_pct2=20, blend_comp3=3, blend_pct3=70, clean_deduct_prod=2, "
					"hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			// 20% P3, 70% P2, 10% P1 (seq order = p3, p2)
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = '90 Octane', "
					"blend_pct1=10, blend_comp2=3, blend_pct2=20, blend_comp3=2, blend_pct3=70, clean_deduct_prod=1, "
					"hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			// 100% P1
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = 'Ethanol', "
					"blend_pct1=100, clean_deduct_prod=0, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			// 100% P2
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = 'Premium', "
					"blend_comp2=2, blend_pct2=100, clean_deduct_prod=1, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			// 100% P3
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = 'Regular', "
					"blend_comp2=3, blend_pct2=100, clean_deduct_prod=2, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
			// 25% P3, 25% P2, 50% P1 (seq order = p3, p2)
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = '50/25/25', "
					"blend_pct1=50, blend_comp2=3, blend_pct2=25, blend_comp3=2, blend_pct3=25, clean_deduct_prod=1, "
					"hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		}
	}
	else // RATIO_BLENDER
	{
		// 90% P3, 10%P1
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = '87 Octane', "
			"blend_pct1=10, blend_pct3=90, clean_deduct_prod=2, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		// 70% P3, 20% P2, 10% P1
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = '88 Octane', "
			"blend_pct1=10, blend_pct2=20, blend_pct3=70, clean_deduct_prod=2, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		// 20% P3, 70% P2, 10% P1
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = '90 Octane', "
			"blend_pct1=10, blend_pct2=70, blend_pct3=20, clean_deduct_prod=1, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		// 100% P1
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = 'Ethanol', "
			"blend_pct1=100, clean_deduct_prod=0, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		// 100% P2
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = 'Premium', "
			"blend_pct2=100, clean_deduct_prod=1, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		// 100% P3
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = 'Regular', "
			"blend_pct3=100, clean_deduct_prod=2, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		// 50% P2, 50% P3
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE recipe_config SET recipe_used=%d, recipe_name = '50/50', "
			"blend_pct2=50, blend_pct3=50, clean_deduct_prod=1, hm_class_prod=2 WHERE recipe_config_id=%d", arm+1, recipe_no++);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
	}

	return(rc);
}

int ratio_blend_field_test_init(sqlite3 *db, int num_arms, int units)
{
	int arm, mtr, dig_in, dig_out, ana_io, num_prds, rc, a4b_available;
	char sql[500];

	rc = sqlite3_exec(db, "BEGIN IMMEDIATE TRANSACTION", NULL, NULL, NULL);

	// clear out factory default recipe assignments
	rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used = 0, recipe_name = ''", NULL,NULL,NULL);

	// limit to 2 arms
	if (num_arms > 2)
		num_arms = 2;

	num_prds = 3;

	a4b_available = (!A4M_only && (num_arms > 1)) ? true:false;

	sqlite3_snprintf(sizeof(sql), sql,
			"UPDATE load_arm_layout SET num_physical_arms=%d, arm1_config = 2, arm1_num_prds=%d, arm2_config = 2, arm2_num_prds=%d, a4b_available=%d",
			num_arms, num_prds, num_prds, a4b_available);
	rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);


	// preset_vol_type = GV, delivery_vol_type = GSV, recipes_per_tran = MULTIPLE, display resolution = hundredths
	rc |= sqlite3_exec(db,
		"UPDATE system_config SET flow_simulator=1, pulse_mode_type=1, preset_vol_type=1, delivery_vol_type=3, batch_per_tran=5, recipes_per_tran=1, display_resolution=2", NULL, NULL, NULL);

	// vol_units = gal, temp_units = F, dens_units = API, press_units = PSI
	if (units == US_CUSTOMARY_UNITS)
		rc |= sqlite3_exec(db,
			"UPDATE system_config SET vol_units=0, temp_units=1, ref_temp=60.0, dens_units=1, press_units=1 ", NULL, NULL, NULL);
	else // vol_units = L, temp_units = C, dens_units = kg/m3, press_units = kPa,
		rc |= sqlite3_exec(db,
			"UPDATE system_config SET vol_units = 3, temp_units=2, ref_temp=15.0, dens_units=3, press_units=4 ", NULL, NULL, NULL);


	rc |= sqlite3_exec(db, "UPDATE comm_config SET user_text_saved=1", NULL, NULL, NULL);

	rc |= sqlite3_exec(db, "UPDATE alarm_config SET clr_able_alarm=5", NULL, NULL, NULL);
	rc |= sqlite3_exec(db, "UPDATE prompt_config SET prompts_in_use=2, prompt_timeout=30, prompt_msg_1='Enter Driver ID', prompt_msg_2='Enter Pin #', "
					"prompt_length_1=9, prompt_length_2=4", NULL, NULL, NULL);


	for (arm=0, dig_out=1; arm<num_arms; arm++)
	{
		// digital outputs 1-6 on A4M (1-3 are DC and 4-6 are AC) used for arm 1 meters 1-3 up/downstream solenoids
		// digital outputs 31-36 on A4B (Bi-state DC outputs) used for arm 2 meters 1-3 up/downstream solenoids
		if (a4b_available && (arm >= 1) && (dig_out <= FIRST_BIO_DIG_OUT))
			dig_out = FIRST_BIO_DIG_OUT+1;		// make it one based

		for (mtr=0; mtr<3; mtr++)
		{
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_func=%d, dig_out_arm=%d, dig_out_mtr=%d WHERE dig_out_config_id=%d", UPSTREAM_SOL, arm, mtr, dig_out++);
			rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_func=%d, dig_out_arm=%d, dig_out_mtr=%d WHERE dig_out_config_id=%d", DOWNSTREAM_SOL, arm, mtr, dig_out++);
			rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
		}

		rc |= ratio_blend_arm_init(db, arm, RATIO_BLENDER, num_prds);

	}

	// set dig_out to next available output on A4M
	if (a4b_available)
		dig_out = 7;

	// Use AC inputs (dig_in 7-11 on A4M and 12-15 on A4B)
	for (arm=0, dig_in=7; arm<num_arms; arm++)
	{
		if (a4b_available && (arm >= 1) && (dig_out <= FIRST_A4B_DIG_OUT))
			dig_out = FIRST_A4B_DIG_OUT+1;		// make it one based

		if (a4b_available && (arm >= 1) && (dig_in <= FIRST_A4B_DIG_IN))
			dig_in = FIRST_A4B_DIG_IN+1;		// make it one based

		if (a4b_available || (dig_in <= NUM_A4M_DIG_INPUTS))
		{
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_in_config SET dig_in_func=%d, dig_in_arm=%d WHERE dig_in_config_id=%d", PERMISSIVE_1, arm, dig_in++);
			rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_in_config SET dig_in_func=%d, dig_in_arm=%d WHERE dig_in_config_id=%d", PERMISSIVE_2, arm, dig_in++);
			rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
		}
		for (mtr=0; mtr<3; mtr++)
		{
			// if A4B is not available, we may not have enough outputs for all meters to have a pump
			if (a4b_available || (dig_out <= NUM_A4M_DIG_OUTPUTS))
			{
				sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_func=%d, dig_out_arm=%d, dig_out_mtr=%d WHERE dig_out_config_id=%d", PUMP_OUT, arm, mtr, dig_out++);
				rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
			}
		}
	}

	for (arm=0, ana_io=1; arm<num_arms; arm++, ana_io++)
	{
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE analog_io_config SET analog_io_tag='Flow A%d', analog_io_func=%d, analog_io_arm=%d, analog_io_mtr=0, "
			" analog_io_type=%d, ana_io_min_value=%f, ana_io_max_value=%f WHERE analog_io_config_id=%d", ana_io, FLOW_OUT, arm, MA_OUT, 0.0, 600.0, ana_io);
		rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
	}

	// setup arm # to use timed blend algorithm
	if (num_arms == 2)
		rc |= sqlite3_exec(db, "UPDATE arm_config set ratio_blend_alg=1 WHERE arm_config_id=2", NULL,NULL,NULL);


	rc |= sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
	if (rc != SQLITE_OK)
	{
		set_diagnostic_alarm("Field Test Init Failed", __LINE__, __FILE__);
		sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
	}

	return(rc);
}


int hybrid_blend_field_test_init(sqlite3 *db, int num_arms, int units)
{
	float min_temp, max_temp, min_dens, max_dens, min_pres, max_pres;
	int arm, mtr, prd, dig_in, dig_out, ana_io, num_prds, num_ratio, num_mtrs, rc, a4b_available;
	char sql[500];

	rc = sqlite3_exec(db, "BEGIN IMMEDIATE TRANSACTION", NULL, NULL, NULL);

	// clear out factory default recipe assignments
	rc |= sqlite3_exec(db, "UPDATE recipe_config SET recipe_used = 0, recipe_name = ''", NULL,NULL,NULL);

	// limit to 3 arms
	if (num_arms > 3)
		num_arms = 3;

	if (num_arms == 3) {
		num_prds = 3;
		num_ratio = 1;
	}
	else {
		num_prds = 4;
		num_ratio = 2;
	}

	num_mtrs = num_ratio + 1;

	// 2 meters per arm (dual channel - 4 pulse inputs per arm)
	a4b_available = (A4M_only && (num_arms <= 2)) ? false:true;

	sqlite3_snprintf(sizeof(sql), sql, "UPDATE load_arm_layout SET num_physical_arms=%d, a4b_available=%d", num_arms, a4b_available);
	rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);

	for (arm=0; arm<num_arms; arm++)
	{
		sqlite3_snprintf(sizeof(sql), sql,
			"UPDATE load_arm_layout SET arm%d_config = 5, arm%d_num_prds=%d, arm%d_ratio_prds=%d", arm+1, arm+1, num_prds, arm+1, num_ratio, num_prds);
		rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
	}

	// preset_vol_type = GV, delivery_vol_type = GSV, recipes_per_tran = MULTIPLE, display resolution = hundredths
	rc |= sqlite3_exec(db,
		"UPDATE system_config SET flow_simulator=1, pulse_mode_type=1, preset_vol_type=1, delivery_vol_type=3, batch_per_tran=5, recipes_per_tran=1, display_resolution=2", NULL, NULL, NULL);

	// vol_units = gal, temp_units = F, dens_units = API, press_units = PSI
	if (units == US_CUSTOMARY_UNITS) {
		min_temp = 20; max_temp = 120; min_dens = 30; max_dens = 80, min_pres = 0, max_pres = 300;
		rc |= sqlite3_exec(db,
			"UPDATE system_config SET vol_units=0, temp_units=1, ref_temp=60.0, dens_units=1, press_units=1 ", NULL, NULL, NULL);
	}
	else { // vol_units = L, temp_units = C, dens_units = kg/m3, press_units = kPa,
		min_temp = -30; max_temp = 50; min_dens = 500; max_dens = 1100, min_pres = 0, max_pres = 2000;
		rc |= sqlite3_exec(db,
			"UPDATE system_config SET vol_units = 3, temp_units=2, ref_temp=15.0, dens_units=3, press_units=4 ", NULL, NULL, NULL);
	}

	(void)max_pres; (void)min_pres; (void)min_dens; (void)max_dens;

	rc |= sqlite3_exec(db, "UPDATE comm_config SET user_text_saved=1", NULL, NULL, NULL);

	rc |= sqlite3_exec(db, "UPDATE alarm_config SET clr_able_alarm=5", NULL, NULL, NULL);
	rc |= sqlite3_exec(db, "UPDATE prompt_config SET prompts_in_use=2, prompt_timeout=30, prompt_msg_1='Enter Driver ID', prompt_msg_2='Enter Pin #', "
					"prompt_length_1=9, prompt_length_2=4", NULL, NULL, NULL);


	for (arm=0, dig_out=1; arm<num_arms; arm++)
	{
		// digital outputs 1-6 on A4M (1-3 are DC and 4-6 are AC) used for arm 1 meters 1-3 up/downstream solenoids
		// digital outputs 31-36 on A4B (Bi-state DC outputs) used for arm 2 meters 1-3 up/downstream solenoids

		// if 2 arms and A4B available - arm 1 on A4M. arm 2 on A4B.  NOTE: "arm" variable is 0 based.
		if (a4b_available && (num_arms == 2) && (arm == 1) && (dig_out <= FIRST_BIO_DIG_OUT))
			dig_out = FIRST_BIO_DIG_OUT+1;		// make it one based

		for (mtr=0; mtr<num_mtrs; mtr++)
		{
			// arm 1 on A4M. arm 2 - mtr 1 on A4M, mtr 2 on A4B.  arm 3 on A4B.
			if ((num_arms == 3) && (arm == 1) && (mtr==1) && (dig_out <= FIRST_BIO_DIG_OUT))
				dig_out = FIRST_BIO_DIG_OUT+1;		// make it one based

			sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_func=%d, dig_out_arm=%d, dig_out_mtr=%d WHERE dig_out_config_id=%d", UPSTREAM_SOL, arm, mtr, dig_out++);
			rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);

			sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_func=%d, dig_out_arm=%d, dig_out_mtr=%d WHERE dig_out_config_id=%d", DOWNSTREAM_SOL, arm, mtr, dig_out++);
			rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
		}

		rc |= ratio_blend_arm_init(db, arm, HYBRID_BLENDER, num_ratio);

	}

	// set dig_out to next available output on A4M
	if (a4b_available)
	{
		// 2 arms 2 mtrs
		if ((num_arms == 2) && (num_mtrs == 2))
			dig_out = 5;
		else // 2 arm 3 mtrs OR 3 arm 2 mtrs
			dig_out = 7;
	}

	// Use AC inputs (dig_in 7-11 on A4M and 12-15 on A4B)
	for (arm=0, dig_in=7; arm<num_arms; arm++)
	{
		if (a4b_available && (arm >= 2) && (dig_out <= FIRST_A4B_DIG_OUT))
			dig_out = FIRST_A4B_DIG_OUT+1;		// make it one based

		if (a4b_available && (arm >= 2) && (dig_in <= FIRST_A4B_DIG_IN))
			dig_in = FIRST_A4B_DIG_IN+1;		// make it one based

		if (a4b_available || (dig_in <= NUM_A4M_DIG_INPUTS))
		{
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_in_config SET dig_in_func=%d, dig_in_arm=%d WHERE dig_in_config_id=%d", PERMISSIVE_1, arm, dig_in++);
			rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_in_config SET dig_in_func=%d, dig_in_arm=%d WHERE dig_in_config_id=%d", PERMISSIVE_2, arm, dig_in++);
			rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
		}
		// 1 or 2 ratio meters and 1 sequential meter
		for (mtr=0; mtr<num_mtrs; mtr++)
		{
			// if A4B is not available, we may not have enough outputs for all meters to have a pump
			if (a4b_available || (dig_out <= NUM_A4M_DIG_OUTPUTS))
			{
				sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_func=%d, dig_out_arm=%d, dig_out_mtr=%d WHERE dig_out_config_id=%d", PUMP_OUT, arm, mtr, dig_out++);
				rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
			}
		}
		// if 3 arms, P2 and P3 are sequential products. If < 3 arms, P3 and P4 are sequential products.
		for (prd=num_ratio; prd<num_prds; prd++)
		{
			if (a4b_available || (dig_out <= NUM_A4M_DIG_OUTPUTS))
			{
				sqlite3_snprintf(sizeof(sql), sql, "UPDATE dig_out_config SET dig_out_func=%d, dig_out_arm=%d, dig_out_prd=%d WHERE dig_out_config_id=%d", BLOCK_VALVE, arm, prd, dig_out++);
				rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
			}
		}


	}

	for (arm=0, ana_io=1; arm<num_arms; arm++)
	{
		for (mtr=0; mtr<num_mtrs; mtr++, ana_io++)
		{
			if (arm < 2)
				sqlite3_snprintf(sizeof(sql), sql, "UPDATE analog_io_config SET analog_io_tag='TP100%d', analog_io_func=%d, analog_io_arm=%d, analog_io_mtr=%d, "
					" analog_io_type=%d WHERE analog_io_config_id=%d", ana_io, TEMPERATURE_IN, arm, mtr, RTD, ana_io);
			else
				sqlite3_snprintf(sizeof(sql), sql, "UPDATE analog_io_config SET analog_io_tag='TP100%d', analog_io_func=%d, analog_io_arm=%d, analog_io_mtr=%d, "
					" analog_io_type=%d, ana_io_min_value=%f, ana_io_max_value=%f WHERE analog_io_config_id=%d", ana_io, TEMPERATURE_IN, arm, mtr, MA_IN, min_temp, max_temp, ana_io);
			rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
		}
	}

	// if available, assign an analog i/o as an output
	if (ana_io <= MAX_ANALOG_IO)
	{
		sqlite3_snprintf(sizeof(sql), sql, "UPDATE analog_io_config SET analog_io_tag='Flow A1', analog_io_func=%d, analog_io_arm=0, analog_io_mtr=0, "
			" analog_io_type=%d, ana_io_min_value=%f, ana_io_max_value=%f WHERE analog_io_config_id=%d", FLOW_OUT, MA_OUT, 0.0, 600.0, ana_io++);
		rc |= sqlite3_exec(db, sql, NULL,NULL,NULL);
	}


	// setup arm # 2 to use timed blend algorithm
	if (num_arms >= 2)
		rc |= sqlite3_exec(db, "UPDATE arm_config set ratio_blend_alg=1 WHERE arm_config_id=2", NULL,NULL,NULL);


	// sequential products need block valve delays setup
	for (arm=0; arm<num_arms; arm++)
		for (prd=num_ratio; prd<num_prds; prd++)
		{
			sqlite3_snprintf(sizeof(sql), sql, "UPDATE product_config SET bv_delay_open=4, bv_delay_close=4 WHERE arm_no=%d AND prd_no=%d", arm+1, prd+1);
			rc |= sqlite3_exec(db, sql, NULL, NULL, NULL);
		}

	rc |= sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
	if (rc != SQLITE_OK)
	{
		set_diagnostic_alarm("Field Test Init Failed", __LINE__, __FILE__);
		sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
	}

	return(rc);
}


#ifdef zzz
enum {num_physical_arms, arm_config, arm_num_prds, arm_ratio_prds};
struct s_ascii_write_values write_arm_layout[] = {
		{"num_physical_arms",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"0"},
		{"arm1_config",				NULL, 0, 		NULL, 0, 		NULL, 0, 	"0"/*STRAIGHT*/},
		{"arm1_num_prds",			NULL, 0, 		NULL, 0, 		NULL, 0, 	"0"},
		{"arm1_ratio_prds",			NULL, 0, 		NULL, 0, 		NULL, 0, 	"0"}
};
#define NUM_ARM_LAYOUT_PARAMS sizeof(write_arm_layout)/sizeof(struct s_ascii_write_values)

enum {flow_rate_desc, vol_units, mass_units, temp_units, dens_units, press_units};
struct s_ascii_write_values write_system_config[] = {
	//col_name			primary key 1		primary key 2		primary key 3	new value
	{"flow_rate_desc",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"GPM"},
	{"vol_units",			NULL, 0, 		NULL, 0, 		NULL, 0, 	"0"/*GAL*/},
	{"mass_units",			NULL, 0, 		NULL, 0, 		NULL, 0, 	"0"/*LBS*/},
	{"temp_units",			NULL, 0,		NULL, 0,		NULL, 0,	"1"/*F*/},
	{"ref_temp",			NULL, 0,		NULL, 0,		NULL, 0,	"60"},
	{"dens_units",			NULL, 0,		NULL, 0,		NULL, 0,	"1"/*API*/},
	{"press_units",			NULL, 0,		NULL, 0,		NULL, 0,	"1"/*PSI*/},
	{"preset_vol_type",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"1"/*GV*/},
	{"delivery_vol_type",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"3"/*GSV*/},
	{"batch_per_tran",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"5"},
	{"recipes_per_tran",		NULL, 0,		NULL, 0,		NULL, 0,	"1"/*MULTIPLE*/},
	{"display_resolution",		NULL, 0,		NULL, 0,		NULL, 0,	"2"/*TENTHS*/},
};
#define NUM_SYSTEM_CONFIG_PARAMS sizeof(write_system_config)/sizeof(struct s_ascii_write_values)

struct s_ascii_write_values write_alarm_config[] = {
	//col_name			primary key 1		primary key 2		primary key 3	new value
	{"clr_able_alrms",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"5"}
};
#define NUM_ALARM_CONFIG_PARAMS sizeof(write_alarm_config)/sizeof(struct s_ascii_write_values)

struct s_ascii_write_values write_prompt_config[] = {
	//col_name			primary key 1		primary key 2		primary key 3	new value
	{"prompts_in_use",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"2"},
	{"prompts_timeout",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"30"},
	{"prompts_msg_1",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"Enter Driver ID"},
	{"prompts_input_type_1",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"0"/*NUMERIC*/},
	{"prompts_length_1",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"9"},
	{"prompts_msg_2",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"Enter Pin #"},
	{"prompts_input_type_2",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"1"/*HIDDEN*/},
	{"prompts_length_2",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"4"}

};
#define NUM_PROMPT_CONFIG_PARAMS sizeof(write_prompt_config)/sizeof(struct s_ascii_write_values)

enum {ready_msg, load_arm_id, perm_1_msg, perm_2_msg};
struct s_ascii_write_values write_arm_config[] = {
	//col_name		primary key 1		primary key 2		primary key 3	new value
	{"ready_msg",			"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"AccuLoad IV Ready"},
	{"load_arm_id",			"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"Field Test - Load Arm 1"},
	{"permissive_1_msg",	"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"Connect Ground - Arm 1"},
	{"permissive_2_msg",	"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"Truck Overfill - Arm 1"},
	{"permissive_1_type",	"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"2"/*PERM_CONTINUOUS*/},
	{"permissive_1_start",	"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"1"/*PERM_AUTO_START*/},
	{"permissive_2_type",	"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"2"/*PERM_CONTINUOUS*/},
	{"permissive_2_start",	"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"1"/*PERM_AUTO_START*/},
	{"valve_delay_on",		"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"5"},
	{"pump_delay_off",		"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"4"},
	{"start_stop_delay",	"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"5"},
	{"low_flow_start_vol",	"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"50"},
	{"low_flow_start_rate",	"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"100"},
	{"valve_fault_timeout",	"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"15"},
	{"bv_pos",				"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"2"/*OPEN_END_CLOSE_STOP*/},
	{"clean_product",		"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"0"/*PRD_1*/},
	{"clean_arm_limit",		"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"4"},
	{"blend_correction",	"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"2"/*SELF_CORR_COMPLETE*/},
	{"clean_volume",		"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"10"},
	{"blend_tol_pct",		"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"1.0"},
	{"blend_tol_vol",		"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"2.0"},
	{"overrun_alrm_limit",	"arm_config_id", 1, 		NULL, 0, 		NULL, 0, 	"5"},
};
#define NUM_ARM_CONFIG_PARAMS sizeof(write_arm_config)/sizeof(struct s_ascii_write_values)

struct s_ascii_write_values write_meter_config[] = {
	//col_name		primary key 1		primary key 2		primary key 3	new value
	{"meter_name",		"arm_no", 1, 	"mtr_no", 1, 		NULL, 0, 	"Genesis 1"},
	{"k_factor",		"arm_no", 1, 	"mtr_no", 1, 		NULL, 0, 	"100.0"},

	{"meter_name",		"arm_no", 1, 	"mtr_no", 2, 		NULL, 0, 	"Genesis 2"},
	{"k_factor",		"arm_no", 1, 	"mtr_no", 2, 		NULL, 0, 	"200.0"},
};
#define NUM_METER_CONFIG_PARAMS sizeof(write_meter_config)/sizeof(struct s_ascii_write_values)

enum {product_id, api_table, ref_dens_units, ref_dens, maint_temp, maint_press, maint_dens, hi_temp_alrm_limit, lo_temp_alrm_limit};
struct s_ascii_write_values write_product_config[] = {
	//col_name		primary key 1			primary key 2	primary key 3	new value
	{"product_id",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"Heating Oil"},
	{"api_table",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"1"/*API_B_REFINED*/},
	{"ref_dens_units",	"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"0"/*API_GRAVITY*/},
	{"ref_dens",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"45.2"},		////CTL = 1.01837 @24.2F
	{"maint_temp",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"24.2"},
	{"maint_press",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"0"},
	{"maint_dens",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"0"},
	{"hi_temp_alrm_limit",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"95"},
	{"lo_temp_alrm_limit",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"0"},

//	{"prd_type",		"product_config_id", 2, 	NULL, 0, 	NULL, 0, 	"4"/*API_E_LPG*/},
//	{"ref_dens_units",	"product_config_id", 2, 	NULL, 0, 	NULL, 0, 	"1"/*REL_DENS*/},
//	{"ref_density",		"product_config_id", 2, 	NULL, 0, 	NULL, 0, 	"0.4515"},		////CTL = 0.93275 @87.4F

//	{"prd_type",		"product_config_id", 3, 	NULL, 0, 	NULL, 0, 	"1"/*API_B_REFINED*/},
//	{"ref_dens_units",	"product_config_id", 3, 	NULL, 0, 	NULL, 0, 	"2"/*KGM3*/},
//	{"ref_density",		"product_config_id", 3, 	NULL, 0, 	NULL, 0, 	"820"},

	{"mfac",	"arm_no", 1,		"prd_no", 1, 	NULL, 1, 	"0.99"},
	{"mfac_frate",	"arm_no", 1,		"prd_no", 1, 	NULL, 1, 	"500"},
	{"mfac",	"arm_no", 1,		"prd_no", 1, 	NULL, 2, 	"0.95"},
	{"mfac_frate",	"arm_no", 1,		"prd_no", 1, 	NULL, 2, 	"100"},

	{"flow_tol_percent",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"7.0"},
	{"hi_flow_rate",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"500"},
	{"min_flow_rate",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"20"},
	{"first_trip_vol",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"30"},
	{"sec_trip_vol",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"0.2"},

	{"bv_delay_open",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"5"},
	{"bv_delay_close",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"5"},
	{"min_batch_vol",		"arm_no", 1, 	"prd_no", 1, 	NULL, 0, 	"0"},

};
#define NUM_PRODUCT_CONFIG_PARAMS sizeof(write_product_config)/sizeof(struct s_ascii_write_values)


enum {dig_in_1_func, dig_in_1_arm, dig_in_2_func, dig_in_2_arm};
struct s_ascii_write_values write_dig_in_config[] = {
	//col_name		primary key 1		primary key 2		primary key 3		new value
	{"dig_in_func",	"dig_in_config_id", 1,		NULL, 0, 	NULL, 0, 		"3"},/*PERMISSIVE_1*/
	{"dig_in_arm",	"dig_in_config_id", 1,		NULL, 0, 	NULL, 0, 		"0"},
	{"dig_in_func",	"dig_in_config_id", 2,		NULL, 0, 	NULL, 0, 		"4"},/*PERMISSIVE_2*/
	{"dig_in_arm",	"dig_in_config_id", 2,		NULL, 0, 	NULL, 0, 		"0"},
};
#define NUM_DIG_IN_CONFIG_PARAMS sizeof(write_dig_in_config)/sizeof(struct s_ascii_write_values)

enum {dig_out_func, dig_out_arm, dig_out_mtr, dig_out_prd};
struct s_ascii_write_values write_dig_out_config[] = {
	//col_name		primary key 1		primary key 2		primary key 3		new value
	{"dig_out_func",	"dig_out_config_id", 1,		NULL, 0, 	NULL, 0, 		"1"},/*PUMP_OUT*/
	{"dig_out_arm",	"dig_out_config_id", 1,		NULL, 0, 	NULL, 0, 		"0"},
	{"dig_out_mtr",	"dig_out_config_id", 1,		NULL, 0, 	NULL, 0, 		"0"},/*MTR_1*/
	{"dig_out_prd",	"dig_out_config_id", 1,		NULL, 0, 	NULL, 0, 		"0"},/*PRD_1*/

	{"dig_out_func",	"dig_out_config_id", 2,		NULL, 0, 	NULL, 0, 		"2"},/*UPSTREAM_SOL*/
	{"dig_out_arm",	"dig_out_config_id", 2,		NULL, 0, 	NULL, 0, 		"0"},
	{"dig_out_mtr",	"dig_out_config_id", 2,		NULL, 0, 	NULL, 0, 		"0"},/*MTR_1*/
	{"dig_out_prd",	"dig_out_config_id", 2,		NULL, 0, 	NULL, 0, 		"0"},/*PRD_1*/

	{"dig_out_func",	"dig_out_config_id", 3,		NULL, 0, 	NULL, 0, 		"3"},/*DOWNSTREAM_SOL*/
	{"dig_out_arm",	"dig_out_config_id", 3,		NULL, 0, 	NULL, 0, 		"0"},
	{"dig_out_mtr",	"dig_out_config_id", 3,		NULL, 0, 	NULL, 0, 		"0"},/*MTR_1*/
	{"dig_out_prd",	"dig_out_config_id", 3,		NULL, 0, 	NULL, 0, 		"0"},/*PRD_1*/

	{"dig_out_func",	"dig_out_config_id", 4,		NULL, 0, 	NULL, 0, 		"7"},/*BLOCK_VALVE*/
	{"dig_out_arm",	"dig_out_config_id", 4,		NULL, 0, 	NULL, 0, 		"0"},
	{"dig_out_mtr",	"dig_out_config_id", 4,		NULL, 0, 	NULL, 0, 		"0"},/*MTR_1*/
	{"dig_out_prd",	"dig_out_config_id", 4,		NULL, 0, 	NULL, 0, 		"0"},/*PRD_1*/

	{"dig_out_func",	"dig_out_config_id", 5,		NULL, 0, 	NULL, 0, 	"7"},/*BLOCK_VALVE*/
	{"dig_out_arm",	"dig_out_config_id", 5,		NULL, 0, 	NULL, 0, 		"1"},
	{"dig_out_mtr",	"dig_out_config_id", 5,		NULL, 0, 	NULL, 0, 		"0"},/*MTR_1*/
	{"dig_out_prd",	"dig_out_config_id", 5,		NULL, 0, 	NULL, 0, 		"2"},/*PRD_2*/
};
#define NUM_DIG_OUT_CONFIG_PARAMS sizeof(write_dig_out_config)/sizeof(struct s_ascii_write_values)

struct s_ascii_write_values write_analog_config[] = {
	//col_name		primary key 1		primary key 2		primary key 3		new value
	{"analog_io_func",	"analog_io_config_id", 1,		NULL, 0, 	NULL, 0, 		"1"},/*TEMPERATURE*/
	{"analog_io_arm",	"analog_io_config_id", 1,		NULL, 0, 	NULL, 0, 		"1"},
	{"analog_io_mtr",	"analog_io_config_id", 2,		NULL, 0, 	NULL, 0, 		"1"},/*MTR_1*/
	{"analog_io_type",	"analog_io_config_id", 2,		NULL, 0, 	NULL, 0, 		"1"},/*MA_IN*/
	{"ana_io_min_value",	"analog_io_config_id", 2,		NULL, 0, 	NULL, 0, 		"-10"},
	{"ana_io_max_value",	"analog_io_config_id", 2,		NULL, 0, 	NULL, 0, 		"135"},
	{"rtd_offset",	"analog_io_config_id", 2,		NULL, 0, 	NULL, 0, 		"1"},
};
#define NUM_ANALOG_CONFIG_PARAMS sizeof(write_analog_config)/sizeof(struct s_ascii_write_values)

struct s_ascii_write_values write_comm_config[] = {
	//col_name		primary key 1		primary key 2		primary key 3		new value
	{"comm_addr1",	NULL, 0,		NULL, 0,	NULL, 0, 		"1"},
	{"comm_addr2",	NULL, 0,		NULL, 0,	NULL, 0, 		"2"},
	{"comm_addr3",	NULL, 0,		NULL, 0,	NULL, 0, 		"3"},
	{"comm_addr4",	NULL, 0,		NULL, 0,	NULL, 0, 		"4"},
	{"comm_addr5",	NULL, 0,		NULL, 0,	NULL, 0, 		"5"},
	{"comm_addr6",	NULL, 0,		NULL, 0,	NULL, 0, 		"6"},
	{"ethernet_control",	NULL, 0,		NULL, 1,	NULL, 0, 		"5"},
	{"ip_addr",		NULL, 0,		NULL, 1,	NULL, 0, 		"1"},
	{"netmask",		NULL, 0,		NULL, 1,	NULL, 0, 		"3"},
	{"gateway",	NULL, 0,		NULL, 0,	NULL, 0, 		"2"},/*MINICOMP_HOST*/
};
#define NUM_COMM_CONFIG_PARAMS sizeof(write_comm_config)/sizeof(struct s_ascii_write_values)

struct s_ascii_write_values write_serial_port_config[] = {
	//col_name		primary key 1		primary key 2		primary key 3		new value
	{"comm_protocol",	"serial_port_config_id", 1,		NULL, 0,	NULL, 0, 		"2"},/*MINICOMP_HOST*/
	{"host_control",	"serial_port_config_id", 1,		NULL, 0,	NULL, 0, 		"5"},/*POLL_PROGRAM*/
	{"baud_rate",		"serial_port_config_id", 1,		NULL, 0,	NULL, 0, 		"1"},/*BAUD_9600*/
	{"data_parity",		"serial_port_config_id", 1,		NULL, 0,	NULL, 0, 		"3"},/*EIGHT_NONE*/
};
#define NUM_SERIAL_PORT_PARAMS sizeof(write_serial_port_config)/sizeof(struct s_ascii_write_values)


struct s_ascii_write_values write_recipe_config[] = {
	//col_name		primary key 1		primary key 2		primary key 3		new value
		{"recipe_used",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"1"},
		{"blend_pct1",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"20"},
		{"blend_pct2",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"30"},
		{"blend_pct3",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"20"},
		{"blend_pct4",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"30"},
		{"blend_comp1",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"1"},
		{"blend_comp2",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"2"},
		{"blend_comp3",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"1"},
		{"blend_comp4",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"2"},
		{"clean_deduct_prod",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"0"/*PRD_1*/},

		{"recipe_used",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"1"},
		{"blend_pct1",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"40"},
		{"blend_pct2",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"60"},
		{"blend_comp1",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"2"},
		{"blend_comp2",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"1"},
		{"clean_deduct_prod",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"0"/*PRD_1*/},

		{"recipe_used",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"1"},
		{"blend_pct1",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"20"},
		{"blend_comp1",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"1"},
		{"clean_deduct_prod",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"0"/*PRD_1*/},

		{"recipe_used",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"1"},
		{"blend_pct1",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"100"},
		{"blend_comp1",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"2"},
		{"clean_deduct_prod",		"recipe_config_id", 1, 	NULL, 0, 	NULL, 0, 	"0"/*PRD_1*/},

};
#define NUM_RECIPE_PARAMS sizeof(write_recipe_config)/sizeof(struct s_ascii_write_values)

void DBRevel::SEQ_FieldTestInit ( sqlite3 *dbHandle, const char *dbName, sqlite3 *dbHandle_eventlog, int arm, int units )
{
	int rc;
	unsigned int i;

	/********************************************
	 * arm_config
	 */
	for (i=0; i<NUM_ARM_CONFIG_PARAMS; i++)
		write_arm_config[i].pkey1_value = arm+1;

	write_arm_config[load_arm_id].new_value = pDB.arm[arm].load_arm_id;
	write_arm_config[perm_1_msg].new_value = pDB.arm[arm].permissive_msg[0];
	write_arm_config[perm_2_msg].new_value = pDB.arm[arm].permissive_msg[1];

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "arm_config", write_arm_config, NUM_ARM_CONFIG_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_ARM_CONFIG_PARAMS; i++)
			if (write_arm_config[i].write_error != SQLITE_OK)
				fprintf(stderr, "arm_config Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

	/********************************************
	 * meter_config
	 */
	for (i=0; i<NUM_METER_CONFIG_PARAMS; i++)
		write_meter_config[i].pkey1_value = arm+1;

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "meter_config", write_meter_config, NUM_METER_CONFIG_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_METER_CONFIG_PARAMS; i++)
			if (write_meter_config[i].write_error != SQLITE_OK)
				fprintf(stderr, "meter_config Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

	/*******************************************
	 * product_config
	 */
	for (i=0; i<NUM_METER_CONFIG_PARAMS; i++)
		write_meter_config[i].pkey1_value = arm+1;

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "product_config", write_product_config, NUM_PRODUCT_CONFIG_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_PRODUCT_CONFIG_PARAMS; i++)
			if (write_product_config[i].write_error != SQLITE_OK)
				fprintf(stderr, "product_config Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "meter_factors", write_meter_factors, NUM_METER_FACTOR_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_METER_FACTOR_PARAMS; i++)
			if (write_meter_factors[i].write_error != SQLITE_OK)
				fprintf(stderr, "meter_factors Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "dig_in_config", write_dig_in_config, NUM_DIG_IN_CONFIG_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_DIG_IN_CONFIG_PARAMS; i++)
			if (write_dig_in_config[i].write_error != SQLITE_OK)
				fprintf(stderr, "dig_in_config Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "dig_out_config", write_dig_out_config, NUM_DIG_OUT_CONFIG_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_DIG_OUT_CONFIG_PARAMS; i++)
			if (write_dig_out_config[i].write_error != SQLITE_OK)
				fprintf(stderr, "dig_out_config Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "ticket_config", write_ticket_config, NUM_TICKET_CONFIG_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_TICKET_CONFIG_PARAMS; i++)
			if (write_ticket_config[i].write_error != SQLITE_OK)
				fprintf(stderr, "ticket_config Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "serial_port_config", write_serial_port_config, NUM_SERIAL_PORT_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_SERIAL_PORT_PARAMS; i++)
			if (write_serial_port_config[i].write_error != SQLITE_OK)
				fprintf(stderr, "serial_port_config Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

	if (POS_params[0].pos_upgrade)
		write_customer_category_parameters( dbHandle, dbName, dbHandle_eventlog );

}

struct s_ascii_write_values write_SI_system_config[] = {
	//col_name			primary key 1		primary key 2		primary key 3	new value
	{"unit_id",			NULL, 0, 		NULL, 0, 		NULL, 0, 	"Evolution - SI"},
	{"vol_units",			NULL, 0, 		NULL, 0, 		NULL, 0, 	"1"/*LIT*/},
	{"mass_units",			NULL, 0, 		NULL, 0, 		NULL, 0, 	"1"/*KGS*/},
	{"temp_units",			NULL, 0, 		NULL, 0, 		NULL, 0, 	"1"/*DEG_C*/},
	{"base_temp",			NULL, 0, 		NULL, 0, 		NULL, 0, 	"15.0"},
	{"display_decimal",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"0"/*HUNDREDTHS*/},
	{"flow_rate_base",		NULL, 0, 		NULL, 0, 		NULL, 0, 	"0"/*PER_MIN*/},
	{"hose_charge_action",		NULL, 0,		NULL, 0,		NULL, 0,	"2"/*HOSE_CHARGE_START*/},
	{"language_select",		NULL, 0,		NULL, 0,		NULL, 0,	"0"/*ENGLISH_TRANSLATION*/},
	{"no_flow_timer",		NULL, 0,		NULL, 0,		NULL, 0,	"180"},
	{"temp_comp_msg",		NULL, 0,		NULL, 0,		NULL, 0,	"1"/*YES*/},
	{"presets_allowed",		NULL, 0,		NULL, 0,		NULL, 0,	"3"/*GV_PRESET and NET_PRESET*/},
	{"prompt_mult_deliveries",	NULL, 0,		NULL, 0,		NULL, 0,	"1"/*YES*/},
	{"prompt_price_tax",		NULL, 0,		NULL, 0,		NULL, 0,	"1"/*YES*/},
	{"ticket_format",		NULL, 0,		NULL, 0,		NULL, 0,	"0"/*COMPACT*/},
	{"printer_one_type",		NULL, 0,		NULL, 0,		NULL, 0,	"1"/*EPSON_290_SLIP*/},
	//{"printer_two_type",		NULL, 0,		NULL, 0,		NULL, 0,	"0"/*PRINTER_NOT_USED*/},
};
struct s_ascii_write_values write_SI_meter_config[] = {
	//col_name		primary key 1		primary key 2		primary key 3	new value
	{"meter_name",		"meter_config_id", 1, 	NULL, 0, 		NULL, 0, 	"Genesis 1"},
	{"k_factor",		"meter_config_id", 1, 	NULL, 0, 		NULL, 0, 	"100.0"},
	{"mfac_dev_pct",	"meter_config_id", 1, 	NULL, 0, 		NULL, 0, 	"10.0"},		// large % for our test meter factors

	{"meter_name",		"meter_config_id", 2, 	NULL, 0, 		NULL, 0, 	"Genesis 2"},
	{"k_factor",		"meter_config_id", 2, 	NULL, 0, 		NULL, 0, 	"200.0"},
	{"mfac_dev_pct",	"meter_config_id", 2, 	NULL, 0, 		NULL, 0, 	"10.0"},  		// large % for our test meter factors
};
struct s_ascii_write_values write_SI_product_config[] = {
	//col_name		primary key 1			primary key 2	primary key 3	new value
	{"prd_name",		"product_config_id", 1, 	NULL, 0, 	NULL, 0, 	"Heating Oil"},
	{"prd_code",		"product_config_id", 1, 	NULL, 0, 	NULL, 0, 	"H101"},
	{"prd_group",		"product_config_id", 1, 	NULL, 0, 	NULL, 0, 	"0"/*DISTILLATE*/},
	{"prd_type",		"product_config_id", 1, 	NULL, 0, 	NULL, 0, 	"1"/*API_B_REFINED*/},
	{"ref_dens_units",	"product_config_id", 1, 	NULL, 0, 	NULL, 0, 	"2"/*KGM3*/},
	{"ref_density",		"product_config_id", 1, 	NULL, 0, 	NULL, 0, 	"805.5"},		////CTL = 0.98120 @35.4C
	{"first_stage_closure",	"product_config_id", 1, 	NULL, 0, 	NULL, 0, 	"10.0"},
	{"maint_temp",		"product_config_id", 1, 	NULL, 0, 	NULL, 0, 	"35.4"},

	{"prd_name",		"product_config_id", 2, 	NULL, 0, 	NULL, 0, 	"Propane"},
	{"prd_code",		"product_config_id", 2, 	NULL, 0, 	NULL, 0, 	"P101"},
	{"prd_group",		"product_config_id", 2, 	NULL, 0, 	NULL, 0, 	"8"/*LPG*/},
	{"prd_type",		"product_config_id", 2, 	NULL, 0, 	NULL, 0, 	"4"/*API_E_LPG*/},
	{"ref_dens_units",	"product_config_id", 2, 	NULL, 0, 	NULL, 0, 	"2"/*KG_M3*/},
	{"ref_density",		"product_config_id", 2, 	NULL, 0, 	NULL, 0, 	"539.5"},		////CTL = 0.84917 @68.35C
	{"first_stage_closure",	"product_config_id", 2, 	NULL, 0, 	NULL, 0, 	"10.0"},
	{"maint_temp",		"product_config_id", 2, 	NULL, 0, 	NULL, 0, 	"68.35"},

	{"prd_name",		"product_config_id", 3, 	NULL, 0, 	NULL, 0, 	"87 OCT"},
	{"prd_code",		"product_config_id", 3, 	NULL, 0, 	NULL, 0, 	"D101"},
	{"prd_group",		"product_config_id", 3, 	NULL, 0, 	NULL, 0, 	"4"/*GASOLINE*/},
	{"prd_type",		"product_config_id", 3, 	NULL, 0, 	NULL, 0, 	"1"/*API_B_REFINED*/},
	{"ref_dens_units",	"product_config_id", 3, 	NULL, 0, 	NULL, 0, 	"2"/*KG_M3*/},
	{"ref_density",		"product_config_id", 3, 	NULL, 0, 	NULL, 0, 	"820.0"},
	{"first_stage_closure",	"product_config_id", 3, 	NULL, 0, 	NULL, 0, 	"10.0"},
	{"maint_temp",		"product_config_id", 3, 	NULL, 0, 	NULL, 0, 	"999.0"/*use live temp*/}

};

#define NUM_SI_SYSTEM_CONFIG_PARAMS sizeof(write_SI_system_config)/sizeof(struct s_ascii_write_values)
#define NUM_SI_METER_CONFIG_PARAMS sizeof(write_SI_meter_config)/sizeof(struct s_ascii_write_values)
#define NUM_SI_PRODUCT_CONFIG_PARAMS sizeof(write_SI_product_config)/sizeof(struct s_ascii_write_values)

void DBRevel::SI_field_test_init ( sqlite3 *dbHandle, const char *dbName, sqlite3 *dbHandle_eventlog )
{
	int rc;
	unsigned int i;

	// verify hard coded SQL strings are valid
	assert(0==API_GRAVITY && 1==REL_DENS && 2==KGM3 && 1==API_B_REFINED && 4==API_E_LPG && 0==HUNDREDTHS && 0==PER_MIN);
	assert(0==GAL && 1==LIT && 2==BBL && 3==M3 && 0==LBS && 1==KGS && 0==DEG_F && 1==DEG_C);

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "system_config", write_SI_system_config, NUM_SI_SYSTEM_CONFIG_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_SI_SYSTEM_CONFIG_PARAMS; i++)
			if (write_SI_system_config[i].write_error != SQLITE_OK)
				fprintf(stderr, "system_config Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "meter_config", write_SI_meter_config, NUM_SI_METER_CONFIG_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_SI_METER_CONFIG_PARAMS; i++)
			if (write_SI_meter_config[i].write_error != SQLITE_OK)
				fprintf(stderr, "meter_config Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "product_config", write_SI_product_config, NUM_SI_PRODUCT_CONFIG_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_SI_PRODUCT_CONFIG_PARAMS; i++)
			if (write_SI_product_config[i].write_error != SQLITE_OK)
				fprintf(stderr, "product_config Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "meter_factors", write_meter_factors, NUM_METER_FACTOR_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_METER_FACTOR_PARAMS; i++)
			if (write_meter_factors[i].write_error != SQLITE_OK)
				fprintf(stderr, "meter_factors Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "dig_in_config", write_dig_in_config, NUM_DIG_IN_CONFIG_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_DIG_IN_CONFIG_PARAMS; i++)
			if (write_dig_in_config[i].write_error != SQLITE_OK)
				fprintf(stderr, "dig_in_config Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "dig_out_config", write_dig_out_config, NUM_DIG_OUT_CONFIG_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_DIG_OUT_CONFIG_PARAMS; i++)
			if (write_dig_out_config[i].write_error != SQLITE_OK)
				fprintf(stderr, "dig_out_config Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

	rc = DATABASE_secure_write_ascii(dbHandle, dbName, 0, dbHandle_eventlog, "serial_port_config", write_serial_port_config, NUM_SERIAL_PORT_PARAMS, SEC_LEVEL_5);
	if (rc != SQLITE_OK) {
		for (i=0; i<NUM_SERIAL_PORT_PARAMS; i++)
			if (write_serial_port_config[i].write_error != SQLITE_OK)
				fprintf(stderr, "serial_port_config Param Write Error - pararm #%d - error code #%d\n", i+1, rc);
	}

}



#endif
