#define IMGUI_DEFINE_MATH_OPERATORS
#define GLFW_INCLUDE_VULKAN
#define mut
#include <Windows.h>
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>
typedef uint32_t uint32;

#include "Rendering/VulkanInternals.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Other/tiny_obj_loader.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>
#define GLM_ENABLE_EXPERIMENTAL
#include <array>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "Other/stb_image.h"

#include <chrono>

#define VMA_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4100)
#pragma warning(disable : 4324)
#pragma warning(disable:4189)
#pragma warning(disable: 4127)
#include "vk_mem_alloc.h"
#pragma warning(pop)

const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct GameState
{
  float someV = 90.0f;
  float gammaValue = 1;
};

GameState State;

#include "Globals.cpp"
#include "Rendering/VulkanSetup.cpp"
#include "ImguiOverlay.cpp"
#include "Rendering/NativeWindow.cpp"

struct Vertex 
{
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 texCoord;

  static VkVertexInputBindingDescription getBindingDescription() 
  {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  bool operator==(const Vertex &other) const 
  {
    return pos == other.pos && color == other.color &&
           texCoord == other.texCoord;
  }

  static std::array<VkVertexInputAttributeDescription, 3>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
  }
};

namespace std 
{
template <> struct hash<Vertex> {
  size_t operator()(Vertex const &vertex) const {
    return ((hash<glm::vec3>()(vertex.pos) ^
             (hash<glm::vec3>()(vertex.color) << 1)) >>
            1) ^
           (hash<glm::vec2>()(vertex.texCoord) << 1);
  }
};
} // namespace std

struct UniformBufferObject 
{
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
  float gamma;
};

const int NUMBER_OF_IMAGES = 2;

const std::string MODEL_PATH = "models/skull.obj";
const std::string MODEL2_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "models/viking_room.png";

std::vector<Vertex> m_ModelVertexes;
std::vector<uint32_t> m_VertexIndices;
std::vector<Vertex> m_Model2Vertexes;
std::vector<uint32_t> m_Vertex2Indices;

VulkanContext VContext;

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

struct SwapChain
{
};

// Swapchain(I think it can go with the above)
VkSwapchainKHR m_SwapChain;
VkFormat m_SwapChainImageFormat;
VkExtent2D m_SwapChainExtent;
std::vector<VkImage> m_SwapChainImages;
std::vector<VkImageView> m_SwapChainImageViews;
std::vector<VkFramebuffer> m_SwapChainFrameBuffers;


VkDescriptorPool m_DescriptorPool;
std::vector<VkDescriptorSet> m_DescriptorSets;
VkRenderPass m_RenderPass;
VkDescriptorSetLayout m_DescriptorSetLayout;
VkPipelineLayout m_PipelineLayout;
VkPipeline m_GraphicsPipeline;
VkCommandPool m_CommandPool;

std::vector<VkBuffer> m_UniformBuffers;
VkBuffer m_VertexBuffer;
VkBuffer m_IndexBuffer;

VmaAllocation m_VertexBufferAllocation;
VmaAllocation m_IndexBufferAllocation;
VkDeviceMemory m_IndexBufferMemory;
std::vector<VkDeviceMemory> m_UniformBuffersMemory;

VkImage m_TextureImage[NUMBER_OF_IMAGES];
VkDeviceMemory m_TextureImageMemory[NUMBER_OF_IMAGES];
VkImageView m_TextureImageView[NUMBER_OF_IMAGES];
VkSampler m_TextureSampler[NUMBER_OF_IMAGES];

VkImage m_DepthImage;
VkDeviceMemory m_DepthImageMemory;
VkImageView m_DepthImageView;

uint32_t m_ImageIndex = 0;

VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
std::array<VkClearValue, 2> m_ClearValues = {};

uint32_t g_QueueFamily = (uint32_t)-1;
std::vector<VkCommandBuffer> m_CommandBuffers;
std::vector<VkSemaphore> m_ImageAvailableSemaphores;
std::vector<VkSemaphore> m_RenderFinishedSemaphores;
std::vector<VkFence> m_InFlightFences;
std::vector<VkFence> m_ImagesInFlight;

