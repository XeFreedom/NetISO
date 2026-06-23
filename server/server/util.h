#ifndef _UTIL_H
#define _UTIL_H

#define u16Rev(x) (((x&0xFF)<<8)+(((x&0xFF00)>>8)))
#define bswap16(x) u16Rev(x)

#define u32Rev(x) ((((x&0xFF)<<24))+(((x&0xFF00)<<8))+(((x&0xFF0000)>>8))+(((x&0xFF000000)>>24)))
#define bswap32(x) u32Rev(x)

#ifndef _MSC_VER
#define u64Rev(x) (((x&0xFF)<<56)+((x&0xFF00)<<40)+((x&0xFF0000)<<24)+((x&0xFF000000)<<8)+((x>>8)&0xFF000000)+((x>>24)&0xFF0000)+((x>>40)&0xFF00)+((x>>56)&0xFF))
#define bswap64(x) u64Rev(x)
#else
#define u64Rev(x) _byteswap_uint64(x)
#define bswap64(x) _byteswap_uint64(x)
#endif

u16 getBeU16(void* prt);
u32 getBeU32(void* prt);
u64 getBeU64(void* prt);
u32 getLeU32(void* prt);
void setBeU16(u16 val, void* prt);
void setBeU32(u32 val, void* prt);
void setBeU64(unsigned long long num, void* prt);
void display_buffer_hex(u8 *buffer, int size, int verbLevel);
void dump_buffer_hex(char* filename, void* buffer, int size);
BOOL writeBufferToFile(char* filename, void* buffer, int size);

// gets the 64bit size of the file and rewinds it to offset 0
s64 getFileSize64(FILE* fptr);

// returns true if a file exists
BOOL FileExists(char* filename);


#endif // _UTIL_H
