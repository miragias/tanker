#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "Globals.cpp"

extern "C" __declspec(dllexport) void ProcessSimulation(const SimulationInput* input)
{
    Camera camera;
    camera.Position     = glm::vec3(0.0f, 4.0f, -10.0f);
    camera.Target       = glm::vec3(0.0f, -1.0f, 0.0f);
    camera.Up           = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.FovRadians   = input->fovRadians;
    camera.AspectRatio  = input->aspectRatio;
    camera.NearPlane    = 0.1f;
    camera.FarPlane     = 1000.0f;

    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(1.0f), input->time * glm::radians(60.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));

    ubo.view = camera.GetViewMatrix();
    ubo.proj = camera.GetProjectionMatrix();
    ubo.gamma = 1.0f / input->gamma;

    void* data;
    vkMapMemory(input->device, input->uniformBuffersMemory[input->imageIndex],
                0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(input->device, input->uniformBuffersMemory[input->imageIndex]);
}
