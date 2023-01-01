#pragma once

namespace AIS {

enum DataFlags
{
    NO_HDG = 511,
    NO_COG = 3600,
    NO_SOG = 1023,
    HIGH_SOG = 1022,
    NO_LAT = 0x3412140,
    NO_LON = 0x6791AC0,
};

struct Target
{
    double lat, lon;
    double sog, cog, brg, range, hdg;
    int    id, imo, alt;
    char   callSign [6], name [20];
    bool   sar, mob;
};

}