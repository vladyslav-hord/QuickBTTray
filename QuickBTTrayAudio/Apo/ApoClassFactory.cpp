#include "ApoClassFactory.h"

#include "ApoRegistration.h"
#include "QuickBTTrayApo.h"

#include <new>

namespace quickbttray::audio
{
volatile LONG g_serverObjectCount = 0;
volatile LONG g_serverLockCount = 0;

ApoClassFactory::ApoClassFactory() noexcept
{
    InterlockedIncrement(&g_serverObjectCount);
}

ApoClassFactory::~ApoClassFactory() noexcept
{
    InterlockedDecrement(&g_serverObjectCount);
}

HRESULT STDMETHODCALLTYPE ApoClassFactory::QueryInterface(REFIID riid, void** ppvObject)
{
    if (ppvObject == nullptr)
    {
        return E_POINTER;
    }
    *ppvObject = nullptr;

    if (riid == IID_IUnknown || riid == IID_IClassFactory)
    {
        *ppvObject = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE ApoClassFactory::AddRef()
{
    return static_cast<ULONG>(InterlockedIncrement(&referenceCount_));
}

ULONG STDMETHODCALLTYPE ApoClassFactory::Release()
{
    const ULONG remaining = static_cast<ULONG>(InterlockedDecrement(&referenceCount_));
    if (remaining == 0)
    {
        delete this;
    }
    return remaining;
}

HRESULT STDMETHODCALLTYPE ApoClassFactory::CreateInstance(
    IUnknown* pUnkOuter,
    REFIID riid,
    void** ppvObject)
{
    if (ppvObject == nullptr)
    {
        return E_POINTER;
    }
    *ppvObject = nullptr;
    if (pUnkOuter != nullptr)
    {
        return CLASS_E_NOAGGREGATION;
    }

    auto* object = new (std::nothrow) QuickBTTrayApo();
    if (object == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    const HRESULT result = object->QueryInterface(riid, ppvObject);
    object->Release();
    return result;
}

HRESULT STDMETHODCALLTYPE ApoClassFactory::LockServer(BOOL fLock)
{
    if (fLock != FALSE)
    {
        InterlockedIncrement(&g_serverObjectCount);
        InterlockedIncrement(&g_serverLockCount);
    }
    else
    {
        LONG lockCount = InterlockedCompareExchange(&g_serverLockCount, 0, 0);
        while (lockCount > 0)
        {
            const LONG previousLockCount = InterlockedCompareExchange(
                &g_serverLockCount, lockCount - 1, lockCount);
            if (previousLockCount == lockCount)
            {
                InterlockedDecrement(&g_serverObjectCount);
                return S_OK;
            }
            lockCount = previousLockCount;
        }
        return E_UNEXPECTED;
    }
    return S_OK;
}

HRESULT CreateApoClassFactory(REFIID riid, void** ppvObject)
{
    if (ppvObject == nullptr)
    {
        return E_POINTER;
    }
    *ppvObject = nullptr;

    auto* factory = new (std::nothrow) ApoClassFactory();
    if (factory == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    const HRESULT result = factory->QueryInterface(riid, ppvObject);
    factory->Release();
    return result;
}
}
