/*********************************************
* 			RETHANDLE.C
*
* This is a slightly modified version of rethandle.c
* which is part of the WiiMU project.
* All credit goes to the original authors.
*
* Mod by : TheBigWhoop863
*********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <ogcsys.h>
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

#include "rethandle.h"
#include "systemhl.h"

/*
Infinite "blinking" loop, to signal error code.
The idea is to blink the error code (libwiilight) even if we don't have an initialized console output.
At this point, execution is stopped and your Wii must be reset.

Make sure handlers for the RESET and POWER button were set !!
*/

static void __printerrmsg(const char* errmsg,...)
{
	va_list vl;
	va_start(vl,errmsg);  // TODO: This might work, but C standard defines that argument "errmsg" CANNOT be of ARRAY type..Problem?
	if(SYSTEMHL_querySysState(STATE_CONSOLE_INIT))
		vprintf(errmsg,vl);
	va_end(vl);
}

void RetvalFail(int badness)
{
	if (badness==0)
	{
	 // Error of badness 0 mean program can continue
	 // Only a warning is given
		if( SYSTEMHL_querySysState(STATE_WIILIGHT_INIT) )
		{
			WIILIGHT_SetLevel(127);
			WIILIGHT_TurnOn();
			sleep(3);
			WIILIGHT_TurnOff();
			WIILIGHT_SetLevel(0);
		}
		else
			sleep(3);

	 return;
	}
	
	if( SYSTEMHL_querySysState(STATE_WIILIGHT_INIT) )
	{
		int i;
		WIILIGHT_SetLevel(32);
		WIILIGHT_TurnOn();
		while(1)
		{
			for (i=0;i<badness;i++)
			{
				WIILIGHT_SetLevel(255);
				usleep(500000);   // Half a second
				WIILIGHT_SetLevel(32);
				usleep(500000);
			}
			sleep(6);
		}
	}
	else
	{
        SYS_ResetSystem(SYS_SHUTDOWN,0,0);
		while(1); //Halt execution and freeze system
	}
	
}

int CheckESRetval(int retval)
{
	switch(retval)
	{
		case ES_EINVAL:
			__printerrmsg("FAILED! (ES: Invalid Argument)\n");
			RetvalFail(3);
			break;

		case ES_ENOMEM:
			__printerrmsg("FAILED! (ES: Out of memory)\n");
			RetvalFail(2);
			break;

		case ES_ENOTINIT:
			__printerrmsg("FAILED! (ES: Not Initialized)\n");
			RetvalFail(3);
			break;

		case ES_EALIGN:
			__printerrmsg("FAILED! (ES: Not Aligned)\n");
			RetvalFail(3);
			break;
		default:
			if(retval<0)
			{
				__printerrmsg("FAILED! (ES: Unknown error %d)\n", retval);
				RetvalFail(4);
			}else
				return retval;
			break;
	}
	return 0;
}

int CheckISFSRetval(int retval)
{
	switch(retval)
	{
		case ISFS_EINVAL:
			__printerrmsg("FAILED! (ISFS: Invalid Argument)\n");
			RetvalFail(3);
			break;

		case ISFS_ENOMEM:
			__printerrmsg("FAILED! (ISFS: Out of memory)\n");
			RetvalFail(2);
			break;
		default:
			if(retval<0)
			{
				CheckIPCRetval(retval);
			}else
				return retval;
			break;
	}
	return 0;
}

int CheckIPCRetval(int retval)
{
	switch(retval)
	{
		case IPC_EINVAL:
			__printerrmsg("FAILED! (IPC: Invalid Argument)\n");
			RetvalFail(3);
			break;

		case IPC_ENOMEM:
			__printerrmsg("FAILED! (IPC: Out of memory)\n");
			RetvalFail(2);
			break;

		case IPC_ENOHEAP:
			__printerrmsg("FAILED! (IPC: Out of heap (?))\n");
			RetvalFail(2);
			break;

		case IPC_ENOENT:
			__printerrmsg("FAILED! (IPC: No entity (?))\n");
			RetvalFail(3);
			break;

		case IPC_EQUEUEFULL:
			__printerrmsg("FAILED! (IPC: Queue Full)\n");
			RetvalFail(1);
			break;

		default:
			if(retval<0)
			{
				__printerrmsg("FAILED! (IPC: Unknown error %d)\n", retval);
				RetvalFail(4);
			}else
				return retval;
			break;
	}
	return 0;
}

