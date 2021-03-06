/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicEncoderDist - Ambisonic Encoder with distance                 #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicEncoderDist.cpp                                 #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicEncoderDist.h"

CAmbisonicEncoderDist::CAmbisonicEncoderDist()
{
	m_nSampleRate = 0;
	m_fDelay = 0.f;
	m_nDelay = 0;
	m_nDelayBufferLength = 0;
	m_pfDelayBuffer = 0;
	m_nIn = 0;
	m_nOutA = 0;
	m_nOutB = 0;
	m_fRoomRadius = 5.f;
	m_fInteriorGain = 0.f;
	m_fExteriorGain = 0.f;

	Create(DEFAULT_ORDER, DEFAULT_HEIGHT, DEFAULT_SAMPLERATE);
}

CAmbisonicEncoderDist::~CAmbisonicEncoderDist()
{
	if(m_pfDelayBuffer)
		delete [] m_pfDelayBuffer;
}

bool CAmbisonicEncoderDist::Create(AmbUInt nOrder, AmbBool b3D, AmbUInt nSampleRate)
{
    bool success = CAmbisonicEncoder::Create(nOrder, b3D, 0);
	if(!success)
        return false;
    m_nSampleRate = nSampleRate;
	m_nDelayBufferLength = (AmbUInt)((AmbFloat)knMaxDistance / knSpeedOfSound * m_nSampleRate + 0.5f);
	if(m_pfDelayBuffer)
		delete [] m_pfDelayBuffer;
	m_pfDelayBuffer = new AmbFloat[m_nDelayBufferLength];
	Reset();
    
    return true;
}

void CAmbisonicEncoderDist::Reset()
{
	memset(m_pfDelayBuffer, 0, m_nDelayBufferLength * sizeof(AmbFloat));
	m_fDelay = m_polPosition.fDistance / knSpeedOfSound * m_nSampleRate + 0.5f;
	m_nDelay = (AmbInt)m_fDelay;
	m_fDelay -= m_nDelay;
	m_nIn = 0;
	m_nOutA = (m_nIn - m_nDelay + m_nDelayBufferLength) % m_nDelayBufferLength;
	m_nOutB = (m_nOutA + 1) % m_nDelayBufferLength;
}

void CAmbisonicEncoderDist::Refresh()
{
	CAmbisonicEncoder::Refresh();

	m_fDelay = fabs(m_polPosition.fDistance) / knSpeedOfSound * m_nSampleRate; //TODO abs() sees AmbFloat as int!
	m_nDelay = (AmbInt)m_fDelay;
	m_fDelay -= m_nDelay;
	m_nOutA = (m_nIn - m_nDelay + m_nDelayBufferLength) % m_nDelayBufferLength;
	m_nOutB = (m_nOutA + 1) % m_nDelayBufferLength;

	//Source is outside speaker array
	if(fabs(m_polPosition.fDistance) >= m_fRoomRadius)
	{
		m_fInteriorGain	= (m_fRoomRadius / fabs(m_polPosition.fDistance)) / 2.f;
		m_fExteriorGain	= m_fInteriorGain;
	}
	else
	{
		m_fInteriorGain = (2.f - fabs(m_polPosition.fDistance) / m_fRoomRadius) / 2.f;
		m_fExteriorGain = (fabs(m_polPosition.fDistance) / m_fRoomRadius) / 2.f;
	}
}

void CAmbisonicEncoderDist::Process(AmbFloat* pfSrc, AmbUInt nSamples, CBFormat* pfDst)
{
	AmbUInt niChannel = 0;
	AmbUInt niSample = 0;
	AmbFloat fSrcSample = 0;
	
	for(niSample = 0; niSample < nSamples; niSample++)
	{
		//Store
		m_pfDelayBuffer[m_nIn] = pfSrc[niSample];
		//Read
		fSrcSample = m_pfDelayBuffer[m_nOutA] * (1.f - m_fDelay) 
					+ m_pfDelayBuffer[m_nOutB] * m_fDelay;
		
		pfDst->m_ppfChannels[kW][niSample] = fSrcSample * m_fInteriorGain * m_pfCoeff[kW];
		
		fSrcSample *= m_fExteriorGain;
		for(niChannel = 1; niChannel < m_nChannelCount; niChannel++)
		{
			pfDst->m_ppfChannels[niChannel][niSample] = fSrcSample * m_pfCoeff[niChannel];
		}

		m_nIn = (m_nIn + 1) % m_nDelayBufferLength;
		m_nOutA = (m_nOutA + 1) % m_nDelayBufferLength;
		m_nOutB = (m_nOutB + 1) % m_nDelayBufferLength;
	}
}

void CAmbisonicEncoderDist::SetRoomRadius(AmbFloat fRoomRadius)
{
	m_fRoomRadius = fRoomRadius;
}

AmbFloat CAmbisonicEncoderDist::GetRoomRadius()
{
	return m_fRoomRadius;
}