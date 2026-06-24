#ifndef SYSTEM_H
#define SYSTEM_H

#ifndef CLOSE_NATIVE_CODE
#ifdef ANDROID_OS
#include <jni.h>
#endif
#endif

#include <string>

namespace MediaLibrary {

class ImageFrame;

#ifndef CLOSE_NATIVE_CODE
#ifdef ANDROID_OS
bool isAttachCurrentThread();
JNIEnv* attachCurrentThread();
void detachCurrentThread();
JNIEnv* attachCurrentThreadIfNeeded(bool* isNeeded = nullptr);
#endif
#endif

std::string getDumpDataPath();

void dumpFrame(ImageFrame *imageFrame, const std::string &fileName);
bool dumpDataToFile(const std::string &filePath, const uint8_t *data, const int dataSize);
FILE* openDumpFile(const std::string &filePath);
void closeDumpFile(FILE* file);
bool dumpDataToFile(FILE *file, const uint8_t *data, const int dataSize);

bool fileExists(const std::string& filePath);
bool deleteFile(const std::string& filePath);

}

#endif // SYSTEM_H
