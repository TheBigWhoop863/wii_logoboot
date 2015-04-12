#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <ogc/es.h>
#include <ogc/isfs.h>
#include <ogc/ipc.h>
#include <ogc/ios.h>
#include <ogc/dvd.h>
#include <ogc/wiilaunch.h>	
#include <fat.h>
#include <gcmodplay.h>
#include <wiiuse/wpad.h>

#include "systemhl.h"
#include "iospatch.h"



//---------------------------------------------
int main(int argc, char **argv)
//---------------------------------------------
{
	int isfs_ret;

	SYSTEMHL_init(); // Init console and controllers
	printf("\x1b[2;0H");
	printf("Init Complete! Hello, World!\n\n");
	VIDEO_WaitVSync();
	
	IOSInfo *inf = SYSTEMHL_queryIOS();
	printf("  Using IOS: %d v.%d\n",inf->ver,inf->rev);
	printf("  Flag HW_AHBPROT is:  ");
	
	if (HAVE_AHBPROT) // Is HW_AHBPROT set? See iospatch.c
		printf("SET!!");
	else
		printf("not set.");
	printf("\n\n");


/*
	printf("Now patching in-mem IOS..");
	u32 retcount= IOSPATCH_Apply();
	if(retcount) 
		printf("OK!! (%d)\n\n",retcount);
	else
		printf("ERR! :(\n\n");
*/	


//-------------------------------------------------------------------------------------
	
	/*
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
	*/
	
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
	
	
/***************************************************
* Test for NAND r/w access.
*
* STATUS: 2015-03-30 crashes (DSI) on creating file
* Jacopo Viti - Mar.2015
***************************************************/

/*
	printf("Read/Write Test follows..\nPress A when ready.\n");
		SYSTEMHL_waitForButtonAPress();

//	printf("Attempting to write+read in NAND file (ISFS)...");
	u32 buf[512];
	u32 size = sizeof(u32)*512;
	
	s32 test = SYSTEMHL_TestNandAccess(&buf[0],size);
	
	if (test<0)
		printf("\nERRCODE: %d",test);
	else
	{
		if( (buf[0]==0xBEBEBEBE) && (buf[63]==0xBEBEBEBE) && (buf[511]==0xBEBEBEBE))
		{
			printf("\nAll tests OK! : ");
			printf("(%#08x), (%#08x), (%#08x)\n",buf[0],buf[63],buf[511]);
		}
		else
		{
			printf("\n Data Mismatch!\n");
			printf("Read:     (%#08x), (%#08x), (%#08x)\nExpected: (0xBEBEBEBE), (0xBEBEBEBE), (0xBEBEBEBE)\n",buf[0],buf[63],buf[511]);
		}
	}
*/


/***************************************************
* Test for DVD direct hw access.
*
* STATUS: 2015-03-20 freezes probably on DVD_Mount()
* Jacopo Viti - Mar.2015
***************************************************/
// Test A: libOGC / DVD
/*	
	printf("Now reading disc info from DVD drive...");
	if( !SYSTEMHL_checkDVD() )
	{
		printf("OK!\n");
		DvdDriveInfo * dvd = SYSTEMHL_getDVDInfo();
		printf(" --> Drive status before mount was: %d\n",dvd->drive_status_a);
		if (dvd->drive_status_b == 0)
			printf("Drive is empty or didn't respond to mount attempt\n");
		else
		{
			printf(" --> Game Title is: %s\n", (char *)&(dvd->gamename));
			printf(" --> Company ID is: %s\n", (char *)&(dvd->company));
			printf(" --> Drive status after mount was: %d\n",dvd->drive_status_b);
		}
		printf("\n");
	}
*/

//Test B: libdi / DI
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

	printf("Press A to return to loader..\n");
	SYSTEMHL_waitForButtonAPress();
	
	// Quitting gracefully..
	/*
	WPAD_Shutdown();
	usleep(500); 
	SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
	*/
	
	exit(0); // Should return to loader!
	
	return 0;// Just because it expects a return value. This point in code is not reached anyway.
}
