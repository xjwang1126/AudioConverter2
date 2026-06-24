#ifndef MEDIAUTILDELEGATE_H
#define MEDIAUTILDELEGATE_H

#include <cstdint>

using namespace  std;

namespace MediaLibrary {

class MediaUtilDelegate
{
public:
    virtual ~MediaUtilDelegate() = default;

    virtual void notifyExportProgress(int64_t currentTime, int64_t totalTime) const = 0;

    virtual void notifyExportFinish() const = 0;
};

}

#endif // MEDIAUTILDELEGATE_H
