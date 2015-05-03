#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <gccore.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

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
	
	IOSInfo *inf = SYSTEMHL_queryIOS();
	printf("  Using IOS %d (Rev. %d)\n",inf->ver,inf->rev);
	printf("  Flag HW_AHBPROT is:  ");
	
	if (HAVE_AHBPROT) // Is HW_AHBPROT set? See systemhl.h, thanks to FTPii project for this useful bit of code!
		printf("SET!!");
	else
		printf("not set.");
	printf("\n\n");

//-------------------------------------------------------------------------------------
	
	printf("Accessing NAND via IPC/IOS call...");
	if( !SYSTEMHL_readSMState() )
	{
		printf("OK!\n");
		
		printf(" --> Flags set are: %#02x\n",state.flags);
		printf(" --> Type set is: %d\n", state.type);
		printf(" --> Disc state is: %d\n", state.discstate);
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
	const DISC_HEADER *header = (DISC_HEADER *)getDataBuf();
	printf("Now reading disc info from DVD drive (libdi)...");
	s32 retcode = SYSTEMHL_checkDVDViaDI();
	if( !retcode )
	{
		printf("OK!\n");
		printf("Data Read From Disc Header: \n");
			printf(" --> Disc ID    : %c %c%c %c\n", (char)header->game_code_ext[0],(char)header->game_code_ext[1],(char)header->game_code_ext[2],(char)header->game_code_ext[3]);
			printf(" --> Maker Code : %c%c\n", (char)header->maker_code[0], (char)header->maker_code[1]);
			printf(" --> Disc Nr    : %d\n",header->disc_nr);
			printf(" --> Disc Ver   : %d\n",header->disc_version);
			printf(" --> Audio Strm : %d\n",header->audio_streaming);
			printf(" --> Strm BufSz : %d\n",header->streaming_buffer_size);
			
				u32 magic = 0;
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
	else
		printf("Error: %d\n",retcode);
	
//-------------------------------------------------------------------------------------


	printf("Press A to proceed with next tests...\n");
	SYSTEMHL_waitForButtonAPress();
	
	SYSTEMHL_ClearScreen();
	
	if(!SYSTEMHL_dumpMemToSDFile())
		printf("Succeeded dumping memory to SD file. Analyse it! ;-)\n");
	else
		printf("Failed dumping memory to SD file.. :-(\n");
	
	printf("\n");
	
	printf("Will now copy header into special memory location..");
	SYSTEMHL_waitForButtonAPress();
	memcpy((u32*)INMEM_DVD_BI2,(const u32*)header,sizeof(DISC_HEADER));
	
	printf("\nWill attempt to write state.dol. ");
	SYSTEMHL_waitForButtonAPress();
	
	if( !SYSTEMHL_writeSMState() )
	{
		printf("\nSuccessfully updated state.dat!\n");
		if( !SYSTEMHL_readSMState() )
		{
			printf(" --> Flags set are: %#02x\n",state.flags);
			printf(" --> Type set is: %d\n", state.type);
			printf(" --> Disc state is: %d\n", state.discstate);
			printf("\n");
		}
	}
			
			
			
			
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