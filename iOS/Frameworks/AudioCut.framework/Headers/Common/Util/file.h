#ifndef FILE_H
#define FILE_H

#include <string>

using namespace std;

namespace MediaLibrary {

class File
{
public:
    File();
    File(const string& filePath);
    File(const File& file);
    File& operator=(const File& file) = delete;

    string getFileName() const;
    string getExtension() const;

    bool exist() const;
    static bool exist(const string& filePath);

private:
    const string filePath_;
};

}

#endif // FILE_H
