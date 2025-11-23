package io.nava.downloader_multi;

import org.libsdl.app.SDLActivity;

/**
 * Minimal MainActivity that extends SDL3's SDLActivity
 *
 * All application logic is handled in C++ through SDL3.
 * This Java class only provides the Android entry point.
 */
public class MainActivity extends SDLActivity {

    /**
     * Return the name of the native library to load.
     * This must match the library name in CMakeLists.txt
     */
    @Override
    protected String[] getLibraries() {
        return new String[]{
                "c++_shared",
                "downloader_multi"
        };
    }

    /**
     * Return the arguments to pass to SDL_main()
     */
    @Override
    protected String[] getArguments() {
        return new String[0];
    }
}
