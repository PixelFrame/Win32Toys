#include "pktmonapi.h"
#include <iostream>
#include <iomanip>
#include <combaseapi.h>
#include <ip2string.h>

#pragma comment(lib, "ntdll.lib")

using std::wcout;
using std::endl;

PACKETMONITOR_HANDLE hPktmon;
PACKETMONITOR_SESSION hSession;
PACKETMONITOR_REALTIME_STREAM hStream;

PacketMonitorInitialize pfn_PacketMonitorInitialize;
PacketMonitorUninitialize pfn_PacketMonitorUninitialize;
PacketMonitorEnumDataSources pfn_PacketMonitorEnumDataSources;
PacketMonitorCreateLiveSession pfn_PacketMonitorCreateLiveSession;
PacketMonitorCloseSessionHandle pfn_PacketMonitorCloseSessionHandle;
PacketMonitorAddCaptureConstraint pfn_PacketMonitorAddCaptureConstraint;
PacketMonitorAddSingleDataSourceToSession pfn_PacketMonitorAddSingleDataSourceToSession;
PacketMonitorCreateRealtimeStream pfn_PacketMonitorCreateRealtimeStream;
PacketMonitorCloseRealtimeStream pfn_PacketMonitorCloseRealtimeStream;
PacketMonitorAttachOutputToSession pfn_PacketMonitorAttachOutputToSession;
PacketMonitorSetSessionActive pfn_PacketMonitorSetSessionActive;

void PrintDataSources(PACKETMONITOR_DATA_SOURCE_LIST* pDataSourceList)
{
    wcout << L"Number of data sources: " << pDataSourceList->NumDataSources << endl;
    for (UINT32 i = 0; i < pDataSourceList->NumDataSources; ++i)
    {
        const PACKETMONITOR_DATA_SOURCE_SPECIFICATION* pDataSource = pDataSourceList->DataSources[i];
        wcout << L"Data Source " << i + 1 << L":" << endl;
        wcout << L"  ID: " << pDataSource->Id << endl;
        wcout << L"  Secondary ID: " << pDataSource->SecondaryId << endl;
        wcout << L"  Parent ID: " << pDataSource->ParentId << endl;
        wcout << L"  Kind: " << pDataSource->Kind << endl;
        wcout << L"  Name: " << pDataSource->Name << endl;
        wcout << L"  Description: " << pDataSource->Description << endl;

        if (pDataSource->IsPresent.Guid)
        {
            wchar_t* guidstr = new wchar_t[39];
            int strlen = StringFromGUID2(pDataSource->Detail.Guid, guidstr, 39);
            wcout << L"  GUID: " << guidstr << endl;
            delete[] guidstr;
        }
        else if (pDataSource->IsPresent.IpV4Address)
        {
            wcout << L"  IPv4 Address: "
                << pDataSource->Detail.IpAddress.IPv4_bytes[0] << L"."
                << pDataSource->Detail.IpAddress.IPv4_bytes[1] << L"."
                << pDataSource->Detail.IpAddress.IPv4_bytes[2] << L"."
                << pDataSource->Detail.IpAddress.IPv4_bytes[3]
                << endl;
        }
        else if (pDataSource->IsPresent.IpV6Address)
        {
            wchar_t* ipv6str = new wchar_t[46];
            RtlIpv6AddressToString((in6_addr*)&(pDataSource->Detail.IpAddress), ipv6str);
            wcout << L"  IPv6 Address: " << ipv6str << endl;
            delete[] ipv6str;
        }
        else if (pDataSource->IsPresent.MacAddress)
        {
            wcout << L"  MAC Address: " << std::hex
                << std::setw(2) << std::setfill(L'0') << pDataSource->Detail.MacAddress[0] << L":"
                << std::setw(2) << std::setfill(L'0') << pDataSource->Detail.MacAddress[1] << L":"
                << std::setw(2) << std::setfill(L'0') << pDataSource->Detail.MacAddress[2] << L":"
                << std::setw(2) << std::setfill(L'0') << pDataSource->Detail.MacAddress[3] << L":"
                << std::setw(2) << std::setfill(L'0') << pDataSource->Detail.MacAddress[4] << L":"
                << std::setw(2) << std::setfill(L'0') << pDataSource->Detail.MacAddress[5]
                << std::dec << endl;
        }
    }
}

