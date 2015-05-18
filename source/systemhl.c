/*  SYSTEMHL.C
*  This is a collection of high-level functions handling most of the
*  lower level library calls (libOGC, libDI, libWiiLight, etc.)
*
*  Jacopo - 11/03/2015
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>	
#include <sys/unistd.h>

#include <fat.h>
#include <sys/dirent.h>


// Wii stuff
#include <gccore.h>
#include <ogcsys.h>
#include <ogc/lwp_watchdog.h>
#include <ogc/es.h>
#include <ogc/isfs.h>
#include <ogc/ipc.h>
#include <ogc/ios.h>
#include <ogc/dvd.h>
#include <ogc/wiilaunch.h>
#include <di/di.h>
#include <wiiuse/wpad.h>
#include <wiilight.h>

#include "systemhl.h"
#include "rethandle.h"
#include "disc.h"

static char __statefile[] ATTRIBUTE_ALIGN(32) = "/title/00000001/00000002/data/state.dat";
static tikview __gcviews ATTRIBUTE_ALIGN(32);

static IOSInfo __iosinfo ATTRIBUTE_ALIGN(32);
BS2State g_BS2State ATTRIBUTE_ALIGN(32);
static u32 __sysState = 0;

void *xfb = NULL;
GXRModeObj *rmode = NULL;


/********************************************
* Static functions
********************************************/
// Hidden functions, copied from wiilaunch.c
// These are not accessible from outside wiilaunch.c (static), hence copy/paste
static u32 __CalcChecksum(u32 *buf, int len)
{
	u32 sum = 0;
	int i;
	len = (len/4);

	for(i=1; i<len; i++)
		sum += buf[i];

	return sum;
}


static void __SetChecksum(void *buf, int len)
{
	u32 *p = (u32*)buf;
	p[0] = __CalcChecksum(p, len);
}


static int __ValidChecksum(void *buf, int len)
{
	u32 *p = (u32*)buf;
	return p[0] == __CalcChecksum(p, len);
}
//-------------------------------------------------------




// Debug printf function
// TODO: Restructure to use vprintf, easier to pass args
inline void dbgprint(const char *format)
{
	#ifdef _DEBUG
	if( SYSTEMHL_querySysState(STATE_CONSOLE_INIT) )
		printf(format);
	#endif
}

inline u32 SYSTEMHL_querySysState(u32 state)
{
	return (__sysState & state); 
}

void SYSTEMHL_init()
{
	// Init subsystems by calling appropriate init()
	// Subsystems APIs provided by libOGC
	
	// One exception is the DriveInterface API (libdi)
	// It MUST be init'ed as the very first thing.
	if ( !SYSTEMHL_querySysState(STATE_DVD_INIT) )
	{
		CheckIOSRetval( DI_Init() );
		__sysState |= STATE_DVD_INIT;
	}
	
	if ( !SYSTEMHL_querySysState(STATE_VIDEO_INIT) )
	{
		VIDEO_Init();
		__sysState |= STATE_VIDEO_INIT;
	}
	
	if( !SYSTEMHL_querySysState(STATE_AUDIO_INIT) )
	{
		AUDIO_Init(NULL);
		__sysState |= STATE_AUDIO_INIT;
	}
	
	if ( !SYSTEMHL_querySysState(STATE_PAD_INIT) )
	{
		WPAD_Init(); // Start WiiMote support
		PAD_Init();  // Start GC Pad support

		__sysState |= STATE_PAD_INIT;
	}
	
	if( !SYSTEMHL_querySysState(STATE_WIILIGHT_INIT) )
	{
		WIILIGHT_Init();
		__sysState |= STATE_WIILIGHT_INIT;
	}

	
	if( !SYSTEMHL_querySysState(STATE_CONSOLE_INIT) )
	{
		// Obtain the preferred video mode from the system
		// This will correspond to the settings in the Wii menu
		rmode = VIDEO_GetPreferredMode(NULL);

		// Allocate memory for the display in the uncached region
		xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
		
		// Initialise the console, required for printf
		console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
		
		// Set up the video registers with the chosen mode
		VIDEO_Configure(rmode);
		
		// Tell the video hardware where our display memory is
		VIDEO_SetNextFramebuffer(xfb);
		
		// Make the display visible
		VIDEO_SetBlack(FALSE);

		// Flush the video register changes to the hardware
		VIDEO_Flush();

		// Wait for Video setup to complete
		VIDEO_WaitVSync();
		if(rmode->viTVMode&VI_NON_INTERLACE)
			VIDEO_WaitVSync();
		
		__sysState |= STATE_CONSOLE_INIT;
	}
}



