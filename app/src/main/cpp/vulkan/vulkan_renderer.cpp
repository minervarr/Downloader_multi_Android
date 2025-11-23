/**
 * @file vulkan_renderer.cpp
 * @brief Vulkan renderer implementation for Android
 */

#include "vulkan_renderer.hpp"
#include <android/log.h>
#include <array>
#include <chrono>
#include <cstring>

#define LOG_TAG "VulkanRenderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

namespace downloader {
namespace vulkan {

static const int MAX_FRAMES_IN_FLIGHT = 2;

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() {
    cleanup();
}

bool VulkanRenderer::initialize(ANativeWindow* window, const RendererConfig& config) {
    if (m_initialized) {
        LOGI("Renderer already initialized");
        return true;
    }

    m_config = config;
    LOGI("Initializing Vulkan renderer %dx%d", config.width, config.height);

    if (!create_instance()) {
        LOGE("Failed to create Vulkan instance");
        return false;
    }

    if (!create_surface(window)) {
        LOGE("Failed to create Vulkan surface");
        return false;
    }

    if (!select_physical_device()) {
        LOGE("Failed to select physical device");
        return false;
    }

    if (!create_logical_device()) {
        LOGE("Failed to create logical device");
        return false;
    }

    if (!create_swapchain()) {
        LOGE("Failed to create swapchain");
        return false;
    }

    if (!create_render_pass()) {
        LOGE("Failed to create render pass");
        return false;
    }

    if (!create_framebuffers()) {
        LOGE("Failed to create framebuffers");
        return false;
    }

    if (!create_command_pool()) {
        LOGE("Failed to create command pool");
        return false;
    }

    if (!create_command_buffers()) {
        LOGE("Failed to create command buffers");
        return false;
    }

    if (!create_sync_objects()) {
        LOGE("Failed to create sync objects");
        return false;
    }

    if (!create_pipeline()) {
        LOGE("Failed to create graphics pipeline");
        return false;
    }

    m_initialized = true;
    LOGI("Vulkan renderer initialized successfully");
    return true;
}

void VulkanRenderer::cleanup() {
    if (!m_initialized) return;

    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
    }

    cleanup_swapchain();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (m_render_finished_semaphores.size() > i && m_render_finished_semaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_render_finished_semaphores[i], nullptr);
        }
        if (m_image_available_semaphores.size() > i && m_image_available_semaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_image_available_semaphores[i], nullptr);
        }
        if (m_in_flight_fences.size() > i && m_in_flight_fences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(m_device, m_in_flight_fences[i], nullptr);
        }
    }

    if (m_command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_command_pool, nullptr);
    }

    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
    }

    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    }

    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
    }

    m_initialized = false;
    LOGI("Vulkan renderer cleaned up");
}

bool VulkanRenderer::create_instance() {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = m_config.app_name.c_str();
    app_info.applicationVersion = m_config.app_version;
    app_info.pEngineName = "Downloader Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };

    std::vector<const char*> layers;
    if (m_config.enable_validation) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    create_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
    create_info.ppEnabledLayerNames = layers.data();

    VkResult result = vkCreateInstance(&create_info, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        LOGE("vkCreateInstance failed: %d", result);
        return false;
    }

    LOGI("Vulkan instance created");
    return true;
}

bool VulkanRenderer::create_surface(ANativeWindow* window) {
    VkAndroidSurfaceCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    create_info.window = window;

    VkResult result = vkCreateAndroidSurfaceKHR(m_instance, &create_info, nullptr, &m_surface);
    if (result != VK_SUCCESS) {
        LOGE("vkCreateAndroidSurfaceKHR failed: %d", result);
        return false;
    }

    LOGI("Vulkan surface created");
    return true;
}

bool VulkanRenderer::select_physical_device() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

    if (device_count == 0) {
        LOGE("No Vulkan-compatible GPUs found");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

    // Select first suitable device (in production, would score devices)
    for (const auto& device : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);

        LOGI("Found GPU: %s", props.deviceName);

        // Check queue families
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        bool has_graphics = false;
        bool has_present = false;

        for (uint32_t i = 0; i < queue_family_count; i++) {
            if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                has_graphics = true;
            }

            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &present_support);
            if (present_support) {
                has_present = true;
            }
        }

        if (has_graphics && has_present) {
            m_physical_device = device;
            LOGI("Selected GPU: %s", props.deviceName);
            return true;
        }
    }

    LOGE("No suitable GPU found");
    return false;
}