int CheckCARDRetval(int retval)
{
	switch(retval)
	{
		case CARD_ERROR_BUSY:
			__printerrmsg("FAILED! (CARD: Card is busy)\n");
			RetvalFail(1);
			break;

		case CARD_ERROR_WRONGDEVICE:
			__printerrmsg("FAILED! (CARD: Wrong device connected to card slot)\n");
			RetvalFail(1);
			break;

		case CARD_ERROR_NOCARD:
			__printerrmsg("FAILED! (CARD: No card connected)\n");
			RetvalFail(1);
			break;

		case CARD_ERROR_NOFILE:
			__printerrmsg("FAILED! (CARD: File does not exist)\n");
			RetvalFail(0);
			break;

		case CARD_ERROR_IOERROR:
			__printerrmsg("FAILED! (CARD: Internal EXI I/O Error!)\n");
			RetvalFail(3);
			break;

		case CARD_ERROR_BROKEN:
			__printerrmsg("FAILED! (CARD: File/Dir Entry is broken)\n");
			RetvalFail(0);
			break;

		case CARD_ERROR_EXIST:
			__printerrmsg("FAILED! (CARD: File already exists)\n");
			RetvalFail(0);
			break;

		case CARD_ERROR_NOENT:
			__printerrmsg("FAILED! (CARD: No empty blocks to create file)\n");
			RetvalFail(0);
			break;

		case CARD_ERROR_INSSPACE:
			__printerrmsg("FAILED! (CARD: Not enough space to write file)\n");
			RetvalFail(0);
			break;

		case CARD_ERROR_NOPERM:
			__printerrmsg("FAILED! (CARD: Not enough permissions)\n");
			RetvalFail(0);
			break;

		case CARD_ERROR_LIMIT:
			__printerrmsg("FAILED! (CARD: Card size limit reached)\n");
			RetvalFail(0);
			break;

		case CARD_ERROR_NAMETOOLONG:
			__printerrmsg("FAILED! (CARD: Filename is too long)\n");
			RetvalFail(0);
			break;

		case CARD_ERROR_ENCODING:
			__printerrmsg("FAILED! (CARD: Wrong region memory card)\n");
			RetvalFail(0);
			break;

		case CARD_ERROR_CANCELED:
			__printerrmsg("FAILED! (CARD: Card operation canceled)\n");
			RetvalFail(0);
			break;

		case CARD_ERROR_FATAL_ERROR:
			__printerrmsg("FAILED! (CARD: Unrecoverable error!)...\n");
			RetvalFail(3);
			break;

		default:
			if(retval<0)
			{
				__printerrmsg("FAILED! (CARD: Unknown error %d)\n", retval);
				RetvalFail(4);
			}else
				return retval;
			break;
	}
	return 0;
}

int CheckCONFRetval(int retval)
{
	switch(retval)
	{
		case CONF_ENOMEM:
			__printerrmsg("FAILED! (CONF: Out of memory)\n");
			RetvalFail(2);
			break;

		case CONF_EBADFILE:
			__printerrmsg("FAILED! (CONF: File (?) is bad)\n");
			RetvalFail(4);
			break;

		case CONF_ENOENT:
			__printerrmsg("FAILED! (CONF: No Entity (?))\n");
			RetvalFail(2);
			break;

		case CONF_ETOOBIG:
			__printerrmsg("FAILED! (CONF: Too big)\n");
			RetvalFail(1);
			break;

		case CONF_ENOTINIT:
			__printerrmsg("FAILED! (CONF: Not initialized)\n");
			RetvalFail(1);
			break;

		case CONF_ENOTIMPL:
			__printerrmsg("FAILED! (CONF: Not implied)\n");
			RetvalFail(0);
			break;

		case CONF_EBADVALUE:
			__printerrmsg("FAILED! (CONF: Bad value)\n");
			RetvalFail(0);
			break;

		default:
			if(retval<0)
			{
				__printerrmsg("FAILED! (CONF: Unknown error %d)\n", retval);
				RetvalFail(4);
			}else
				return retval;
			break;
	}
	return 0;
}

int CheckDVDRetval(int retval)
{
	switch(retval)
	{
		case DVD_ERROR_FATAL:
			__printerrmsg("FAILED! (DVD: Fatal Error)\n");
			RetvalFail(2);
			break;

		case DVD_ERROR_IGNORED:
			__printerrmsg("FAILED! (DVD: Ignored)\n");
			RetvalFail(0);
			break;

		case DVD_ERROR_CANCELED:
			__printerrmsg("FAILED! (DVD: Canceled)\n");
			RetvalFail(0);
			break;

		case DVD_ERROR_COVER_CLOSED:
			__printerrmsg("FAILED! (DVD: Cover closed)\n");
			RetvalFail(0);
			break;
		default:
			if(retval<0)
			{
				__printerrmsg("FAILED! (DVD: Unknown error %d)\n", retval);
				RetvalFail(4);
			}else
				return retval;
			break;
	}
	return 0;
}

int CheckIOSRetval(int retval)
{
	switch(retval)
	{
		case IOS_EINVAL:
			__printerrmsg("FAILED! (IOS: Invalid Argument)\n");
			RetvalFail(3);
			break;

		case IOS_EBADVERSION:
			__printerrmsg("FAILED! (IOS: Bad version)\n");
			RetvalFail(2);
			break;

		case IOS_ETOOMANYVIEWS:
			__printerrmsg("FAILED! (IOS: Too many ticket views)\n");
			RetvalFail(3);
			break;

		case IOS_EMISMATCH:
			__printerrmsg("FAILED! (IOS: Mismatch)\n");
			RetvalFail(3);
			break;
		default:
			if(retval<0)
			{
				__printerrmsg("FAILED! (IOS: Unknown error %d)\n", retval);
				RetvalFail(4);
			}else
				return retval;
			break;
	}
	return 0;
}

