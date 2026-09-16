#pragma once

#include <windows.h>

namespace quickbttray::audio
{
// Private project identity. It is intentionally not registered by this phase.
inline const CLSID kQuickBTTrayApoClsid =
{
    0x6f5c4e4a, 0x1e56, 0x4d3d,
    { 0x9a, 0x4f, 0x6e, 0x2c, 0x71, 0xb8, 0x93, 0x40 }
};
}

extern "C" HRESULT WINAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    void** ppv);

extern "C" HRESULT WINAPI DllCanUnloadNow();
