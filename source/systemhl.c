/*  SYSTEMHL.C
*  This is a collection of high-level functions handling most of the
*  lower level library calls (libOGC, DVD, WiiLight, etc.)
*
*  Jacopo - 11/03/2015
*/


#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <string.h>
#include <malloc.h>	
#include <wiiuse/wpad.h>
#include <ogc/es.h>
#include <ogc/isfs.h>
#include <ogc/ipc.h>
#include <ogc/ios.h>
#include <ogc/dvd.h>
#include <ogc/wiilaunch.h>

#include "systemhl.h"

static char __stateflags[] ATTRIBUTE_ALIGN(32) = "/title/00000001/00000002/data/state.dat";

static StateFlags stateflags ATTRIBUTE_ALIGN(32);
static DvdDriveInfo driveinfo ATTRIBUTE_ALIGN(32);
static IOSInfo iosinfo ATTRIBUTE_ALIGN(32);

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static u32 __SYSTEMHL_init = 0;


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
}

void SYSTEMHL_getScreenData(void* fb, GXRModeObj* mode)
{
	fb=xfb;
	mode=rmode;
}

void SYSTEMHL_setScreenData(void* fb, GXRModeObj* mode)
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
		  
		A_pressed = (pressed & WPAD_BUTTON_A) | (pressedGC & PAD_BUTTON_A); 
		// Wait for next video frame
		VIDEO_WaitVSync();
	}
}
/* SYSTEMHL_readStateFlags()
*
* This replicates a lower level function in wiilaunch (part of libOGC :) )
* Flags are read from special files in the NAND filesystem
* and put in static, global specialized structs.
*/
s32 SYSTEMHL_readStateFlags()
{
	/* This function is copy/pasted from wiilaunch.c (part of libOGC)
	* It refreshes data in one global struct, which can then be accessed.
	*/
	
	int fd;
	int ret;

	fd = IOS_Open(__stateflags,IPC_OPEN_READ);
	if(fd < 0) {
		memset(&stateflags,0,sizeof(stateflags));
		return WII_EINTERNAL;
	}

	ret = IOS_Read(fd, &stateflags, sizeof(stateflags));
	IOS_Close(fd);
	if(ret != sizeof(stateflags)) {
		memset(&stateflags,0,sizeof(stateflags));
		return WII_EINTERNAL;
	}
	if(!__ValidChecksum(&stateflags, sizeof(stateflags))) {
		memset(&stateflags,0,sizeof(stateflags));
		return WII_ECHECKSUM;
	}
	return 0;
}

s32 SYSTEMHL_ReadStateViaISFS()
{
	// Quick retcodes compendium (to be rewritten! Debug only!!)
	// 1: cannot Init
	// 2: Cannot Mount
	// 3: Cannot open file (path invalid??) or other file err.
	
	int fd;
	int ret;
	
	if (ISFS_Initialize() != IPC_OK) 
	{
		ISFS_Deinitialize();
		return 1;
	}
	
	fd = ISFS_Open(__stateflags,ISFS_OPEN_READ);
	if(fd < 0) {
		memset(&stateflags,0,sizeof(stateflags));
		//return WII_EINTERNAL;
		return 2;
	}

	ret = ISFS_Read(fd, &stateflags, sizeof(stateflags));
	ISFS_Close(fd);
	if(ret != sizeof(stateflags)) {
		memset(&stateflags,0,sizeof(stateflags));
		//return WII_EINTERNAL;
		return 3;
	}
	if(!__ValidChecksum(&stateflags, sizeof(stateflags))) {
		memset(&stateflags,0,sizeof(stateflags));
		return WII_ECHECKSUM;
	}
	
	ISFS_Deinitialize();
	
	return 0;
}

s32 SYSTEMHL_checkDVD()
{
	u32 i;
	memset(&driveinfo,0,sizeof(driveinfo));
	
	DVD_Init();
	#ifdef _DEBUG
		if(__SYSTEMHL_init)
			printf('\n   Init OK!..');
	#endif
	
	driveinfo.drive_status_a = DVD_GetDriveStatus(); // Before mount
	#ifdef _DEBUG
		if(__SYSTEMHL_init)
			printf('\n   GetDriveStatus OK!..');
	#endif
	
	if( DVD_Mount() )
	{
		dvddiskid *currDisk = DVD_GetCurrentDiskID();
		// Copy arrays
		for( i=0;i<4;i++)
			driveinfo.gamename[i] = currDisk->gamename[i];
		for( i=0;i<2;i++)
			driveinfo.company[i] = currDisk->company[i];
		driveinfo.drive_status_b = DVD_GetDriveStatus(); // After mount
	}
	DVD_Reset(0);
		
		
	return 0;
}

StateFlags *SYSTEMHL_getStateFlags()
{
	return &stateflags;
}
DvdDriveInfo *SYSTEMHL_getDVDInfo()
{
	return &driveinfo;
}

IOSInfo *SYSTEMHL_QueryIOS()
{
	memset(&iosinfo,0,sizeof(iosinfo));
	iosinfo.ver = IOS_GetVersion();
	iosinfo.rev = IOS_GetRevision();
	return &iosinfo;
}

s32 SYSTEMHL_TestNandAccess(u32 *buf,u32 size)
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
	
	printf("Finishing SYSTEMHL_TestNandAccess()..");
		SYSTEMHL_waitForButtonAPress();
		
	ISFS_Close(fd);
	fd = 0;
	
	// Delete file
	ISFS_Delete(__testfile);
	
	ISFS_Deinitialize();
	
	printf("OK!\n");
	return 0;
}
////////////////////////////////////////

