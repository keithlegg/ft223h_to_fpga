#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>    


#include "../ftd2xx.h"


#define false 0 

//declare parameters for 93C56
#define MemSize 32 //define data quantity you want to send out

//declare for BAD command 
const BYTE AA_ECHO_CMD_1         = '\xAA';
const BYTE AB_ECHO_CMD_2         = '\xAB';
const BYTE BAD_COMMAND_RESPONSE  = '\xFA'; 

//declare for MPSSE command
const BYTE MSB_RISING_EDGE_CLOCK_BYTE_OUT   = '\x10'; 
const BYTE MSB_FALLING_EDGE_CLOCK_BYTE_OUT  = '\x11';
const BYTE MSB_RISING_EDGE_CLOCK_BIT_OUT    = '\x12';
const BYTE MSB_FALLING_EDGE_CLOCK_BIT_OUT   = '\x13';
const BYTE MSB_RISING_EDGE_CLOCK_BYTE_IN    = '\x20';
const BYTE MSB_RISING_EDGE_CLOCK_BIT_IN     = '\x22';
const BYTE MSB_FALLING_EDGE_CLOCK_BYTE_IN   = '\x24';
const BYTE MSB_FALLING_EDGE_CLOCK_BIT_IN    = '\x26';

//KEITH ADDED THESE 
//SEE AN_108:Command processor for MPSEE
const BYTE MSB_RISING_IN_FALLING_OUT_BYTE_INOUT = '\x31';
const BYTE MSB_FALLING_IN_RISING_OUT_BYTE_INOUT = '\x34';


/******************************************/

FT_STATUS ftStatus;

uint dwClockDivisor   = 14; //was 29 //Value of clock divisor, SCL Frequency = 60/((1+29)*2) (MHz) = 1Mhz

BYTE OutputBuffer[512];      //Buffer to hold MPSSE commands AND DATA to be SENT to FT2232H
BYTE InputBuffer[512];       //Buffer to hold Data bytes to be READ from FT2232H

BYTE fileSendBuffer[512];     //Buffer to hold Data bytes read from file on disk to SEND to FT2232H

uint dwNumBytesToSend = 0;  //Index of output buffer
uint dwNumBytesSent   = 0, dwNumBytesRead = 0, dwNumInputBuffer = 0;

BYTE DataOutBuffer[MemSize];
BYTE DataInBuffer[MemSize];

BYTE ByteDataRead;
WORD MemAddress = 0x00;
WORD i=0;


/******************************************/
/******************************************/

/*

   " Standard SPI " is: 
       Idle low clock polarity
       Active to idle clock edge 
       Middle sample phase
       CS active low 
   ---------------------------
    
    //YOU HAVE TO RUN THIS FIRST TO DISABLE VCP !

    sudo rmmod ftdi_sio; sudo rmmod usbserial;

   ---------------------------

*/

/******************************************/
/******************************************/

void show_buffers(){

    uint xx = 0;

	if(dwNumBytesRead){
	    printf("#$ --- Number of bytes in input buffer %d \n", dwNumBytesRead);
        
	    if(dwNumBytesRead){	
	        for(xx=0;xx<dwNumBytesRead;xx++){    
	            printf("#$     --- input buffer byte:%d %d\n", xx, InputBuffer[xx]);	    
	        }
        }
    }

	if(dwNumBytesSent){    
		printf("#$ --- Number of bytes sent  %d \n", dwNumBytesSent);
	    if(dwNumBytesSent){	    
	        for(xx=0;xx<dwNumBytesSent;xx++){    
	            printf("#$ ---     output buffer byte:%d %d\n", xx, OutputBuffer[xx]);	    
	        }       
	    }		
    }

}


/******************************************/
//this routine is used to enable SPI device
void SPI_CSEnable(){
	int loop = 0;
	for( loop=0;loop<5;loop++) //one 0x80 command can keep 0.2us, do 5 times to stay in this situation for 1us
	{
		OutputBuffer[dwNumBytesToSend++] = '\x80'; //GPIO command for ADBUS
		OutputBuffer[dwNumBytesToSend++] = '\x00'; //set CS, MOSI and SCL low ***
		OutputBuffer[dwNumBytesToSend++] = '\x0b'; //bit3:CS, bit2:MISO,bit1:MOSI, bit0:SCK
	}
}

