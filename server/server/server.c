#include <WinSock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "util.h"
#include "iso.h"
#include "isoList.h"

// #define DEBUG_OUT		1

#define SERVER_LISTEN_PORT		4323
#define COMMAND_TIMEOUT			2		// the amount of time it will wait for a command
#define PACKET_SIZE_SEND		1452

#define CMD_MAGIC				0x49535652 // 'ISVR'
#define CMD_OK_MAGIC			0x6F6B4F4B // 'okOK'

enum {
	CMD_NOOP = 0,
	CMD_GEOMETRY,
	CMD_MEDIA_TYPE,
	CMD_READ,
	CMD_ISO_INDEX,
	CMD_ISO_MOUNT,
};

typedef struct _SESSION_STRUCT{
	SOCKET sac;
	PISO_LIST_ITEM curDisk;
	int timeoutCount;
	BOOL isConnected;
}SESSION_STRUCT, *PSESSION_STRUCT;

#pragma pack(push, 1)
typedef struct _CMD_STRUCT {
	u32 magic;		// 0 0x49535652 // 'ISVR'
	u16 command;	// 4
	u16 index;		// 6
	s64 offset;		// 8
	u32 size;		// 0x10
} CMD_STRUCT, *PCMD_STRUCT;
C_ASSERT(sizeof(CMD_STRUCT) == 0x14);
#pragma pack(pop)

char* cmdNames[] = {"NOOP", "Geometry", "MediaType", "Read", "Index"};

BOOL isRunning = TRUE;
SOCKET sServ = INVALID_SOCKET;

int receiveCommand(SOCKET sCmd, u16* cmd, u16* idx, u32* size, s64* offset)
{
	u8 cmdData[sizeof(CMD_STRUCT)];
	DWORD ret;
	int iBytes = 0;
	TIMEVAL tv;
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(sCmd, &fds);
	tv.tv_sec = COMMAND_TIMEOUT;
	tv.tv_usec = 0;
	ret = select(0, &fds, 0, 0, &tv);
	if((ret == SOCKET_ERROR) || (ret == 0))
	{
//		printf("timeout waiting for command, %d\n", WSAGetLastError());
		return -2; // Timeout
	}
	ret = recv(sCmd, (char*)cmdData, sizeof(CMD_STRUCT), 0);
// 	printf("rec %d of %d\n", ret, sizeof(CMD_STRUCT));
	if((ret == SOCKET_ERROR) || (ret == 0))
	{
		printf("socket error receiving command, %d\n", WSAGetLastError());
		return -1; // Network error
	}
	if(getBeU32(cmdData) == CMD_MAGIC)
	{
		*cmd = getBeU16(&cmdData[4]);
		*idx = getBeU16(&cmdData[6]);
		*offset = (s64)getBeU64(&cmdData[8]);
		*size = getBeU32(&cmdData[0x10]);
		//printf("cmd: %d offset: 0x%llx size: 0x%x\n", *cmd, *offset, *size);
		return 1; // OK
	}
	else
		printf("error! command magic is unexpected value!\n");
	return 0; // some other error
}

char* receiveData(SOCKET sCli, u32 dwBytesToRec)
{
	if(dwBytesToRec == 0)
		return NULL;
	else
	{
		char* recBy = (char*)malloc(dwBytesToRec+1);
		if(recBy != NULL)
		{
			DWORD ret;
			memset(recBy, 0, dwBytesToRec+1);
			ret = recv(sCli, recBy, dwBytesToRec, 0);
			if(ret != SOCKET_ERROR)
			{
				return recBy;
			}
			free(recBy);
		}
	}
	return NULL;
}

// send data
BOOL sendData(SOCKET sCli, void* data, u32 dwBytesToSend, BOOL track)
{
	int ret;
	char* pdata = (char*)data;
	//unsigned char sendSz[4];
	u32 sndsz = PACKET_SIZE_SEND;
	u32 dwSent = 0;
	//setBeU32(dwBytesToSend, sendSz); // xbox expects big endian!
	//ret = send(sCli, (char*)&sendSz, 4, 0);
	//if(ret == SOCKET_ERROR)
	//{
	//	printf("socket error sending data size, %d\n", WSAGetLastError());
	//	return FALSE;
	//}
	while(dwSent < dwBytesToSend)
	{
		if((dwBytesToSend-dwSent) < PACKET_SIZE_SEND)
			sndsz = dwBytesToSend-dwSent;
		ret = send(sCli, &pdata[dwSent], sndsz, 0);
		if(ret == SOCKET_ERROR)
		{
// 			if(track)
// 				printf("\n");
			printf("socket error sending data (1), %d\n", WSAGetLastError());
			return FALSE;
		}
		dwSent += ret;
// 		if(track)
// 			printf("sent 0x%08x of 0x%08x\r", dwSent, dwBytesToSend);
	}
// 	if(track)
// 		printf("\n");

	return TRUE;
}

