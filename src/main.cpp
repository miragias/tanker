#include "pch.h"
#include "shared.h"
#include "Globals.cpp"
#include "Core/HotReloading.cpp"

#include "Core/CoreIO.cpp"
#include "Rendering/VulkanCommon.cpp"
#include "Rendering/VulkanSetup.cpp"
#include "ImguiOverlay.cpp"
#include "Rendering/NativeWindow.cpp"
#include "Rendering/VulkanSwapChain.cpp"

//TODO(JohnMir): 
//Arena Allocators

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

const std::string MODEL_PATH = "models/skull.obj";
const std::string MODEL2_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "models/viking_room.png";

std::vector<Vertex> m_ModelVertexes;
std::vector<uint32_t> m_VertexIndices;
std::vector<Vertex> m_Model2Vertexes;
std::vector<uint32_t> m_Vertex2Indices;

//Globals
VkBuffer m_VertexBuffer;
VkBuffer m_IndexBuffer;

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

VmaAllocation m_VertexBufferAllocation;
VmaAllocation m_IndexBufferAllocation;
VkDeviceMemory m_IndexBufferMemory;

uint32_t m_ImageIndex = 0;

VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

uint32_t g_QueueFamily = (uint32_t)-1;
std::vector<VkSemaphore> m_ImageAvailableSemaphores;
std::vector<VkSemaphore> m_RenderFinishedSemaphores;
std::vector<VkFence> m_InFlightFences;
std::vector<VkFence> m_ImagesInFlight;

VmaAllocator m_Allocator;

size_t m_CurrentFrame = 0;

void cleanupSwapChain(VulkanContext vulkanContext) 
{
  for (size_t i = 0; i < SwapChain.m_SwapChainFrameBuffers.size(); i++) 
  {
    vkDestroyFramebuffer(vulkanContext.m_Device, SwapChain.m_SwapChainFrameBuffers[i], nullptr);
  }

  vkFreeCommandBuffers(vulkanContext.m_Device, m_CommandPool,
                        static_cast<uint32_t>(m_CommandBuffers.size()),
                        m_CommandBuffers.data());

  vkDestroyPipeline(vulkanContext.m_Device, m_GraphicsPipeline, nullptr);
  vkDestroyPipelineLayout(vulkanContext.m_Device, m_PipelineLayout, nullptr);
  vkDestroyRenderPass(vulkanContext.m_Device, m_RenderPass, nullptr);

  for (size_t i = 0; i < SwapChain.m_SwapChainImageViews.size(); i++) {
    vkDestroyImageView(vulkanContext.m_Device, SwapChain.m_SwapChainImageViews[i], nullptr);
  }

  vkDestroyImageView(vulkanContext.m_Device, SwapChain.m_DepthImageView, nullptr);

  vkDestroySwapchainKHR(vulkanContext.m_Device, SwapChain.m_SwapChain, nullptr);

  for (size_t i = 0; i < SwapChain.m_SwapChainImages.size(); i++) {
    vkDestroyBuffer(vulkanContext.m_Device, m_UniformBuffers[i], nullptr);
    vkFreeMemory(vulkanContext.m_Device, m_UniformBuffersMemory[i], nullptr);
  }

  vkDestroyDescriptorPool(vulkanContext.m_Device, m_DescriptorPool, nullptr);
}

