static void framebufferResizeCallback(GLFWwindow *window, int width,
                                      int height) {
  g_FrameBufferResized = true;
}

GLFWwindow* CreateAppWindow(int width, int height)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
    if (!window)
    {
        return nullptr;
    }

    glfwSetWindowUserPointer(window, nullptr);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    return window;
}
