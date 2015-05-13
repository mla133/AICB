/**
 * main.cpp
 *
 *  @date: 	July 23, 2014
 *  @author: 	Mike Ryan <mike.ryan@fmcti.com>
 *
 *  @section DESCRIPTION
 *
 *  This module contains the main entry point for the JSON writer program.
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
#include <string.h>
#include <fstream>
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
#include <openssl/sha.h>	// For calculating hash
#include <sys/sysinfo.h>	// Used when getting system time
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "sqlite3.h"
#include "DBAccu4.h"
#include "db_access.h"

#include "tiva_interface.h"


#define BUF_LEN				(sizeof(struct inotify_event) + NAME_MAX + 1)
#define SHA_LENGTH_IN_BYTES	32
struct _file_list
{
	int wd;
	char name[500];
} file_list[20]; // Can watch max of 20 files simultaneously


// ---------------------------------------------------------------------------
// G L O B A L    D A T A

// This data is read from output FIFO.

output_io_board_data_t output_io_board_data;

// Increased size due to 'data' type payload, more than 1,600 bytes.
//#define SIZE_EACH_PACKET	1024	// # bytes transmitted each time to Tiva firmware
//#define SIZE_EACH_PACKET	2048	// # bytes transmitted each time to Tiva firmware
#define SIZE_EACH_PACKET	1700	// # bytes transmitted each time to Tiva firmware

#define MAX_PAYLOAD_SIZE	240		// Maximum number of ASCII characters we can send at once

//char tmp_output_buf[500+(80*66)];
char tmp_output_buf[10000];
char ascii_buf[500+(80*66)];		// MONDAY, try huge output file
char tmp_printer_buf[500+(80*66)];	// MONDAY, try huge output file

char json_out_buf[MAX_LEN_SERIAL_JSON];	// Maximum size of output JSON text.
char tx_buf[SIZE_EACH_PACKET + 4];

int g_current_arm = 0;		// Needed?

int convert_json = false;

// Shared secret key also used by Tivas to encode and decode hash.

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
static unsigned char keyed_hash[32];	// Will contain result of HMAC calculation.

// Serial port configuration ASCII strings to put into JSON values.

char comm_protocol[MAX_LEN_ENUM_STRING];
char baud_rate[MAX_LEN_ENUM_STRING];
char data_parity[MAX_LEN_ENUM_STRING];
char host_control[MAX_LEN_ENUM_STRING];
char comm_timeout[MAX_LEN_ENUM_STRING];
char serial_protocol[MAX_LEN_ENUM_STRING];
char rs485_duplex[MAX_LEN_ENUM_STRING];
char term_resistors[MAX_LEN_ENUM_STRING];

serial_port_config_t serial_ports[4];



// ---------------------------------------------------------------------------
// Function:	main
// Purpose:		Entry point for the JSON converter program.
// Inputs:		argv[0] = name of this program
//				argv[1] = "a4m", "a4b", "a4i1", or "a4i2".
//				argv[2] = "data", "printer", "serial", or "command".

int main(int argc, char **argv)
{

	void create_json(void *, int, char *, int, int, int);
	void create_hash(char *, char *, unsigned char *, unsigned char *);
	int find_printers();
	int open_fifo(char *);
	int readfifo;
	int nbytes;
	int fd;
	char buf[BUF_LEN] __attribute__ ((aligned(4)));
	ssize_t numRead;
	struct inotify_event *event;
	int handle_notify_event(struct inotify_event *, char *);
	int status;
	char fifo_name[128];
	char input_file[128];
	char dest_board[20];
	int dest_board_id;
	int data_type;
	ifstream prn_file;
 	ofstream prn_del;
 	string line;
	output_io_board_data_t *pIO_control_data;
	printer_data_t *pPrinter_data;
	serial_data_t *pSerial_data;
	serial_data_t *pSmart_injector_data;
	command_data_t *pCommand_data;
	promass_data_t *pPromass_data;
	struct timespec timespec;
	int start_time = 0;
	int loop;
	int num_packets_to_send;
	int num_bytes_to_send;
	void add_trailing_blanks(char *);
	char packet_fragment[1024];
	int serial_port;
	bool set_alarm = false;
 	int rc;
 	char query[MAX_LEN_SQL_QUERY];
 	bool two_ports = false;
 	sqlite3 *SQLite_flash_ALIV_db;
 	sqlite3 *SQLite_ram_ALIV_db;
	int read_serial_port_config(sqlite3 *, int, bool, int*);

	// Get the input arguments, used to identify the input and output file names for this instance.

	memset(dest_board, 0, sizeof(dest_board));
	strcpy(dest_board, argv[2]);
	memset(fifo_name, 0, sizeof(fifo_name));

	if(argv[3] != NULL && !strcmp(argv[3],"2"))
	{
		two_ports = true;
	}
	else
	{
		two_ports = false;
	}
	if (!strcmp(argv[1], "data"))
	{
		sprintf(fifo_name, "/var/tmp/%s/%s", dest_board, FIFO_DATA);
		convert_json = true;
		data_type = TYPE_IO_CONTROL;
	}
	else if (!strcmp(argv[1], "printer"))
	{
		if(argv[3] == NULL || atoi(argv[3]) > 2 || atoi(argv[3]) < 1)
		{
			cout << "Invalid Argument: Enter either 1 or 2 for the number of ports.\n";
			exit(-1);
		}
		sprintf(input_file, "/var/tmp/%s/printer_input_data_file%s", dest_board, argv[3]);
		sprintf(fifo_name, "/var/tmp/%s/%s%s", dest_board, FIFO_PRINTER, argv[3]);
		data_type = TYPE_PRINTER;
	}
	else if (!strcmp(argv[1], "smith_comm"))
	{
		if(argv[3] == NULL || atoi(argv[3]) > 2 || atoi(argv[3]) < 1)
		{
			cout << "Invalid Argument: Enter either 1 or 2 for the number of ports.\n";
			exit(-1);
		}
		sprintf(fifo_name, "/var/tmp/%s/%s%s", dest_board, FIFO_SMITH_COMM, argv[3]);
		data_type = TYPE_SMITH_COMM;
	}
	else if (!strcmp(argv[1], "smart_injector"))
	{
		if(argv[3] == NULL || atoi(argv[3]) > 2 || atoi(argv[3]) < 1)
		{
			cout << "Invalid Argument: Enter either 1 or 2 for the number of ports.\n";
			exit(-1);
		}
		sprintf(fifo_name, "/var/tmp/%s/%s%s", dest_board, FIFO_SMART_INJECTOR, argv[3]);
		data_type = TYPE_SMART_INJECTOR;
	}
	else if (!strcmp(argv[1], "promass"))
	{
		sprintf(fifo_name, "/var/tmp/%s/%s", dest_board, FIFO_PROMASS);
		data_type = TYPE_PROMASS;
	}
	else if (!strcmp(argv[1], "command"))
	{
		sprintf(fifo_name, "/var/tmp/%s/%s", dest_board, FIFO_COMMAND);
		data_type = TYPE_COMMAND;
	}
	else
	{
		printf("%s: Error, unknown arg '%s'\n", __FUNCTION__, argv[1]);
		exit(-1);
	}

	// Read the serial port configuration where necessary.

	if(data_type == TYPE_PRINTER)
	{
		//Open the Ram Database
		rc = sqlite3_open_v2("/dev/shm/accu4_db_ram.sqlite", &SQLite_ram_ALIV_db, SQLITE_OPEN_READWRITE|SQLITE_OPEN_FULLMUTEX, NULL);
		if (rc != SQLITE_OK)
		{
			syslog(LOG_ALERT, "json_write: %s on line:%d in file:%s\n", "Error opening accu4 RAM database", __LINE__, __FILE__);
			sqlite3_exec(SQLite_ram_ALIV_db, "ROLLBACK", NULL, NULL, NULL);
			sqlite3_close(SQLite_ram_ALIV_db);
			printf("json_write: error opening RAM database--exiting, error=%d\n", rc);
			return rc;
		}
		sqlite3_busy_timeout(SQLite_ram_ALIV_db, 5000/*ms*/);   // wait 5 second for existing locks to be freed
	}
	if(data_type != TYPE_IO_CONTROL && data_type != TYPE_COMMAND)
	{
		rc = sqlite3_open_v2("/var/lib/accu4/accu4_db.sqlite", &SQLite_flash_ALIV_db, SQLITE_OPEN_READWRITE|SQLITE_OPEN_NOMUTEX, NULL);
		if (rc != SQLITE_OK)
		{
			syslog(LOG_ALERT, "json_write: %s on line:%d in file:%s\n", "Error opening flash database", __LINE__, __FILE__);
			sqlite3_exec(SQLite_flash_ALIV_db, "ROLLBACK", NULL, NULL, NULL);
			sqlite3_close(SQLite_flash_ALIV_db);
			return rc;
		}
		sqlite3_busy_timeout(SQLite_flash_ALIV_db, 5000);

		do
		{
			read_serial_port_config(SQLite_flash_ALIV_db, data_type, two_ports, &serial_port);
			sleep(1);

		}while(serial_port < 0);
	}

	// Create an inotify instance for this process. Then add a watch for the given file.

    fd = inotify_init();
    file_list[0].wd = inotify_add_watch(fd, fifo_name, IN_CLOSE_WRITE);
	if (file_list[0].wd == -1)
	{
		printf("%s: error getting write watch descriptor for file %s\n", __FUNCTION__, fifo_name);
		exit (-1);
	}

	sprintf(&file_list[0].name[0], "%s", fifo_name);

	// Set the "dest_board_id" variable, because we keep track of sequence numbers separately to each board.

	if (!strcmp(argv[2], "a4m"))
		dest_board_id = board_a4m;
	else if (!strcmp(argv[2], "a4b"))
		dest_board_id = board_a4b;
	else if (!strcmp(argv[2], "a4i1"))
		dest_board_id = board_a4i1;
	else if (!strcmp(argv[2], "a4i2"))
		dest_board_id = board_a4i2;
	else
		dest_board_id = board_unknown;

	// Open the read FIFO.

	readfifo = open_fifo(fifo_name);
	if (readfifo == 0)
	{
		printf("%s: error opening or creating FIFO '%s'\n", __FUNCTION__, fifo_name);
		exit(-1);
	}

	// (Blocking) wait until FIFO is modified, then read it and print the corresponding JSON to stdout.

	while (1)
	{
		numRead = read(fd, buf, (unsigned int) BUF_LEN);
        if (numRead <= 0)
        {
        	printf("%s: read() from inotify read returned error, exit\n", __FUNCTION__);
        	exit(-1);
        }
       	event = (struct inotify_event *) buf;
       	status = handle_notify_event(event, fifo_name);
       	if (status == 0)
       	{
       		// Allocate a buffer large enough to contain the input data.
       		// Then read the entire file's contents into memory.

       		switch (data_type)
       		{
       		case TYPE_IO_CONTROL:
       			pIO_control_data = (output_io_board_data_t *) malloc(sizeof(output_io_board_data_t));
           		nbytes = read(readfifo, pIO_control_data, sizeof(output_io_board_data_t));
           		memset(tx_buf,0,sizeof(tx_buf));
       			break;
       		case TYPE_PRINTER:
       			pPrinter_data = (printer_data_t *) malloc(sizeof(printer_data_t));
       			memset(pPrinter_data, 0, sizeof(printer_data_t));

       			//Allow Time for the BeagleBone to Write to the FIFO
       			usleep(500000);

           		nbytes = read(readfifo, pPrinter_data, sizeof(printer_data_t));
           		syslog(LOG_ALERT, "json_write: %s on line:%d in file:%s\n", "Starting Read", __LINE__, __FILE__);
       			break;
       		case TYPE_SMITH_COMM:
       			pSerial_data = (serial_data_t *) malloc(sizeof(serial_data_t));
       			memset(pSerial_data, 0, sizeof(serial_data_t));
           		nbytes = read(readfifo, pSerial_data, sizeof(serial_data_t));
//printf("got '%s'\n", pSerial_data->text);
           		break;
       		case TYPE_SMART_INJECTOR:
       			pSmart_injector_data = (serial_data_t *) malloc(sizeof(serial_data_t));
       			memset(pSmart_injector_data, 0, sizeof(serial_data_t));
           		nbytes = read(readfifo, pSmart_injector_data, sizeof(smart_injector_data_t));
       			break;
       		case TYPE_PROMASS:
       			pPromass_data = (promass_data_t *) malloc(sizeof(promass_data_t));
       			memset(pPromass_data, 0, sizeof(promass_data_t));
           		nbytes = read(readfifo, pPromass_data, sizeof(promass_data_t));
       			break;
       		case TYPE_COMMAND:
       			pCommand_data = (command_data_t *) malloc(sizeof(command_data_t));
       			memset(pCommand_data, 0, sizeof(command_data_t));
           		nbytes = read(readfifo, pCommand_data, sizeof(command_data_t));
       			break;
       		}
			if (nbytes == 0)
			{
				// If we get EOF from the FIFO, the peer has exited so re-open the FIFO.
				syslog(LOG_ALERT, "json_write: %s on line:%d in file:%s\n", "FIFO Empty", __LINE__, __FILE__);
	       		switch (data_type)
	       		{
	       		case TYPE_IO_CONTROL:
	       			free(pIO_control_data);
	       			break;
	       		case TYPE_PRINTER:
	       			free(pPrinter_data);
	       			break;
	       		case TYPE_SMITH_COMM:
	       			free(pSerial_data);
	       			break;
	       		case TYPE_SMART_INJECTOR:
	       			free(pSmart_injector_data);
	       			break;
	       		case TYPE_PROMASS:
	       			free(pPromass_data);
	       			break;
	       		case TYPE_COMMAND:
	       			free(pCommand_data);
	       			break;
	       		}
				close(readfifo);
				readfifo = open(fifo_name, O_RDONLY);
				continue;
			}
			memset(ascii_buf, 0, sizeof(ascii_buf));
			memset(json_out_buf, 0, sizeof(json_out_buf));

			// Create outgoing JSON text.  For printer data, the total amount of data to send might
			// exceed the number of bytes we can send in one packet (240 characters; this affects the
			// number of symbols interpreted by the JSON parser on the Tiva in file frozen.c) so we
			// may have to split the text into multiple packets.

			switch (data_type)
			{
			case TYPE_IO_CONTROL:
				create_json(pIO_control_data, (int) sizeof(ascii_buf), ascii_buf, data_type, dest_board_id, serial_port);
				break;
			case TYPE_PRINTER:

				if (strlen(pPrinter_data->text) >= MAX_PAYLOAD_SIZE)
				{
					num_bytes_to_send = nbytes;
					num_packets_to_send = nbytes / MAX_PAYLOAD_SIZE;
					if (num_packets_to_send == 0)
					{
						num_packets_to_send = 1;
					}
					else
					{
						if (nbytes % MAX_PAYLOAD_SIZE)
						{
							num_packets_to_send++;
						}
					}
					for (loop=0; loop<num_packets_to_send-1; loop++)
					{
						memset(packet_fragment, 0, sizeof(packet_fragment));
						strncpy(packet_fragment, (char *)pPrinter_data+(loop*MAX_PAYLOAD_SIZE), MAX_PAYLOAD_SIZE);
						create_json(packet_fragment, MAX_PAYLOAD_SIZE, ascii_buf, data_type, dest_board_id, serial_port);
						// Mike: on 14Nov, hash only DATA, not including other JSON elements
						create_hash(ascii_buf, json_out_buf, key, keyed_hash);
						add_trailing_blanks(json_out_buf);

						memset(tx_buf, 0, sizeof(tx_buf));
						sprintf(tx_buf, "%04d", strlen(json_out_buf));
						strcat(tx_buf, json_out_buf);
						fprintf(stderr, "\n\n\nJSON_WRITE: debug-outgoing JSON='%s'\n\n\n", tx_buf);

						printf("%s", tx_buf);
						fflush(stdout);
						memset(ascii_buf, 0, sizeof(ascii_buf));
						memset(json_out_buf, 0, sizeof(json_out_buf));
						num_bytes_to_send -= MAX_PAYLOAD_SIZE;

						clock_gettime(CLOCK_MONOTONIC, &timespec);
						start_time = timespec.tv_sec + timespec.tv_nsec/1000000000.0;

						//Wait for the Printer's Response

						while(1)
						{
							//Check Timer in Case of Tiva/Ethernet Failure
							clock_gettime(CLOCK_MONOTONIC, &timespec);
							//syslog(LOG_ALERT, "time: %f on line:%d in file:%s\n", ((timespec.tv_sec + timespec.tv_nsec/1000000000.0) - start_time), __LINE__, __FILE__);
							//cout << (timespec.tv_sec + timespec.tv_nsec/1000000000.0) - start_time << endl;
							if(((timespec.tv_sec + timespec.tv_nsec/1000000000.0) - start_time) > (1000))
							{
								set_alarm = true;
								break;
							}

							usleep(10000);

							//Open the Printer File
							prn_file.open(input_file);

							getline(prn_file, line);
							prn_file.close();

							if(strcmp(line.c_str(), "FINISHED_NO_ERROR") == 0)
								break;

							else if(strcmp(line.c_str(), "PRINT_FINISHED_XOFF_TIMEOUT") == 0)
							{
								fprintf(stderr, "\n\n\nPRINT_FINISHED_XOFF_TIMEOUT\n\n\n");
								set_alarm = true;
								break;
							}
						}//while

						fprintf(stderr, "\n\n\nOutput Buffer:\t%s\n\n\n", line.c_str());
						line.clear();
						remove(input_file);

						if(set_alarm == true)
						{
							fprintf(stderr, "\n\n\nMAJOR PTB ERROR\n\n\n");
							fflush(stderr);
							num_bytes_to_send = 0;
							break;
						}
					}//for

					if (num_bytes_to_send > 0 && set_alarm == false)
					{
						strncpy(packet_fragment, (char *)pPrinter_data+(loop*MAX_PAYLOAD_SIZE), MAX_PAYLOAD_SIZE);
						create_json(packet_fragment, num_bytes_to_send, ascii_buf, data_type, dest_board_id, serial_port);
						create_hash(ascii_buf, json_out_buf, key, keyed_hash);
						add_trailing_blanks(json_out_buf);

						memset(tx_buf, 0, sizeof(tx_buf));
						sprintf(tx_buf, "%04d", strlen(json_out_buf));
						strcat(tx_buf, json_out_buf);
						fprintf(stderr, "\n\n\nJSON_WRITE: debug-outgoing JSON='%s'\n\n\n", tx_buf);

						printf("%s", tx_buf);
						fflush(stdout);

						clock_gettime(CLOCK_MONOTONIC, &timespec);
						start_time = timespec.tv_sec + timespec.tv_nsec/1000000000.0;

						//Wait for the Printer's Response

						while(1)
						{
							//Check Timer in Case of Tiva/Ethernet Failure
							clock_gettime(CLOCK_MONOTONIC, &timespec);
							if(((timespec.tv_sec + timespec.tv_nsec/1000000000.0) - start_time) > (1000))
							{
								set_alarm = true;
								break;
							}

							usleep(10000);

							//Open the Printer File
							prn_file.open(input_file);

							getline(prn_file, line);
							prn_file.close();

							if(strcmp(line.c_str(), "FINISHED_NO_ERROR") == 0)
								break;

							else if(strcmp(line.c_str(), "PRINT_FINISHED_XOFF_TIMEOUT") == 0)
							{
								fprintf(stderr, "\n\n\nPRINT_FINISHED_XOFF_TIMEOUT\n\n\n");
								set_alarm = true;
								prn_file.close();
								break;
							}
						}//while

						fprintf(stderr, "\n\n\nOutput Buffer:\t%s\n\n\n", line.c_str());
						line.clear();
						remove(input_file);
					}//if
				}//if

				//Only One Packet is Needed
				else
				{
					create_json(pPrinter_data->text, strlen(pPrinter_data->text), ascii_buf, data_type, dest_board_id, serial_port);
					create_hash(ascii_buf, json_out_buf, key, keyed_hash);
					add_trailing_blanks(json_out_buf);

					memset(tx_buf, 0, sizeof(tx_buf));
					sprintf(tx_buf, "%04d", strlen(json_out_buf));
					strcat(tx_buf, json_out_buf);

					printf("%s", tx_buf);
					fflush(stdout);

					clock_gettime(CLOCK_MONOTONIC, &timespec);
					start_time = timespec.tv_sec + timespec.tv_nsec/1000000000.0;

					//Wait for the Printer's Response

					while(1)
					{
						//Check Timer in Case of Tiva/Ethernet Failure
						clock_gettime(CLOCK_MONOTONIC, &timespec);
						if(((timespec.tv_sec + timespec.tv_nsec/1000000000.0) - start_time) > (1000))
						{
							set_alarm = true;
							break;
						}

						usleep(10000);

						//Open the Printer File
						prn_file.open(input_file);

						getline(prn_file, line);
						prn_file.close();

						if(strcmp(line.c_str(), "FINISHED_NO_ERROR") == 0)
							break;

						else if(strcmp(line.c_str(), "PRINT_FINISHED_XOFF_TIMEOUT") == 0)
						{
							fprintf(stderr, "\n\n\nPRINT_FINISHED_XOFF_TIMEOUT\n\n\n");
							set_alarm = true;
							prn_file.close();
							break;
						}
					}
				}//else

				prn_file.close();

				if(set_alarm == false)
				{
					//Finished Printing, Notify the Printer Process
					fprintf(stderr,"\n\n\n\n\n\n------------------------Print Successful------------------------\n\n\n\n\n\n");
					snprintf(query, sizeof(query), "INSERT INTO print_queue (print_cmd, arg1) values ('Tiva', 0)");
					rc = sqlite3_exec(SQLite_ram_ALIV_db, query, NULL,NULL,NULL);
					if (rc != SQLITE_OK)
					{
						//fprintf(stderr,"Query: %s\n", query);
						//fprintf(stderr,"Failed\n");
						syslog(LOG_ALERT, "json_write: %s on line:%d in file:%s\n", sqlite3_errmsg(SQLite_ram_ALIV_db), __LINE__, __FILE__);

						sqlite3_exec(SQLite_ram_ALIV_db, "ROLLBACK", NULL, NULL, NULL);
						sqlite3_close(SQLite_ram_ALIV_db);
					}
					syslog(LOG_ALERT, "json_write: %s on line:%d in file:%s\n", "Writing to Tiva(Success)", __LINE__, __FILE__);
				}
				else
				{
					syslog(LOG_ALERT, "json_write: %s on line:%d in file:%s\n", "Writing to Tiva(Fail)", __LINE__, __FILE__);
					snprintf(query, sizeof(query), "INSERT INTO print_queue (print_cmd, arg1) values ('Tiva', 1)");
					rc = sqlite3_exec(SQLite_ram_ALIV_db, query, NULL,NULL,NULL);
					if (rc != SQLITE_OK)
					{
						syslog(LOG_ALERT, "json_write: %s on line:%d in file:%s\n", "Error Writing to ram database", __LINE__, __FILE__);
						sqlite3_exec(SQLite_ram_ALIV_db, "ROLLBACK", NULL, NULL, NULL);
						sqlite3_close(SQLite_ram_ALIV_db);
					}
					set_alarm = false;
				}

				break;
			case TYPE_SMITH_COMM:
				create_json(pSerial_data, (int) sizeof(ascii_buf), ascii_buf, data_type, dest_board_id, serial_port);
//create_hash((char *) pSerial_data, json_out_buf, key, keyed_hash);

				break;

			case TYPE_SMART_INJECTOR:

				create_json(pSmart_injector_data, (int) sizeof(ascii_buf), ascii_buf, data_type, dest_board_id, serial_port);
//create_hash((char *) pSmart_injector_data, json_out_buf, key, keyed_hash);
				break;

			case TYPE_PROMASS:
				create_json(pPromass_data, (int) sizeof(ascii_buf), ascii_buf, data_type, dest_board_id, serial_port);
//create_hash((char *) pPromass_data, json_out_buf, key, keyed_hash);
				break;

			case TYPE_COMMAND:
				create_json(pCommand_data, (int) sizeof(ascii_buf), ascii_buf, data_type, dest_board_id, serial_port);
				//create_hash((char *) pCommand_data, json_out_buf, key, keyed_hash);
				break;
			}
			create_hash(ascii_buf, json_out_buf, key, keyed_hash);

			// Print the jsonized text to stdout, which will be read on socat's stdin.

			if(data_type != TYPE_PRINTER)
			{
				add_trailing_blanks(json_out_buf);
//				printf("%s", json_out_buf);
				sprintf(tx_buf, "%04d", strlen(json_out_buf));
				strcat(tx_buf, json_out_buf);
//fprintf(stderr, "\n\n\nJSON_WRITE: debug-outgoing JSON='%s'\n\n\n", tx_buf);
//fflush(stderr);

				printf("%s", tx_buf);

				fflush(stdout);
			}

			// Free the dynamically allocated memory.

       		switch (data_type)
       		{
       		case TYPE_IO_CONTROL:
       			free(pIO_control_data);
       			break;
       		case TYPE_PRINTER:
       			free(pPrinter_data);
       			break;
       		case TYPE_SMITH_COMM:
       			free(pSerial_data);
       			break;
       		case TYPE_SMART_INJECTOR:
       			free(pSmart_injector_data);
       			break;
       		case TYPE_PROMASS:
       			free(pPromass_data);
       			break;
       		case TYPE_COMMAND:
       			free(pCommand_data);
       			break;
       		}
       	}
	}
	close(readfifo);
}