/******************************************/
//this routine is used to disable SPI device
void SPI_CSDisable()
{
	int loop=0;
	for( loop=0;loop<5;loop++) //one 0x80 command can keep 0.2us, do 5 times to stay in this situation for 1us
	{
		OutputBuffer[dwNumBytesToSend++] = '\x80'; //GPIO command for ADBUS 
		OutputBuffer[dwNumBytesToSend++] = '\x08'; //set CS high, MOSI and SCL low	*** 	
		OutputBuffer[dwNumBytesToSend++] = '\x0b'; //bit3:CS, bit2:MISO,bit1:MOSI, bit0:SCK
	}
}

/******************************************/
 
//this routine is used to initial SPI interface
BOOL SPI_Initial(FT_HANDLE ftHandle)
{
	uint dwCount;
	dwNumBytesToSend = 0;
	dwNumBytesRead   = 0;

	ftStatus = FT_ResetDevice(ftHandle); //Reset USB device
	
	//Purge USB receive buffer first by reading out all old data from FT2232H receive buffer
	ftStatus |= FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);
	
	// Get the number of bytes in the FT2232H receive buffer
	if ((ftStatus == FT_OK) && (dwNumInputBuffer > 0))
	{
		ftStatus |= FT_Read(ftHandle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead);
	}
	
	/**************************************************/
	//Read out the data from FT2232H receive buffer...
	//ARRRG. THE DOCS SAY EXPLICITLY SET DEFAULTS EVEN IF YOU DONT NEED TO FOR CLARITY
	//YOU ONLY WANT TO RUN THIS ONCE TO "PRIME" THE CHIP  
	//IF YOU DO RUN THIS A SECOND TIME YOU GET "failed to synchronize MPSSE with command '0xAA'" 
    // ftStatus |= FT_SetUSBParameters(ftHandle, 65535, 65535); //Set USB request transfer size
	// ftStatus |= FT_SetChars(ftHandle, false, 0, false, 0);   //Disable event and error characters
	// ftStatus |= FT_SetTimeouts(ftHandle, 3000, 3000); 	     //Sets the read and write timeouts in 3 sec for the FT2232H
	// ftStatus |= FT_SetLatencyTimer(ftHandle, 1);       	     //Set the latency timer (default is 16mS)
	// ftStatus |= FT_SetBitMode(ftHandle, 0x0, 0x00);   	     //Reset controller
	// ftStatus |= FT_SetBitMode(ftHandle, 0x0, 0x02);          //Enable MPSSE mode
	//THIS BLOCK OF CODE SEEMS TO CAUSE THE "EVERY OTHER - 0XAA init BUG"
	/**************************************************/

	if (ftStatus != FT_OK)
	{
		printf("fail on initialize FT2232H device !\n");
		return false;
	}

    usleep(50000); // Wait 50ms for all the USB stuff to complete and work
	
	//////////////////////////////////////////////////////////////////
	// Synchronize the MPSSE by sending a bogus opcode (0xAB), 
	// The MPSSE will respond with "Bad Command" (0xFA) followed by
	// the bogus opcode itself.
	//////////////////////////////////////////////////////////////////

	dwNumBytesToSend = 0;
	dwNumBytesRead   = 0;

	OutputBuffer[dwNumBytesToSend++] ='\xAA'; //Add BAD command 0xAA
	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);	// Send off the BAD commands
	dwNumBytesToSend = 0;	//Clear output buffer
	do
	{
		ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer); // Get the number of bytes in the device input buffer
	} while ((dwNumInputBuffer == 0) && (ftStatus == FT_OK));      //or Timeout
	
	bool bCommandEchod = false;

	ftStatus = FT_Read(ftHandle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead); 	//Read out the data from input buffer
	for (dwCount = 0; dwCount < (dwNumBytesRead-1); dwCount++)                      //Checkif Bad command and echo command received
	{
		if ((InputBuffer[dwCount]==(BYTE)('\xFA')) && (InputBuffer[dwCount+1]==(BYTE)('\xAA')) )
		{
		    bCommandEchod = true;
		    break;
		}
	}

	if(bCommandEchod == false)
	{
	    printf("\n-------------------------\nfailed to synchronize MPSSE with command '0xAA'\n");
		printf("\n-------------------------\n");
        show_buffers();

  	    return false;
	    // Error, cant receive echo command , fail to synchronize MPSSE interface;
	}

	//////////////////////////////////////////////////////////////////
	// Synchronize the MPSSE interface by sending bad command 0xAB
	//////////////////////////////////////////////////////////////////

	dwNumBytesToSend = 0;

	//Clear output buffer
	OutputBuffer[dwNumBytesToSend++] ='\xAB'; //Send BAD command 0xAB
	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend,&dwNumBytesSent); // Send off the BAD commands
	dwNumBytesToSend = 0; //Clear output buffer
	do
	{
		ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer);
		//Get the number of bytes in the device input buffer
	}while((dwNumInputBuffer == 0) && (ftStatus == FT_OK)); //or Timeout
	bCommandEchod = false;


	ftStatus = FT_Read(ftHandle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead); //Read out the data from input buffer
	for (dwCount = 0; dwCount < (dwNumBytesRead-1); dwCount++)//Check if Bad command and echo command received
	{
		if((InputBuffer[dwCount]==(BYTE)('\xFA')) && (InputBuffer[dwCount+1]==(BYTE)('\xAB')))
		{
			bCommandEchod = true;
			break;
		}
    }

	if(bCommandEchod==false)
	{
		printf("\n-------------------------\nfailed to synchronize MPSSE with command '0xAB'\n");
		printf("\n-------------------------\n");		
        show_buffers();

		return false;
		// Error, cant receive echo command , fail to synchronize MPSSE interface; 
	}

	////////////////////////////////////////////////////////////////////
	//Configure the MPSSE for SPI communication with EEPROM
	//Although the default settings may be appropriate for a particular application, it is always a good 
    //practice to explicitly send all of the op-codes to enable or disable each of these features
	//////////////////////////////////////////////////////////////////

	OutputBuffer[dwNumBytesToSend++] ='\x8A'; //Ensure disable clock divide by5 for 60Mhz master clock
	OutputBuffer[dwNumBytesToSend++] ='\x97'; //Ensure turn off adaptive clocking
    OutputBuffer[dwNumBytesToSend++] ='\x8D'; //disable 3 phase data clock
	
	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend,&dwNumBytesSent);// Send out the commands
	
	dwNumBytesToSend = 0;//Clear output buffer
	OutputBuffer[dwNumBytesToSend++] ='\x80'; //Command to set directions of lower 8 pins and force value on bits set as output
	OutputBuffer[dwNumBytesToSend++] ='\x00'; //Set SDA, SCL high, WP disabledby SK, DO at bit＆＊, GPIOL0 at bit＆＊
	OutputBuffer[dwNumBytesToSend++] ='\x0b'; //Set SK,DO,GPIOL0 pins as output with bit＊＊, other pins as input with bit＆＊
	
	//The FT2232D is based around a 12MHz clock.  A 16-bit divisor is used to program the data transmission 
	// The SK clock frequency can be worked out by below algorithm with divide by 5 set as off
	// SK frequency = 60MHz /((1 + [(1 +0xValueH*256) OR 0xValueL])*2)
	
	OutputBuffer[dwNumBytesToSend++] ='\x86';	                      //Command to set clock divisor
	
	OutputBuffer[dwNumBytesToSend++] = (BYTE)(dwClockDivisor &'\xFF');  //Set 0xValueL of clock divisor
	OutputBuffer[dwNumBytesToSend++] = (BYTE)(dwClockDivisor >> 8);     //Set 0xValueH 

	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend,&dwNumBytesSent);// Send out the commands
	dwNumBytesToSend = 0; //Clear output buffer
	usleep(20000); //20 ms

	//Delay for a while
	//Turn off loop back in case
	OutputBuffer[dwNumBytesToSend++] ='\x85'; //Command to turn off loopback of TDI/TDO connection
	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend,&dwNumBytesSent);// Send out the commands
	dwNumBytesToSend = 0; //Clear output buffer
	usleep(30000); //(30 ms);

	//Delay for a while
	printf("SPI initial successful\n");
	return true;

}

