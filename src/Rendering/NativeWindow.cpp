static void framebufferResizeCallback(GLFWwindow *window, int width,
                                      int height) {
  g_FrameBufferResized = true;
}

void initWindow(GLFWwindow** window, int width, int height) 
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  *window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
  glfwSetWindowUserPointer(*window, nullptr);
  glfwSetFramebufferSizeCallback(*window, framebufferResizeCallback);
}

