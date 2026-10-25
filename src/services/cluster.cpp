#include "cluster.h"


Circle::Circle(set<unsigned int>* const& ss, const size_t& cz, const size_t& bz) {
    this->id = simple_hash(ss);
    this->cz = cz;
    this->bz = bz;
}


Timespan::Timespan(list<time_t>* const& ss, const size_t& cz, const size_t& bz, const size_t& hz) {
    this->id = (ss->size() > 1) ? simple_hash(ss->front(), ss->back()) : simple_hash(ss);
    this->cz = cz;
    this->bz = bz;
    this->hz = hz;
}

Timespan::Timespan(
    const time_t& bgnutm, const time_t& endutm, const size_t& cz, const size_t& bz, const size_t& hz) {
    this->id = (bgnutm == endutm) ? simple_hash(0, bgnutm) : simple_hash(bgnutm, endutm);
    this->cz = cz;
    this->bz = bz;
    this->hz = hz;
}


Cluster::Cluster(
    const IdType& sid,
    const IdType& tid,
    const size_t& cz,  // Total cases in window
    const size_t& cg,  // Total cases in global zone
    const size_t& bz,  // Average population in window by time
    const size_t& bg,  // Average population in global zone by time
    const size_t& hz,  // Number of timespans in temporal window
    const size_t& hg   // Total number of timespans that exist in the whole temporal global zone
) {
    this->id = simple_hash(sid, tid);
    this->sid = sid;
    this->tid = tid;
    this->cz = cz;
    this->bz = bz;
    this->hz = hz;
    this->mz = calc_mz(cg, bz, bg, hz, hg);
    this->llr = calc_llr(cz, cg, this->mz);
}


Cluster::Cluster(
    const IdType& sid,
    const IdType& tid,
    const size_t& cz,
    const size_t& bz,
    const size_t& hz
) {
    this->id = simple_hash(sid, tid);
    this->sid = sid;
    this->tid = tid;
    this->cz = cz;
    this->bz = bz;
    this->hz = hz;
}


void destroy(S_Circles*& s_circles) {
    if (s_circles == nullptr)
        return;
    
    for (auto iter = s_circles->begin(); iter != s_circles->end(); ++iter) {
        auto &circle = iter->second;
        if (circle == nullptr)
            continue;
        delete circle;
        circle = nullptr;
    }
    s_circles->clear();
    delete s_circles;
    s_circles = nullptr;
}

void destroy(T_Circles*& t_circles) {
    if (t_circles == nullptr)
        return;

    for (auto iter = t_circles->begin(); iter != t_circles->end(); ++iter) {
        auto &circle = iter->second;
        if (circle == nullptr)
            continue;
        delete circle;
        circle = nullptr;
    }
    t_circles->clear();
    delete t_circles;
    t_circles = nullptr;
}

void destroy(ST_Circles*& st_circles, const bool& ignore_circles = false) {
    if (st_circles == nullptr)
        return;

    for (auto iter = st_circles->begin(); iter != st_circles->end(); ++iter) {
        auto &t_circles = iter->second;
        if (!ignore_circles)
            destroy(t_circles);
        else if (t_circles) {
            t_circles->clear();
            delete t_circles;
            t_circles = nullptr;
        }
    }
    st_circles->clear();
    delete st_circles;
    st_circles = nullptr;
}

void destroy(TS_Circles*& ts_circles, const bool& ignore_circles = false) {
    if (ts_circles == nullptr)
        return;

    for (auto iter = ts_circles->begin(); iter != ts_circles->end(); ++iter) {
        auto &s_circles = iter->second;
        if (!ignore_circles)
            destroy(s_circles);
        else if (s_circles) {
            s_circles->clear();
            delete s_circles;
            s_circles = nullptr;
        }
    }
    ts_circles->clear();
    delete ts_circles;
    ts_circles = nullptr;
}

void destroy(T_Timespans*& t_timespan) {
    if (t_timespan == nullptr)
        return;

    for (auto iter = t_timespan->begin(); iter != t_timespan->end(); ++iter) {
        auto &timespan = iter->second;
        if (timespan == nullptr)
            continue;
        delete timespan;
        timespan = nullptr;
    }
    t_timespan->clear();
    delete t_timespan;
    t_timespan = nullptr;
}

void destroy(Clusters*& clusters) {
    if (clusters == nullptr)
        return;

    clusters->clear();
    delete clusters;
    clusters = nullptr;
}

void destroy(LocssMap*& locssmap) {
    if (locssmap == nullptr)
        return;

    for (auto iter = locssmap->begin(); iter != locssmap->end(); ++iter) {
        auto &locss = iter->second;
        if (locss == nullptr)
            continue;
        locss->clear();
        delete locss;
        locss = nullptr;
    }
    locssmap->clear();
    delete locssmap;
    locssmap = nullptr;
}

void destroy(UtmssMap*& utmssmap) {
    if (utmssmap == nullptr)
        return;

    for (auto iter = utmssmap->begin(); iter != utmssmap->end(); ++iter) {
        auto &utmss = iter->second;
        if (utmss == nullptr)
            continue;
        utmss->clear();
        delete utmss;
        utmss = nullptr;
    }
    utmssmap->clear();
    delete utmssmap;
    utmssmap = nullptr;
}