VmaAllocator m_Allocator;

size_t m_CurrentFrame = 0;


VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  // Check if khr allows us whatever format
  if (availableFormats.size() == 1 &&
      availableFormats[0].format == VK_FORMAT_UNDEFINED) {
    return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  }
  // If not check all available formats and return the one that we want if we
  // find it.
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  // If all else fails just return the very first available one
  return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> availablePresentModes) {
  VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      bestMode = availablePresentMode;
    }
  }

  return bestMode;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) 
{
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) 
  {
    return capabilities.currentExtent;
  }
  else 
  {
    int width, height;
    glfwGetFramebufferSize(VContext.m_Window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                                static_cast<uint32_t>(height)};

    actualExtent.width = std::max(
        capabilities.minImageExtent.width,
        std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(
        capabilities.minImageExtent.height,
        std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
  }
}

void createSwapChain(VulkanContext vulkanContext) 
{
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(vulkanContext.m_PhysicalDevice, vulkanContext.m_Surface);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

  uint32 imageCount = swapChainSupport.capabilities.minImageCount + 1;

  // 0 Here means there is no maximum
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) 
  {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = vulkanContext.m_Surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32 queueFamilyIndices[] = 
  {
    vulkanContext.m_QueueFamilyIndicesUsed.graphicsFamily.value(),
    vulkanContext.m_QueueFamilyIndicesUsed.presentFamily.value()
  };

  if (vulkanContext.m_QueueFamilyIndicesUsed.graphicsFamily != vulkanContext.m_QueueFamilyIndicesUsed.presentFamily) 
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else 
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  if (vkCreateSwapchainKHR(vulkanContext.m_Device, &createInfo, nullptr, &m_SwapChain) !=
      VK_SUCCESS) 
  {
    throw std::runtime_error("failed to create swap chain!");
  }

  vkGetSwapchainImagesKHR(vulkanContext.m_Device, m_SwapChain, &imageCount, nullptr);
  m_SwapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(vulkanContext.m_Device, m_SwapChain, &imageCount,
                          m_SwapChainImages.data());

  m_SwapChainImageFormat = surfaceFormat.format;
  m_SwapChainExtent = extent;
}


void cleanupSwapChain(VulkanContext vulkanContext) 
{
  for (size_t i = 0; i < m_SwapChainFrameBuffers.size(); i++) {
    vkDestroyFramebuffer(vulkanContext.m_Device, m_SwapChainFrameBuffers[i], nullptr);
  }

  vkFreeCommandBuffers(vulkanContext.m_Device, m_CommandPool,
                        static_cast<uint32_t>(m_CommandBuffers.size()),
                        m_CommandBuffers.data());

  vkDestroyPipeline(vulkanContext.m_Device, m_GraphicsPipeline, nullptr);
  vkDestroyPipelineLayout(vulkanContext.m_Device, m_PipelineLayout, nullptr);
  vkDestroyRenderPass(vulkanContext.m_Device, m_RenderPass, nullptr);

  for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
    vkDestroyImageView(vulkanContext.m_Device, m_SwapChainImageViews[i], nullptr);
  }

  vkDestroyImageView(vulkanContext.m_Device, m_DepthImageView, nullptr);

  vkDestroySwapchainKHR(vulkanContext.m_Device, m_SwapChain, nullptr);

  for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
    vkDestroyBuffer(vulkanContext.m_Device, m_UniformBuffers[i], nullptr);
    vkFreeMemory(vulkanContext.m_Device, m_UniformBuffersMemory[i], nullptr);
  }

  vkDestroyDescriptorPool(vulkanContext.m_Device, m_DescriptorPool, nullptr);
}

VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkDevice device) 
{
  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView imageView;
  if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image view!");
  }

  return imageView;
}


