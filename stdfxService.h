/*预编译头*/

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>


#pragma comment(lib,"advapi32.lib")

#define SERVICE_NAME TEXT("TestForService")

SERVICE_STATUS g_ServiceStatus;

SERVICE_STATUS_HANDLE g_ServiceStatusHandle;

HANDLE g_hServiceStopEvent = NULL;

VOID ServiceInstall(void);//SrvInstall();

VOID WINAPI ServiceCtrlHandler(DWORD);//服务控制函数

VOID WINAPI ServiceMain(DWORD,LPTSTR*);//服务函数入口


void ReportServiceStatus(DWORD,DWORD,DWORD);//ReportSrv()

VOID ServiceInitial(DWORD,LPTSTR*);//SrvInit()

void ServiceReportEvent(LPTSTR);//SrvReportEvent();