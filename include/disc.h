#ifndef _DISC_H
#define _DISC_H

// Source: http://www.wiibrew.org/wiki/Wii_Disc
#define MAGIC_WII 0x5D1C9EA3 
#define MAGIC_GC  0xC2339F3D

typedef struct {
	u8 game_code_ext[4];  //u8 disc_id;
						  //u8 game_code[2];
						  //u8 region_code;
    u8 maker_code[2];
    u8 disc_nr;
    u8 disc_version;
    u8 audio_streaming;
    u8 streaming_buffer_size;
    u8 unused[0x0e];
    u32 magic_wii;
    u32 magic_gc;
    u8 title[64];
    u8 disable_hashes;
    u8 disable_encryption;
	u8 padding[30];
} __attribute__((packed)) discheader_ext;


// Based on: YAGCD, section 4.2.1.1.1
typedef struct {
	u8 gamecode[4];
	u8 company[2];
	u8 diskID;
	u8 version;
	u8 streaming;
	u8 streamBufSz;
	u8 padding[0x0f];
	u32 disc_magic;
} __attribute__((packed)) discheader_short;

//Global Variables
extern discheader_ext* g_discIDext;
extern discheader_short* g_discID;

//Disc functions
s32 copyHeader_e2e(discheader_ext* dst, const discheader_ext* src);
s32 copyHeader_e2s(discheader_short* dst, const discheader_ext* src);
s32 copyHeader_s2s(discheader_short* dst, const discheader_short* src);

s32 readDiscHeader();
#endif