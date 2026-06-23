#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "iso.h"

#define MAX_LIST_ITEMS	256
static char BasePath[MAXPATHLEN];
static int isoCount = 0;
static PISO_LIST_ITEM list[MAX_LIST_ITEMS];

void isoListSetBasePath(char* path)
{
	strcpy(BasePath, path);
	printf("basepath set to: %s\n", BasePath);
}

void isoListBuildItemPath(char* item, char* path)
{
	strcpy(path, BasePath);
	strcat(path, "\\");
	strcat(path, item);
}

BOOL isoListBuildIndexPath(int idx, char* path)
{
	if(idx < isoCount)
	{
		strcpy(path, BasePath);
		strcat(path, "\\");
		strcat(path, list[idx]->name);
		return TRUE;
	}
	return FALSE;
}

PISO_LIST_ITEM isoListGetIdxItem(int idx)
{
	if(idx < isoCount)
		return list[idx];
	return NULL;
}

void isoListAddItem(char* item)
{
	ISO_LIST_ITEM litem;
	char path[MAXPATHLEN];
	isoListBuildItemPath(item, path);
	if(isoIsImage(path, &litem))
	{
		PISO_LIST_ITEM aitem;
		aitem = (PISO_LIST_ITEM)malloc(sizeof(ISO_LIST_ITEM));
		if(aitem)
		{
			memset(aitem, 0, sizeof(ISO_LIST_ITEM));
			aitem->name = (char*)malloc(strlen(item)+1);
			if(aitem->name)
			{
				strcpy(aitem->name, item);
				aitem->baseOffset = litem.baseOffset;
				aitem->maxOffset = litem.maxOffset;
				aitem->diskType = litem.diskType;
				aitem->numSectors = litem.numSectors;
				list[isoCount] = aitem;
				isoCount++;
			}
			else
				free(aitem);
		}
	}
}

static int isoListCompare(const void * a, const void * b)
{
	const PISO_LIST_ITEM ia = *(PISO_LIST_ITEM *) a;
	const PISO_LIST_ITEM ib = *(PISO_LIST_ITEM *) b;

	return stricmp(ia->name, ib->name);
}

void isoListBuildList(void)
{
	int i;
	HANDLE hFind;
	WIN32_FIND_DATA findData;
	char path[MAXPATHLEN];
	isoCount = 0;
	strcpy(path, BasePath);
	strcat(path, "\\*");
	printf("building iso list at %s\n", path);
	hFind = FindFirstFile(path, &findData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do 
		{
			if((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				//printf("%s\n", findData.cFileName);
				isoListAddItem(findData.cFileName);
			}
		} while (FindNextFile(hFind, &findData));
		FindClose(hFind);
	}
	else
		printf("FindFirstFile failed! Error: %d\n", GetLastError());

	printf("sorted list:\n");
	qsort((void*)list, isoCount, sizeof(PISO_LIST_ITEM), isoListCompare);
	for(i = 0; i < isoCount; i++)
	{
		printf("%d: %s\n", i, list[i]->name);
	}
}

char* isoListGetIndexName(int idx)
{
	if(idx < isoCount)
		return list[idx]->name;
	return NULL;
}

int isoListGetIdxFromName(char* name, u32 namelen)
{
	int i; // '\battlefield_hardline_d1.iso\Mount\'
	char* tname = strchr(&name[1], '\\');
	printf("isoListGetIdxFromName: %s\n", name);
	tname[0] = 0;
	printf("fixed name: %s\n", &name[1]);
	if(strcmp(&name[1], "[Disable Current ISO]") == 0)
		return -2;
	for(i = 0; i < isoCount; i++)
	{
		if(stricmp(&name[1], list[i]->name) == 0)
		{
			printf("match found at idx %d\n", i);
			return i;
		}
	}
	printf("no match found!!");
	return -1;
}
