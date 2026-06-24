#include "system.h"

#include <thread>
#include <cassert>
#include <string>
#include <sstream>

using namespace std;

namespace MediaLibrary {

std::string getDumpDataPath()
{
    return "";
}

bool fileExists(const std::string& filePath)
{
    (void)filePath;
    return false;
}

bool deleteFile(const std::string& filePath)
{
    (void)filePath;
    return false;
}

}
