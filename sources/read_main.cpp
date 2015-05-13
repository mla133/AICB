/**
 * main.cpp
 *
 *  @date: 	July 23, 2014
 *  @author: 	Mike Ryan <mike.ryan@fmcti.com>
 *
 *  @section DESCRIPTION
 *
 *  This module contains the main entry point for the JSON reader program.
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
#if 0
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdint.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "frozen.h"
#include <openssl/sha.h>	// For calculating hash

#include "tiva_interface.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdint.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/inotify.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <math.h>
#include <string.h>
#include "frozen.h"
#include <openssl/sha.h>	// For calculating hash
#include <sys/sysinfo.h>	// Used when getting system time
#include <sys/types.h>
#include <sys/stat.h>
#include "sqlite3.h"

#include "tiva_interface.h"


// ---------------------------------------------------------------------------
// D E F I N I T I O N S

#define SHA_LENGTH_IN_BYTES		32
#define MAX_LEN_JSON			4096


// ---------------------------------------------------------------------------
// G L O B A L    D A T A

char inbuf[MAX_LEN_JSON];
char input_without_hash[MAX_LEN_JSON];
char printer_response_buf[512];
char smith_comm_buf[512];
char command_response_buf[512];
char smart_injector_buf[512];
char promass_buf[512];
char promass_response_buf[512];
char command_buf[512];
input_io_board_data_t input_io_control_data;
struct json_token tokens[2000];
int convert_json = false;
int g_uiInstance;
char g_uiBoard[4];
int g_uiSeqnum;
int g_uiTimestamp;
int g_uiType;
int g_uiResync_pulses;
int g_uiReset_ack;

// CPU utilization on Tiva.   Probably will remove when deployed.

int g_tiva_cpu_load = 0;

// Security-related variables.

unsigned char key[20] =		// Key is 20 bytes long in RFC 2104
{
	0x0B, 0x0B, 0x0B, 0x0B,
	0x0B, 0x0B, 0x0B, 0x0B,
	0x0B, 0x0B, 0x0B, 0x0B,
	0x0B, 0x0B, 0x0B, 0x0B,
	0x0B, 0x0B, 0x0B, 0x0B
};
static unsigned char i_pad[64];			// Part of HMAC algorithm.
static unsigned char o_pad[64];			// So is this.
unsigned char received_keyed_hash[SHA_LENGTH_IN_BYTES];
unsigned char calculated_keyed_hash[SHA_LENGTH_IN_BYTES];



// ---------------------------------------------------------------------------
// Function:	main
// Purpose:		Entry point for the JSON interpreter program.
// Inputs:		argv[0] = name of program
//				argv[1] = "data", "printer", "serial", or "command".
//				argv[2] = "a4m", "a4b", "a4i1", or "a4i2".

int main(int argc, char **argv)
{
	int interpret_json(char *);
	int compare_keyed_hashes(char *);
	int open_fifo(char *);
	int write_to_file(char *, int);
	int braces;
	int i;
	int readfifo;
	char inchar;
	int nbytes;
	char fifo_name[128];
	char output_file_name[128];
	char dest_board[20];
	char instance;
	int num_chars;
	int status;




	// Open the read FIFO.
#if USE_STDIN
	readfifo = open_fifo(fifo_name);
	if (readfifo == 0)
	{
		printf("%s: error, FIFO '%s' does not exist\n", __FUNCTION__, fifo_name);
		exit (-1);
	}
#endif
	// Wait until data is available for reading, then convert the input JSON into binary
	// (if source is the io_control task, otherwise leave the data in ASCII).  Then
	// calculate a keyed hash over the input ASCII and compared with received hash.
	// If these match, write data to a file.

	memset(inbuf, 0, sizeof(inbuf));
	num_chars = 0;
	braces = 0;
	i = 0;
	while (1)
	{
		inchar = getc(stdin);
		nbytes = 1;

		num_chars++;
//printf("%s: received '%c', num=%d\n", __FUNCTION__, inchar, num_chars);
		inbuf[i++] = inchar;
		if (inchar == '{') braces++;
		if (inchar == '}') braces--;
		if (braces == 0)
		{
printf("%s: received %d chars, JSON='%s'\n", __FUNCTION__, num_chars, inbuf);
			memset(output_file_name, 0, sizeof(output_file_name));
			g_uiInstance = 0;
			memset(g_uiBoard, 0, sizeof(g_uiBoard));

			status = interpret_json(inbuf);



			if (g_uiType == TYPE_IO_CONTROL)
			    {
			        sprintf(output_file_name, "/var/tmp/%s/%s", g_uiBoard, OUTPUT_FILE_DATA);
			        convert_json = true;
			    }
			    else if (g_uiType == TYPE_PRINTER)
			    {
			        if(g_uiInstance < 1 || g_uiInstance > 2)
			        {
			            printf("Invalid Argument: Enter either 1 or 2 for the number of ports.\n");
			            exit(-1);
			        }
			        sprintf(output_file_name, "/var/tmp/%s/%s%c", g_uiBoard, OUTPUT_FILE_PRINTER, g_uiInstance + '0');
			    }
			    else if (g_uiType == TYPE_SMITH_COMM)
			    {
				if(g_uiInstance < 1 || g_uiInstance > 2)
			        {
			            printf("Invalid Argument: Enter either 1 or 2 for the number of ports.\n");
			            exit(-1);
			        }
			        sprintf(output_file_name, "/var/tmp/%s/%s%c", g_uiBoard, OUTPUT_FILE_SMITH_COMM, g_uiInstance + '0');
			    }
			    else if (g_uiType == TYPE_SMART_INJECTOR)
			    {
				    if(g_uiInstance < 1 || g_uiInstance > 2)
			        {
			            printf("Invalid Argument: Enter either 1 or 2 for the number of ports.\n");
			            exit(-1);
			        }
			        sprintf(output_file_name, "/var/tmp/%s/%s%c", g_uiBoard, OUTPUT_FILE_SMART_INJECTOR, g_uiInstance + '0');
			    }
			    else if (g_uiType == TYPE_COMMAND)
			    {
			        sprintf(output_file_name, "/var/tmp/%s/%s", g_uiBoard, OUTPUT_FILE_COMMAND);
			    }
			    else if (g_uiType == TYPE_PROMASS)
			    {
			        sprintf(output_file_name, "/var/tmp/%s", OUTPUT_FILE_PROMASS);
			    }
			    else
			    {
			        printf("%s: Error, unknown arg '%s'\n", __FUNCTION__, argv[1]);
			        exit(-1);
			    }

			printf("File Name: %s\n", output_file_name);
			if (status == 1)
			{
//printf("%s: JSON is okay\n", __FUNCTION__);
				if (compare_keyed_hashes(inbuf) == 1)
				{
					write_to_file(output_file_name, input_io_control_data.type);
				}
			}
			i = 0;
			memset(inbuf, 0, sizeof(inbuf));
			num_chars = 0;
		}
	}
}


// ---------------------------------------------------------------------------
// Name:		compare_keyed_hashes
// Purpose:		Given ASCII (JSON) text and a keyed hash already extracted
//				from the text, remove the existing "hash:" attribute and values
//				and calculate another hash over the remaining ASCII.  Then
//				compare the two hashes and return the result (1=same).
// Inputs:		p = ptr to JSON text.
//				received_keyed_hash (global memory)
// Outputs:		Creates "calculated_keyed_hash" in global memory.
// Returns:		1 if hashes are identical, else 0.

int compare_keyed_hashes(char *p)
{
	void calc_rfc_2104_hmac(unsigned char *, char *, int, unsigned char *);

	char * hash_pos;

	// Remove the "hash:" attribute and the leading '{' character from the input.
	// It might seem poor practice to use a hard-coded length here but we know
	// that the input is always of this form, by definition, unless there's an error in input.

//printf("%s: input=%s\n", __FUNCTION__, p);
	memset(input_without_hash, 0, sizeof(input_without_hash));
	input_without_hash[0] = '{';
	p++;

	hash_pos = strstr(p,"h");

	if(hash_pos != NULL)
	{
		strncpy((input_without_hash + 1), p, hash_pos - (p + 2));
		input_without_hash[(hash_pos - (p + 1))] = '}';
	}

	// Calculate a keyed hash over the input that lacks the keyed hash attribute, then compare.

	calc_rfc_2104_hmac(key, input_without_hash, strlen(input_without_hash), calculated_keyed_hash);
#if 0
printf("%s: stripped=%s\n", __FUNCTION__, input_without_hash);
int i;
printf("%s: calculated hash follows\n", __FUNCTION__);
for (i=0; i<32; i++)
{
	printf("%02x ", calculated_keyed_hash[i]);
}
printf("\n");
printf("%s: received follows\n", __FUNCTION__);
for (i=0; i<32; i++)
{
	printf("%02x ", received_keyed_hash[i]);
}
printf("\n");
#endif
	if (memcmp(received_keyed_hash, calculated_keyed_hash, SHA_LENGTH_IN_BYTES) != 0)
	{
		printf("%s: hash failure, reject\n", __FUNCTION__);
		return 0;
	}
	//printf("%s: hash passed, accept\n", __FUNCTION__);
	return 1;
}


// ---------------------------------------------------------------------------
// Name:		interpret_json
// Purpose:		Given ASCII (JSON) text, parses and outputs binary equivalent.
//				Extracts keyed hash attribute and value, converts to binary,
//				saves in received_keyed_hash buffer.
// Inputs:		p = ptr to JSON text.
// Outputs:		Binary data is written to data structure "input_io_control_data".
//				ASCII data if supplied is written to 'input_ascii_data'.
// Returns:		1 if JSON was interpreted successfully, else 0.

int interpret_json(char *p)
{
	static int bad_count = 0;
	int decode_pulses(char *, const struct json_token *);
	int decode_valve_status(char *, const struct json_token *);
	int decode_hash(char *, const struct json_token *);
	int decode_seqnum(char *, const struct json_token *);
	int decode_timestamp(char *, const struct json_token *);
	int decode_cpu_load(char *, const struct json_token *);
	int decode_board_identity(const struct json_token *);
	int decode_dig_in(char *, const struct json_token *);
	int decode_type(char *, const struct json_token *);
	int decode_instance(char *, const struct json_token *);
	int decode_resync_pulses(char *, const struct json_token *);
	int decode_reset_ack(char *, const struct json_token *);
	int decode_ana_in(char *, const struct json_token *);
	int decode_printer_response(char *, const struct json_token *);
	int decode_smith_comm_message(char *, const struct json_token *);
	int decode_command_message(char *, const struct json_token *);
	int decode_smart_injector_message(char *, const struct json_token *);
	int decode_command_message(char *, const struct json_token *);
	int decode_promass_message(char *, const struct json_token *);
	int size_tok_array = sizeof(tokens) / sizeof(tokens[0]);
	const struct json_token *tok = NULL;
	char *json = p;
	int status;

//	printf("\n\n\n****************%s: RX JSON='%s'****************\n\n", __FUNCTION__, p);

	// Parse the input JSON into the "tokens" array.

	if (strlen(p) == 1 && (*p == 13 || *p == 10)) return 0;
	status = parse_json(json, strlen(json), tokens, size_tok_array);
	if (status <= 0)
	{
		printf("\n%s: JSON invalid, parsing failed: ", __FUNCTION__);
		if (status == JSON_STRING_INVALID) printf("STRING IS INVALID\n");
		if (status == JSON_STRING_INCOMPLETE) printf("STRING IS INCOMPLETE\n");
		if (status == JSON_TOKEN_ARRAY_TOO_SMALL) printf("TOKEN ARRAY IS TOO SMALL\n");
printf("\n\n\n****************%s: bad JSON='%s'****************\n\n", __FUNCTION__, p);
		bad_count++;
		if (bad_count > 3)
		{
			printf("\n%s: Error, out of sync, exiting\n", __FUNCTION__);
			exit (-1);
		}
		return 0;
	}
	bad_count = 0;

	// If the object-name "h" exists, parse the 32 bytes comprising the keyed hash.

	tok = find_json_token(tokens, "h");
	if (tok != 0)
	{
		status = decode_hash(p, tok);
//		printf("%s: hash?=%d\n", __FUNCTION__, status);
#if 0
// NOTE:  When hashing is required, ignore this packet if hash fails?  TBD.
		if (status == 0)
		{
			return;
		}
#endif
	}

	// If the object-name "id" exists, parse its identity.

	tok = find_json_token(tokens, "id");
	if (tok != 0)
	{
		input_io_control_data.board_role = decode_board_identity(tok);

		if(input_io_control_data.board_role != board_unknown)
		{
			strncpy(g_uiBoard, tok->ptr, tok->len);
		}
//		printf("%s: role?=%d\n", __FUNCTION__, input_io_control_data.board_role);
	}

	// Extract the sequence number.

	tok = find_json_token(tokens, "seq");
	if (tok != 0)
	{
		input_io_control_data.seqnum = decode_seqnum(p, tok);
//		printf("%s: seqnum=%d\n", __FUNCTION__, g_uiSeqnum);
	}

	// Extract the timestamp.

	tok = find_json_token(tokens, "ts");
	if (tok != 0)
	{
		input_io_control_data.timestamp = decode_timestamp(p, tok);
		//printf("%s: timestamp=%d\n", __FUNCTION__, g_uiTimestamp);
		input_io_control_data.timestamp = g_uiTimestamp;
	}

	// Extract the Tiva's CPU utilization percentage.

	tok = find_json_token(tokens, "cpu");
	if (tok != 0)
	{
		if (decode_cpu_load(p, tok))
		{
			printf("%s: tiva_cpu_load_percentage: %d\n", __FUNCTION__, g_tiva_cpu_load);
		}
	}

	// Extract the packet type.

	tok = find_json_token(tokens, "type");
	if (tok != 0)
	{
		decode_type(p, tok);
		//printf("%s: type=%d\n", __FUNCTION__, g_uiType);
		input_io_control_data.type = g_uiType;
	}


	tok = find_json_token(tokens, "ins");
	if (tok != 0)
	{
		decode_instance(p, tok);
		//printf("%s: type=%d\n", __FUNCTION__, g_uiType);
	}

	// If the payload contains ASCII data, copy it out and stop parsing the input here.

	if (g_uiType == TYPE_PRINTER)
	{
		tok = find_json_token(tokens, "ps");
		if (tok != 0)
		{
			memset(printer_response_buf, 0, sizeof(printer_response_buf));
			status = decode_printer_response(p, tok);
			if (status == 1)
			{
				if (!strcmp(printer_response_buf, "FINISHED_NO_ERROR"))
				{
//					printf("DONE, no errors\n");
				}
			}
		}
		return 1;
	}
	else if (g_uiType == TYPE_SMITH_COMM)
	{
		tok = find_json_token(tokens, "text");
		if (tok != 0)
		{
			memset(smith_comm_buf, 0, sizeof(smith_comm_buf));
			status = decode_smith_comm_message(p, tok);
			if (status == 1)
			{
//printf("%s: SmithComm text found, copied to buffer\n", __FUNCTION__);
			}
		}
		return 1;
	}
	else if (g_uiType == TYPE_SMART_INJECTOR)
	{
		tok = find_json_token(tokens, "text");
		if (tok != 0)
		{
			memset(smart_injector_buf, 0, sizeof(smart_injector_buf));
			status = decode_smart_injector_message(p, tok);
			if (status == 1)
			{
//printf("%s: smart injector text found, copied to buffer\n", __FUNCTION__);
				fprintf(stderr, "\E[32;40mRecv'd[%s]\E[0m\n",smart_injector_buf);
			}
		}
		return 1;
	}
	else if (g_uiType == TYPE_COMMAND)
	{
		tok = find_json_token(tokens, "text");
		if (tok != 0)
		{
			memset(command_response_buf, 0, sizeof(command_response_buf));
			status = decode_command_message(p, tok);
			if (status == 1)
			{
//printf("%s: command response text found, copied to buffer\n", __FUNCTION__);
			}
		}
		return 1;
	}
	else if (g_uiType == TYPE_COMMAND)
	{
		memset(command_buf, 0, sizeof(command_buf));
		status = decode_command_message(p, tok);
		if (status == 1)
		{
			printf("%s: command text found, copied to buffer\n", __FUNCTION__);
		}
	}
	else if (g_uiType == TYPE_PROMASS)
	{
		tok = find_json_token(tokens, "text");
		if (tok != 0)
		{
			memset(promass_buf, 0, sizeof(promass_buf));
			status = decode_promass_message(p, tok);
			if (status == 1)
			{
//printf("%s: smart injector text found, copied to buffer\n", __FUNCTION__);
			}
		}
		return 1;
	}

	// If the remaining input JSON contains only ASCII data coming from
	// a UART or printer, or a command response, stop here.

	if (convert_json == false)
	{
//		printf("%s: convert=false\n", __FUNCTION__);
		return 1;
	}

	// If the object-name "pi" exists, parse current pulse counts.

	tok = find_json_token(tokens, "pi");
	if (tok != 0)
	{
		status = decode_pulses(p, tok);
//		printf("%s: pulses?=%d\n", __FUNCTION__, status);
	}

	// If the object-name "vs" exists, parse flow control valve status.

	tok = find_json_token(tokens, "vs");
	if (tok != 0)
	{
		status = decode_valve_status(p, tok);
//		printf("%s: valve_status?=%d\n", __FUNCTION__, status);
	}

	// If the object-name "di" exists, parse digital inputs.

	tok = find_json_token(tokens, "di");
	if (tok != 0)
	{
		status = decode_dig_in(p, tok);
//		printf("%s: dig_in?=%d\n", __FUNCTION__, status);
	}

	// If the object-name "ai" exists, parse analog inputs.

	tok = find_json_token(tokens, "ai");
	if (tok != 0)
	{
		status = decode_ana_in(p, tok);
	}

	// If the object-name "rp" exists, get it.

	tok = find_json_token(tokens, "rp");
	if (tok != 0)
	{
		status = decode_resync_pulses(p, tok);
//		printf("%s: got resync flag?=%d\n", __FUNCTION__, status);
	}

	// If the object-name "ra" exists, get it.

	tok = find_json_token(tokens, "ra");
	if (tok != 0)
	{
		status = decode_reset_ack(p, tok);
//		printf("%s: got reset_ack flag?=%d\n", __FUNCTION__, status);
	}
	if (*p == 10) return 1;
	return 1;
}


// ---------------------------------------------------------------------------
// Name:		decode_smith_comm_message
// Purpose:		Copies incoming ASCII text to a buffer.
// Inputs:		p = ptr to text.
//				tok = ptr to struct json_token
// Outputs:		g_uiType is initialized.
// Returns:		1 if text was copied in, else 0.

int decode_smith_comm_message(char *p, const struct json_token *tok)
{
	char search_buffer[20];
	char *p1;
	int i;

	sprintf(search_buffer, "text");
	tok = find_json_token(tokens, search_buffer);
	if (tok != 0)
	{
		p1 = (char *)tok->ptr;
		for (i=0; i<(int) sizeof(smith_comm_buf); i++)
		{
			if (*(p1+i) == '\"') break;
//			printf("%c", *(p1+i));
			smith_comm_buf[i] = *(p1+i);
		}
		return 1;
	}
	return 0;
}


// ---------------------------------------------------------------------------
// Name:		decode_command_message
// Purpose:		Copies incoming ASCII text to a buffer.
// Inputs:		p = ptr to text.
//				tok = ptr to struct json_token
// Outputs:		g_uiType is initialized.
// Returns:		1 if text was copied in, else 0.

int decode_command_message(char *p, const struct json_token *tok)
{
	char search_buffer[20];
	char *p1;
	int i;

	sprintf(search_buffer, "text");
	tok = find_json_token(tokens, search_buffer);
	if (tok != 0)
	{
		p1 = (char *)tok->ptr;
		for (i=0; i<(int) sizeof(command_response_buf); i++)
		{
			if (*(p1+i) == '\"') break;
			command_response_buf[i] = *(p1+i);
		}
		return 1;
	}
	return 0;
}


// ---------------------------------------------------------------------------
// Name:		decode_smart_injector_message
// Purpose:		Copies incoming ASCII text to a buffer.
// Inputs:		p = ptr to text.
//				tok = ptr to struct json_token
// Outputs:		g_uiType is initialized.
// Returns:		1 if text was copied in, else 0.

int decode_smart_injector_message(char *p, const struct json_token *tok)
{
	char search_buffer[20];
	char *p1;
	int i;

	sprintf(search_buffer, "text");
	tok = find_json_token(tokens, search_buffer);
	if (tok != 0)
	{
		p1 = (char *)tok->ptr;
		for (i=0; i<(int) sizeof(smart_injector_buf); i++)
		{
			if (*(p1+i) == '\"') break;
			smart_injector_buf[i] = *(p1+i);
		}
		return 1;
	}
	return 0;
}

// ---------------------------------------------------------------------------
// Name:		decode_promass_message
// Purpose:		Copies incoming ASCII text to a buffer.
// Inputs:		p = ptr to text.
//				tok = ptr to struct json_token
// Outputs:		g_uiType is initialized.
// Returns:		1 if text was copied in, else 0.

int decode_promass_message(char *p, const struct json_token *tok)
{
	char search_buffer[20];
	char *p1;
	int i;

	sprintf(search_buffer, "text");
	tok = find_json_token(tokens, search_buffer);
	if (tok != 0)
	{
		p1 = (char *)tok->ptr;
		for (i=0; i<(int) sizeof(promass_buf); i++)
		{
			if (*(p1+i) == '\"') break;
			promass_buf[i] = *(p1+i);
		}
		return 1;
	}
	return 0;
}


// ---------------------------------------------------------------------------
// Name:		decode_printer_response
// Purpose:		Copies incoming ASCII text to a buffer.
// Inputs:		p = ptr to text.
//				tok = ptr to struct json_token
// Outputs:		g_uiType is initialized.
// Returns:		1 if text was copied in, else 0.

int decode_printer_response(char *p, const struct json_token *tok)
{
	char search_buffer[100];
	char *p1;
	int i;

	sprintf(search_buffer, "ps");
	tok = find_json_token(tokens, search_buffer);
	if (tok != 0)
	{
		p1 = (char *)tok->ptr;
		for (i=0; i<sizeof(search_buffer); i++)
		{
			if (*(p1+i) == '\"') break;
//			printf("%c", *(p1+i));
			printer_response_buf[i] = *(p1+i);
		}
		return 1;
	}
	return 0;
}


// ---------------------------------------------------------------------------
// Name:		decode_type
// Purpose:		Given ASCII text, extract the incoming packet type.
// Inputs:		p = ptr to text.
//				tok = ptr to struct json_token
// Outputs:		g_uiType is initialized.
// Returns:		1 if packet type was decoded, else 0.

int decode_type(char *p, const struct json_token *tok)
{
	char search_buffer[20];

	sprintf(search_buffer, "type");
	tok = find_json_token(tokens, search_buffer);
	if (tok != 0)
	{
		if (!strncmp(tok->ptr, "d", strlen("d")))
		{
			g_uiType = TYPE_IO_CONTROL;
		}
		if (!strncmp(tok->ptr, "p", strlen("p")))
		{
			g_uiType = TYPE_PRINTER;
		}
		if (!strncmp(tok->ptr, "s", strlen("s")))
		{
			g_uiType = TYPE_SMITH_COMM;
		}
		if (!strncmp(tok->ptr, "si", strlen("si")))
		{
			g_uiType = TYPE_SMART_INJECTOR;
		}
		if (!strncmp(tok->ptr, "cmd", strlen("cmd")))
		{
			g_uiType = TYPE_COMMAND;
		}
		return 1;
	}
	return 0;
}

// ---------------------------------------------------------------------------
// Name:		decode_type
// Purpose:		Given ASCII text, extract the incoming packet type.
// Inputs:		p = ptr to text.
//				tok = ptr to struct json_token
// Outputs:		g_uiType is initialized.
// Returns:		1 if packet type was decoded, else 0.

int decode_instance(char *p, const struct json_token *tok)
{
	char search_buffer[20];

	sprintf(search_buffer, "ins");
	tok = find_json_token(tokens, search_buffer);
	if (tok != 0)
	{
		sscanf(tok->ptr, "%i", &g_uiInstance);
		return 1;
	}
	return 0;
}

// ---------------------------------------------------------------------------
// Name:		decode_seqnum
// Purpose:		Given ASCII text, extract the incoming sequence number.
// Inputs:		p = ptr to text.
//				tok = ptr to struct json_token
// Outputs:		g_uiSeqnum is initialized.
// Returns:		1 if sequence number was decoded, else 0.

int decode_seqnum(char *p, const struct json_token *tok)
{
	char search_buffer[20];

	sprintf(search_buffer, "seq");
	tok = find_json_token(tokens, search_buffer);
	if (tok != 0)
	{
		sscanf(tok->ptr, "%d", &g_uiSeqnum);
		return 1;
	}
	return 0;
}


// ---------------------------------------------------------------------------
// Name:		decode_resync_pulses
// Purpose:		Given ASCII text, extract the resync_pulses flag.
// Inputs:		p = ptr to text.
//				tok = ptr to struct json_token
// Outputs:		g_uiResync_pulses = 0 or 1.
// Returns:		0 (failure), 1 (success)

int decode_resync_pulses(char *p, const struct json_token *tok)
{
	char search_buffer[20];

	sprintf(search_buffer, "rp");
	tok = find_json_token(tokens, search_buffer);
	if (tok != 0)
	{
		sscanf(tok->ptr, "%d", &g_uiResync_pulses);
		input_io_control_data.resync_pulses = g_uiResync_pulses;
		return 1;
	}
	return 0;
}


// ---------------------------------------------------------------------------
// Name:		decode_reset_ack
// Purpose:		Given ASCII text, extract the reset_ack flag.
// Inputs:		p = ptr to text.
//				tok = ptr to struct json_token
// Outputs:		g_uiReset_ack = 0 or 1.
// Returns:		0 (failure), 1 (success)

int decode_reset_ack(char *p, const struct json_token *tok)
{
	char search_buffer[20];

	sprintf(search_buffer, "ra");
	tok = find_json_token(tokens, search_buffer);
	if (tok != 0)
	{
		sscanf(tok->ptr, "%d", &g_uiReset_ack);
		input_io_control_data.reset_ack = g_uiReset_ack;
		return 1;
	}
	return 0;
}


// ---------------------------------------------------------------------------
// Name:		decode_timestamp
// Purpose:		Given ASCII text, extract the incoming timestamp.
// Inputs:		p = ptr to text.
//				tok = ptr to struct json_token
// Outputs:		g_uiTimestamp is initialized.
// Returns:		1 if timestamp was decoded, else 0.

int decode_timestamp(char *p, const struct json_token *tok)
{
	char search_buffer[20];

	sprintf(search_buffer, "ts");
	tok = find_json_token(tokens, search_buffer);
	if (tok != 0)
	{
		sscanf(tok->ptr, "%d", &g_uiTimestamp);
		return 1;
	}
	return 0;
}


// ---------------------------------------------------------------------------
// Name:		decode_hash
// Purpose:		Given ASCII text, convert it to 256 bits and store in memory.
// Inputs:		p = ptr to text.
//				tok = ptr to struct json_token
// Outputs:		received_keyed_hash buffer receives 256-bit binary keyed hash value.
// Returns:		1 if hash was decoded, else 0.

int decode_hash(char *p, const struct json_token *tok)
{
	int i;
	char search_buffer[100];
	unsigned int hash_word;

	memset(received_keyed_hash, 0, sizeof(received_keyed_hash));
	for (i=0; i<SHA_LENGTH_IN_BYTES; i++)
	{
		sprintf(search_buffer, "h[%d]", i);
		tok = find_json_token(tokens, search_buffer);
		if (tok != 0)
		{
			sscanf(tok->ptr, "%02x", &hash_word);
			*(received_keyed_hash+i) = (unsigned char) hash_word;
		}
		else
		{
			return 0;
		}
	}
	return 1;
}


// ---------------------------------------------------------------------------
// Name:		decode_dig_in
// Purpose:		Extract digital inputs into global data.
// Inputs:		p = ptr to text.
//				ptok = ptr to struct json_token
// Outputs:		pout = ptr to output data structure.
// Returns:		1 if successfully read input values, else 0.

int decode_dig_in
(
	char *p,
	const struct json_token *ptok
)
{
	int i;
	char search_buffer[100];
	unsigned int hash_word;
	unsigned char *pout = input_io_control_data.dig_in;
	for (i=0; i<NUM_DIG_INS; i++)
	{
		sprintf(search_buffer, "di[%d]", i);
		ptok = find_json_token(tokens, search_buffer);
		if (ptok != 0)
		{
			sscanf(ptok->ptr, "%d", &hash_word);
			*(pout+i) = (unsigned char) hash_word;
		}
		else
		{
			return 0;
		}
	}
	return 1;
}


// ---------------------------------------------------------------------------
// Name:		decode_ana_in
// Purpose:		Extract analog input count values.
// Inputs:		p = ptr to text.
//				ptok = ptr to struct json_token
// Outputs:		pout = ptr to output data structure.
// Returns:		1 if successfully read input values, else 0.

int decode_ana_in
(
	char *p,
	const struct json_token *ptok
)
{
	int i;
	char search_buffer[100];
	unsigned int hash_word;
	unsigned int *pout = input_io_control_data.ana_in;
	for (i=0; i<NUM_ANA_IO; i++)
	{
		sprintf(search_buffer, "ai[%d]", i);
		ptok = find_json_token(tokens, search_buffer);
		if (ptok != 0)
		{
			sscanf(ptok->ptr, "%d", &hash_word);
			*(pout+i) = (unsigned short) hash_word;
		}
		else
		{
			return 0;
		}
	}
	return 1;
}


// ---------------------------------------------------------------------------
// Name:		decode_pulses
// Purpose:		Extract input pulse information into global data.
// Inputs:		p = ptr to text.
//				ptok = ptr to struct json_token
// Outputs:		Pulse counts in input_io_control_data are read in.
// Returns:		0 if no pulse information is found, else 1.

int decode_pulses
(
	char *p,
	const struct json_token *ptok
)
{
	const struct json_token *mytok;
	char stringcopy[1024];

	mytok = find_json_token(tokens, "pi");
	if (mytok != 0)
	{
		// Copy input token, eliminating square brackets and commas so sscanf can work.

		memset(stringcopy, 0, sizeof(stringcopy));
		strcpy(stringcopy, mytok->ptr);
		for (int i=0; i<mytok->len; i++)
		{
			if (*(mytok->ptr+i) == '[' || *(mytok->ptr+i) == ']' || *(mytok->ptr+i) == ',')
			{
				stringcopy[i] = ' ';
			}
		}
//		input_io_control_data.pulse_in[0].forward_pulse_count = 2;
		sscanf(stringcopy, "%d %d %d %ld %ld  %d %d %d %ld %ld  %d %d %d %ld %ld  %d %d %d %ld %ld",
				&input_io_control_data.pulse_in[0].forward_pulse_count,
				&input_io_control_data.pulse_in[0].reverse_pulse_count,
				&input_io_control_data.pulse_in[0].error_pulse_count,
				&input_io_control_data.pulse_in[0].period_a,
				&input_io_control_data.pulse_in[0].period_b,
				&input_io_control_data.pulse_in[1].forward_pulse_count,
				&input_io_control_data.pulse_in[1].reverse_pulse_count,
				&input_io_control_data.pulse_in[1].error_pulse_count,
				&input_io_control_data.pulse_in[1].period_a,
				&input_io_control_data.pulse_in[1].period_b,
				&input_io_control_data.pulse_in[2].forward_pulse_count,
				&input_io_control_data.pulse_in[2].reverse_pulse_count,
				&input_io_control_data.pulse_in[2].error_pulse_count,
				&input_io_control_data.pulse_in[2].period_a,
				&input_io_control_data.pulse_in[2].period_b,
				&input_io_control_data.pulse_in[3].forward_pulse_count,
				&input_io_control_data.pulse_in[3].reverse_pulse_count,
				&input_io_control_data.pulse_in[3].error_pulse_count,
				&input_io_control_data.pulse_in[3].period_a,
				&input_io_control_data.pulse_in[3].period_b);
		return 1;
	}
	else
		return 0;
}


// ---------------------------------------------------------------------------
// Name:		decode_valve_status
// Purpose:		Extract flow control valve status information into global data.
// Inputs:		p = ptr to text.
//				ptok = ptr to struct json_token
// Outputs:		Valve status and up/down solenoid counts in input_io_control_data are read in.
// Returns:		0 if no pulse information is found, else 1.

int decode_valve_status
(
	char *p,
	const struct json_token *ptok
)
{
	const struct json_token *mytok;
	char stringcopy[1024];

	mytok = find_json_token(tokens, "vs");
	if (mytok != 0)
	{
		// Copy input token, eliminating square brackets and commas so sscanf can work.

		memset(stringcopy, 0, sizeof(stringcopy));
		strcpy(stringcopy, mytok->ptr);
		for (int i=0; i<mytok->len; i++)
		{
			if (*(mytok->ptr+i) == '[' || *(mytok->ptr+i) == ']' || *(mytok->ptr+i) == ',')
			{
				stringcopy[i] = ' ';
			}
		}
		sscanf(stringcopy, "%d %d %d  %d %d %d  %d %d %d  %d %d %d  %d %d %d  %d %d %d  %d %d %d  %d %d %d",
				&input_io_control_data.flow_valve[0].valve_status,
				&input_io_control_data.flow_valve[0].up_solenoid_count,
				&input_io_control_data.flow_valve[0].down_solenoid_count,
				&input_io_control_data.flow_valve[1].valve_status,
				&input_io_control_data.flow_valve[1].up_solenoid_count,
				&input_io_control_data.flow_valve[1].down_solenoid_count,
				&input_io_control_data.flow_valve[2].valve_status,
				&input_io_control_data.flow_valve[2].up_solenoid_count,
				&input_io_control_data.flow_valve[2].down_solenoid_count,
				&input_io_control_data.flow_valve[3].valve_status,
				&input_io_control_data.flow_valve[3].up_solenoid_count,
				&input_io_control_data.flow_valve[3].down_solenoid_count,
				&input_io_control_data.flow_valve[4].valve_status,
				&input_io_control_data.flow_valve[4].up_solenoid_count,
				&input_io_control_data.flow_valve[4].down_solenoid_count,
				&input_io_control_data.flow_valve[5].valve_status,
				&input_io_control_data.flow_valve[5].up_solenoid_count,
				&input_io_control_data.flow_valve[5].down_solenoid_count,
				&input_io_control_data.flow_valve[6].valve_status,
				&input_io_control_data.flow_valve[6].up_solenoid_count,
				&input_io_control_data.flow_valve[6].down_solenoid_count,
				&input_io_control_data.flow_valve[7].valve_status,
				&input_io_control_data.flow_valve[7].up_solenoid_count,
				&input_io_control_data.flow_valve[7].down_solenoid_count);
		return 1;
	}
	else
		return 0;
}


// ---------------------------------------------------------------------------
// Name:		decode_board_identity
// Purpose:		Find the identity of the board that sent this payload.
// Inputs:		tok = ptr to struct json_token
// Outputs:		Nothing
// Returns:		Board identity enumeration

int decode_board_identity(const struct json_token *tok)
{
	if (!strncmp(tok->ptr, "a4m", tok->len))
	{
//printf("%s: board=A4M\n", __FUNCTION__);
		return board_a4m;
	}
	if (!strncmp(tok->ptr, "a4b", tok->len))
	{
//printf("%s: board=A4B\n", __FUNCTION__);
		return board_a4b;
	}
	if (!strncmp(tok->ptr, "a4i1", tok->len))
	{
//printf("%s: board=A4I1\n", __FUNCTION__);
		return board_a4i1;
	}
	if (!strncmp(tok->ptr, "a4i2", tok->len))
	{
//printf("%s: board=A4I2\n", __FUNCTION__);
		return board_a4i2;
	}
	return board_unknown;
}


// ---------------------------------------------------------------------------
// Name:		decode_cpu_load
// Purpose:		Extract the Tiva's CPU utilization.
// Inputs:		tok = ptr to struct json_token
// Outputs:		g_tiva_cpu_load.
// Returns:		0 if attribute is not found, else 1.

int decode_cpu_load(char *p, const struct json_token *tok)
{
	char search_buffer[20];
	sprintf(search_buffer, "cpu");
	tok = find_json_token(tokens, search_buffer);
	if (tok != 0)
	{
		sscanf(tok->ptr, "%d", &g_tiva_cpu_load);
		return 1;
	}
	return 0;
}


// ---------------------------------------------------------------------------
// Name:		open_fifo
// Purpose:		Opens the input data FIFO for reading.
// Inputs:		p = ptr to FIFO's name in file system.
// Outputs:		readfd is initialized if FIFO is opened.
// Returns:		File handle.

int open_fifo(char *p)
{
	int readfd;
	int status;

	umask(0); // Needed so 0666 permissions don't get modified by default umask
	status = mkfifo(p, 0666);// 0666
	if (status < 0)
	{
		if (errno == EEXIST)
		{
			// This is okay because the FIFO already existed; continue.
		}
		else
		{
			perror("mkfifo");
			return 0;
		}
	}
	readfd = open(p, O_RDONLY);
	return readfd;
}


// ---------------------------------------------------------------------------
// Name:		write_to_file
// Purpose:		Opens/truncates the output data file, writes current input, closes.
// Inputs:		p = ptr to output file name.
//				type = type of incoming data
// Outputs:		File is written.
// Returns:		1 if file was written successfully and closed, else 0.

int write_to_file(char *p, int type)
{
	int writefd;
//printf("JSON_READ: write file here to %s\n", p);

	// Unlink (destroy) file 'p' and recreate it.

	unlink(p);

	// Open and write to the file.

	writefd = open(p, O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH);
	if (type == TYPE_IO_CONTROL)
	{
#ifdef debug_response
printf("%s: write data response (binary) to file\n", __FUNCTION__);
printf("RECEIVED:\ndig_in[]: ");
		int i;
		for (i=0; i<NUM_DIG_INS; i++)
		{
			printf("%02x ", input_io_control_data.dig_in[i]);
		}
printf("\nana_in[]: ");
		for (i=0; i<NUM_ANA_IO; i++)
		{
			printf("%08x ", input_io_control_data.ana_in[i]);
		}
printf("\npulse_in[]\n");
		for (i=0; i<MAX_PULSE_PAIRS; i++)
		{
			printf("\tforward: %08x ", input_io_control_data.pulse_in[i].forward_pulse_count);
			printf("\treverse: %08x ", input_io_control_data.pulse_in[i].reverse_pulse_count);
			printf("\terrors: %08x ", input_io_control_data.pulse_in[i].error_pulse_count);
			printf("\trate: %08lx", input_io_control_data.pulse_in[i].period_a);
			printf("\trate: %08lx\n", input_io_control_data.pulse_in[i].period_b);
		}
#endif
#if 0
int i;
printf("\nana_in[]: ");
for (i=0; i<NUM_ANA_IO; i++)
{
	printf("%08x ", input_io_control_data.ana_in[i]);
}
printf("\n");
printf("board_role: %d\n", input_io_control_data.board_role);
printf("seqnum:     %d\n", input_io_control_data.seqnum);
printf("timestamp:  %d\n", input_io_control_data.timestamp);
printf("type:       %d\n", input_io_control_data.type);
printf("resync:     %d\n", input_io_control_data.resync_pulses);
#endif



		write(writefd, &input_io_control_data, sizeof(input_io_board_data_t));
	}
	else if (type == TYPE_PRINTER)
	{
printf("%s: write printer response=%s\n", __FUNCTION__, printer_response_buf);
		write(writefd, printer_response_buf, strlen(printer_response_buf));
	}
	else if (type == TYPE_SMITH_COMM)
	{
//printf("%s: write SmithComm message=%s\n", __FUNCTION__, smith_comm_buf);
		write(writefd, smith_comm_buf, strlen(smith_comm_buf));
	}
	else if (type == TYPE_SMART_INJECTOR)
	{
//printf("%s: write smart injector message=%s\n", __FUNCTION__, smart_injector_buf);
		write(writefd, smart_injector_buf, strlen(smart_injector_buf));
	}
	else if (type == TYPE_COMMAND)
	{
printf("%s: write command response=%s\n", __FUNCTION__, command_response_buf);
		write(writefd, command_response_buf, strlen(command_response_buf));
	}
	else if (type == TYPE_PROMASS)
	{
printf("%s: write command response=%s\n", __FUNCTION__, promass_response_buf);
		write(writefd, promass_response_buf, strlen(promass_response_buf));
	}
	close(writefd);
	return 1;
}


// ---------------------------------------------------------------------------
// Name:		create_pad_bufs
// Purpose:		Creates opad and ipad buffers, XORing magic values with the key.
// Inputs:		pKey = ptr to 20-byte shared secret key.
// Outputs:		i_pad and o_pad buffers are initialized.
// Returns:		Nothing.

static void create_pad_bufs(unsigned char *pKey)
{
	bzero(i_pad, (unsigned int) sizeof(i_pad));
	bzero(o_pad, (unsigned int) sizeof(o_pad));
	bcopy(pKey, i_pad, 20);
	bcopy(pKey, o_pad, 20);

	for (unsigned int i=0; i<sizeof(i_pad); i++)
	{
		i_pad[i] ^= 0x36;  // These magic values are defined in RFC 2104.
		o_pad[i] ^= 0x5C;
	}
}


// ---------------------------------------------------------------------------
// Name:		calc_rfc_2104_hmac
// Purpose:		Performs H(K XOR opad, H(K XOR ipad, data)) as specified in RFC 2104.
// Inputs:		iDataLen = length of data buffer
//				pDataBuf = pointer to input data buffer
//				pKey = ptr to shared secret key
//				pHashBuf = pointer to output buffer
// Outputs:		Hash buffer contains result of algorithm.
// Returns:		Nothing.

void calc_rfc_2104_hmac
(
	unsigned char *pKey,
	char *pDataBuf,
	int iDataLen,
	unsigned char *pHashBuf
)
{
	SHA256_CTX sha_context;

	// Create the i_pad and o_pad data used in the HMAC algorithm.

	create_pad_bufs(pKey);

	// Compute the inner hash.

	SHA256_Init(&sha_context);							// Init context for first pass
	SHA256_Update(&sha_context, i_pad, 64);				// Start with inner pad
	SHA256_Update(&sha_context, pDataBuf, iDataLen);	// Continue with data buffer
	SHA256_Final(pHashBuf, &sha_context);				// Conclude first pass

	// Compute the outer hash, place 256-bit result in 'pHashBuf'.

	SHA256_Init(&sha_context);							// Init context for second pass
	SHA256_Update(&sha_context, o_pad, 64);				// Start with outer pad
	SHA256_Update(&sha_context, pHashBuf, 32);			// Continue with results of first pass
	SHA256_Final(pHashBuf, &sha_context);				// Conclude second pass
}
