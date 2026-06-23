// some useful utility functions
#include <stdio.h>
#include "types.h"

u16 getBeU16(void* prt)
{
	u8* ptr = (u8*)prt;
	return ((ptr[0]&0xFF)<<8)|(ptr[1]&0xFF);
}

u32 getBeU32(void* prt)
{
	u8* ptr = (u8*)prt;
	u32 ret = (ptr[0]&0xFF)<<24;
	ret |= (ptr[1]&0xFF)<<16;
	ret |= (ptr[2]&0xFF)<<8;
	ret |= ptr[3]&0xFF;
	return ret;
}

u64 getBeU64(void* prt)
{
	u8* ptr = (u8*)prt;
	u64 res = getBeU32(ptr);
	res = res << 32;
	res |= (getBeU32(ptr+4)&0xFFFFFFFF);
	return res;
}

u32 getLeU32(void* prt)
{
	u8* ptr = (u8*)prt;
	u32 ret = (ptr[3]&0xFF)<<24;
	ret |= (ptr[2]&0xFF)<<16;
	ret |= (ptr[1]&0xFF)<<8;
	ret |= ptr[0]&0xFF;
	return ret;
}

void setBeU16(u16 val, void* prt)
{
	u8* ptr = (u8*)prt;
	ptr[0] = (val>>8)&0xFF;
	ptr[1] = val&0xFF;
}

void setBeU32(u32 val, void* prt)
{
	u8* ptr = (u8*)prt;
	ptr[0] = (val>>24)&0xFF;
	ptr[1] = (val>>16)&0xFF;
	ptr[2] = (val>>8)&0xFF;
	ptr[3] = val&0xFF;
}

void setBeU64(unsigned long long num, void* prt)
{
	int i;
	u8* ptr = (u8*)prt;
	for(i = 0; i < 8; i++)
	{
		ptr[7-i] = num&0xFF;
		num = num>>8;
	}
}

void display_buffer_hex(u8 *buffer, int size, int verbLevel)
{
	int i;
	for (i=0; i<size; i++)
	{
		if (!(i%0x10))
			printf("\n  ");
		printf(" %02X", buffer[i]);
	}
	printf("\n");
}

//dump_buffer_hex("kv.bin", boxdata.blInf[FS_KEYVAULT].data, boxdata.blInf[FS_KEYVAULT].len);
void dump_buffer_hex(char* filename, void* buffer, int size)
{
	FILE* fptr;
	printf("dump buffer: n:'%s' b:0x%x s:0x%x\n", filename, buffer, size);
	if((buffer != NULL)&&(filename != NULL)&&(size != 0))
	{
		fptr = fopen(filename, "wb");
		if(fptr != NULL)
		{
			fwrite(buffer, size, 1, fptr);
			fclose(fptr);
		}
		else
			printf("fopen ERROR\n");
	}
	else
		printf("dump buffer arg error\n");
	printf("dump OK\n");
}

BOOL writeBufferToFile(char* filename, void* buffer, int size)
{
	BOOL ret = FALSE;
	FILE* fptr;
	if((buffer != NULL)&&(filename != NULL)&&(size != 0))
	{
		fptr = fopen(filename, "wb");
		if(fptr != NULL)
		{
			if(fwrite(buffer, size, 1, fptr) == 1)
				ret = TRUE;
			fclose(fptr);
		}
	}
	return ret;
}

s64 getFileSize64(FILE* fptr)
{
	s64 len;
	if(fptr == NULL)
	{
		return 0;
	}
	_fseeki64(fptr, 0, SEEK_END);
	len = _ftelli64(fptr);
	rewind (fptr);
	return len;
}

BOOL FileExists(char* filename)
{
	FILE* inp;
	inp = fopen(filename, "rb");
	if(inp == NULL)
		return FALSE;
	fclose(inp);
	return TRUE;
}