void SYSTEMHL_ClearScreen()
{
	if( !( SYSTEMHL_querySysState(STATE_CONSOLE_INIT) ) )
		return;
	
	VIDEO_WaitVSync();
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
}


/* SYSTEMHL_waitForButtonAPress()
* 
* This is a locking function, entering an infinite loop until button A is pressed on any controller.
* Both Wii and GC controllers are supported.
* 
* Jacopo - 11/03/2015
*/
void SYSTEMHL_waitForButtonAPress()
{
	if( !SYSTEMHL_querySysState(STATE_PAD_INIT) )
		return;
	
	u16 A_pressed = 0;
	dbgprint("Press A to continue..");
	while(!A_pressed)
	{
		// Call WPAD_ScanPads, this reads the latest controller states
		WPAD_ScanPads();
	
		// Same thing, only for GC controllers
		PAD_ScanPads();
	
		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released.
		// Button state from all four possible controllers (0-3) are bitwise OR-ed, 
		// so it detects if A was pressed on ANY controller.
		u16 pressed = WPAD_ButtonsDown(0);
		  pressed |= WPAD_ButtonsDown(1);
		  pressed |= WPAD_ButtonsDown(2);
		  pressed |= WPAD_ButtonsDown(3);
		
		u16 pressedGC = PAD_ButtonsDown(0);
		  pressedGC |= PAD_ButtonsDown(1);
		  pressedGC |= PAD_ButtonsDown(2);
		  pressedGC |= PAD_ButtonsDown(3);
		  
		A_pressed = (pressed & WPAD_BUTTON_A) | (pressed & WPAD_CLASSIC_BUTTON_A) | (pressedGC & PAD_BUTTON_A); 
		// Wait for next video frame
		VIDEO_WaitVSync();
	}
}


s32 SYSTEMHL_checkDVDViaDI()
{
	return readDiscHeader();
}


/* SYSTEMHL_readBF2State()
*
* This replicates a lower level function in wiilaunch (part of libOGC :) )
* Flags are read from special files in the NAND filesystem
* and put in a global struct.
*/
s32 SYSTEMHL_readBF2State()
{
	// This function is copy/pasted from wiilaunch.c (part of libOGC)
	// It refreshes data in one global struct, which can then be accessed.
	
	int fd;
	int ret;
		
	fd = IOS_Open(__statefile,IPC_OPEN_READ);
	if(fd < 0) {
		memset(&g_BS2State,0,sizeof(BS2State));
		return WII_EINTERNAL;
	}
	ret = IOS_Read(fd, &g_BS2State, sizeof(BS2State));
	IOS_Close(fd);
	if(ret != sizeof(BS2State)) {
		memset(&g_BS2State,0,sizeof(BS2State));
		return WII_EINTERNAL;
	}
	if(!__ValidChecksum(&g_BS2State, sizeof(BS2State))) {
		memset(&g_BS2State,0,sizeof(BS2State));
		return WII_ECHECKSUM;
	}
	return 0;
}




const IOSInfo* SYSTEMHL_queryIOS()
{
	memset(&__iosinfo,0,sizeof(__iosinfo));
	__iosinfo.ver = CheckIOSRetval( IOS_GetVersion() );
	__iosinfo.rev = CheckIOSRetval( IOS_GetRevision() );
	return &__iosinfo;
}



