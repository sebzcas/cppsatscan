#ifndef _Cluster_H_
#define _Cluster_H_

#include <iostream>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <list>
#include <string>
#include <cmath>
#include <limits>

using namespace std;


#define KULLDORFF_YDAY 365.2425


typedef long long IdType;


struct Circle {
    IdType id = -1;
    size_t cz = 0;
    size_t bz = 0;
    
    unsigned int centre;
    unsigned int border;

    Circle(set<unsigned int>* const&, const size_t&, const size_t&);
    Circle() {};
};


struct Timespan {
    IdType id = -1;
    size_t cz = 0;
    size_t bz = 0;
    size_t hz = 1;

    Timespan(list<time_t>* const&, const size_t&, const size_t&);
    Timespan(list<time_t>* const&, const size_t&, const size_t&, const size_t&);
    Timespan(const time_t&, const time_t&, const size_t&, const size_t&, const size_t&);
    Timespan() {};
};


// Cluster represents a candidate spatial, temporal, or space-time cluster.
//
// Statistical fields:
//   cz / bz / hz: cluster-level case, population-exposure, and time quantities.
//   cg / bg / hg: corresponding study-wide quantities.
//   llr: log-likelihood ratio.
//   pval: Monte Carlo significance estimate.
//
// Exact aggregation semantics depend on the scanning mode.
struct Cluster {
    IdType id = -1;
    IdType sid = -1;
    IdType tid = -1;
    size_t cz = 0, bz = 0, hz = 0;
    double llr = -1;
    double pval = -1;

    double radius = -1;
    string centre = "";

    size_t cg = 0, bg = 0, hg = 0;
    double mz = -1;
    double rr = -1;
    double oer = -1;
    double ann = -1;

    Cluster(const IdType&, const IdType&, const size_t&, const size_t&, const size_t&,
            const size_t&, const size_t&, const size_t&);
    Cluster(const IdType&, const IdType&, const size_t&, const size_t&, const size_t&);
    Cluster() {};

    inline static double calc_mz(
        const double& cg, const double& bz, const double& bg, const double& hz, const double hg) {
        return (cg / bg) * bz * (hz / hg);
    };
    inline static double calc_llr(const double& cz, const double& cg, const double& mz) {
        // Returns NaN when the inputs violate the valid range of the likelihood-ratio calculation.
        if (cz < 0.0 || cg <= 0.0 || cz > cg || mz <= 0.0 || mz >= cg)
            return std::numeric_limits<double>::quiet_NaN();
    
        // Returns zero when the observed cluster count does not exceed
        // its expected count under the cluster alternative.
        if (cz <= mz)
            return 0.0;
        
        double llr = 0.0;
        if (cz > 0.0)
            llr += cz * std::log(cz / mz);

        if (cg > cz)
            llr += (cg - cz) * std::log((cg - cz) / (cg - mz));
    
        return llr;
    };
    inline static double calc_rr(
        const double& cz, const double& cg, const double& mz) { return (cz / mz) / ((cg - cz) / (cg - mz)); };
    inline static double calc_oer(const double& cz, const double& mz) { return cz / mz; };
    inline static double calc_ann(
        const double& cz, const double& bx, const double& hx) { return cz / hx * KULLDORFF_YDAY / bx * 100000; };

    inline static int compare(const void* c1, const void* c2) {
        return ((Cluster*) c1)->llr > ((Cluster*) c2)->llr;
    };
};


inline static long long simple_hash(const set<unsigned int>* const& ss) {
    unsigned int seed = 131;
    unsigned int hval = 0;
    for (auto iter = ss->cbegin(); iter != ss->cend(); ++iter)
        hval = hval * seed + (*iter);
    return hval & 0x7FFFFFFF;
};

inline static long long simple_hash(const list<time_t>* const& ss) {
    unsigned int seed = 131;
    unsigned int hval = 0;
    for (auto iter = ss->cbegin(); iter != ss->cend(); ++iter)
        hval = hval * seed + (*iter);
    return hval & 0x7FFFFFFF;
};

inline static long long simple_hash(const long long& v1, const long long& v2) {
    unsigned int seed = 131;
    unsigned int hval = 0;
    hval = hval * seed + v1;
    hval = hval * seed + v2;
    return hval & 0x7FFFFFFF;
};


typedef unordered_map<IdType, Circle*> S_Circles;
typedef map<time_t, S_Circles*> TS_Circles;

typedef map<time_t, Circle*> T_Circles;
typedef unordered_map<IdType, T_Circles*> ST_Circles;

typedef unordered_map<IdType, Timespan*> T_Timespans;

typedef vector<Cluster*> Clusters;

typedef unordered_map<IdType, set<unsigned int>*> LocssMap;
typedef unordered_map<IdType, list<time_t>*> UtmssMap;

void destroy(S_Circles*&);
void destroy(T_Circles*&);
void destroy(TS_Circles*&, const bool&);
void destroy(ST_Circles*&, const bool&);
void destroy(T_Timespans*&);
void destroy(Clusters*&);
void destroy(LocssMap*&);
void destroy(UtmssMap*&);

#include "cluster.cpp"
#endif
