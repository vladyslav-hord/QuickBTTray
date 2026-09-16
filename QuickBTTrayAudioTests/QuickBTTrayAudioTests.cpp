#include <windows.h>
#include <audioenginebaseapo.h>
#include <objbase.h>

#include <atomic>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

namespace
{
const CLSID kQuickBTTrayApoClsid =
{
    0x6f5c4e4a, 0x1e56, 0x4d3d,
    { 0x9a, 0x4f, 0x6e, 0x2c, 0x71, 0xb8, 0x93, 0x40 }
};

const CLSID kUnknownClsid =
{
    0x1d7b9a2e, 0x8e30, 0x4f22,
    { 0x91, 0x6f, 0x2a, 0x50, 0x4b, 0x7e, 0x1c, 0x63 }
};

typedef HRESULT (WINAPI *DllCanUnloadNowFn)();
typedef HRESULT (WINAPI *DllGetClassObjectFn)(REFCLSID, REFIID, LPVOID*);

std::wstring DefaultDllPath()
{
    wchar_t executablePath[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, executablePath,
        ARRAYSIZE(executablePath));
    if (length == 0 || length >= ARRAYSIZE(executablePath))
    {
        return {};
    }

    std::wstring path(executablePath, length);
    const std::wstring::size_type separator = path.find_last_of(L"\\/");
    if (separator == std::wstring::npos)
    {
        return {};
    }

    path.resize(separator + 1);
    path += L"..\\..\\..\\QuickBTTrayAudio\\x64\\Release\\QuickBTTrayAudio.dll";
    return path;
}

bool ExpectHr(const char* label, HRESULT actual, HRESULT expected)
{
    if (actual != expected)
    {
        std::printf("FAIL %s: got 0x%08lx, expected 0x%08lx\n",
            label,
            static_cast<unsigned long>(actual),
            static_cast<unsigned long>(expected));
        return false;
    }

    std::printf("PASS %s\n", label);
    return true;
}

bool ExpectSupported(const char* label, IUnknown* object, REFIID iid)
{
    IUnknown* interfacePointer = nullptr;
    const HRESULT result = object->QueryInterface(iid,
        reinterpret_cast<void**>(&interfacePointer));
    const bool passed = ExpectHr(label, result, S_OK) && interfacePointer != nullptr;
    if (!passed && result == S_OK)
    {
        std::printf("FAIL %s: returned a null interface pointer\n", label);
    }
    if (interfacePointer != nullptr)
    {
        interfacePointer->Release();
    }
    return passed;
}

bool RunLockServerLifetimeStress(IClassFactory* factory, DllCanUnloadNowFn canUnloadNow)
{
    constexpr int kSeedLocks = 1;
    constexpr int kWorkerCount = 64;
    constexpr int kOperationsPerWorker = 15000;

    for (int index = 0; index < kSeedLocks; ++index)
    {
        if (factory->LockServer(TRUE) != S_OK)
        {
            std::printf("FAIL stress setup LockServer(TRUE) at %d\n", index);
            return false;
        }
    }

    std::atomic<bool> start{ false };
    std::atomic<bool> unlockStart{ false };
    std::atomic<int> activeWorkers{ kWorkerCount * 2 };
    std::atomic<int> trueStarted{ 0 };
    std::atomic<int> falseStarted{ 0 };
    std::atomic<bool> workerFailed{ false };
    std::atomic<HRESULT> firstWorkerFailure{ S_OK };
    std::vector<std::thread> workers;
    workers.reserve(kWorkerCount * 2);

    const auto recordWorkerFailure = [&](HRESULT result)
    {
        HRESULT expected = S_OK;
        firstWorkerFailure.compare_exchange_strong(
            expected,
            result,
            std::memory_order_relaxed,
            std::memory_order_relaxed);
        workerFailed.store(true, std::memory_order_release);
    };

    for (int worker = 0; worker < kWorkerCount; ++worker)
    {
        workers.emplace_back([&]()
        {
            while (!start.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            for (int operation = 0; operation < kOperationsPerWorker; ++operation)
            {
                trueStarted.fetch_add(1, std::memory_order_release);
                const HRESULT result = factory->LockServer(TRUE);
                if (result != S_OK)
                {
                    recordWorkerFailure(result);
                }
                if ((operation & 0x3f) == 0)
                {
                    std::this_thread::yield();
                }
            }
            activeWorkers.fetch_sub(1, std::memory_order_release);
        });

        workers.emplace_back([&]()
        {
            while (!start.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }
            while (!unlockStart.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            for (int operation = 0; operation < kOperationsPerWorker; ++operation)
            {
                while (trueStarted.load(std::memory_order_acquire) + kSeedLocks
                    <= falseStarted.load(std::memory_order_relaxed))
                {
                    std::this_thread::yield();
                }

                falseStarted.fetch_add(1, std::memory_order_release);
                HRESULT result = E_UNEXPECTED;
                while (result == E_UNEXPECTED)
                {
                    result = factory->LockServer(FALSE);
                    if (result == E_UNEXPECTED)
                    {
                        std::this_thread::yield();
                    }
                }
                if (result != S_OK)
                {
                    recordWorkerFailure(result);
                }
                if ((operation & 0x3f) == 0)
                {
                    std::this_thread::yield();
                }
            }
            activeWorkers.fetch_sub(1, std::memory_order_release);
        });
    }

    start.store(true, std::memory_order_release);
    int unloadViolations = 0;
    while (trueStarted.load(std::memory_order_acquire) < kWorkerCount * 4)
    {
        if (canUnloadNow() != S_FALSE)
        {
            ++unloadViolations;
        }
        std::this_thread::yield();
    }
    unlockStart.store(true, std::memory_order_release);
    while (activeWorkers.load(std::memory_order_acquire) != 0)
    {
        if (canUnloadNow() != S_FALSE)
        {
            ++unloadViolations;
        }
        std::this_thread::yield();
    }

    for (auto& worker : workers)
    {
        worker.join();
    }

    bool passed = true;
    if (workerFailed.load(std::memory_order_acquire))
    {
        std::printf("FAIL stress LockServer operation returned 0x%08lx\n",
            static_cast<unsigned long>(firstWorkerFailure.load(std::memory_order_relaxed)));
        passed = false;
    }
    if (unloadViolations != 0)
    {
        std::printf("FAIL stress observed %d unloadable states while factory was live\n",
            unloadViolations);
        passed = false;
    }

    for (int index = 0; index < kSeedLocks; ++index)
    {
        if (factory->LockServer(FALSE) != S_OK)
        {
            std::printf("FAIL stress teardown LockServer(FALSE) at %d\n", index);
            passed = false;
            break;
        }
    }

    passed = ExpectHr("concurrent LockServer stress preserves active factory",
        canUnloadNow(), S_FALSE) && passed;
    return passed;
}
}

int wmain(int argc, wchar_t** argv)
{
    const std::wstring dllPath = argc > 1
        ? std::wstring(argv[1])
        : DefaultDllPath();
    if (dllPath.empty())
    {
        std::printf("FAIL could not determine the executable directory\n");
        return 1;
    }

    HMODULE module = LoadLibraryW(dllPath.c_str());
    if (module == nullptr)
    {
        const DWORD error = GetLastError();
        ::wprintf(L"EXPECTED FAILURE: DLL could not be loaded from %ls (Win32 error %lu)\n",
            dllPath.c_str(),
            static_cast<unsigned long>(error));
        return 1;
    }

    const auto canUnloadNow = reinterpret_cast<DllCanUnloadNowFn>(
        GetProcAddress(module, "DllCanUnloadNow"));
    const auto getClassObject = reinterpret_cast<DllGetClassObjectFn>(
        GetProcAddress(module, "DllGetClassObject"));
    if (canUnloadNow == nullptr || getClassObject == nullptr)
    {
        std::printf("FAIL required DLL export is missing\n");
        FreeLibrary(module);
        return 2;
    }

    bool passed = true;
    passed = ExpectHr("initial DllCanUnloadNow is S_OK",
        canUnloadNow(), S_OK) && passed;

    IClassFactory* factory = nullptr;
    passed = ExpectHr("unknown CLSID is rejected",
        getClassObject(kUnknownClsid, IID_IClassFactory,
            reinterpret_cast<void**>(&factory)),
        CLASS_E_CLASSNOTAVAILABLE) && passed;

    passed = ExpectHr("known CLSID returns a class factory",
        getClassObject(kQuickBTTrayApoClsid, IID_IClassFactory,
            reinterpret_cast<void**>(&factory)),
        S_OK) && passed;

    if (factory == nullptr)
    {
        std::printf("FAIL known CLSID returned a null factory\n");
        passed = false;
    }
    else
    {
        passed = ExpectHr("active factory prevents unloading",
            canUnloadNow(), S_FALSE) && passed;

        passed = RunLockServerLifetimeStress(factory, canUnloadNow) && passed;

        IUnknown* object = nullptr;
        passed = ExpectHr("class factory creates the APO",
            factory->CreateInstance(nullptr, IID_IUnknown,
                reinterpret_cast<void**>(&object)),
            S_OK) && passed;

        if (object == nullptr)
        {
            std::printf("FAIL created APO pointer is null\n");
            passed = false;
        }
        else
        {
            passed = ExpectHr("active object prevents unloading",
                canUnloadNow(), S_FALSE) && passed;
            passed = ExpectSupported("IAudioProcessingObject is supported", object,
                __uuidof(IAudioProcessingObject)) && passed;
            passed = ExpectSupported("IAudioProcessingObjectConfiguration is supported", object,
                __uuidof(IAudioProcessingObjectConfiguration)) && passed;
            passed = ExpectSupported("IAudioProcessingObjectRT is supported", object,
                __uuidof(IAudioProcessingObjectRT)) && passed;
            passed = ExpectSupported("IAudioSystemEffects is supported", object,
                __uuidof(IAudioSystemEffects)) && passed;

            IUnknown* unsupported = nullptr;
            passed = ExpectHr("IAudioSystemEffects2 is not exposed", object->QueryInterface(
                __uuidof(IAudioSystemEffects2), reinterpret_cast<void**>(&unsupported)),
                E_NOINTERFACE) && passed;
            if (unsupported != nullptr)
            {
                unsupported->Release();
                passed = false;
            }

            passed = ExpectHr("LockServer(TRUE) succeeds",
                factory->LockServer(TRUE), S_OK) && passed;
            passed = ExpectHr("LockServer(FALSE) matches the lock",
                factory->LockServer(FALSE), S_OK) && passed;
            passed = ExpectHr("matched server lock preserves active state",
                canUnloadNow(), S_FALSE) && passed;
            object->Release();
        }

        passed = ExpectHr("factory remains active after object release",
            canUnloadNow(), S_FALSE) && passed;
        factory->Release();
        passed = ExpectHr("released factory allows unloading",
            canUnloadNow(), S_OK) && passed;
    }

    factory = nullptr;
    passed = ExpectHr("factory for unmatched unlock test is created",
        getClassObject(kQuickBTTrayApoClsid, IID_IClassFactory,
            reinterpret_cast<void**>(&factory)),
        S_OK) && passed;
    if (factory == nullptr)
    {
        std::printf("FAIL unmatched unlock test factory is null\n");
        passed = false;
    }
    else
    {
        passed = ExpectHr("unmatched LockServer(FALSE) fails",
            factory->LockServer(FALSE), E_UNEXPECTED) && passed;
        passed = ExpectHr("unmatched unlock preserves non-unloadable state",
            canUnloadNow(), S_FALSE) && passed;
        factory->Release();
        passed = ExpectHr("unmatched unlock does not corrupt final unload state",
            canUnloadNow(), S_OK) && passed;
    }

    FreeLibrary(module);
    std::printf("%s\n", passed ? "NATIVE BEHAVIOR TEST PASSED" : "NATIVE BEHAVIOR TEST FAILED");
    return passed ? 0 : 3;
}