// ---------------------------------------------------------------------------
// Name:		add_trailing_blanks
// Purpose:		Opens the input data FIFO for reading.
// Inputs:		p = ptr to name of FIFO
// Outputs:		readfd is initialized if FIFO is opened.
// Returns:		File handle.

void add_trailing_blanks(char *p)
{
	int i;
if (1==1) return; // Mike 5Jan15, don't do this
	if (strlen(p) >= SIZE_EACH_PACKET) return;
	for (i=strlen(p); i<SIZE_EACH_PACKET; i++)
	{
		*(p+i) = ' ';
	}
}


// ---------------------------------------------------------------------------
// Name:		open_fifo
// Purpose:		Opens the input data FIFO for reading.
// Inputs:		p = ptr to name of FIFO
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
// Name:		handle_notify_event
// Purpose:		Checks input event mask, checks for write to FIFO.
// Inputs:		Ptr to notify event struct described in sys/inotify.h.
//				Ptr to file name.
// Outputs:		None.
// Returns:		1 if FIFO was written, else 0.


int handle_notify_event
(
	struct inotify_event *pEvent,
	char *fname
)
{
	if (pEvent->len > 0)
    {
    }
	if (pEvent->wd != file_list[0].wd)
	{
		return 0;
	}
    if (pEvent->mask & IN_MODIFY)
    {
    	return 1;
    }
    return 0;
}


