#ifndef AUDIOGENERATORINFO_H
#define AUDIOGENERATORINFO_H

#include <functional>

namespace MediaLibrary {

    using ReadCallback = std::function<int(uint8_t* buffer, int bufferSize)>;

    using SeekCallback = std::function<int64_t(int64_t offset, int whence)>;

    using WriteCallback = std::function<int(uint8_t* buffer, int bufferSize)>;

}

#endif // AUDIOGENERATORINFO_H
