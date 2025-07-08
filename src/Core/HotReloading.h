#pragma once
#include "shared.h"
#include <windows.h>  // Needed for HMODULE
#include <filesystem> // Needed for std::filesystem::file_time_type
#include "shared.h"   // Your own shared types

typedef void (*ProcessSimulationFn)(const GameState*);

struct HotReloadData
{
  GameState GameState;
  ProcessSimulationFn ProcessSimulation;
  HMODULE DllHandle;
  std::filesystem::file_time_type LastDLLWriteTime;
};
