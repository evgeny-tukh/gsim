#pragma once

namespace AIS {

struct Target
{
    double lat, lon;
    double sog, cog, brg, range;
    int    id, imo, alt;
    char   callSign [6], name [20];
    bool   sar, mob;
};

}