int CheckTSS2Retval(int retval)
{
	switch(retval)
	{
		case MQ_ERROR_TOOMANY:
			__printerrmsg("FAILED! (Thread Subsystem 2: Too many threads (?))\n");
			RetvalFail(2);
			break;

		default:
			if(retval<0)
			{
				__printerrmsg("FAILED! (Thread Subsystem 2: Unknown error %d)\n", retval);
				RetvalFail(4);
			}else
				return retval;
			break;
	}
	return 0;
}

int CheckPADRetval(int retval)
{
	switch(retval)
	{
		case PAD_ERR_NO_CONTROLLER:
			__printerrmsg("FAILED! (PAD: No controller)\n");
			RetvalFail(0);
			break;

		case PAD_ERR_NOT_READY:
			__printerrmsg("FAILED! (PAD: Not ready)\n");
			RetvalFail(1);
			break;

		case PAD_ERR_TRANSFER:
			__printerrmsg("FAILED! (PAD: Transfer)\n");
			RetvalFail(2);
			break;

		default:
			if(retval<0)
			{
				__printerrmsg("FAILED! (PAD: Unknown error %d)\n", retval);
				RetvalFail(4);
			}else
				return retval;
			break;
	}
	return 0;
}

int CheckSTMRetval(int retval)
{
	switch(retval)
	{
		case STM_EINVAL:
			__printerrmsg("FAILED! (STM: Invalid argument)\n");
			RetvalFail(3);
			break;

		case STM_ENOTINIT:
			__printerrmsg("FAILED! (STM: Not initialized)\n");
			RetvalFail(3);
			break;

		case STM_ENOHANDLER:
			__printerrmsg("FAILED! (STM: No handler)\n");
			RetvalFail(3);
			break;

		default:
			if(retval<0)
			{
				__printerrmsg("FAILED! (STM: Unknown error %d)\n", retval);
				RetvalFail(4);
			}else
				return retval;
			break;
	}
	return 0;
}

int CheckUSBSTORAGERetval(int retval)
{
	switch(retval)
	{
		case USBSTORAGE_ENOINTERFACE:
			__printerrmsg("FAILED! (USBSTORAGE: No interface)\n");
			RetvalFail(1);
			break;

		case USBSTORAGE_ESENSE:
			__printerrmsg("FAILED! (USBSTORAGE: Sense error (?))\n");
			RetvalFail(1);
			break;

		case USBSTORAGE_ESHORTWRITE:
			__printerrmsg("FAILED! (USBSTORAGE: Short write)\n");
			RetvalFail(1);
			break;

		case USBSTORAGE_ESHORTREAD:
			__printerrmsg("FAILED! (USBSTORAGE: Short read)\n");
			RetvalFail(1);
			break;

		case USBSTORAGE_ESIGNATURE:
			__printerrmsg("FAILED! (USBSTORAGE: Bad signature (?))\n");
			RetvalFail(1);
			break;

		case USBSTORAGE_ETAG:
			__printerrmsg("FAILED! (USBSTORAGE: Bad tag (?))\n");
			RetvalFail(1);
			break;

		case USBSTORAGE_ESTATUS:
			__printerrmsg("FAILED! (USBSTORAGE: Bad status (?))\n");
			RetvalFail(1);
			break;

		case USBSTORAGE_EDATARESIDUE:
			__printerrmsg("FAILED! (USBSTORAGE: Data residue (?))\n");
			RetvalFail(1);
			break;

		case USBSTORAGE_ETIMEDOUT:
			__printerrmsg("FAILED! (USBSTORAGE: Timed out)\n");
			RetvalFail(0);
			break;

		case USBSTORAGE_EINIT:
			__printerrmsg("FAILED! (USBSTORAGE: Not initialized (?))\n");
			RetvalFail(1);
			break;

		default:
			if(retval<0)
			{
				__printerrmsg("FAILED! (USBSTORAGE: Unknown error %d)\n", retval);
				RetvalFail(4);
			}else
				return retval;
			break;
	}
	return 0;
}

int CheckWIIRetval(int retval)
{
	switch(retval)
	{
		case WII_ENOTINIT:
			__printerrmsg("FAILED! (WII: Not initialized)\n");
			RetvalFail(3);
			break;

		case WII_EINTERNAL:
			__printerrmsg("FAILED! (WII: Internal error)\n");
			RetvalFail(3);
			break;

		case WII_ECHECKSUM:
			__printerrmsg("FAILED! (WII: Checksum error)\n");
			RetvalFail(3);
			break;

		case WII_EINSTALL:
			__printerrmsg("FAILED! (WII: Title not installed)\n");
			RetvalFail(1);
			break;

		case WII_E2BIG:
			__printerrmsg("FAILED! (WII: Argument list too big)\n");
			RetvalFail(1);
			break;

		default:
			if(retval<0)
			{
				__printerrmsg("FAILED! (WII: Unknown error %d)\n", retval);
				RetvalFail(4);
			}else
				return retval;
			break;
	}
	return 0;
}


