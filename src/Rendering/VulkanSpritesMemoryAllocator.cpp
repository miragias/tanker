//Globals
VkBuffer m_VertexBuffer;
VkBuffer m_IndexBuffer;
VmaAllocation m_VertexBufferAllocation;
VmaAllocation m_IndexBufferAllocation;


//Handles allocating the buffers and memory for the game
void CreateSpriteDescriptorSets(VkDevice device, SpriteList& spriteList, size_t swapchainImageCount)
{
    size_t spriteCount = spriteList.size();

    // Resize the container holding all descriptor sets
    // For example, each sprite has a vector of descriptor sets, one per swapchain image
    for (auto& sprite : spriteList)
        sprite.DescriptorSets.resize(swapchainImageCount);

    std::vector<VkDescriptorSetLayout> layouts(swapchainImageCount, g_DescriptorSetLayout);

    for (size_t spriteIndex = 0; spriteIndex < spriteCount; spriteIndex++)
    {
        Sprite& sprite = spriteList[spriteIndex];

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapchainImageCount);
        allocInfo.pSetLayouts = layouts.data();

        std::vector<VkDescriptorSet> descriptorSets(swapchainImageCount);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate descriptor sets for sprites!");
        }

        // Update descriptor sets for each swapchain image
        for (uint32_t imageIndex = 0; imageIndex < swapchainImageCount; imageIndex++)
        {
            // Uniform buffer for this sprite and swapchain image
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = sprite.UniformBuffers[imageIndex]; // Assuming you have a vector of buffers per sprite
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            // Texture sampler info
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = sprite.TextureImageView;
            imageInfo.sampler = sprite.TextureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[imageIndex];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[imageIndex];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

            // Save the descriptor set for this sprite and image index
            sprite.DescriptorSets[imageIndex] = descriptorSets[imageIndex];
        }
    }
}

void CreateUniformBuffersForSprites(VkDevice device, VmaAllocator allocator,
                                    SpriteList& spriteList, size_t swapchainImageCount)
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    for (Sprite& sprite : spriteList)
    {
        sprite.UniformBuffers.resize(swapchainImageCount);
        sprite.UniformBufferAllocations.resize(swapchainImageCount);
        sprite.MappedUniformData.resize(swapchainImageCount);

        for (size_t i = 0; i < swapchainImageCount; i++)
        {
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = bufferSize;
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

            VmaAllocation allocation;
            VmaAllocationInfo allocInfoOut;

            //Buffer info of sprite filled here
            if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &sprite.UniformBuffers[i], &allocation, &allocInfoOut) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create sprite uniform buffer!");
            }
            sprite.UniformBufferAllocations[i] = allocation;
            sprite.MappedUniformData[i] = allocInfoOut.pMappedData;
        }
    }
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
    CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

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
    CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

    // Cleanup staging
    vmaDestroyBuffer(m_Allocator, stagingBuffer, stagingAllocation);
}
