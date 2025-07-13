#include "pch.h"
#include "shared.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

extern "C" __declspec(dllexport) void ProcessSimulation(const GameState* state)
{
    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(1.0f), state->time * glm::radians(100.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.view = state->Cam.GetViewMatrix();
    ubo.proj = state->Cam.GetProjectionMatrix();
    ubo.gamma = state->gamma;
  
    ubo.model = glm::mat4(1.0f);
    ubo.view = glm::mat4(1.0f);
    ubo.proj = glm::mat4(1.0f);

    for(auto s : state->uniformBufferMappedData)
    {
        // Instead of mapping every frame, memcpy directly to the persistently mapped memory pointer:
        memcpy(s[state->imageIndex], &ubo, sizeof(ubo));
    }
}
