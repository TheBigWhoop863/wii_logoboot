#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <gccore.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
/*
#include <malloc.h>
#include <ogc/es.h>
#include <ogc/isfs.h>
#include <ogc/ipc.h>
#include <ogc/ios.h>
#include <ogc/dvd.h>
#include <ogc/wiilaunch.h>
#include <gcmodplay.h>
*/	
#include <fat.h>
#include <wiiuse/wpad.h>

#include "systemhl.h"
#include "rethandle.h"


//---------------------------------------------------------------------------------
static void power_cb()
//---------------------------------------------------------------------------------
{
	STM_ShutdownToStandby();
}

//---------------------------------------------------------------------------------
static void reset_cb()
//---------------------------------------------------------------------------------
{
	STM_RebootSystem();
}

//---------------------------------------------
int main(int argc, char **argv)
//---------------------------------------------
{
	// Set callbacks to the physical "Power" and "Reset" button on the console, so they actually do something.
	SYS_SetPowerCallback (power_cb);
    SYS_SetResetCallback (reset_cb);
	
	SYSTEMHL_init(); // Init console and controllers
		
	printf("\x1b[2;0H");
	printf("Init Complete! Hello, World!\n\n");
	VIDEO_WaitVSync();
	
	IOSInfo *inf = SYSTEMHL_queryIOS();
	printf("  Using IOS: %d v.%d\n",inf->ver,inf->rev);
	printf("  Flag HW_AHBPROT is:  ");
	
	if (HAVE_AHBPROT) // Is HW_AHBPROT set? See systemhl.h, thanks to FTPii project for this useful bit of code!
		printf("SET!!");
	else
		printf("not set.");
	printf("\n\n");


// Not necessary if I have HW_AHBPROT. If I DON'T have HW_AHBPROT, patching will fail anyway.
/*
	printf("Now patching in-mem IOS..");
	u32 retcount= IOSPATCH_Apply();
	if(retcount) 
		printf("OK!! (%d)\n\n",retcount);
	else
		printf("ERR! :(\n\n");
*/	


//-------------------------------------------------------------------------------------
	
	
	printf("Accessing NAND via IPC/IOS call...");
	if( !SYSTEMHL_readStateFlags() )
	{
		printf("OK!\n");
		StateFlags * sflags = SYSTEMHL_getStateFlags(); 
		printf(" --> Flags set are: %#02x\n",sflags->flags);
		printf(" --> Type set is: %d\n", sflags->type);
		printf(" --> Disc state is: %d\n", sflags->discstate);
		printf("\n");
	}
	
	/*
	printf("Accessing NAND via ISFS call...");
	isfs_ret = SYSTEMHL_readStateViaISFS();
	switch(isfs_ret)
	{
	case 0:
		printf("OK!\n");
		StateFlags * sflags = SYSTEMHL_getStateFlags(); 
		printf(" --> Flags set are: %#02x\n",sflags->flags);
		printf(" --> Type set is: %d\n", sflags->type);
		printf(" --> Disc state is: %d\n", sflags->discstate);
		printf("\n");
		break;
	case 1:
		printf("ERR!\nCannot Init ISFS.\n");
		break;
	case 2:
		printf("ERR!\nCannot Mount ISFS.\n");
		break;
	case 3:
		printf("ERR!\nCannot Read File. (Path invalid?)\n");
		break;
	}	
	*/
	


/***************************************************
* Test for DVD direct hw access.
*
* STATUS: 2015-04-11 libdi works!
* Jacopo Viti - Mar.2015
***************************************************/
	int i;
	printf("Now reading disc info from DVD drive (libdi)...");
	s32 retcode = SYSTEMHL_checkDVDViaDI();
	if( !retcode )
	{
		printf("OK!\n");
		const DISC_HEADER *header = (DISC_HEADER *)getDataBuf();
		printf("Data Read From Disc Header: \n");
			printf(" --> Disc ID    : %c\n", (char)header->disc_id);
			printf(" --> Game Code  : ");
				for(i=0;i<2;i++)
				{	
					printf("%c",(char)header->game_code[i]);
				}
				printf("\n");
			printf(" --> Region Code: %c\n",(char)header->region_code);	
			printf(" --> Maker Code : ");
				for(i=0;i<2;i++)
				{	
					printf("%c",header->maker_code[i]);
				}
				printf("\n");
			printf(" --> Disc Nr    : %d\n",header->disc_nr);
			printf(" --> Disc Ver   : %d\n",header->disc_version);
			printf(" --> Audio Strm : %d\n",header->audio_streaming);
			printf(" --> Strm BufSz : %d\n",header->streaming_buffer_size);
			printf(" --> Magic Word : 0x");
				for(i=0;i<4;i++)
				{
					printf("%x",header->magic_wii[i]);
				}
				printf(" | 0x");
				for(i=0;i<4;i++)
				{
					printf("%x",header->magic_gc[i]);
				}
				printf("  ");
				if( !memcmp(header->magic_wii, MAGIC_WII, 4) )
					printf("(WII)");
				if( !memcmp(header->magic_gc, MAGIC_GC, 4) )
					printf("(GameCube)");
				printf("\n");
			printf(" --> Game Title: %s\n",(char *)&header->title); //CAREFUL! May not have a terminator...
		
	}
	else
		printf("Error: %d\n",retcode);
	
//-------------------------------------------------------------------------------------

	printf("Press A to return to HBC; HOME to return to System Menu.\n");
	//SYSTEMHL_waitForButtonAPress();
	
	while(1)
	{
		// Refresh latest controller states
		WPAD_ScanPads(); // Wiimote + Connected devices (Nunchuck, Classic controller, etc..)
		PAD_ScanPads();  // Gamecube Pads
		
		u16 pressed_wii = WPAD_ButtonsDown(0) | WPAD_ButtonsDown(1) | WPAD_ButtonsDown(2) | WPAD_ButtonsDown(3);
		u16 pressed_gc = PAD_ButtonsDown(0) | PAD_ButtonsDown(1) | PAD_ButtonsDown(2) | PAD_ButtonsDown(3);
		
		if ( (pressed_wii & WPAD_BUTTON_HOME) | (pressed_wii & WPAD_CLASSIC_BUTTON_HOME) )
		{
			// Quitting gracefully..
			WPAD_Shutdown();
			usleep(500); 
			SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
		}
		if ( (pressed_wii & WPAD_BUTTON_A) | (pressed_wii & WPAD_CLASSIC_BUTTON_A) | (pressed_gc & PAD_BUTTON_A) )
		{
			//Should return to loader, but in Priiloader v0.8 beta returns to HBC instead..
			exit(0);
		}
		
		VIDEO_WaitVSync();
	}
	
	return 0;// Just because it expects a return value. This point in code is never reached.
}