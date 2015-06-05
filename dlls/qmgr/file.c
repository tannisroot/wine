/*
 * Queue Manager (BITS) File
 *
 * Copyright 2007, 2008 Google (Roy Shea, Dan Hipschman)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "wininet.h"
#define COBJMACROS
#include "urlmon.h"
#include "qmgr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qmgr);

static inline BackgroundCopyFileImpl *impl_from_IBackgroundCopyFile2(
    IBackgroundCopyFile2 *iface)
{
    return CONTAINING_RECORD(iface, BackgroundCopyFileImpl, IBackgroundCopyFile2_iface);
}

static HRESULT WINAPI BackgroundCopyFile_QueryInterface(
    IBackgroundCopyFile2 *iface,
    REFIID riid,
    void **obj)
{
    BackgroundCopyFileImpl *file = impl_from_IBackgroundCopyFile2(iface);

    TRACE("(%p)->(%s %p)\n", file, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IBackgroundCopyFile) ||
        IsEqualGUID(riid, &IID_IBackgroundCopyFile2))
    {
        *obj = iface;
    }
    else
    {
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IBackgroundCopyFile2_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI BackgroundCopyFile_AddRef(
    IBackgroundCopyFile2 *iface)
{
    BackgroundCopyFileImpl *file = impl_from_IBackgroundCopyFile2(iface);
    ULONG ref = InterlockedIncrement(&file->ref);
    TRACE("(%p)->(%d)\n", file, ref);
    return ref;
}

static ULONG WINAPI BackgroundCopyFile_Release(
    IBackgroundCopyFile2 *iface)
{
    BackgroundCopyFileImpl *file = impl_from_IBackgroundCopyFile2(iface);
    ULONG ref = InterlockedDecrement(&file->ref);

    TRACE("(%p)->(%d)\n", file, ref);

    if (ref == 0)
    {
        IBackgroundCopyJob3_Release(&file->owner->IBackgroundCopyJob3_iface);
        HeapFree(GetProcessHeap(), 0, file->info.LocalName);
        HeapFree(GetProcessHeap(), 0, file->info.RemoteName);
        HeapFree(GetProcessHeap(), 0, file);
    }

    return ref;
}

/* Get the remote name of a background copy file */
static HRESULT WINAPI BackgroundCopyFile_GetRemoteName(
    IBackgroundCopyFile2 *iface,
    LPWSTR *pVal)
{
    BackgroundCopyFileImpl *file = impl_from_IBackgroundCopyFile2(iface);

    TRACE("(%p)->(%p)\n", file, pVal);

    return return_strval(file->info.RemoteName, pVal);
}

static HRESULT WINAPI BackgroundCopyFile_GetLocalName(
    IBackgroundCopyFile2 *iface,
    LPWSTR *pVal)
{
    BackgroundCopyFileImpl *file = impl_from_IBackgroundCopyFile2(iface);

    TRACE("(%p)->(%p)\n", file, pVal);

    return return_strval(file->info.LocalName, pVal);
}

static HRESULT WINAPI BackgroundCopyFile_GetProgress(
    IBackgroundCopyFile2 *iface,
    BG_FILE_PROGRESS *pVal)
{
    BackgroundCopyFileImpl *file = impl_from_IBackgroundCopyFile2(iface);

    TRACE("(%p)->(%p)\n", file, pVal);

    EnterCriticalSection(&file->owner->cs);
    pVal->BytesTotal = file->fileProgress.BytesTotal;
    pVal->BytesTransferred = file->fileProgress.BytesTransferred;
    pVal->Completed = file->fileProgress.Completed;
    LeaveCriticalSection(&file->owner->cs);

    return S_OK;
}

static HRESULT WINAPI BackgroundCopyFile_GetFileRanges(
    IBackgroundCopyFile2 *iface,
    DWORD *RangeCount,
    BG_FILE_RANGE **Ranges)
{
    BackgroundCopyFileImpl *file = impl_from_IBackgroundCopyFile2(iface);
    FIXME("(%p)->(%p %p)\n", file, RangeCount, Ranges);
    return E_NOTIMPL;
}

