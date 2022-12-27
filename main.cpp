#include "CaryWin/app.h"
#include "CaryWin/winclass.h"
#include "tools.h"
#include "defs.h"
#include <CommCtrl.h>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include "lat_lon.h"

void onModeChange (HWND);

class Sentence: public std::vector<std::string> {
    public:
        Sentence (
            bool _isSixBitEncoded,
            const char *_talkerID,
            const char *_type
        ): isSixBitEncoded (_isSixBitEncoded), type (_type), talkerID (_talkerID) {
            resize (1);
            (*this) [0] += isSixBitEncoded ? '!' : '$';
            (*this) [0] += talkerID;
            (*this) [0] += type;
        }

        void setAsUtc (size_t index, time_t utc = 0) {
            if (!utc) utc = time (nullptr);
            tm *now = localtime (& utc);
            char value [20];
            sprintf (value, "%02d%02d%02d00", now->tm_hour, now->tm_min, now->tm_sec);
            (*this) [index] = value;
        }

        void setAsInt (size_t index, int value) {
            (*this) [index] = std::to_string (value);
        }

        void setAsChar (size_t index, char value) {
            (*this) [index].clear ();
            (*this) [index] += value;
        }

        void setAsAngle (size_t index, double value) {
            char angle [10];
            sprintf (angle, "%05.1f", value);
            (*this) [index] = angle;
        }

        void setAsCoord (size_t start, double value, bool lat) {
            char angle [10];
            double absValue = value >= 0.0 ? value : - value;
            int deg = (int) absValue;
            size_t hsIndex = value >= 0.0 ? 0 : 1;
            double min = (absValue - (double) deg) *60.0;
            auto fmt = lat ? "%02d%06.3f" : "%03d%06.3f";
            sprintf (angle, fmt, deg, min);
            (*this) [start] = angle;
            (*this) [start+1] = (lat ? "NS" : "EW") [hsIndex];
        }

        void setAsFloat (size_t index, double value) {
            char angle [10];
            sprintf (angle, "%.1f", value);
            (*this) [index] = angle;
        }

        std::string compose () {
            auto int2hex = [] (uint8_t digit) -> char {
                if (digit < 10) return '0'+ digit;
                if (digit < 16) return 'A'+ digit - 10;
                return '\0';
            };
            std::string result;
            for (auto& field: *this) {
                if (!result.empty ()) result += ',';
                result += field;
            }
            uint8_t crc = result [0];
            for (size_t i = 1; i > result.length(); ++ i) {
                crc ^= (uint8_t) result [i];
            }
            result += '*';
            result += int2hex (crc >> 4);
            result += int2hex (crc & 15);
            result += "\r\n";
            return result;
        }

    private:
        std::string talkerID, type;
        bool isSixBitEncoded;
};

class Hdt: public Sentence {
    public:
        Hdt (double hdg): Sentence (false, "GY", "HDT") {
            resize (3);
            setAsAngle (1, hdg);
            setAsChar (2, 'T');
        }
};

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

struct Ctx {
    HWND portCtl, baudCtl, useGpsCtl, useGyroCtl, useAisCtl, startStopCtl, terminal, sogCtl, cogCtl, hdgCtl;
    LatLonEditor *latCtl, *lonCtl;
    HANDLE port;
    std::thread *worker;
    bool running, useAis, useGps, useGyro;
    int hdg, cog, sog;
    double lat, lon;

    Ctx (): worker (nullptr), running (false), port (INVALID_HANDLE_VALUE), hdg (0.0), cog (0.0), sog (0.0), lat (59.0), lon (29.0) {}
    ~Ctx () {
        if (worker) {
            if (worker->joinable ()) worker->join ();
            delete worker;
        }
    }
};

