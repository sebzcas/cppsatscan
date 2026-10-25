#include "data_io.h"


void load_geo_from_csv(GeoData*& geo, const char*& path) {
    auto& locations = geo->locations;
    auto& coordinates = geo->coordinates;

    ifstream in_file(path, ios::in);
    if (!in_file) {
        const char *err = "Error: failed to open file for geo data";
        cout << err << endl;
        throw err;
    }

    unsigned int i = 0;
    string line;
    while (getline(in_file, line)) {
        string field;
        istringstream stream_in(line);
        unsigned int j = 0;
        string loc;
        double lat, lon;
        while (getline(stream_in, field, ',')) {
            const char* c = field.c_str();
            if (j == 0)
                loc = str(c).to_string();
            else if (j == 1)
                lon = atof(c);
            else
                lat = atof(c);
            j++;
        }
        locations->insert(pair<string, unsigned int>(loc, i));
        coordinates->push_back(new Coordinate(loc, lat, lon));
        i++;
    }
    in_file.close();
}

void load_pop_from_csv(PopData*& pop, const char*& path) {
    auto& rows = pop->rows;
    auto& cols = pop->cols;
    auto& vals = pop->vals;

    ifstream in_file(path, ios::in);
    if (!in_file) {
        const char *err = "Error: failed to open file for pop data";
        cout << err << endl;
        throw err;
    }

    DemRow* row;
    DemCol* col;
    auto iter = rows->begin();
    auto jter = cols->begin();
    unsigned int i = 0;
    string line;
    while (getline(in_file, line)) {
        string field;
        istringstream stream_in(line);
        unsigned int j = 0;
        string loc;
        time_t utm;
        size_t val;
        while (getline(stream_in, field, ',')) {
            const char* c = field.c_str();
            if (j == 0)
                loc = str(c).to_string();
            else if (j == 1)
                utm = Xtm(c, YMD_DASH).to_utm();
            else
                val = str(c).to_int();
            j++;
        }

        if (j < 3 || loc.empty() || utm == static_cast<time_t>(-1))
            throw runtime_error("Invalid population record at line " + to_string(i + 1) + ": " + line);

        row = ((iter = rows->find(loc)) == rows->end()) ? new DemRow() : iter->second;        
        row->insert(pair<time_t, unsigned int>(utm, i));
        col = ((jter = cols->find(utm)) == cols->end()) ? new DemCol() : jter->second;
        col->insert(pair<string, unsigned int>(loc, i));
    
        rows->insert(pair<string, DemRow*>(loc, row));
        cols->insert(pair<time_t, DemCol*>(utm, col));
        vals->push_back(new size_t[1]{val});

        i++;
    }
    in_file.close();
}

void load_ps_pop_from_csv(PopData*& pop, const char*& path) {
    auto& rows = pop->rows;
    auto& cols = pop->cols;
    auto& vals = pop->vals;

    ifstream in_file(path);
    if (!in_file)
        throw runtime_error("Error: failed to open file for pop data");

    unordered_map<string, size_t> totals;
    unordered_map<string, size_t> counts;

    string line;
    while (getline(in_file, line)) {
        istringstream ss(line);
        string loc, year_str, val_str;

        if (!getline(ss, loc, ',') ||
            !getline(ss, year_str, ',') ||
            !getline(ss, val_str, ',')) {
            throw runtime_error("Invalid population CSV row: " + line);
        }

        loc = str(loc.c_str()).to_string();
        size_t val = str(val_str.c_str()).to_int();

        totals[loc] += val;
        counts[loc]++;
    }

    time_t utm = 0;
    unsigned int i = 0;
    auto* col = new DemCol();

    for (const auto& entry : totals) {
        const string& loc = entry.first;
        const size_t total = entry.second;

        if (counts[loc] == 0)
            throw runtime_error("No population observations: " + loc);

        size_t average = total / counts[loc];

        auto* row = new DemRow();
        row->insert({utm, i});
        rows->insert({loc, row});

        col->insert({loc, i});
        vals->push_back(new size_t[1]{average});

        ++i;
    }

    cols->insert({utm, col});
}