static HRESULT WINAPI BackgroundCopyFile_SetRemoteName(
    IBackgroundCopyFile2 *iface,
    LPCWSTR Val)
{
    BackgroundCopyFileImpl *file = impl_from_IBackgroundCopyFile2(iface);
    FIXME("(%p)->(%s)\n", file, debugstr_w(Val));
    return E_NOTIMPL;
}

static const IBackgroundCopyFile2Vtbl BackgroundCopyFile2Vtbl =
{
    BackgroundCopyFile_QueryInterface,
    BackgroundCopyFile_AddRef,
    BackgroundCopyFile_Release,
    BackgroundCopyFile_GetRemoteName,
    BackgroundCopyFile_GetLocalName,
    BackgroundCopyFile_GetProgress,
    BackgroundCopyFile_GetFileRanges,
    BackgroundCopyFile_SetRemoteName
};

HRESULT BackgroundCopyFileConstructor(BackgroundCopyJobImpl *owner,
                                      LPCWSTR remoteName, LPCWSTR localName,
                                      BackgroundCopyFileImpl **file)
{
    BackgroundCopyFileImpl *This;
    int n;

    TRACE("(%s, %s, %p)\n", debugstr_w(remoteName), debugstr_w(localName), file);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof *This);
    if (!This)
        return E_OUTOFMEMORY;

    n = (lstrlenW(remoteName) + 1) * sizeof(WCHAR);
    This->info.RemoteName = HeapAlloc(GetProcessHeap(), 0, n);
    if (!This->info.RemoteName)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return E_OUTOFMEMORY;
    }
    memcpy(This->info.RemoteName, remoteName, n);

    n = (lstrlenW(localName) + 1) * sizeof(WCHAR);
    This->info.LocalName = HeapAlloc(GetProcessHeap(), 0, n);
    if (!This->info.LocalName)
    {
        HeapFree(GetProcessHeap(), 0, This->info.RemoteName);
        HeapFree(GetProcessHeap(), 0, This);
        return E_OUTOFMEMORY;
    }
    memcpy(This->info.LocalName, localName, n);

    This->IBackgroundCopyFile2_iface.lpVtbl = &BackgroundCopyFile2Vtbl;
    This->ref = 1;

    This->fileProgress.BytesTotal = BG_SIZE_UNKNOWN;
    This->fileProgress.BytesTransferred = 0;
    This->fileProgress.Completed = FALSE;
    This->owner = owner;
    IBackgroundCopyJob3_AddRef(&owner->IBackgroundCopyJob3_iface);

    *file = This;
    return S_OK;
}

static DWORD CALLBACK copyProgressCallback(LARGE_INTEGER totalSize,
                                           LARGE_INTEGER totalTransferred,
                                           LARGE_INTEGER streamSize,
                                           LARGE_INTEGER streamTransferred,
                                           DWORD streamNum,
                                           DWORD reason,
                                           HANDLE srcFile,
                                           HANDLE dstFile,
                                           LPVOID obj)
{
    BackgroundCopyFileImpl *file = obj;
    BackgroundCopyJobImpl *job = file->owner;
    ULONG64 diff;

    EnterCriticalSection(&job->cs);
    diff = (file->fileProgress.BytesTotal == BG_SIZE_UNKNOWN
            ? totalTransferred.QuadPart
            : totalTransferred.QuadPart - file->fileProgress.BytesTransferred);
    file->fileProgress.BytesTotal = totalSize.QuadPart;
    file->fileProgress.BytesTransferred = totalTransferred.QuadPart;
    job->jobProgress.BytesTransferred += diff;
    LeaveCriticalSection(&job->cs);

    return (job->state == BG_JOB_STATE_TRANSFERRING
            ? PROGRESS_CONTINUE
            : PROGRESS_CANCEL);
}

