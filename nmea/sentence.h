#pragma once

#include <vector>
#include <string>
#include <time.h>

namespace nmea { namespace builder {

class Sentence: public std::vector<std::string> {
    public:
        Sentence (bool _isSixBitEncoded, const char *_talkerID, const char *_type);
        void setAsUtc (size_t index, time_t utc = 0);
        void setAsInt (size_t index, int value);
        void setAsChar (size_t index, char value);
        void setAsAngle (size_t index, double value);
        void setAsCoord (size_t start, double value, bool lat);
        void setAsFloat (size_t index, double value);
        std::string compose ();
        
    private:
        std::string talkerID, type;
        bool isSixBitEncoded;
};

} }