void load_pt_pop_from_csv(PopData*& pop, const char*& path) {
    auto& rows = pop->rows;
    auto& cols = pop->cols;
    auto& vals = pop->vals;

    ifstream in_file(path, ios::in);
    if (!in_file)
        throw runtime_error("Error: failed to open file for pop data");

    const string loc("");
    map<time_t, size_t> totals;

    string line;
    while (getline(in_file, line)) {
        istringstream stream_in(line);
        string loc_field, year_field, val_field;

        if (!getline(stream_in, loc_field, ',') ||
            !getline(stream_in, year_field, ',') ||
            !getline(stream_in, val_field, ','))
            throw runtime_error("Invalid population CSV row: " + line);

        time_t utm = Xtm(year_field.c_str(), YMD_DASH).to_utm();
        size_t val = str(val_field.c_str()).to_int();

        totals[utm] += val;
    }

    in_file.close();

    DemRow* row = new DemRow();
    DemCol* col;
    unsigned int i = 0;

    for (map<time_t, size_t>::const_iterator iter = totals.begin(); iter != totals.end(); ++iter) {
        time_t utm = iter->first;
        size_t total = iter->second;

        row->insert(make_pair(utm, i));

        col = new DemCol();
        col->insert(make_pair(loc, i));
        cols->insert(make_pair(utm, col));

        vals->push_back(new size_t[1]{total});
        ++i;
    }

    rows->insert(make_pair(loc, row));
}

void load_cas_from_csv(CasData*& cas, const char*& path) {
    auto& rows = cas->rows;
    auto& cols = cas->cols;
    auto& vals = cas->vals;

    ifstream in_file(path, ios::in);
    if (!in_file) {
        const char *err = "Error: failed to open file for cas data";
        cout << err << endl;
        throw err;
    }

    DemRow* row;
    DemCol* col;
    auto iter = rows->begin();
    auto jter = cols->begin();
    unsigned int i = 0;
    string line;
    while (getline(in_file, line)) {
        string field;
        istringstream stream_in(line);
        unsigned int j = 0;
        string loc;
        time_t utm;
        size_t val;
        while (getline(stream_in, field, ',')) {
            const char* c = field.c_str();
            if (j == 0)
                loc = str(c).to_string();
            else if (j == 1)
                utm = Xtm(c, YMD_DASH).to_utm();
            else
                val = str(c).to_int();
            j++;
        }

        row = ((iter = rows->find(loc)) == rows->end()) ? new DemRow() : iter->second;        
        row->insert(pair<time_t, unsigned int>(utm, i));
        col = ((jter = cols->find(utm)) == cols->end()) ? new DemCol() : jter->second;
        col->insert(pair<string, unsigned int>(loc, i));
    
        rows->insert(pair<string, DemRow*>(loc, row));
        cols->insert(pair<time_t, DemCol*>(utm, col));
        vals->push_back(new size_t[1]{val});

        i++;
    }
    in_file.close();
}

void load_ps_cas_from_csv(CasData*& cas, const char*& path) {
    auto& rows = cas->rows;
    auto& cols = cas->cols;
    auto& vals = cas->vals;

    ifstream in_file(path);
    if (!in_file)
        throw runtime_error("Error: failed to open file for cas data");

    unordered_map<string, size_t> totals;

    string line;
    while (getline(in_file, line)) {
        istringstream ss(line);
        string loc, year_str, val_str;

        if (!getline(ss, loc, ',') ||
            !getline(ss, year_str, ',') ||
            !getline(ss, val_str, ','))
            throw runtime_error("Invalid case CSV row: " + line);

        loc = str(loc.c_str()).to_string();
        size_t val = str(val_str.c_str()).to_int();

        totals[loc] += val;
    }

    time_t utm = 0;
    unsigned int i = 0;
    auto* col = new DemCol();

    for (const auto& entry : totals) {
        const string& loc = entry.first;
        const size_t total = entry.second;
        
        auto* row = new DemRow();
        row->insert({utm, i});
        rows->insert({loc, row});

        col->insert({loc, i});
        vals->push_back(new size_t[1]{total});

        ++i;
    }

    cols->insert({utm, col});
}

