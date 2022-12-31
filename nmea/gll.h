#pragma once

#include "sentence.h"

namespace nmea { namespace builder {

class Gll: public Sentence {
    public:
        Gll (double lat, double lon, char modeIndicator = 'D'): Sentence (false, "GP", "GLL") {
            resize (8);
            setAsCoord (1, lat, true);
            setAsCoord (3, lon, false);
            setAsUtc (5);
            setAsChar (6, 'A');
            setAsChar (7, modeIndicator);
        }
};

} }
