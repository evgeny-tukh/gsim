#pragma once

#include <vector>
#include <string>

namespace tools {
    typedef std::vector<std::string> strings;

    strings& getSerialPortsList (strings& ports);
}
