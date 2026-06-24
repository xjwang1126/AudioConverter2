#include "mediamuxer.h"
#include "Util/system.h"
#include "Model/imageframe.h"

namespace MediaLibrary {

bool MediaMuxer::savePicture(ImageFrame *imageFrame)
{
#ifndef CLOSE_NATIVE_CODE
    bool result = false;

    JNIEnv* env = attachCurrentThreadIfNeeded();
    if (!env) {
        return false;
    }

    jclass fileCls = nullptr;
    jclass fileOutputStreamCls = nullptr;
    jclass bitmapCls = nullptr;
    jclass configCls = nullptr;
    jclass compressFormatCls = nullptr;

    jobject jFile = nullptr;
    jobject jFileOutputStream = nullptr;
    jobject jBitmap = nullptr;
    jintArray jColors = nullptr;
    jobject jConfig = nullptr;
    jobject jCompressFormat = nullptr;

    jstring jFilepath = env->NewStringUTF(filePath_.c_str());

    do {
        if (filePath_.empty() || !imageFrame) {
            break;
        }

        string suffix;
        const size_t suffixIndex = filePath_.rfind(".");
        if (suffixIndex != string::npos) {
            suffix = filePath_.substr(suffixIndex, filePath_.length() - suffixIndex);
        }
        std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
        const bool isPNGSuffix = !suffix.compare(".png");
        const bool isJPGSuffix = !suffix.compare(".jpg") || !suffix.compare(".jpeg");
        if (!isPNGSuffix && !isJPGSuffix) {
            break;
        }

        fileCls = env->FindClass("java/io/File");
        if (env->ExceptionCheck() || !fileCls) {
            env->ExceptionDescribe();
            break;
        }
        jmethodID fileInitMethodId = env->GetMethodID(fileCls, "<init>", "(Ljava/lang/String;)V");
        if (env->ExceptionCheck() || !fileInitMethodId) {
            env->ExceptionDescribe();
            break;
        }
        jFile = env->NewObject(fileCls, fileInitMethodId, jFilepath);
        if (env->ExceptionCheck() || !jFile) {
            env->ExceptionDescribe();
            break;
        }

        fileOutputStreamCls = env->FindClass("java/io/FileOutputStream");
        if (env->ExceptionCheck() || !fileOutputStreamCls) {
            env->ExceptionDescribe();
            break;
        }
        jmethodID fileOutputStreamInitMethodId = env->GetMethodID(fileOutputStreamCls, "<init>", "(Ljava/io/File;)V");
        if (env->ExceptionCheck() || !fileOutputStreamInitMethodId) {
            env->ExceptionDescribe();
            break;
        }
        jFileOutputStream = env->NewObject(fileOutputStreamCls, fileOutputStreamInitMethodId, jFile);
        if (env->ExceptionCheck() || !jFileOutputStream) {
            env->ExceptionDescribe();
            break;
        }

        bitmapCls = env->FindClass("android/graphics/Bitmap");
        if (env->ExceptionCheck() || !bitmapCls) {
            env->ExceptionDescribe();
            break;
        }
        jmethodID createBitmapMethodId = env->GetStaticMethodID(bitmapCls, "createBitmap", "([IIILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
        if (env->ExceptionCheck() || !createBitmapMethodId) {
            env->ExceptionDescribe();
            break;
        }
        const uint8_t* data = static_cast<uint8_t*>(imageFrame->getFrame());
        const MSize size = imageFrame->getSize();
        const int width = size.getWidth();
        const int height = size.getHeight();
        jColors = env->NewIntArray(width * height);
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                uint8_t* color = const_cast<uint8_t*>(data + width * 4 * j + 4 * i);
                const uint8_t red = *(color + 0);
                const uint8_t green = *(color + 1);
                const uint8_t blue = *(color + 2);
                const uint8_t alpha = *(color + 3);
                *(color + 0) = blue;
                *(color + 1) = green;
                *(color + 2) = red;
                *(color + 3) = alpha;
            }
        }
        env->SetIntArrayRegion(jColors, 0, width * height, reinterpret_cast<const jint*>(data));
        configCls = env->FindClass("android/graphics/Bitmap$Config");
        if (env->ExceptionCheck() || !configCls) {
            env->ExceptionDescribe();
            break;
        }
        jfieldID configFieldId = env->GetStaticFieldID(configCls, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
        if (env->ExceptionCheck() || !configFieldId) {
            env->ExceptionDescribe();
            break;
        }
        jConfig = env->GetStaticObjectField(configCls, configFieldId);
        if (env->ExceptionCheck() || !jConfig) {
            env->ExceptionDescribe();
            break;
        }
        jBitmap = env->CallStaticObjectMethod(bitmapCls, createBitmapMethodId, jColors, width, height, jConfig);
        if (env->ExceptionCheck() || !jBitmap) {
            env->ExceptionDescribe();
            break;
        }
        compressFormatCls = env->FindClass("android/graphics/Bitmap$CompressFormat");
        if (env->ExceptionCheck() || !compressFormatCls) {
            env->ExceptionDescribe();
            break;
        }
        jfieldID compressFormatFieldId = nullptr;
        if (isPNGSuffix) {
            compressFormatFieldId = env->GetStaticFieldID(compressFormatCls, "PNG", "Landroid/graphics/Bitmap$CompressFormat;");
        } else {
            compressFormatFieldId = env->GetStaticFieldID(compressFormatCls, "JPEG", "Landroid/graphics/Bitmap$CompressFormat;");
        }
        if (env->ExceptionCheck() || !compressFormatFieldId) {
            env->ExceptionDescribe();
            break;
        }
        jCompressFormat = env->GetStaticObjectField(compressFormatCls, compressFormatFieldId);
        if (env->ExceptionCheck() || !jCompressFormat) {
            env->ExceptionDescribe();
            break;
        }
        jmethodID compressMethodId = env->GetMethodID(bitmapCls, "compress", "(Landroid/graphics/Bitmap$CompressFormat;ILjava/io/OutputStream;)Z");
        if (env->ExceptionCheck() || !compressMethodId) {
            env->ExceptionDescribe();
            break;
        }
        jboolean compressResult = env->CallBooleanMethod(jBitmap, compressMethodId, jCompressFormat, 100, jFileOutputStream);

        jmethodID closeMethodId = env->GetMethodID(fileOutputStreamCls, "close", "()V");
        if (env->ExceptionCheck() || !closeMethodId) {
            env->ExceptionDescribe();
            break;
        }
        env->CallVoidMethod(jFileOutputStream, closeMethodId);

        result = compressResult;
    } while (false);

    env->DeleteLocalRef(jFilepath);
    jFilepath = nullptr;

    if (jCompressFormat) {
        env->DeleteLocalRef(jCompressFormat);
        jCompressFormat = nullptr;
    }
    if (jConfig) {
        env->DeleteLocalRef(jConfig);
        jConfig = nullptr;
    }
    if (jColors) {
        env->DeleteLocalRef(jColors);
        jColors = nullptr;
    }
    if (jBitmap) {
        env->DeleteLocalRef(jBitmap);
        jBitmap = nullptr;
    }
    if (jFileOutputStream) {
        env->DeleteLocalRef(jFileOutputStream);
        jFileOutputStream = nullptr;
    }
    if (jFile) {
        env->DeleteLocalRef(jFile);
        jFile = nullptr;
    }

    if (fileCls) {
        env->DeleteLocalRef(fileCls);
        fileCls = nullptr;
    }
    if (fileOutputStreamCls) {
        env->DeleteLocalRef(fileOutputStreamCls);
        fileOutputStreamCls = nullptr;
    }
    if (bitmapCls) {
        env->DeleteLocalRef(bitmapCls);
        bitmapCls = nullptr;
    }
    if (configCls) {
        env->DeleteLocalRef(configCls);
        configCls = nullptr;
    }
    if (compressFormatCls) {
        env->DeleteLocalRef(compressFormatCls);
        compressFormatCls = nullptr;
    }

    return result;
#else
    (void)imageFrame;
    return false;
#endif
}

}
