/**
 * @file vulkan_renderer.hpp
 * @brief Vulkan renderer for Android UI
 *
 * This provides the main interface for Vulkan-based rendering on Android,
 * integrated with SDL3 for window management.
 */

#ifndef VULKAN_RENDERER_HPP
#define VULKAN_RENDERER_HPP

#include <vulkan/vulkan.h>
#include <android/native_window.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace downloader {
namespace vulkan {

/**
 * @brief Configuration for Vulkan renderer
 */
struct RendererConfig {
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool enable_validation = false;
    bool vsync = true;
    std::string app_name = "Downloader Multi";
    uint32_t app_version = VK_MAKE_VERSION(1, 0, 0);
};

/**
 * @brief Render statistics
 */
struct RenderStats {
    float fps = 0.0f;
    float frame_time_ms = 0.0f;
    uint64_t frame_count = 0;
    size_t memory_usage = 0;
};

/**
 * @brief UI element types for rendering
 */
enum class UIElementType {
    Button,
    Text,
    ProgressBar,
    List,
    Image,
    Panel
};

/**
 * @brief UI element for rendering
 */
struct UIElement {
    UIElementType type;
    float x, y, width, height;
    std::string text;
    float progress = 0.0f;  // For progress bars
    uint32_t color = 0xFFFFFFFF;
    bool visible = true;
    bool enabled = true;
    std::function<void()> on_click;
};

/**
 * @brief Main Vulkan renderer class
 */
class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    // Non-copyable
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;

    /**
     * @brief Initialize Vulkan renderer with Android native window
     * @param window Android native window
     * @param config Renderer configuration
     * @return true if initialization succeeded
     */
    bool initialize(ANativeWindow* window, const RendererConfig& config = {});

    /**
     * @brief Cleanup Vulkan resources
     */
    void cleanup();

    /**
     * @brief Check if renderer is initialized
     */
    bool is_initialized() const { return m_initialized; }

    /**
     * @brief Begin a new frame
     * @return true if frame can be rendered
     */
    bool begin_frame();

    /**
     * @brief End current frame and present
     */
    void end_frame();

    /**
     * @brief Add UI element to render queue
     * @param element UI element to render
     */
    void draw_element(const UIElement& element);

    /**
     * @brief Draw text at position
     * @param text Text to draw
     * @param x X position
     * @param y Y position
     * @param color Text color (ARGB)
     * @param size Font size
     */
    void draw_text(const std::string& text, float x, float y,
                   uint32_t color = 0xFFFFFFFF, float size = 16.0f);

    /**
     * @brief Draw a rectangle
     * @param x X position
     * @param y Y position
     * @param width Width
     * @param height Height
     * @param color Fill color (ARGB)
     */
    void draw_rect(float x, float y, float width, float height, uint32_t color);

    /**
     * @brief Draw progress bar
     * @param x X position
     * @param y Y position
     * @param width Width
     * @param height Height
     * @param progress Progress value (0.0 to 1.0)
     * @param bg_color Background color
     * @param fg_color Foreground color
     */
    void draw_progress_bar(float x, float y, float width, float height,
                           float progress, uint32_t bg_color = 0xFF333333,
                           uint32_t fg_color = 0xFF00AA00);

    /**
     * @brief Handle window resize
     * @param width New width
     * @param height New height
     */
    void resize(uint32_t width, uint32_t height);

    /**
     * @brief Get render statistics
     */
    RenderStats get_stats() const { return m_stats; }

    /**
     * @brief Set clear color
     * @param r Red (0-1)
     * @param g Green (0-1)
     * @param b Blue (0-1)
     * @param a Alpha (0-1)
     */
    void set_clear_color(float r, float g, float b, float a = 1.0f);

private:
    bool create_instance();
    bool create_surface(ANativeWindow* window);
    bool select_physical_device();
    bool create_logical_device();
    bool create_swapchain();
    bool create_render_pass();
    bool create_framebuffers();
    bool create_command_pool();
    bool create_command_buffers();
    bool create_sync_objects();
    bool create_pipeline();

    void cleanup_swapchain();
    void recreate_swapchain();

private:
    bool m_initialized = false;
    RendererConfig m_config;
    RenderStats m_stats;

    // Vulkan handles
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphics_queue = VK_NULL_HANDLE;
    VkQueue m_present_queue = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkRenderPass m_render_pass = VK_NULL_HANDLE;
    VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
    VkPipeline m_graphics_pipeline = VK_NULL_HANDLE;
    VkCommandPool m_command_pool = VK_NULL_HANDLE;

    std::vector<VkImage> m_swapchain_images;
    std::vector<VkImageView> m_swapchain_image_views;
    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<VkCommandBuffer> m_command_buffers;
    std::vector<VkSemaphore> m_image_available_semaphores;
    std::vector<VkSemaphore> m_render_finished_semaphores;
    std::vector<VkFence> m_in_flight_fences;

    VkFormat m_swapchain_format;
    VkExtent2D m_swapchain_extent;
    uint32_t m_current_frame = 0;
    uint32_t m_image_index = 0;

    float m_clear_color[4] = {0.1f, 0.1f, 0.15f, 1.0f};

    // UI rendering state
    std::vector<UIElement> m_ui_elements;
};

} // namespace vulkan
} // namespace downloader

#endif // VULKAN_RENDERER_HPP
