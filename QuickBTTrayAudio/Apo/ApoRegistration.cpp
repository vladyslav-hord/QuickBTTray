#include "ApoRegistration.h"

#include "ApoClassFactory.h"

#include <objbase.h>

extern "C" HRESULT WINAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    void** ppv)
{
    if (ppv == nullptr)
    {
        return E_POINTER;
    }
    *ppv = nullptr;

    if (!IsEqualCLSID(rclsid, quickbttray::audio::kQuickBTTrayApoClsid))
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    return quickbttray::audio::CreateApoClassFactory(riid, ppv);
}

extern "C" HRESULT WINAPI DllCanUnloadNow()
{
    return InterlockedCompareExchange(&quickbttray::audio::g_serverObjectCount, 0, 0) == 0
        ? S_OK
        : S_FALSE;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(module);
    UNREFERENCED_PARAMETER(reserved);
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}
