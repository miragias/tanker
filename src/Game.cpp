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
    ubo.gamma = state->gammaValue;

    UniformBufferObject ubo2 = ubo;
    ubo2.model = glm::rotate(glm::mat4(1.0f), state->time * glm::radians(20.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));
  
    uint32 index = 0;
    for(auto s : state->uniformBufferMappedData)
    {
        // Instead of mapping every frame, memcpy directly to the persistently mapped memory pointer:
        if(index == 0){
            memcpy(s[state->imageIndex], &ubo, sizeof(ubo));
        }
        else{
            memcpy(s[state->imageIndex], &ubo2, sizeof(ubo));
        }
        index++;
    }
}
