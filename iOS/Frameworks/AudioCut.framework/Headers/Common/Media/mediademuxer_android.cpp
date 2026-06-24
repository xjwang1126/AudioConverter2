#include "mediademuxer.h"
#include "Util/system.h"
#include "Model/imageframe.h"

#ifndef CLOSE_NATIVE_CODE
#include <android/bitmap.h>
#endif

namespace MediaLibrary {

#if 0
bool MediaDemuxer::openPicture(const MSize &size)
{
    ImageFrame* pictureFrame = nullptr;

    if (filePath_.empty()) {
        return false;
    }

    JNIEnv* env = attachCurrentThreadIfNeeded();
    if (!env) {
        return false;
    }

    jstring jPathName = nullptr;

    do {
        jclass bitmapFactoryCls = env->FindClass("android/graphics/BitmapFactory");
        if (env->ExceptionCheck() || !bitmapFactoryCls) {
            env->ExceptionDescribe();
            break;
        }

        jmethodID decodeFileMethodID = env->GetStaticMethodID(bitmapFactoryCls, "decodeFile", "(Ljava/lang/String;)Landroid/graphics/Bitmap;");
        if (env->ExceptionCheck() || !decodeFileMethodID) {
            env->ExceptionDescribe();
            break;
        }

        jPathName = env->NewStringUTF(filePath_.c_str());

        jobject bitmap = env->CallStaticObjectMethod(bitmapFactoryCls, decodeFileMethodID, jPathName);
        if (env->ExceptionCheck() || !bitmap) {
            env->ExceptionDescribe();
            break;
        }

        if (size.getWidth() > 0 && size.getHeight() > 0) {
            jclass bitmapCls = env->FindClass("android/graphics/Bitmap");
            if (env->ExceptionCheck() || !bitmapCls) {
                env->ExceptionDescribe();
                break;
            }

            jmethodID createScaledBitmapMethodID = env->GetStaticMethodID(bitmapCls, "createScaledBitmap", "(Landroid/graphics/Bitmap;IIZ)Landroid/graphics/Bitmap;");
            if (env->ExceptionCheck() || !createScaledBitmapMethodID) {
                env->ExceptionDescribe();
                break;
            }

            jobject scaleBitmap = env->CallStaticObjectMethod(bitmapCls, createScaledBitmapMethodID, bitmap, size.getWidth(), size.getHeight(), true);

            if (scaleBitmap != bitmap) {
                env->DeleteLocalRef(bitmap);
                bitmap = nullptr;

                bitmap = scaleBitmap;
                scaleBitmap = nullptr;
            }
        }

        void* data = nullptr;
        AndroidBitmap_lockPixels(env, bitmap, &data);
        AndroidBitmapInfo info;
        AndroidBitmap_getInfo(env, bitmap, &info);
        //Note, Only process color format RGBA888, later process other color format.
        if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
            assert(0);
        }
        uint8_t* frame = new uint8_t[info.width * info.height * 4];
        for (uint32_t i = 0; i < info.height; i++) {
            memcpy(frame + i * info.width * 4, static_cast<uint8_t*>(data) + i * info.stride, info.width * 4);
        }
        pictureFrame = new ImageFrame;
        pictureFrame->setTimeBase(TimeBase(1, SECOND_TO_MILLISECOND_UNIT));
        pictureFrame->setFrame(frame);
        pictureFrame->setSize(MSize(info.width, info.height));
        AndroidBitmap_unlockPixels(env, bitmap);

        env->DeleteLocalRef(bitmap);
        bitmap = nullptr;
    } while(false);

    if (jPathName) {
        env->DeleteLocalRef(jPathName);
        jPathName = nullptr;
    }

    pictureFrame_ = pictureFrame;

    return true;
}
#endif

}
