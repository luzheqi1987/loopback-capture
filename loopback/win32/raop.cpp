#include <Windows.h>

#include "prefs.h"
#include "loopback-capture.h"

#include "../airplay/raop_client.h"

#define AIRPLAY_IO_TIMEOUT 5

static int loopback_stream_cb(void *arg, guchar *buf, int buflen) {
    CPrefs* prefs = (CPrefs*)arg;
    DWORD dwWaitResult = WaitForSingleObject(prefs->m_AudioBufferMutex, INFINITE);

    int copy_len = 0;
    printf("streamcb\n");

    switch (dwWaitResult)
    {
    case WAIT_OBJECT_0:
        __try {
            if (prefs->m_AudioBufferLength > 0) {
                bool wraparound = false;
                copy_len = buflen > prefs->m_AudioBufferLength ? prefs->m_AudioBufferLength : buflen;
                if (prefs->m_AudioBufferLastRead + buflen > prefs->m_AudioBufferSize) {
                    copy_len = prefs->m_AudioBufferSize - prefs->m_AudioBufferLastRead;
                    wraparound = true;
                }
                memcpy(buf, prefs->m_AudioBuffer+prefs->m_AudioBufferLastRead, copy_len);
                prefs->m_AudioBufferLastRead += copy_len;
                if (wraparound) {
                    prefs->m_AudioBufferLastRead = 0;
                }
                prefs->m_AudioBufferLength -= copy_len;
            } else {
                // send nothing
                memset(buf, 0, buflen);
                return buflen;
            }
        }

        __finally {
            if (! ReleaseMutex(prefs->m_AudioBufferMutex))
            {
                // TODO: Handle error.
            }
        }
        return copy_len;
        break;

    case WAIT_ABANDONED:
        return 0;
    }
}

int raop_worker(CPrefs* prefs) {
    fd_set rfds, wfds, efds;
    struct timeval timeout;
    raop_client_t* raop;

    raop_client_init(&raop);

    if (raop_client_connect(raop, "192.168.40.129", 5002) != RAOP_EOK) {
        printf("RAOP client connect failed\n");
        return 0;
    }
    printf("raop connected\n");
    raop_client_set_stream_cb(raop, loopback_stream_cb, prefs);
    raop_client_set_volume(raop, 4);
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
        }
        if (stream_fd != -1) {
            if (FD_ISSET (stream_fd, &rfds))
                raop_client_handle_io (raop, stream_fd, G_IO_IN);
            if (FD_ISSET (stream_fd, &wfds))
                raop_client_handle_io (raop, stream_fd, G_IO_OUT);
            if (FD_ISSET (stream_fd, &efds)) {
                raop_client_handle_io (raop, stream_fd, G_IO_ERR);
                printf("stream_fd err\n");
                break;
            }
        }
    }
    return 0;
}

DWORD WINAPI raop_thread(LPVOID pContext) {
    CPrefs* prefs = (CPrefs*)pContext;
    return raop_worker(prefs);
}