#include "calculation.h"


void aggregate_population(size_t*& demval, DemVals* const& vals) {
    size_t length = get_size(demval);
    for (auto iter = vals->cbegin(); iter != vals->cend(); ++iter) {
        for (auto i = 0; i < length; ++i)
            demval[i] += (*iter)[i];
    }
}

size_t aggregate_population(DemVals* const& vals) {
    size_t demval = 0;
    for (auto iter = vals->cbegin(); iter != vals->cend(); ++iter)
        demval += (*iter)[0];
    return demval;
}

size_t aggregate_population(DemVals* const& vals, DemCols* const& cols, const time_t& utm) {
    auto &col = cols->find(utm)->second;

    size_t demval = 0;
    for (auto iter = col->cbegin(); iter != col->cend(); ++iter) {
        demval += vals->at(iter->second)[0];
    }

    return demval;
}


time_t encode_t(
    const time_t& utm, const char*& st_dtm, const char*& nd_dtm, const char*& temporal_step) {
    
    const time_t st_utm = Xtm(st_dtm, YMD_DASH).to_utm();
    const time_t nd_utm = Xtm(nd_dtm, YMD_DASH).to_utm();
    auto *data_pair = Xtm::from_rule(temporal_step);
    auto step_u = data_pair->get_first();
    auto step_a = abs(data_pair->get_second());
    auto &min_utm = st_utm, &max_utm = nd_utm;
    time_t (*encode)(const time_t&);

    if (step_u == 'd') {
        encode = [](const time_t& _x)->time_t { return _x / 86400; };
    } else if (step_u == 'm') {
        encode = [](const time_t& _x)->time_t { Xtm _xtm(_x); return _xtm['Y'] * 12 + _xtm['m']; };
    } else if (step_u == 'Y') {
        encode = [](const time_t& _x)->time_t { Xtm _xtm(_x); return _xtm['Y']; };
    } else {
        const char *err = "Error: unit of temporal step must be on of 'Y', 'm', or 'd'";
        cout << err << endl;
        system("pause");
        throw err;
    }

    return encode(utm);
}

time_t decode_t(
    const time_t& addrx, const char*& st_dtm, const char*& nd_dtm, const char*& temporal_step) {
    
    const time_t st_utm = Xtm(st_dtm, YMD_DASH).to_utm();
    const time_t nd_utm = Xtm(nd_dtm, YMD_DASH).to_utm();
    auto *data_pair = Xtm::from_rule(temporal_step);
    auto step_u = data_pair->get_first();
    auto step_a = abs(data_pair->get_second());
    auto &min_utm = st_utm, &max_utm = nd_utm;
    time_t (*encode)(const time_t&);

    if (step_u == 'd') {
        encode = [](const time_t& _x)->time_t { return _x / 86400; };
    } else if (step_u == 'm') {
        encode = [](const time_t& _x)->time_t { Xtm _xtm(_x); return _xtm['Y'] * 12 + _xtm['m']; };
    } else if (step_u == 'Y') {
        encode = [](const time_t& _x)->time_t { Xtm _xtm(_x); return _xtm['Y']; };
    } else {
        const char *err = "Error: unit of temporal step must be on of 'Y', 'm', or 'd'";
        cout << err << endl;
        system("pause");
        throw err;
    }

    time_t spx = (encode(max_utm) - encode(min_utm)) / step_a;
    return Xtm(max_utm).move(step_u, (spx - addrx) * -step_a).to_utm();
}

time_t calculate_time_step_index(
    const time_t& utm, const char*& st_dtm, const char*& nd_dtm, const char*& temporal_step) {
    auto* datapair = Xtm::from_rule(temporal_step);
    const char unit = datapair->get_first();
    const time_t step = std::abs(datapair->get_second());
    delete datapair;

    if (step == 0)
        throw std::invalid_argument("Temporal step cannot be zero");

    auto encode = [unit](const time_t& t) -> time_t {
        Xtm xtm(t);

        switch (unit) {
        case 'd':
            return t / 86400;
        case 'm':
            return xtm['Y'] * 12 + xtm['m'];
        case 'Y':
            return xtm['Y'];
        default:
            throw std::invalid_argument("Unsupported temporal step unit");
        }
    };

    const time_t start = Xtm(st_dtm, YMD_DASH).to_utm();

    return (encode(utm) - encode(start)) / step;
}

