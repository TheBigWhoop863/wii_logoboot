#ifndef _SYSTEM_H
#define _SYSTEM_H

#define HAVE_AHBPROT (*(vu32*)0xcd800064 == 0xFFFFFFFF ? 1 : 0)
#define _DEBUG

#define BUFFER_SIZE 0x8000

// Source: http://www.wiibrew.org/wiki/Wii_Disc
#define MAGIC_WII "\x5d\x1c\x9e\xa3" //0x5D1C9EA3
#define MAGIC_GC "\xc2\x33\x9f\x3d" //0xC2339F3D

typedef struct {
	u32 checksum;
	u8 flags;
	u8 type;
	u8 discstate;
	u8 returnto;
	u32 unknown[6];
} StateFlags;

typedef struct {
	s8 gamename[4];
	u8 strterm_a;	// Used as terminators (null) so I can read the gamename[] as a C string, null-terminated..
	s8 company[2];
	u8 strterm_b;
	s32 drive_status_a;
	s32 drive_status_b;
} DvdDriveInfo;

typedef struct {
	s32 ver;
	s32 rev;
} IOSInfo;


typedef struct {
    u8 disc_id;
    u8 game_code[2];
    u8 region_code;
    u8 maker_code[2];
    u8 disc_nr;
    u8 disc_version;
    u8 audio_streaming;
    u8 streaming_buffer_size;
    u8 unused[14];
    u8 magic_wii[4];
    u8 magic_gc[4];
    u8 title[64];
    u8 disable_hashes;
    u8 disable_encryption;
} __attribute__((packed)) DISC_HEADER;


static char __testfile[] ATTRIBUTE_ALIGN(32) = "/title/00000001/00000002/data/accesstest.dat";

void SYSTEMHL_getScreenData(void* fb, GXRModeObj* mode);
void SYSTEMHL_setScreenData(void* fb, GXRModeObj* mode);
void SYSTEMHL_init(void);
void SYSTEMHL_ClearScreen(void);
StateFlags *SYSTEMHL_getStateFlags(void);
s32 SYSTEMHL_readStateFlags(void); // Possibly, an IOS with NAND_Access is required for this to work.
							   // TODO: Check if true!
void SYSTEMHL_waitForButtonAPress(void);
DvdDriveInfo *SYSTEMHL_getDVDInfo(void);
s32 SYSTEMHL_checkDVDViaDI(void);
s32 SYSTEMHL_checkDVD(void);
s32 SYSTEMHL_readStateViaISFS(void);
IOSInfo * SYSTEMHL_queryIOS(void);
s32 SYSTEMHL_testNandAccess(u32 *,u32);
u32 SYSTEMHL_initDisc(void);
const u8 *getDataBuf(void);
#endif //_SYSTEM_H

