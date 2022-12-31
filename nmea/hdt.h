#pragma once

#include "sentence.h"

namespace nmea { namespace builder {

class Hdt: public Sentence {
    public:
        Hdt (double hdg): Sentence (false, "GY", "HDT") {
            resize (3);
            setAsAngle (1, hdg);
            setAsChar (2, 'T');
        }
};

} }