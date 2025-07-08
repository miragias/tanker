#include "Core/HotReloading.h"

const char* DLL_PATH = "build_dll/Game.dll";

bool LoadSimulationDLL(HotReloadData& hotReload)
{
    CopyFileA(DLL_PATH, "build_dll/Game_temp.dll", FALSE);
    hotReload.DllHandle = LoadLibraryA("build_dll/Game_temp.dll");
    if (!hotReload.DllHandle)
    {
        MessageBoxA(0, "Failed to load DLL", "Error", MB_OK);
        return false;
    }

    hotReload.ProcessSimulation = (ProcessSimulationFn)GetProcAddress(hotReload.DllHandle, "ProcessSimulation");
    if (!hotReload.ProcessSimulation)
    {
        MessageBoxA(0, "Failed to find ProcessSimulation in DLL", "Error", MB_OK);
        return false;
    }

    return true;
}

bool HasDLLChanged(HotReloadData& hotReload)
{
  std::filesystem::file_time_type currentWriteTime = std::filesystem::last_write_time(DLL_PATH);

    if (currentWriteTime != hotReload.LastDLLWriteTime)
    {
        std::cout << "RELOADED DLL\n";
        hotReload.LastDLLWriteTime = currentWriteTime;
        return true;
    }

    return false;
}

void UnloadSimulationDLL(HotReloadData& hotReload)
{
    if (hotReload.DllHandle)
    {
        FreeLibrary(hotReload.DllHandle);
        hotReload.DllHandle = nullptr;
        hotReload.ProcessSimulation = nullptr;
    }
}

void TryHotReloadDLL(HotReloadData& hotReload)
{
    if (HasDLLChanged(hotReload))
    {
        UnloadSimulationDLL(hotReload);

        // Wait a bit to avoid race with compiler/linker still writing the DLL
        Sleep(100); // in ms

        if (!LoadSimulationDLL(hotReload))
        {
            MessageBoxA(0, "Reload failed", "Hot Reload", MB_OK);
        }
        else
        {
            OutputDebugStringA("Hot Reloaded Simulation DLL\n");
        }
    }
}

