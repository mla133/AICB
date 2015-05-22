/*********************************************************************************
*                   Smart Additive Injector Pass Thru Functions                  *
*                               (SAI_PASS.C)                                     *
*   -For the Accuload III.                                                       *
*   -Michael Becker                                                              *
*                                                                                *
*   This is the file which contains the specific fcns related to Smart Additive  *
*   Injector Pass Thru Communications.                                           *
*                                                                                *
*       - SAI_pass_thru_xmit                                                     *
*       - SAI_pass_thru_rec                                                      *
*       - Xmit_SAI_pass_thru                                                     *
*   Initial Editing/Creation Date: 11/24/98                                      *
*********************************************************************************/


#include "options.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "database.h"
//TODO: #include "init.h"
#include "comdrv.h"
#include "add_inj.h"
#include "comm_inj.h"
#include "sai_pass.h"
#include "comm.h"


//TODO: #pragma sep_on
/* 	This is the flag that coordinates the SAI pass thru functionality 
	between tasks.  It makes sure that the timing is consistant. */
enum SAI_Pass_Status saiPassThruState[COMDRV_last_virt_port];


static unsigned char sai_pass_tx_buf[COMDRV_last_virt_port][256];
static unsigned char sai_pass_tx_length[COMDRV_last_virt_port];

struct SAI_Pass_Thru_Response saiPassThruRecord[COMDRV_last_virt_port];

// Right now, we need to save the current port in operation, so we may 
// grab the correct data when asked for it.  Later, maybe this data may be provided 
// by the calling fcn?
enum COMDRV_virtual_ports curSAIPassThruPort;
//TODO: #pragma sep_off


static enum COMDRV_virtual_ports DecodeSAIPassThruAddress (unsigned char *buf)
{
	//--------------------------------------------
	char localAddress[4];
	unsigned int address;
	unsigned char i;
	unsigned char localPort;
	//--------------------------------------------

	// Grab what should be the address, copy it locally, ad null terminate it
	memcpy(&localAddress[0], buf, 3);
	localAddress[3] = (unsigned char)0;
	
	// Get an integer from this address
	address = atoi(&localAddress[0]);

	// Verify preliminarily the address
	if ((address == 0) || (address > 999))
		// BAD  The addresses may not be in this range
		return(COMDRV_last_virt_port);


	for (i=0; i<MAX_INJECTORS; i++)
		{
		if (	(pDB.add_inj.inj[i].inj_address == address)
			&&	(rDB.add_inj.inj[i].type 	!= NOT_CONFIG)
			)
			{
			localPort = rDB.add_inj.inj[i].port;
			saiPassThruRecord[localPort].message_port 	= (enum COMDRV_virtual_ports)localPort;
			saiPassThruRecord[localPort].message_address 	= address;
			// GOOD  The address checked
			return((enum COMDRV_virtual_ports)localPort);
			}

		}

	// BAD  Did not find the address programmed
	return(COMDRV_last_virt_port);

}



//---------------------------------------------------------------------------------
/* USER INTERFACE */
/*  This fcn is called with a buffer location and a length.  It is formatted 
    for Smart Additive Inj Communication and sent out. It will return a 1 if it
    can't xmit now, and a 0 if everything is ok. */
//---------------------------------------------------------------------------------
enum COMM_error_codes SAI_pass_thru_xmit(unsigned char *buf, unsigned char length)
{
	/******************************************/
    unsigned char i;
	unsigned char arm;
	unsigned char accuload_released = 0;
    unsigned char sai_not_installed_flag = 1;
	unsigned char sai_address_decode_failed = 0;
	register enum COMDRV_virtual_ports curPort;
	/******************************************/

	// With multiple comm ports, we will now decode the address and use that to 
	// send out on the right comm port
	curPort = DecodeSAIPassThruAddress (buf);

	if (curPort == COMDRV_last_virt_port)
		sai_address_decode_failed = 1;


	/* This means that there is already one being processed, so wait 'till we're done. */    							   
    if (saiPassThruState[curPort] != SAI_PASS_NO_MSG) 
		/*This is a 6. */
    	return (COMM_OP_NOT_ALLOWED);