void load_pt_cas_from_csv(CasData*& cas, const char*& path) {
    auto& rows = cas->rows;
    auto& cols = cas->cols;
    auto& vals = cas->vals;

    ifstream in_file(path, ios::in);
    if (!in_file)
        throw runtime_error("Error: failed to open file for cas data");

    const string loc("");
    map<time_t, size_t> totals;

    string line;
    while (getline(in_file, line)) {
        istringstream stream_in(line);
        string loc_field, year_field, val_field;

        if (!getline(stream_in, loc_field, ',') ||
            !getline(stream_in, year_field, ',') ||
            !getline(stream_in, val_field, ','))
            throw runtime_error("Invalid case CSV row: " + line);

        time_t utm = Xtm(year_field.c_str(), YMD_DASH).to_utm();
        size_t val = str(val_field.c_str()).to_int();

        totals[utm] += val;
    }

    in_file.close();

    DemRow* row = new DemRow();
    DemCol* col;
    unsigned int i = 0;

    for (map<time_t, size_t>::const_iterator iter = totals.begin(); iter != totals.end(); ++iter) {
        time_t utm = iter->first;
        size_t total = iter->second;

        row->insert(make_pair(utm, i));

        col = new DemCol();
        col->insert(make_pair(loc, i));
        cols->insert(make_pair(utm, col));

        vals->push_back(new size_t[1]{total});
        ++i;
    }

    rows->insert(make_pair(loc, row));
}


void _ssmap_to_strmap(StrMap*& strmap, LocssMap* const& locssmap, GeoData* const& geo) {
    auto &coordinates = geo->coordinates;
    for (auto iter = locssmap->cbegin(); iter != locssmap->cend(); ++iter) {
        auto &locss = iter->second;
        string s = "";
        unsigned int idx = 0;
        for (auto jter = locss->cbegin(); jter != locss->cend(); ++jter, ++idx) {
            s += "'" + coordinates->at(*jter)->loc + "'";
            if (idx < locss->size() - 1 || locss->size() == 1)
                s += (locss->size() == 1) ? "," : ", ";
        }
        strmap->insert(pair<IdType, string>(iter->first, s));
    }
}

void _ssmap_to_strmap(StrMap*& strmap, UtmssMap* const& utmssmap) {
    for (auto iter = utmssmap->cbegin(); iter != utmssmap->cend(); ++iter) {
        auto &utmss = iter->second;
        string s = "";
        unsigned int idx = 0;
        for (auto jter = utmss->cbegin(); jter != utmss->cend(); ++jter, ++idx) {
            s += "'" + Xtm(*jter).to_dtm(YMD_DASH) + "'";
            if (idx < utmss->size() - 1 || utmss->size() == 1)
                s += (utmss->size() == 1) ? "," : ", ";
        }
        strmap->insert(pair<IdType, string>(iter->first, s));
    }
}

