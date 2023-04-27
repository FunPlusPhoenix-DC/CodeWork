#include "stdfxService.h"


/*Purpose:Entry point for the process*/

int __cdecl _tmain(int argc,TCHAR *argv[]){
    if (lstrcmpi(argv[1],TEXT("Install")) == 0)
    {
        ServiceInstall();
        return 0;
    }
    
    //可在service_table_entry里添加多几项想要开启的服务但最后一项必须是{NULL,NULL}
    SERVICE_TABLE_ENTRY DispatchTalbe[] = {
        {SERVICE_NAME,(LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL,NULL}
    };

    if (!StartServiceCtrlDispatcher(DispatchTalbe))
    {
        ServiceReportEvent(TEXT("StartServiceCtrlDispatcher"));
    }
    
}


/*Install a service in SCM database*/
VOID ServiceInstall(void){
    SC_HANDLE schSCMnager;
    SC_HANDLE schService;
    TCHAR szUnquotedPath[MAX_PATH];

    if(!GetModuleFileName(NULL,szUnquotedPath,MAX_PATH)){
        printf("Can not install Service with error code (%d)",GetLastError());
        return;
    }

    //路径包含空格，引用时也要加上。
    TCHAR szPath[MAX_PATH]{0};
    StringCbPrintf(szPath,MAX_PATH,TEXT("\"%s\""),szUnquotedPath);

    schSCMnager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);//本地电脑,服务数据库，全部权限。
    if(schSCMnager == NULL){
        printf("OpenSCManager failed . Error code is (%d)\n",GetLastError());
    }

    //创建服务

    schService = CreateService(
        schSCMnager,//SCM database
        SERVICE_NAME,//Name of Service
        SERVICE_NAME,//name to display
        SERVICE_ALL_ACCESS,//all access
        SERVICE_WIN32_OWN_PROCESS,//service type
        SERVICE_DEMAND_START,//start type
        SERVICE_ERROR_NORMAL,//error control type   
        szPath,//path to service's binary
        NULL,//no load ordering group
        NULL,//no tag identifier
        NULL,//no dependencies
        NULL,//local system account
        NULL//no password
    );

    if (schService == NULL)
    {
        printf("CreateService failed (%d)\n",GetLastError());
        CloseServiceHandle(schSCMnager);
        return;
    }
    else
        printf("Service installed successfully!\n");
    
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCMnager);
    
}

/*Entry point for the service*/

/*parameters:
    dwArgc  -  Number of arguments in the lpszArgv arry
    lpszArgv  -  Array of strings. The first string is the name of
    the service and subsequent strings are passed by the process
    that called the StartService function to start the service.
 
 Return value:
 None.*/

 void WINAPI ServiceMain(DWORD dwArgc,LPTSTR* lpszArgv){
    // Register the handler function for the service

    g_ServiceStatusHandle = RegisterServiceCtrlHandler(
        SERVICE_NAME,
        ServiceCtrlHandler
    );

    if (g_ServiceStatusHandle == NULL)
    {
        ServiceReportEvent(TEXT("RegisterServiceCtrlHandler"));
        return;
    }
    
    //set SERVICE_STATUS members remain as set here
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;

    //report initial status to the scm

    ReportServiceStatus(SERVICE_START_PENDING,NO_ERROR,3000);

    //Initialization
    ServiceInitial(dwArgc,lpszArgv);

    //Work
    int TimeCount = 0;
    RECT rect;
    LPRECT lpRect = &rect;
    HWND DeskTopHwnd = GetDesktopWindow();
    while (true)
    {
        if(TimeCount > 200){
            SetEvent(g_hServiceStopEvent);
            return;
        }

        GetWindowRect(GetDesktopWindow(),lpRect);
        ::PostMessageA(DeskTopHwnd,WM_KEYDOWN,VK_F5,NULL);
        bool SetCurPosiStatus = SetCursorPos(lpRect->right,lpRect->left);

        if (!SetCurPosiStatus)
        {
            DWORD ErrorCode = GetLastError();
            printf("SetCurPosition error code is (%d)\n",ErrorCode);
            SetEvent(g_hServiceStopEvent);
            return;
        }
        TimeCount++;
        Sleep(100);
    }
    
 }



