#ifndef _SYSTEM_H
#define _SYSTEM_H

#define _DEBUG

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
s32 SYSTEMHL_checkDVD(void);
s32 SYSTEMHL_ReadStateViaISFS(void);
IOSInfo * SYSTEMHL_QueryIOS(void);
s32 SYSTEMHL_TestNandAccess(u32 *,u32);
#endif //_SYSTEM_H

