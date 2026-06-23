#include <stdio.h>
#include <string.h>
#include "types.h"
#include "util.h"
#include "iso.h"
#include "isoList.h"

#define XGD_IMAGE_MAGIC		"MICROSOFT*XBOX*MEDIA"
#define XGD_MAGIC_SIZE		20

#define XGD_MAGIC_OFFSET_GOD	0x2000		// not supported yet
#define XGD_ISO_BASE_OFFSET		0x10000
#define XGD_MAGIC_OFFSET_XDKI	XGD_ISO_BASE_OFFSET		// images made with the xdk image builder

#define XGD_MAGIC_OFFSET_XGD2	0xFDA0000
#define XGD2_PFI_OFFSET			0xFD8E800
#define XGD2_DMI_OFFSET			0xFD8F000
#define XGD2_SS_OFFSET			0xFD8F800

#define XGD_MAGIC_OFFSET_XGD3	0x2090000
#define XGD3_PFI_OFFSET			0x2076800
#define XGD3_DMI_OFFSET			0x2077000
#define XGD3_SS_OFFSET			0x2077800

char* dtypeNames[] = {
	"standard",
	"requires overlapped io" // some games like forza seem to iocomplete slightly differently, nothing tried in the driver seemed to fix this
};

BOOL isoReadInternal(FILE* imageHandle, s64 offset, u8* dest, int len)
{
	_fseeki64(imageHandle, offset, SEEK_SET);
	fread(dest, len, 1, imageHandle);
	return TRUE; // bah! who needs error handling?
}

BOOL isoRead(PISO_LIST_ITEM itm, s64 offset, u8* dest, int len)
{
	return isoReadInternal(itm->currHandle, offset, dest, len);
}

BOOL getXgdBaseOffset(FILE* imageHandle, s64 maxOffset, s64* baseOffset)
{
	u8 buf[XGD_MAGIC_SIZE+1];
	memset(buf, 0, XGD_MAGIC_SIZE+1);
	// at some point it should be possible to get GOD working over network too
	//isoRead(XGD_MAGIC_OFFSET_GOD, buf, XGD_MAGIC_SIZE);
	//if(memcmp(buf, XGD_IMAGE_MAGIC, XGD_MAGIC_SIZE) == 0)
	//{
	//	baseOffset = XGD_MAGIC_OFFSET_GOD;
	//}
	if(maxOffset > (XGD_MAGIC_OFFSET_XDKI+XGD_MAGIC_SIZE))
	{
		isoReadInternal(imageHandle, XGD_MAGIC_OFFSET_XDKI, buf, XGD_MAGIC_SIZE); // xdk images
		if(memcmp(buf, XGD_IMAGE_MAGIC, XGD_MAGIC_SIZE) == 0)
		{
			*baseOffset = XGD_MAGIC_OFFSET_XDKI-XGD_ISO_BASE_OFFSET; // should be 0
			return TRUE;
		}
	}
	if(maxOffset > (XGD_MAGIC_OFFSET_XGD3+XGD_MAGIC_SIZE))
	{
		isoReadInternal(imageHandle, XGD_MAGIC_OFFSET_XGD3, buf, XGD_MAGIC_SIZE); // XGD3 images
		if(memcmp(buf, XGD_IMAGE_MAGIC, XGD_MAGIC_SIZE) == 0)
		{
			*baseOffset = XGD_MAGIC_OFFSET_XGD3-XGD_ISO_BASE_OFFSET;
			return TRUE;
		}
	}
	if(maxOffset > (XGD_MAGIC_OFFSET_XGD2+XGD_MAGIC_SIZE))
	{
		isoReadInternal(imageHandle, XGD_MAGIC_OFFSET_XGD2, buf, XGD_MAGIC_SIZE); // XGD2 images
		if(memcmp(buf, XGD_IMAGE_MAGIC, XGD_MAGIC_SIZE) == 0)
		{
			*baseOffset = XGD_MAGIC_OFFSET_XGD2-XGD_ISO_BASE_OFFSET;
			return TRUE;
		}
	}
	return FALSE;
}

u32 getDiskTypeByFile(char* path)
{
	char tpath[MAXPATHLEN];
	u32 diskType = 0;
	strcpy(tpath, path);
	strcat(tpath, ".type1");
	//printf("looking for %s\n", tpath);
	if(FileExists(tpath))
		diskType = 1;
	return diskType;
}

PISO_LIST_ITEM isoSetImage(int idx)
{
	char path[MAXPATHLEN];
	if(isoListBuildIndexPath(idx, path))
	{
		if(FileExists(path))
		{
			PISO_LIST_ITEM itm = isoListGetIdxItem(idx);
			printf("mounting %s...", path);
			itm->currHandle = fopen(path, "rb");
			if(itm->currHandle != NULL)
			{
				printf("success!\n");
				return itm;
			}
			printf("failed!\n");
		}
	}
	return NULL;
}

BOOL isoIsImage(char* image, PISO_LIST_ITEM itm)
{
	BOOL ret = FALSE;
	if(FileExists(image))
	{
		FILE* fhand;
		fhand = fopen(image, "rb");
		if(fhand != NULL)
		{
			itm->maxOffset = getFileSize64(fhand);
			if(getXgdBaseOffset(fhand, itm->maxOffset, &itm->baseOffset))
			{
				itm->diskType = getDiskTypeByFile(image);
				itm->numSectors = (itm->maxOffset)/0x800LL;
				ret = TRUE;
			}
			fclose(fhand);
		}
	}
	return ret;
}
