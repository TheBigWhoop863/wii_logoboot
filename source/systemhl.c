/*  SYSTEMHL.C
*  This is a collection of high-level functions handling most of the
*  lower level library calls (libOGC, DVD, WiiLight, etc.)
*
*  Jacopo - 11/03/2015
*/


#include <stdio.h>
#include <gccore.h>
#include <string.h>	
#include <unistd.h>

// Wii stuff
#include <ogc/es.h>
#include <ogc/isfs.h>
#include <ogc/ipc.h>
#include <ogc/ios.h>
//#include <ogc/dvd.h>
#include <ogc/wiilaunch.h>
#include <di/di.h>
#include <wiiuse/wpad.h>

//#include <ogcsys.h>
//#include <ogc/lwp_watchdog.h>
//#include <stdlib.h>

#include "systemhl.h"
#include "rethandle.h"

static char __statefile[] ATTRIBUTE_ALIGN(32) = "/title/00000001/00000002/data/state.dat";

//static DvdDriveInfo driveinfo ATTRIBUTE_ALIGN(32);
static IOSInfo iosinfo ATTRIBUTE_ALIGN(32);
StateFlags state ATTRIBUTE_ALIGN(32);

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static u32 __SYSTEMHL_init = 0;
static bool dvdDriveReady = false;

static u8 read_buffer[BUFFER_SIZE];
// Debug printf function
static inline void dbgprint(const char *text)
{
	#ifdef _DEBUG
	if(__SYSTEMHL_init)
		printf(text);
	#endif
	return;
}

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


void SYSTEMHL_init()
{
	// Init subsystems by calling appropriate init()
	// Subsystems APIs provided by libOGC
	
	// One exception is the DriveInterface API (libdi)
	// It MUST be called as the very first thing.
	
	int statusDIinit = -1;
	statusDIinit = DI_Init();
	// WARNING: If a disc is never inserted at this point, program might crash at the end (?). Known libdi bug.
	
	VIDEO_Init();
	AUDIO_Init(NULL);
	
	// Start WiiMote support
	WPAD_Init();
	
	// Start GC Pad support
	PAD_Init();
	
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
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	
	__SYSTEMHL_init=1;
	
	if(statusDIinit!=0)
		dbgprint("WARNING: There probably was a problem while performing DI_Init()\n");

}

void* SYSTEMHL_getFramebuffer()
{
	return xfb;
}

GXRModeObj* SYSTEMHL_getVideoMode()
{
	return rmode;
}

void SYSTEMHL_setVideo(void* fb, GXRModeObj* mode)
{
	xfb=fb;
	rmode=mode;
}

