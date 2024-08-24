package com.tondz.celebrecognition;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.view.Surface;

public class CelebSDK {
    public native boolean loadModel(AssetManager assetManager, int yoloDetect, int faceDectector, int trafficLight);

    public native boolean openCamera(int facing);

    public native boolean closeCamera();

    public native boolean setOutputWindow(Surface surface);


    public native String getEmbedding();

    public native Bitmap getFaceAlign();

    public native String getEmbeddingFromPath(String path);
    public native Bitmap getFaceAlignFromPath(String path);

    static {
        System.loadLibrary("celebsdk");
    }
}
