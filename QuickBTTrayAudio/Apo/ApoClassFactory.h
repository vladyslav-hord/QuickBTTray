#pragma once

#include <windows.h>
#include <objbase.h>

namespace quickbttray::audio
{
extern volatile LONG g_serverObjectCount;

class ApoClassFactory final : public IClassFactory
{
public:
    ApoClassFactory() noexcept;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE CreateInstance(
        IUnknown* pUnkOuter,
        REFIID riid,
        void** ppvObject) override;
    HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock) override;

private:
    ~ApoClassFactory() noexcept;

    volatile LONG referenceCount_ = 1;
};

HRESULT CreateApoClassFactory(REFIID riid, void** ppvObject);
}
