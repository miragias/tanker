#include "imgui/imgui.cpp"
#include "imgui/backends/imgui_impl_glfw.cpp"
#include "imgui/backends/imgui_impl_vulkan.cpp"
#include "imgui/imgui_demo.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"

float f1 = 90.0f;
float gammaValue = 1;

void imGuiSetupWindow(VkExtent2D swapChainExtent) {
  ImGuiIO &io = ImGui::GetIO();
  // Start the Dear ImGui frame
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();

  auto WindowSize =
      ImVec2((float)swapChainExtent.width, (float)swapChainExtent.height);
  ImGui::SetNextWindowSize(WindowSize, ImGuiCond_::ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_::ImGuiCond_FirstUseEver);
  ImGui::NewFrame();

  // render your GUI
  ImGui::Begin("JohnMirTest");
  ImGui::SliderFloat("Amount of perspective", &f1, 0.0f, 90.0f,
                      "ratio = %.3f");
  ImGui::SliderFloat("gamma", &gammaValue, 0.0f, 4.0f, "%.2f");
  ImGui::ShowDemoWindow();
  ImGui::End();
  // Render dear imgui UI box into our window
  ImGui::Render();
}

void initImGui(VkInstance instance, VulkanContext &context, VkDescriptorPool &descriptorPool,
               VkRenderPass &renderPass, std::vector<VkImage> &swapChainImages,
               float width, float height) 
{
  // IMGUI TESTS
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  unsigned char *fontData;
  int texWidth, texHeight;
  io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

  ImGui_ImplGlfw_InitForVulkan(context.m_Window, true);
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = instance;
  init_info.PhysicalDevice = context.m_PhysicalDevice;
  init_info.Device = context.m_Device;
  init_info.QueueFamily = context.m_QueueFamilyIndicesUsed.graphicsFamily.value();
  init_info.Queue = context.m_PresentQueue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = descriptorPool;
  init_info.RenderPass = renderPass;
  init_info.Allocator = NULL;
  init_info.MinImageCount = 2;
  init_info.ImageCount = static_cast<uint32_t>(swapChainImages.size());
  init_info.CheckVkResultFn = NULL;
  ImGui_ImplVulkan_Init(&init_info);

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
}

void recreateImguiContext(VkInstance instance, VulkanContext &context, VkDescriptorPool &descriptorPool,
               VkRenderPass &renderPass, std::vector<VkImage> &swapChainImages, VkExtent2D swapChainExtent) 
{
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  initImGui(instance, context, descriptorPool, renderPass, swapChainImages,
            float(swapChainExtent.width), float(swapChainExtent.height));
  imGuiSetupWindow(swapChainExtent);
}

void ShutDownOverlay()
{
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