BOOL sendGeometry(PSESSION_STRUCT ss)
{
	u8 geoResp[8];
	if(ss->curDisk == NULL)
		memset(geoResp, 0, 8);
	else
	{
		u32 sec = (ss->curDisk->numSectors&0xFFFFFFFF);
		setBeU32(sec, geoResp);
		setBeU32(0x800, &geoResp[4]);
	}
	return sendData(ss->sac, geoResp, 8, FALSE);
}

BOOL sendType(PSESSION_STRUCT ss)
{
	u8 dtype[4];
	if(ss->curDisk == NULL)
		memset(dtype, 0, 4);
	else
		setBeU32(ss->curDisk->diskType, dtype);
	return sendData(ss->sac, dtype, 4, FALSE);
}

BOOL sendOk(SOCKET sac)
{
	u8 okResp[8];
	setBeU32(CMD_MAGIC, okResp);
	setBeU32(CMD_OK_MAGIC, &okResp[4]);
	return sendData(sac, okResp, 8, FALSE);
}

BOOL sendIsoRead(PSESSION_STRUCT ss, s64 offset, u32 size)
{
	BOOL ret = FALSE;
	u8* data;
	data = (u8*)malloc(size);
	if(data != NULL)
	{
		if(isoRead(ss->curDisk, offset, data, size))
			ret = sendData(ss->sac, data, size, FALSE);
		else
			printf("isoRead failed! size: 0x%x offset: 0x%llx\n", size, offset);
		free(data);
	}
	else
		printf("unable to malloc 0x%x bytes!\n", size);
	return ret;
}

BOOL sendIsoIndex(SOCKET sac, u16 index, u32 maxsize)
{
	BOOL ret = FALSE;
	u8* data;
	data = (u8*)malloc(maxsize);
	if(data != NULL)
	{
		char* lname;
		memset(data, 0, maxsize); // sending a zeroed buffer when there is none left
		lname = isoListGetIndexName(index);
		if(lname != NULL)
		{
			strncpy((char*)data, lname, maxsize);
		}
		ret = sendData(sac, data, maxsize, FALSE);
		free(data);
	}
	else
		printf("unable to malloc 0x%x bytes!\n", maxsize);
	return ret;
}

BOOL sendIsoMountIndexFromRec(SOCKET sac, u32 nameLen, u16* idx)
{
	BOOL ret = FALSE;
	char* dname = receiveData(sac, nameLen);
	if(dname != NULL)
	{
		int iret = isoListGetIdxFromName(dname, nameLen);
		printf("index is %i\n", iret);
		if(iret >= 0)
		{
			*idx = iret&0xFFFF;
			ret = TRUE;
		}
		free(dname);
	}
	return ret;
}

BOOL sendIsoMount(PSESSION_STRUCT ss, u32 size)
{
	BYTE resp[4];
	BOOL ret = FALSE;
	u16 index;
	memset(resp, 0, 4);
	if(sendIsoMountIndexFromRec(ss->sac, size, &index))
	{
		if(ss->curDisk != NULL)
		{
			ss->curDisk->clients--;
			if(ss->curDisk->clients == 0)
				fclose(ss->curDisk->currHandle);
			ss->curDisk = NULL;
		}
		ss->curDisk = isoSetImage(index);
		if(ss->curDisk != NULL)
		{
			resp[3] = 1;
			ss->curDisk->clients++;
		}
	}
	else
	{
		printf("iso mount failed or dismount issued\n");
		if(ss->curDisk != NULL)
		{
			ss->curDisk->clients--;
			if(ss->curDisk->clients == 0)
				fclose(ss->curDisk->currHandle);
			ss->curDisk = NULL;
		}
	}
	ret = sendData(ss->sac, resp, 4, FALSE);
	return ret;
}

