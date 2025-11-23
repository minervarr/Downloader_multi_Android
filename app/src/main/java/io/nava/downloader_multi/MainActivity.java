package io.nava.downloader_multi;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.TextView;

import java.io.File;

import io.nava.downloader_multi.databinding.ActivityMainBinding;

/**
 * Main Activity for Downloader Multi
 *
 * This activity handles:
 * - Native library initialization
 * - Vulkan surface management
 * - Video extraction and download coordination
 */
public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {
    private static final String TAG = "MainActivity";

    // Load native library
    static {
        System.loadLibrary("downloader_multi");
    }

    private ActivityMainBinding binding;
    private SurfaceView surfaceView;
    private boolean vulkanInitialized = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Display native library info
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI() + "\n" + getBuildInfo());

        // Initialize native components
        initializeNative();

        Log.i(TAG, "MainActivity created");
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        cleanupNative();
        Log.i(TAG, "MainActivity destroyed");
    }

    @Override
    protected void onPause() {
        super.onPause();
        // Could pause rendering here
    }

    @Override
    protected void onResume() {
        super.onResume();
        // Could resume rendering here
    }

    // =========================================================================
    // Native Initialization
    // =========================================================================

    private void initializeNative() {
        // Get Android directories
        String cacheDir = getCacheDir().getAbsolutePath();
        String filesDir = getFilesDir().getAbsolutePath();
        String externalDir = getExternalFilesDir(null) != null
                ? getExternalFilesDir(null).getAbsolutePath()
                : filesDir;

        // Initialize native app
        boolean success = nativeInit(cacheDir, filesDir, externalDir);
        Log.i(TAG, "Native init: " + (success ? "SUCCESS" : "FAILED"));

        if (success) {
            // Test URL support
            String testUrl = "https://zoom.us/rec/play/test";
            boolean supported = isUrlSupported(testUrl);
            Log.i(TAG, "Zoom URL supported: " + supported);
        }
    }

    private void cleanupNative() {
        if (vulkanInitialized) {
            nativeCleanupVulkan();
            vulkanInitialized = false;
        }
        nativeCleanup();
    }

    // =========================================================================
    // SurfaceHolder.Callback for Vulkan
    // =========================================================================

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.i(TAG, "Surface created");
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.i(TAG, "Surface changed: " + width + "x" + height);

        Surface surface = holder.getSurface();
        if (surface != null && surface.isValid()) {
            if (!vulkanInitialized) {
                vulkanInitialized = nativeInitVulkan(surface, width, height);
                Log.i(TAG, "Vulkan init: " + (vulkanInitialized ? "SUCCESS" : "FAILED"));
            } else {
                nativeResize(width, height);
            }
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.i(TAG, "Surface destroyed");
        if (vulkanInitialized) {
            nativeCleanupVulkan();
            vulkanInitialized = false;
        }
    }

    // =========================================================================
    // Public API Methods
    // =========================================================================

    /**
     * Extract video information from URL
     * @param url Video URL
     * @return JSON string with video info
     */
    public String extractVideo(String url) {
        return extractVideoInfo(url);
    }

    /**
     * Start downloading a video
     * @param url Video URL
     * @param outputPath Output file path
     * @return Download ID
     */
    public long downloadVideo(String url, String outputPath) {
        return startDownload(url, outputPath);
    }

    /**
     * Get default download directory
     * @return Download directory path
     */
    public String getDownloadDirectory() {
        File downloadDir = getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
        if (downloadDir != null) {
            return downloadDir.getAbsolutePath();
        }
        return getFilesDir().getAbsolutePath() + "/Downloads";
    }

    // =========================================================================
    // Native Methods - Application Lifecycle
    // =========================================================================

    /**
     * Initialize native application
     * @param cacheDir App cache directory
     * @param filesDir App files directory
     * @param externalDir External storage directory
     * @return true if successful
     */
    public native boolean nativeInit(String cacheDir, String filesDir, String externalDir);

    /**
     * Cleanup native application
     */
    public native void nativeCleanup();

    // =========================================================================
    // Native Methods - Vulkan
    // =========================================================================

    /**
     * Initialize Vulkan renderer
     * @param surface Android Surface
     * @param width Surface width
     * @param height Surface height
     * @return true if successful
     */
    public native boolean nativeInitVulkan(Surface surface, int width, int height);

    /**
     * Cleanup Vulkan renderer
     */
    public native void nativeCleanupVulkan();

    /**
     * Render a frame
     */
    public native void nativeRenderFrame();

    /**
     * Handle surface resize
     * @param width New width
     * @param height New height
     */
    public native void nativeResize(int width, int height);

    // =========================================================================
    // Native Methods - Video Extraction
    // =========================================================================

    /**
     * Get native library version string
     */
    public native String stringFromJNI();

    /**
     * Get build configuration info
     */
    public native String getBuildInfo();

    /**
     * Extract video information from URL
     * @param url Video URL
     * @return JSON string with video info
     */
    public native String extractVideoInfo(String url);

    /**
     * Check if URL is supported
     * @param url URL to check
     * @return true if supported
     */
    public native boolean isUrlSupported(String url);

    // =========================================================================
    // Native Methods - Download Management
    // =========================================================================

    /**
     * Start a download
     * @param url Video URL
     * @param outputPath Output file path
     * @return Download ID, or -1 on error
     */
    public native long startDownload(String url, String outputPath);

    /**
     * Get download progress
     * @param downloadId Download ID
     * @return Progress (0.0 to 1.0)
     */
    public native float getDownloadProgress(long downloadId);

    /**
     * Cancel a download
     * @param downloadId Download ID
     * @return true if cancelled
     */
    public native boolean cancelDownload(long downloadId);

    /**
     * Get download status
     * @param downloadId Download ID
     * @return Status string
     */
    public native String getDownloadStatus(long downloadId);

    // =========================================================================
    // Native Methods - Cookie Management
    // =========================================================================

    /**
     * Load cookies from file
     * @param cookieFile Path to cookie file (Netscape format)
     * @return true if successful
     */
    public native boolean loadCookies(String cookieFile);

    // =========================================================================
    // Native Methods - Utility
    // =========================================================================

    /**
     * Get library version
     */
    public native String getVersion();

    /**
     * Get list of supported sites
     * @return JSON array of supported sites
     */
    public native String getSupportedSites();
}
