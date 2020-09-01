#pragma once

class CSnapResTestPropertyPage : public CPropertyPage
{
	bool m_bStarting;
	int m_totalCount;
	int m_count;
	HANDLE m_hThread;
public:
	CSnapResTestPropertyPage();

	int GetTotalCount() const;
	bool IsStarting() const;

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROPERTY_SNAP_RES_TEST };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnSnapCountUpdate(WPARAM wp, LPARAM lp);
	afx_msg void OnEnChangeEditSnapCount();
	afx_msg void OnBnClickedButtonStart();
	DECLARE_MESSAGE_MAP()
private:
	void UpdateHint();
};