void AddAllNetworkInterfaces(PACKETMONITOR_DATA_SOURCE_LIST* pDataSourceList)
{
    for (UINT32 i = 0; i < pDataSourceList->NumDataSources; ++i)
    {
        const PACKETMONITOR_DATA_SOURCE_SPECIFICATION* pDataSource = pDataSourceList->DataSources[i];
        pfn_PacketMonitorAddSingleDataSourceToSession(hSession, pDataSource);
    }
}

void StreamEventCallback(VOID* context, PACKETMONITOR_STREAM_EVENT_INFO const* StreamEventInfo, PACKETMONITOR_STREAM_EVENT_KIND eventKind)
{
    wcout << L"STREAM EVENT CALLBACK" << endl;
}

void StreamDataCallback(VOID* context, PACKETMONITOR_STREAM_DATA_DESCRIPTOR const* data)
{
    const BYTE* baseData = static_cast<const BYTE*>(data->Data);
    const PACKETMONITOR_STREAM_METADATA* metadata = reinterpret_cast<const PACKETMONITOR_STREAM_METADATA*>(baseData + data->MetadataOffset);

    wcout << L"STREAM DATA CALLBACK: "
        << L"PktGroupId: " << metadata->PktGroupId
        << L", PktCount: " << metadata->PktCount
        << L", AppearanceCount: " << metadata->AppearanceCount
        << L", DirectionName: " << metadata->DirectionName
        << L", PacketType: " << metadata->PacketType
        << L", ComponentId: " << metadata->ComponentId
        << L", EdgeId: " << metadata->EdgeId
        << endl;
}

