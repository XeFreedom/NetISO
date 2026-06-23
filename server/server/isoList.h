#ifndef _ISO_LIST_H
#define _ISO_LIST_H

void isoListSetBasePath(char* path);
BOOL isoListBuildIndexPath(int idx, char* path);
PISO_LIST_ITEM isoListGetIdxItem(int idx);
void isoListBuildList(void);
char* isoListGetIndexName(int idx);
int isoListGetIdxFromName(char* name, u32 namelen);

#endif // _ISO_LIST_H