s32 SYSTEMHL_dumpMemToSDFile()
{
	if(!fatInitDefault())
	{
		dbgprint("Cannot initialize SD Card !!\n");
		fatUnmount(0);
		return -1;
	}
	DIR *root = opendir("sd:/");
	if(!root)
	{
		dbgprint("Cannot OPEN SD Card !!\n");
		fatUnmount(0);
		return -1;
	}
	closedir(root);
	
	FILE *fp = fopen("sd:/memdump.bin","wb");
	if (fp == NULL)
	{
		dbgprint("Cannot open file for writing.\n");
		fatUnmount(0);
		return -1;
	}
	
	fwrite((const void*)0x80000000,1024,1024,fp);
	fclose(fp);
	
	fatUnmount(0);
	return 0;
}


void SYSTEMHL_bootGCDisc()
{
	copyHeader_e2s( g_discID, (discheader_ext*)g_discIDext );
	
	// This address range is related to: PI - Processor Interface (Interrupt Interface)
	// It's most likely a Hardware Register into the PPC, its usage being specific to the Wii (undocumented use in GameCube)
	// Source: YAGCD (Yet Another GameCube Documentation), section 5.4
	*(volatile unsigned int *)0xCC003024 |= 7;	// I don't know what this does.
	
	CheckESRetval(ES_GetTicketViews(BC, &__gcviews, 1));
	CheckESRetval(ES_LaunchTitle(BC, &__gcviews));
	
	printf("\nHalting Execution.. (Boot failed?)");
	VIDEO_WaitVSync();
	CheckWIIRetval(WII_EINTERNAL);
}



void SYSTEMHL_exitToSysMenu()
{
	WPAD_Shutdown();
	__sysState &= !STATE_PAD_INIT;
	usleep(500); 
	SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}

void SYSTEMHL_exitToLoader()
{
	exit(0);
}
/*
s32 SYSTEMHL_writeBF2State()
{
	const volatile discheader_ext* dh = g_discIDext; // Disc data must already be in memory prior to calling this function
	
	int fd;
	int ret;
	u32 magic = 0;
	magic |= dh->magic_wii;
	magic |= dh->magic_gc;
	// Fill struct with appropriate data
	state.returnto = RETURN_TO_MENU;
	if ( magic == MAGIC_WII ){
		dbgprint("\nWill write BS2 state: Autoboot WII Disc.\n");
		state.type = TYPE_RETURN;
		state.flags = FLAGS_STARTWIIGAME;
		state.discstate = DISCSTATE_WII;
	}
	else if( magic == MAGIC_GC ){
		dbgprint("\nWill write BS2 state: Autoboot GC Disc.\n");
		state.type = TYPE_RETURN;
		state.flags = FLAGS_STARTGCGAME;
		state.discstate = DISCSTATE_GC;
	}
	else {
		dbgprint("\nWill write BS2 state: Eject disc.\n");
		state.type = TYPE_EJECTDISC;
		state.flags = FLAGS_FLAG1 | FLAGS_FLAG3 ; //OSReturnToMenu
		state.discstate = DISCSTATE_OPEN;
	}
	
	SYSTEMHL_waitForButtonAPress();
	
	__SetChecksum(&state,sizeof(state));
	if(!__ValidChecksum(&state, sizeof(state))) {
		memset(&state,0,sizeof(state));
		return WII_ECHECKSUM;
	}
	
	fd = IOS_Open(__statefile,IPC_OPEN_RW);
	if(fd < 0)
		return WII_EINTERNAL;
	
	dbgprint("Opened file..");	

	ret = IOS_Write(fd, &state, sizeof(state));
	IOS_Close(fd);
	if(ret != sizeof(state))
		return WII_EINTERNAL;
	
	dbgprint("Write OK..");
	return 0;
}
*/
////////////////////////////////////////////////////////

