#include <Windows.h>
#include <Netcfgx.h>
#include <devguid.h>
#include <iostream>
#include <iomanip>

int main()
{
    INetCfg* pnetcfg = NULL;
    INetCfgComponent* pncfgcomp = NULL;
    IEnumNetCfgComponent* penumncfgcomp = NULL;
    HRESULT hr = S_OK;

    LPWSTR szwrClient = NULL;
    LPWSTR pszwDisplayName = new wchar_t[512];
    LPWSTR pszwId = new wchar_t[512];

    hr = CoInitialize(NULL);

    hr = CoCreateInstance(CLSID_CNetCfg, NULL, CLSCTX_SERVER, IID_INetCfg, (LPVOID*)&pnetcfg);
    hr = pnetcfg->Initialize(NULL);

    // Enum device class Network Adapter
    hr = pnetcfg->EnumComponents(&GUID_DEVCLASS_NET, &penumncfgcomp);
    while (penumncfgcomp->Next(1, &pncfgcomp, NULL) == S_OK)
    {
        pncfgcomp->GetDisplayName(&pszwDisplayName);
        pncfgcomp->GetId(&pszwId);
        std::wcout << L"Adapter: " << std::left << std::setw(48) << pszwDisplayName << L"Id: " << pszwId << std::endl;

        pncfgcomp->Release();
    }
    hr = pnetcfg->EnumComponents(&GUID_DEVCLASS_NETTRANS, &penumncfgcomp);
    while (penumncfgcomp->Next(1, &pncfgcomp, NULL) == S_OK)
    {
        pncfgcomp->GetDisplayName(&pszwDisplayName);
        pncfgcomp->GetId(&pszwId);
        std::wcout << L"Transport: " << std::left << std::setw(48) << pszwDisplayName << L"Id: " << pszwId << std::endl;

        pncfgcomp->Release();
    }
    hr = pnetcfg->EnumComponents(&GUID_DEVCLASS_NETSERVICE, &penumncfgcomp);
    while (penumncfgcomp->Next(1, &pncfgcomp, NULL) == S_OK)
    {
        pncfgcomp->GetDisplayName(&pszwDisplayName);
        pncfgcomp->GetId(&pszwId);
        std::wcout << L"Service: " << std::left << std::setw(48) << pszwDisplayName << L"Id: " << pszwId << std::endl;

        pncfgcomp->Release();
    }
    hr = pnetcfg->EnumComponents(&GUID_DEVCLASS_NETCLIENT, &penumncfgcomp);
    while (penumncfgcomp->Next(1, &pncfgcomp, NULL) == S_OK)
    {
        pncfgcomp->GetDisplayName(&pszwDisplayName);
        pncfgcomp->GetId(&pszwId);
        std::wcout << L"Client: " << std::left << std::setw(48) << pszwDisplayName << L"Id: " << pszwId << std::endl;

        pncfgcomp->Release();
    }

    hr = pnetcfg->Uninitialize();
    pnetcfg->Release();

    CoUninitialize();

    return 0;
}