void compress_temporal_cases(
    CasData*& cas, const char*& st_dtm, const char*& nd_dtm, const char*& temporal_step) {
    
    const time_t st_utm = Xtm(st_dtm, YMD_DASH).to_utm();
    const time_t nd_utm = Xtm(nd_dtm, YMD_DASH).to_utm();
    auto *data_pair = Xtm::from_rule(temporal_step);
    auto step_u = data_pair->get_first();
    auto step_a = abs(data_pair->get_second());

    // predefine a function pointer that could be any one of encoders for step 'Y', 'm', or 'd'
    time_t (*encode)(const time_t&);
    // and predefine another pointer as decoding function that is able to recover codecs back
    time_t (*decode)(const time_t&);
    
    if (step_u == 'd') {
        // 2016-12-29 2016-12-30 2016-12-31 2017-01-01 2017-01-02 ...
        // eg. (utm(2016-12-29) - utm(2016-12-29)) / 86400 = 0 -> 1
        //     (utm(2016-12-30) - utm(2016-12-29)) / 86400 = 1 -> 2
        //     (utm(2016-12-31) - utm(2016-12-29)) / 86400 = 2 -> 3
        //     (utm(2017-01-01) - utm(2016-12-29)) / 86400 = 3 -> 4
        //     (utm(2017-01-02) - utm(2016-12-29)) / 86400 = 4 -> 5
        //     ...
        //  => (utm / 86400) - (min(utm) / 86400) + 1
        //  OR (max(utm) / 86400) - (utm / 86400) + 1
        encode = [](const time_t& _x)->time_t { return _x / 86400; };
    } else if (step_u == 'm') {
        // 2016-11 2016-12 2017-01 2017-02 2017-03
        // eg. 2016 * 12 + 11                   (1 * 12 + 11) = 23 -> 1
        //     2016 * 12 + 12 = 2017 * 12 + 0 = (1 * 12 + 12) = 24 -> 2
        //     2017 * 12 + 01                   (2 * 12 + 01) = 25 -> 3
        //     2017 * 12 + 02                   (2 * 12 + 02) = 26 -> 4
        //     ...
        //  => (yr * 12 + mon) - min(yr * 12 + mon) + 1
        //  OR max(yr * 12 + mon) - (yr * 12 + mon) + 1
        encode = [](const time_t& _x)->time_t { Xtm _xtm(_x); return _xtm['Y'] * 12 + _xtm['m']; };
    } else if (step_u == 'Y') {
        // 2016 2017 2018
        // eg. 2016 -> 1
        //     2017 -> 2
        //     2018 -> 3
        //     ...
        //  => yr - min(yr) + 1
        //  OR max(yr) - yr + 1
        encode = [](const time_t& _x)->time_t { Xtm _xtm(_x); return _xtm['Y']; };
    } else {
        const char *err = "Error: unit of temporal step must be on of 'Y', 'm', or 'd'";
        cout << err << endl;
        system("pause");
        throw err;
    }

    auto &min_utm = st_utm, &max_utm = nd_utm;
    time_t min_x = encode(min_utm), max_x = encode(max_utm);

    // after defining `fn` and `min_x` for different steps ('Y', 'm', or 'd')
    // all regularized timestamps become the result of expression:
    //     `cur_x` = `fn(utm)` - `min_x` + 1
    //  or `cur_x` = `max_x` - `fn(utm)` + 1 (here we use this equation)
    // and steps for different units become constant value 1 with consistent unit with the original unit,
    // which means 1 year under 'Y', 1 month under 'm', and 1 day under 'd';
    // and then the compression process can be simplified to a "hashtable" stuff like:
    //     `addrx` = (int) (`cur_x` / `step_a`)
    // to map all timestamps into several bags, of which each bag is a temporal unit for further scanning
    
    // cout << "\n=== Before temporal compression ===\n";
    // cout << "cas->cols size = " << cas->cols->size() << '\n';
    // for (auto iter = cas->cols->cbegin(); iter != cas->cols->cend(); ++iter) {
    //     const auto& utm = iter->first;
    //     const auto& col = iter->second;
        
    //     size_t total_cases = 0;
    //     for (auto jter = col->cbegin(); jter != col->cend(); ++jter) {
    //         total_cases += cas->vals->at(jter->second)[0];
    //     }

    //     cout << "time = " << Xtm(utm).to_dtm(YMD_DASH) << ", key = " << utm << ", cases = " << total_cases << '\n';
    // }

    auto &rows = cas->rows;
    auto &cols = cas->cols;
    auto &vals = cas->vals;
    time_t cur_x, addrx, spx = (max_x - min_x) / step_a;
    // for (auto iter = cols->begin(); iter != cols->end(); ++iter) {
    //     auto &utm = iter->first;
    // // for (auto i = 0; i < 366; ++i) {
    // //     time_t utm = min_utm + 86400 * i;
    //     cur_x = max_x - encode(utm);
    //     addrx = spx - cur_x / step_a;
    //     cout << Xtm(utm).to_dtm(YMD_DASH) << " -> ";
    //     cout << Xtm(decoding(addrx, st_dtm, nd_dtm, temporal_step)).to_dtm(YMD_DASH) << endl;
    //     // cout << Xtm(max_utm).move(step_u, -(spx - addrx) * step_a).to_dtm(YMD_DASH) << " " << addrx << endl;
    // }
    // return;
    
    auto *new_rows = new DemRows();
    auto *new_cols = new DemCols();
    auto *new_vals = new DemVals();
    auto new_cnts = vector<unsigned int>();
    DemRow *new_row;
    DemCol *new_col;
    size_t *new_val;
    time_t *addrval;
    DemCols::iterator cols_iter;
    AddrTable::iterator addr_iter;

    auto iter = rows->cbegin();
    // in loop i, each iteration is a location
    while (iter != rows->cend()) {
        auto &loc = iter->first;
        auto &row = iter->second;
        auto jter = row->cbegin();
        new_row = new DemRow();
        // in loop j, each iteratoin is a timestamp
        while (jter != row->cend()) {
            auto &curutm = jter->first;
            
            addrx = spx - (max_x - encode(curutm)) / step_a;
            size_t *cur_val = vals->at(row->find(curutm)->second);

            // addrval = (addr_iter = addr_table->find(addrx)) == addr_table->end() ?
            //           new time_t[2]{curutm, curutm} : addr_iter->second;
            // addrval[0] = (curutm < addrval[0]) ? curutm : addrval[0];
            // addrval[1] = (curutm > addrval[1]) ? curutm : addrval[1];
            // addr_table->insert(pair<time_t, time_t*>(addrx, addrval));
            
            auto kter = new_row->find(addrx);
            if (kter == new_row->end()) {
                new_vals->push_back(new size_t[2]{cur_val[0], cur_val[1]});
                new_cnts.push_back(1);
                new_row->insert(pair<time_t, unsigned int>(addrx, new_vals->size() - 1));
                
                // add new col {loc: idx, ...} to new cols {{loc: idx, ...}, ...}
                new_col = (cols_iter = new_cols->find(addrx)) == new_cols->end() ?
                          new DemCol() : cols_iter->second;
                new_col->insert(pair<string, unsigned int>(loc, new_vals->size() - 1));
                new_cols->insert(pair<time_t, DemCol*>(addrx, new_col));

            } else {
                auto &ori_val = new_vals->at(kter->second);
                ori_val[0] += cur_val[0];
                ori_val[1] += cur_val[1];
                auto &ori_cnt = new_cnts.at(kter->second);
                ori_cnt += 1;
            }

            ++jter;
        }
        
        // add new row {utm: idx, ...} to new rows {loc: {utm: idx, ...}, ...}
        new_rows->insert(pair<string, DemRow*>(loc, new_row));

        ++iter;
    }

    // delete the old `Cas` and assign a new one with compressed data
    delete cas;
    cas = new CasData(new_rows, new_cols, new_vals);

    delete data_pair;
    data_pair = nullptr;
}