	/**** Look for various conditions on the AccuLoad ***********/
    for (i=0; i<MAX_INJECTORS; i++)
        {
        if (!IS_SMART_INJECTOR(i))
            {
            sai_not_installed_flag = 0;
            break;
            }
        }

	for (arm=0; arm<MAX_LOAD_ARMS; arm++)
		{
		if (rDB.arm[arm].accuload_released == TRUE)
			accuload_released = 1;
		}
	/************************************************************/


	/*** Check to make sure everything is OKey Dokey ************/
    if (sai_not_installed_flag)
        {
        saiPassThruState[curPort] = SAI_PASS_NO_MSG;
		/* This is a 19. */
        saiPassThruRecord[curPort].message_status = COMM_OPTION_NOT_INSTALLED;
		return (COMM_OPTION_NOT_INSTALLED);
        }
    else if (sai_address_decode_failed)
        {
        saiPassThruState[curPort] = SAI_PASS_NO_MSG;
		/* This is a 19. */
        saiPassThruRecord[curPort].message_status = COMM_OPTION_NOT_INSTALLED;
		return (COMM_OPTION_NOT_INSTALLED);
        }
	else if (accuload_released)
		{
        saiPassThruState[curPort] = SAI_PASS_NO_MSG;
		/* This is a 02. */
        saiPassThruRecord[curPort].message_status = COMM_RELEASED;
		return (COMM_RELEASED);
		}
//	else if (rDB.in_program_mode == TRUE)
	else if (rDB.in_prg_common_area == TRUE)
		{
        saiPassThruState[curPort] = SAI_PASS_NO_MSG;
		/* This is a 01. */
        saiPassThruRecord[curPort].message_status = COMM_IN_PROG_MODE;
		return (COMM_IN_PROG_MODE);
		}
    else
        {
				curSAIPassThruPort = curPort;  // remember port used for last command issued R6:JMP 12/01
        memcpy(&sai_pass_tx_buf[curPort][0], buf, length);  
        sai_pass_tx_length[curPort] = length;
        saiPassThruState[curPort] = SAI_PASS_TX;
		/* This is a 06. */
        saiPassThruRecord[curPort].message_status = COMM_OP_NOT_ALLOWED;
	   	/* This is a 254. */
    	return(COMM_OK);
        }
	/************************************************************/
}



/* USER INTERFACE */
/* Returns the pointer to the beginning of the message. */
struct SAI_Pass_Thru_Response *SAI_pass_thru_rec (void)
{
	//-----------------------------------------------
	unsigned char i;
	unsigned char sai_not_installed_flag = 1;	// R6:JMP 12/01 - was not being initialized
	register enum COMDRV_virtual_ports curPort;
	//-----------------------------------------------

	curPort = curSAIPassThruPort;


    for (i=0; i<MAX_INJECTORS; i++)
        {
        if (!IS_SMART_INJECTOR(i))
            {
            sai_not_installed_flag = 0;
            break;
            }
        }


    if (sai_not_installed_flag)
		/* This is a 19. */
        saiPassThruRecord[curPort].message_status = COMM_OPTION_NOT_INSTALLED;

	else if (saiPassThruRecord[curPort].message_status == (enum COMM_error_codes)0)
		/* This is a 06. */
        saiPassThruRecord[curPort].message_status = COMM_OP_NOT_ALLOWED;


    return (&saiPassThruRecord[curPort]);
}



/* This fcn sends it out the wire.  It is called from inj_tx task. */
/* Internal fcn */
void Xmit_SAI_pass_thru (enum COMDRV_virtual_ports CurPort)
{
	/******************************************************/
	register struct SAI_port_data_struct 	*pSAIPort;
	/******************************************************/

	pSAIPort = &SAIPort[CurPort];

    pSAIPort->TxIndex = 0; 
    *(pSAIPort->TxBuf + pSAIPort->TxIndex) = STX_SAI;
	pSAIPort->TxIndex++;

    memcpy((pSAIPort->TxBuf + pSAIPort->TxIndex), &sai_pass_tx_buf[CurPort][0], sai_pass_tx_length[CurPort]);
	pSAIPort->TxIndex += sai_pass_tx_length[CurPort];

    FormatSAIPackettFooter (CurPort);
    SmartAITransmit(CurPort);

	return;
}

					