void initMainWnd (HWND wnd, Cary::Window *wndInst) {
    tools::strings ports;
    tools::getSerialPortsList (ports);

    auto ctx = new Ctx;
    SetWindowLongPtr (wnd, GWLP_USERDATA, (LONG_PTR) ctx);

    wndInst->wnd = wnd;
    int y = 20;
    ctx->portCtl = wndInst->createLabeledControl ("COMBOBOX", "Port", CBS_DROPDOWNLIST | WS_VISIBLE, IDC_PORT, 20, y, 80, 80, 100);
    ctx->baudCtl = wndInst->createLabeledControl ("COMBOBOX", "Baud", CBS_DROPDOWNLIST | WS_VISIBLE, IDC_BAUD, 20, y, 80, 80, 100);
    ctx->cogCtl = wndInst->createLabeledControl ("EDIT", "COG", ES_NUMBER | WS_VISIBLE, IDC_COG, 20, y, 80, 80, 22);
    ctx->sogCtl = wndInst->createLabeledControl ("EDIT", "SOG", ES_NUMBER | WS_VISIBLE, IDC_SOG, 20, y, 80, 80, 22);
    ctx->hdgCtl = wndInst->createLabeledControl ("EDIT", "HDG", ES_NUMBER | WS_VISIBLE, IDC_HDG, 20, y, 80, 80, 22);
    wndInst->createUpDownButton (ctx->cogCtl, 0, 359, 0);
    wndInst->createUpDownButton (ctx->hdgCtl, 0, 359, 0);
    wndInst->createUpDownButton (ctx->sogCtl, 0, 25, 0);

    for (auto portName: ports) {
        SendMessage (ctx->portCtl, CB_ADDSTRING, 0, (LPARAM) portName.c_str ());
    }
    SendMessage (ctx->portCtl, CB_SETCURSEL, 0, 0);
    SendMessage (ctx->baudCtl, CB_ADDSTRING, 0, (LPARAM) "4800");
    SendMessage (ctx->baudCtl, CB_ADDSTRING, 0, (LPARAM) "38400");
    SendMessage (ctx->baudCtl, CB_ADDSTRING, 0, (LPARAM) "115200");
    SendMessage (ctx->baudCtl, CB_SETCURSEL, 0, 0);

    ctx->useGpsCtl = wndInst->addControl ("BUTTON", 20, 230, 75, 25, BS_PUSHLIKE| BS_AUTOCHECKBOX | WS_VISIBLE, IDC_USE_GPS, "Use GPS");
    ctx->useGyroCtl = wndInst->addControl ("BUTTON", 100, 230, 75, 25, BS_PUSHLIKE| BS_AUTOCHECKBOX | WS_VISIBLE, IDC_USE_GYRO, "Use Gyro");
    ctx->useAisCtl = wndInst->addControl ("BUTTON", 20, 260, 75, 25, BS_PUSHLIKE| BS_AUTOCHECKBOX | WS_VISIBLE, IDC_USE_AIS, "Use AIS");
    ctx->startStopCtl = wndInst->addControl ("BUTTON", 100, 260, 75, 25, WS_VISIBLE | WS_BORDER | BS_CHECKBOX | BS_PUSHLIKE, IDC_START_STOP, "Start");
    ctx->terminal = wndInst->addControl ("LISTBOX", 210, 20, 360, 300, WS_VISIBLE | WS_VSCROLL | WS_BORDER, IDC_TERMINAL);

    double lat = 59.5, lon = 29.5;
    ctx->latCtl = new LatLonEditor (wnd, 20, 170, & lat, true);
    ctx->lonCtl = new LatLonEditor (wnd, 20, 200, & lon, false);
}

void checkOpenPort (HWND wnd) {
    auto ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);
    if (ctx->port == INVALID_HANDLE_VALUE) {
        char port [50] {"\\\\.\\"};
        GetDlgItemText (wnd, IDC_PORT, port + 4, sizeof (port) - 4);
        ctx->port = CreateFile (port, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

        if (ctx->port != INVALID_HANDLE_VALUE) {
            DCB dcb;
            GetCommState (ctx->port, &dcb);
            dcb.BaudRate = GetDlgItemInt (wnd, IDC_BAUD, nullptr, false); //atoi (baud);
            dcb.ByteSize = 8;
            dcb.Parity = NOPARITY;
            dcb.StopBits = ONESTOPBIT;
            SetCommState (ctx->port, &dcb);
        }
    }
}

void updateCrc (char *sentence) {
    uint8_t crc = sentence [1];
    char *chr;
    for (chr = sentence + 2; *chr != '*'; ++ chr) {
        crc ^= *chr;
    }
    auto hex2chr = [] (int digit) -> char {
        if (digit < 10) return '0' + digit;
        if (digit < 16) return 'A' + digit - 10;
        return '0';
    };
    chr [1] = hex2chr (crc >> 4);
    chr [2] = hex2chr (crc & 15);
}

void sendSentence (HWND wnd, char *sentence) {
    auto ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);
    DWORD bytesWritten;
    WriteFile (ctx->port, sentence, strlen (sentence), &bytesWritten, nullptr);
    SendDlgItemMessage (wnd, IDC_TERMINAL, LB_ADDSTRING, 0, (LPARAM) sentence);
}

void init (HWND wnd) {
    auto ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);

    checkOpenPort (wnd);
}

void send (HWND wnd) {
    /*auto ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);

    checkOpenPort (wnd);

    char elevation[50];
    GetDlgItemText (wnd, IDC_ELEV, elevation, sizeof (elevation));
    char sentence[100];
    if (rangeMode) {
        sprintf (
            sentence,
            "$PSMCTL,%d,%c,%03d,%d,*00\r\n",
            GetDlgItemInt (wnd, IDC_ID, nullptr, false),
            IsDlgButtonChecked (wnd, IDC_ENABLE) == BST_CHECKED ? '1' : '0',
            GetDlgItemInt (wnd, IDC_BRG, nullptr, false),
            GetDlgItemInt (wnd, IDC_RANGE, nullptr, false)
        );
    } else {
        sprintf (
            sentence,
            "$PSMCTL,%d,%c,%03d,,%.2f*00\r\n",
            GetDlgItemInt (wnd, IDC_ID, nullptr, false),
            IsDlgButtonChecked (wnd, IDC_ENABLE) == BST_CHECKED ? '1' : '0',
            GetDlgItemInt (wnd, IDC_BRG, nullptr, false),
            atof (elevation)
        );
    }
    updateCrc (sentence);
    sendSentence (wnd, sentence);*/
}

