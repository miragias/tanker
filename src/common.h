#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define IMGUI_DEFINE_MATH_OPERATORS

#include <Windows.h>
#include <GLFW/glfw3.h>

// Standard headers
#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <set>
#include <map>
#include <optional>
#include <functional>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <algorithm>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

//My stuff
#include "Core/HotReloading.h"
//

// Types
typedef uint32_t uint32;
using vec3 = glm::vec3;
using vec2 = glm::vec2;
