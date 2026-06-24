#include "encoder.h"

namespace MediaLibrary {

Encoder::Encoder(EncoderDelegate *delegate)
    : delegate_(delegate)
{
}

Encoder::~Encoder()
{
}

}
