package io.nava.downloader_multi;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import io.nava.downloader_multi.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'downloader_multi' library on application startup.
    static {
        System.loadLibrary("downloader_multi");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());
    }

    /**
     * A native method that is implemented by the 'downloader_multi' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}