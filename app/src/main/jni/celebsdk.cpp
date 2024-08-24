#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

#include <android/log.h>

#include <jni.h>

#include <string>
#include <vector>

#include <platform.h>
#include <benchmark.h>

#include "ndkcamera.h"
#include "scrfd.h"
#include "face_emb.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <android/bitmap.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;

#if __ARM_NEON
#include <arm_neon.h>
#endif // __ARM_NEON

static SCRFD *g_scrfd = 0;

static FaceEmb *g_faceEmb = 0;

static ncnn::Mutex lock;


static std::vector<FaceObject> faceObjects;
static std::vector<float> embedding;
static cv::Mat faceAligned;
static std::vector<float> resultLightTraffic;

class MyNdkCamera : public NdkCameraWindow {
public:
    virtual void on_image_render(cv::Mat &rgb) const;
};

void MyNdkCamera::on_image_render(cv::Mat &rgb) const {
    {
        ncnn::MutexLockGuard g(lock);
        if (g_scrfd) {
            g_scrfd->detect(rgb, faceObjects);
            if (faceObjects.size() > 0) {
                g_faceEmb->getEmbeding(rgb, faceObjects[0].landmark, embedding, faceAligned);
            }
            g_scrfd->draw(rgb, faceObjects);
        }
    }
}

static MyNdkCamera *g_camera = 0;

extern "C" {
JNIEXPORT jint
JNI_OnLoad(JavaVM *vm, void *reserved) {
    g_camera = new MyNdkCamera;

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    {
        ncnn::MutexLockGuard g(lock);
        delete g_scrfd;
        g_scrfd = 0;
        delete g_faceEmb;
        g_faceEmb = 0;
    }

    delete g_camera;
    g_camera = 0;
}


extern "C" jboolean
Java_com_tondz_celebrecognition_CelebSDK_loadModel(JNIEnv *env, jobject thiz, jobject assetManager,
                                            jint yoloDetect, jint faceDectector,
                                            jint trafficLight) {
    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
    ncnn::MutexLockGuard g(lock);
    const char *modeltype = "n";
    if (faceDectector == 1) {
        if (!g_scrfd)
            g_scrfd = new SCRFD;
        g_scrfd->load(mgr, modeltype, false);
        if (!g_faceEmb)
            g_faceEmb = new FaceEmb;
        g_faceEmb->load(mgr);
    } else {
        if (g_scrfd) {
            delete g_scrfd;
            delete g_faceEmb;
            g_scrfd = 0;
            g_faceEmb = 0;
        }


    }
    return JNI_TRUE;
}
extern "C" jboolean
Java_com_tondz_celebrecognition_CelebSDK_openCamera(JNIEnv *env, jobject thiz, jint facing) {
    if (facing < 0 || facing > 1)
        return JNI_FALSE;
    g_camera->open((int) facing);

    return JNI_TRUE;
}

extern "C" jboolean
Java_com_tondz_celebrecognition_CelebSDK_closeCamera(JNIEnv *env, jobject thiz) {
    g_camera->close();

    return JNI_TRUE;
}

extern "C" jboolean
Java_com_tondz_celebrecognition_CelebSDK_setOutputWindow(JNIEnv *env, jobject thiz, jobject surface) {
    ANativeWindow *win = ANativeWindow_fromSurface(env, surface);
    g_camera->set_window(win);
    return JNI_TRUE;
}

}
extern "C"
JNIEXPORT jstring JNICALL
Java_com_tondz_celebrecognition_CelebSDK_getEmbedding(JNIEnv *env, jobject thiz) {
    if (embedding.size() > 0) {
        std::ostringstream oss;

        // Convert each element to string and add it to the stream
        for (size_t i = 0; i < embedding.size(); ++i) {
            if (i != 0) {
                oss << ",";  // Add a separator between elements
            }
            oss << embedding[i];
        }

        // Convert the stream to a string
        std::string embeddingStr = oss.str();
        embedding.clear();
        return env->NewStringUTF(embeddingStr.c_str());
    }
    return env->NewStringUTF("");

}


