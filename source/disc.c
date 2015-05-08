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
#include <ogc/card.h>
#include <ogc/conf.h>
#include <ogc/dvd.h>
#include <ogc/ios.h>
#include <ogc/message.h>
#include <ogc/pad.h>
#include <ogc/stm.h>
#include <ogc/usbstorage.h>
#include <ogc/wiilaunch.h>
#include <wiiuse/wpad.h>
#include <wiiuse/wiiuse.h>
#include <wiilight.h>

#include "disc.h"


// In memory location of disk header, when loaded by IPL (Source: YAGCD, Chap. 4.2.1.4)
discheader_ext* g_discIDext = (discheader_ext*)0x800000f4;
discheader_short* g_discID = (discheader_short*)0x80000000;

s32 copyHeader_e2e(discheader_ext* dst, const discheader_ext* src)
{
	memcpy(dst,src,sizeof(discheader_ext));
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
	return 0;
}