bool VulkanRenderer::create_logical_device() {
    // Find queue families
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, queue_families.data());

    uint32_t graphics_family = UINT32_MAX;
    uint32_t present_family = UINT32_MAX;

    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_family = i;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physical_device, i, m_surface, &present_support);
        if (present_support) {
            present_family = i;
        }

        if (graphics_family != UINT32_MAX && present_family != UINT32_MAX) {
            break;
        }
    }

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::array<uint32_t, 2> unique_families = {graphics_family, present_family};

    float queue_priority = 1.0f;
    for (uint32_t family : unique_families) {
        if (family == UINT32_MAX) continue;

        bool already_added = false;
        for (const auto& info : queue_create_infos) {
            if (info.queueFamilyIndex == family) {
                already_added = true;
                break;
            }
        }
        if (already_added) continue;

        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features{};

    std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();

    VkResult result = vkCreateDevice(m_physical_device, &create_info, nullptr, &m_device);
    if (result != VK_SUCCESS) {
        LOGE("vkCreateDevice failed: %d", result);
        return false;
    }

    vkGetDeviceQueue(m_device, graphics_family, 0, &m_graphics_queue);
    vkGetDeviceQueue(m_device, present_family, 0, &m_present_queue);

    LOGI("Vulkan logical device created");
    return true;
}

bool VulkanRenderer::create_swapchain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &format_count, formats.data());

    // Choose format
    VkSurfaceFormatKHR surface_format = formats[0];
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = format;
            break;
        }
    }
    m_swapchain_format = surface_format.format;

    // Choose extent
    if (capabilities.currentExtent.width != UINT32_MAX) {
        m_swapchain_extent = capabilities.currentExtent;
    } else {
        m_swapchain_extent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, m_config.width));
        m_swapchain_extent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, m_config.height));
    }

    uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = m_surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = m_swapchain_extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform = capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    create_info.presentMode = m_config.vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swapchain);
    if (result != VK_SUCCESS) {
        LOGE("vkCreateSwapchainKHR failed: %d", result);
        return false;
    }

    // Get swapchain images
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, nullptr);
    m_swapchain_images.resize(image_count);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, m_swapchain_images.data());

    // Create image views
    m_swapchain_image_views.resize(m_swapchain_images.size());
    for (size_t i = 0; i < m_swapchain_images.size(); i++) {
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = m_swapchain_images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = m_swapchain_format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &view_info, nullptr, &m_swapchain_image_views[i]) != VK_SUCCESS) {
            LOGE("Failed to create image view %zu", i);
            return false;
        }
    }

    LOGI("Swapchain created: %dx%d, %zu images", m_swapchain_extent.width, m_swapchain_extent.height, m_swapchain_images.size());
    return true;
}

bool VulkanRenderer::create_render_pass() {
    VkAttachmentDescription color_attachment{};
    color_attachment.format = m_swapchain_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device, &render_pass_info, nullptr, &m_render_pass) != VK_SUCCESS) {
        LOGE("Failed to create render pass");
        return false;
    }

    LOGI("Render pass created");
    return true;
}

bool VulkanRenderer::create_framebuffers() {
    m_framebuffers.resize(m_swapchain_image_views.size());

    for (size_t i = 0; i < m_swapchain_image_views.size(); i++) {
        VkImageView attachments[] = {m_swapchain_image_views[i]};

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = m_render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = m_swapchain_extent.width;
        framebuffer_info.height = m_swapchain_extent.height;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &m_framebuffers[i]) != VK_SUCCESS) {
            LOGE("Failed to create framebuffer %zu", i);
            return false;
        }
    }

    LOGI("Framebuffers created: %zu", m_framebuffers.size());
    return true;
}

bool VulkanRenderer::create_command_pool() {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, queue_families.data());

    uint32_t graphics_family = 0;
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_family = i;
            break;
        }
    }

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = graphics_family;

    if (vkCreateCommandPool(m_device, &pool_info, nullptr, &m_command_pool) != VK_SUCCESS) {
        LOGE("Failed to create command pool");
        return false;
    }

    LOGI("Command pool created");
    return true;
}

bool VulkanRenderer::create_command_buffers() {
    m_command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = m_command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = static_cast<uint32_t>(m_command_buffers.size());

    if (vkAllocateCommandBuffers(m_device, &alloc_info, m_command_buffers.data()) != VK_SUCCESS) {
        LOGE("Failed to allocate command buffers");
        return false;
    }

    LOGI("Command buffers allocated: %zu", m_command_buffers.size());
    return true;
}

