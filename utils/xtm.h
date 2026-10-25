// Xtm provides the subset of date/time operations.
// It supports Unix timestamp conversion, temporal-step parsing, and alignment operations.

#ifndef _Xtm_H_
#define _Xtm_H_

#include <string.h>
#include <vector>
#include <ctime>
#include <regex>

#include "base.h"
#include "str.h"


class Xtm {
public:
#define YMD_MHS "%Y-%m-%d %H:%M:%S"
#define YMD_MHS_CONCAT "%Y%m%d%H%M%S"
#define YMD_SLASH "%Y/%m/%d"
#define YMD_DASH "%Y-%m-%d"
#define YMD_CONCAT "%Y%m%d"
#define UTC "%Y-%m-%dT%H:%M:%SZ"

    Xtm();
    Xtm(const std::time_t&);
    Xtm(const string&, const string& fmt = "");
    Xtm(const std::tm&);
    ~Xtm() {};

    Xtm move(const string& s, const bool& inplace = false);
    Xtm move(const char& u, const int& a, const bool& inplace = false);
    string to_dtm(const string& fmt = YMD_MHS);
    std::time_t to_utm();
    Xtm to_gmt(const bool& inplace = false);
    
    static DataPair<char, int>* from_rule(const string& s) { return _from_rule(s); };
    static string to_rule(DataPair<char, int>* d) { return _to_rule(*d); };
    static string to_rule(const char& u, const int& a) { return _to_rule(DataPair<char, int>(u, a)); };

    static string to_fmt(const string& s) { return _to_fmt(s); };

    inline static string yesterday() { return Xtm().move("%(-1)d").to_dtm(YMD_DASH); };
    inline static string today() { return Xtm().to_dtm(YMD_DASH); };
    inline static string tomorrow() { return Xtm().move("%(+1)d").to_dtm(YMD_DASH); };

    friend std::ostream& operator << (std::ostream&, const Xtm&);
	friend Xtm operator + (const Xtm&, const Xtm&);
	friend Xtm operator + (const Xtm&, const string&);
	friend Xtm operator + (const string&, const Xtm&);
	friend bool operator == (const Xtm&, const Xtm&);
	friend bool operator == (const Xtm&, const string&);
	friend bool operator == (const string&, const Xtm&);
	friend bool operator > (const Xtm&, const Xtm&);
	friend bool operator < (const Xtm&, const Xtm&);
	friend bool operator >= (const Xtm&, const Xtm&);
	friend bool operator <= (const Xtm&, const Xtm&);
	friend bool operator != (const Xtm&, const Xtm&);
	friend Xtm operator += (const Xtm&, const string&);
    int operator [] (const char&);

private:
#define _REG_YMD_MHS "^[\\d]{4}-[\\d]{2}-[\\d]{2} [\\d]{2}:[\\d]{2}:[\\d]{2}$"
#define _REG_YMD_MHS_CONCAT "^[\\d]{4}[\\d]{2}[\\d]{2}[\\d]{2}[\\d]{2}[\\d]{2}$"
#define _REG_YMD_SLASH "^[\\d]{4}/[\\d]{2}/[\\d]{2}$"
#define _REG_YMD_DASH "^[\\d]{4}-[\\d]{2}-[\\d]{2}$"
#define _REG_YMD_CONCAT "^[\\d]{4}[\\d]{2}[\\d]{2}$"
#define _REG_UTC "^[\\d]{4}-[\\d]{2}-[\\d]{2}T[\\d]{2}:[\\d]{2}:[\\d]{2}Z$"

    int _yr, _mon, _wk, _wd, _day, _hr, _min, _sec;

    static string _to_fmt(const string&);
    static DataPair<char, int>* _from_rule(const string&);
    static string _to_rule(DataPair<char, int>);
    static void _assign(const std::tm*, Xtm*);
    static void _assign(const Xtm*, std::tm*);
};

#define _TM_SEC  'S'
#define _TM_MIN  'M'
#define _TM_HOUR 'H'
#define _TM_MDAY 'd'
#define _TM_MON  'm'
#define _TM_YEAR 'Y'
#define _TM_WDAY 'w'
#define _TM_YDAY 'j'

std::tm* _strptime(const char*, const char*, std::tm*);


#include "xtm.cpp"
#endif
