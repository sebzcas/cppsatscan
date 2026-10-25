#ifndef _DataFrame_H_
#define _DataFrame_H_

#include <unordered_map>
#include <vector>
#include <map>
#include <cmath>
#include <string>

#include "../../utils/base.h"
#include "../../utils/xtm.h"
#include "../../utils/str.h"


struct Coordinate {
    string loc;
    double lat;
    double lon;

    Coordinate(const string& loc, const double& lat, const double& lon) : loc(loc), lat(lat), lon(lon) {};
};

typedef unordered_map<string, unsigned int> Locations;
typedef vector<Coordinate*> Coordinates;

struct GeoData {
    GeoData() : locations(new Locations()), coordinates(new Coordinates()) {};
    ~GeoData() {
        for (auto iter = coordinates->begin(); iter != coordinates->end(); ++iter) {
            auto &coord = *iter;
            delete coord;
            coord = nullptr;
        }
        coordinates->clear();
        delete coordinates;
        coordinates = nullptr;

        locations->clear();
        delete locations;
        locations = nullptr;
    };

    Locations* locations;
    Coordinates* coordinates;
};

typedef map<time_t, unsigned int> DemRow;
typedef unordered_map<string, DemRow*> DemRows;

typedef unordered_map<string, unsigned int> DemCol;
typedef map<time_t, DemCol*> DemCols;

typedef vector<size_t*> DemVals;

void destroy(DemRows*& rows, DemCols*& cols, DemVals*& vals) {
    for (auto iter = vals->begin(); iter != vals->end(); ++iter) {
        auto &demval = *iter;
        delete demval;
        demval = nullptr;
    }
    vals->clear();
    delete vals;
    vals = nullptr;
        
    for (auto iter = cols->begin(); iter != cols->end(); iter++) {
        auto &col = iter->second;
        col->clear();
        delete col;
        col = nullptr;
    }
    cols->clear();
    delete cols;
    cols = nullptr;
        
    for (auto iter = rows->begin(); iter != rows->end(); iter++) {
        auto &row = iter->second;
        row->clear();
        delete row;
        row = nullptr;
    }
    rows->clear();
    delete rows;
    rows = nullptr;
};

struct PopData {
    PopData() : rows(new DemRows()), cols(new DemCols()), vals(new DemVals()) {};
    ~PopData() { destroy(rows, cols, vals); };

    DemRows* rows;
    DemCols* cols;
    DemVals* vals;
};

struct CasData {
    CasData() : rows(new DemRows()), cols(new DemCols()), vals(new DemVals()) {};
    CasData(DemRows* const& r, DemCols* const& c, DemVals* const& v) : rows(r), cols(c), vals(v) {};
    ~CasData() { destroy(rows, cols, vals); };

    DemRows* rows;
    DemCols* cols;
    DemVals* vals;
    
    CasData& as_type(const char& typ) { _typ = typ; return *this; };
    char _typ = 'A';
};


#include "dataframe.cpp"
#endif