void SYSTEMHL_ClearScreen()
{
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
/* SYSTEMHL_readSMState()
*
* This replicates a lower level function in wiilaunch (part of libOGC :) )
* Flags are read from special files in the NAND filesystem
* and put in a global struct.
*/
s32 SYSTEMHL_readSMState()
{
	/* This function is copy/pasted from wiilaunch.c (part of libOGC)
	* It refreshes data in one global struct, which can then be accessed.
	*/
	
	int fd;
	int ret;

	fd = IOS_Open(__statefile,IPC_OPEN_READ);
	if(fd < 0) {
		memset(&state,0,sizeof(state));
		return WII_EINTERNAL;
	}

	ret = IOS_Read(fd, &state, sizeof(state));
	IOS_Close(fd);
	if(ret != sizeof(state)) {
		memset(&state,0,sizeof(state));
		return WII_EINTERNAL;
	}
	if(!__ValidChecksum(&state, sizeof(state))) {
		memset(&state,0,sizeof(state));
		return WII_ECHECKSUM;
	}
	return 0;
}

s32 SYSTEMHL_writeSMState()
{
	const volatile DISC_HEADER* dh = (void*)0x8000000; // Disc data must already be in memory prior to calling this function
	
	int fd;
	int ret;
	u32 magic,m2;
	
	memcpy(&magic,(const void*)dh->magic_wii,4);
	memcpy(&m2,(const void*)dh->magic_gc,4);
	magic |= m2;
	// Fill struct with appropriate data
	state.returnto = RETURN_TO_MENU;
	if ( !memcmp(&magic,MAGIC_WII,4) ){
		dbgprint("\nWill write BS2 state: Autoboot WII Disc.\n");
		state.type = TYPE_RETURN;
		state.flags = FLAGS_STARTWIIGAME;
		state.discstate = DISCSTATE_WII;
	}
	else if( !memcmp(&magic,MAGIC_GC,4) ){
		dbgprint("\nWill write BS2 state: Autoboot GC Disc.\n");
		state.type = TYPE_RETURN;
		state.flags = FLAGS_STARTGCGAME;
		state.discstate = DISCSTATE_GC;
	}
	else {
		dbgprint("\nWill write BS2 state: Eject disc.\n");
		state.type = TYPE_EJECTDISC;
		state.flags = FLAGS_FLAG1 | FLAGS_FLAG3 ;
		state.discstate = DISCSTATE_OPEN;
	}
	
	SYSTEMHL_waitForButtonAPress();
	
	__SetChecksum(&state,sizeof(state));
	if(!__ValidChecksum(&state, sizeof(state))) {
		memset(&state,0,sizeof(state));
		return WII_ECHECKSUM;
	}
	
	fd = IOS_Open(__statefile,IPC_OPEN_WRITE);
	if(fd < 0)
		return WII_EINTERNAL;

	ret = IOS_Write(fd, &state, sizeof(state));
	IOS_Close(fd);
	if(ret != sizeof(state))
		return WII_EINTERNAL;
	
	return 0;
}

/* Different philosophies:
* IOS_Open is a call to the IOS to get a specific system resource.
* Call is notified to IOS using using IPC (InterProcess Communication);
* this is the official, correct way to do stuff. IOS restrictions may apply.
*
* IOS_Open works for different resources: can be a file on NAND, can be a device (/dev), etc.
* It is possible to use an IOS_Open call to get access to the DVD drive, for example : IOS_Open("/dev/di",...);
* For this reason, IOS_Read exist, but not IOS_Write (some resources might not be writable).
* 
* ISFS calls, on the other hand, still use IPC, but everything is specific to the NAND filesystem.
* You MUST use ISFS calls (or raw ioctls opcodes :) ) if you need to write to the NAND filesystem.
*/
/*
s32 SYSTEMHL_readStateViaISFS()
{
	// Quick retcodes compendium (to be rewritten! Debug only!!)
	// 1: cannot Init
	// 2: Cannot Mount
	// 3: Cannot open file (path invalid??) or other file err.
	
	int fd;
	int ret;
	
	memset(&stateflags,0xBB,sizeof(stateflags)); // TODO: Remove

	if (ISFS_Initialize() != IPC_OK) 
	{
		ISFS_Deinitialize();
		return 1;
	}
	
	fd = ISFS_Open(__stateflags,ISFS_OPEN_READ);
	if(fd < 0) {
		//memset(&stateflags,0,sizeof(stateflags));
		//return WII_EINTERNAL;
		return 2;
	}

	ret = ISFS_Read(fd, &stateflags, sizeof(stateflags));
	ISFS_Close(fd);
	if(ret != sizeof(stateflags)) {
		//memset(&stateflags,0,sizeof(stateflags));
		//return WII_EINTERNAL;
		return 3;
	}
	if(!__ValidChecksum(&stateflags, sizeof(stateflags))) {
		//memset(&stateflags,0,sizeof(stateflags));
		return WII_ECHECKSUM;
	}
	
	ISFS_Deinitialize();
	
	return 0;
}
*/
/*
StateFlags* SYSTEMHL_getStateFlags()
{
	return &stateflags;
}
*/
/*
DvdDriveInfo* SYSTEMHL_getDVDInfo()
{
	return &driveinfo;
}
*/
IOSInfo* SYSTEMHL_queryIOS()
{
	memset(&iosinfo,0,sizeof(iosinfo));
	iosinfo.ver = IOS_GetVersion();
	iosinfo.rev = IOS_GetRevision();
	return &iosinfo;
}

/*
s32 SYSTEMHL_testNandAccess(u32 *buf,u32 size)
{
	//u32 buf[512]; // Data buffer , 2K size
	
	printf("Entering SYSTEMHL_TestNandAccess(u32 *,u32)\n");
		SYSTEMHL_waitForButtonAPress();

	int fd;
	s32 ret;
	
	memset(&buf,0xBE,size); // Fill the buffer
	
	// Init ISFS subsystem
	printf("Now Initializing ISFS..");
		SYSTEMHL_waitForButtonAPress();

	if (ISFS_Initialize() != IPC_OK) 
	{
		ISFS_Deinitialize();
		return 1;
	}
	printf("OK!\n");
	
	
	// Create target file
	printf("Creating file..");
		SYSTEMHL_waitForButtonAPress();
		
	ret = ISFS_CreateFile(__testfile,0,3,3,3);
	if (ret!=0)
	{
		ISFS_Deinitialize();
		return ret;
	}
	printf("OK!\n");	
	
	// Write into file
	printf("Writing file..");
		SYSTEMHL_waitForButtonAPress();
		
	fd = ISFS_Open(__testfile,ISFS_OPEN_WRITE);
	if(fd < 0) 
		return WII_EINTERNAL;
	
	ret = ISFS_Write(fd,buf,size);
	if (ret!=0)
	{
		ISFS_Deinitialize();
		return -101;
	}
	printf("OK!\n");
	
	// Close file, read file (verify)	
	printf("Reopening file..");
		SYSTEMHL_waitForButtonAPress();
		
	ISFS_Close(fd);
	fd = 0;
	memset(&buf,0,size);
	
	fd = ISFS_Open(__testfile,ISFS_OPEN_READ);
	if(fd < 0) 
	{
		ISFS_Deinitialize();	
		return WII_EINTERNAL;
	}
	printf("OK!\n");
	printf("Read file..");
		SYSTEMHL_waitForButtonAPress();
		
	ret = ISFS_Read(fd,buf,size);
	if (ret!=0)
	{	
		ISFS_Deinitialize();
		return -102;
	}
	
	printf("OK!\n");
	
	printf("Finishing SYSTEMHL_testNandAccess()..");
		SYSTEMHL_waitForButtonAPress();
		
	ISFS_Close(fd);
	fd = 0;
	
	// Delete file
	ISFS_Delete(__testfile);
	
	ISFS_Deinitialize();
	
	printf("OK!\n");
	return 0;
}
*/


 u32 SYSTEMHL_initDisc()
{
	u32 query_attempts = 0;
	DI_Mount();
	while (DI_GetStatus() & (DVD_INIT | DVD_NO_DISC)) 
	{
		query_attempts++;
		if (query_attempts>8)
			break;
		else
			sleep(2);
	}
	dvdDriveReady = (query_attempts>8) ? false : true;	// Very ugly
	return 0;
}

/* s32 SYSTEMHL_checkDVDViaDI()
* 
* Uses the DriveInterface (libdi) functions to access and issue commands to DVD Drive
* See if those work better than DVD functions, which don't seem to be able to access the drive
*/
s32 SYSTEMHL_checkDVDViaDI()
{
	// As per the README of libdi, DI_Init() MUST be called as the very first thing in my program!
	// Therefore I am moving the call as the first thing in SYSTEMHL_Init();
	// DI_Init();
	//
	SYSTEMHL_initDisc();
	if( !dvdDriveReady ) return -1;
	
	memset(&read_buffer,0,BUFFER_SIZE);
	
	if (DI_ReadDVD(read_buffer, 1, 0)) return -4;
    DISC_HEADER *header = (DISC_HEADER *)read_buffer;
	
	DI_Close();
	dvdDriveReady = false;
	DI_Reset();
	
    if (memcmp(header->magic_wii, MAGIC_WII, 4) * memcmp(header->magic_gc, MAGIC_GC, 4)) // should be 0 if either one matches
		return -100; //Err: Invalid Data?
	
	return 0;
}

const u8* getDataBuf()
{
	return read_buffer;
}
////////////////////////////////////////

