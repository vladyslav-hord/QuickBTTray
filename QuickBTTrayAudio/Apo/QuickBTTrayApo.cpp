#include "QuickBTTrayApo.h"

#include <objbase.h>

namespace quickbttray::audio
{
extern volatile LONG g_serverObjectCount;

QuickBTTrayApo::QuickBTTrayApo() noexcept
{
    InterlockedIncrement(&g_serverObjectCount);
}

QuickBTTrayApo::~QuickBTTrayApo() noexcept
{
    InterlockedDecrement(&g_serverObjectCount);
}

HRESULT STDMETHODCALLTYPE QuickBTTrayApo::QueryInterface(REFIID riid, void** ppvObject)
{
    if (ppvObject == nullptr)
    {
        return E_POINTER;
    }
    *ppvObject = nullptr;

    if (riid == IID_IUnknown || riid == __uuidof(IAudioProcessingObject))
    {
        *ppvObject = static_cast<IAudioProcessingObject*>(this);
    }
    else if (riid == __uuidof(IAudioProcessingObjectConfiguration))
    {
        *ppvObject = static_cast<IAudioProcessingObjectConfiguration*>(this);
    }
    else if (riid == __uuidof(IAudioProcessingObjectRT))
    {
        *ppvObject = static_cast<IAudioProcessingObjectRT*>(this);
    }
    else if (riid == __uuidof(IAudioSystemEffects))
    {
        *ppvObject = static_cast<IAudioSystemEffects*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE QuickBTTrayApo::AddRef()
{
    return static_cast<ULONG>(InterlockedIncrement(&referenceCount_));
}

ULONG STDMETHODCALLTYPE QuickBTTrayApo::Release()
{
    const ULONG remaining = static_cast<ULONG>(InterlockedDecrement(&referenceCount_));
    if (remaining == 0)
    {
        delete this;
    }
    return remaining;
}

HRESULT STDMETHODCALLTYPE QuickBTTrayApo::Reset()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QuickBTTrayApo::GetLatency(HNSTIME* pTime)
{
    if (pTime == nullptr)
    {
        return E_POINTER;
    }
    *pTime = 0;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QuickBTTrayApo::GetRegistrationProperties(APO_REG_PROPERTIES** ppRegProps)
{
    if (ppRegProps == nullptr)
    {
        return E_POINTER;
    }
    *ppRegProps = nullptr;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QuickBTTrayApo::Initialize(UINT32 cbDataSize, BYTE* pbyData)
{
    if (cbDataSize != 0 && pbyData == nullptr)
    {
        return E_INVALIDARG;
    }
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QuickBTTrayApo::IsInputFormatSupported(
    IAudioMediaType* pOppositeFormat,
    IAudioMediaType* pRequestedInputFormat,
    IAudioMediaType** ppSupportedInputFormat)
{
    UNREFERENCED_PARAMETER(pOppositeFormat);
    UNREFERENCED_PARAMETER(pRequestedInputFormat);
    if (ppSupportedInputFormat == nullptr)
    {
        return E_POINTER;
    }
    *ppSupportedInputFormat = nullptr;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QuickBTTrayApo::IsOutputFormatSupported(
    IAudioMediaType* pOppositeFormat,
    IAudioMediaType* pRequestedOutputFormat,
    IAudioMediaType** ppSupportedOutputFormat)
{
    UNREFERENCED_PARAMETER(pOppositeFormat);
    UNREFERENCED_PARAMETER(pRequestedOutputFormat);
    if (ppSupportedOutputFormat == nullptr)
    {
        return E_POINTER;
    }
    *ppSupportedOutputFormat = nullptr;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QuickBTTrayApo::GetInputChannelCount(UINT32* pu32ChannelCount)
{
    if (pu32ChannelCount == nullptr)
    {
        return E_POINTER;
    }
    *pu32ChannelCount = 0;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QuickBTTrayApo::LockForProcess(
    UINT32 u32NumInputConnections,
    APO_CONNECTION_DESCRIPTOR** ppInputConnections,
    UINT32 u32NumOutputConnections,
    APO_CONNECTION_DESCRIPTOR** ppOutputConnections)
{
    UNREFERENCED_PARAMETER(u32NumInputConnections);
    UNREFERENCED_PARAMETER(ppInputConnections);
    UNREFERENCED_PARAMETER(u32NumOutputConnections);
    UNREFERENCED_PARAMETER(ppOutputConnections);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE QuickBTTrayApo::UnlockForProcess()
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE QuickBTTrayApo::APOProcess(
    UINT32 u32NumInputConnections,
    APO_CONNECTION_PROPERTY** ppInputConnections,
    UINT32 u32NumOutputConnections,
    APO_CONNECTION_PROPERTY** ppOutputConnections)
{
    UNREFERENCED_PARAMETER(u32NumInputConnections);
    UNREFERENCED_PARAMETER(ppInputConnections);
    UNREFERENCED_PARAMETER(u32NumOutputConnections);
    UNREFERENCED_PARAMETER(ppOutputConnections);
    // Phase 2 deliberately performs no audio processing or buffer copying.
}

UINT32 STDMETHODCALLTYPE QuickBTTrayApo::CalcInputFrames(UINT32 u32OutputFrameCount)
{
    UNREFERENCED_PARAMETER(u32OutputFrameCount);
    return 0;
}

UINT32 STDMETHODCALLTYPE QuickBTTrayApo::CalcOutputFrames(UINT32 u32InputFrameCount)
{
    UNREFERENCED_PARAMETER(u32InputFrameCount);
    return 0;
}
}
