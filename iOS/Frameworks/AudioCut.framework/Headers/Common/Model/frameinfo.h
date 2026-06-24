#ifndef FRAMEINFO_H
#define FRAMEINFO_H

#ifndef SECOND_TO_MILLISECOND_UNIT
#define SECOND_TO_MILLISECOND_UNIT 1000
#endif

#ifndef MILLISECOND_TO_MICROSECOND_UNIT
#define MILLISECOND_TO_MICROSECOND_UNIT 1000
#endif

#ifndef MICROSECOND_TO_NANOSECOND_UNIT
#define MICROSECOND_TO_NANOSECOND_UNIT 1000
#endif

#ifndef SECOND_TO_MICROSECOND_UNIT
#define SECOND_TO_MICROSECOND_UNIT SECOND_TO_MILLISECOND_UNIT * MILLISECOND_TO_MICROSECOND_UNIT
#endif

namespace MediaLibrary {

struct TimeBase {
    TimeBase() : num(1), den(1) {}
    TimeBase(int num, int den) : num(num), den(den) {}

    int num;
    int den;
};

}

#endif // FRAMEINFO_H
