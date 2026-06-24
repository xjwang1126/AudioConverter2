#include "system.h"

#include "Model/imageframe.h"

namespace MediaLibrary {

void dumpFrame(ImageFrame *imageFrame, const std::string &fileName)
{
    static int count = 0;
    const MSize size = imageFrame->getSize();
    string filePath = getDumpDataPath() + "/"
            + fileName + "_" + std::to_string(static_cast<int>(size.getWidth())) + "_" + std::to_string(static_cast<int>(size.getHeight()))
            + "_" + std::to_string(count) + ".rgba";
    if (fileExists(filePath)) {
        deleteFile(filePath);
    }
    dumpDataToFile(filePath, static_cast<uint8_t*>(imageFrame->getFrame()), size.getWidth() * size.getHeight() * 4);
    count++;
}

bool dumpDataToFile(const string &filePath, const uint8_t *data, const int dataSize)
{
    FILE *file = openDumpFile(filePath.c_str());
    if (!file) {
        return false;
    }

    dumpDataToFile(file, data, dataSize);

    closeDumpFile(file);

    return true;
}

FILE* openDumpFile(const string &filePath)
{
    FILE *file = fopen(filePath.c_str(), "wb");
    return file;
}

void closeDumpFile(FILE* file)
{
    if (!file) {
        return;
    }

    fclose(file);
    file = nullptr;
}

bool dumpDataToFile(FILE *file, const uint8_t *data, const int dataSize)
{
    if (!file) {
        return false;
    }

    fwrite(data, static_cast<size_t>(dataSize), 1, file);

    return true;
}

}
