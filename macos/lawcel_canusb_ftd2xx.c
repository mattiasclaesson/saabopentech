/*
 *  lawcel_canusb_ftd2xx.c
 *  saabopentechproj
 *
 *  Created by Mattias Claesson on 2012-03-01.
 *  Copyright 2012. All rights reserved.
 *
 */

#include "lawcel_canusb_ftd2xx.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

void initializeCanUsb()
{
	FT_SetVIDPID(0x0403,0xffa8);
}

void getVersionInfo(FT_HANDLE ftHandle)
{
	FT_STATUS status;
	char buf[BUF_SIZE];
	char *p;
	DWORD nBytesWritten;
	DWORD eventStatus;
	DWORD nRxCnt;// Number of characters in receive queue
	DWORD nTxCnt;// Number of characters in transmit queue
	
	memset( buf, 0, sizeof( buf ) );
	printf("getVersionInfo()\n");
	
	FT_Purge( ftHandle, FT_PURGE_RX | FT_PURGE_TX );
	
	sprintf( buf, "V\r" );
	if ( FT_OK != ( status = FT_Write( ftHandle, buf, strlen( buf ), &nBytesWritten ) ) ) {
		printf("Error: Failed to write command. return code = %d\n", status );
		return;
	}
	
	// Check if there is something to receive
	while ( 1 ){
		
		if ( FT_OK == FT_GetStatus( ftHandle, &nRxCnt, &nTxCnt, &eventStatus ) ) {
			
			// If there are characters to receive
			if ( nRxCnt ) {
				
				if ( FT_OK != ( status = FT_Read( ftHandle, buf, nRxCnt, &nBytesWritten ) ) ) {
					printf("Error: Failed to read data. return code = %d\n", status );
					return;
				}
				
				p = buf;
				while ( *p ) {
					if ( 0x0d == *p ) {
						*p = 0;
						break;
					}
					p++;
				}
				printf( "Version = %s\n", buf );
				break;
				
			}
			
		}
		else {
			printf("Error: Failed to get status. return code = %d\n", status );
			return;
		}
		
	}
	
}

// Turn OFF the Time Stamp feature.
void setTimeStampOff( FT_HANDLE ftHandle )
{
	FT_STATUS status;
	char buf[BUF_SIZE];
	char *p;
	DWORD nBytesWritten;
	DWORD eventStatus;
	DWORD nRxCnt;// Number of characters in receive queue
	DWORD nTxCnt;// Number of characters in transmit queue
	
	memset( buf, 0, sizeof( buf ) );
	printf("setTimeStampOff()\n");
	
	FT_Purge( ftHandle, FT_PURGE_RX | FT_PURGE_TX );
	
	// code
	sprintf( buf, "Z0\r" );
	if ( FT_OK != ( status = FT_Write( ftHandle, buf, strlen( buf ), &nBytesWritten ) ) ) {
		printf("Error: Failed to write command. return code = %d\n", status );
		return;
	}
	
	// Check if there is something to receive
	while ( 1 ){
		
		if ( FT_OK == FT_GetStatus( ftHandle, &nRxCnt, &nTxCnt, &eventStatus ) ) {
			
			// If there are characters to receive
			if ( nRxCnt ) {
				
				if ( FT_OK != ( status = FT_Read( ftHandle, buf, nRxCnt, &nBytesWritten ) ) ) {
					printf("Error: Failed to read data. return code = %d\n", status );
					return;
				}
				
				p = buf;
				while ( *p ) {
					if ( 0x0d == *p ) {
						*p = 0;
						break;
					}
					p++;
				}
				
				printf( "OK timestamp\n", buf  );
				break;
				
			}
			
		}
		else {
			printf("Error: Failed to get status. return code = %d\n", status );
			return;
		}
		
	}	
}

void setCodeRegister( FT_HANDLE ftHandle )
{
	FT_STATUS status;
	char buf[BUF_SIZE];
	char *p;
	DWORD nBytesWritten;
	DWORD eventStatus;
	DWORD nRxCnt;// Number of characters in receive queue
	DWORD nTxCnt;// Number of characters in transmit queue
	
	memset( buf, 0, sizeof( buf ) );
	printf("setCodeRegister()\n");
	
	FT_Purge( ftHandle, FT_PURGE_RX | FT_PURGE_TX );
	
	// code
	sprintf( buf, "M00000000\r" );
	if ( FT_OK != ( status = FT_Write( ftHandle, buf, strlen( buf ), &nBytesWritten ) ) ) {
		printf("Error: Failed to write command. return code = %d\n", status );
		return;
	}
	
	// Check if there is something to receive
	while ( 1 ){
		
		if ( FT_OK == FT_GetStatus( ftHandle, &nRxCnt, &nTxCnt, &eventStatus ) ) {
			
			// If there are characters to receive
			if ( nRxCnt ) {
				
				if ( FT_OK != ( status = FT_Read( ftHandle, buf, nRxCnt, &nBytesWritten ) ) ) {
					printf("Error: Failed to read data. return code = %d\n", status );
					return;
				}
				
				p = buf;
				while ( *p ) {
					if ( 0x0d == *p ) {
						*p = 0;
						break;
					}
					p++;
				}
				
				printf( "OK code\n", buf  );
				break;
				
			}
			
		}
		else {
			printf("Error: Failed to get status. return code = %d\n", status );
			return;
		}
		
	}	
}

