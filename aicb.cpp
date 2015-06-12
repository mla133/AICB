/* AICB/Metered Injector Engine - by Matt Allen */

/* Constantly tries to read file PIPE_FIFOx for buffered message
 * and then outputs to write file RXFILE# for response from pseudo
 * smart injector 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <fstream>

#define RXFILE1		"/var/tmp/a4m/serial_input_data_file1"
#define RXFILE2		"/var/tmp/a4m/serial_input_data_file2"
#define RXFILE3		"/var/tmp/a4m/serial_input_data_file3"
#define RXFILE4		"/var/tmp/a4m/serial_input_data_file4"
#define PIPE_FIFO1   	"/var/tmp/a4m/socat_output_serial_fifo1"
#define PIPE_FIFO2   	"/var/tmp/a4m/socat_output_serial_fifo2"
#define PIPE_FIFO3   	"/var/tmp/a4m/socat_output_serial_fifo3"
#define PIPE_FIFO4   	"/var/tmp/a4m/socat_output_serial_fifo4"
#define debugger { printf("%s:%d\n",__FILE__,__LINE__);}
#define MAX_BUF 25
#define MAX_BUF_LEN 17
#define MAX_TMP_BUF 40
#define ADDR_SIZE 3

//  File stream for event log
  std::ofstream logger("log.txt");

//void log( std::string file, int line, std::string message = "" )
void log(int line, std::string message = "" )
{
	//Write message to file
	logger << __FILE__<< "[" << line << "]: " << message << std::endl;
} 

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
	printf("USAGE:  ./SAI [option]\n");
	printf("1.  SERIAL #1\n");
	printf("2.  SERIAL #2\n");
	printf("3.  SERIAL #3\n");
	printf("4.  SERIAL #4\n");
	/*
	printf("5.  MTR_INJ  \n");
	printf("6.  A4I (AICB) #1\n");
	printf("6.  A4I (AICB) #2\n");
	*/
	return(1);
  }

  char buffer[MAX_BUF];
  char response[MAX_BUF];
  char tempBuf[MAX_TMP_BUF];
  char address_buf[ADDR_SIZE];
  char log_msg[MAX_BUF];

  FILE * pFile;
  long lSize;
  size_t result;
  float CONV_FACTOR = 3785.412;
  double NRT_1, NRT_2, NRT_3, NRT_4, pulse_1, pulse_2, pulse_3, pulse_4 = 0;
  int sol_1,sol_2, sol_3, sol_4, pump_1,pump_2, pump_3, pump_4;

  enum {
	OFF,
	ON};

  int pipe, buf_size, inj_address;

  // SETTING UP WHICH FIFO TO USE FOR COMMs TESTING
  log(__LINE__, "***STARTING UP AICB BLACK BOX***");
  if ((atoi(argv[1])) == 1)
  {
	log(__LINE__, "FIFO -- /var/tmp/a4m/socat_output_serial_fifo1");
	log(__LINE__, "FILE -- /var/tmp/a4m/serial_input_data_file1");
  }
  else if ((atoi(argv[1])) == 2)
  {

	log(__LINE__, "FIFO -- /var/tmp/a4m/socat_output_serial_fifo2");
	log(__LINE__, "FILE -- /var/tmp/a4m/serial_input_data_file2");
  }
  else if ((atoi(argv[1])) == 3)
  {

	log(__LINE__, "FIFO -- /var/tmp/a4m/socat_output_serial_fifo2");
	log(__LINE__, "FILE -- /var/tmp/a4m/serial_input_data_file2");
  }
  else if ((atoi(argv[1])) == 4)
  {

	log(__LINE__, "FIFO -- /var/tmp/a4m/socat_output_serial_fifo2");
	log(__LINE__, "FILE -- /var/tmp/a4m/serial_input_data_file2");
  }
  else
 	return(1);

  memset(buffer, 0, MAX_BUF);

  while(1)
  {
	//Read in inj_tx to buffer
	if ((atoi(argv[1])) == 1)
		if ( (pipe = open( PIPE_FIFO1 , O_RDONLY)) < 0) log(__LINE__, "ERROR - SERIAL1 missing");
	if ((atoi(argv[1])) == 2)
		if ( (pipe = open( PIPE_FIFO2 , O_RDONLY)) < 0) log(__LINE__, "ERROR - SERIAL2 missing");
	if ((atoi(argv[1])) == 3)
		if ( (pipe = open( PIPE_FIFO1 , O_RDONLY)) < 0) log(__LINE__, "ERROR - SERIAL3 missing");
	if ((atoi(argv[1])) == 4)
		if ( (pipe = open( PIPE_FIFO1 , O_RDONLY)) < 0) log(__LINE__, "ERROR - SERIAL4 missing");

	if ( (buf_size = read(pipe, buffer, MAX_BUF)) < 0) log(__LINE__, "ERROR - Buffer misread");
	buffer[buf_size] = 0; //null terminate string

	sprintf(log_msg, " Incoming: %s",buffer);
	log(__LINE__, log_msg);
        
	// Pull out address to strncat into response later
	memcpy(address_buf, buffer, 3);
	address_buf[3] = 0;
	inj_address = atoi(address_buf);

	if(inj_address<400)
	{
	  printf("\E[31;40mAddressing Metered Injectors[%d]\E[0m\n",inj_address);
	  continue;
	}

	strcpy(response, address_buf);

	// Parse buffer, supply response buffer
		
	if ( strncmp ("IN",buffer+3, 2) == 0 ) 			// II
	{
	  if (inj_address < 400)
	  {
		NRT_1 += 0.013;
		pulse_1 += 65.0;
		sol_1 = ON;
	  }
	  if (inj_address == 402)
	  {
	  	NRT_2 += 0.013;
		pulse_2 += 65.0;
		sol_2 = ON;
		
	  }
	  if (inj_address == 403)
	  {
	  	NRT_3 += 0.013;
		pulse_3 += 65.0;
		sol_3 = ON;
	  } 
	  if (inj_address == 404) {
	  	NRT_4 += 0.026;
		pulse_4 += 130.0;
		sol_4 = ON;
	  }

	  strcat(response, "\E[33;40mOK\E[0m");
	}

	else if ( strncmp ("EP",buffer+3, 2) == 0 )		// AUTHORIZE
	{
	  pump_1 = pump_2 = pump_3 = pump_4 = ON;
	  strcat(response, "OK");
	
	}
	else if ( strncmp ("DP",buffer+3, 2) == 0 )		// DEAUTHORIZE
	{
	  pump_1 = pump_2 = pump_3 = pump_4 = OFF;
	  strcat(response, "OK");
	}
	else if ( strncmp ("RC",buffer+3, 2) == 0 )		// RESET_PULSE_COUNT
	{
	  pulse_1 = 0;
	  pulse_2 = 0;
	  pulse_3 = 0;
	  pulse_4 = 0;
	  strcat(response, "OK");
	}
	else if ( strncmp ("AI",buffer+3, 2) == 0 )		// AUTHORIZE_IO
	  strcat(response, "OK");
	else if ( strncmp ("DI",buffer+3, 2) == 0 )		// DEAUTHORIZE_IO
	  strcat(response, "OK");
	else if ( strncmp ("CA",buffer+3, 2) == 0 )		// CLEAR_ALARMS
	  strcat(response, "OK");
	else if ( strncmp ("ST",buffer+3, 2) == 0 )		// POLL_ALARMS
	  strcat( response, "ST 0000");
	else if ( strncmp ("SV",buffer+3, 2) == 0 )		// SOFTWARE_VERSION
	  strcat ( response, "SV 06 ABCDEF01");
	else if ( strncmp ("TS",buffer+3, 2) == 0 )		// POLL_TOTALS_AND_ALARMS
	{
	  if (inj_address < 400)
	  {
		sol_1 = OFF;
	  	sprintf(tempBuf, "TS %12.3f 0000", NRT_1);
	  }
	  if (inj_address == 402)
	  {
	 	sol_2 = OFF;
	  	sprintf(tempBuf, "TS %12.3f 0000", NRT_2);
	  }
	  if (inj_address == 403)
	  {
		sol_3 = OFF;
	  	sprintf(tempBuf, "TS %12.3f 0000", NRT_3);
	  }
	  if (inj_address == 404)
	  {
	 	sol_4 = OFF;
	  	sprintf(tempBuf, "TS %12.3f 0000", NRT_4);
	  }

	  strcat(response, tempBuf);
	}

	else if ( strncmp ("PC",buffer+3, 2) == 0 )		// READ PULSE COUNTS
	{
	  if (inj_address < 400)
	  	sprintf(tempBuf, "PC %8.1f", pulse_1);
	  if (inj_address == 402)
	  	sprintf(tempBuf, "PC %8.1f", pulse_2);
	  if (inj_address == 403)
	  	sprintf(tempBuf, "PC %8.1f", pulse_3);
	  if (inj_address == 404)
	  	sprintf(tempBuf, "PC %8.1f", pulse_4);

	  strcat(response, tempBuf);
	}

	else if ( strncmp ("OS S",buffer+3, 4) == 0 )		// GET SAI STATE OF SOLENOID 
	{
		if(inj_address < 400)
		  sprintf(tempBuf, "OS S %d", sol_1);
		if(inj_address == 402)
		  sprintf(tempBuf, "OS S %d", sol_2);
		if(inj_address == 403)
		  sprintf(tempBuf, "OS S %d", sol_3);
		if(inj_address == 404)
		  sprintf(tempBuf, "OS S %d", sol_4);

		strcat(response, tempBuf);
	}

	else if ( strncmp ("OS P",buffer+3, 4) == 0 )		// GET SAI STATE OF PUMP
	{
		if(inj_address < 400)
		  sprintf(tempBuf, "OS P %d", pump_1);
		if(inj_address == 402)
		  sprintf(tempBuf, "OS P %d", pump_2);
		if(inj_address == 403)
		  sprintf(tempBuf, "OS P %d", pump_3);
		if(inj_address == 404)
		  sprintf(tempBuf, "OS P %d", pump_4);

		strcat(response, tempBuf);
	}

	else if ( strncmp ("PW",buffer+3, 2) == 0 )		// SET PARAMETERS
	{
	  strcat(response, "OK");
	}

	// GENERAL ERROR RESPONSES
	else if ( strncmp ("", buffer, MAX_BUF_LEN) == 0)
	{
	  memset(response,0,MAX_BUF);
	  log(__LINE__, "Error:  No incoming buffer detected");
	  continue;
	}
	else
	{
	  strcat(response,"NO00");
	  log(__LINE__, "Error:  No command match");
	}

	// Open up inj_rx and write response
	if ((atoi(argv[1])) == 1)
		pFile = fopen( RXFILE1, "w+");
	if ((atoi(argv[1])) == 2)
		pFile = fopen( RXFILE2, "w+");
	if ((atoi(argv[1])) == 3)
		pFile = fopen( RXFILE3, "w+");
	if ((atoi(argv[1])) == 4)
		pFile = fopen( RXFILE4, "w+");

  	//fwrite(response, sizeof(char), strlen(response), pFile);
	fprintf(pFile, "%s", response);
  	fclose(pFile);

	sprintf(log_msg, "Outgoing: %s",response);
	log(__LINE__, log_msg);

	//Zero out tx/rx buffer strings and close up pipe
	memset(response,0,MAX_BUF);
	memset(buffer,0,MAX_BUF);
	close(pipe);
  }

  return 0;
}