bool hasStencilComponent(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
          format == VK_FORMAT_D24_UNORM_S8_UINT;
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

void LoadTextureToGpuMemory(const char *filePath, VkImage *image, VkDeviceMemory *memory)
{
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
        throw std::runtime_error("failed to load texture image!");

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    CreateBuffer(VContext.m_Device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void *data;
    vkMapMemory(VContext.m_Device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(VContext.m_Device, stagingBufferMemory);

    stbi_image_free(pixels);

    CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image,
                memory);

    transitionImageLayout(*image, VK_FORMAT_R8G8B8A8_UNORM,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copyBufferToImage(stagingBuffer, *image,
                      static_cast<uint32_t>(texWidth),
                      static_cast<uint32_t>(texHeight));

    transitionImageLayout(*image, VK_FORMAT_R8G8B8A8_UNORM,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(VContext.m_Device, stagingBuffer, nullptr);
    vkFreeMemory(VContext.m_Device, stagingBufferMemory, nullptr);
}

void CreateTextureImageView(VkDevice& device, VkImage& image, VkImageView* imageView) 
{
  *imageView = CreateImageView(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, device);
}

void CreateTextureSampler(VkSampler *outTextureSampler) 
{
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

  if (vkCreateSampler(VContext.m_Device, &samplerInfo, nullptr, outTextureSampler) != VK_SUCCESS) 
  {
    throw std::runtime_error("failed to create texture sampler!");
  }
}

void FillSpriteVerticesAndIndices(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, float x, float y, float width, float height)
{
    // Define 4 vertices of the quad (z = 0), white color, proper UVs
    vertices.push_back({ {x,          y,           0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f} }); // bottom-left
    vertices.push_back({ {x + width,  y,           0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f} }); // bottom-right
    vertices.push_back({ {x + width,  y + height,  0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f} }); // top-right
    vertices.push_back({ {x,          y + height,  0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f} }); // top-left

    uint32_t baseIndex = static_cast<uint32_t>(vertices.size()) - 4; // Index of first vertex of this quad

    // Two triangles (6 indices) to form the quad
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);

    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
    indices.push_back(baseIndex + 0);
}

SpriteList LoadSpriteListIntoGpuMemory(StartingSpritePaths spritePaths)
{
  SpriteList spriteListToReturn = {};
  for(auto spritePath : spritePaths)
  {
    Sprite sprite = {};
    sprite.OriginalFilePath = spritePath;
    sprite.Vertices.clear();
    sprite.Indices.clear();
    LoadTextureToGpuMemory(sprite.OriginalFilePath, &sprite.TextureImage, &sprite.TextureMemory);
    CreateTextureImageView(VContext.m_Device, sprite.TextureImage, &sprite.TextureImageView);
    CreateTextureSampler(&sprite.TextureSampler);
    float x = 0.0f;   // Sprite position
    float y = 0.0f;
    float width = 100.0f;  // Sprite size
    float height = 100.0f;
    FillSpriteVerticesAndIndices(sprite.Vertices, sprite.Indices, x, y, width, height);

    //Ubo creation
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    CreateBuffer(VContext.m_Device,
      bufferSize,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      sprite.UniformBuffer,
      sprite.UniformBufferMemory
    );
    spriteListToReturn.push_back(sprite);
  }
  return spriteListToReturn;
}


void loadModel(bool second) 
{
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



void CreateVertexBuffersSprites(const std::vector<Sprite>& sprites)
{
    // Calculate total size for all sprites
    size_t totalVertexCount = 0;
    for (const auto& sprite : sprites) 
  {
        totalVertexCount += sprite.Vertices.size();
    }
    VkDeviceSize bufferSize = sizeof(Vertex) * totalVertexCount;

    // Create staging buffer
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;

    VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    stagingBufferInfo.size = bufferSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    vmaCreateBuffer(m_Allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, nullptr);

    // Map and copy all vertex data sequentially
    void* data;
    vmaMapMemory(m_Allocator, stagingAllocation, &data);

    char* dataPtr = static_cast<char*>(data);
    for (const auto& sprite : sprites) 
    {
        size_t spriteSize = sprite.Vertices.size() * sizeof(Vertex);
        memcpy(dataPtr, sprite.Vertices.data(), spriteSize);
        dataPtr += spriteSize;
    }

    vmaUnmapMemory(m_Allocator, stagingAllocation);

    // Create vertex buffer on GPU
    VkBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = bufferSize;
    vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vertexAllocInfo = {};
    vertexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vmaCreateBuffer(m_Allocator, &vertexBufferInfo, &vertexAllocInfo, &m_VertexBuffer, &m_VertexBufferAllocation, nullptr);

    // Copy staging buffer to vertex buffer
    copyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

    // Cleanup
    vmaDestroyBuffer(m_Allocator, stagingBuffer, stagingAllocation);
}

void CreateIndexBuffersSprites(const std::vector<Sprite>& sprites)
{
    // Calculate total indices count
    size_t totalIndexCount = 0;
    for (const auto& sprite : sprites) 
  {
        totalIndexCount += sprite.Indices.size();
    }
    VkDeviceSize bufferSize = sizeof(uint32_t) * totalIndexCount;

    // Create staging buffer
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    stagingBufferInfo.size = bufferSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    vmaCreateBuffer(m_Allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, nullptr);

    // Map and copy index data sequentially
    void* data;
    vmaMapMemory(m_Allocator, stagingAllocation, &data);

    char* dataPtr = static_cast<char*>(data);
    for (const auto& sprite : sprites) 
  {
        size_t spriteSize = sprite.Indices.size() * sizeof(uint32_t);
        memcpy(dataPtr, sprite.Indices.data(), spriteSize);
        dataPtr += spriteSize;
    }

    vmaUnmapMemory(m_Allocator, stagingAllocation);

    // Create GPU index buffer
    VkBufferCreateInfo indexBufferInfo = {};
    indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferInfo.size = bufferSize;
    indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo indexAllocInfo = {};
    indexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vmaCreateBuffer(m_Allocator, &indexBufferInfo, &indexAllocInfo, &m_IndexBuffer, &m_IndexBufferAllocation, nullptr);

    // Copy staging to GPU index buffer
    copyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

    // Cleanup staging
    vmaDestroyBuffer(m_Allocator, stagingBuffer, stagingAllocation);
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

void createIndexBuffer() 
{
  size_t dummyModelSize = sizeof(m_VertexIndices[0]) * m_VertexIndices.size();
  size_t dummyModelSize2 =
      sizeof(m_Vertex2Indices[0]) * m_Vertex2Indices.size();

  VkDeviceSize bufferSize = dummyModelSize + dummyModelSize2;

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

void createSyncObjects() 
{
  m_ImageAvailableSemaphores.resize(SwapChain.m_SwapChainImages.size());
  m_RenderFinishedSemaphores.resize(SwapChain.m_SwapChainImages.size());
  m_InFlightFences.resize(SwapChain.m_SwapChainImages.size());

  m_ImagesInFlight.resize(SwapChain.m_SwapChainImages.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < SwapChain.m_SwapChainImages.size(); i++) {
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

void setupCommandBuffers(uint32 i, SpriteList spriteList) 
{
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  size_t frameFenceIndex = (m_CurrentFrame + 1) % SwapChain.m_SwapChainImages.size();
  vkWaitForFences(VContext.m_Device, 1, &m_InFlightFences[frameFenceIndex], VK_TRUE,
                  UINT64_MAX);
  if (vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo) != VK_SUCCESS) 
  {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = m_RenderPass;
  renderPassInfo.framebuffer = SwapChain.m_SwapChainFrameBuffers[i];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = SwapChain.m_SwapChainExtent;
  renderPassInfo.clearValueCount =
      static_cast<uint32_t>(m_ClearValues.size());
  renderPassInfo.pClearValues = m_ClearValues.data();

  vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

  for (size_t spriteIdx = 0; spriteIdx < spriteList.size(); ++spriteIdx) 
  {
      const Sprite& sprite = spriteList[spriteIdx];

      // Bind vertex and index buffers for this sprite
      VkBuffer vertexBuffers[] = { sprite.SpriteVertexBuffer };
      VkDeviceSize offsets[] = { 0 };
      vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);
      vkCmdBindIndexBuffer(m_CommandBuffers[i], sprite.SpriteIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

      // Bind the descriptor set for this sprite (contains its UBO and texture)
      vkCmdBindDescriptorSets(
          m_CommandBuffers[i],
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          m_PipelineLayout,
          0, 1, &sprite.DescriptorSet,  // per-sprite descriptor set
          0, nullptr);

      // Draw the sprite
      vkCmdDrawIndexed(m_CommandBuffers[i],
                      static_cast<uint32_t>(sprite.Indices.size()),
                      1, 0, 0, 0);
  }


  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CommandBuffers[i]);

  vkCmdEndRenderPass(m_CommandBuffers[i]);

  if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS) 
  {
    throw std::runtime_error("failed to record command buffer!");
  }
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

  //Swapchain images and sampler
  for (size_t i = 0; i < SWAPCHAIN_IMAGE_NUM; ++i) 
  {
    vkDestroyImageView(VContext.m_Device, SwapChain.m_SwapChainImageViews[i], nullptr);
    vkDestroyImage(VContext.m_Device, SwapChain.m_SwapChainImages[i], nullptr);
  }
  vkDestroySampler(VContext.m_Device, SwapChain.SwapChainSampler, nullptr);

  vkDestroyImage(VContext.m_Device, SwapChain.m_DepthImage, nullptr);
  vkFreeMemory(VContext.m_Device, SwapChain.m_DepthImageMemory, nullptr);

  vkDestroyDescriptorSetLayout(VContext.m_Device, g_DescriptorSetLayout, nullptr);

  vkDeviceWaitIdle(VContext.m_Device);
  vkDestroyBuffer(VContext.m_Device, m_IndexBuffer, nullptr);
  vkFreeMemory(VContext.m_Device, m_IndexBufferMemory, nullptr);

  vkDestroyBuffer(VContext.m_Device, m_VertexBuffer, nullptr);
  vmaFreeMemory(m_Allocator, m_VertexBufferAllocation);

  for (size_t i = 0; i < SwapChain.m_SwapChainImages.size(); i++) {
    vkDestroySemaphore(VContext.m_Device, m_RenderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(VContext.m_Device, m_ImageAvailableSemaphores[i], nullptr);
    vkDestroyFence(VContext.m_Device, m_InFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(VContext.m_Device, m_CommandPool, nullptr);
  vkDestroyDevice(VContext.m_Device, nullptr);

  if (enableValidationLayers) 
  {
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
  while (width == 0 || height == 0) 
  {
    glfwWaitEvents();
    glfwGetFramebufferSize(VContext.m_Window, &width, &height);
  }

  vkDeviceWaitIdle(vulkanContext.m_Device);
  cleanupSwapChain(vulkanContext);

  SwapChain = CreateSwapChain(vulkanContext, G_GameSprites);

  //TODO:JohnMir: the last var
  recreateImguiContext(VContext, m_DescriptorPool, m_RenderPass, SwapChain.m_SwapChainImages,
                       SwapChain.m_SwapChainExtent, G_GameState);
}

void drawFrame() {
  glfwGetFramebufferSize(VContext.m_Window, &WIDTH, &HEIGHT);

  // The vkWaitForFences function takes an array of fences and waits for
  // either any or all of them to be signaled before returning.
  vkWaitForFences(VContext.m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE,
                  UINT64_MAX);

  // Acquire an image from the swapchain
  VkResult result =
      vkAcquireNextImageKHR(VContext.m_Device, SwapChain.m_SwapChain, UINT64_MAX,
                            m_ImageAvailableSemaphores[m_CurrentFrame],
                            VK_NULL_HANDLE, &m_ImageIndex);

  if (m_ImagesInFlight[m_ImageIndex] != VK_NULL_HANDLE) {
    vkWaitForFences(VContext.m_Device, 1, &m_ImagesInFlight[m_ImageIndex], VK_TRUE,
                    UINT64_MAX);
  }

  // Setup the cmd buffers for that image index
  setupCommandBuffers(m_ImageIndex, G_GameSprites);

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

  VkSwapchainKHR swapChains[] = {SwapChain.m_SwapChain};
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

  m_CurrentFrame = (m_CurrentFrame + 1) % SwapChain.m_SwapChainImages.size();
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
  createDescriptorSetLayout(VContext.m_Device, &g_DescriptorSetLayout);
  createCommandPool(VContext.m_PhysicalDevice, VContext.m_Surface, VContext.m_Device);

  G_GameSprites = LoadSpriteListIntoGpuMemory(LoadFileList(SPRITES_PATH));

  //loadModel(false);
  //loadModel(true);

  CreateVertexBuffersSprites(G_GameSprites);
  CreateIndexBuffersSprites(G_GameSprites);

  SwapChain = CreateSwapChain(VContext, G_GameSprites);

  createSyncObjects();

  initImGui(VContext, m_DescriptorPool, m_RenderPass, SwapChain.m_SwapChainImages,
            float(SwapChain.m_SwapChainExtent.width), float(SwapChain.m_SwapChainExtent.height));
}


void mainLoop() 
{ 
    //TODO(JohnMir): Make this a pointer
    G_GameState = {};
    //The state initialization
    //Some stuff
    G_GameState.aspectRatio = SwapChain.m_SwapChainExtent.width / (float)SwapChain.m_SwapChainExtent.height;
    G_GameState.gamma = G_GameState.gammaValue;
    G_GameState.device = VContext.m_Device;
    G_GameState.uniformBuffersMemory = m_UniformBuffersMemory;
    G_GameState.swapchainExtent = SwapChain.m_SwapChainExtent;

    //Make camera
    Camera camera;
    camera.Position     = glm::vec3(0.0f, 4.0f, -10.0f);
    camera.Target       = glm::vec3(0.0f, -1.0f, 0.0f);
    camera.Up           = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.FovRadians   = glm::radians(90.0f);
    camera.AspectRatio  = SwapChain.m_SwapChainExtent.width / (float)SwapChain.m_SwapChainExtent.height;
    camera.NearPlane    = 0.1f;
    camera.FarPlane     = 1000.0f;
    G_GameState.Cam = camera;

  // Engine loop
  while (!glfwWindowShouldClose(VContext.m_Window)) 
  {
    glfwPollEvents();

    renderImgui(SwapChain.m_SwapChainExtent, G_GameState);
    TryHotReloadDLL(G_HotReloadData);

    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                      currentTime - startTime)
                      .count();
    G_GameState.time = time;
    G_GameState.imageIndex = m_ImageIndex;

    if (G_HotReloadData.ProcessSimulation) G_HotReloadData.ProcessSimulation(&G_GameState);
    drawFrame();

    //Check close the window
    if (glfwGetKey(VContext.m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(VContext.m_Window, GLFW_TRUE);
    }
  }

  // Shutdown
  vkDeviceWaitIdle(VContext.m_Device);
  ShutDownOverlay();
}

void run() 
{
  G_GameState = {};
  GLFWwindow* window = CreateAppWindow(WIDTH, HEIGHT);
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
