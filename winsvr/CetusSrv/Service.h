
#define SERVICE_NAME	"cetussvc"
#define SERVICE_DISPLAY "Cetus Service"	

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
BOOL InstallService();
BOOL DeleteService();
