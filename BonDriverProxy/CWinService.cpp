#include "CWinService.h"

CWinService CWinService::This;
SERVICE_STATUS CWinService::serviceStatus;
SERVICE_STATUS_HANDLE CWinService::serviceStatusHandle;
HANDLE CWinService::hServerStopEvent;

CWinService::CWinService()
{
	//������
	hServerStopEvent = NULL;

	// �T�[�r�X����ݒ肷��
	::GetModuleFileName(NULL, serviceExePath, BUFSIZ);
	::_tsplitpath_s(serviceExePath, NULL, 0, NULL, 0, serviceName, BUFSIZ, NULL, 0);
}

CWinService::~CWinService()
{
}

//
// SCM�ւ̃C���X�g�[��
//
BOOL CWinService::Install()
{
	SC_HANDLE hManager = NULL;
	SC_HANDLE hService = NULL;
	BOOL ret = FALSE;

	do
	{
		hManager = ::OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
		if (hManager == NULL)
			break;

		hService = ::CreateService(hManager,
			serviceName,				// service name
			serviceName,				// service name to display
			0,							// desired access
			SERVICE_WIN32_OWN_PROCESS,	// service type
			SERVICE_DEMAND_START,		// start type
			SERVICE_ERROR_NORMAL,		// error control type
			serviceExePath,				// service's binary
			NULL,						// no load ordering group
			NULL,						// no tag identifier
			NULL,						// no dependencies
			NULL,						// LocalSystem account
			NULL);						// no password
		if (hService == NULL)
			break;

		ret = TRUE;
	}
	while (0);

	if (hService)
		::CloseServiceHandle(hService);

	if (hManager)
		::CloseServiceHandle(hManager);

	return ret;
}

//
// SCM����폜
//
BOOL CWinService::Remove()
{
	SC_HANDLE hManager = NULL;
	SC_HANDLE hService = NULL;
	BOOL ret = FALSE;

	do
	{
		hManager = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
		if (hManager == NULL)
			break;

		hService = ::OpenService(hManager, serviceName, DELETE);
		if (hService == NULL)
			break;

		if (::DeleteService(hService) == FALSE)
			break;

		ret = TRUE;
	}
	while (0);

	if (hService)
		::CloseServiceHandle(hService);

	if (hManager)
		::CloseServiceHandle(hManager);

	return ret;
}

//
// �T�[�r�X�N��
//
BOOL CWinService::Start()
{
	SC_HANDLE hManager = NULL;
	SC_HANDLE hService = NULL;
	BOOL ret = FALSE;
	SERVICE_STATUS sStatus;
	DWORD waited = 0;

	do
	{
		hManager = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
		if (hManager == NULL)
			break;

		hService = ::OpenService(hManager, serviceName, SERVICE_START | SERVICE_QUERY_STATUS);
		if (hService == NULL)
			break;

		if (::QueryServiceStatus(hService, &sStatus) == FALSE)
			break;

		if (sStatus.dwCurrentState == SERVICE_RUNNING)
		{
			ret = TRUE;
			break;
		}

		if (::StartService(hService, NULL, NULL) == FALSE)
			break;

		while (1)
		{
			if (::QueryServiceStatus(hService, &sStatus) == FALSE)
				break;

			if (sStatus.dwCurrentState == SERVICE_RUNNING)
			{
				ret = TRUE;
				break;
			}

			if (waited >= sStatus.dwWaitHint)
				break;

			::Sleep(500);
			waited += 500;
		}
	}
	while (0);

	if (hService)
		::CloseServiceHandle(hService);

	if (hManager)
		::CloseServiceHandle(hManager);

	return ret;
}