jobject mat_to_bitmap(JNIEnv *env, Mat &src, bool needPremultiplyAlpha) {
    jclass java_bitmap_class = env->FindClass("android/graphics/Bitmap");
    jclass bmpCfgCls = env->FindClass("android/graphics/Bitmap$Config");
    jmethodID bmpClsValueOfMid = env->GetStaticMethodID(bmpCfgCls, "valueOf",
                                                        "(Ljava/lang/String;)Landroid/graphics/Bitmap$Config;");
    jobject jBmpCfg = env->CallStaticObjectMethod(bmpCfgCls, bmpClsValueOfMid,
                                                  env->NewStringUTF("ARGB_8888"));

    jmethodID mid = env->GetStaticMethodID(java_bitmap_class,
                                           "createBitmap",
                                           "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");

    jobject bitmap = env->CallStaticObjectMethod(java_bitmap_class,
                                                 mid, src.cols, src.rows,
                                                 jBmpCfg);

    AndroidBitmapInfo info;
    void *pixels = nullptr;


    // Validate
    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
        std::runtime_error("Failed to get Bitmap info.");
    }
    if (src.type() != CV_8UC1 && src.type() != CV_8UC3 && src.type() != CV_8UC4) {
        std::runtime_error("Unsupported cv::Mat type.");
    }
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
        std::runtime_error("Failed to lock Bitmap pixels.");
    }
    if (!pixels) {
        std::runtime_error("Bitmap pixels are null.");
    }

    // Convert cv::Mat to the Bitmap format
    if (info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
        Mat tmp(info.height, info.width, CV_8UC4, pixels);
        if (src.type() == CV_8UC1) {
            cvtColor(src, tmp, COLOR_GRAY2RGBA);
        } else if (src.type() == CV_8UC3) {
            cvtColor(src, tmp, COLOR_RGB2RGBA);
        } else if (src.type() == CV_8UC4) {
            if (needPremultiplyAlpha) {
                cvtColor(src, tmp, COLOR_RGBA2mRGBA);
            } else {
                src.copyTo(tmp);
            }
        }
    } else if (info.format == ANDROID_BITMAP_FORMAT_RGB_565) {
        Mat tmp(info.height, info.width, CV_8UC2, pixels);
        if (src.type() == CV_8UC1) {
            cvtColor(src, tmp, COLOR_GRAY2BGR565);
        } else if (src.type() == CV_8UC3) {
            cvtColor(src, tmp, COLOR_RGB2BGR565);
        } else if (src.type() == CV_8UC4) {
            cvtColor(src, tmp, COLOR_RGBA2BGR565);
        }
    }

    AndroidBitmap_unlockPixels(env, bitmap);
    return bitmap;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_tondz_celebrecognition_CelebSDK_getFaceAlign(JNIEnv *env, jobject thiz) {
    jobject bitmap = mat_to_bitmap(env, faceAligned, false);
    return bitmap;
}
extern "C"
JNIEXPORT jstring JNICALL
Java_com_tondz_celebrecognition_CelebSDK_getEmbeddingFromPath(JNIEnv *env, jobject thiz, jstring path) {
    std::vector<float> result;
    jboolean isCopy;
    const char *convertedValue = (env)->GetStringUTFChars(path, &isCopy);
    std::string strPath = convertedValue;
    static std::vector<FaceObject> faceObjects1;
    static std::vector<float> embedding1;
    static cv::Mat faceAligned1;


    cv::Mat bgr = imread(strPath, IMREAD_COLOR);
    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    g_scrfd->detect(rgb, faceObjects1);
    __android_log_print(ANDROID_LOG_DEBUG, "LOGFACE", "len face %f", faceObjects1[0].rect.width);
    if (faceObjects1.size() > 0) {
        g_faceEmb->getEmbeding(bgr, faceObjects1[0].landmark, embedding1, faceAligned1);
        __android_log_print(ANDROID_LOG_DEBUG, "LOGFACE", "len embedding %zu", embedding1.size());
        std::ostringstream oss;

        // Convert each element to string and add it to the stream
        for (size_t i = 0; i < embedding1.size(); ++i) {
            if (i != 0) {
                oss << ",";  // Add a separator between elements
            }
            oss << embedding1[i];
        }

        // Convert the stream to a string
        std::string embeddingStr = oss.str();
        return env->NewStringUTF(embeddingStr.c_str());
    }
    return env->NewStringUTF("");
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_tondz_celebrecognition_CelebSDK_getFaceAlignFromPath(JNIEnv *env, jobject thiz, jstring path) {
    std::vector<float> result;
    jboolean isCopy;
    const char *convertedValue = (env)->GetStringUTFChars(path, &isCopy);
    std::string strPath = convertedValue;
    static std::vector<FaceObject> faceObjects1;
    static std::vector<float> embedding1;
    static cv::Mat faceAligned1;


    cv::Mat bgr = imread(strPath, IMREAD_COLOR);
    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);


    g_scrfd->detect(rgb, faceObjects1);
    __android_log_print(ANDROID_LOG_DEBUG, "LOGFACE", "len face %f", faceObjects1[0].rect.width);
    if (faceObjects1.size() > 0) {
        g_faceEmb->getEmbeding(rgb, faceObjects1[0].landmark, embedding1, faceAligned1);
        jobject bitmap = mat_to_bitmap(env, faceAligned1, false);
        return bitmap;
    }
    jobject bitmap = mat_to_bitmap(env, faceAligned1, false);
    return bitmap;
}