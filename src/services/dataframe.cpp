#include "dataframe.h"


ostream& operator << (ostream& out, const GeoData& geo) {
    auto &locations = geo.locations;
    auto &coordinates = geo.coordinates;
    cout << "GeoData(" << locations->size() << "):" << endl;
    unsigned int i = 0;
    for (auto iter = locations->begin(); iter != locations->end(); iter++, ++i) {
        auto &idx = iter->second;
        auto &coord = (*coordinates)[idx];
        cout << "  [" << i << "]  " << iter->first << ", ";
        cout << "Coord(" << coord->lat << ", " << coord->lon << ")" << endl;
    }
    cout << endl;
    return out;
}


ostream& operator << (ostream& out, const PopData& pop) {
    auto &data = pop.rows;
    auto &values = pop.vals;
    if (data->size() == 0)
        return out;

    cout << "PopData(" << data->size() << " * " << pop.cols->size() << "):" << endl;
    DemRow *row;
    unsigned int i = 0;
    for (auto iter = data->begin(); iter != data->end(); iter++, ++i) {
        cout << "  [" << i << "]  " << iter->first << ", ";
        row = iter->second;
        for (auto jter = row->begin(); jter != row->end(); jter++) {
            auto demval = (*values)[jter->second];
            cout << "(" << Xtm(jter->first).to_dtm(YMD_DASH) << ": " << demval[0] << "); ";
        }
        cout << endl;
    }
    cout << endl;
    return out;
}


ostream& operator << (ostream& out, const CasData& cas) {
    auto &data = cas.rows;
    auto &values = cas.vals;
    if (data->size() == 0)
        return out;

    cout << "CasData(" << data->size() << " * " << cas.cols->size() << "):" << endl;
    unsigned int i = 0;

    if (cas._typ == 'T') {
        auto &cols = cas.cols;
        if (cols->size() == 0)
            return out;

        bool use_addrx = abs(cols->cbegin()->first - (++cols->cbegin())->first) < 86400;
        
        DemCol *col;
        for (auto iter = cols->begin(); iter != cols->end(); iter++, ++i) {
            auto &utm = iter->first;
            cout << "  [" << i << "]  ";
            if (use_addrx)
                cout << utm << ", ";
            else
               cout << Xtm(utm).to_dtm(YMD_DASH) << ", ";
            col = iter->second;
            for (auto jter = col->begin(); jter != col->end(); jter++) {
                auto demval = (*values)[jter->second];
                cout << "(" << jter->first << ": " << demval[0] << "); ";
            }
            cout << endl;
        }
        cout << endl;
        return out;
    }

    DemRow *row;
    for (auto iter = data->begin(); iter != data->end(); iter++, ++i) {
        cout << "  [" << i << "]  " << iter->first << ", ";
        row = iter->second;
        for (auto jter = row->begin(); jter != row->end(); jter++) {
            auto demval = (*values)[jter->second];
            cout << "(" << jter->first << ": " << demval[0] << "); ";
            // cout << "(" << Xtm(jter->first).to_dtm(YMD_DASH) << ": " << demval[0] << "); ";
        }
        cout << endl;
    }
    cout << endl;
    return out;
}