void write_clusters_to_csv(
    const char* path, Clusters* const& clusters, GeoData* const& geo,
    LocssMap* const locssmap = nullptr, UtmssMap* const utmssmap = nullptr,
    const size_t& cg = 0, const size_t& bg = 0, const size_t& hg = 0) {

    // * if `locssmap` or `utmssmap` is input to add specific `ss` entities into file,
    //   then convert them to string values firstly to avoid performance costs in each iterations below
    StrMap *locss_strmap = nullptr, *utmss_strmap = nullptr;
    if (locssmap != nullptr) {
        locss_strmap = new StrMap();
        _ssmap_to_strmap(locss_strmap, locssmap, geo);
    }
    if (utmssmap != nullptr) {
        utmss_strmap = new StrMap();
        _ssmap_to_strmap(utmss_strmap, utmssmap);
    }

    ofstream out_file(path, ios::out);

    unsigned int idx = 0;
    Cluster* cluster;

    out_file << "id,sid,tid,locations,timespan,cz,bz,hz,llr,p_val,centre,radius,cg,bg,hg,mz,rr,oer,ann" << endl;
    for (auto iter = clusters->cbegin(); iter != clusters->cend(); ++iter, ++idx) {
        cluster = *iter;
        auto &sid = cluster->sid;
        auto &tid = cluster->tid;
        out_file << idx << ",";
        out_file << cluster->id << "," << sid << "," << tid << ",";
        out_file << "\"(";
        out_file << ((locss_strmap) ? locss_strmap->find(sid)->second : "");
        out_file << ")\",\"(";
        out_file << ((utmss_strmap) ? utmss_strmap->find(tid)->second : "");
        out_file << ")\",";
        out_file << cluster->cz << ","  << cluster->bz << "," << cluster->hz << ",";
        out_file << ROUND(cluster->llr, 6) << ",";
        out_file << ROUND(cluster->pval, 3) << ",";
        out_file << cluster->centre << "," << cluster->radius << ",";
        out_file << cg << "," << bg << "," << hg << ",";
        out_file << ROUND(cluster->mz, 2) << ",";
        out_file << ROUND(cluster->rr, 2) << ",";
        out_file << ROUND(cluster->oer, 2) << ",";
        out_file << ROUND(cluster->ann, 1) << endl;
    }

    out_file.close();
    cout << "completed writing clusters objects to file \"" << path << "\"" << endl;

    // after written string into files, free `locssmap` and `utmssmap` if they are input as instances
    // free memory for `locssmap` when it's not null
    if (locss_strmap) {
        locss_strmap->clear();
        delete locss_strmap;
        locss_strmap = nullptr;
    }
    // free memory for `utmssmap` when it's not null
    if (utmss_strmap) {
        utmss_strmap->clear();
        delete utmss_strmap;
        utmss_strmap = nullptr;
    }
}


void write_message(const char* path, const string& message) {
    ofstream out_file(path, ios::out);
    out_file << message << endl;
    out_file.close();
    cout << "complete writing messages to file \"" << path << "\"" << endl;
}


void show_clusters(
    Clusters* const& clusters, GeoData* const& geo,
    LocssMap* locssmap = nullptr, UtmssMap* utmssmap = nullptr) {
    
    StrMap *locss_strmap = nullptr, *utmss_strmap = nullptr;
    if (locssmap != nullptr)
        _ssmap_to_strmap(locss_strmap = new StrMap(), locssmap, geo);
    if (utmssmap != nullptr)
        _ssmap_to_strmap(utmss_strmap = new StrMap(), utmssmap);

    for (auto iter = clusters->begin(); iter != clusters->end(); ++iter) {
        auto &cluster = *iter;
        cout << cluster->id << ": " << endl;
        cout << "  S(" << ((locss_strmap) ? locss_strmap->find(cluster->sid)->second : "") << "),\n";
        cout << "  T(" << ((locss_strmap) ? locss_strmap->find(cluster->tid)->second : "") << ")\n  => ";
        cout << "(LLR = " << cluster->llr;
        cout << ", CZ = " << cluster->cz;
        cout << ", BZ = " << cluster->bz;
        cout << ", HZ = " << cluster->hz << ")" << endl;
    }

    if (locss_strmap)
        destroy(locss_strmap);
    if (utmss_strmap)
        destroy(utmss_strmap);
}


void read_parameters(const char* path, unordered_map<string, string>& kwargs) {
    ifstream in_file(path, ios::in);
    if (!in_file) {
        const char *err = "Error: failed to open file for parameters";
        cout << err << endl;
        throw err;
    }

    unsigned int i = 0;
    string line;
    while (getline(in_file, line)) {
        string field;
        istringstream stream_in(line);
        unsigned int j = 0;
        string key, value;
        while (getline(stream_in, field, ' ')) {
            const char* c = field.c_str();
            if (j == 0)
                key = str(c).to_string();
            else
                value = str(c).to_string();
            j++;
        }
        kwargs.insert(pair<string, string>(key, value));
        i++;
    }
    in_file.close();
}
