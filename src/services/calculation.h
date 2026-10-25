#ifndef _Calculation_H_
#define _Calculation_H_


#include <cmath>
#include <ctime>
#include <list>
#include <map>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "../services/data_io.h"


#define _PI 3.14159265358979323846
#define _EARTH_RADIUS 6381372
#define _MILL 2.3

constexpr double _W = _PI * _EARTH_RADIUS * 2;
constexpr double _H = _PI * _EARTH_RADIUS;
constexpr double _RADIANS(double d) { return d * _PI / 180; };

inline void to_cartesian(Coordinate* const& coord) {
    double &lat = coord->lat, &lon = coord->lon;
    double x = lat * _PI / 180;
    double y = 1.25 * log(tan(0.25 * _PI + 0.4 * (lon * _PI / 180)));
    x = (_W / 2) + (_W / (2 * _PI)) * x;
    y = (_H / 2) - (_H / (2 * _MILL)) * y;
    lat = x;
    lon = y;
};

inline void to_geographic(Coordinate* const& coord) {
    double &x = coord->lat;
    double &y = coord->lon;
    double lat = (x - _W / 2) * 360 / _W;
    double lon = ((_H / 2 - y) * 2 * _MILL) / (1.25 * _H);
    lon = ((atan(exp(lon)) - 0.25 * _PI) * 180) / (0.4 * _PI);
    x = lat;
    y = lon;
};

inline void to_cartesian(Coordinates*& coordinates) {
    for (auto iter = coordinates->begin(); iter != coordinates->end(); ++iter)
        to_cartesian(*iter);
};

inline void to_geographic(Coordinates*& coordinates) {
    for (auto iter = coordinates->begin(); iter != coordinates->end(); ++iter)
        to_geographic(*iter);
};


inline double euclidean_distance(Coordinate*& a, Coordinate*& b) {
    double &x_a = a->lat, &y_a = a->lon;
    double &x_b = b->lat, &y_b = b->lon;
    double distance = sqrt(pow(x_a - x_b, 2) + pow(y_a - y_b, 2));
    return distance;
};

inline double geographic_distance(Coordinate*& a, Coordinate*& b) {
    double lat_a = _RADIANS(a->lat), lon_a = _RADIANS(a->lon);
    double lat_b = _RADIANS(b->lat), lon_b = _RADIANS(b->lon);
    double d_lat = lat_b - lat_a;
    double d_lon = lon_b - lon_a;
    double t = pow(sin(d_lat / 2), 2) + cos(lat_a) * cos(lat_b) * pow(sin(d_lon / 2), 2);
    double distance = 2 * 6371 * asin(sqrt(t));
    return distance;
};


void aggregate_population(size_t*&, DemVals* const&);

size_t aggregate_population(DemVals* const&);

size_t aggregate_population(DemVals* const&, DemCols* const&, const time_t&);


typedef map<time_t, set<time_t>*> AddrTable;
time_t encode_t(const time_t&, const char*&, const char*&, const char*&);
time_t decode_t(const time_t&, const char*&, const char*&, const char*&);
time_t calculate_time_step_index(const time_t&, const char*&, const char*&, const char*&);
void compress_temporal_cases(CasData*&, const char*&, const char*&, const char*&);
void compress_temporal_cases(CasData*&, AddrTable*&, const char*&, const char*&, const char*&);


#define SPATIAL_STEP 0.01
typedef map<unsigned int, list<string>*> BorderSeq;
typedef map<string, BorderSeq*> RadTable;
void geography_spatial_radiation(RadTable*&, GeoData* const&, const bool&);


struct PopulationExposureIndex {
    std::vector<time_t> times;
    std::map<time_t, size_t> time_index;
    std::unordered_map<unsigned int, std::vector<double>> prefix;
    std::unordered_map<unsigned int, double> average_population;
    double total_exposure = 0.0;
    double average_total_population = 0.0;
};

PopulationExposureIndex build_population_exposure_index(
    GeoData* const&, PopData* const&, CasData* const&, AddrTable* const&, const time_t&, const time_t&);


template <typename T>
void approx_bisect(T**&, T* const&);


void random_poisson(double*&, const double&, const unsigned int&);


void destroy(AddrTable*&);
void destroy(RadTable*&);


#include "calculation.cpp"
#endif
