struct QueueFamilyIndices 
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() 
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct VulkanContext
{
  VkInstance m_Instance;
  GLFWwindow *m_Window;
  VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
  VkSurfaceKHR m_Surface;
  VkDevice m_Device;
  QueueFamilyIndices m_QueueFamilyIndicesUsed;
  VkDebugUtilsMessengerEXT m_Callback;
  VkQueue m_GraphicsQueue;
  VkQueue m_PresentQueue;
};