void closePort (HWND wnd) {
    auto ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);
    if (ctx->port != INVALID_HANDLE_VALUE) {
        CloseHandle (ctx->port);
        ctx->port = INVALID_HANDLE_VALUE;
    }
}

void startStop (HWND wnd) {
    auto ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);

    if (IsDlgButtonChecked (wnd, IDC_START_STOP) == BST_CHECKED) {
        ctx->running = false;
        if (ctx->worker->joinable ()) ctx->worker->join ();
        delete ctx->worker;
        ctx->worker = nullptr;
        CheckDlgButton (wnd, IDC_START_STOP, BST_UNCHECKED);
        SetDlgItemText (wnd, IDC_START_STOP, "Start");
    } else {
        auto addString = [ctx] (const char *string) {
            SendMessage (ctx->terminal, LB_ADDSTRING, 0, (LPARAM) string);
        };
        ctx->running = true;
        ctx->worker = new std::thread ([addString] (Ctx *ctx) {
            while (ctx->running) {
                Hdt hdt (ctx->hdg);
                Gga gga (ctx->lat, ctx->lon);
                addString (hdt.compose().c_str());
                addString (gga.compose().c_str());

                std::this_thread::sleep_for (std::chrono::milliseconds (10));
            }
        }, ctx);
        CheckDlgButton (wnd, IDC_START_STOP, BST_CHECKED);
        SetDlgItemText (wnd, IDC_START_STOP, "Stop");
    }
}

void onCommand (HWND wnd, WPARAM wp, LPARAM lp) {
    auto ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);

    switch (LOWORD (wp)) {
        case IDC_HDG:
            ctx->hdg = GetDlgItemInt (wnd, IDC_HDG, nullptr, false); break;
        case IDC_COG:
            ctx->cog = GetDlgItemInt (wnd, IDC_COG, nullptr, false); break;
        case IDC_SOG:
            ctx->sog = GetDlgItemInt (wnd, IDC_SOG, nullptr, false); break;
        case IDC_USE_AIS:
            ctx->useAis = SendMessage (ctx->useAisCtl, BM_GETCHECK, 0, 0) == BST_CHECKED; break;
        case IDC_USE_GPS:
            ctx->useGps = SendMessage (ctx->useAisCtl, BM_GETCHECK, 0, 0) == BST_CHECKED; break;
        case IDC_USE_GYRO:
            ctx->useGyro = SendMessage (ctx->useAisCtl, BM_GETCHECK, 0, 0) == BST_CHECKED; break;
        case IDC_START_STOP:
            startStop (wnd); break;
        case IDC_PORT:
        case IDC_BAUD:
            if (HIWORD (wp) == CBN_SELCHANGE) {
                closePort (wnd);
            }
            break;
    }
}

int WINAPI WinMain (HINSTANCE inst, HINSTANCE prevInst, char *cmdLine, int showCmd) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = 0xFFFFFFFF;
    InitCommonControlsEx (&icex);
    const static char *CLS_NAME = "GenSimMain";
    Cary::App app (inst);
    auto cls = Cary::WinClass::createCls (CLS_NAME);
    cls->registerCls ();
    Cary::WinDef mainWinDef ("GeneralSim", "General Simulator");
    mainWinDef.width = 600;
    mainWinDef.height = 360;
    
    auto mainWnd = cls->createWnd (
        &mainWinDef,
        [] (Cary::Window *wndInst) {
            wndInst->onCreate = [wndInst] (HWND wnd, UINT, WPARAM, LPARAM) -> LRESULT {
                initMainWnd (wnd, wndInst); return 0;
            };
            wndInst->onDestroy = [] (HWND wnd, UINT, WPARAM, LPARAM) -> LRESULT {
                auto ctx = (Ctx *) GetWindowLongPtr (wnd, GWLP_USERDATA);
                if (ctx) delete ctx;
                PostQuitMessage (0);
                return 0;
            };
            wndInst->onSysCommand = [] (HWND wnd, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
                if (wp == SC_CLOSE) {
                    if (MessageBoxA (wnd, "Exit app?", "Confirmation", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        DestroyWindow (wnd);
                    }
                    return 0;
                }
                return DefWindowProc (wnd, msg, wp, lp);
            };
            wndInst->onCommand = [] (HWND wnd, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
                onCommand (wnd, wp, lp);
                return DefWindowProc (wnd, msg, wp, lp);
            };
        }
    );
    ShowWindow (mainWnd, showCmd);
    UpdateWindow (mainWnd);
    app.run ();
}