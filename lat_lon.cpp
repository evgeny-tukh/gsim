#include <Windows.h>
#include <CommCtrl.h>

#include "CaryWin/window.h"
#include "CaryWin/windef.h"
#include "CaryWin/winclass.h"

#include "lat_lon.h"

LatLonEditor::LatLonEditor (HWND _parent, int _x, int _y, double *_val, bool _isLat): parent (_parent), x (_x), y (_y), val (_val), isLat (_isLat) {
    auto cls = Cary::WinClass::createCls (CLS_NAME);
    cls->registerCls ();
    Cary::WinDef wd (CLS_NAME, "", WS_CHILD | WS_VISIBLE);
    wd.width = 165;
    wd.height = 30;
    wd.x = _x;
    wd.y = _y;
    wnd = cls->createWnd (
        &wd,
        [this] (Cary::Window *wndInst) {
            wndInst->onCreate = [this, wndInst] (HWND wnd, UINT, WPARAM, LPARAM) -> LRESULT {
                SetWindowLongPtr (wnd, GWLP_USERDATA, (LONG_PTR) this);
                wndInst->addControl ("STATIC", 0, 2, 25, 20, SS_LEFT | WS_VISIBLE | WS_CHILD, IDC_STATIC, isLat ? "Lat" : "Lon");
                double absVal = *val >= 0 ? *val : - *val;
                int deg = (int) absVal;
                double min = (absVal - (double) deg) * 60.0;
                char degStr [10],minStr [20];
                sprintf (minStr, "%05.3f", min);
                sprintf (degStr, "%02d", deg);
                degCtl = wndInst->addControl ("EDIT", 30, 0, 45, 22, ES_NUMBER | WS_VISIBLE, IDC_DEG);
                degSpin = wndInst->addControl ("msctls_updown32", 50, 0, 10, 22, UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | WS_VISIBLE, IDC_DEG_SPIN);
                minCtl = wndInst->addControl ("EDIT", 75, 0, 50, 22, WS_VISIBLE, IDC_MIN, minStr);
                hsCtl = wndInst->addControl ("COMBOBOX", 125, 0, 35, 100, CBS_DROPDOWNLIST | WS_VISIBLE, IDC_HS);
                SendMessage (degSpin, UDM_SETBUDDY, (WPARAM) degCtl, 0);
                SendMessage (degSpin, UDM_SETRANGE32, 0, 59);
                SendMessage (degSpin, UDM_SETPOS32, 0, deg);
                SetWindowText (minCtl, minStr);
                if (isLat) {
                    SendMessage (hsCtl, CB_ADDSTRING, 0, (LPARAM) "N");
                    SendMessage (hsCtl, CB_ADDSTRING, 0, (LPARAM) "S");
                } else {
                    SendMessage (hsCtl, CB_ADDSTRING, 0, (LPARAM) "E");
                    SendMessage (hsCtl, CB_ADDSTRING, 0, (LPARAM) "W");
                }
                SendMessage (hsCtl, CB_SETCURSEL, *val >= 0 ? 0 : 1, 0);
                return 0;
            };
        },
        parent
    );
}

double LatLonEditor::getValue () {
    char deg[20], min[20];
    GetWindowText (degCtl, deg, sizeof (deg));
    GetWindowText (minCtl, min, sizeof (min));
    double val = atof (deg) + atof (min) / 60.0;
    return SendMessage (hsCtl, CB_GETCURSEL, 0, 0) == 0 ? val : -val;
}