void compress_temporal_cases(
    CasData*& cas, AddrTable*& addr_table,
    const char*& st_dtm, const char*& nd_dtm, const char*& temporal_step) {
    
    const time_t st_utm = Xtm(st_dtm, YMD_DASH).to_utm();
    const time_t nd_utm = Xtm(nd_dtm, YMD_DASH).to_utm();
    auto *data_pair = Xtm::from_rule(temporal_step);
    auto step_u = data_pair->get_first();
    auto step_a = abs(data_pair->get_second());

    // predefine a function pointer that could be any one of encoders for step 'Y', 'm', or 'd'
    time_t (*encode)(const time_t&);
    // and predefine another pointer as decoding function that is able to recover codecs back
    time_t (*decode)(const time_t&);
    
    if (step_u == 'd') {
        // 2016-12-29 2016-12-30 2016-12-31 2017-01-01 2017-01-02 ...
        // eg. (utm(2016-12-29) - utm(2016-12-29)) / 86400 = 0 -> 1
        //     (utm(2016-12-30) - utm(2016-12-29)) / 86400 = 1 -> 2
        //     (utm(2016-12-31) - utm(2016-12-29)) / 86400 = 2 -> 3
        //     (utm(2017-01-01) - utm(2016-12-29)) / 86400 = 3 -> 4
        //     (utm(2017-01-02) - utm(2016-12-29)) / 86400 = 4 -> 5
        //     ...
        //  => (utm / 86400) - (min(utm) / 86400) + 1
        //  OR (max(utm) / 86400) - (utm / 86400) + 1
        encode = [](const time_t& _x)->time_t { return _x / 86400; };
    } else if (step_u == 'm') {
        // 2016-11 2016-12 2017-01 2017-02 2017-03
        // eg. 2016 * 12 + 11                   (1 * 12 + 11) = 23 -> 1
        //     2016 * 12 + 12 = 2017 * 12 + 0 = (1 * 12 + 12) = 24 -> 2
        //     2017 * 12 + 01                   (2 * 12 + 01) = 25 -> 3
        //     2017 * 12 + 02                   (2 * 12 + 02) = 26 -> 4
        //     ...
        //  => (yr * 12 + mon) - min(yr * 12 + mon) + 1
        //  OR max(yr * 12 + mon) - (yr * 12 + mon) + 1
        encode = [](const time_t& _x)->time_t { Xtm _xtm(_x); return _xtm['Y'] * 12 + _xtm['m']; };
    } else if (step_u == 'Y') {
        // 2016 2017 2018
        // eg. 2016 -> 1
        //     2017 -> 2
        //     2018 -> 3
        //     ...
        //  => yr - min(yr) + 1
        //  OR max(yr) - yr + 1
        encode = [](const time_t& _x)->time_t { Xtm _xtm(_x); return _xtm['Y']; };
    } else {
        const char *err = "Error: unit of temporal step must be on of 'Y', 'm', or 'd'";
        cout << err << endl;
        system("pause");
        throw err;
    }

    auto &min_utm = st_utm, &max_utm = nd_utm;
    time_t min_x = encode(min_utm), max_x = encode(max_utm);

    // after defining `fn` and `min_x` for different steps ('Y', 'm', or 'd')
    // all regularized timestamps become the result of expression:
    //     `cur_x` = `fn(utm)` - `min_x` + 1
    //  or `cur_x` = `max_x` - `fn(utm)` + 1 (here we use this equation)
    // and steps for different units become constant value 1 with consistent unit with the original unit,
    // which means 1 year under 'Y', 1 month under 'm', and 1 day under 'd';
    // and then the compression process can be simplified to a "hashtable" stuff like:
    //     `addrx` = (int) (`cur_x` / `step_a`)
    // to map all timestamps into several bags, of which each bag is a temporal unit for further scanning
    
    auto &rows = cas->rows;
    auto &cols = cas->cols;
    auto &vals = cas->vals;
    time_t cur_x, addrx, spx = (max_x - min_x) / step_a;
    // for (auto iter = cols->begin(); iter != cols->end(); ++iter) {
    //     auto &utm = iter->first;
    // // for (auto i = 0; i < 366; ++i) {
    // //     time_t utm = min_utm + 86400 * i;
    //     cur_x = max_x - encode(utm);
    //     addrx = spx - cur_x / step_a;
    //     cout << Xtm(utm).to_dtm(YMD_DASH) << " -> ";
    //     cout << Xtm(decoding(addrx, st_dtm, nd_dtm, temporal_step)).to_dtm(YMD_DASH) << endl;
    //     // cout << Xtm(max_utm).move(step_u, -(spx - addrx) * step_a).to_dtm(YMD_DASH) << " " << addrx << endl;
    // }
    // return;
    
    auto *new_rows = new DemRows();
    auto *new_cols = new DemCols();
    auto *new_vals = new DemVals();
    auto new_cnts = vector<unsigned int>();
    DemRow *new_row;
    DemCol *new_col;
    size_t *new_val;
    set<time_t> *addrval;
    DemCols::iterator cols_iter;
    AddrTable::iterator addr_iter;

    auto iter = rows->cbegin();
    // in loop i, each iteration is a location
    while (iter != rows->cend()) {
        auto &loc = iter->first;
        auto &row = iter->second;
        auto jter = row->cbegin();
        new_row = new DemRow();
        // in loop j, each iteratoin is a timestamp
        while (jter != row->cend()) {
            auto &curutm = jter->first;
            
            addrx = spx - (max_x - encode(curutm)) / step_a;
            size_t *cur_val = vals->at(row->find(curutm)->second);

            addrval = (addr_iter = addr_table->find(addrx)) == addr_table->end() ?
                      new set<time_t>() : addr_iter->second;
            addrval->insert(curutm);
            addr_table->insert(pair<time_t, set<time_t>*>(addrx, addrval));
            
            auto kter = new_row->find(addrx);
            if (kter == new_row->end()) {
                new_vals->push_back(new size_t[2]{cur_val[0], cur_val[1]});
                new_cnts.push_back(1);
                new_row->insert(pair<time_t, unsigned int>(addrx, new_vals->size() - 1));
                
                // add new col {loc: idx, ...} to new cols {{loc: idx, ...}, ...}
                new_col = (cols_iter = new_cols->find(addrx)) == new_cols->end() ?
                          new DemCol() : cols_iter->second;
                new_col->insert(pair<string, unsigned int>(loc, new_vals->size() - 1));
                new_cols->insert(pair<time_t, DemCol*>(addrx, new_col));

            } else {
                auto &ori_val = new_vals->at(kter->second);
                ori_val[0] += cur_val[0];
                ori_val[1] += cur_val[1];
                auto &ori_cnt = new_cnts.at(kter->second);
                ori_cnt += 1;
            }

            ++jter;
        }
        
        // add new row {utm: idx, ...} to new rows {loc: {utm: idx, ...}, ...}
        new_rows->insert(pair<string, DemRow*>(loc, new_row));

        ++iter;
    }

    // delete the old `Cas` and assign a new one with compressed data
    delete cas;
    cas = new CasData(new_rows, new_cols, new_vals);

    delete data_pair;
    data_pair = nullptr;
}


