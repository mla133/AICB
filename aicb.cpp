/* AICB Engine - by Matt Allen */

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
#define RXFILE1		"/var/tmp/a4m/socat_smart_injector_input_data_file1"
#define RXFILE2		"/var/tmp/a4m/socat_smart_injector_input_data_file2"
#define PIPE_FIFO1   	"/var/tmp/a4m/socat_output_smart_injector_fifo1"
#define PIPE_FIFO2   	"/var/tmp/a4m/socat_output_smart_injector_fifo2"
#define debugger { printf("%s:%d\n",__FILE__,__LINE__);}
#define MAX_BUF 25
#define MAX_BUF_LEN 17

//  File stream for event log
  std::ofstream logger("log.txt");

//void log( std::string file, int line, std::string message = "" )
void log(int line, std::string message = "" )
{
	//Write message to file
	logger << __FILE__<< "[" << line << "]: " << message << std::endl;
} 

//#define DEBUG 0
int main(int argc, char *argv[])
{
  if (argc != 2)
  {
	printf("USAGE:  ./SAI [option]\n");
	printf("1.  FIFO #1\n");
	printf("2.  FIFO #2\n");
	printf("3.  BOTH FIFOs (not implemented yet)\n");
	return(1);
  }

  char buffer[MAX_BUF];
  char response[MAX_BUF];
  char tempBuf[40];
  char address_buf[3];
  char log_msg[MAX_BUF];

  FILE * pFile;
  long lSize;
  size_t result;
  double NRT = 0.0;
  double NRT2 = 0.0;
  double NRT3 = 0.0;
  double NRT4 = 0.0;
  int pipe, buf_size, inj_address;

  // SETTING UP WHICH FIFO TO USE FOR COMMs TESTING
#ifdef DEBUG
  log(__LINE__, "***STARTING UP AICB BLACK BOX***");
  if ((atoi(argv[1])) == 1)
  {
	log(__LINE__, "FIFO -- /var/tmp/a4m/socat_output_smart_injector_fifo1");
	log(__LINE__, "FILE -- /var/tmp/a4m/socat_smart_injector_input_data_file1");
  }
  else if ((atoi(argv[1])) == 2)
  {

	log(__LINE__, "FIFO -- /var/tmp/a4m/socat_output_smart_injector_fifo2");
	log(__LINE__, "FILE -- /var/tmp/a4m/socat_smart_injector_input_data_file2");
  }
#endif

  memset(buffer, 0, MAX_BUF);

  while(1)
  {
	//Read in inj_tx to buffer
	if ((atoi(argv[1])) == 1)
		if ( (pipe = open( PIPE_FIFO1 , O_RDONLY)) < 0) log(__LINE__, "ERROR - PIPE1 missing");
	if ((atoi(argv[1])) == 2)
		if ( (pipe = open( PIPE_FIFO2 , O_RDONLY)) < 0) log(__LINE__, "ERROR - PIPE2 missing");

	if ( (buf_size = read(pipe, buffer, MAX_BUF)) < 0) log(__LINE__, "ERROR - Buffer misread");
	buffer[buf_size] = 0; //null terminate string

#ifdef DEBUG
	sprintf(log_msg, "Incoming: %s",buffer);
	log(__LINE__, log_msg);
        
	printf("[ ");
	for (int i=0;i<MAX_BUF;i++)
	  printf("\E[31;40m%02x ", buffer[i]);
	printf("]\E[0m");
#endif

	// Pull out address to strncat into response later
	memcpy(address_buf, buffer, 3);
	address_buf[3] = 0;
	inj_address = atoi(address_buf);

	strcpy(response, address_buf);

	// Parse buffer, supply response buffer
		
	if ( strncmp ("IN",buffer+3, 2) == 0 ) 			// II
	{
	  if(inj_address == 100) NRT4 += 0.01;
	  strcat(response, "OK");
	}
	else if ( strncmp ("EP",buffer+3, 2) == 0 )		// AUTHORIZE
	  strcat(response, "OK");
	else if ( strncmp ("DP",buffer+3, 2) == 0 )		// DEAUTHORIZE
	  strcat(response, "OK");
	else if ( strncmp ("RC",buffer+3, 2) == 0 )		// RESET_PULSE_COUNT
	  strcat(response, "OK");
	else if ( strncmp ("AI",buffer+3, 2) == 0 )		// AUTHORIZE_IO
	  strcat(response, "OK");
	else if ( strncmp ("DI",buffer+3, 2) == 0 )		// DEAUTHORIZE_IO
	  strcat(response, "OK");
	else if ( strncmp ("CA",buffer+3, 2) == 0 )		// CLEAR_ALARMS
	  strcat(response, "OK");
	else if ( strncmp ("ST",buffer+3, 2) == 0 )		// POLL_ALARMS
	  strcat( response, "ST 0000");
	else if ( strncmp ("SV",buffer+3, 2) == 0 )		// SOFTWARE_VERSION
	  strcat ( response, "SV 999 ABCDEF01");
	else if ( strncmp ("TS",buffer+3, 2) == 0 )		// POLL_TOTALS_AND_ALARMS
	{
	  sprintf(tempBuf, "TS %012.3f 0000", NRT4);
	  strcat(response, tempBuf);
	}

	else if ( strncmp ("PW",buffer+3, 2) == 0 )		// SET ???
	  strcat(response, "OK");

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

#ifdef DEBUG
	sprintf(log_msg, "Incoming: %s",response);
	log(__LINE__, log_msg);

	printf("[ ");
	for (int i=0;i<MAX_BUF;i++)
	  printf("\E[32;40m%02x ", response[i]);
	printf("]\E[0m\n");
#endif

	// Open up inj_rx and write response
	if ((atoi(argv[1])) == 1)
		pFile = fopen( RXFILE1, "w+");
	if ((atoi(argv[1])) == 2)
		pFile = fopen( RXFILE2, "w+");
  	fwrite(response, sizeof(char), strlen(response), pFile);
  	fclose(pFile);

	//Zero out tx/rx buffer strings and close up pipe
	memset(response,0,MAX_BUF);
	memset(buffer,0,MAX_BUF);
	close(pipe);
  }

  return 0;
}
