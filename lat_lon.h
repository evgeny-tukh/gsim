#pragma once

#include <cstdint>

#include <Windows.h>
#include <CommCtrl.h>

#include "CaryWin/window.h"
#include "CaryWin/windef.h"
#include "CaryWin/winclass.h"

class LatLonEditor {
    public:
        LatLonEditor (HWND _parent, int _x, int _y, double *_val, bool _isLat);

        double getValue ();
        
    private:
        static inline const char *CLS_NAME = "LatLonEdit";
        static const uint32_t IDC_DEG = 100, IDC_MIN = 101, IDC_DEG_SPIN = 102, IDC_HS = 103;

        double *val;
        bool isLat;
        int x, y;
        HWND parent, wnd, degCtl, degSpin, minCtl, hsCtl;
};