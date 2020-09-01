#pragma once

class CSettingPropertySheet;
class CAutoTestToolDlg : public CDialogEx
{
	CComboBox m_cameraList;
	CString m_cameraID;
	CString m_cameraName;
	BITMAPINFOHEADER m_header;
	void* m_pImageData;
	CSettingPropertySheet* m_pSettingPropertySheet;
public:
	CAutoTestToolDlg(CWnd* pParent = nullptr);	

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_AUTOTESTTOOL_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	

protected:
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
	afx_msg LRESULT OnMsgCamevent(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnPreviewResChanged(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnCloseOpen(WPARAM wp, LPARAM lp);
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnClose();
	afx_msg void OnBnClickedButtonSetting();
	afx_msg void OnBnClickedButtonTest();
	afx_msg void OnBnClickedButton1();
	DECLARE_MESSAGE_MAP()
private:
	void EnumCameras();
	void UpdateButtonsStates();
	void StartCamera();
	void CloseCamera();
	void OnEventError();
	void OnEventImage();
	void OnEventExpo();
	void OnEventTempTint();
	void OnEventStillImage();
	void UpdateInfo();
};

extern CAutoTestToolDlg* g_pMainDlg;