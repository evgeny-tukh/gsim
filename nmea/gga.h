#pragma once

#include "sentence.h"

namespace nmea { namespace builder {

class Gga: public Sentence {
    public:
        Gga (double lat, double lon, char qualityIndicator = '2'): Sentence (false, "GP", "GGA") {
            resize (15);
            setAsUtc (1);
            setAsCoord (2, lat, true);
            setAsCoord (4, lon, false);
            setAsChar (6, qualityIndicator);
            setAsChar (10, 'M');
            setAsChar (12, 'M');
        }
};

} }