void createImageViews(VkDevice device) 
{
  m_SwapChainImageViews.resize(m_SwapChainImages.size());

  for (uint32_t i = 0; i < m_SwapChainImages.size(); i++) {
    m_SwapChainImageViews[i] =
        createImageView(m_SwapChainImages[i], m_SwapChainImageFormat,
                        VK_IMAGE_ASPECT_COLOR_BIT, device);
  }
}

VkShaderModule createShaderModule(const std::vector<char> &code) {
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(VContext.m_Device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }
  return shaderModule;
}

VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                              VkImageTiling tiling,
                              VkFormatFeatureFlags features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(VContext.m_PhysicalDevice, format, &props);
    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("failed to find supported format!");
}


VkFormat findDepthFormat() {
  return findSupportedFormat({VK_FORMAT_D32_SFLOAT,
                              VK_FORMAT_D32_SFLOAT_S8_UINT,
                              VK_FORMAT_D24_UNORM_S8_UINT},
                              VK_IMAGE_TILING_OPTIMAL,
                              VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}


void createRenderPass() {
  // Color attachment
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = m_SwapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // Depth attachment
  VkAttachmentDescription depthAttachment = {};
  depthAttachment.format = findDepthFormat();
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  std::array<VkAttachmentDescription, 2> attachments = {colorAttachment,
                                                        depthAttachment};
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(VContext.m_Device, &renderPassInfo, nullptr, &m_RenderPass) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
  }
}

void createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 2;
  samplerLayoutBinding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
      uboLayoutBinding, samplerLayoutBinding};

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(VContext.m_Device, &layoutInfo, nullptr,
                                  &m_DescriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

static std::vector<char> readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }
  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}


