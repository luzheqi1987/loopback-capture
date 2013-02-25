#include <Windows.h>
#include <assert.h>

#include "prefs.h"
#include "loopback-capture.h"

#include "../airplay/raop_client.h"

#include "../alac/codec/ALACEncoder.h"

enum
{
    kTestFormatFlag_16BitSourceData    = 1,
    kTestFormatFlag_20BitSourceData    = 2,
    kTestFormatFlag_24BitSourceData    = 3,
    kTestFormatFlag_32BitSourceData    = 4
};

#define AIRPLAY_IO_TIMEOUT 1

static int loopback_stream_cb(void *arg, guchar *buf, int buflen);

class RAOPBridge {
    ALACEncoder m_Encoder;
    CPrefs* m_Prefs;
    AudioFormatDescription m_OutputFormat;
    AudioFormatDescription m_InputFormat;
    BYTE* m_EncoderOutputBuffer;
    BYTE* m_EncoderInputBuffer;
    size_t m_PCMSize;

public:
    RAOPBridge(CPrefs* prefs) : m_Encoder(), m_Prefs(prefs) {
        m_OutputFormat.mSampleRate = 44100;
        m_OutputFormat.mFormatID = kALACFormatAppleLossless;
        m_OutputFormat.mFormatFlags = kTestFormatFlag_16BitSourceData;
        m_OutputFormat.mFramesPerPacket = RAOP_ALAC_FRAME_SIZE;
        m_OutputFormat.mChannelsPerFrame = 2;
        m_OutputFormat.mBytesPerPacket = m_OutputFormat.mBytesPerFrame = m_OutputFormat.mBitsPerChannel = m_OutputFormat.mReserved = 0;
        m_Encoder.SetFrameSize(m_OutputFormat.mFramesPerPacket);
        m_Encoder.InitializeEncoder(m_OutputFormat);

        m_InputFormat.mSampleRate = 44100;
        m_InputFormat.mFormatID = kALACFormatLinearPCM;
        m_InputFormat.mFramesPerPacket = 1;
        m_InputFormat.mBitsPerChannel = 16;
        m_InputFormat.mChannelsPerFrame = 2;
        m_InputFormat.mFormatFlags = kALACFormatFlagIsSignedInteger | kALACFormatFlagIsPacked;
        m_InputFormat.mBytesPerFrame = (m_InputFormat.mBitsPerChannel >> 3) * m_InputFormat.mChannelsPerFrame;
        m_InputFormat.mBytesPerPacket = m_InputFormat.mFramesPerPacket * m_InputFormat.mBytesPerFrame;
        m_InputFormat.mReserved = 0;

        m_PCMSize = RAOP_ALAC_FRAME_SIZE * m_InputFormat.mBytesPerFrame;
        m_EncoderOutputBuffer = (BYTE*)malloc(m_PCMSize*4); // use input format size here
        m_EncoderInputBuffer = (BYTE*)malloc(m_PCMSize);
    }

    ~RAOPBridge() {
        if (m_EncoderOutputBuffer != NULL) {
            free(m_EncoderOutputBuffer);
        }
        if (m_EncoderInputBuffer != NULL) {
            free(m_EncoderInputBuffer);
        }
    }

