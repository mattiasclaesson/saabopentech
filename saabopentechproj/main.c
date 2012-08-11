/*
  Name: SaabOpenTech
  Copyright: Freeware, use as you please
  Author: Tomi Liljemark (firstname.surname@gmail.com)
  Created: 2007-08-19
  Modified: 2007-09-01
  Compiler: Dev-C++ v4.9.9.2 (shouldn't matter)
            Uses libraries canusbdrv.lib and FTD2XX.lib
  Description: Freeware Saab maintenance terminal
               (tested only on a Saab 9-3 2001 Manual)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "ftd2xx.h"
#include "info_texts.h"
#include "lawcel_canusb_ftd2xx.h"

//#define MAX( a, b )( ( (a) > (b) ) ? (a) : (b) )
//#define MIN( a, b )( ( (a) < (b) ) ? (a) : (b) )
//#define ABS( a )(( (int) (a) < 0 ) ? ((a) ^ 0xffffffff) + 1 : (a) )

#define RELEASE_VERSION "0.40"
#define RELEASE_DATE    "2007-09-01"

#define AUDIO   0
#define SID     1
#define ACC     2
#define MIU     3
#define TWICE   4
#define TRIONIC 5
#define CDC     6   // needs more research...

/* Global constants */
const unsigned char init_table[] = { 0x81, 0x65, 0x98, 0x61, 0x45, 0x11, 0x28 };
const unsigned char unit_table[] = { 0x91, 0x96, 0x98, 0x9A, 0x9B, 0xA1 };
const int reply_table[] = { 0x248, 0x24B, 0x24D, 0x24E, 0x24F, 0x258 };
const int init_reply_table[] = { 0x228, 0x22B, 0x22D, 0x22E, 0x22F, 0x238 };

/* Function prototypes */
BOOL send_msg( FT_HANDLE handle, int id, const unsigned char *data );
int wait_for_msg( FT_HANDLE handle, int id, int timeout, unsigned char *data );
char ask_obd2_value( FT_HANDLE handle, unsigned char id, int *value);
int query_data( FT_HANDLE handle, unsigned char unit, unsigned char data_id, unsigned char *answer);
int init_connection( FT_HANDLE handle, unsigned char unit );
int test_mode( FT_HANDLE handle, unsigned char unit, const unsigned char *test_mode, unsigned char len );
int change_setting( FT_HANDLE handle, unsigned char unit, const unsigned char *setting, unsigned char len );
int change_setting2( FT_HANDLE handle, unsigned char unit, const unsigned char *setting, unsigned char len );
int change_setting3( FT_HANDLE handle, unsigned char unit, const unsigned char *setting, unsigned char len );
int query_info( FT_HANDLE handle, unsigned char unit, unsigned char *answer);
int query_dtc( FT_HANDLE handle, unsigned char unit, unsigned char *answer);
int print_info( const char *string, const struct info_text *info_table);

// Prototypes
BOOL canusbToCanMsg( char * p, CANMsg *pMsg );
BOOL readFrame( FT_HANDLE ftHandle, CANMsg *msg );
BOOL sendFrame( FT_HANDLE ftHandle, CANMsg *pmsg );
FT_STATUS writeCommand( FT_HANDLE ftHandle, char *cmd, int cmd_size );
FT_STATUS readCommand( FT_HANDLE ftHandle, int length, char *cmd, int *cmd_size );


long gettickscount();

// Globals
char gbufferRx[ BUF_SIZE ];
int gnReceivedFrames;

/* Global variables */
unsigned char debug;
FT_HANDLE h = NULL;