typedef struct
{
    IBindStatusCallback IBindStatusCallback_iface;
    IHttpNegotiate      IHttpNegotiate_iface;
    BackgroundCopyFileImpl *file;
    LONG ref;
} DLBindStatusCallback;

static inline DLBindStatusCallback *impl_from_IBindStatusCallback(IBindStatusCallback *iface)
{
    return CONTAINING_RECORD(iface, DLBindStatusCallback, IBindStatusCallback_iface);
}

static HRESULT WINAPI DLBindStatusCallback_QueryInterface(
    IBindStatusCallback *iface,
    REFIID riid,
    void **ppvObject)
{
    DLBindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IBindStatusCallback))
    {
        *ppvObject = &This->IBindStatusCallback_iface;
    }
    else if (IsEqualGUID(riid, &IID_IHttpNegotiate))
    {
        *ppvObject = &This->IHttpNegotiate_iface;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IBindStatusCallback_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI DLBindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    DLBindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI DLBindStatusCallback_Release(IBindStatusCallback *iface)
{
    DLBindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
    {
        IBackgroundCopyFile2_Release(&This->file->IBackgroundCopyFile2_iface);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI DLBindStatusCallback_GetBindInfo(
    IBindStatusCallback *iface,
    DWORD *grfBINDF,
    BINDINFO *pbindinfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DLBindStatusCallback_GetPriority(
    IBindStatusCallback *iface,
    LONG *pnPriority)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DLBindStatusCallback_OnDataAvailable(
    IBindStatusCallback *iface,
    DWORD grfBSCF,
    DWORD dwSize,
    FORMATETC *pformatetc,
    STGMEDIUM *pstgmed)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DLBindStatusCallback_OnLowResource(
    IBindStatusCallback *iface,
    DWORD reserved)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DLBindStatusCallback_OnObjectAvailable(
    IBindStatusCallback *iface,
    REFIID riid,
    IUnknown *punk)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DLBindStatusCallback_OnProgress(
    IBindStatusCallback *iface,
    ULONG progress,
    ULONG progressMax,
    ULONG statusCode,
    LPCWSTR statusText)
{
    DLBindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    BackgroundCopyFileImpl *file = This->file;
    BackgroundCopyJobImpl *job = file->owner;
    ULONG64 diff;

    EnterCriticalSection(&job->cs);
    diff = (file->fileProgress.BytesTotal == BG_SIZE_UNKNOWN
            ? progress
            : progress - file->fileProgress.BytesTransferred);
    file->fileProgress.BytesTotal = progressMax ? progressMax : BG_SIZE_UNKNOWN;
    file->fileProgress.BytesTransferred = progress;
    job->jobProgress.BytesTransferred += diff;
    LeaveCriticalSection(&job->cs);

    return S_OK;
}

static HRESULT WINAPI DLBindStatusCallback_OnStartBinding(
    IBindStatusCallback *iface,
    DWORD dwReserved,
    IBinding *pib)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DLBindStatusCallback_OnStopBinding(
    IBindStatusCallback *iface,
    HRESULT hresult,
    LPCWSTR szError)
{
    return E_NOTIMPL;
}

static const IBindStatusCallbackVtbl DLBindStatusCallback_Vtbl =
{
    DLBindStatusCallback_QueryInterface,
    DLBindStatusCallback_AddRef,
    DLBindStatusCallback_Release,
    DLBindStatusCallback_OnStartBinding,
    DLBindStatusCallback_GetPriority,
    DLBindStatusCallback_OnLowResource,
    DLBindStatusCallback_OnProgress,
    DLBindStatusCallback_OnStopBinding,
    DLBindStatusCallback_GetBindInfo,
    DLBindStatusCallback_OnDataAvailable,
    DLBindStatusCallback_OnObjectAvailable
};

static inline DLBindStatusCallback *impl_from_IHttpNegotiate(IHttpNegotiate *iface)
{
    return CONTAINING_RECORD(iface, DLBindStatusCallback, IHttpNegotiate_iface);
}

static HRESULT WINAPI http_negotiate_QueryInterface(
    IHttpNegotiate *iface, REFIID riid, void **ppv)
{
    DLBindStatusCallback *callback = impl_from_IHttpNegotiate(iface);
    return IBindStatusCallback_QueryInterface(&callback->IBindStatusCallback_iface, riid, ppv);
}

static ULONG WINAPI http_negotiate_AddRef(
    IHttpNegotiate *iface)
{
    DLBindStatusCallback *callback = impl_from_IHttpNegotiate(iface);
    return IBindStatusCallback_AddRef(&callback->IBindStatusCallback_iface);
}

static ULONG WINAPI http_negotiate_Release(
    IHttpNegotiate *iface)
{
    DLBindStatusCallback *callback = impl_from_IHttpNegotiate(iface);
    return IBindStatusCallback_Release(&callback->IBindStatusCallback_iface);
}

static HRESULT WINAPI http_negotiate_BeginningTransaction(
    IHttpNegotiate *iface, LPCWSTR url, LPCWSTR headers, DWORD reserved, LPWSTR *add_headers)
{
    DLBindStatusCallback *callback = impl_from_IHttpNegotiate(iface);
    FIXME("(%p)->(%s %s %u %p)\n", callback, debugstr_w(url), debugstr_w(headers), reserved, add_headers);
    return E_NOTIMPL;
}

static HRESULT error_from_http_response(DWORD code)
{
    switch (code)
    {
    case 200: return S_OK;
    case 400: return BG_E_HTTP_ERROR_400;
    case 401: return BG_E_HTTP_ERROR_401;
    case 404: return BG_E_HTTP_ERROR_404;
    case 407: return BG_E_HTTP_ERROR_407;
    case 414: return BG_E_HTTP_ERROR_414;
    case 501: return BG_E_HTTP_ERROR_501;
    case 503: return BG_E_HTTP_ERROR_503;
    case 504: return BG_E_HTTP_ERROR_504;
    case 505: return BG_E_HTTP_ERROR_505;
    default:
        FIXME("unhandled response code %u\n", code);
        return S_OK;
    }
}

static HRESULT WINAPI http_negotiate_OnResponse(
    IHttpNegotiate *iface, DWORD code, LPCWSTR resp_headers, LPCWSTR req_headers, LPWSTR *add_reqheaders)
{
    DLBindStatusCallback *callback = impl_from_IHttpNegotiate(iface);
    BackgroundCopyJobImpl *job = callback->file->owner;

    TRACE("(%p)->(%d %s %s %p)\n", callback, code, debugstr_w(resp_headers), debugstr_w(req_headers),
          add_reqheaders);

    if ((job->error.code = error_from_http_response(code)))
    {
        job->error.context = BG_ERROR_CONTEXT_REMOTE_FILE;
        if (job->error.file) IBackgroundCopyFile2_Release(job->error.file);
        job->error.file = &callback->file->IBackgroundCopyFile2_iface;
        IBackgroundCopyFile2_AddRef(job->error.file);
    }
    else
    {
        job->error.context = 0;
        if (job->error.file)
        {
            IBackgroundCopyFile2_Release(job->error.file);
            job->error.file = NULL;
        }
    }
    *add_reqheaders = NULL;
    return S_OK;
}

static const IHttpNegotiateVtbl http_negotiate_vtbl =
{
    http_negotiate_QueryInterface,
    http_negotiate_AddRef,
    http_negotiate_Release,
    http_negotiate_BeginningTransaction,
    http_negotiate_OnResponse
};

static DLBindStatusCallback *DLBindStatusCallbackConstructor(
    BackgroundCopyFileImpl *file)
{
    DLBindStatusCallback *This = HeapAlloc(GetProcessHeap(), 0, sizeof *This);
    if (!This)
        return NULL;

    This->IBindStatusCallback_iface.lpVtbl = &DLBindStatusCallback_Vtbl;
    This->IHttpNegotiate_iface.lpVtbl = &http_negotiate_vtbl;
    IBackgroundCopyFile2_AddRef(&file->IBackgroundCopyFile2_iface);
    This->file = file;
    This->ref = 1;
    return This;
}

BOOL processFile(BackgroundCopyFileImpl *file, BackgroundCopyJobImpl *job)
{
    static const WCHAR prefix[] = {'B','I','T', 0};
    DLBindStatusCallback *callbackObj;
    WCHAR tmpDir[MAX_PATH];
    WCHAR tmpName[MAX_PATH];
    HRESULT hr;

    if (!GetTempPathW(MAX_PATH, tmpDir))
    {
        ERR("Couldn't create temp file name: %d\n", GetLastError());
        /* Guessing on what state this should give us */
        transitionJobState(job, BG_JOB_STATE_QUEUED, BG_JOB_STATE_TRANSIENT_ERROR);
        return FALSE;
    }

    if (!GetTempFileNameW(tmpDir, prefix, 0, tmpName))
    {
        ERR("Couldn't create temp file: %d\n", GetLastError());
        /* Guessing on what state this should give us */
        transitionJobState(job, BG_JOB_STATE_QUEUED, BG_JOB_STATE_TRANSIENT_ERROR);
        return FALSE;
    }

    callbackObj = DLBindStatusCallbackConstructor(file);
    if (!callbackObj)
    {
        ERR("Out of memory\n");
        transitionJobState(job, BG_JOB_STATE_QUEUED, BG_JOB_STATE_TRANSIENT_ERROR);
        return FALSE;
    }

    EnterCriticalSection(&job->cs);
    file->fileProgress.BytesTotal = BG_SIZE_UNKNOWN;
    file->fileProgress.BytesTransferred = 0;
    file->fileProgress.Completed = FALSE;
    LeaveCriticalSection(&job->cs);

    TRACE("Transferring: %s -> %s -> %s\n",
          debugstr_w(file->info.RemoteName),
          debugstr_w(tmpName),
          debugstr_w(file->info.LocalName));

    transitionJobState(job, BG_JOB_STATE_QUEUED, BG_JOB_STATE_TRANSFERRING);

    DeleteUrlCacheEntryW(file->info.RemoteName);
    hr = URLDownloadToFileW(NULL, file->info.RemoteName, tmpName, 0,
                            &callbackObj->IBindStatusCallback_iface);
    IBindStatusCallback_Release(&callbackObj->IBindStatusCallback_iface);
    if (hr == INET_E_DOWNLOAD_FAILURE)
    {
        TRACE("URLDownload failed, trying local file copy\n");
        if (!CopyFileExW(file->info.RemoteName, tmpName, copyProgressCallback,
                         file, NULL, 0))
        {
            ERR("Local file copy failed: error %d\n", GetLastError());
            transitionJobState(job, BG_JOB_STATE_TRANSFERRING, BG_JOB_STATE_ERROR);
            return FALSE;
        }
    }
    else if (FAILED(hr))
    {
        ERR("URLDownload failed: eh 0x%08x\n", hr);
        transitionJobState(job, BG_JOB_STATE_TRANSFERRING, BG_JOB_STATE_ERROR);
        return FALSE;
    }
    else if (job->error.code)
    {
        ERR("transfer error: 0x%08x\n", job->error.code);
        transitionJobState(job, BG_JOB_STATE_TRANSFERRING, BG_JOB_STATE_ERROR);
        return FALSE;
    }

    if (transitionJobState(job, BG_JOB_STATE_TRANSFERRING, BG_JOB_STATE_QUEUED))
    {
        lstrcpyW(file->tempFileName, tmpName);

        EnterCriticalSection(&job->cs);
        file->fileProgress.Completed = TRUE;
        job->jobProgress.FilesTransferred++;
        LeaveCriticalSection(&job->cs);

        return TRUE;
    }
    else
    {
        DeleteFileW(tmpName);
        return FALSE;
    }
}
