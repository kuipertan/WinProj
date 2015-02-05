#include "stdafx.h"
#include <Windows.h>
#include <Winsvc.h>
#include <Mmsystem.h>
#include <TlHelp32.h>
#include <string>
#include "Service.h"

#pragma comment(lib, "Winmm.lib")

SERVICE_STATUS			gStat;
SERVICE_STATUS_HANDLE	gStatHandle;

#define PROCESS_NAME	"cetus_agent.exe"
#define CRONTAB_TIME	60000 //msc

HANDLE gProcHandle;

void WINAPI serviceCtrlHandler(DWORD opcode);
BOOL startCetus();
BOOL stopCetus();

BOOL InstallService()
{
	SC_HANDLE schSCManager, schService;

	char binPathName[MAX_PATH] = {0};
	if(0 == GetModuleFileName(NULL, binPathName, sizeof(binPathName)))
	{
		return FALSE;
	}

	if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) == NULL)
	{
		return FALSE;
	}

	schService = CreateService(schSCManager, SERVICE_NAME, SERVICE_DISPLAY, 
		SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START,
		SERVICE_ERROR_NORMAL, binPathName, NULL, NULL, NULL,NULL, NULL);
	if (NULL == schService)
	{
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	
	return TRUE;
}

BOOL DeleteService()
{
	SC_HANDLE schSCManager, schService;

	if ((schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) == NULL)
	{
		return FALSE;
	}


	schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_STOP | DELETE);
	if (NULL == schService)
	{
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	SERVICE_STATUS svstat;
	ControlService(schService, SERVICE_STOP, &svstat);
	if( 0 == DeleteService(schService))
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return TRUE;
}

BOOL startCetus()
{
	char szAppPath[MAX_PATH] = {0};
	GetModuleFileName(NULL, szAppPath, MAX_PATH);
	std::string path = szAppPath;
	size_t pos = path.find_last_of("\\");
	path.replace(pos + 1, path.length() - pos, "");
	std::string cmd = path + PROCESS_NAME + " -conf=";
	cmd += path;
	cmd += "cetus.conf ";
	cmd += "-log_dir=";
	cmd += path;
	cmd += "log";

	STARTUPINFO si = {sizeof(si)}; 
	PROCESS_INFORMATION pi;
	BOOL ret =  CreateProcess(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	if (!ret)
	{
		return ret;
	}

	gProcHandle = pi.hProcess;
	return ret;
}

BOOL stopCetus()
{
	return TerminateProcess(gProcHandle,0);
}

BOOL checkAlive()
{
	PROCESSENTRY32 pe;
	DWORD id = 0;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnap, &pe)){
		CloseHandle(hSnap);
		return FALSE;
	}
	do 
	{
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (!Process32Next(hSnap, &pe))
			break;
		if (strcmp(pe.szExeFile, PROCESS_NAME) == 0){
			CloseHandle(hSnap);
			return TRUE;
		}
	} while (TRUE);

	CloseHandle(hSnap);
	return FALSE;
}

void WINAPI serviceCtrlHandler(DWORD opcode)
{
	switch (opcode)
	{
	case SERVICE_CONTROL_PAUSE:
		gStat.dwCurrentState = SERVICE_PAUSED;
		break;
	case SERVICE_CONTROL_CONTINUE:						
		gStat.dwCurrentState = SERVICE_RUNNING;				
		break;
	case SERVICE_CONTROL_STOP:
		stopCetus();           
		gStat.dwWin32ExitCode = 0;
		gStat.dwCurrentState = SERVICE_STOPPED;
		gStat.dwCheckPoint = 0;
		gStat.dwWaitHint = 0;		
		break;
	default:
		break;
	}
	SetServiceStatus(gStatHandle, &gStat);
	return;
}


void WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	HANDLE shutDownEvent = CreateEvent(NULL, TRUE, FALSE, NULL); 

	gStat.dwServiceType = SERVICE_WIN32;
	gStat.dwCurrentState = SERVICE_START_PENDING;
	gStat.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	gStat.dwWin32ExitCode = 0;
	gStat.dwServiceSpecificExitCode = 0;
	gStat.dwCheckPoint = 0;
	gStat.dwWaitHint = 0;

	gStatHandle = RegisterServiceCtrlHandler(SERVICE_NAME, 
		serviceCtrlHandler); 
	
	gStat.dwCurrentState = SERVICE_RUNNING;
	gStat.dwCheckPoint = 0;
	gStat.dwWaitHint = 0;
	SetServiceStatus (gStatHandle, &gStat);

	while (gStat.dwCurrentState != SERVICE_STOPPED)
	{
		if (!checkAlive())
		{
			startCetus();
		}
		Sleep(CRONTAB_TIME);
	}
	
}