void createGraphicsPipeline() {
  auto vertShaderCode = readFile("shaders/vert.spv");
  auto fragShaderCode = readFile("shaders/frag.spv");

  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)m_SwapChainExtent.width;
  viewport.height = (float)m_SwapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = m_SwapChainExtent;

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;

  if (vkCreatePipelineLayout(VContext.m_Device, &pipelineLayoutInfo, nullptr,
                              &m_PipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkPipelineDepthStencilStateCreateInfo depthStencil = {};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = m_PipelineLayout;
  pipelineInfo.renderPass = m_RenderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.pDepthStencilState = &depthStencil;

  if (vkCreateGraphicsPipelines(VContext.m_Device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &m_GraphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(VContext.m_Device, fragShaderModule, nullptr);
  vkDestroyShaderModule(VContext.m_Device, vertShaderModule, nullptr);
}

void createFramebuffers() {
  m_SwapChainFrameBuffers.resize(m_SwapChainImageViews.size());
  for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
    std::array<VkImageView, 2> attachments = {m_SwapChainImageViews[i],
                                              m_DepthImageView};

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = m_RenderPass;
    framebufferInfo.attachmentCount =
        static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = m_SwapChainExtent.width;
    framebufferInfo.height = m_SwapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(VContext.m_Device, &framebufferInfo, nullptr,
                            &m_SwapChainFrameBuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void createCommandPool(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDevice device) 
{
  QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  // Is this really needed?
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) 
  {
    throw std::runtime_error("failed to create command pool!");
  }
}

uint32_t findMemoryType(uint32_t typeFilter,
                        VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(VContext.m_PhysicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      return i;
    }
  }
  throw std::runtime_error("failed to find suitable memory type!");
}


void createImage(uint32_t width, uint32_t height, VkFormat format,
                  VkImageTiling tiling, VkImageUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkImage &image,
                  VkDeviceMemory &imageMemory) {
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(VContext.m_Device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(VContext.m_Device, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(VContext.m_Device, &allocInfo, nullptr, &imageMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
  }

  vkBindImageMemory(VContext.m_Device, image, imageMemory, 0);
}


void createDepthResources(VkDevice device) 
{
  VkFormat depthFormat = findDepthFormat();

  createImage( m_SwapChainExtent.width, m_SwapChainExtent.height, depthFormat,
      VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);

  m_DepthImageView = createImageView(m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, device);
}

bool hasStencilComponent(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
          format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer &buffer,
                  VkDeviceMemory &bufferMemory) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(VContext.m_Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(VContext.m_Device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(VContext.m_Device, &allocInfo, nullptr, &bufferMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate buffer memory!");
  }

  vkBindBufferMemory(VContext.m_Device, buffer, bufferMemory, 0);
}

VkCommandBuffer beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = m_CommandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(VContext.m_Device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(VContext.m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(VContext.m_GraphicsQueue);

  vkFreeCommandBuffers(VContext.m_Device, m_CommandPool, 1, &commandBuffer);
}


// TODO(JohnMir): What does this do?
void transitionImageLayout(VkImage image, VkFormat format,
                            VkImageLayout oldLayout, VkImageLayout newLayout) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
              newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                        nullptr, 0, nullptr, 1, &barrier);
  endSingleTimeCommands(commandBuffer);
}

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                        uint32_t height) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region = {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  endSingleTimeCommands(commandBuffer);
}


void createTextureImage() {
  for (int i = 0; i < NUMBER_OF_IMAGES; ++i) {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight,
                                &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
      throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory);

    void *data;
    vkMapMemory(VContext.m_Device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(VContext.m_Device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage[i],
                m_TextureImageMemory[i]);

    transitionImageLayout(m_TextureImage[i], VK_FORMAT_R8G8B8A8_UNORM,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, m_TextureImage[i],
                      static_cast<uint32_t>(texWidth),
                      static_cast<uint32_t>(texHeight));
    transitionImageLayout(m_TextureImage[i], VK_FORMAT_R8G8B8A8_UNORM,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(VContext.m_Device, stagingBuffer, nullptr);

    vkFreeMemory(VContext.m_Device, stagingBufferMemory, nullptr);
  }
}

void createDescriptorPool() {
  std::array<VkDescriptorPoolSize, 3> poolSizes = {};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount =
      static_cast<uint32_t>(m_SwapChainImages.size());
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount =
      static_cast<uint32_t>(m_SwapChainImages.size()) * 2;
  // Imgui descriptorset
  poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[2].descriptorCount =
      static_cast<uint32_t>(m_SwapChainImages.size());

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(m_SwapChainImages.size() + 1);
  ;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

  if (vkCreateDescriptorPool(VContext.m_Device, &poolInfo, nullptr,
                              &m_DescriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void createDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(m_SwapChainImages.size(),
                                              m_DescriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = m_DescriptorPool;
  allocInfo.descriptorSetCount =
      static_cast<uint32_t>(m_SwapChainImages.size());
  allocInfo.pSetLayouts = layouts.data();

  m_DescriptorSets.resize(m_SwapChainImages.size());
  if (vkAllocateDescriptorSets(VContext.m_Device, &allocInfo,
                                m_DescriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = m_UniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo[2] = {};
    imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo[0].imageView = m_TextureImageView[0];
    imageInfo[0].sampler = m_TextureSampler[0];
    imageInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo[1].imageView = m_TextureImageView[1];
    imageInfo[1].sampler = m_TextureSampler[1];

    std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_DescriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = m_DescriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 2;
    descriptorWrites[1].pImageInfo = imageInfo;

    vkUpdateDescriptorSets(VContext.m_Device,
                            static_cast<uint32_t>(descriptorWrites.size()),
                            descriptorWrites.data(), 0, nullptr);
  }
}

void createCommandBuffers() {
  m_CommandBuffers.resize(m_SwapChainFrameBuffers.size());

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_CommandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

  m_ClearValues[0].color = {0.f, 0.f, 0.f, 1.f};
  m_ClearValues[1].depthStencil = {1.f, 0};

  if (vkAllocateCommandBuffers(VContext.m_Device, &allocInfo,
                                m_CommandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
}

void createTextureImageView(VkDevice device) 
{
  for (int i = 0; i < NUMBER_OF_IMAGES; i++) {
    m_TextureImageView[i] =
        createImageView(m_TextureImage[i], VK_FORMAT_R8G8B8A8_UNORM,
                        VK_IMAGE_ASPECT_COLOR_BIT, device);
  }
}

void createTextureSampler() {
  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = 1;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  for (int i = 0; i < NUMBER_OF_IMAGES; ++i) {
    if (vkCreateSampler(VContext.m_Device, &samplerInfo, nullptr,
                        &m_TextureSampler[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create texture sampler!");
    }
  }
}

void loadModel(bool second) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (second) {
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          MODEL2_PATH.c_str())) {
      throw std::runtime_error(warn + err);
    }
  } else {
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          MODEL_PATH.c_str())) {
      throw std::runtime_error(warn + err);
    }
  }

  std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      Vertex vertex = {};

      vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};
      if (second) {
        vertex.pos += glm::vec3(1, 1, 1);
      }

      vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                          1.0f -
                              attrib.texcoords[2 * index.texcoord_index + 1]};

      vertex.color = {1.0f, 1.0f, 1.0f};

      if (second) {
        if (uniqueVertices.count(vertex) == 0) {
          uniqueVertices[vertex] =
              static_cast<uint32_t>(m_Model2Vertexes.size());
          m_Model2Vertexes.push_back(vertex);
        }

        m_Vertex2Indices.push_back(uniqueVertices[vertex]);
      } else {
        if (uniqueVertices.count(vertex) == 0) {
          uniqueVertices[vertex] =
              static_cast<uint32_t>(m_ModelVertexes.size());
          m_ModelVertexes.push_back(vertex);
        }

        m_VertexIndices.push_back(uniqueVertices[vertex]);
      }
    }
  }
}
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion = {};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);
}


void CreateVertexBuffers() 
{
  // Creating vertex buffers for 2 objects
  size_t model1Size = sizeof(m_ModelVertexes[0]) * m_ModelVertexes.size();
  size_t model2Size = sizeof(m_Model2Vertexes[0]) * m_Model2Vertexes.size();

  VkDeviceSize bufferSize = model1Size + model2Size;

  VkBuffer stagingBuffer;

  VkBufferCreateInfo stagingBufferInfo = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  stagingBufferInfo.size = bufferSize;
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VmaAllocationCreateInfo stagingAllocInfo = {};
  stagingAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  VmaAllocation stagingAllocation;
  vmaCreateBuffer(m_Allocator, &stagingBufferInfo, &stagingAllocInfo,
                  &stagingBuffer, &stagingAllocation, nullptr);

  void *data;
  vmaMapMemory(m_Allocator, stagingAllocation, &data);
  memcpy(data, m_ModelVertexes.data(), model1Size);
  memcpy(static_cast<char *>(data) + model1Size, m_Model2Vertexes.data(),
          model2Size);
  vmaUnmapMemory(m_Allocator, stagingAllocation);

  // Vertex buffer
  VkBufferCreateInfo vertexBufferInfo = {};
  vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vertexBufferInfo.size = bufferSize;
  vertexBufferInfo.usage =
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VmaAllocationCreateInfo vertexAllocInfo = {};
  vertexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  vmaCreateBuffer(m_Allocator, &vertexBufferInfo, &vertexAllocInfo,
                  &m_VertexBuffer, &m_VertexBufferAllocation, nullptr);

  // Fill the buffer
  copyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

  // Cleanup the staging buffer
  vmaDestroyBuffer(m_Allocator, stagingBuffer, stagingAllocation);
}

void createIndexBuffer() {
  size_t dummyModelSize = sizeof(m_VertexIndices[0]) * m_VertexIndices.size();
  size_t dummyModelSize2 =
      sizeof(m_Vertex2Indices[0]) * m_Vertex2Indices.size();

  // TODO:
  VkDeviceSize bufferSize = 1000000000;

  VkBuffer stagingBuffer;

  VkBufferCreateInfo stagingBufferInfo = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  stagingBufferInfo.size = bufferSize;
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VmaAllocationCreateInfo stagingAllocInfo = {};
  stagingAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  VmaAllocation stagingAllocation;
  vmaCreateBuffer(m_Allocator, &stagingBufferInfo, &stagingAllocInfo,
                  &stagingBuffer, &stagingAllocation, nullptr);

  void *data;
  vmaMapMemory(m_Allocator, stagingAllocation, &data);
  memcpy(data, m_VertexIndices.data(), dummyModelSize);
  memcpy(static_cast<char *>(data) + dummyModelSize, m_Vertex2Indices.data(),
          dummyModelSize2);
  vmaUnmapMemory(m_Allocator, stagingAllocation);

  VkBufferCreateInfo indexBufferInfo = {};
  indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  indexBufferInfo.size = bufferSize;
  indexBufferInfo.usage =
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VmaAllocationCreateInfo indexAllocInfo = {};
  indexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  vmaCreateBuffer(m_Allocator, &indexBufferInfo, &indexAllocInfo,
                  &m_IndexBuffer, &m_IndexBufferAllocation, nullptr);

  copyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

  vmaDestroyBuffer(m_Allocator, stagingBuffer, stagingAllocation);
}

void createUniformBuffers() 
{
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  m_UniformBuffers.resize(m_SwapChainImages.size());
  m_UniformBuffersMemory.resize(m_SwapChainImages.size());

  for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  m_UniformBuffers[i], m_UniformBuffersMemory[i]);
  }
}