DWORD clientThread(SOCKET sac)
{
	SESSION_STRUCT ss;
	ss.curDisk = NULL;
	ss.isConnected = TRUE;
	ss.timeoutCount = 0;
	ss.sac = sac;
	while((ss.isConnected)&&(ss.timeoutCount < 4))
	{
		int retsuc;
		u16 cmd;
		u16 index;
		s64 offset;
		u32 size;
		// receive command
		retsuc = receiveCommand(sac, &cmd, &index, &size, &offset);
		if(retsuc == 1)
		{
			ss.timeoutCount = 0;
//			printf("received command %d (%s)\n", cmd, cmdNames[cmd]);
			// send response
			switch(cmd)
			{
			case CMD_NOOP:
#ifdef DEBUG_OUT
				printf("noop\n");
#endif
				ss.isConnected = sendOk(sac);
				break;
			case CMD_GEOMETRY:
#ifdef DEBUG_OUT
				printf("sendGeometry\n");
#endif
				ss.isConnected = sendGeometry(&ss);
				break;
			case CMD_MEDIA_TYPE:
#ifdef DEBUG_OUT
				printf("sendMediaType\n");
#endif
				ss.isConnected = sendType(&ss);
				break;
			case CMD_READ:
#ifdef DEBUG_OUT
				printf("sendIsoRead offset 0x%I64x size 0x%x\n", offset, size);
#endif
				ss.isConnected = sendIsoRead(&ss, offset, size);
				break;
			case CMD_ISO_INDEX:
#ifdef DEBUG_OUT
				printf("sendIsoIndex index %d maxsize 0x%x\n", index, size);
#endif
				ss.isConnected = sendIsoIndex(sac, index, size);
				break;
			case CMD_ISO_MOUNT:
#ifdef DEBUG_OUT
				printf("sendIsoMount name size 0x%x\n", size);
#endif
				ss.isConnected = sendIsoMount(&ss, size);
				break;
			default:
				printf("unknown command %d\n", cmd);
			}
		}
		else if(retsuc == -1) // close socket
		{
			printf("closing socket on error\n");
			ss.isConnected = FALSE;
		}
		else if(retsuc == -2) // timeout
		{
			ss.timeoutCount += 1;
		}
		else
			printf("erroneous command received!!\n");
	}
	closesocket(sac);
	return 0;
}

void serverRun(void)
{
	printf("waiting for a connection\n");
	while(isRunning)
	{
		SOCKADDR_IN ac;
		SOCKET saccp;
		int acsz = sizeof(SOCKADDR_IN);
		if ((saccp = accept(sServ, (PSOCKADDR)&ac, &acsz)) != INVALID_SOCKET)
		{
			DWORD thid;
			HANDLE hTh;
			printf("accepted client from %d.%d.%d.%d:%d\n", ac.sin_addr.s_net, ac.sin_addr.s_host, ac.sin_addr.s_lh, ac.sin_addr.s_impno, ac.sin_port);
			hTh = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)clientThread, (LPVOID)saccp, 0, &thid);
			CloseHandle(hTh);
		}
		else
			printf("accept failed %d\n", WSAGetLastError());
	}
}

void serverStartup(void)
{
	WSADATA wsd;
	printf("starting server\n");
	if(WSAStartup(MAKEWORD(2,2),&wsd) == 0)
	{
		sServ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(sServ != INVALID_SOCKET)
		{
			SOCKADDR_IN siPc;
			memset(&siPc, 0, sizeof(SOCKADDR_IN));
			siPc.sin_family = AF_INET;
			siPc.sin_port = htons(SERVER_LISTEN_PORT);
			siPc.sin_addr.s_addr = INADDR_ANY;
			if(bind(sServ, (PSOCKADDR)&siPc, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
			{
				printf("unable to set bind socket, error %d\n", WSAGetLastError());
				closesocket(sServ);
				return;
			}
			if(listen(sServ, SOMAXCONN) != 0)
			{
				printf("unable to listen on socket, error %d\n", WSAGetLastError());
				return;
			}
			serverRun();
			closesocket(sServ);
		}
		else
			printf("unable to allocate socket, error %d\n", WSAGetLastError());
	}
	else
		printf("WSAStartup failed, error %d\n", WSAGetLastError());
}
