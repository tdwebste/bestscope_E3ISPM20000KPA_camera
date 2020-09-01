#pragma once

class CBitDepthTestPropertyPage : public CPropertyPage
{
	bool m_bStarting;
	int m_totalCount;
	int m_count;
	HANDLE m_hThread;
public:
	CBitDepthTestPropertyPage();

	int GetTotalCount() const;
	BOOL IsStarting() const;

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROPERTY_BITDEPTH_TEST };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeEditBitDepthTestCount();
	afx_msg void OnBnClickedButtonBitDepthTestStart();
	afx_msg LRESULT OnBitDepthTestCountUpdate(WPARAM wp, LPARAM lp);
	DECLARE_MESSAGE_MAP()
private:
	void UpdateHint();
};