// ---------------------------------------------------------------------------
// Name:		create_json
// Purpose:		Given outgoing data, creates JSON-formatted text.
// Inputs:		inbuf = ptr to outgoing data
//				len_data = size of this packet chunk to send (for printer)
//				packet_type = TYPE_IO_CONTROL, etc.
//				board_id = ID of destination board (enum offset)
//				serial_port = 1-4 (used if packet_type is PRINTER or SERIAL)
// Outputs:		outbuf = ptr to buffer for output ASCII data

void create_json
(
	void *inbuf,
	int len_data,
	char *outbuf,
	int packet_type,
	int board_id,
	int serial_port
)
{
	static int seqnum[5]={0};
    struct timespec monotomic_time;
    struct sysinfo info;
	output_io_board_data_t *pIO_control_data = (output_io_board_data_t *) inbuf;
	printer_data_t *pPrinter_data = (printer_data_t *) inbuf;
	serial_data_t *pSerial_data = (serial_data_t *) inbuf;
	command_data_t *pCommand_data = (command_data_t *) inbuf;
	promass_data_t *pPromass_data = (promass_data_t *)inbuf;

	int i;
	char meter_num[3];

	//serial_port is Now Zero Based
	serial_port++;

    // Get the current seconds lapsed since the Linux Epoch (1/1/1970).

    sysinfo(&info);
    clock_gettime(CLOCK_MONOTONIC, &monotomic_time);

	// Add elements that all payloads must have: (1) timestamp, (2) sequence number, (3) board role, (4) data type.

	memset(tmp_output_buf, 0, sizeof(tmp_output_buf));


	sprintf(tmp_output_buf, "{\"ts\":%u,", (unsigned int) monotomic_time.tv_sec);
	strcat(outbuf, tmp_output_buf);
	seqnum[board_id]++;
	sprintf(tmp_output_buf, "\"seq\":%u,", seqnum[board_id]);
	strcat(outbuf, tmp_output_buf);

	sprintf(tmp_output_buf, "\"type\":");
	if (packet_type == TYPE_IO_CONTROL)
	{
		strcat(tmp_output_buf, "\"d\",");
	}
	else if (packet_type == TYPE_PRINTER)
	{
		strcat(tmp_output_buf, "\"p\",");
	}
	else if (packet_type == TYPE_SMITH_COMM)
	{
		strcat(tmp_output_buf, "\"s\",");
	}
	else if (packet_type == TYPE_SMART_INJECTOR)
	{
		strcat(tmp_output_buf, "\"si\",");
	}
	else if (packet_type == TYPE_PROMASS)
	{
		strcat(tmp_output_buf, "\"pm\",");
	}
	else if (packet_type == TYPE_COMMAND)
	{
		strcat(tmp_output_buf, "\"cmd\",");
	}
	else
		strcat(tmp_output_buf, "\"unknown\",");
	strcat(outbuf, tmp_output_buf);

	strcat(outbuf, "\"id\":");
	strcat(outbuf, "\"beaglebone\",");

	// Add the digital output values for io_control task on Tiva.

	if (packet_type == TYPE_IO_CONTROL)
	{
		// dig_out

		sprintf(tmp_output_buf, "\"do\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d], ",
				pIO_control_data->dig_out[0],
				pIO_control_data->dig_out[1],
				pIO_control_data->dig_out[2],
				pIO_control_data->dig_out[3],
				pIO_control_data->dig_out[4],
				pIO_control_data->dig_out[5],
				pIO_control_data->dig_out[6],
				pIO_control_data->dig_out[7],
				pIO_control_data->dig_out[8],
				pIO_control_data->dig_out[9],
				pIO_control_data->dig_out[10],
				pIO_control_data->dig_out[11],
				pIO_control_data->dig_out[12],
				pIO_control_data->dig_out[13],
				pIO_control_data->dig_out[14],
				pIO_control_data->dig_out[15],
				pIO_control_data->dig_out[16],
				pIO_control_data->dig_out[17],
				pIO_control_data->dig_out[18],
				pIO_control_data->dig_out[19],
				pIO_control_data->dig_out[20],
				pIO_control_data->dig_out[21],
				pIO_control_data->dig_out[22],
				pIO_control_data->dig_out[23]);
		strcat(outbuf, tmp_output_buf);

		// ana_out

		sprintf(tmp_output_buf, "\"ao\":[%d,%d,%d,%d,%d,%d],",
				pIO_control_data->ana_out[0],
				pIO_control_data->ana_out[1],
				pIO_control_data->ana_out[2],
				pIO_control_data->ana_out[3],
				pIO_control_data->ana_out[4],
				pIO_control_data->ana_out[5]);
		strcat(outbuf, tmp_output_buf);

		// ana_io_map:  per module, send 0 to mean 'input', 1 to mean 'output'

		sprintf(tmp_output_buf, "\"aim\":[%d,%d,%d,%d,%d,%d],",
				 pIO_control_data->ana_io_map & 1,
				(pIO_control_data->ana_io_map >> 1) & 1,
				(pIO_control_data->ana_io_map >> 2) & 1,
				(pIO_control_data->ana_io_map >> 3) & 1,
				(pIO_control_data->ana_io_map >> 4) & 1,
				(pIO_control_data->ana_io_map >> 5) & 1);
		strcat(outbuf, tmp_output_buf);

		// 	ana_calibrate

		sprintf(tmp_output_buf, "\"ac\":%d,", pIO_control_data->ana_calibrate);
		strcat(outbuf, tmp_output_buf);

		//	bistate_map
		sprintf(tmp_output_buf, "\"bi\":[%d,%d,%d,%d,%d,%d,%d,%d],",
				 pIO_control_data->bistate_map & 1,
				(pIO_control_data->bistate_map >> 1) & 1,
				(pIO_control_data->bistate_map >> 2) & 1,
				(pIO_control_data->bistate_map >> 3) & 1,
				(pIO_control_data->bistate_map >> 4) & 1,
				(pIO_control_data->bistate_map >> 5) & 1,
				(pIO_control_data->bistate_map >> 6) & 1,
				(pIO_control_data->bistate_map >> 7) & 1);
		strcat(outbuf, tmp_output_buf);

#ifdef dont_think_we_will_need
		// 	pulse_multiplier
		sprintf(tmp_output_buf, "\"pm\":[%d,%d,%d,%d,%d,%d,%d,%d],",
				pIO_control_data->pulse_multiplier[0],
				pIO_control_data->pulse_multiplier[1],
				pIO_control_data->pulse_multiplier[2],
				pIO_control_data->pulse_multiplier[3],
				pIO_control_data->pulse_multiplier[4],
				pIO_control_data->pulse_multiplier[5],
				pIO_control_data->pulse_multiplier[6],
				pIO_control_data->pulse_multiplier[7]);
		strcat(outbuf, tmp_output_buf);
#endif

		// 	pulse_period_samples

		sprintf(tmp_output_buf, "\"pps\":[%d,%d,%d,%d,%d,%d,%d,%d],",
				pIO_control_data->pulse_period_samples[0],
				pIO_control_data->pulse_period_samples[1],
				pIO_control_data->pulse_period_samples[2],
				pIO_control_data->pulse_period_samples[3],
				pIO_control_data->pulse_period_samples[4],
				pIO_control_data->pulse_period_samples[5],
				pIO_control_data->pulse_period_samples[6],
				pIO_control_data->pulse_period_samples[7]);
		strcat(outbuf, tmp_output_buf);

		// 	pulse_input_mode

		sprintf(tmp_output_buf, "\"pim\":[%d,%d,%d,%d],",
				 pIO_control_data->pulse_input_mode & 1,
				(pIO_control_data->pulse_input_mode >> 1) & 1,
				(pIO_control_data->pulse_input_mode >> 2) & 1,
				(pIO_control_data->pulse_input_mode >> 3) & 1);
		strcat(outbuf, tmp_output_buf);

		// 	pulse_input_function

		sprintf(tmp_output_buf, "\"pif\":[%d,%d,%d,%d,%d,%d,%d,%d],",
				pIO_control_data->pulse_input_function[0],
				pIO_control_data->pulse_input_function[1],
				pIO_control_data->pulse_input_function[2],
				pIO_control_data->pulse_input_function[3],
				pIO_control_data->pulse_input_function[4],
				pIO_control_data->pulse_input_function[5],
				pIO_control_data->pulse_input_function[6],
				pIO_control_data->pulse_input_function[7]);
		strcat(outbuf, tmp_output_buf);

		// 	prover_out_select

		sprintf(tmp_output_buf, "\"pro\":%d,", pIO_control_data->prover_out_select);
		strcat(outbuf, tmp_output_buf);

		//	pulse_output

		sprintf(tmp_output_buf, "\"po\":[");
		strcat(outbuf, tmp_output_buf);
		for (i=0; i<MAX_PULSE_OUTS; i++)
		{
			sprintf(tmp_output_buf, "{\"pc\":%d,", pIO_control_data->pulse_output[i].output_pulses);
			strcat(outbuf, tmp_output_buf);
			if(i < (MAX_PULSE_OUTS - 1))
				sprintf(tmp_output_buf, "\"pf\":%d},", pIO_control_data->pulse_output[i].output_freq);
			else
				sprintf(tmp_output_buf, "\"pf\":%d}],", pIO_control_data->pulse_output[i].output_freq);
			strcat(outbuf, tmp_output_buf);
		}

		// 	flow_control

		sprintf(tmp_output_buf, "\"fc\":[");
		strcat(outbuf, tmp_output_buf);
		for (i=0; i<MAX_PULSE_INPUTS; i++)
		{
			sprintf(tmp_output_buf, "{\"min\":%ld,", pIO_control_data->flow_control[i].min_period);
			strcat(outbuf, tmp_output_buf);
			sprintf(tmp_output_buf, "\"max\":%ld,", pIO_control_data->flow_control[i].max_period);
			strcat(outbuf, tmp_output_buf);
			sprintf(tmp_output_buf, "\"up\":%d,", pIO_control_data->flow_control[i].up_sol_io_pt);
			strcat(outbuf, tmp_output_buf);
			sprintf(tmp_output_buf, "\"down\":%d,", pIO_control_data->flow_control[i].down_sol_io_pt);
			strcat(outbuf, tmp_output_buf);
			sprintf(tmp_output_buf, "\"sc\":%ld,", pIO_control_data->flow_control[i].shutdown_counts);
			strcat(outbuf, tmp_output_buf);
			if (i < (MAX_PULSE_INPUTS-1))
				sprintf(tmp_output_buf, "\"ec\":%d},", pIO_control_data->flow_control[i].enable_flow_control);
			else
				sprintf(tmp_output_buf, "\"ec\":%d}]", pIO_control_data->flow_control[i].enable_flow_control);
			strcat(outbuf, tmp_output_buf);
		}
	}
	else if (packet_type == TYPE_PRINTER)
	{
		sprintf(tmp_output_buf, "\"p\":%u,", serial_port);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"prot\":\"%s\",", comm_protocol);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"b\":\"%s\",", baud_rate);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"par\":\"%s\",", data_parity);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"ctl\":\"%s\",", host_control);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"tmo\":\"%s\",", comm_timeout);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"sprot\":\"%s\",", serial_protocol);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"dup\":\"%s\",", rs485_duplex);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"term\":\"%s\",", term_resistors);
		strcat(outbuf, tmp_output_buf);

		memset(tmp_printer_buf, 0, sizeof(tmp_printer_buf));
		strcat(tmp_printer_buf, pPrinter_data->text);

		if (tmp_printer_buf[len_data-1] <= 13)
		{
			tmp_printer_buf[len_data-1] = 0;  // Make sure last byte is not an end-of-line
		}
		sprintf(tmp_output_buf, "\"text\":\"%s\" ", tmp_printer_buf);
		strcat(outbuf, tmp_output_buf);
	}
	else if (packet_type == TYPE_SMITH_COMM)
	{
		sprintf(tmp_output_buf, "\"p\":%u,", serial_port);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"prot\":\"%s\",", comm_protocol);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"b\":\"%s\",", baud_rate);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"par\":\"%s\",", data_parity);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"ctl\":\"%s\",", host_control);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"tmo\":\"%s\",", comm_timeout);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"sprot\":\"%s\",", serial_protocol);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"dup\":\"%s\",", rs485_duplex);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"term\":\"%s\",", term_resistors);
		strcat(outbuf, tmp_output_buf);

		int len = strlen(pSerial_data->text);
		if (pSerial_data->text[len-1] <= 13)
		{
			pSerial_data->text[len-1] = 0;  // Make sure last byte is not an end-of-line
		}
		sprintf(tmp_output_buf, "\"text\":\"%s\" ", pSerial_data->text);
		strcat(outbuf, tmp_output_buf);
	}
	else if (packet_type == TYPE_SMART_INJECTOR)
	{
		sprintf(tmp_output_buf, "\"p\":%u,", serial_port);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"prot\":\"%s\",", comm_protocol);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"b\":\"%s\",", baud_rate);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"par\":\"%s\",", data_parity);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"ctl\":\"%s\",", host_control);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"tmo\":\"%s\",", comm_timeout);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"sprot\":\"%s\",", serial_protocol);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"dup\":\"%s\",", rs485_duplex);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"term\":\"%s\",", term_resistors);
		strcat(outbuf, tmp_output_buf);

		int len = strlen(pSerial_data->text);
		if (pSerial_data->text[len-1] <= 13)
		{
			pSerial_data->text[len-1] = 0;  // Make sure last byte is not an end-of-line
		}
		sprintf(tmp_output_buf, "\"text\":\"%s\" ", pSerial_data->text);
		fprintf(stderr, "\E[31;40mXmit'd[%s]\E[0m\n", pSerial_data->text);
		strcat(outbuf, tmp_output_buf);
	}
	else if (packet_type == TYPE_PROMASS)
	{
		memcpy(meter_num, pPromass_data->text, 2);
		meter_num[2] = 0;

		sprintf(tmp_output_buf, "\"p\":%u,", serial_port);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"prot\":\"%s\",", comm_protocol);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"b\":\"%s\",", baud_rate);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"par\":\"%s\",", data_parity);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"ctl\":\"%s\",", host_control);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"tmo\":\"%s\",", comm_timeout);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"sprot\":\"%s\",", serial_protocol);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"dup\":\"%s\",", rs485_duplex);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"term\":\"%s\",", term_resistors);
		strcat(outbuf, tmp_output_buf);

		sprintf(tmp_output_buf, "\"add\":\"%s\",", meter_num);
		strcat(outbuf, tmp_output_buf);

		int len = strlen(pPromass_data->text);
		if (pPromass_data->text[len-1] <= 13)
		{
			pPromass_data->text[len-1] = 0;  // Make sure last byte is not an end-of-line
		}
		sprintf(tmp_output_buf, "\"text\":\"%s\" ", pPromass_data->text + 2);
		strcat(outbuf, tmp_output_buf);
	}
	else if (packet_type == TYPE_COMMAND)
	{
		int len = strlen(pCommand_data->text);
		if (pCommand_data->text[len-1] <= 13)
		{
			pCommand_data->text[len-1] = 0;  // Make sure last byte is not an end-of-line
		}
		sprintf(tmp_output_buf, "\"command\":\"%s\" ", pCommand_data->text);
		strcat(outbuf, tmp_output_buf);
	}