// Purpose: 
// The service code
//
// Parameters:
// dwArgc - Number of arguments in the lpszArgv array
// lpszArgv - Array of strings. The first string is the name of
// the service and subsequent strings are passed by the process
// that called the StartService function to start the service.
// 
// Return value:
// None
//
 void ServiceInitial(DWORD dwArgc,LPTSTR* lpszArgv){
    g_hServiceStopEvent = CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL
    );

    if (g_hServiceStopEvent = NULL)
    {
        ReportServiceStatus(SERVICE_STOPPED,GetLastError(),0);
        return;
    }
    
    ReportServiceStatus(SERVICE_RUNNING,NO_ERROR,0);

    /*
    Here to perform work until service stop.
    */

   while (true)
   {
    WaitForSingleObject(g_hServiceStopEvent,INFINITE);
    ReportServiceStatus(SERVICE_STOPPED,NO_ERROR,0);
    return;
   }
   
 }


 // Purpose: 
// Sets the current service status and reports it to the SCM.
//
// Parameters:
// dwCurrentState - The current state (see SERVICE_STATUS)
// dwWin32ExitCode - The system error code
// dwWaitHint - Estimated time for pending operation, 
// in milliseconds
// 
// Return v
void ReportServiceStatus(DWORD dwCurrentState,DWORD dwWin32ExitCode,DWORD dwWaitHint){
    static DWORD dwCheckPoint = 1;


    g_ServiceStatus.dwCurrentState = dwCurrentState;
    g_ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    g_ServiceStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState = SERVICE_START_PENDING)
    {
        g_ServiceStatus.dwControlsAccepted = 0;
    }
    else
    {
        g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }

    if ((dwCurrentState = SERVICE_RUNNING) 
        ||
        (dwCurrentState = SERVICE_STOPPED))
    {
        g_ServiceStatus.dwCheckPoint = 0;
    }
    else
    {
        g_ServiceStatus.dwCheckPoint = dwCheckPoint++;
    }
    
    SetServiceStatus(g_ServiceStatusHandle,&g_ServiceStatus);

    
}


// Purpose: 
// Called by SCM whenever a control code is sent to the service
// using the ControlService function.
//
// Parameters:
// dwCtrl - control code
// 
// Return value:
// None
//
void WINAPI ServiceCtrlHandler(DWORD dwCtrl){
    //Handle the requested control code

    switch (dwCtrl)
    {
    case SERVICE_CONTROL_STOP:
        ReportServiceStatus(SERVICE_STOP_PENDING,NO_ERROR,0);

        //Signal the service to stop
        SetEvent(g_hServiceStopEvent);
        ReportServiceStatus(g_ServiceStatus.dwCurrentState,NO_ERROR,0);
        return;

        case SERVICE_CONTROL_INTERROGATE:
        break;

        case SERVICE_CONTROL_CONTINUE:
        ReportServiceStatus(SERVICE_CONTINUE_PENDING,NO_ERROR,0);
        ResetEvent(g_hServiceStopEvent);
        ReportServiceStatus(g_ServiceStatus.dwCurrentState,NO_ERROR,0);
    
    default:
        break;
    }
}


// Purpose: 
// Logs messages to the event log
//
// Parameters:
// szFunction - name of function that failed
// 
// Return value:
// None
//
// Remarks:
// The service must have an entry in the Application event log.
void ServiceReportEvent(LPTSTR szFunction){
    HANDLE hEventSourse;
    LPCTSTR lpszStrings[2]{0};
    TCHAR Buffer[80]{0};

    hEventSourse = RegisterEventSource(NULL,SERVICE_NAME);

    if (NULL != hEventSourse)
    {
        StringCbPrintf(Buffer,80,TEXT("%s failed with %d"),szFunction,GetLastError());

        lpszStrings[0] = SERVICE_NAME;
        lpszStrings[1] = Buffer;

        ReportEvent(
            hEventSourse,
            EVENTLOG_ERROR_TYPE,
            0,
            SERVICE_ERROR_SEVERE,
            NULL,
            2,
            0,
            lpszStrings,
            NULL
        );

        DeregisterEventSource(hEventSourse);
    }
    
}