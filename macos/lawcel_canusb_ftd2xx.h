/*
 *  lawcel_canusb_ftd2xx.h
 *  saabopentechproj
 *
 *  Created by Mattias Claesson on 2012-03-01.
 *  Copyright 2012. All rights reserved.
 *
 */

#include "ftd2xx.h"

#define BUF_SIZE 100

#define CANUSB_STATE_NONE  0
#define CANUSB_STATE_MSG   1

// Message flags
#define CANMSG_EXTENDED   0x80 // Extended CAN id
#define CANMSG_RTR        0x40 // Remote frame

// CAN Frame
typedef struct {
	unsigned long id;         // Message id
	unsigned long timestamp;  // timestamp in milliseconds
	unsigned char flags;      // [extended_id|1][RTR:1][reserver:6]
	unsigned char len;        // Frame size (0.8)
	unsigned char data[ 8 ];  // Databytes 0..7
} CANMsg;

void initializeCanUsb();
void getVersionInfo(FT_HANDLE ftHandle);
void getSerialNumber( FT_HANDLE ftHandle );
void setCodeRegister(FT_HANDLE ftHandle);
void setMaskRegister(FT_HANDLE ftHandle);
void setTimeStampOff( FT_HANDLE ftHandle );
BOOL openChannel( FT_HANDLE ftHandle, char* bitrate );
BOOL closeChannel( FT_HANDLE ftHandle );
FT_STATUS setTimeouts( FT_HANDLE ftHandle, ULONG ReadTimeout, ULONG WriteTimeout);
BOOL sendFrame( FT_HANDLE ftHandle, CANMsg *pmsg );
FT_STATUS writeCommand( FT_HANDLE ftHandle, char *cmd, int cmd_size );
FT_STATUS readCommand( FT_HANDLE ftHandle, int length, char *cmd, int *cmd_size );
BOOL readFrame( FT_HANDLE ftHandle, CANMsg *msg );