void geography_spatial_radiation(
    RadTable*& rad_table, GeoData* const& geo, const bool& use_geo_coord = true) {

    auto &coordinates = geo->coordinates;

    const size_t& geo_len = coordinates->size();
    double** dist_matrix = new double*[geo_len];

    unsigned int idx = 0;
    while (idx < geo_len) {
        auto &c_coord = coordinates->at(idx);
        auto &centre = c_coord->loc;
        
        BorderSeq* border_seq = new BorderSeq();
        double* dist_row = new double[geo_len - idx];

        unsigned int jdx = 0;
        while (jdx < idx) {
            auto &ex_centre = coordinates->at(jdx)->loc;
            auto &ex_row = dist_matrix[jdx];

            auto &ex_distance = ex_row[idx - jdx];

            unsigned int step = ex_distance / SPATIAL_STEP;
            auto ex_pair = border_seq->find(step);
            if (ex_pair == border_seq->end())
                border_seq->insert(pair<unsigned int, list<string>*>(step, new list<string>(1, ex_centre)));
            else
                ex_pair->second->push_back(ex_centre);

            ++jdx;
            // cout << "ex [" << jdx << "]" << centre << " ~ " << ex_centre;
            // cout << " = " << ex_distance << " / " << step << endl;
        }

        border_seq->insert(pair<unsigned int, list<string>*>(0, new list<string>(1, centre)));
        ++jdx;
        // // in cout output, column indexes of the first centre may not be right,
        // //   however it is OK with that since the distances results are validated by Python results;
        // cout << "z- [" << jdx << "]" << centre << " ~ " << centre;
        // cout << " = " << 0 << " / " << 0 << endl;

        while (jdx < geo_len) {
            auto &b_coord = coordinates->at(jdx);
            auto &border = b_coord->loc;

            double distance = (use_geo_coord) ? geographic_distance(c_coord, b_coord)
                                              : euclidean_distance(c_coord, b_coord);
            dist_row[jdx - idx] = distance;

            unsigned int step = distance / SPATIAL_STEP;
            auto ex_pair = border_seq->find(step);
            if (ex_pair == border_seq->end())
                border_seq->insert(pair<unsigned int, list<string>*>(step, new list<string>(1, border)));
            else
                ex_pair->second->push_back(border);

            ++jdx;
            // cout << "rs [" << jdx << "]" << centre << " ~ " << border;
            // cout << " = " << distance << " / " << step << endl;
        }

        dist_matrix[idx] = dist_row;
        rad_table->insert(pair<string, BorderSeq*>(centre, border_seq));

        ++idx;
    }

    for (auto i = 0; i < geo_len; ++i) {
        delete dist_matrix[i];
        dist_matrix[i] = nullptr;
    }
    delete dist_matrix;
    dist_matrix = nullptr;

    // int i = 0;
    // for (auto iter = rad_table->begin(); iter != rad_table->end(); ++iter, ++i) {
    //     auto &border_seq = iter->second;
    //     int j = 0;
    //     for (auto jter = border_seq->cbegin(); jter != border_seq->cend(); ++jter, ++j) {
    //         auto &border = jter->second;
    //         cout << "[" << i << ", " << j << "] " << iter->first << " ~ " << *(border->cbegin());
    //         cout << " = dist / " << jter->first << endl;
    //     }
    // }
}


