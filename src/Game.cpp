#include "common.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "Globals.cpp"

extern "C" __declspec(dllexport) void ProcessSimulation(const GameState* state)
{
    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(1.0f), state->time * glm::radians(100.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));

    ubo.view = state->Cam.GetViewMatrix();
    ubo.proj = state->Cam.GetProjectionMatrix();
    ubo.gamma = 1.0f / state->gamma;

    void* data;
    vkMapMemory(state->device, state->uniformBuffersMemory[state->imageIndex],
                0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(state->device, state->uniformBuffersMemory[state->imageIndex]);
}
