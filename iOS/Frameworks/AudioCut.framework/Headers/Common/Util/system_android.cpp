#include "system.h"

#include <thread>
#include <cassert>
#include <string>
#include <sstream>

using namespace std;

namespace MediaLibrary {

#ifndef CLOSE_NATIVE_CODE
JavaVM* g_jvm = nullptr;

bool isAttachCurrentThread()
{
    bool result = false;

    do {
        if (!g_jvm) {
            break;
        }

        JNIEnv* env = nullptr;
        jint status = g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        if (status == JNI_OK) {
            result = true;
        }
    } while (false);

    return result;
}

JNIEnv* attachCurrentThread()
{
    JNIEnv* env = nullptr;

    do {
        if (!g_jvm) {
            break;
        }

        JavaVMAttachArgs args;
        args.version = JNI_VERSION_1_6;
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        string threadName = oss.str();
        args.name = threadName.c_str();
        args.group = nullptr;

        if (g_jvm->AttachCurrentThread(&env, &args) != JNI_OK) {
            break;
        }
    } while (false);

    return env;
}

void detachCurrentThread()
{
    if (!g_jvm) {
        return;
    }

    if (g_jvm->DetachCurrentThread() != JNI_OK) {
        assert(0);
    }
}

JNIEnv* attachCurrentThreadIfNeeded(bool *isNeeded)
{
    JNIEnv* env = nullptr;

    if (isNeeded) {
        *isNeeded = false;
    }

    jint status = g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    switch (status) {
    case JNI_EDETACHED:
    {
        JavaVMAttachArgs args;
        args.version = JNI_VERSION_1_6;
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        string threadName = oss.str();
        args.name = threadName.c_str();
        args.group = nullptr;

        if (g_jvm->AttachCurrentThread(&env, &args) != JNI_OK) {
            assert(0);
        }

        if (isNeeded) {
            *isNeeded = true;
        }
    }
        break;
    case JNI_OK:
    {
    }
        break;
    case JNI_EVERSION:
    {
    }
        break;
    default:
    {
        assert(0);
    }
        break;
    }

    return env;
}
#endif

std::string getDumpDataPath()
{
    return "/mnt/sdcard";
}

bool fileExists(const std::string& filePath)
{
#ifndef CLOSE_NATIVE_CODE
    if (filePath.empty()) {
        return false;
    }

    FILE* file = fopen(filePath.c_str(), "r");
    if (file) {
        fclose(file);
        return true;
    }
    DIR* dir = opendir(filePath.c_str());
    if (dir) {
        closedir(dir);
        return true;
    }
    return false;
#else
    (void)filePath;
    return false;
#endif
}

bool deleteFile(const std::string& filePath)
{
    (void)filePath;
    return false;
}

}