void setMaskRegister( FT_HANDLE ftHandle )
{
	FT_STATUS status;
	char buf[BUF_SIZE];
	char *p;
	DWORD nBytesWritten; 
	DWORD eventStatus;
	DWORD nRxCnt;// Number of characters in receive queue
	DWORD nTxCnt;// Number of characters in transmit queue
	
	memset( buf, 0, sizeof( buf ) );
	printf("setMaskRegister()\n");
	
	FT_Purge( ftHandle, FT_PURGE_RX | FT_PURGE_TX );
	
	// code
	sprintf( buf, "mFFFFFFFF\r" );
	if ( FT_OK != ( status = FT_Write( ftHandle, buf, strlen( buf ), &nBytesWritten ) ) ) {
		printf("Error: Failed to write command. return code = %d\n", status );
		return;
	}
	
	// Check if there is something to receive
	while ( 1 ){
		
		if ( FT_OK == FT_GetStatus( ftHandle, &nRxCnt, &nTxCnt, &eventStatus ) ) {
			
			// If there are characters to receive
			if ( nRxCnt ) {
				
				if ( FT_OK != ( status = FT_Read( ftHandle, buf, nRxCnt, &nBytesWritten ) ) ) {
					printf("Error: Failed to read data. return code = %d\n", status );
					return;
				}
				
				p = buf;
				while ( *p ) {
					if ( 0x0d == *p ) {
						*p = 0;
						break;
					}
					p++;
				}
				
				printf( "OK mask\n", buf  );
				break;
				
			}
			
		}
		else {
			printf("Error: Failed to get status. return code = %d\n", status );
			return;
		}
		
	}	
}


void getSerialNumber( FT_HANDLE ftHandle )
{
	FT_STATUS status;
	char buf[BUF_SIZE];
	char *p;
	DWORD nBytesWritten;
	DWORD eventStatus;
	DWORD nRxCnt;// Number of characters in receive queue
	DWORD nTxCnt;// Number of characters in transmit queue
	
	memset( buf, 0, sizeof( buf ) );
	printf("getSerialNumber()\n");
	
	FT_Purge( ftHandle, FT_PURGE_RX | FT_PURGE_TX );
	
	sprintf( buf, "N\r" );
	if ( FT_OK != ( status = FT_Write( ftHandle, buf, strlen( buf ), &nBytesWritten ) ) ) {
		printf("Error: Failed to write command. return code = %d\n", status );
		return;
	}
	
	// Check if there is something to receive
	while ( 1 ){
		
		if ( FT_OK == FT_GetStatus( ftHandle, &nRxCnt, &nTxCnt, &eventStatus ) ) {
			
			// If there are characters to receive
			if ( nRxCnt ) {
				
				if ( FT_OK != ( status = FT_Read( ftHandle, buf, nRxCnt, &nBytesWritten ) ) ) {
					printf("Error: Failed to read data. return code = %d\n", status );
					return;
				}
				
				p = buf;
				while ( *p ) {
					if ( 0x0d == *p ) {
						*p = 0;
						break;
					}
					p++;
				}
				
				printf( "Serial = %s \n", buf  );
				break;
				
			}
			
		}
		else {
			printf("Error: Failed to get status. return code = %d\n", status );
			return;
		}
		
	}
	
}

BOOL openChannel( FT_HANDLE ftHandle, char* bitrate )
{
	char buf[BUF_SIZE];
	DWORD retLen;
	
	// Set baudrate
	FT_Purge( ftHandle, FT_PURGE_RX );
	//sxxyy[CR] 
	//Setup with BTR0/BTR1 CAN bit-rates where xx and yy is a hex value. 
	//This command is only active if the CAN channel is closed.
	//	xx	BTR0 value in hex yy	BTR1 value in hex
	//	Example:	s031C[CR] Setup CAN with BTR0=0x03 & BTR1=0x1C
	//	which equals to 125Kbit.
	//	Returns:	CR (Ascii 13) for OK or BELL (Ascii 7) for ERROR.
	if ( !( FT_OK == FT_Write( ftHandle, bitrate, strlen(bitrate), &retLen ) ) )
	{ 
		printf("Write failed\n");
		return FALSE;
	}
	
	// Open device
	FT_Purge( ftHandle, FT_PURGE_RX );
	strcpy( buf, "O\r" );
	if ( !( FT_OK == FT_Write( ftHandle, buf, 2, &retLen ) ) )
	{
		printf("Write failed\n");
		return FALSE;
	}
	
	return TRUE;
}