/******************************************/
/******************************************/

//keith made this from WriteEECmd to send a single byte. Works but may not be right.
FT_STATUS single_byte_write(FT_HANDLE ftHandle, BYTE command, WORD *bdata)
{

	dwNumBytesSent=0;

	SPI_CSEnable();

	OutputBuffer[dwNumBytesToSend++] = MSB_FALLING_EDGE_CLOCK_BIT_OUT;
	OutputBuffer[dwNumBytesToSend++] = 7; //7+1 = 8
	OutputBuffer[dwNumBytesToSend++] = command;

	SPI_CSDisable();

	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend,&dwNumBytesSent); //send MPSSE command to MPSSE engine.
	dwNumBytesToSend = 0; //Clear output buffer

	return ftStatus;

}


/******************************************/


//this routine is used to read one word data from a random address
//const BYTE MSB_RISING_IN_FALLING_OUT_BYTE_INOUT = '\x31';
//const BYTE MSB_FALLING_IN_RISING_OUT_BYTE_INOUT = '\x34';

BOOL SPI_KeithFullDuplexTest(FT_HANDLE ftHandle, uint numsend, BYTE *spi_send)  
{

	dwNumBytesToSend = 0; //Clear output buffer
    dwNumBytesRead   = 0; //Clear input buffer

	SPI_CSEnable(); //sends 3 bytes    
	OutputBuffer[dwNumBytesToSend++] = MSB_RISING_IN_FALLING_OUT_BYTE_INOUT; // '\x31';
	
	//tell the FT2232H how many bytes are heading that way (zero index 16 bit LSB)
    unsigned int high = (unsigned int)(numsend >> 8);
    unsigned int low  =                numsend & 0xff;
	OutputBuffer[dwNumBytesToSend++] = low;  //'\x04'; //write Size Low  (8 = 7+1 bytes )
	OutputBuffer[dwNumBytesToSend++] = high; //'\x00'; //write Size High

    //HERE IS THE BURST OF 8 BYTES THAT GET SENT 
	uint i = 0;
	for(i=0;i<numsend;i++){
        OutputBuffer[dwNumBytesToSend++] = spi_send[i];
	}

	SPI_CSDisable(); //sends 3 bytes 

	ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent); //send out MPSSE command to MPSSE engine
	ftStatus = FT_Read(ftHandle, InputBuffer, numsend, &dwNumBytesRead); //Read XXX bytes from device receive buffer


    dwNumBytesRead   = 0; //Clear Input buffer 
    dwNumBytesToSend = 0; //Clear output buffer

	return ftStatus;
}