#if 0
	else
	{
		strcat(outbuf, inbuf->text);
	}
#endif
	strcat(outbuf, "}");
}


// ---------------------------------------------------------------------------
// Name:	create_pad_bufs
// Purpose:	Creates opad and ipad buffers, XORing magic values with the key.
// Inputs:	pKey = ptr to 20-byte shared secret key.
// Outputs:	i_pad and o_pad buffers are initialized.
// Returns:	Nothing.

static void create_pad_bufs(unsigned char *pKey)
{
	bzero(i_pad, sizeof(i_pad));
	bzero(o_pad, sizeof(o_pad));
	bcopy(pKey, i_pad, 20);
	bcopy(pKey, o_pad, 20);

	for (unsigned int i=0; i<sizeof(i_pad); i++)
	{
		i_pad[i] ^= 0x36;  // These magic values are defined in RFC 2104.
		o_pad[i] ^= 0x5C;
	}
}


// ---------------------------------------------------------------------------
// Name:	calc_rfc_2104_hmac
// Purpose:	Performs H(K XOR opad, H(K XOR ipad, data)) as specified in RFC 2104.
// Inputs:	iDataLen = length of data buffer
//			pDataBuf = pointer to data buffer
//			pKey = ptr to shared secret key
//			pHashBuf = pointer to output buffer
// Outputs:	Hash buffer contains result of algorithm.
// Returns:	Nothing.

