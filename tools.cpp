#include <Windows.h>
#include "tools.h"

namespace tools {

    strings& getSerialPortsList (strings& ports) {
        HKEY scomKey;
        int count = 0;
        DWORD error = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "Hardware\\DeviceMap\\SerialComm", 0, KEY_QUERY_VALUE, & scomKey);

        if (error == S_OK) {
            char valueName[100], valueValue[100];
            DWORD nameSize, valueSize, valueType;
            BOOL valueFound;

            do {
                nameSize = sizeof (valueName);
                valueSize = sizeof (valueValue);
                valueFound = RegEnumValue (scomKey, (DWORD) count ++, valueName, & nameSize, NULL, & valueType, (BYTE *) valueValue, & valueSize) == S_OK;

                if (valueFound) ports.push_back (valueValue);
            } while (valueFound);

            RegCloseKey (scomKey);
        }

        return ports;
    }

}