#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <gcutil.h>
#include <ogc/lwp_queue.h>
#include <gccore.h>	
#include <ctype.h>
#include <unistd.h>
#include <ogc/dvd.h>
#include <di/di.h>

#include "disc.h"
#include "rethandle.h"
#include "systemhl.h"

static u8 __readBuffer[0x200];

// In memory location of disk header, when loaded by IPL (Source: YAGCD, Chap. 4.2.1.4)
discheader_ext* g_discIDext = (discheader_ext*)0x800000f4;
discheader_short* g_discID = (discheader_short*)0x80000000;

s32 copyHeader_e2e(discheader_ext* dst, const discheader_ext* src)
{
	memcpy(dst,src,sizeof(discheader_ext));
	if ( !( memcmp(dst,src,sizeof(discheader_ext)) ) )
		return WII_EINTERNAL;
		
	return 0;
}
s32 copyHeader_e2s(discheader_short* dst, const discheader_ext* src)
{
	memset(dst,0,sizeof(discheader_short));
	u32 magic = src->magic_wii | src->magic_gc;
	memcpy( dst->gamecode, src->game_code_ext, 4);
	memcpy( dst->company, src->maker_code, 2);
	dst->diskID = src->disc_nr;
	dst->version = src->disc_version;
	dst->streaming = src->audio_streaming;
	dst->streamBufSz = src->streaming_buffer_size;
	dst->disc_magic = magic;
	return 0; 
}
s32 copyHeader_s2s(discheader_short* dst, const discheader_short* src)
{
	memcpy(dst,src,sizeof(discheader_short));
	if ( !( memcmp(dst,src,sizeof(discheader_short)) ) )
		return WII_EINTERNAL;
	
	return 0;
}

/* s32 readDiscHeader() 
 *  
 * Uses the DriveInterface (libdi) functions to access and issue commands to DVD Drive 
 * See if those work better than DVD functions, which don't seem to be able to access the drive 
*/ 
s32 readDiscHeader()
{
	if (!SYSTEMHL_querySysState(STATE_DVD_INIT))
		return DVD_ERROR_CANCELED;
		
		
	int retries;
	DI_Mount(); 
 	// "Sort of" timeout. It's locking, so no parallel execution is possible. We are waiting for dvd to finish..
	for(retries=0;retries<8;retries++) 
	{
		if (DI_GetStatus() & (DVD_INIT | DVD_NO_DISC))
			break;
 		
    	sleep(2); 
 	} 
 	if (retries==8)
	    return DVD_ERROR_COVER_CLOSED;
	
	
 	memset(__readBuffer,0,0x200); 
 	if (DI_ReadDVD(__readBuffer, 1, 0))
		return DVD_ERROR_FATAL;
		 	 
			 
 	DI_Close(); 
 	DI_Reset();
	return CheckWIIRetval( copyHeader_e2e(g_discIDext,(discheader_ext*)__readBuffer) );
}