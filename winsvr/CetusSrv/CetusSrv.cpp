// CetusSrv.cpp :
//

#include "stdafx.h"

#include <Windows.h>
#include "Service.h"


int main(int argc, char *argv[])
{
	if (argc > 1)
	{
		if (strcmp(argv[1], "-i") == 0)
		{
			if (!InstallService())
			{
				printf("Failed to install service.\n");
			}
		}
		else if (strcmp(argv[1], "-u") == 0)
		{
			if (!DeleteService())
			{
				printf("Failed to uninstall service.\n");
			}
		}

		return 0;
	}

	SERVICE_TABLE_ENTRY dispTab[] = {
		{SERVICE_NAME, ServiceMain},
		{NULL, NULL}
	};

	if (!StartServiceCtrlDispatcher(dispTab))
	{
		printf("Failed to start service dispatcher.\n");
	}

	return 0;
}