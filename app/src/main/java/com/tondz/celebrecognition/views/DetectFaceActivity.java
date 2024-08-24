package com.tondz.celebrecognition.views;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.speech.tts.TextToSpeech;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.tondz.celebrecognition.CelebSDK;
import com.tondz.celebrecognition.R;
import com.tondz.celebrecognition.utils.Common;

import java.util.List;
import java.util.Locale;

public class DetectFaceActivity extends AppCompatActivity implements SurfaceHolder.Callback {
    CelebSDK yolov8Ncnn = new CelebSDK();
    private SurfaceView cameraView;
    private static final int REQUEST_CAMERA = 510;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_detect_face);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        init();
        reload();
        click();
    }


    private void click() {
        findViewById(R.id.btnCapture).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String stringEmb = yolov8Ncnn.getEmbedding();
                if (!stringEmb.isEmpty()) {
                    ThemNguoiThanActivity.embedding = stringEmb;
                    ThemNguoiThanActivity.bitmap = yolov8Ncnn.getFaceAlign();
                    ThemNguoiThanActivity.imgAvatar.setImageBitmap(ThemNguoiThanActivity.bitmap);
                    onBackPressed();
                }
            }
        });
    }

    private void init() {
        cameraView = findViewById(R.id.cameraview);

        cameraView.getHolder().addCallback(this);

    }


    private void reload() {
        boolean ret_init = yolov8Ncnn.loadModel(getAssets(), 0,1,0);
        if (!ret_init) {
            Log.e("MainActivity", "yolov8ncnn loadModel failed");
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        yolov8Ncnn.setOutputWindow(holder.getSurface());


    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
    }

    @Override
    public void onResume() {
        super.onResume();

        if (ContextCompat.checkSelfPermission(getApplicationContext(), android.Manifest.permission.CAMERA) == PackageManager.PERMISSION_DENIED) {
            ActivityCompat.requestPermissions(this, new String[]{android.Manifest.permission.CAMERA}, REQUEST_CAMERA);
        }

        yolov8Ncnn.openCamera(1);
    }

    @Override
    public void onPause() {
        super.onPause();

        yolov8Ncnn.closeCamera();
    }
}