void quit()
{  
	printf("ctrl-c\n");
	closeChannel( h );
	printf("\nCAN channel closed.\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	FT_STATUS ftStatus;
    int i, k;
    unsigned char data[8], buf[256];

    printf("SaabOpenTech v%s - Freeware Saab maintenance terminal\r\n"
           "by Tomi Liljemark %s\r\nMattias Claesson (Macos X)\r\n", RELEASE_VERSION, RELEASE_DATE);
	
	signal(SIGINT, quit);		// trap ctrl-c call quit fn 

    if( argc < 2 )
    {
        printf("Usage: SaabOpenTech <func> [param]\n\n"
               "where func is S = Seat belt warning beep (0|1)\n"
               "              L = Trunk locking delay (-,0,1..254 sec)\n"
               "              R = Trunk re-locking delay (-,0,1...254 sec)\n"
               "              V = Trunk speed locking (0,2,4,6,8,10,12,14)\n"
               "              I = System information\n"
               "              T = SID display test\n"
               "              M = Read Trionic information\n"
               "              B = Test brake lights (0|1)\n"
               "              U = Double unlocking with remote (0|1)\n"
               /*"              A = Audio Head Unit (0=divorce,1=marry)\n"*/  );
        return -1;
    }
    
    debug = 0;
    if( argc > 3 )
    {
        if( *argv[3] == 'd' || *argv[3] == 'D' )
        {
            debug = 1;
            printf("Debug info...\n");
        }
    }
    else if( argc > 2 )
    {
        if( *argv[2] == 'd' || *argv[2] == 'D' )
        {
            debug = 1;
            printf("Debug info...\n");
        }
    }
	
	initializeCanUsb();

    printf("Opening CAN channel to Saab I-Bus (47,619 kBit/s)...");
	ftStatus = FT_OpenEx( "CANUSB", FT_OPEN_BY_DESCRIPTION, &h);
	if(ftStatus != FT_OK) {
		printf("FT_OpenEx() failed. rv=%d\n", ftStatus);
		return 1;
	}
	
	setTimeouts( h, 3000, 3000 );       // 3 second read + write timeouts
	setTimeStampOff(h);
	
	//bitrates:
	//  "scb9a" == (47,619kBits/s (0xcb:0x9a)
	//"S6" == 500kbit/s
	if ( !openChannel( h, "scb9a\r" ) ) {
		printf("Failed to open channel\n");
		return FALSE;
	}
	printf("OK channel open\n");
	
	if( wait_for_msg( h, 0, 1000, data ) != 0 )
	{
		printf("Message received from bus, everything seems OK.\n\n");
	}
	else
	{
		printf("Error: Timeout while listening to bus for any messages.\n");
		closeChannel( h );
		printf("\nCAN channel closed.\n");
		return -1;
	}
	
	if( argc > 1 ) 
	{
		usleep(1000000);
		
		if( *argv[1] == 's' || *argv[1] == 'S' )
		{
			printf("\nTWICE - seat belt warning beep\n\n");
        
            // Send initializaton
            init_connection( h, TWICE);
    
            // Check setting
            i = query_data( h, TWICE, 0x02, buf );
            if( i >= 26 )
            {
                printf("Seat belt warning beep %s\n", ( buf[25] & 0x80 ) ? "ON" : "OFF" );
            }
        
            if( argc > 2 )
            {
                usleep(500000); 
                
                // Set seat belt warning beep ON or OFF
                buf[0] = 0x3E;
                buf[1] = ( *argv[2] == '1' ) ? buf[25] | 0x80 : buf[25] & ~0x80;
                change_setting( h, TWICE, buf, 2 );

                usleep(500000);
        
                // Check setting
                i = query_data( h, TWICE, 0x02, buf );
                if( i >= 26 )
                {
                    printf("Seat belt warning beep %s\n", ( buf[25] & 0x80 ) ? "ON" : "OFF" );
                }
            }
    
        }
        if( *argv[1] == 'u' || *argv[1] == 'U' )
        {
            printf("\nTWICE - double unlocking with remote\n\n");
        
            // Send initializaton
            init_connection( h, TWICE);
    
            // Check setting
            i = query_data( h, TWICE, 0x02, buf );
            if( i >= 26 )
            {
                printf("Double unlocking with remote %s\n", ( buf[25] & 0x20 ) ? "ON" : "OFF" );
            }
        
            if( argc > 2 )
            {
                usleep(500000);
                
                // Set double unlocking with remote ON or OFF
                buf[0] = 0x3E;
                buf[1] = ( *argv[2] == '1' ) ? buf[25] | 0x20 : buf[25] & ~0x20;
                change_setting( h, TWICE, buf, 2 );

                usleep(500000);
        
                // Check setting
                i = query_data( h, TWICE, 0x02, buf );
                if( i >= 26 )
                {
                    printf("Double unlocking with remote %s\n", ( buf[25] & 0x20 ) ? "ON" : "OFF" );
                }
            }
    
        }
        else if( *argv[1] == 'l' || *argv[1] == 'L' )
        {
            printf("\nTWICE - Trunk locking delay\n\n");
        
            // Send initializaton
            init_connection( h, TWICE);
    
            // Check setting
            i = query_data( h, TWICE, 0x02, buf );
            if( i >= 32 )
            {
                printf("Trunk locking delay ");
                if( buf[32] == 0x00 ) printf("NO LOCKING\n");
                else if( buf[32] == 0x01 ) printf("IMMEDIATE\n");
                else printf("%d sec\n", buf[32]-1);
            }
        
            if( argc > 2 )
            {
                usleep(500000);
                
                // This is required for some reason
                buf[0] = 0x4E;
                buf[1] = 0x01;
                change_setting( h, TWICE, buf, 2 );

                usleep(500000);

                // Set locking delay
                buf[0] = 0x42;
                buf[1] = ( *argv[2] == '-' ) ? 0x00 : atoi( argv[2] ) + 1;
                change_setting( h, TWICE, buf, 2 );

                usleep(500000);
        
                // Check setting
                i = query_data( h, TWICE, 0x02, buf );
                if( i >= 32 )
                {
                    printf("Trunk locking delay ");
                    if( buf[32] == 0x00 ) printf("NO LOCKING\n");
                    else if( buf[32] == 0x01 ) printf("IMMEDIATE\n");
                    else printf("%d sec\n", buf[32]-1);
                }
            }
    
            
        }
        else if( *argv[1] == 'v' || *argv[1] == 'V' )
        {
            printf("\nTWICE - Trunk speed locking\n\n");
        
            // Send initializaton
            init_connection( h, TWICE);
    
            // Check setting
            i = query_data( h, TWICE, 0x02, buf );
            if( i >= 31 )
            {
                printf("Trunk speed locking ");
                if( buf[30] == 0x88 ) printf("NO LOCKING\n");
                else if( buf[30] >= 0x89 && buf[30] <= 0x8F )
                    printf("%d km/h\n", (buf[30] - 0x88) << 1);
                else
                    printf("UNKNOWN (0x%02X)\n", buf[30] );
            }
        
            if( argc > 2 )
            {
                i = atoi( argv[2] );
                if( i == 0 || i == 2 || i == 4 || i == 6 || i == 8 || i == 10 || i == 12 || i == 14 )
                {
                    i = i/2 + 0x88;
                    usleep(500000);
                    
                    // This is required for some reason
                    // If no speed locking, use 0xAA otherwise 0xAE
                    buf[0] = 0x4C;
                    buf[1] = ( i == 0x88 ) ? 0xAA : 0xAE;
                    change_setting( h, TWICE, buf, 2 );
    
                    usleep(500000);
    
                    // Set locking speed
                    buf[0] = 0x41;
                    buf[1] = i;
                    change_setting( h, TWICE, buf, 2 );
    
                    usleep(500000);
            
                    // Check setting
                    i = query_data( h, TWICE, 0x02, buf );
                    if( i >= 31 )
                    {
                        printf("Trunk speed locking ");
                        if( buf[30] == 0x88 ) printf("NO LOCKING\n");
                        else if( buf[30] >= 0x89 && buf[30] <= 0x8F )
                            printf("%d km/h\n", (buf[30] - 0x88) << 1);
                        else
                            printf("UNKNOWN (0x%02X)\n", buf[30] );
                    }
                }
                else
                {
                    printf("Incorrect parameter value, must be 0,2,4,6,8,10,12 or 14.\n");
                }
            }
        }
        else if( *argv[1] == 'r' || *argv[1] == 'R' )
        {
            printf("\nTWICE - Trunk re-locking delay\n\n");
        
            // Send initializaton
            init_connection( h, TWICE);
    
            // Check setting
            i = query_data( h, TWICE, 0x02, buf );
            if( i >= 33 )
            {
                printf("Trunk re-locking delay ");
                if( buf[33] == 0x00 ) printf("NO LOCKING\n");
                else if( buf[33] == 0x01 ) printf("IMMEDIATE\n");
                else printf("%d sec\n", buf[33]-1);
            }
        
            if( argc > 2 )
            {
                usleep(500000);
                
                // This is required for some reason
                buf[0] = 0x4E;
                buf[1] = 0x01;
                change_setting( h, TWICE, buf, 2 );

                usleep(500000);
                
                // Set locking delay
                buf[0] = 0x43;
                buf[1] = ( *argv[2] == '-' ) ? 0x00 : atoi( argv[2] ) + 1;
                change_setting( h, TWICE, buf, 2 );

                usleep(500000);
        
                // Check setting
                i = query_data( h, TWICE, 0x02, buf );
                if( i >= 33 )
                {
                    printf("Trunk re-locking delay ");
                    if( buf[33] == 0x00 ) printf("NO LOCKING\n");
                    else if( buf[33] == 0x01 ) printf("IMMEDIATE\n");
                    else printf("%d sec\n", buf[33]-1);
                }
            }
        }
        else if( *argv[1] == 'i' || *argv[1] == 'I' )
        {
            printf("\nSystem information\n\n");
        
            init_connection( h, MIU );
            i = query_info( h, MIU, buf );
            for( k = 0; k < i; k++ ) if(buf[k] == 0 ) buf[k] = '_';
            if( i != -1 )
            {
                //if( print_info( buf, miu_info_text ) == -1 )
                    printf("MIU,     len=%d, answer='%s'\n", i, buf);
            }
            else printf("MIU,     no response\n");
        
            init_connection( h, ACC );
            i = query_info( h, ACC, buf );
            for( k = 0; k < i; k++ ) if(buf[k] == 0 ) buf[k] = '_';
            if( i != -1 )
            {
                //if( print_info( buf, acc_info_text ) == -1 )
                    printf("ACC,     len=%d, answer='%s'\n", i, buf);
            }
            else printf("ACC,     no response\n");
        
            init_connection( h, AUDIO );
            i = query_info( h, AUDIO, buf );
            for( k = 0; k < i; k++ ) if(buf[k] == 0 ) buf[k] = '_';
            if( i != -1 )
            {
                //if( print_info( buf, ehu_info_text ) == -1 )
                    printf("AUDIO,   len=%d, answer='%s'\n", i, buf);
            }
            else printf("AUDIO,   no response\n");
        
            init_connection( h, SID );
            i = query_info( h, SID, buf );
            for( k = 0; k < i; k++ ) if(buf[k] == 0 ) buf[k] = '_';
            if( i != -1 )
            {
                //if( print_info( buf, sid_info_text ) == -1 )
                    printf("SID,     len=%d, answer='%s'\n", i, buf);
            }
            else printf("SID,     no response\n");
        
            init_connection( h, TWICE );
            i = query_info( h, TWICE, buf );
            for( k = 0; k < i; k++ ) if(buf[k] == 0 ) buf[k] = '_';
            if( i != -1 )
            {
                //if( print_info( buf, twice_info_text ) == -1 )
                    printf("TWICE,   len=%d, answer='%s'\n", i, buf);
            }
            else printf("TWICE,   no response\n");

            init_connection( h, TRIONIC );
            i = query_info( h, TRIONIC, buf );
            for( k = 0; k < i; k++ ) if(buf[k] == 0 ) buf[k] = '_';
            if( i != -1 )
            {
                //if( print_info( buf, trionic_info_text ) == -1 )
                printf("TRIONIC, len=%d, answer='%s'\n", i, buf);
            }
            else printf("TRIONIC, no response\n");
        }
        else if( *argv[1] == 't' || *argv[1] == 'T' )
        {
            printf("\nSID - Display test\n\n");
        
            // Send initializaton
            init_connection( h, SID);
        
            // Enter display test mode (everything ON)
            buf[0] = 0x36;
            buf[1] = 0x06;
            buf[2] = 0x02;
            test_mode( h, SID, buf, 3 );
            
            printf("All pixels turned ON\n");
            usleep(5000000); 
            
            // Enter display test mode (everything OFF)
            buf[0] = 0x36;
            buf[1] = 0x06;
            buf[2] = 0x01;
            test_mode( h, SID, buf, 3 );
        
            printf("All pixels turned OFF\n");
            usleep(1000000);
        
            // Leave display test mode
            buf[0] = 0x36;
            buf[1] = 0x00;
            test_mode( h, SID, buf, 2 );
            
            printf("Display test ended\n");
        }
        else if( *argv[1] == 'm' || *argv[1] == 'M' )
        {
            printf("Trionic - Read values\n\n");
            
            // Send initializaton
            init_connection( h, TRIONIC);
            
            // Retrieve information
            i = query_data( h, TRIONIC, 0xAC , buf );
            
            // Display information
            if( i == 8 )
            {
                printf("Knocking, cylinder 1        %5d times\n", buf[0] << 8 | buf[1]);
                printf("Knocking, cylinder 2        %5d times\n", buf[2] << 8 | buf[3]);
                printf("Knocking, cylinder 3        %5d times\n", buf[4] << 8 | buf[5]);
                printf("Knocking, cylinder 4        %5d times\n", buf[6] << 8 | buf[7]);
            }
            
            // Retrieve information
            i = query_data( h, TRIONIC, 0xAD , buf );
            
            // Display information
            if( i >= 8 )
            {
                printf("Misfire, cylinder 1         %5d times\n", buf[0] << 8 | buf[1]);
                printf("Misfire, cylinder 2         %5d times\n", buf[2] << 8 | buf[3]);
                printf("Misfire, cylinder 3         %5d times\n", buf[4] << 8 | buf[5]);
                printf("Misfire, cylinder 4         %5d times\n", buf[6] << 8 | buf[7]);
            }
            else
            {
                printf("Received %d bytes!\n", i);
            }

            // Retrieve information
            i = query_data( h, TRIONIC, 0xBD , buf );
            if( i == 2 ) printf("Airmass / Combustion        %5d mg/comb\n", buf[0] << 8 | buf[1] );
            // Retrieve information
            i = query_data( h, TRIONIC, 0x59 , buf );
            if( i == 2 ) printf("Fuel tank pressure          %5.3f kPa\n", (float)(buf[0] << 8 | buf[1])/1000.0 );

/*            
            // Retrieve Diagnostic Trouble Codes
            printf("\nDTC data\n");
            i = query_dtc( h, TRIONIC, buf );
            if( i != -1 )
            {
                for( k = 0; k < i; k++ )
                {
                    printf("0x%02X ", buf[i] );
                }
                printf("\n");
            }
*/
        }
        else if( *argv[1] == 'b' || *argv[1] == 'B' )
        {
            printf("\nSID - Test brake lights\n\n");
        
            // Send initializaton
            init_connection( h, SID);
    
            // Check setting
            i = query_data( h, SID, 0x02, buf );
            if( i >= 17 )
            {
                if( buf[16] == 0xEF || buf[16] == 0xFF )
                {
                    printf("Test brake lights announcement %s\n", (buf[16] == 0xEF ) ? "OFF" : "ON" );
                }
                else
                {
                    printf("Test brake lights announcement UNKNOWN (0x%02X)\n", buf[16] );
                }
            }
    
            if( argc > 2 )
            {
                usleep(500000);
                
                // Set test brake lights ON or OFF
                buf[0] = 0x40;
                buf[1] = 0x01;
                buf[2] = ( *argv[2] == '1' ) ? 0xFF : 0xEF;
                buf[3] = 0x80;
                change_setting2( h, SID, buf, 4 );

                usleep(500000);

                // Check setting
                i = query_data( h, SID, 0x02, buf );
                if( i >= 17 )
                {
                    if( buf[16] == 0xEF || buf[16] == 0xFF )
                    {
                        printf("Test brake lights announcement %s\n", (buf[16] == 0xEF ) ? "OFF" : "ON" );
                    }
                    else
                    {
                        printf("Test brake lights announcement UNKNOWN (0x%02X)\n", buf[16] );
                    }
                }
            }
        }
        else if( *argv[1] == 'a' || *argv[1] == 'A' )
        {
            printf("\nAUDIO - Marry / Divorce\n\n");
        
            // Send initializaton
            init_connection( h, AUDIO);
    
            // Check setting
            i = query_data( h, AUDIO, 0x00, buf );
            if( i >= 10 )
            {
                if( buf[9] == 0x02 || buf[9] == 0x00 )
                {
                    printf("Audio Head Unit is %s\n", (buf[9] == 0x02 ) ? "MARRIED" : "DIVORCED" );
                }
                else
                {
                    printf("Audio Head Unit status UNKNOWN (0x%02X)\n", buf[9] );
                }
            }
    
            if( argc > 2 )
            {
                usleep(500000);
                
                // DIVORCE or MARRY Audio head unit
                buf[2] = ( *argv[2] == '1' ) ? 0x50 : 0x51;
                change_setting3( h, SID, buf, 1 );

                usleep(500000);

                // Check setting
                i = query_data( h, AUDIO, 0x00, buf );
                if( i >= 10 )
                {
                    if( buf[9] == 0x02 || buf[9] == 0x00 )
                    {
                        printf("Audio Head Unit is %s\n", (buf[9] == 0x02 ) ? "MARRIED" : "DIVORCED" );
                    }
                    else
                    {
                        printf("Audio Head Unit status UNKNOWN (0x%02X)\n", buf[9] );
                    }
                }
            }
        }
    }
	
    closeChannel( h );

    return 0;
}

int init_connection( FT_HANDLE handle, unsigned char unit )
{
    unsigned char init[8] = { 0x3F, 0x81, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00 };
    unsigned char data[8];

    
    if( unit > TRIONIC ) return -1;
    
    init[3] = init_table[unit];
    
	printf("send 0x220\n");
    // Send init
    if( !send_msg( handle, 0x220, init ) )
    {
        if( debug ) printf("send 0x220 failed\n");
        return -1;
    }
    
    if( wait_for_msg( handle, init_reply_table[unit], 500, data ) == init_reply_table[unit] )
    {
        return 0;
    }    

    if( debug ) printf("timeout waiting for initialization reply 0x%03X\n", init_reply_table[unit] );

    // Retry init
    if( !send_msg( handle, 0x220, init ) )
    {
        if( debug ) printf("send 0x220 failed\n");
        return -1;
    }
    
    if( wait_for_msg( handle, init_reply_table[unit], 500, data ) == init_reply_table[unit] )
    {
        return 0;
    }    

    if( debug ) printf("timeout #2 waiting for initialization reply 0x%03X\n", init_reply_table[unit] );

    // Retry init second time
    if( !send_msg( handle, 0x220, init ) )
    {
        if( debug ) printf("send 0x220 failed\n");
        return -1;
    }

    if( wait_for_msg( handle, init_reply_table[unit], 500, data ) == init_reply_table[unit] )
    {
        return 0;
    }    

    if( debug ) printf("timeout #3 waiting for initialization reply 0x%03X\n", init_reply_table[unit] );
    return -1;
}

int test_mode( FT_HANDLE handle, unsigned char unit, const unsigned char *test_mode, unsigned char len )
{
    unsigned char test[8] = { 0x40, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00 };
    unsigned char ack[8]   = { 0x40, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00 };
    unsigned char data[8], i;
    int ret;

    
    if( unit > TRIONIC || len > 4 ) return -1;
    
    ret = 0;
    test[1] = unit_table[unit];
    ack[1]  = unit_table[unit];
    test[2] = len + 1;
    
    // Get test mode bytes
    for( i = 0; i < len; i++ )
    {
        test[4+i] = test_mode[i];
    }
    if( debug ) 
    {
        printf( "sending...\n" );
        for( i = 0; i < 8; i++ )
        {
            printf("0x%02X ", test[i]);
        }
        printf("\n");
    }
    // Send test mode request
    if( !send_msg( handle, 0x240, test ) )
    {
        if( debug ) printf("send 0x240 failed\n");
        return -1;
    }
    
    if( wait_for_msg( handle, reply_table[unit], 1000, data ) == reply_table[unit] )
    {
        // Send acknowledgement
        ack[3] = data[0] & 0xBF;
        !send_msg( handle, 0x266, ack );
        if( debug ) 
        {
            for( i = 0; i < 8; i++ )
            {
                printf("0x%02X ", data[i]);
            }
            printf("\n");
        }
/*
        if( data[3] != 0x7F )
        {
            printf("didn't receive 0x7F\n");
            for( i = 0; i < 8; i++ )
            {
                printf("0x%02X ", data[i]);
            }
        }
*/
    }    
    else
    {
        if( debug ) printf("timeout waiting for first reply\n");
        ret = -1;
    }

    if( wait_for_msg( handle, reply_table[unit], 300, data ) == reply_table[unit] )
    {
        // Send acknowledgement
        ack[3] = data[0] & 0xBF;
        send_msg( handle, 0x266, ack );
        if( debug ) 
        {
            for( i = 0; i < 8; i++ )
            {
                printf("0x%02X ", data[i]);
            }
            printf("\n");
        }
/*
        if( data[4] != test_mode[0] || data[5] != test_mode[1] )
        {
            printf("didn't receive 0x%02X and 0x02X\n", test_mode[0], test_mode[1] );
            for( i = 0; i < 8; i++ )
            {
                printf("0x%02X ", data[i]);
            }
        }
*/
    }    
    else
    {
        if( debug ) printf("timeout waiting for second reply\n");
        ret = -1;
    }
    
    return ret;
}

int change_setting( FT_HANDLE handle, unsigned char unit, const unsigned char *setting, unsigned char len )
{
    unsigned char msg[8] = { 0x40, 0x00, 0x00, 0x3B, 0x00, 0x00, 0x00, 0x00 };
    unsigned char ack[8] = { 0x40, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00 };
    unsigned char data[8], i;
    int ret;

    
    if( unit > TRIONIC || len > 4 ) return -1;
    
    ret = 0;
    msg[1] = unit_table[unit];
    ack[1]  = unit_table[unit];
    msg[2] = len + 1;
    
    // Get setting bytes
    for( i = 0; i < len; i++ )
    {
        msg[4+i] = setting[i];
    }
    if( debug ) 
    {
        printf( "sending...\n" );
        for( i = 0; i < 8; i++ )
        {
            printf("0x%02X ", msg[i]);
        }
        printf("\n");
    }
    // Send change setting request
    if( !send_msg( handle, 0x240, msg ) )
    {
        if( debug ) printf("send 0x240 failed\n");
        return -1;
    }
    
    if( wait_for_msg( handle, reply_table[unit], 1000, data ) == reply_table[unit] )
    {
        // Send acknowledgement
        ack[3] = data[0] & 0xBF;
        send_msg( handle, 0x266, ack );
        if( debug ) 
        {
            for( i = 0; i < 8; i++ )
            {
                printf("0x%02X ", data[i]);
            }
            printf("\n");
        }
    }    
    else
    {
        if( debug ) printf("timeout waiting for reply\n");
        ret = -1;
    }

    if( wait_for_msg( handle, reply_table[unit], 300, data ) == reply_table[unit] )
    {
        // Send acknowledgement
        ack[3] = data[0] & 0xBF;
        send_msg( handle, 0x266, ack );
        if( debug ) 
        {
            for( i = 0; i < 8; i++ )
            {
                printf("0x%02X ", data[i]);
            }
            printf("\n");
        }
    }    
    
    return ret;
}

int change_setting2( FT_HANDLE handle, unsigned char unit, const unsigned char *setting, unsigned char len )
{
    unsigned char premsg1[8] = { 0x41, 0x00, 0x08, 0x34, 0x16, 0x00, 0x00, 0x00 };
    unsigned char premsg2[8] = { 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00 };
    unsigned char msg[8] = { 0x40, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00 };
    unsigned char msg2[8] ={ 0x40, 0x00, 0x01, 0x37, 0x00, 0x00, 0x00, 0x00 };
    unsigned char ack[8] = { 0x40, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00 };
    unsigned char data[8], i;
    int ret;

    
    if( unit > TRIONIC || len > 4 ) return -1;
    
    ret = 0;
    msg[1] = unit_table[unit];
    msg2[1] = unit_table[unit];
    premsg1[1] = unit_table[unit];
    premsg2[1] = unit_table[unit];
    ack[1]  = unit_table[unit];
    msg[2] = len + 1;
    
    
    // send premsg
    if( debug ) 
    {
        printf( "sending...\n" );
        for( i = 0; i < 8; i++ )
        {
            printf("0x%02X ", premsg1[i]);
        }
        printf("\n");
    }

    if( !send_msg( handle, 0x240, premsg1 ) )
    {
        if( debug ) printf("send 0x240 failed\n");
        return -1;
    }

    if( debug ) 
    {
        for( i = 0; i < 8; i++ )
        {
            printf("0x%02X ", premsg2[i]);
        }
        printf("\n");
    }

    if( !send_msg( handle, 0x240, premsg2 ) )
    {
        if( debug ) printf("send 0x240 failed\n");
        return -1;
    }
    
    if( wait_for_msg( handle, reply_table[unit], 1000, data ) == reply_table[unit] )
    {
        // Send acknowledgement
        ack[3] = data[0] & 0xBF;
        send_msg( handle, 0x266, ack );
        if( debug ) 
        {
            for( i = 0; i < 8; i++ )
            {
                printf("0x%02X ", data[i]);
            }
            printf("\n");
        }
    }    
    else
    {
        if( debug ) printf("timeout waiting for reply (0x%03X)\n", reply_table[unit]);
        ret = -1;
    }

    usleep(100000);

    // send actual msg
    for( i = 0; i < len; i++ )
    {
        msg[4+i] = setting[i];
    }
    if( debug ) 
    {
        printf( "sending...\n" );
        for( i = 0; i < 8; i++ )
        {
            printf("0x%02X ", msg[i]);
        }
        printf("\n");
    }
    // Send change setting request
    if( !send_msg( handle, 0x240, msg ) )
    {
        if( debug ) printf("send 0x240 failed\n");
        return -1;
    }
    
    if( wait_for_msg( handle, reply_table[unit], 1000, data ) == reply_table[unit] )
    {
        // Send acknowledgement
        ack[3] = data[0] & 0xBF;
        send_msg( handle, 0x266, ack );
        if( debug ) 
        {
            for( i = 0; i < 8; i++ )
            {
                printf("0x%02X ", data[i]);
            }
            printf("\n");
        }
    }    
    else
    {
        if( debug ) printf("timeout waiting for reply\n");
        ret = -1;
    }

    if( wait_for_msg( handle, reply_table[unit], 1000, data ) == reply_table[unit] )
    {
        // Send acknowledgement
        ack[3] = data[0] & 0xBF;
        send_msg( handle, 0x266, ack );
        if( debug ) 
        {
            for( i = 0; i < 8; i++ )
            {
                printf("0x%02X ", data[i]);
            }
            printf("\n");
        }
    }    
    else
    {
        if( debug ) printf("timeout waiting for reply\n");
        ret = -1;
    }

    usleep(100000);

    // send postmsg  
    if( debug ) 
    {
        printf( "sending...\n" );
        for( i = 0; i < 8; i++ )
        {
            printf("0x%02X ", msg2[i]);
        }
        printf("\n");
    }

    if( !send_msg( handle, 0x240, msg2 ) )
    {
        if( debug ) printf("send 0x240 failed\n");
        return -1;
    }
    
    if( wait_for_msg( handle, reply_table[unit], 1000, data ) == reply_table[unit] )
    {
        // Send acknowledgement
        ack[3] = data[0] & 0xBF;
        send_msg( handle, 0x266, ack );
        if( debug ) 
        {
            for( i = 0; i < 8; i++ )
            {
                printf("0x%02X ", data[i]);
            }
            printf("\n");
        }
    }    
    else
    {
        if( debug ) printf("timeout waiting for reply\n");
        ret = -1;
    }
/*
    if( wait_for_msg( handle, reply_table[unit], 300, data ) == reply_table[unit] )
    {
        // Send acknowledgement
        ack[3] = data[0] & 0xBF;
        send_msg( handle, 0x266, ack );
        for( i = 0; i < 8; i++ )
        {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
    }    
*/    
    return ret;
}

int change_setting3( FT_HANDLE handle, unsigned char unit, const unsigned char *setting, unsigned char len )
{
    unsigned char msg[8] = { 0x40, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00 };
    unsigned char ack[8] = { 0x40, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00 };
    unsigned char data[8], i;
    int ret;

    
    if( unit > TRIONIC || len > 4 ) return -1;
    
    ret = 0;
    msg[1] = unit_table[unit];
    ack[1]  = unit_table[unit];
    msg[2] = len + 1;
    
    // Get setting bytes
    for( i = 0; i < len; i++ )
    {
        msg[4+i] = setting[i];
    }
    if( debug ) 
    {
        printf( "sending...\n" );
        for( i = 0; i < 8; i++ )
        {
            printf("0x%02X ", msg[i]);
        }
        printf("\n");
    }
    // Send change setting request
    if( !send_msg( handle, 0x240, msg ) )
    {
        if( debug ) printf("send 0x240 failed\n");
        return -1;
    }
    
    if( wait_for_msg( handle, reply_table[unit], 1000, data ) == reply_table[unit] )
    {
        // Send acknowledgement
        ack[3] = data[0] & 0xBF;
        send_msg( handle, 0x266, ack );
        if( debug ) 
        {
            for( i = 0; i < 8; i++ )
            {
                printf("0x%02X ", data[i]);
            }
            printf("\n");
        }
    }    
    else
    {
        if( debug ) printf("timeout waiting for reply\n");
        ret = -1;
    }

    if( wait_for_msg( handle, reply_table[unit], 300, data ) == reply_table[unit] )
    {
        // Send acknowledgement
        ack[3] = data[0] & 0xBF;
        send_msg( handle, 0x266, ack );
        if( debug ) 
        {
            for( i = 0; i < 8; i++ )
            {
                printf("0x%02X ", data[i]);
            }
            printf("\n");
        }
    }    
    
    return ret;
}


int query_data( FT_HANDLE handle, unsigned char unit, unsigned char data_id, unsigned char *answer)
{
    unsigned char data[8], length, i;
    int rcv_length;
    unsigned char query[8] = { 0x40, 0x00, 0x02, 0x21, 0x00, 0x00, 0x00, 0x00 };
    unsigned char ack[8]   = { 0x40, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00 };
    

    if( unit > TRIONIC ) return -1;
   
    query[1] = unit_table[unit];
    ack[1]   = unit_table[unit];
    // If data_id is zero, decrease length field
    if( data_id != 0x00 ) query[4] = data_id;
    else query[2] = 0x01;
    
    data[0] = 0x00;
    rcv_length = 0;
    
    // Send query
    if( !send_msg( handle, 0x240, query ) )
    {
        if( debug ) printf("send 0x240 failed\n");
        return -1;
    }

    // Read response messages
    while( data[0] != 0x80 && data[0] != 0xC0 )
    {
        if( wait_for_msg( handle, reply_table[unit], 2000, data ) == reply_table[unit] )
        {
            if( data[0] & 0x40 )
            {
                if( data[2] > 2 ) length = data[2] - 2;   // subtract two non-payload bytes
                else length = 0;
                if( --length > 0 ) *answer++ = data[5], rcv_length++;
                if( --length > 0 ) *answer++ = data[6], rcv_length++;
                if( --length > 0 ) *answer++ = data[7], rcv_length++;
            }
            else
            {
                for( i = 0; i < 6; i++ )
                {
                    *answer++ = data[2+i];
                    length--;
                    rcv_length++;
                    if( length == 0 ) i = 6;
                }
            }
            // Send acknowledgement
            ack[3] = data[0] & 0xBF;
            send_msg( handle, 0x266, ack );
        }
        else
        {
            // Timeout
            if( debug ) printf("Timeout waiting for 0x%03X.\n", reply_table[unit]);
            return -1;
        }
    }

    // Set end of string
    *answer = 0;
    return rcv_length;
}


int query_dtc( FT_HANDLE handle, unsigned char unit, unsigned char *answer)
{
    unsigned char data[8], length, i;
    int rcv_length;
    unsigned char query[8] = { 0x40, 0x00, 0x04, 0x18, 0x00, 0xFF, 0x00, 0x00 };
    unsigned char ack[8]   = { 0x40, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00 };
    

    if( unit > TRIONIC ) return -1;
   
    query[1] = unit_table[unit];
    ack[1]   = unit_table[unit];
    data[0] = 0x00;
    rcv_length = 0;
    
    // Send query
    if( !send_msg( handle, 0x240, query ) )
    {
        if( debug ) printf("send 0x240 failed\n");
        return -1;
    }

    // Read response messages
    while( data[0] != 0x80 && data[0] != 0xC0 )
    {
        if( wait_for_msg( handle, reply_table[unit], 2000, data ) == reply_table[unit] )
        {
            if( data[0] & 0x40 )
            {
                if( data[2] > 2 ) length = data[2] - 2;   // subtract two non-payload bytes
                else length = 0;
                if( --length > 0 ) *answer++ = data[5], rcv_length++;
                if( --length > 0 ) *answer++ = data[6], rcv_length++;
                if( --length > 0 ) *answer++ = data[7], rcv_length++;
            }
            else
            {
                for( i = 0; i < 6; i++ )
                {
                    *answer++ = data[2+i];
                    length--;
                    rcv_length++;
                    if( length == 0 ) i = 6;
                }
            }
            // Send acknowledgement
            ack[3] = data[0] & 0xBF;
            send_msg( handle, 0x266, ack );
        }
        else
        {
            // Timeout
            if( debug ) printf("Timeout waiting for 0x%03X.\n", reply_table[unit]);
            return -1;
        }
    }

    // Set end of string
    *answer = 0;
    return rcv_length;
}

int query_info( FT_HANDLE handle, unsigned char unit, unsigned char *answer)
{
    unsigned char data[8], length, i;
    int rcv_length;
    unsigned char query[8] = { 0x40, 0x00, 0x02, 0x1A, 0x80, 0x00, 0x00, 0x00 };
    unsigned char ack[8]   = { 0x40, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00 };
    

    if( unit > TRIONIC ) return -1;
   
    query[1] = unit_table[unit];
    ack[1]   = unit_table[unit];
    data[0] = 0x00;
    rcv_length = 0;
    
    // Send query
    if( !send_msg( handle, 0x240, query ) )
    {
        if( debug ) printf("send 0x240 failed\n");
        return -1;
    }

    // Read response messages
    while( data[0] != 0x80 && data[0] != 0xC0 )
    {
        if( wait_for_msg( handle, reply_table[unit], 2000, data ) == reply_table[unit] )
        {
            if( data[0] & 0x40 )
            {
                if( data[2] > 2 ) length = data[2] - 2;   // subtract two non-payload bytes
                else length = 0;
                if( --length > 0 ) *answer++ = data[5], rcv_length++;
                if( --length > 0 ) *answer++ = data[6], rcv_length++;
                if( --length > 0 ) *answer++ = data[7], rcv_length++;
            }
            else
            {
                for( i = 0; i < 6; i++ )
                {
                    *answer++ = data[2+i];
                    length--;
                    rcv_length++;
                    if( length == 0 ) i = 6;
                }
            }
            // Send acknowledgement
            ack[3] = data[0] & 0xBF;
            send_msg( handle, 0x266, ack );
        }
        else
        {
            // Timeout
            if( debug ) printf("Timeout waiting for 0x%03X.\n", reply_table[unit]);
            return -1;
        }
    }

    // Set end of string
    *answer = 0;
    return rcv_length;
}


char ask_obd2_value( FT_HANDLE handle, unsigned char id, int *value)
{
    unsigned char data[8], i;
    unsigned char query[8] = { 0x40, 0xA1, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00 };
    unsigned char ack[8]   = { 0x40, 0xA1, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00 };
    char value_ok;
   
    query[4] = id;
    data[0] = 0x00;
    value_ok = 0;
    
    // Send query to Trionic
    send_msg( handle, 0x240, query );

    // Read response messages
    while( data[0] != 0x80 && data[0] != 0xC0 )
    {
        if( wait_for_msg( handle, 0x258, 1000, data ) == 0x258 )
        {
            for( i = 0; i < 8; i++ ) printf("0x%02X ", data[i]);
            printf("- ");
            if( data[0] == 0x80 && data[2] == id )
            {
                *value = data[3] << 8 | data[4];
                value_ok = 1;
                // TODO: insert something like below here also
            }
            else if( data[0] == 0xC0 && data[4] == id )
            {
                 if( data[2] == 0x05 )
                 {
                     *value = data[5] << 16 | data[6] << 8 | data[7];
                 }
                 else if( data[2] == 0x04 )
                 {
                     *value = data[5] << 8 | data[6];
                 }
                 else if( data[2] == 0x03 )
                 {
                      *value = data[5];
                 }
                 else
                 {
                     printf("Unknown size!\n");
                 }
                 value_ok = 1;
            }
            // Send acknowledgement
            ack[3] = data[0] & 0xBF;
            send_msg( handle, 0x266, ack );
        }
        else
        {
            // Timeout
            printf("Timeout waiting for 0x258.\n");
            return value_ok;
        }
    }

    return value_ok;
}


BOOL send_msg( FT_HANDLE handle, int id, const unsigned char *data )
{
    CANMsg msg;
    
    msg.id = id;
    msg.len = 8;
    msg.flags = 0;
    msg.data[0] = data[0];
    msg.data[1] = data[1];
    msg.data[2] = data[2];
    msg.data[3] = data[3];
    msg.data[4] = data[4];
    msg.data[5] = data[5];
    msg.data[6] = data[6];
    msg.data[7] = data[7];
    
    return sendFrame( handle, &msg );
}

int wait_for_msg( FT_HANDLE handle, int id, int timeout, unsigned char *data )
{
    CANMsg msg;
    int timeout_temp;
    char not_received, got_it;
    long dwStart;
	BOOL ret;
	
	setTimeouts( handle, timeout, timeout );
    
    dwStart = gettickscount();
    timeout_temp = timeout;
    msg.id = 0x0;
    not_received = 1;
    got_it = 0;
    while( not_received == 1 )
    {
        ret = TRUE;
        while( ret )
        {
			ret = readFrame ( handle, &msg );
			
            if( ret )
            {
                if( msg.id == id || id == 0 )
                {
                    not_received = 0;
                    got_it = 1;
                    *(data+0) = msg.data[0];
                    *(data+1) = msg.data[1];
                    *(data+2) = msg.data[2];
                    *(data+3) = msg.data[3];
                    *(data+4) = msg.data[4];
                    *(data+5) = msg.data[5];
                    *(data+6) = msg.data[6];
                    *(data+7) = msg.data[7];
                    break;
                }
            }
		}
        if( (gettickscount() - dwStart) > timeout )
		{
			not_received = 0;	
		}
    }
    return msg.id;
}

int print_info( const char *string, const struct info_text *info_table)
{
    unsigned char i, k, str_counter, len_sum;

    i = 0;
    str_counter = 0;
    len_sum = 0;
    while( (info_table+i)->len != 0 )
    {
        len_sum += (info_table+i)->len;
        i++;
    }        
    
    if( len_sum != strlen(string) )
    {
        return -1;
    }
    
    while( (info_table+i)->name[0] != 0 )
    {
        printf( "%-30s : ", (info_table+i)->name );
        for( k = 0; k < (info_table+i)->len; k++ )
        {
            if( string[str_counter+k] == 0 )
            {
                printf("\n");
                return 1;
            }
            if( string[str_counter+k] != 0 )
                putchar( string[str_counter+k] );
            else
                putchar( ' ' );
        }
        str_counter += k;
        printf("\n");
        i++;
    }
    
    return 0;
}



long gettickscount()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	long ticks = now.tv_sec * 1000l;
	ticks += now.tv_usec / 1000l;
	return ticks;
}