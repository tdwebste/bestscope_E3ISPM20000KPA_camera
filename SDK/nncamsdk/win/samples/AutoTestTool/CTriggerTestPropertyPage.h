#pragma once

class CTriggerTestPropertyPage : public CPropertyPage
{
	bool m_bStarting;
	int m_totalCount;
	int m_count;
	int m_interval;
	HANDLE m_hThread;
public:
	CTriggerTestPropertyPage();

	int GetTotalCount() const;
	int GetInterval() const;
	bool IsStarting() const;
	
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROPERTY_TRIGGER_TEST };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnTriggerTestCountUpdate(WPARAM wp, LPARAM lp);
	afx_msg void OnEnChangeEditTriggerTestTimes();
	afx_msg void OnEnChangeEditTriggerTestInterval();
	afx_msg void OnBnClickedButtonTriggerTestStart();
	DECLARE_MESSAGE_MAP()
private:
	void UpdateHint();
};
