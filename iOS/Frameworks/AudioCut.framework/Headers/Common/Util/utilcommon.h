#ifndef UTILCOMMON_H
#define UTILCOMMON_H

#include "Format/msize.h"

#include <string>

using namespace std;

namespace MediaLibrary {

class ImageFrame;

ImageFrame* parsePicture(const string& filePath, const MSize& size);

}

#endif // UTILCOMMON_H
