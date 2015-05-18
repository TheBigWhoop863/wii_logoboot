#ifndef _BS2STATE_H
#define _BS2STATE_H

#define TYPE_EJECTDISC 1
#define TYPE_REBOOT 2
#define TYPE_RETURN 3
#define TYPE_NANDBOOT 4
#define TYPE_SHUTDOWNSYSTEM 5
#define TYPE_UNKNOWN 0xFF
#define RETURN_TO_MENU 0
#define RETURN_TO_SETTINGS 1
#define RETURN_TO_ARGS 2
#define FLAGS_FLAG1 0x80
#define FLAGS_FLAG2 0x40
#define FLAGS_FLAG3 0x04
#define FLAGS_FLAG4 0x02
#define FLAGS_FLAG5 0x01
// OSRebootSystem => FLAG1 | FLAG3 | (sometimes FLAG2)
// OSShutdownSystemForBS => FLAG1 | FLAG3 | (sometimes FLAG2)
// OSReturnToMenu => FLAG1 | FLAG3 | (sometimes FLAG2)
// BS2SetStateFlags => FLAG1 | FLAG5 | (sometimes FLAG2)
#define FLAGS_STARTWIIGAME 0xC1
#define FLAGS_STARTGCGAME 0x82
#define FLAGS_UNK_1 0x84   // set to 0x84 or 0xC4 by OSReturnToMenu
#define FLAGS_UNK_2 0xC4
#define DISCSTATE_WII 1
#define DISCSTATE_GC 2
#define DISCSTATE_OPEN 3  // "cover open"?
typedef struct {
	u32 checksum;
	u8 flags;
	u8 type;
	u8 discstate;
	u8 returnto;
	u32 unknown[6];
} BS2State;

#endif