void calc_rfc_2104_hmac
(
	unsigned char *pKey,
	char *pDataBuf,
	int iDataLen,
	unsigned char *pHashBuf
)
{
	SHA256_CTX sha_context;
//printf("\n****hashbuf='%s'\n\n", pDataBuf);
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

// ---------------------------------------------------------------------------
// Name:		create_hash
// Purpose:		Given an ASCII buffer and a key, creates a keyed hash using
//				SHA-256 algorithm.  Then inserts into JSON text.
// Inputs:		inbuf = ptr to ASCII buffer
//				key = ptr to key
// Outputs:		outbuf = ptr to buffer for output ASCII data

void create_hash(char *inbuf, char *outbuf, unsigned char *key, unsigned char *keyedHash)
{
	int i;
	char ascii_hash[20];

//printf("\ncreate_hash: buf=>>'%s'<<\n", inbuf);


	// Hash ASCII "inbuf" using "key", put resulting 256 bits in "keyedHash" buffer.

	calc_rfc_2104_hmac(key, inbuf, strlen(inbuf), keyedHash);

	// Create "outbuf" using JSON from "inbuf" (less first character) and keyed hash we just created.

	strcpy(outbuf, "{\"h\":[");
	for (i=0; i<SHA_LENGTH_IN_BYTES-1; i++)
	{
		sprintf(ascii_hash, "\"%02x\",", keyedHash[i]);
		strcat(outbuf, ascii_hash);
	}
	sprintf(ascii_hash, "\"%02x\"],", keyedHash[i]);
	strcat(outbuf, ascii_hash);
	strncat(outbuf, (inbuf+1), strlen(inbuf)-1);  // Copy all JSON except first character to output buffer
}


// ---------------------------------------------------------------------------
// Function:	read_serial_port_config
// Purpose:		Reads the settings for the given serial device.
// Inputs:		type = type of serial device (printer, smart injector, etc.)
// Outputs:		pSerial_port_config = ptr to struct to contain data.
// Returns:		0 (success) or <0 (database error)

int read_serial_port_config(sqlite3 * db, int data_type, bool second_port, int* serial_port)
{
	char query[MAX_LEN_SQL_QUERY];
	sqlite3_stmt *stmt;
	int rc;
	void map_enums_to_serial_port_settings(int, bool,int*, char *, char *, char *, char *, char *, char *, char *, char *);

	rc = sqlite3_exec(db, "BEGIN IMMEDIATE TRANSACTION", NULL, NULL, NULL);
	if (rc != SQLITE_OK)
	{
		sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
		syslog(LOG_ALERT, "json_write: %s on line:%d in file:%s\n", "Error locking flash database", __LINE__, __FILE__);
		return rc;
	}
	sprintf(query, "SELECT * FROM serial_port_config");
	rc = sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		syslog(LOG_ALERT, "json_write: %s on line:%d in file:%s\n", "Error preparing to read COM port setting", __LINE__, __FILE__);
		sqlite3_finalize(stmt);
		sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
		return rc;
	}
	rc = sqlite3_step(stmt);

	if (rc != SQLITE_ROW)
	{
		sqlite3_finalize(stmt);
		sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
		return rc;
	}
	int i = 0;
	while(rc == SQLITE_ROW)
	{
		serial_ports[i].comm_protocol	= sqlite3_column_int(stmt, 1);
		serial_ports[i].baud_rate 	= sqlite3_column_int(stmt, 2);
		serial_ports[i].data_parity 	= sqlite3_column_int(stmt, 3);
		serial_ports[i].host_control 	= sqlite3_column_int(stmt, 4);
		serial_ports[i].comm_timeout 	= sqlite3_column_int(stmt, 5);
		serial_ports[i].serial_protocol	= sqlite3_column_int(stmt, 6);
		serial_ports[i].rs485_duplex	= sqlite3_column_int(stmt, 7);
		serial_ports[i].term_resistors	= sqlite3_column_int(stmt, 8);

		rc = sqlite3_step(stmt);
		i++;
	}


	sqlite3_finalize(stmt);
	sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);

	// Fill out ASCII buffers with strings matching the enums used to set each of these parameters.

	map_enums_to_serial_port_settings
	(
		data_type,second_port,serial_port,
		comm_protocol, baud_rate, data_parity, host_control, comm_timeout,
		serial_protocol, rs485_duplex, term_resistors
	);
	return 0;
}