int wmain()
{
    HRESULT hr = S_OK;
    SIZE_T buffSize = 0;
    PACKETMONITOR_DATA_SOURCE_LIST* pDataSourceList;
    PACKETMONITOR_REALTIME_STREAM_CONFIGURATION streamCfg = {};

    try
    {
        pfn_PacketMonitorInitialize = (PacketMonitorInitialize)GetPktmonAPI("PacketMonitorInitialize");
        pfn_PacketMonitorUninitialize = (PacketMonitorUninitialize)GetPktmonAPI("PacketMonitorUninitialize");
        pfn_PacketMonitorEnumDataSources = (PacketMonitorEnumDataSources)GetPktmonAPI("PacketMonitorEnumDataSources");
        pfn_PacketMonitorCreateLiveSession = (PacketMonitorCreateLiveSession)GetPktmonAPI("PacketMonitorCreateLiveSession");
        pfn_PacketMonitorCloseSessionHandle = (PacketMonitorCloseSessionHandle)GetPktmonAPI("PacketMonitorCloseSessionHandle");
        pfn_PacketMonitorAddCaptureConstraint = (PacketMonitorAddCaptureConstraint)GetPktmonAPI("PacketMonitorAddCaptureConstraint");
        pfn_PacketMonitorAddSingleDataSourceToSession = (PacketMonitorAddSingleDataSourceToSession)GetPktmonAPI("PacketMonitorAddSingleDataSourceToSession");
        pfn_PacketMonitorCreateRealtimeStream = (PacketMonitorCreateRealtimeStream)GetPktmonAPI("PacketMonitorCreateRealtimeStream");
        pfn_PacketMonitorCloseRealtimeStream = (PacketMonitorCloseRealtimeStream)GetPktmonAPI("PacketMonitorCloseRealtimeStream");
        pfn_PacketMonitorAttachOutputToSession = (PacketMonitorAttachOutputToSession)GetPktmonAPI("PacketMonitorAttachOutputToSession");
        pfn_PacketMonitorSetSessionActive = (PacketMonitorSetSessionActive)GetPktmonAPI("PacketMonitorSetSessionActive");
    }
    catch (std::runtime_error& e)
    {
        wcout << L"Error: " << e.what() << endl;
        return -1;
    }

    hr = pfn_PacketMonitorInitialize(PACKETMONITOR_API_VERSION_1_0, nullptr, &hPktmon);
    if (FAILED(hr))
    {
        wcout << L"Failed to initialize Packet Monitor API: " << hr << endl;
        return -1;
    }

    pfn_PacketMonitorEnumDataSources(hPktmon, PACKETMONITOR_DATA_SOURCE_KIND::PacketMonitorDataSourceKindNetworkInterface,
        FALSE, 0, &buffSize, nullptr);
    pDataSourceList = (PACKETMONITOR_DATA_SOURCE_LIST*)malloc(buffSize);
    if (pDataSourceList == nullptr)
    {
        wcout << L"Failed to allocate memory for data source list." << endl;
        pfn_PacketMonitorUninitialize(hPktmon);
        return -1;
    }
    hr = pfn_PacketMonitorEnumDataSources(hPktmon, PACKETMONITOR_DATA_SOURCE_KIND::PacketMonitorDataSourceKindNetworkInterface,
        FALSE, buffSize, &buffSize, pDataSourceList);
    if (FAILED(hr))
    {
        wcout << L"Failed to enumerate data sources: " << hr << endl;
        free(pDataSourceList);
        pfn_PacketMonitorUninitialize(hPktmon);
        return -1;
    }
    PrintDataSources(pDataSourceList);

    hr = pfn_PacketMonitorCreateLiveSession(hPktmon, L"TestSession", &hSession);
    if (FAILED(hr))
    {
        wcout << L"Failed to create live session: " << hr << endl;
        free(pDataSourceList);
        pfn_PacketMonitorUninitialize(hPktmon);
        return -1;
    }
    AddAllNetworkInterfaces(pDataSourceList);

    streamCfg.EventCallback = StreamEventCallback;
    streamCfg.DataCallback = StreamDataCallback;

    hr = pfn_PacketMonitorCreateRealtimeStream(hPktmon, &streamCfg, &hStream);
    if (FAILED(hr))
    {
        wcout << L"Failed to create realtime stream: " << hr << endl;
        free(pDataSourceList);
        pfn_PacketMonitorCloseSessionHandle(hSession);
        pfn_PacketMonitorUninitialize(hPktmon);
        return -1;
    }

    hr = pfn_PacketMonitorAttachOutputToSession(hSession, hStream);
    if (FAILED(hr))
    {
        wcout << L"Failed to attach output: " << hr << endl;
        free(pDataSourceList);
        pfn_PacketMonitorCloseRealtimeStream(hStream);
        pfn_PacketMonitorCloseSessionHandle(hSession);
        pfn_PacketMonitorUninitialize(hPktmon);
        return -1;
    }

    wcout << L"Starting session..." << endl;
    hr = pfn_PacketMonitorSetSessionActive(hSession, TRUE);
    if (FAILED(hr))
    {
        wcout << L"Failed to start session: " << hr << endl;
        free(pDataSourceList);
        pfn_PacketMonitorCloseRealtimeStream(hStream);
        pfn_PacketMonitorCloseSessionHandle(hSession);
        pfn_PacketMonitorUninitialize(hPktmon);
        return -1;
    }

    Sleep(5000);

    wcout << L"Ending session..." << endl;
    pfn_PacketMonitorSetSessionActive(hSession, FALSE);
    if (FAILED(hr))
    {
        wcout << L"Failed to end session: " << hr << endl;
        free(pDataSourceList);
        pfn_PacketMonitorCloseRealtimeStream(hStream);
        pfn_PacketMonitorCloseSessionHandle(hSession);
        pfn_PacketMonitorUninitialize(hPktmon);
        return -1;
    }

    free(pDataSourceList);
    pfn_PacketMonitorCloseRealtimeStream(hStream);
    pfn_PacketMonitorCloseSessionHandle(hSession);
    pfn_PacketMonitorUninitialize(hPktmon);

    return 0;
}