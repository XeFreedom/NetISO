#include <stdio.h>
#include "types.h"
#include "server.h"
#include "iso.h"
#include "isoList.h"

int main(int argc, char* argv[])
{
	//int i;
	//for(i = 0; i < argc; i++)
	//	printf("arg%d:'%s'\n", i, argv[i]);
	printf("Hello from netiso server!\n");
	if(argc == 1)
		isoListSetBasePath(".");
	else
		isoListSetBasePath(argv[1]);
	isoListBuildList();
	serverStartup();

	return 0;
}

