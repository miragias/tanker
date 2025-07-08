#pragma once
#include "pch.h"
#include "shared.h"

typedef void (*ProcessSimulationFn)(const GameState*);

struct HotReloadData
{
  GameState GameState;
  ProcessSimulationFn ProcessSimulation;
  HMODULE DllHandle;
  std::filesystem::file_time_type LastDLLWriteTime;
};