// ---------------------------------------------------------------------------
// Function:	map_enums_to_serial_port_settings
// Purpose:		Copies strings to output buffers, mapping from binary enum
//				values, so the JSON formatting inserts the proper text.
// Inputs:		comm_protocol, baud_rate, data_parity, host_control, comm_timeout
//				are all enums.
//				p1-5 are pointers to buffers to contain strings.
// Outputs:		String buffers are initialized.
// Returns:		Nothing

void map_enums_to_serial_port_settings
(
		int data_type,
		bool second_port,
		int* port,
		char *p1,
		char *p2,
		char *p3,
		char *p4,
		char *p5,
		char *p6,
		char *p7,
		char *p8
)
{

	*port = -1;
	//Find the Desired Port
	switch(data_type)
	{
		case TYPE_PRINTER:

			for(int i = 0; i < sizeof(serial_ports)/sizeof(serial_ports[0]); i++)
			{
				if(serial_ports[i].comm_protocol == PRINTER ||
						serial_ports[i].comm_protocol == SHARED_PRINTER)
				{
					if(second_port == true)
					{
						second_port = false;
						continue;
					}
					else
					{
						*port = i;
						break;
					}
				}
			}
			break;

		case TYPE_SMITH_COMM:
			for(int i = 0; i < sizeof(serial_ports)/sizeof(serial_ports[0]); i++)
			{
				if(serial_ports[i].comm_protocol == TERM_HOST_COMM ||
						serial_ports[i].comm_protocol == MINICOMP_HOST)
				{
					if(second_port == true)
					{
						second_port = false;
						continue;
					}
					else
					{
						*port = i;
						break;
					}
				}
			}

			break;

		case TYPE_SMART_INJECTOR:
			for(int i = 0; i < sizeof(serial_ports)/sizeof(serial_ports[0]); i++)
			{
				if(serial_ports[i].comm_protocol == SMART_INJECTOR)
				{
					if(second_port == true)
					{
						second_port = false;
						continue;
					}
					else
					{
						*port = i;
						break;
					}
				}
			}

			break;

		case TYPE_PROMASS:
			for(int i = 0; i < sizeof(serial_ports)/sizeof(serial_ports[0]); i++)
			{
				if(serial_ports[i].comm_protocol == EH_PROMASS)
				{
					if(second_port == true)
					{
						second_port = false;
						continue;
					}
					else
					{
						*port = i;
						break;
					}
				}
			}

			break;
	}

	//TODO: Error condition when port is a -1?

	if (port < 0)
	{
		return;
	}

	sprintf(p5,"%d", serial_ports[*port].comm_timeout);

	switch (serial_ports[*port].comm_protocol)
	{
		case NO_COMM:
			strcpy(p1, "NO_COMM");
			break;
		case TERM_HOST_COMM:
			strcpy(p1, "TERM_HOST_COMM");
			break;
		case MINICOMP_HOST:
			strcpy(p1, "MINICOMP_HOST");
			break;
		case MODBUS:
			strcpy(p1, "MODBUS");
			break;
		case PRINTER:
			strcpy(p1, "PRINTER");
			break;
		case SHARED_PRINTER:
			strcpy(p1, "SHARED_PRINTER");
			break;
		case SMART_INJECTOR:
			strcpy(p1, "SMART_INJECTOR");
			break;
		case EH_PROMASS:
			strcpy(p1, "EH_PROMASS");
			break;
		case SMITH_CARD_READER:
			strcpy(p1, "SMITH_CARD_READER");
			break;
		case NEDAP_READER:
			strcpy(p1, "NEDAP_READER");
			break;
		case FA_SENING_COP:
			strcpy(p1, "FA_SENING_COP");
			break;
		case DIAGNOSTIC:
			strcpy(p1, "DIAGNOSTIC");
			break;
	}
	switch (serial_ports[*port].baud_rate)
	{
		case BAUD_4800:
			strcpy(p2, "BAUD_4800");
			break;
		case BAUD_9600:
			strcpy(p2, "BAUD_9600");
			break;
		case BAUD_19200:
			strcpy(p2, "BAUD_19200");
			break;
		case BAUD_38400:
			strcpy(p2, "BAUD_38400");
			break;
		case BAUD_57600:
			strcpy(p2, "BAUD_57600");
			break;
		case BAUD_115200:
			strcpy(p2, "BAUD_115200");
			break;
	}
	switch (serial_ports[*port].data_parity)
	{
		case SEVEN_NONE:
			strcpy(p3, "SEVEN_NONE");
			break;
		case SEVEN_ODD:
			strcpy(p3, "SEVEN_ODD");
			break;
		case SEVEN_EVEN:
			strcpy(p3, "SEVEN_EVEN");
			break;
		case EIGHT_NONE:
			strcpy(p3, "EIGHT_NONE");
			break;
		case EIGHT_ODD:
			strcpy(p3, "EIGHT_ODD");
			break;
		case EIGHT_EVEN:
			strcpy(p3, "EIGHT_EVEN");
			break;
		case EIGHT_NONE_2:
			strcpy(p3, "EIGHT_NONE_2");
			break;
	}
	switch (serial_ports[*port].host_control)
	{
		case NO_CONTROL:
			strcpy(p4, "NO_CONTROL");
			break;
		case POLLING_ONLY:
			strcpy(p4, "POLLING_ONLY");
			break;
		case POLL_AUTHORIZE:
			strcpy(p4, "POLL_AUTHORIZE");
			break;
		case REMOTE_CONTROL:
			strcpy(p4, "REMOTE_CONTROL");
			break;
		case XON_XOFF:
			strcpy(p4, "XON_XOFF");
			break;
		case POLL_PROGRAM:
			strcpy(p4, "POLL_PROGRAM");
			break;
		case PTB_FX:
			strcpy(p4, "PTB_FX");
			break;
		case PTB_LQ:
			strcpy(p4, "PTB_LQ");
			break;
	}

	switch (serial_ports[*port].serial_protocol)
	{
		case RS232:
			strcpy(p6, "RS232");
			break;
		case RS485:
			strcpy(p6, "RS485");
			break;
	}

	switch (serial_ports[*port].rs485_duplex)
	{
		case FULL_DUPLEX:
			strcpy(p7, "FULL_DUPLEX");
			break;
		case HALF_DUPLEX:
			strcpy(p7, "HALF_DUPLEX");
			break;
	}

	switch (serial_ports[*port].term_resistors)
	{
		case NO_TERM_RESISTORS:
			strcpy(p8, "0");
			break;
		case YES_TERM_RESISTORS:
			strcpy(p8, "1");
			break;
	}
}