void geography_spatial_radiation(
    GeoData*& geo, RadTable*& rad_table, const bool& use_geo_coord = true) {

    auto &coordinates = geo->coordinates;
    // if (!use_geo_coord)             // since the default coordinates are supposed to be geographic,
    //     to_cartesian(coordinates);  // if user want cartesian coordinates, convert geo to it directly

    const size_t& geo_len = coordinates->size();
    double** dist_matrix = new double*[geo_len];

    unsigned int idx = 0;
    while (idx < geo_len) {
        auto &c_coord = coordinates->at(idx);
        auto &centre = c_coord->loc;
        
        BorderSeq* border_seq = new BorderSeq();
        double* dist_row = new double[geo_len - idx];

        unsigned int jdx = 0;
        while (jdx < idx) {
            auto &ex_centre = coordinates->at(jdx)->loc;
            auto &ex_row = dist_matrix[jdx];

            auto &ex_distance = ex_row[idx - jdx];

            unsigned int step = ex_distance / SPATIAL_STEP;
            auto ex_pair = border_seq->find(step);
            if (ex_pair == border_seq->end())
                border_seq->insert(pair<unsigned int, list<string>*>(step, new list<string>(1, ex_centre)));
            else
                ex_pair->second->push_back(ex_centre);

            ++jdx;
            // cout << "ex [" << jdx << "]" << centre << " ~ " << ex_centre;
            // cout << " = " << ex_distance << " / " << step << endl;
        }

        border_seq->insert(pair<unsigned int, list<string>*>(0, new list<string>(1, centre)));
        ++jdx;
        // // in cout output, column indexes of the first centre may not be right,
        // //   however it is OK with that since the distances results are validated by Python results;
        // cout << "z- [" << jdx << "]" << centre << " ~ " << centre;
        // cout << " = " << 0 << " / " << 0 << endl;

        while (jdx < geo_len) {
            auto &b_coord = coordinates->at(jdx);
            auto &border = b_coord->loc;

            // if use cartesian coordinates here, the distance measurement must be `euclidean`'s
            double distance = (use_geo_coord) ? geographic_distance(c_coord, b_coord)
                                              : euclidean_distance(c_coord, b_coord);
            dist_row[jdx - idx] = distance;

            unsigned int step = distance / SPATIAL_STEP;
            auto ex_pair = border_seq->find(step);
            if (ex_pair == border_seq->end())
                border_seq->insert(pair<unsigned int, list<string>*>(step, new list<string>(1, border)));
            else
                ex_pair->second->push_back(border);

            ++jdx;
            // cout << "rs [" << jdx << "]" << centre << " ~ " << border;
            // cout << " = " << distance << " / " << step << endl;
        }

        dist_matrix[idx] = dist_row;
        rad_table->insert(pair<string, BorderSeq*>(centre, border_seq));

        ++idx;
    }

    for (auto i = 0; i < geo_len; ++i) {
        delete[] dist_matrix[i];
        dist_matrix[i] = nullptr;
    }
    delete[] dist_matrix;
    dist_matrix = nullptr;
}


