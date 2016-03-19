// GMFConvertItem.h: interface for the CGMFConvertItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GMFCONVERTITEM_H__7452E6FB_916E_4C48_8D30_06B61FFF4B86__INCLUDED_)
#define AFX_GMFCONVERTITEM_H__7452E6FB_916E_4C48_8D30_06B61FFF4B86__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <streams.h>

#define CHECKMONIKER(p) (((p) != NULL) && ((p) != (IMoniker*)-1))

class CGMFConvertItem : public CObject
{
public:
	CGMFConvertItem();
	DECLARE_DYNAMIC(CGMFConvertItem)
	virtual ~CGMFConvertItem();

	int GetType() { return m_iType; };
	BOOL GetCompatibilityIndex() { return m_bCompatibilityIndex; };
	long GetMasterStream() { return m_iMasterStream; };
	long GetInterleavingFrequency() { return m_lInterleavingFrequency; };
	long GetInterleavingPreroll() { return m_lInterleavingPreroll; };
	int	GetInterleavingMode() { return m_iInterleavingMode; };

	IMoniker* GetAudioFilterMoniker() { return m_pAudioFilterMoniker; };
	int GetAudioMediaType() { return m_iAudioMediaType; };

	IMoniker* GetVideoFilterMoniker() { return m_pVideoFilterMoniker; };
	double GetVideoQuality() { return m_dVideoQuality; };
	DWORD GetBitRate() { return m_dwBitRate; };
	long GetKeyFrameRate() { return m_lKeyFrameRate; };
	long GetPFramesPerKeyFrame() { return m_lPFramesPerKeyFrame; };
	DWORDLONG GetWindowSize() { return m_dwlWindowSize; };
	LPVOID GetVideoCodecState() { return m_pVideoCodecState; };
	int GetVideoCodecStateSize() { return m_cbVideoCodecState; };

	void SetType(int iType) { m_iType = iType; };
	void SetCompatibilityIndex(BOOL bCompatibilityIndex) { m_bCompatibilityIndex = bCompatibilityIndex; };
	void SetMasterStream(long iMasterStream) { m_iMasterStream = iMasterStream; };
	void SetInterleavingFrequency(long lInterleavingFrequency) { m_lInterleavingFrequency = lInterleavingFrequency; };
	void SetInterleavingPreroll(long lInterleavingPreroll) { m_lInterleavingPreroll = lInterleavingPreroll; };
	void SetInterleavingMode(int iInterleavingMode) { m_iInterleavingMode = iInterleavingMode; };

	void SetAudioFilterMoniker(IMoniker* pMoniker);
	void SetAudioMediaType(int iAudioMediaType) { m_iAudioMediaType = iAudioMediaType; };

	void SetVideoFilterMoniker(IMoniker* pMoniker);
	void SetVideoQuality(double dVideoQuality) { m_dVideoQuality = dVideoQuality; };
	void SetBitRate(DWORD dwBitRate) { m_dwBitRate = dwBitRate; };
	void SetKeyFrameRate(long lKeyFrameRate) { m_lKeyFrameRate = lKeyFrameRate; };
	void SetPFramesPerKeyFrame(long lPFramesPerKeyFrame) { m_lPFramesPerKeyFrame = lPFramesPerKeyFrame; };
	void SetWindowSize(DWORDLONG dwlWindowSize) { m_dwlWindowSize = dwlWindowSize; };
	void SetVideoCodecState(LPVOID pVideoCodecState, int cbVideoCodecState);

private:

	// Output file options
	int		m_iType;
	BOOL	m_bCompatibilityIndex;
	long	m_iMasterStream;
	long	m_lInterleavingFrequency;
	long	m_lInterleavingPreroll;
	int		m_iInterleavingMode;

	// Audio compression options
	IMoniker	*m_pAudioFilterMoniker;
	int			m_iAudioMediaType;

	// Video compression options
	IMoniker	*m_pVideoFilterMoniker;
	double		m_dVideoQuality;
	DWORD		m_dwBitRate;
	long		m_lKeyFrameRate;
	long		m_lPFramesPerKeyFrame;
	DWORDLONG	m_dwlWindowSize;
	LPVOID		m_pVideoCodecState;
	int			m_cbVideoCodecState;
};

#endif // !defined(AFX_GMFCONVERTITEM_H__7452E6FB_916E_4C48_8D30_06B61FFF4B86__INCLUDED_)