    int run() {
        fd_set rfds, wfds, efds;
        struct timeval timeout;
        raop_client_t* raop;


        raop_client_init(&raop);
        //Sleep(1000000);


        if (raop_client_connect(raop, "172.16.172.107", 5002) != RAOP_EOK) {
            printf("RAOP client connect failed\n");
            return 0;
        }
        printf("raop connected\n");
        raop_client_set_stream_cb(raop, loopback_stream_cb, this);
        raop_client_set_volume(raop, -4);
        gdouble vol = 0;
        raop_client_get_volume(raop, &vol);
        printf("vol: %f\n", vol);

        while(1) {
            FD_ZERO (&rfds);
            FD_ZERO (&wfds);
            FD_ZERO (&efds);
            timeout.tv_sec = AIRPLAY_IO_TIMEOUT;
            timeout.tv_usec = 0;

            //FD_SET (wake_fd, &rfds);
            int rtsp_fd = raop_client_rtsp_sock (raop);
            int stream_fd = raop_client_stream_sock (raop);
            if (raop_client_can_read (raop, rtsp_fd)) {
                FD_SET (rtsp_fd, &rfds);
            }
            if (raop_client_can_write (raop, rtsp_fd)) {
                FD_SET (rtsp_fd, &wfds);
            }
            if (raop_client_can_read (raop, stream_fd)) {
                FD_SET (stream_fd, &rfds);
            }
            if (raop_client_can_write (raop, stream_fd)) {
                FD_SET (stream_fd, &wfds);
            }
            FD_SET (rtsp_fd, &efds);
            if (stream_fd != -1)
                FD_SET (stream_fd, &efds);

            int max_fd = rtsp_fd > stream_fd ? rtsp_fd : stream_fd;
            int ret = select (max_fd + 1, &rfds, &wfds, &efds, &timeout);
            if (ret == 0) {
                // broken
                printf("select=0\n");
                continue; //break;
            }
            if (ret < 0) {
                printf("fucked %d\n", WSAGetLastError());
                break;
            }
            /*if (FD_ISSET (wake_fd, &rfds)) {
            char cmd;
            read (wake_fd, &cmd, 1);
            g_mutex_lock (data->raop_mutex);
            continue;
            }*/
            printf("raopwakeup\n");
            if (FD_ISSET (rtsp_fd, &rfds))
                raop_client_handle_io (raop, rtsp_fd, G_IO_IN);
            if (FD_ISSET (rtsp_fd, &wfds))
                raop_client_handle_io (raop, rtsp_fd, G_IO_OUT);
            if (FD_ISSET (rtsp_fd, &efds)) {
                raop_client_handle_io (raop, rtsp_fd, G_IO_ERR);
                printf("rtsp_fd err\n");
                return 1;
            }
            if (stream_fd != -1) {
                if (FD_ISSET (stream_fd, &rfds))
                    raop_client_handle_io (raop, stream_fd, G_IO_IN);
                if (FD_ISSET (stream_fd, &wfds))
                    raop_client_handle_io (raop, stream_fd, G_IO_OUT);
                if (FD_ISSET (stream_fd, &efds)) {
                    raop_client_handle_io (raop, stream_fd, G_IO_ERR);
                    printf("stream_fd err %d\n", WSAGetLastError());
                    return 2;
                }
            }
            Sleep(1);
        }
        return 0;
    }

    bool pop_audiodata() {
        DWORD dwWaitResult = WaitForSingleObject(m_Prefs->m_AudioBufferMutex, INFINITE);
        bool ok = false;
        
        if (dwWaitResult != WAIT_OBJECT_0)
            return false;

        __try {
            if (m_Prefs->m_AudioBufferLength > m_PCMSize) {
                int len = m_PCMSize;
                unsigned char* destBuffer = m_EncoderInputBuffer;

                if (m_Prefs->m_AudioBufferLastRead + m_PCMSize > m_Prefs->m_AudioBufferSize) {
                    len = m_Prefs->m_AudioBufferSize - m_Prefs->m_AudioBufferLastRead;
                    memcpy(destBuffer, m_Prefs->m_AudioBuffer+m_Prefs->m_AudioBufferLastRead, len);
                    destBuffer += len;
                    len = m_PCMSize - len;
                    m_Prefs->m_AudioBufferLastRead = 0;
                }

                memcpy(destBuffer, m_Prefs->m_AudioBuffer+m_Prefs->m_AudioBufferLastRead, len);
                m_Prefs->m_AudioBufferLastRead += len;
                m_Prefs->m_AudioBufferLength -= len;
                ok = true;
            }
        }

        __finally {
            if (! ReleaseMutex(m_Prefs->m_AudioBufferMutex))
            {
                assert(0);
                ok = false;
            }
        }
        return ok;
    }

    int stream_callback(BYTE* buffer, int inBufferSize) {

        if (!pop_audiodata()) {
            Sleep(2);
            return 0;
        }

        int32_t bufferSize = m_PCMSize;
        assert(inBufferSize >= bufferSize);

            printf("AudioData OK  ");
        m_Encoder.Encode(m_InputFormat, m_OutputFormat, m_EncoderInputBuffer, (BYTE*)buffer, &bufferSize);
        
        return bufferSize;
    }
};

static int loopback_stream_cb(void *arg, guchar *buf, int buflen) {
    RAOPBridge* bridge = (RAOPBridge*)arg;
    return bridge->stream_callback(buf, buflen);
}

DWORD WINAPI raop_thread(LPVOID pContext) {
    CPrefs* prefs = (CPrefs*)pContext;
    RAOPBridge bridge(prefs);
    
    return bridge.run();
}