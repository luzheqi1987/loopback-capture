// prefs.h
#include <mmsystem.h>
#include <mmdeviceapi.h>

#ifndef PREFS_H
#define PREFS_H

class CPrefs {
public:
    IMMDevice *m_pMMDevice;
    bool m_bInt16;
    PWAVEFORMATEX m_pwfx;
    BYTE* m_AudioBuffer;
    size_t m_AudioBufferSize; // total bytes
    size_t m_AudioBufferLength; // bytes used
    size_t m_AudioBufferLastRead;
    size_t m_AudioBufferLastWrite;
    HANDLE m_AudioBufferMutex;

    // set hr to S_FALSE to abort but return success
    CPrefs(int argc, LPCWSTR argv[], HRESULT &hr);
    ~CPrefs();

};

#endif