//
// �T�[�r�X��~
//
BOOL CWinService::Stop()
{
	SC_HANDLE hManager = NULL;
	SC_HANDLE hService = NULL;
	BOOL ret = FALSE;
	SERVICE_STATUS sStatus;
	DWORD waited = 0;

	do
	{
		hManager = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
		if (hManager == NULL)
			break;

		hService = ::OpenService(hManager, serviceName, SERVICE_STOP | SERVICE_QUERY_STATUS);
		if (hService == NULL)
			break;

		if (::QueryServiceStatus(hService, &sStatus) == FALSE)
			break;

		if (sStatus.dwCurrentState == SERVICE_STOPPED)
		{
			ret = TRUE;
			break;
		}

		if (::ControlService(hService, SERVICE_CONTROL_STOP, &sStatus) == FALSE)
			break;

		while (1)
		{
			if (::QueryServiceStatus(hService, &sStatus) == FALSE)
				break;

			if (sStatus.dwCurrentState == SERVICE_STOPPED)
			{
				ret = TRUE;
				break;
			}

			if (waited >= sStatus.dwWaitHint)
				break;

			::Sleep(500);
			waited += 500;
		}
	}
	while (0);

	if (hService)
		::CloseServiceHandle(hService);

	if (hManager)
		::CloseServiceHandle(hManager);

	return ret;
}

//
// �T�[�r�X�ċN��
//
BOOL CWinService::Restart()
{
	if (Stop())
		return Start();
	return FALSE;
}

//
// �T�[�r�X���s
//
BOOL CWinService::Run(LPSERVICE_MAIN_FUNCTIONW lpServiceProc)
{
	SERVICE_TABLE_ENTRY DispatchTable[] = { { serviceName, lpServiceProc }, { NULL, NULL } };
	return ::StartServiceCtrlDispatcher(DispatchTable);
}

//
// ServiceMain����T�[�r�X�J�n�O�ɌĂяo��
//
BOOL CWinService::RegisterService()
{
	// �T�[�r�X��~�p�C�x���g���쐬
	hServerStopEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hServerStopEvent == NULL)
		return FALSE;

	// SCM����̐���n���h����o�^
	serviceStatusHandle = ::RegisterServiceCtrlHandlerEx(serviceName, ServiceCtrlHandler, NULL);
	if (serviceStatusHandle == 0)
	{
		::CloseHandle(hServerStopEvent);
		hServerStopEvent = NULL;
		return FALSE;
	}

	// ��Ԃ��J�n���ɐݒ�
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	serviceStatus.dwWin32ExitCode = NO_ERROR;
	serviceStatus.dwServiceSpecificExitCode = 0;
	serviceStatus.dwCheckPoint = 1;
	serviceStatus.dwWaitHint = 30000;
	::SetServiceStatus(serviceStatusHandle, &serviceStatus);

	return TRUE;
}

//
// ServiceMain����T�[�r�X�J�n��ďo��(��~�v���܂�return���Ȃ�)
//
void CWinService::ServiceRunning()
{
	// �g�����Ԉ���Ă�(RegisterService()���Ă�łȂ�)
	if (hServerStopEvent == NULL)
		return;

	// ��Ԃ��J�n�ɐݒ�
	serviceStatus.dwCurrentState = SERVICE_RUNNING;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;
	::SetServiceStatus(serviceStatusHandle, &serviceStatus);

	// �T�[�r�X�ɒ�~�v���������Ă���܂őҋ@
	::WaitForSingleObject(hServerStopEvent, INFINITE);

	return;
}

//
// ServiceMain����T�[�r�X�I��������Ăяo��
//
void CWinService::ServiceStopped()
{
	if (hServerStopEvent)
	{
		//�C�x���g�N���[�Y
		::CloseHandle(hServerStopEvent);
		hServerStopEvent = NULL;

		//��Ԃ��~�ɐݒ�
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		serviceStatus.dwCheckPoint = 0;
		serviceStatus.dwWaitHint = 0;
		::SetServiceStatus(serviceStatusHandle, &serviceStatus);
	}
	return;
}

//
// �T�[�r�X�R���g���[���n���h������
//
DWORD WINAPI CWinService::ServiceCtrlHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	switch (dwControl)
	{
	case SERVICE_CONTROL_STOP:
	{
		if (hServerStopEvent)
		{
			serviceStatus.dwWin32ExitCode = 0;
			serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
			serviceStatus.dwCheckPoint = 0;
			serviceStatus.dwWaitHint = 30000;
			::SetServiceStatus(serviceStatusHandle, &serviceStatus);
			// ��~�C�x���g���g���K
			::SetEvent(hServerStopEvent);
		}
		break;
	}

	case SERVICE_CONTROL_INTERROGATE:
		::SetServiceStatus(serviceStatusHandle, &serviceStatus);
		break;

	default:
		break;
	}
	return NO_ERROR;
}