void createSyncObjects() {
  m_ImageAvailableSemaphores.resize(m_SwapChainImages.size());
  m_RenderFinishedSemaphores.resize(m_SwapChainImages.size());
  m_InFlightFences.resize(m_SwapChainImages.size());

  m_ImagesInFlight.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
    if (vkCreateSemaphore(VContext.m_Device, &semaphoreInfo, nullptr,
                          &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(VContext.m_Device, &semaphoreInfo, nullptr,
                          &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(VContext.m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) !=
            VK_SUCCESS) {

      throw std::runtime_error(
          "failed to create synchronization objects for a frame!");
    }
  }
}

void setupCommandBuffers(uint32 i) 
{
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  size_t frameFenceIndex = (m_CurrentFrame + 1) % m_SwapChainImages.size();
  vkWaitForFences(VContext.m_Device, 1, &m_InFlightFences[frameFenceIndex], VK_TRUE,
                  UINT64_MAX);
  if (vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = m_RenderPass;
  renderPassInfo.framebuffer = m_SwapChainFrameBuffers[i];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = m_SwapChainExtent;
  renderPassInfo.clearValueCount =
      static_cast<uint32_t>(m_ClearValues.size());
  renderPassInfo.pClearValues = m_ClearValues.data();

  vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo,
                        VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_GraphicsPipeline);

  VkBuffer vertexBuffers[] = {m_VertexBuffer};
  VkDeviceSize offsets[] = {0};

  vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(m_CommandBuffers[i], m_IndexBuffer, 0,
                        VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(m_CommandBuffers[i],
                          VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
                          0, 1, &m_DescriptorSets[i], 0, nullptr);

  //TODO(JohnMir): For loop here one command buffer and keep track of sizes etc
  vkCmdDrawIndexed(m_CommandBuffers[i],
                    static_cast<uint32_t>(m_VertexIndices.size()), 1, 0, 0, 0);
  vkCmdDrawIndexed(m_CommandBuffers[i],
                    static_cast<uint32_t>(m_Vertex2Indices.size()), 1,
                    static_cast<uint32_t>(m_VertexIndices.size()), 
                    static_cast<uint32_t>(m_ModelVertexes.size()), 0);

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CommandBuffers[i]);

  vkCmdEndRenderPass(m_CommandBuffers[i]);

  if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

// Just rotating around the object
// TODO: Hot reload this and divide the time /2 as an example
void ProcessSimulation() {
  static auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(
                    currentTime - startTime)
                    .count();

  UniformBufferObject ubo = {};
  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                          glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view =
      glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.proj = glm::perspective(
      glm::radians(State.someV),
      m_SwapChainExtent.width / (float)m_SwapChainExtent.height, 0.1f, 10.0f);
  ubo.proj[1][1] *= -1;
  ubo.gamma = 1 / State.gammaValue;

  void *data;
  vkMapMemory(VContext.m_Device, m_UniformBuffersMemory[m_ImageIndex], 0, sizeof(ubo),
              0, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(VContext.m_Device, m_UniformBuffersMemory[m_ImageIndex]);
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT callback,
                                   const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, callback, pAllocator);
  }
}

void cleanup() 
{
  cleanupSwapChain(VContext);

  for (size_t i = 0; i < NUMBER_OF_IMAGES; ++i) {
    vkDestroySampler(VContext.m_Device, m_TextureSampler[i], nullptr);

    vkDestroyImageView(VContext.m_Device, m_TextureImageView[i], nullptr);
    vkDestroyImage(VContext.m_Device, m_TextureImage[i], nullptr);
    vkFreeMemory(VContext.m_Device, m_TextureImageMemory[i], nullptr);
  }

  vkDestroyImage(VContext.m_Device, m_DepthImage, nullptr);
  vkFreeMemory(VContext.m_Device, m_DepthImageMemory, nullptr);

  vkDestroyDescriptorSetLayout(VContext.m_Device, m_DescriptorSetLayout, nullptr);

  vkDeviceWaitIdle(VContext.m_Device);
  vkDestroyBuffer(VContext.m_Device, m_IndexBuffer, nullptr);
  vkFreeMemory(VContext.m_Device, m_IndexBufferMemory, nullptr);

  vkDestroyBuffer(VContext.m_Device, m_VertexBuffer, nullptr);
  vmaFreeMemory(m_Allocator, m_VertexBufferAllocation);

  for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
    vkDestroySemaphore(VContext.m_Device, m_RenderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(VContext.m_Device, m_ImageAvailableSemaphores[i], nullptr);
    vkDestroyFence(VContext.m_Device, m_InFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(VContext.m_Device, m_CommandPool, nullptr);
  vkDestroyDevice(VContext.m_Device, nullptr);

  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(VContext.m_Instance, VContext.m_Callback, nullptr);
  }

  vkDestroySurfaceKHR(VContext.m_Instance, VContext.m_Surface, nullptr);
  vkDestroyInstance(VContext.m_Instance, nullptr);

  glfwDestroyWindow(VContext.m_Window);
  glfwTerminate();
}

void recreateSwapChain(VulkanContext vulkanContext) 
{
  int width = 0, height = 0;
  glfwGetFramebufferSize(VContext.m_Window, &width, &height);
  while (width == 0 || height == 0) {
    glfwWaitEvents();
    glfwGetFramebufferSize(VContext.m_Window, &width, &height);
  }

  vkDeviceWaitIdle(vulkanContext.m_Device);
  cleanupSwapChain(vulkanContext);

  createSwapChain(vulkanContext);
  createImageViews(vulkanContext.m_Device);
  createRenderPass();
  createGraphicsPipeline();
  createDepthResources(vulkanContext.m_Device);
  createFramebuffers();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();
  createCommandBuffers();

  recreateImguiContext(VContext, m_DescriptorPool, m_RenderPass, m_SwapChainImages, m_SwapChainExtent, State);
}

void drawFrame() {
  glfwGetFramebufferSize(VContext.m_Window, &WIDTH, &HEIGHT);

  // The vkWaitForFences function takes an array of fences and waits for
  // either any or all of them to be signaled before returning.
  vkWaitForFences(VContext.m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE,
                  UINT64_MAX);

  // Acquire an image from the swapchain
  VkResult result =
      vkAcquireNextImageKHR(VContext.m_Device, m_SwapChain, UINT64_MAX,
                            m_ImageAvailableSemaphores[m_CurrentFrame],
                            VK_NULL_HANDLE, &m_ImageIndex);

  if (m_ImagesInFlight[m_ImageIndex] != VK_NULL_HANDLE) {
    vkWaitForFences(VContext.m_Device, 1, &m_ImagesInFlight[m_ImageIndex], VK_TRUE,
                    UINT64_MAX);
  }

  // Setup the cmd buffers for that image index
  setupCommandBuffers(m_ImageIndex);

  m_ImagesInFlight[m_ImageIndex] = m_InFlightFences[m_CurrentFrame];

  // Submit the command buffers for drawing
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_CommandBuffers[m_ImageIndex];

  VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_ImageIndex]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  vkResetFences(VContext.m_Device, 1, &m_InFlightFences[m_CurrentFrame]);
  if (vkQueueSubmit(VContext.m_GraphicsQueue, 1, &submitInfo,
                    m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {m_SwapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;

  presentInfo.pImageIndices = &m_ImageIndex;

  result = vkQueuePresentKHR(VContext.m_PresentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || g_FrameBufferResized) 
  {
    g_FrameBufferResized = false;
    recreateSwapChain(VContext);
  }
  else if (result != VK_SUCCESS) 
  {
    throw std::runtime_error("failed to present swap chain image!");
  }

  m_CurrentFrame = (m_CurrentFrame + 1) % m_SwapChainImages.size();
}

void initVulkan(GLFWwindow* window) 
{
  // Basic init
  VContext = {};
  VContext.m_Window = window;
  VContext.m_Instance = createInstance();
  VContext.m_Callback = setupDebugCallback(VContext.m_Instance);
  VContext.m_Surface = createSurface(window, VContext.m_Instance);
  VContext.m_PhysicalDevice = pickPhysicalDevice(VContext.m_Instance,
                                                 &VContext.m_QueueFamilyIndicesUsed,
                                                        VContext.m_Surface);
  VContext.m_Device = createLogicalDevice(VContext.m_PhysicalDevice,
                                          VContext.m_QueueFamilyIndicesUsed);

  vkGetDeviceQueue(VContext.m_Device, VContext.m_QueueFamilyIndicesUsed.graphicsFamily.value(),
                    0, &VContext.m_GraphicsQueue);
  vkGetDeviceQueue(VContext.m_Device, VContext.m_QueueFamilyIndicesUsed.presentFamily.value(),
                    0, &VContext.m_PresentQueue);


  //Allocator
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = VContext.m_PhysicalDevice; // Your VkPhysicalDevice
  allocatorInfo.device = VContext.m_Device;                 // Your VkDevice
  allocatorInfo.instance = VContext.m_Instance;             // Your VkInstance

  if (vmaCreateAllocator(&allocatorInfo, &m_Allocator) != VK_SUCCESS) 
  {
    throw std::runtime_error("failed to create VMA allocator!");
  }

  // Create swap chain with image views
  createDescriptorSetLayout();
  createCommandPool(VContext.m_PhysicalDevice, VContext.m_Surface, VContext.m_Device);

  createTextureImage();
  createTextureImageView(VContext.m_Device);
  createTextureSampler();

  loadModel(false);
  loadModel(true);

  CreateVertexBuffers();
  createIndexBuffer();

  createSwapChain(VContext);
  createImageViews(VContext.m_Device);
  createRenderPass();
  createGraphicsPipeline();
  createDepthResources(VContext.m_Device);
  createFramebuffers();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();
  createCommandBuffers();

  /*
  createSwapChain();
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createDepthResources(VContext.m_Device);
  createFramebuffers();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();
  createCommandBuffers();
  */


  createSyncObjects();

  initImGui(VContext, m_DescriptorPool, m_RenderPass, m_SwapChainImages,
            float(m_SwapChainExtent.width), float(m_SwapChainExtent.height));
}

void mainLoop() 
{

  // Engine loop
  while (!glfwWindowShouldClose(VContext.m_Window)) 
  {
    glfwPollEvents();    // Input
    renderImgui(m_SwapChainExtent, State);  // Imgui
    ProcessSimulation(); // Do something
    drawFrame();         // Render
  }

  // Shutdown
  vkDeviceWaitIdle(VContext.m_Device);
  ShutDownOverlay();
}

void run() 
{
  GLFWwindow* window;
  State = {};

  initWindow(&window, WIDTH, HEIGHT);
  initVulkan(window);

  mainLoop();
  cleanup();
}

int main() 
{
  try 
  {
    run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
