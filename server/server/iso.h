#ifndef _ISO_H
#define _ISO_H

typedef struct _ISO_LIST_ITEM{
	FILE* currHandle;
	u32 clients;
	char* name;
	s64 baseOffset;
	s64 maxOffset;
	s64 numSectors;
	u32 diskType;
}ISO_LIST_ITEM, *PISO_LIST_ITEM;

BOOL isoRead(PISO_LIST_ITEM itm, s64 offset, u8* dest, int len);

PISO_LIST_ITEM isoSetImage(int idx);
BOOL isoIsImage(char* image, PISO_LIST_ITEM itm);

#endif // _ISO_H
