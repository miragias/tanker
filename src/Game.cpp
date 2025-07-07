#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>
#include "Globals.cpp"

extern "C" __declspec(dllexport) void ProcessSimulation(const SimulationInput* input)
{
    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(1.0f), input->time * glm::radians(60.0f),
                            glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
                           glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(input->fovRadians,
                                input->aspectRatio,
                                0.1f, 10.0f);
    /*
    ubo.model = glm::rotate(glm::mat4(1.0f), input->time * glm::radians(60.0f),
                            glm::vec3(1.0f, 0.0f, 0.0f));
    */

    ubo.proj[1][1] *= -1;
    ubo.gamma = 1 / input->gamma;

    void* data;
    vkMapMemory(input->device, input->uniformBuffersMemory[input->imageIndex],
                0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(input->device, input->uniformBuffersMemory[input->imageIndex]);
}