PopulationExposureIndex build_population_exposure_index(
    GeoData* const& geo, PopData* const& pop, CasData* const& cas, AddrTable* const& addr_table,
    const time_t& st_utm, const time_t& nd_utm) {

    PopulationExposureIndex result;

    for (auto iter = addr_table->cbegin(); iter != addr_table->cend(); ++iter) {

        const time_t bucket = iter->first;
        const auto* original_times = iter->second;

        if (original_times == nullptr || original_times->empty())
            continue;

        bool in_study_period = false;

        for (const time_t t : *original_times) {
            if (t >= st_utm && t <= nd_utm) {
                in_study_period = true;
                break;
            }
        }

        if (in_study_period) {
            result.time_index.emplace(bucket, result.times.size());
            result.times.push_back(bucket);
        }
    }

    if (result.times.empty()) {
        throw std::runtime_error("No case time buckets in study period");
    }

    const size_t hg = result.times.size();

    for (auto iter = geo->locations->cbegin(); iter != geo->locations->cend(); ++iter) {

        const std::string& location = iter->first;
        const unsigned int location_id = iter->second;

        auto row_iter = pop->rows->find(location);

        if (row_iter == pop->rows->end() || row_iter->second == nullptr || row_iter->second->empty())
            throw std::runtime_error("Missing population records for location: " + location);

        const auto* row = row_iter->second;
        std::vector<double> prefix(hg + 1, 0.0);

        for (size_t i = 0; i < hg; ++i) {
            const time_t bucket = result.times[i];
            const auto addr_iter = addr_table->find(bucket);

            if (addr_iter == addr_table->end() || addr_iter->second == nullptr || addr_iter->second->empty()) {
                throw std::runtime_error("Missing original timestamps for bucket");
            }

            double population_exposure = 0.0;
            size_t time_count = 0;

            for (const time_t t : *addr_iter->second) {
                if (t < st_utm || t > nd_utm)
                    continue;

                size_t population = 0;

                if (row->size() == 1) {
                    population = pop->vals->at(row->begin()->second)[0];
                } else {
                    auto pop_iter = row->find(t);

                    if (pop_iter == row->end()) {
                        throw std::runtime_error(
                            "Population timestamp does not match case timestamp for location: " + location);
                    }

                    population = pop->vals->at(pop_iter->second)[0];
                }

                population_exposure += population;
                ++time_count;
            }

            if (time_count == 0)
                throw std::runtime_error("No valid timestamps for population bucket");

            prefix[i + 1] = prefix[i] + population_exposure;
        }

        const double location_exposure = prefix.back();

        result.total_exposure += location_exposure;
        result.average_population[location_id] = location_exposure / static_cast<double>(hg);
        result.prefix.emplace(location_id, std::move(prefix));
    }

    if (result.total_exposure <= 0.0)
        throw std::runtime_error("Total population exposure must be positive");

    result.average_total_population = result.total_exposure / static_cast<double>(hg);
    return result;
}

