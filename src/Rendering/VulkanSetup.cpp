#include "VulkanInternals.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) 
{
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}


VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pCallback) 
{
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) return func(instance, pCreateInfo, pAllocator, pCallback);
  else return VK_ERROR_EXTENSION_NOT_PRESENT;
}

VkDebugUtilsMessengerEXT setupDebugCallback(VkInstance instance) 
{
  if (!enableValidationLayers) return nullptr;

  VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;

  VkDebugUtilsMessengerEXT callBack;
  if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                    &callBack) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug callback!");
  }
  return callBack;
}

bool checkValidationLayerSupport() {

  uint32 layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *layerName : validationLayers) {
    bool layerFound = false;

    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound) {
      return false;
    }
  }
  return true;
}

std::vector<const char *> getRequiredExtensions() {
  uint32 glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions,
                                        glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers) 
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

VkInstance createInstance() 
{
  if (enableValidationLayers && !checkValidationLayerSupport()) 
  {
    throw std::runtime_error(
        "validation layers requested, but not available!");
  }

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Tanker";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  auto instanceInfoExtensions = getRequiredExtensions();
  createInfo.enabledExtensionCount =
      static_cast<uint32>(instanceInfoExtensions.size());
  createInfo.ppEnabledExtensionNames = instanceInfoExtensions.data();

  if (enableValidationLayers) 
  {
    createInfo.enabledLayerCount =
        static_cast<uint32>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  }
  else 
  {
    createInfo.enabledLayerCount = 0;
  }

  VkInstance instanceToReturn;
  if (vkCreateInstance(&createInfo, nullptr, &instanceToReturn) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
  }

  uint32 extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                          extensions.data());
  /*
  std::cout << "available extension:" << std::endl;
  for (const auto& extension : extensions) {
          std::cout << "\t" << extension.extensionName << std::endl;
  }
  */
  return instanceToReturn;
}

