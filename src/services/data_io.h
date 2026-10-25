#ifndef _Data_IO_H_
#define _Data_IO_H_

#include <fstream>
#include <iomanip>
#include <sstream>

#include "../services/dataframe.h"
#include "../services/cluster.h"


void load_geo_from_csv(GeoData*&, const char*&);

void load_pop_from_csv(PopData*&, const char*&);

void load_ps_pop_from_csv(PopData*&, const char*&);

void load_pt_pop_from_csv(PopData*&, const char*&);

void load_cas_from_csv(CasData*&, const char*&);

void load_ps_cas_from_csv(CasData*&, const char*&);

void load_pt_cas_from_csv(CasData*&, const char*&);

typedef unordered_map<IdType, string> StrMap;
void _ssmap_to_strmap(StrMap*&, LocssMap* const&, GeoData* const&);
void _ssmap_to_strmap(StrMap*&, UtmssMap* const&);
void write_clusters_to_csv(
    const char*, Clusters* const&, GeoData* const&,
    LocssMap* const, UtmssMap* const, const double&, const double&, const double&);

void write_message(const char*, const string&);

void show_clusters(Clusters*&, GeoData* const&, LocssMap*, UtmssMap*);

void read_parameters(const char*, unordered_map<string, string>&);


void destroy(StrMap*& strmap) {
    if (strmap == nullptr)
        return;

    strmap->clear();
    delete strmap;
    strmap = nullptr;
};


#include "data_io.cpp"
#endif