bool VulkanRenderer::create_sync_objects() {
    m_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fence_info, nullptr, &m_in_flight_fences[i]) != VK_SUCCESS) {
            LOGE("Failed to create sync objects for frame %zu", i);
            return false;
        }
    }

    LOGI("Sync objects created");
    return true;
}

bool VulkanRenderer::create_pipeline() {
    // Basic pipeline layout (no descriptors for now)
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS) {
        LOGE("Failed to create pipeline layout");
        return false;
    }

    LOGI("Pipeline layout created (basic)");
    // Note: Full graphics pipeline with shaders would be created here
    // For now, we just clear the screen
    return true;
}

void VulkanRenderer::cleanup_swapchain() {
    for (auto framebuffer : m_framebuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
    m_framebuffers.clear();

    for (auto image_view : m_swapchain_image_views) {
        vkDestroyImageView(m_device, image_view, nullptr);
    }
    m_swapchain_image_views.clear();

    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }

    if (m_render_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_render_pass, nullptr);
        m_render_pass = VK_NULL_HANDLE;
    }

    if (m_pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
        m_pipeline_layout = VK_NULL_HANDLE;
    }

    if (m_graphics_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_graphics_pipeline, nullptr);
        m_graphics_pipeline = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::recreate_swapchain() {
    vkDeviceWaitIdle(m_device);
    cleanup_swapchain();
    create_swapchain();
    create_render_pass();
    create_framebuffers();
}

bool VulkanRenderer::begin_frame() {
    if (!m_initialized) return false;

    vkWaitForFences(m_device, 1, &m_in_flight_fences[m_current_frame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
                                             m_image_available_semaphores[m_current_frame],
                                             VK_NULL_HANDLE, &m_image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOGE("Failed to acquire swapchain image");
        return false;
    }

    vkResetFences(m_device, 1, &m_in_flight_fences[m_current_frame]);
    vkResetCommandBuffer(m_command_buffers[m_current_frame], 0);

    // Begin command buffer
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(m_command_buffers[m_current_frame], &begin_info);

    // Begin render pass
    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = m_render_pass;
    render_pass_info.framebuffer = m_framebuffers[m_image_index];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = m_swapchain_extent;

    VkClearValue clear_color = {{{m_clear_color[0], m_clear_color[1], m_clear_color[2], m_clear_color[3]}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(m_command_buffers[m_current_frame], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    m_ui_elements.clear();
    return true;
}

void VulkanRenderer::end_frame() {
    if (!m_initialized) return;

    // End render pass
    vkCmdEndRenderPass(m_command_buffers[m_current_frame]);
    vkEndCommandBuffer(m_command_buffers[m_current_frame]);

    // Submit
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {m_image_available_semaphores[m_current_frame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_command_buffers[m_current_frame];

    VkSemaphore signal_semaphores[] = {m_render_finished_semaphores[m_current_frame]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, m_in_flight_fences[m_current_frame]) != VK_SUCCESS) {
        LOGE("Failed to submit draw command buffer");
    }

    // Present
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchains[] = {m_swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &m_image_index;

    VkResult result = vkQueuePresentKHR(m_present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreate_swapchain();
    }

    m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    m_stats.frame_count++;
}

void VulkanRenderer::draw_element(const UIElement& element) {
    if (!element.visible) return;
    m_ui_elements.push_back(element);
    // Actual rendering would happen in end_frame() with shaders
}

void VulkanRenderer::draw_text(const std::string& text, float x, float y,
                                uint32_t color, float size) {
    UIElement element;
    element.type = UIElementType::Text;
    element.x = x;
    element.y = y;
    element.text = text;
    element.color = color;
    draw_element(element);
}

void VulkanRenderer::draw_rect(float x, float y, float width, float height, uint32_t color) {
    UIElement element;
    element.type = UIElementType::Panel;
    element.x = x;
    element.y = y;
    element.width = width;
    element.height = height;
    element.color = color;
    draw_element(element);
}

void VulkanRenderer::draw_progress_bar(float x, float y, float width, float height,
                                        float progress, uint32_t bg_color, uint32_t fg_color) {
    // Background
    draw_rect(x, y, width, height, bg_color);
    // Foreground (progress)
    draw_rect(x, y, width * progress, height, fg_color);
}

void VulkanRenderer::resize(uint32_t width, uint32_t height) {
    m_config.width = width;
    m_config.height = height;
    recreate_swapchain();
}

void VulkanRenderer::set_clear_color(float r, float g, float b, float a) {
    m_clear_color[0] = r;
    m_clear_color[1] = g;
    m_clear_color[2] = b;
    m_clear_color[3] = a;
}

} // namespace vulkan
} // namespace downloader
