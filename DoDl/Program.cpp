#include <deliveryoptimization.h>
#include <cwchar>
#include <cstdio>

bool inline CheckHResult(HRESULT hr, LPCWSTR func, bool silent = false)
{
    if (hr != S_OK)
    {
        wprintf(L"\033[031mFAILED: %s 0x%x\n", func, hr);
        return false;
    }
    if(!silent) wprintf(L"\033[032mOK: %s\n", func);
    return true;
}

int wmain(int argc, WCHAR** args)
{
    if (argc != 3 && argc != 4) return ERROR_INVALID_PARAMETER;

    IDOManager* pDoMgr                  = nullptr;
    IDODownload* pDoDnld                = nullptr;
    DO_DOWNLOAD_RANGE doRange           = { 0, DO_LENGTH_TO_EOF };
    DO_DOWNLOAD_RANGES_INFO doRangeInfo = { 1, doRange };
    DO_DOWNLOAD_STATUS doStatus         = {};
    BSTR bsUri                          = SysAllocString(args[1]);
    BSTR bsPath                         = SysAllocString(args[2]);
    BSTR bsDisplayName                  = SysAllocString(L"DoDl Download Task");
    VARIANT varUri                      = {};
    VARIANT varPath                     = {};
    VARIANT varDisplayName              = {};
    VARIANT varForegroundPriority       = {};
    VARIANT varNetworkToken             = {};

    varUri.vt = VT_BSTR;
    varUri.bstrVal = bsUri;
    varPath.vt = VT_BSTR;
    varPath.bstrVal = bsPath;
    varDisplayName.vt = VT_BSTR;
    varDisplayName.bstrVal = bsDisplayName;
    varForegroundPriority.vt = VT_BOOL;
    varForegroundPriority.boolVal = VARIANT_TRUE;
    varNetworkToken.vt = VT_BOOL;
    varNetworkToken.boolVal = (argc == 4) ? VARIANT_TRUE : VARIANT_FALSE;
    
    HRESULT hr = S_OK;
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (!CheckHResult(hr, L"CoInitializeEx")) goto Cleanup;
    hr = CoCreateInstance(CLSID_DeliveryOptimization, NULL, CLSCTX_LOCAL_SERVER, IID_IDOManager, (LPVOID*)&pDoMgr);
    if (!CheckHResult(hr, L"CoCreateInstance")) goto Cleanup;
    hr = pDoMgr->CreateDownload(&pDoDnld);
    if (!CheckHResult(hr, L"IDOManager->CreateDownload")) goto Cleanup;

    hr = CoSetProxyBlanket(pDoDnld, 
        RPC_C_AUTHN_DEFAULT,
        RPC_C_AUTHZ_NONE, 
        COLE_DEFAULT_PRINCIPAL, 
        RPC_C_AUTHN_LEVEL_DEFAULT, 
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_STATIC_CLOAKING);
    if (!CheckHResult(hr, L"CoSetProxyBlanket")) goto Cleanup;

    hr = pDoDnld->SetProperty(DODownloadProperty_Uri, &varUri);
    if (!CheckHResult(hr, L"IDODownload->SetProperty DODownloadProperty_Uri")) goto Cleanup;
    hr = pDoDnld->SetProperty(DODownloadProperty_DisplayName, &varDisplayName);
    if (!CheckHResult(hr, L"IDODownload->SetProperty DODownloadProperty_DisplayName")) goto Cleanup;
    hr = pDoDnld->SetProperty(DODownloadProperty_ForegroundPriority, &varForegroundPriority);
    if (!CheckHResult(hr, L"IDODownload->SetProperty DODownloadProperty_ForegroundPriority")) goto Cleanup;
    hr = pDoDnld->SetProperty(DODownloadProperty_LocalPath, &varPath);
    if (!CheckHResult(hr, L"IDODownload->SetProperty DODownloadProperty_LocalPath")) goto Cleanup;
    hr = pDoDnld->SetProperty(DODownloadProperty_NetworkToken, &varNetworkToken);
    if (!CheckHResult(hr, L"IDODownload->SetProperty DODownloadProperty_NetworkToken")) goto Cleanup;

    hr = pDoDnld->Start(&doRangeInfo);
    if (!CheckHResult(hr, L"IDODownload->Start")) goto Cleanup;

    while (true)
    {
        hr = pDoDnld->GetStatus(&doStatus);
        if (!CheckHResult(hr, L"IDODownload->GetStatus", true)) goto Cleanup;
        switch (doStatus.State)
        {
        case DODownloadState_Created:
            wprintf(L"State: Created\n");
            break;
        case DODownloadState_Transferring:
            wprintf(L"State: Transferring | ");
            wprintf(L"Progress: %llu/%llu\n", doStatus.BytesTransferred, doStatus.BytesTotal);
            break;
        case DODownloadState_Transferred:
            wprintf(L"State: Transferred\n");
            goto Finialize;
        default:
            wprintf(L"Unexpected State: %d\n", doStatus.State);
            wprintf(L"Error: 0x%x\n", doStatus.Error);
            wprintf(L"Extended Error: 0x%x\n", doStatus.ExtendedError);
            goto Cleanup;
        }
        Sleep(1000);
    }
Finialize:
    pDoDnld->Finalize();
    CheckHResult(hr, L"IDODownload->Finalize");

Cleanup:
    wprintf(L"\033[0m");
    pDoDnld->Release();
    pDoMgr->Release();
    SysFreeString(bsUri);
    SysFreeString(bsPath);
    SysFreeString(bsDisplayName);
    CoUninitialize();
    return hr;
}