/******************************************/
/*
   read a binary file from disk and send the bytes contained in it over to FT2232H via SPI 
   this just copies the contents of the file into fileSendBuffer to be sent 
*/

void read_file( char *filename )
{
	FILE *fileptr;
	char *buffer;
	long filelen;

	fileptr = fopen(filename, "rb");  // Open the file in binary mode
	fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
	filelen = ftell(fileptr);             // Get the current byte offset in the file
	rewind(fileptr);                      // Jump back to the beginning of the file

	buffer = (char *)malloc((filelen+1)*sizeof(char)); // Enough memory for file + \0
	fread(buffer, filelen, 1, fileptr); // Read in the entire file
	fclose(fileptr); // Close the file

    ///
    int p = 0;
	for(p=0;p<filelen;p++){
        //printf(" 0x%1x ", (BYTE)buffer[p] ); putchar('\n');
        fileSendBuffer[p] = (BYTE)buffer[p];
	}
}

/******************************************/

void write_file( char *filename)
{

    FILE *f = fopen(filename, "w");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    const char *divider = "##################################";

    fprintf(f, "%s\n", divider);  

    for (int x=0;x<5;x++)
    {
        fprintf(f, "data read from FPGA: %i \n", InputBuffer[x] );  
    }

    fclose(f);
}

/******************************************/
/*
    the function that inits the SPI hardware and invokes the read/write full duplex IO 
*/