double population_exposure_in_window(
    const PopulationExposureIndex& index, const std::set<unsigned int>& locations,
    const size_t& start_index, const size_t& end_index) {
    
    if (start_index > end_index || end_index >= index.times.size())
        throw std::out_of_range("Invalid population exposure window");

    double exposure = 0.0;

    for (const unsigned int location_id : locations) {
        auto iter = index.prefix.find(location_id);

        if (iter == index.prefix.end())
            throw std::runtime_error("Missing location population prefix");

        const auto& prefix = iter->second;

        exposure += prefix[end_index + 1] - prefix[start_index];
    }

    return exposure;
}


template <typename T>
void approx_bisect(T**& slices, T* const& ss) {
    size_t n = get_size(slices);
    size_t ss_size = get_size(ss);

    if (n == 0)
        return;

    if (ss_size < n) {
        slices[0] = ss;
        return;
    }

    unsigned int st_pos, nd_pos;
    T* slice;
    for (auto i = 0; i < n; ++i) {
        st_pos = ((double) i) / n * ss_size;
        nd_pos = ((nd_pos = ((double) i + 1) / n * ss_size) < ss_size) ? nd_pos : ss_size;
        cout << "[" << i << "] " << st_pos << " ~ " << nd_pos << endl;
        slice = new T[nd_pos - st_pos];
        for (unsigned int j = st_pos, k = 0; j < nd_pos; ++j, ++k)
            slice[k] = ss[j];
        slices[i] = slice;
    }
}


void random_poisson(double*& ss, const double& lam, const unsigned int& n) {
    static std::mt19937 generator(std::random_device{}());
    std::poisson_distribution<int> distribution(lam);

    for (unsigned int i = 0; i < n; ++i) {
        ss[i] = static_cast<double>(distribution(generator));
    }
}


void destroy(AddrTable*& addr_table) {
    if (addr_table == nullptr)
        return;

    for (auto iter = addr_table->begin(); iter != addr_table->end(); ++iter) {
        auto &addrval = iter->second;
        addrval->clear();
        delete addrval;
        addrval = nullptr;
    }
    addr_table->clear();
    delete addr_table;
    addr_table = nullptr;
}

void destroy(RadTable*& rad_table) {
    if (rad_table == nullptr)
        return;

    for (auto iter = rad_table->begin(); iter != rad_table->end(); ++iter) {
        auto &border_seq = iter->second;
        for (auto jter = border_seq->begin(); jter != border_seq->end(); ++jter) {
            auto &lst = jter->second;
            lst->clear();
            delete lst;
            lst = nullptr;
        }
        border_seq->clear();
        delete border_seq;
        border_seq = nullptr;
    }
    rad_table->clear();
    delete rad_table;
    rad_table = nullptr;
}