BOOL closeChannel( FT_HANDLE ftHandle )
{
	char buf[BUF_SIZE];
	DWORD retLen;
	
	// Close device
	FT_Purge( ftHandle, FT_PURGE_RX | FT_PURGE_TX );
	strcpy( buf, "C\r" );
	if ( !( FT_OK == FT_Write( ftHandle, buf, 2, &retLen ) ) )
	{ 
		return FALSE;
	}
	
	return TRUE;
}

FT_STATUS setTimeouts( FT_HANDLE ftHandle, ULONG ReadTimeout, ULONG WriteTimeout)
{
	return FT_SetTimeouts( ftHandle, ReadTimeout, WriteTimeout );
}

BOOL sendFrame( FT_HANDLE ftHandle, CANMsg *pmsg )
{
	int i; 
	char txbuf[BUF_SIZE];
	unsigned long size;
	DWORD retLen;
	
	retLen = 0;
	
	memset(txbuf, 0, sizeof(txbuf));
	
	if ( pmsg->flags & CANMSG_EXTENDED ) {
		if ( pmsg->flags & CANMSG_RTR ) {
			sprintf( txbuf, "R%8.8lX%i", pmsg->id, pmsg->len );
			pmsg->len = 0; 
		}
		else {
			sprintf( txbuf, "T%8.8lX%i", pmsg->id, pmsg->len );
		}
	}
	else {
		if ( pmsg->flags & CANMSG_RTR ) {
			sprintf( txbuf, "r%3.3lX%i", pmsg->id, pmsg->len );
			pmsg->len = 0; // Just dlc no data for RTR
		}
		else {
			sprintf( txbuf, "t%3.3lX%i", pmsg->id, pmsg->len );
		}
	}
	
	if ( pmsg->len ) {
		char hex[5];
		
		for ( i= 0; i< pmsg->len; i++ ) {
			sprintf( hex, "%2.2X", pmsg->data[i] );
			strcat( txbuf, hex );
		}
	}
	
	// Add CR
	strcat( txbuf, "\r" );	
	size = strlen( txbuf );
	
	// Transmit frame
	if ( !( FT_OK == FT_Write( ftHandle, txbuf, size, &retLen ) ) )
	{ 
		return FALSE;
	}
	
	return TRUE;
}

FT_STATUS writeCommand( FT_HANDLE ftHandle, char *cmd, int cmd_size ) 
{
	FT_STATUS ret;
	DWORD bytes_written = 0;
	ret = FT_Write( ftHandle, cmd, cmd_size, &bytes_written);
	return ret;
}

FT_STATUS readCommand( FT_HANDLE ftHandle, int length, char *cmd, int *cmd_size ) 
{
	FT_STATUS ret;
	DWORD bytes_read;
	DWORD rx_buf_count;
	
	do 
	{
		ret = FT_GetQueueStatus( ftHandle, &rx_buf_count );
		if ( (int) rx_buf_count < length )
		{
			usleep(1000);
		}
	} 
	while ( ( ret == FT_OK ) && ( (int) rx_buf_count < length ) );	
	
	if ( ret == FT_OK )
	{
		ret = FT_Read( ftHandle, cmd, length, &bytes_read );
		if (ret == FT_OK )
		{
			*cmd_size = (int) bytes_read;
		}
	}
	
	return ret;
}


BOOL readFrame( FT_HANDLE ftHandle, CANMsg *msg )
{	
	char rx_buf[100];
	int bytes_read;
	DWORD rx_buf_count;
	int data_offset;
	
	memset( msg, 0, sizeof( CANMsg ) );
	
	FT_GetQueueStatus( ftHandle, &rx_buf_count );
	if ( rx_buf_count == 0 )
	{
		usleep(1000);
	}
	
	if ( readCommand ( ftHandle, 1, rx_buf, &bytes_read ) == FT_OK )
	{
		switch(rx_buf[0])
		{
			case 't':
				if ( readCommand ( ftHandle, 4, rx_buf, &bytes_read ) == FT_OK )
				{
					msg->flags = 0;
					sscanf( rx_buf, "%03x", &msg->id);
					sscanf( &rx_buf[3], "%1x", &data_offset);
					msg->len = rx_buf[3] - '0';
					
					if ( readCommand ( ftHandle, data_offset*2+1, rx_buf, &bytes_read ) == FT_OK )
					{
						int data_byte;
						for ( unsigned int i = 0; i < data_offset; i++ ) {
							sscanf( &rx_buf[i*2], "%2x", &data_byte );
							msg->data[i] = (char)data_byte;
						}
						return TRUE;
					}
				}
				break;
			case 'T':
				break;
			case 'r':
				break;
			case 'R':
				break;
		}
	}
	return FALSE;
}