void ftdi_talk( int numsend ){
	FT_HANDLE ftdiHandle;
	
	uint numDevs;

	FT_DEVICE_LIST_INFO_NODE *devInfo;
	ftStatus = FT_CreateDeviceInfoList(&numDevs);// p* to unsigned long - number of connected devices.

	if(numDevs > 0) 
	{
		// allocate storage for list based on numDevs
		devInfo =(FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
	}else return 1;


	ftStatus = FT_Open(0, &ftdiHandle);
	if (ftStatus != FT_OK)
	{
		printf("Can't open FT2232H device!\n");
		return 1;
	} 
	   else 
	{
	  // Port opened successfully
	  printf("Successfully opened FT2232H device!\n");
    }


	if (SPI_Initial(ftdiHandle) == TRUE)	
	{

		char ReadByte = 0;

		//initial output buffer
		for	(i=0;i<MemSize;i++)	
		{	
			DataOutBuffer[i] = i;
	    }
	
		//Purge USB received buffer first before read operation
		ftStatus = FT_GetQueueStatus(ftdiHandle, &dwNumInputBuffer);
		
		// Get the number of bytes in the device receive buffer
		if((ftStatus == FT_OK) && (dwNumInputBuffer > 0))
		{
		    FT_Read(ftdiHandle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead);
	    }
        /////////////////////////////////////////

        //int numsend = 5;
        //BYTE foobuffer[9-1] = {'\x96', '\x00', '\xf2', '\x00', '\x00' , '\x00', '\x00', '\x00', '\x00'};
		//SPI_KeithFullDuplexTest(ftdiHandle, numsend, &foobuffer[0]);
		
		SPI_KeithFullDuplexTest(ftdiHandle, numsend, &fileSendBuffer[0]);

		int i = 0;
		for(i=0; i<numsend; i++){
		    printf("Read data - idx: %d = %d\n", i, InputBuffer[i]);
	    }
	  
        /////////////////////////////////////////

        //FOR DEBUGGING DATA IN BUFFERS
	    //printf("\n\n#### POST\n");
        //show_buffers();

        //////////////////////////////////////////
	}
 
	FT_Close(ftdiHandle);
}

/******************************************/
/******************************************/

int main(int argc, char **argv) 
{

    /*
        Not perfect, but I found a workable solution for now:

        send 4 "dummy bytes" to syncronize the command count,
        send each command in 4 bytes cycles after that.

        00 00 00 00 | 23 01 31 00 | F0 01 E0 00 | 
        
        ftdi_talk num bytes =  4 + ((num commands * 4) -1 )

        or simply (num commands *4 -1 ) , but ignore the first command 
 

    */

    if (argc < 4){

        // printf("%d \n",sizeof(uint));
        // printf("%d \n",sizeof(int));
        // printf("%d \n",sizeof(short));
        // printf("%d \n",sizeof(long));

		// unsigned char bytes[4];
		// unsigned long n = 175;
		// bytes[0] = (n >> 24) & 0xFF;
		// bytes[1] = (n >> 16) & 0xFF;
		// bytes[2] = (n >> 8)  & 0xFF;
		// bytes[3] = n         & 0xFF;
		// printf("%d", bytes[3]);
        
        // uint x = 5; // 0xaacc 170,204 
		// printf(" \n ----  %u \n", (x >> 8)  );
		// printf(" \n ----  %u \n",  x & 0xff );

        printf("\n\n\n## not enough arguments - needs 3 ##\n");
        printf("## args are 'read/write' 'binfile' 'num commands' \n\n\n");   
        abort();
    }

    int numcommands = atoi(argv[3]); //number of FPGA commands (4 bytes each)
    int command_bytes = 4;           //how big is a "command" packet   

    char commbuffer[10];

    /***********************/
    strcpy(commbuffer, "write");
    if( strcmp(argv[1], commbuffer) == 0)
    {
    	if (argv[2]== NULL){
            printf("please secify a filename \n");
            abort();
    	} 
    	printf("talking to FTDI chip and writing results to file %s \n", argv[2]);
    	ftdi_talk((numcommands*command_bytes)-1);
    	write_file( argv[2] );

    }

    /***********************/
    strcpy(commbuffer, "read");
    if( strcmp(argv[1], commbuffer) == 0)
    {
  		if (argv[2]== NULL){
            printf("please secify a filename \n");
            abort();
    	} 

    	printf("read file %s \n", argv[2]);
    	read_file(argv[2]);
    	ftdi_talk((numcommands*command_bytes)-1);
    	write_file( "read_back.txt" );
    }

    //////////////////////////////////

	return 0;
}













