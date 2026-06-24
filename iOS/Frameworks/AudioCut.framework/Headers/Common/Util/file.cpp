#include "file.h"

namespace MediaLibrary {

File::File()
    : filePath_("")
{
}

File::File(const string &filePath)
    : filePath_(filePath)
{
}

File::File(const File &file)
    : filePath_(file.filePath_)
{
}

string File::getFileName() const
{
    return "";
}

string File::getExtension() const
{
    const size_t dotIndex = filePath_.rfind(".");
    if (dotIndex == string::npos) {
        return "";
    }
    string extension = filePath_.substr(dotIndex, filePath_.length() - dotIndex);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return extension;
}

bool File::exist() const
{
    return File::exist(filePath_);
}

bool File::exist(const string &filePath)
{
    if (filePath.empty()) {
        return false;
    }
    FILE* file = fopen(filePath.c_str(), "r");
    if (!file) {
        return false;
    }
    fclose(file);
    return true;
}

}
