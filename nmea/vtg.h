#pragma once

#include "sentence.h"

namespace nmea { namespace builder {

class Vtg: public Sentence {
    public:
        Vtg (double cog, double sog, char modeIndicator = 'D'): Sentence (false, "GP", "VTG") {
            resize (10);
            setAsAngle (1, cog);
            setAsChar (2, 'T');
            setAsChar (4, 'M');
            setAsFloat (5, sog);
            setAsChar (6, 'N');
            setAsChar (8, 'K');
            setAsChar (9, modeIndicator);
        }
};

} }
