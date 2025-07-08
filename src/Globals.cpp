#include "common.h"

#define VMA_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4100)
#pragma warning(disable : 4324)
#pragma warning(disable:4189)
#pragma warning(disable: 4127)
#include "vk_mem_alloc.h"
#pragma warning(pop)

#define TINYOBJLOADER_IMPLEMENTATION
#include "Other/tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "Other/stb_image.h"

int WIDTH = 2000;
int HEIGHT = 1000;
bool g_FrameBufferResized = false;
bool g_SwapChainRebuild = false;
int g_SwapChainResizeWidth = 0;
int g_SwapChainResizeHeight = 0;
const int NUMBER_OF_IMAGES = 2;

struct UniformBufferObject 
{
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
  float gamma;
};

struct Vertex 
{
  vec3 pos;
  vec3 color;
  vec2 texCoord;

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

struct QueueFamilyIndices 
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() 
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct VulkanSwapChain
{
  VkSwapchainKHR m_SwapChain;
  VkFormat m_SwapChainImageFormat;
  VkExtent2D m_SwapChainExtent;
  std::vector<VkImage> m_SwapChainImages;
  std::vector<VkImageView> m_SwapChainImageViews;
  std::vector<VkFramebuffer> m_SwapChainFrameBuffers;

  //TODO(JohnMir): Bundle them
  VkImage m_DepthImage;
  VkDeviceMemory m_DepthImageMemory;
  VkImageView m_DepthImageView;
};

struct SwapChainSupportDetails 
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct VulkanContext
{
  VkInstance m_Instance;
  GLFWwindow* m_Window;
  VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
  VkSurfaceKHR m_Surface;
  VkDevice m_Device;
  QueueFamilyIndices m_QueueFamilyIndicesUsed;
  VkDebugUtilsMessengerEXT m_Callback;
  VkQueue m_GraphicsQueue;
  VkQueue m_PresentQueue;
};

GameState G_GameState;
VulkanContext VContext;
HotReloadData G_HotReloadData;

VulkanSwapChain SwapChain;
VkRenderPass m_RenderPass;
VkDescriptorSetLayout g_DescriptorSetLayout;
std::vector<VkDescriptorSet> m_DescriptorSets;
VkDescriptorPool m_DescriptorPool;
std::vector<VkBuffer> m_UniformBuffers;
VkImage m_TextureImage[NUMBER_OF_IMAGES];
VkDeviceMemory m_TextureImageMemory[NUMBER_OF_IMAGES];
VkImageView m_TextureImageView[NUMBER_OF_IMAGES];
VkCommandPool m_CommandPool;
VkSampler m_TextureSampler[NUMBER_OF_IMAGES];
std::vector<VkCommandBuffer> m_CommandBuffers;
std::array<VkClearValue, 2> m_ClearValues = {};
VkPipelineLayout m_PipelineLayout;
std::vector<VkDeviceMemory> m_UniformBuffersMemory;
VkPipeline m_GraphicsPipeline;
