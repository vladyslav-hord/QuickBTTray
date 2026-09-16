# QuickBTTrayAudio native skeleton

This phase contains an original, unregistered x64 COM APO skeleton only. It has no INF, registry writes, endpoint attachment, activation path, DSP, or audio pass-through behavior.

Build the native projects with the verified Visual Studio Build Tools installation. These projects are intentionally validated separately from the managed solution: `dotnet build QuickBTTray.sln -c Release --no-restore` validates the managed application only and does not validate the native projects. Phase 2 validation is incomplete unless both native MSBuild invocations and the behavior test below pass:

```text
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" QuickBTTrayAudio\QuickBTTrayAudio.vcxproj /m /t:Rebuild /p:Configuration=Release /p:Platform=x64
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" QuickBTTrayAudioTests\QuickBTTrayAudioTests.vcxproj /m /t:Rebuild /p:Configuration=Release /p:Platform=x64
QuickBTTrayAudioTests\x64\Release\QuickBTTrayAudioTests.exe
QuickBTTrayAudioTests\x64\Release\QuickBTTrayAudioTests.exe QuickBTTrayAudio\x64\Release\QuickBTTrayAudio.dll
```

The DLL exports only `DllGetClassObject` and `DllCanUnloadNow`. The test executable has no third-party or project-library dependencies and validates dynamic loading, CLSID dispatch, COM creation, and interface discovery.
