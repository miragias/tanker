#pragma once

#include "common.h"

struct Camera
{
    glm::vec3 Position;
    glm::vec3 Target;
    glm::vec3 Up;
    float     FovRadians;
    float     AspectRatio;
    float     NearPlane;
    float     FarPlane;

    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(Position, Target, Up);
    }

    glm::mat4 GetProjectionMatrix() const
    {
        glm::mat4 proj = glm::perspective(FovRadians, AspectRatio, NearPlane, FarPlane);
        proj[1][1] *= -1;
        return proj;
    }
};

struct GameState
{
  float gammaValue = 1;
  float time;
  uint32_t imageIndex;
  float aspectRatio;
  float gamma;
  VkDevice device;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  VkExtent2D swapchainExtent;
  Camera Cam;
};
