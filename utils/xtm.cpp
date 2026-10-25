#include "xtm.h"


using namespace std;


void Xtm::_assign(const std::tm* lgtm, Xtm* xtm) {
    if (lgtm == nullptr)
        return;
    xtm -> _yr  = lgtm -> tm_year + 1900;
    xtm -> _mon = lgtm -> tm_mon + 1;
    xtm -> _wk  = lgtm -> tm_yday / 7 + 1;
    xtm -> _wd  = lgtm -> tm_wday;
    xtm -> _day = lgtm -> tm_mday;
    xtm -> _hr  = lgtm -> tm_hour;
    xtm -> _min = lgtm -> tm_min;
    xtm -> _sec = lgtm -> tm_sec;
};

void Xtm::_assign(const Xtm* xtm, std::tm* lgtm) {
    lgtm -> tm_year = xtm -> _yr - 1900;
    lgtm -> tm_mon  = xtm -> _mon - 1;
    lgtm -> tm_yday = xtm -> _wk * 7 - 7;
    lgtm -> tm_wday = xtm -> _wd;
    lgtm -> tm_mday = xtm -> _day;
    lgtm -> tm_hour = xtm -> _hr;
    lgtm -> tm_min  = xtm -> _min;
    lgtm -> tm_sec  = xtm -> _sec;
}

string Xtm::_to_fmt(const string& dtm) {
    if (regex_match(dtm, regex(_REG_UTC)))
        return UTC;

    smatch reg_res;
    string ymd, hms;
    if (!regex_match(dtm, reg_res, regex("^([\\d]{4}[-/]?[\\d]{2}[-/]?[\\d]{2}).*$")))
        return "";

    ymd = reg_res[1];
    regex_match(ymd, reg_res, regex("^[\\d]{4}([-/]?)[\\d]{2}([-/]?)[\\d]{2}$"));
    string concat = reg_res[1];

    if (concat.compare(reg_res[2]) != 0)
        return "";
    
    ymd = "%Y" + concat + "%m" + concat + "%d";

    if (!regex_match(dtm, reg_res, regex("^.*([\\d]{2}[:]?[\\d]{2}[:]?[\\d]{2})$")))
        return "";
    
    hms = reg_res[1];
    if (hms.find(":") != string::npos) {
        hms = " %H:%M:%S";
    } else if (hms.size() == 8) {
        hms = "";
    } else {
        hms = "%H%M%S";
    }

    return ymd + hms;
}

DataPair<char, int>* Xtm::_from_rule(const string& s) {
    DataPair<char, int>* pair;

    smatch reg_res;
    if (!regex_match(s, reg_res, regex("%\\(([-+]?[\\d]+)\\)([jwYmdHMS])")))
        return nullptr;

    string amount = reg_res[1];
    string unit = reg_res[2];

    pair = new DataPair<char, int>(unit[0], atoi(amount.c_str()));
    return pair;
}

string Xtm::_to_rule(DataPair<char, int> data_pair) {
    return str("%(" + str(data_pair.get_second()) + ")").to_string() + string(1, data_pair.get_first());
}

Xtm::Xtm() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    Xtm::_assign(ltm, this);
}

Xtm::Xtm(const time_t& utm) {
    tm *ltm = localtime(&utm);
    Xtm::_assign(ltm, this);
}

Xtm::Xtm(const string& dtm, const string& fmt) {
    string fstr = (fmt.size() == 0) ? (Xtm::_to_fmt(dtm)) : fmt;
    struct tm tm_struct = {0};
    _strptime(dtm.c_str(), fstr.c_str(), &tm_struct);
    Xtm::_assign(&tm_struct, this);
}

Xtm::Xtm(const tm& tm_struct) {
    Xtm::_assign(&tm_struct, this);
}

Xtm Xtm::move(const string& s, const bool& inplace) {
    struct tm tm_struct = {0};
    _assign(this, &tm_struct);

    DataPair<char, int> *rule = Xtm::_from_rule(s);
    switch (rule -> get_first()) {
    case _TM_YDAY: tm_struct.tm_yday += rule -> get_second(); break;
    case _TM_WDAY: tm_struct.tm_wday += rule -> get_second(); break;
    case _TM_YEAR: tm_struct.tm_year += rule -> get_second(); break;
    case _TM_MON:  tm_struct.tm_mon  += rule -> get_second(); break;
    case _TM_MDAY: tm_struct.tm_mday += rule -> get_second(); break;
    case _TM_HOUR: tm_struct.tm_hour += rule -> get_second(); break;
    case _TM_MIN:  tm_struct.tm_min  += rule -> get_second(); break;
    case _TM_SEC:  tm_struct.tm_sec  += rule -> get_second(); break;
    default:
        break;
    }

    delete rule;
    rule = nullptr;

    time_t utm = mktime(&tm_struct);

    if (inplace) {
        tm *ltm = localtime(&utm);
        _assign(ltm, this);
        return *this;
    }

    return Xtm(utm);
}


