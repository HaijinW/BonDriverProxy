#ifndef __CWINSERVICE_H__
#define __CWINSERVICE_H__

// SERVICE_WIN32_OWN_PROCESS�̃T�[�r�X�𑀍삷��ׂ̃��[�e�B���e�B�N���X
class CWinService
{
	static CWinService This;
	static SERVICE_STATUS serviceStatus;
	static SERVICE_STATUS_HANDLE serviceStatusHandle;
	// �I���C�x���g
	static HANDLE hServerStopEvent;
	// �T�[�r�X����
	TCHAR serviceName[BUFSIZ];
	// �T�[�r�X���̃p�X
	TCHAR serviceExePath[BUFSIZ];

	CWinService();
	virtual ~CWinService();

	// �T�[�r�X�R���g���[���n���h��
	static DWORD WINAPI ServiceCtrlHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);

public:
	static CWinService *getInstance(){ return &This; };

	// �C���X�g�[���ƃA���C���X�g�[��
	BOOL Install();
	BOOL Remove();

	// �N���E��~�E�ċN��
	BOOL Start();
	BOOL Stop();
	BOOL Restart();

	// ���s
	BOOL Run(LPSERVICE_MAIN_FUNCTIONW lpServiceProc);

	// �T�[�r�X���C������Ăяo���葱���֐�
	BOOL RegisterService();
	void ServiceRunning();
	void ServiceStopped();
};

#endif // __CWINSERVICE_H__
