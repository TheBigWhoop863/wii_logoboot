#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <gccore.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include <di/di.h>
#include <wiiuse/wpad.h>

#include "systemhl.h"
#include "rethandle.h"
#include "disc.h"


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
	
	IOSInfo *inf = SYSTEMHL_queryIOS();
	printf("  Using IOS %d (Rev. %d)\n",inf->ver,inf->rev);
	printf("  Flag HW_AHBPROT is:  ");
	if (HAVE_AHBPROT) // Is HW_AHBPROT set? See systemhl.h, thanks to FTPii project for this useful bit of code!
		printf("SET!!");
	else
		printf("not set.");
	printf("\n\n");

//-------------------------------------------------------------------------------------
	/*
	printf("Accessing NAND via IPC/IOS call...");
	if( !SYSTEMHL_readSMState() )
	{
		printf("OK!\n");
		
		printf(" --> Flags set are: %#02x\n",state.flags);
		printf(" --> Type set is: %d\n", state.type);
		printf(" --> Disc state is: %d\n", state.discstate);
		printf("\n");
	}
	*/
	
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
	u32 magic = 0;
	const discheader_ext* header = (discheader_ext *)getDataBuf();
	printf("Now reading disc info from DVD drive (libdi)...\n");
	s32 DVDstate = SYSTEMHL_checkDVDViaDI();
	if (DVDstate == 0)
	{
		printf("Data Read From Disc Header: \n");
		printf(" --> Disc ID    : %c %c%c %c\n", (char)header->game_code_ext[0],(char)header->game_code_ext[1],(char)header->game_code_ext[2],(char)header->game_code_ext[3]);
		printf(" --> Maker Code : %c%c\n", (char)header->maker_code[0], (char)header->maker_code[1]);
		printf(" --> Disc Nr    : %d\n",header->disc_nr);
		printf(" --> Disc Ver   : %d\n",header->disc_version);
		printf(" --> Audio Strm : %d\n",header->audio_streaming);
		printf(" --> Strm BufSz : %d\n",header->streaming_buffer_size);
			magic |= header->magic_wii;
			magic |= header->magic_gc;
		printf(" --> Magic Word : %08X", magic);
			if( magic == (u32)MAGIC_WII )
				printf("   (WII)");
			if( magic == (u32)MAGIC_GC )
				printf("   (GameCube)");
			printf("\n");
		
		printf(" --> Game Title: %s\n",(char *)header->title);		
	}
	else if (DVDstate == DVD_NO_DISC)
	{
		printf("No Disc in drive. Exiting to SYSMENU...");
		WPAD_Shutdown();
		sleep(3); 
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
	}
	else
		CheckDVDRetval( DVDstate );
	
//-------------------------------------------------------------------------------------

	if (magic == (u32)MAGIC_GC)
		printf("Press B to attempt autoboot of GC disc\n");
			
	printf("Press A to return to HBC; HOME to return to System Menu.\n");
	
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
		if ( (magic == (u32)MAGIC_GC) && ((pressed_wii & WPAD_BUTTON_B) | (pressed_wii & WPAD_CLASSIC_BUTTON_B) | (pressed_gc & PAD_BUTTON_B)) )
		{
			SYSTEMHL_bootGCDisc();
		}
		VIDEO_WaitVSync();
	}
	
	return 0;// Just because it expects a return value. This point in code is never reached.
}