Xtm Xtm::move(const char& u, const int& a, const bool& inplace) {
    struct tm tm_struct = {0};
    _assign(this, &tm_struct);

    string s = to_rule(u, a);

    DataPair<char, int> *rule = Xtm::_from_rule(s);
    switch (rule -> get_first()) {
    case _TM_YDAY: tm_struct.tm_yday += rule -> get_second(); break;
    case _TM_WDAY: tm_struct.tm_wday += rule -> get_second(); break;
    case _TM_YEAR: tm_struct.tm_year += rule -> get_second(); break;
    case _TM_MON:  tm_struct.tm_mon  += rule -> get_second(); break;
    case _TM_MDAY: tm_struct.tm_mday += rule -> get_second(); break;
    case _TM_HOUR: tm_struct.tm_hour += rule -> get_second(); break;
    case _TM_MIN:  tm_struct.tm_min  += rule -> get_second(); break;
    case _TM_SEC:  tm_struct.tm_sec  += rule -> get_second(); break;
    default:
        break;
    }

    delete rule;
    rule = nullptr;

    time_t utm = mktime(&tm_struct);

    if (inplace) {
        tm *ltm = localtime(&utm);
        _assign(ltm, this);
        return *this;
    }

    return Xtm(utm);
}

string Xtm::to_dtm(const string& fmt) {
    struct tm tm_struct = {0};
    Xtm::_assign(this, &tm_struct);
    char ptr[32];
    strftime(ptr, 32, fmt.c_str(), &tm_struct);
    return string(ptr);
}

time_t Xtm::to_utm() {
    struct tm tm_struct = {0};
    Xtm::_assign(this, &tm_struct);
    return mktime(&tm_struct);
}

Xtm Xtm::to_gmt(const bool& inplace) {
    time_t utm = this -> to_utm();
    tm *gtm = gmtime(&utm);
    
    if (inplace) {
        Xtm::_assign(gtm, this);
        return *this;
    }

    return Xtm(*gtm);
}


int Xtm::operator [] (const char& u) {
    switch (u) {
        case _TM_WDAY: return _wd;
        case _TM_YEAR: return _yr;
        case _TM_MON:  return _mon;
        case _TM_MDAY: return _day;
        case _TM_HOUR: return _hr;
        case _TM_MIN:  return _min;
        case _TM_SEC:  return _sec;
        default: return -1;
    }
}


ostream& operator << (ostream& out, const Xtm& xtm) {
    cout << "{\"Y\": " << xtm._yr << ", \"m\": " << xtm._mon << ", \"d\": " << xtm._day;
    cout << ", \"H\": " << xtm._hr << ", \"M\": " << xtm._min << ", \"S\": " << xtm._sec << "}";
    return out;
}

tm* _strptime(const char* __restrict__ _Buf, const char* __restrict__ _Format, tm* __restrict__ _Tm) {
    int yday = 1, wday = 0, year = 1900, mon = 1, mday = 1, hour = 0, min = 0, sec = 0;
    unsigned int buf_size = get_size(_Buf);
    unsigned int fmt_size = get_size(_Format);
    unsigned int i = 0, j = 0;
    char fc;
    string bs;

    #define _INT(s, default_val) (((s).size() > 0) ? atoi((s).c_str()) : default_val)

    while (i < buf_size && j < fmt_size - 1) {
        fc = _Format[j];

        if (fc == '%') {
            fc = _Format[++j];
            while ('0' <= _Buf[i] && _Buf[i] <= '9') {
                bs += _Buf[i++];
            }

            switch (fc) {
            case _TM_YDAY: yday = _INT(bs, 1); break;
            case _TM_WDAY: wday = _INT(bs, 0); break;
            case _TM_YEAR: year = _INT(bs, 1900); break;
            case _TM_MON:  mon  = _INT(bs, 1); break;
            case _TM_MDAY: mday = _INT(bs, 1); break;
            case _TM_HOUR: hour = _INT(bs, 0); break;
            case _TM_MIN:  min  = _INT(bs, 0); break;
            case _TM_SEC:  sec  = _INT(bs, 0); break;
            default: break;
            }
            bs.clear();
            ++i;
        } else {
            ++j;
        }
    }

    if (yday > 366 || wday > 6 || mon > 12 || mday > 31 || hour > 23 || min > 59 || sec > 59) {
        cout << "error: wrong time format in `dtm`: " << _Buf << endl;
        cout << "wday = " << wday << "\nyday = " << yday << "\nyear = " << year << "\nmon = " << mon;
        cout << "\nmday = " << mday << "\nhour = " << hour << "\nmin = " << min << "\nsec = " << sec;
        return nullptr;
    }

    _Tm -> tm_yday  = yday - 1;
    _Tm -> tm_wday  = wday;
    _Tm -> tm_year  = year - 1900;
    _Tm -> tm_mon   = mon - 1;
    _Tm -> tm_mday  = mday;
    _Tm -> tm_hour  = hour;
    _Tm -> tm_min   = min;
    _Tm -> tm_sec   = sec;
    _Tm -> tm_isdst = 0;
    return _Tm;
}
