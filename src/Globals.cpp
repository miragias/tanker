#include "pch.h"

#define VMA_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4100)
#pragma warning(disable : 4324)
#pragma warning(disable:4189)
#pragma warning(disable: 4127)
#include "Rendering/vk_mem_alloc.h"
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
const int SWAPCHAIN_IMAGE_NUM = 2;
const char* SPRITES_PATH = "sprites.txt";

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

struct Sprite
{
    char* OriginalFilePath;

    // Vertex and index data for the sprite
    std::vector<Vertex> Vertices;
    std::vector<uint32_t> Indices;

    // Texture image and allocation
    VkImage TextureImage;
    VmaAllocation TextureAllocation;

    // Image view and sampler (normal Vulkan handles)
    VkImageView TextureImageView;
    VkSampler TextureSampler;

    // Buffers and allocations
    VkBuffer SpriteVertexBuffer;
    VmaAllocation VertexBufferAllocation;

    VkBuffer SpriteIndexBuffer;
    VmaAllocation IndexBufferAllocation;

    VkBuffer UniformBuffer;
    VmaAllocation UniformBufferAllocation;
  VkDescriptorSet DescriptorSet;
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
  VkSampler SwapChainSampler;

  VkImage m_DepthImage;
  VkDeviceMemory m_DepthImageMemory;
  VkImageView m_DepthImageView;
};
std::vector<VkDescriptorSet> m_DescriptorSets;

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

typedef std::vector<Sprite> SpriteList;
typedef std::vector<char*> StartingSpritePaths;


VmaAllocator m_Allocator;
GameState G_GameState;
VulkanContext VContext;
HotReloadData G_HotReloadData;
SpriteList G_GameSprites= {};
StartingSpritePaths G_StartingSpritePaths;

VulkanSwapChain SwapChain;
VkRenderPass m_RenderPass;
VkDescriptorSetLayout g_DescriptorSetLayout;
VkDescriptorPool m_DescriptorPool;
std::vector<VkBuffer> m_UniformBuffers;

VkCommandPool m_CommandPool;
std::vector<VkCommandBuffer> m_CommandBuffers;
std::array<VkClearValue, 2> m_ClearValues = {};
VkPipelineLayout m_PipelineLayout;
std::vector<VkDeviceMemory> m_UniformBuffersMemory;
VkPipeline m_GraphicsPipeline;
