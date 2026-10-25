#ifndef _Scanning_H_
#define _Scanning_H_


#include "omp.h"

#include "../services/calculation.h"


enum SCANNING_TYPE {PT, PS, ST};


namespace tscan {
    void temporal_scanning(
        T_Timespans*&, UtmssMap*&, PopData* const&, CasData* const&, AddrTable* const&,
        const char*&, const char*&, const char*&, const double&, const unsigned int&);
    
    void update_timespans(
        Clusters*&, T_Timespans* const&, const size_t&, const size_t&, const size_t&);
    
    void filter_clusters(Clusters*&, Clusters* const&, UtmssMap* const&, const size_t&);
    
    void postprocess_clusters(Clusters*&, T_Timespans* const&, const double&, const double&, const double&, const char*&);
}


namespace sscan {
    void spatial_scanning(
        S_Circles*&, LocssMap*&, GeoData* const&, PopData* const&, CasData* const&, RadTable* const&,
        const bool&, const double&, const unsigned int&);

    void update_circles(Clusters*&, S_Circles* const&, const size_t&, const size_t&);
    
    void postprocess_clusters(Clusters*&, S_Circles* const&, GeoData* const&, const bool&);
    void postprocess_clusters(
        Clusters*&, S_Circles* const&, GeoData* const&, const double&, const double&, const double&, const bool&);
}


namespace satscan {
    // Spatial scanning for multidimensional case and population data,
    // where population values vary over time. Spatial scanning is
    // performed independently for each temporal unit.
    void spatial_scanning_impl(
        S_Circles*&, const time_t&, GeoData* const&, PopData* const&, CasData* const&, RadTable* const&);
    void spatial_scanning_3d(
        TS_Circles*&, GeoData* const&, PopData* const&, CasData* const&, const unsigned int&);
        
    // Spatial scanning for multidimensional case data and population
    // data containing only one temporal snapshot. The population
    // baseline is calculated once, and the spatial scan is performed
    // once, producing a single temporal entry with key 0.
    void spatial_scanning_impl(
        S_Circles*&, const time_t&, GeoData* const&, CasData* const&, LocssMap* const&, S_Circles* const&);
    void spatial_scanning_2d(
        TS_Circles*&, LocssMap*&, GeoData* const&, PopData* const&, CasData* const&, RadTable* const&,
        const PopulationExposureIndex&,
        const bool&, const double&, const unsigned int&);
        
    void transpose_clusters(ST_Circles*& , TS_Circles* const&);

    void temporal_scanning_impl(
        Clusters*&, UtmssMap*&, const IdType&, T_Circles* const&, AddrTable* const&,
        LocssMap* const&, const PopulationExposureIndex&,
        const size_t&, const size_t&, const size_t&, const char*&, const char*&, const char*&,
        const double&);
    void temporal_scanning(
        Clusters*&, UtmssMap*&, ST_Circles* const&, AddrTable* const&,
        LocssMap* const&, const PopulationExposureIndex&,
        const size_t&, const size_t&, const size_t&,
        const char*&, const char*&, const char*&,
        const double&, const unsigned int&);

    void postprocess_clusters(Clusters*&, ST_Circles* const&, GeoData* const&, const bool&);
    void postprocess_clusters(
        Clusters*&, ST_Circles* const&, GeoData* const&,
        const double&, const double&, const double&, const bool&, const char*&);
}


typedef unordered_map<string, size_t> SimDemArray;
typedef unordered_map<time_t, SimDemArray*> SimDemMatrix;
void simulation(
    double*&, vector<Cluster*>* const&, const size_t&, const size_t&, const size_t&, const size_t&);


void rank_clusters(Clusters*&, double* const&);

void filter_clusters(Clusters*&, Clusters* const&, LocssMap* const&, const size_t&);

inline double thresholding(const double& p_val) {
    //     0 < x <= 0.001
    // 0.001 < x <= 0.005
    // 0.005 < x <= 0.01
    //  0.01 < x <= 0.05
    //  0.05 < x <= 0.1
    //   0.1 < x <= 0.5
    //   0.5 < x <= 1.0
    if (p_val < 0)
        return 0;

    double arr[] = {0, 0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1};
    size_t size = get_size(arr);
    for (auto i = 0; i < size - 1; ++i) {
        if (arr[i] < p_val && p_val <= arr[i + 1])
            return arr[i + 1];
    }

    return 1;
};


#include "scanning.cpp"
#endif
