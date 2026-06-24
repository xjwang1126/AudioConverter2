#include "decoder.h"

namespace MediaLibrary {

Decoder::Decoder(DecoderDelegate *delegate)
    : delegate_(delegate)
{
}

Decoder::~Decoder()
{
}

}
