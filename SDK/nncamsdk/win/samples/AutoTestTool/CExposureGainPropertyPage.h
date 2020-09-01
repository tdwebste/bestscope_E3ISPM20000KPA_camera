#pragma once

class CExposureGainPropertyPage : public CPropertyPage
{
public:
	CExposureGainPropertyPage();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROPERTY_EXPOSURE_GAIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnAutoExposure(WPARAM wp, LPARAM lp);
	afx_msg void OnBnClickedCheckAuto();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	DECLARE_MESSAGE_MAP()
private:
	void UpdateSlidersEnable();
	void SetTargetValue(int value);
	void SetExpTimeValue(unsigned value);
	void SetGainValue(int value);
};
