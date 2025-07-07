#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "Globals.cpp"

extern "C" __declspec(dllexport) void ProcessSimulation(const SimulationInput* inp)
{
    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(1.0f), inp->time * glm::radians(60.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));

    ubo.view = inp->Cam.GetViewMatrix();
    ubo.proj = inp->Cam.GetProjectionMatrix();
    ubo.gamma = 1.0f / inp->gamma;

    void* data;
    vkMapMemory(inp->device, inp->uniformBuffersMemory[inp->imageIndex],
                0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(inp->device, inp->uniformBuffersMemory[inp->imageIndex]);
}
