#include "scanning.h"

/*
 * Scanning pipeline overview:
 *
 * tscan:  Generates temporal windows and computes temporal cluster statistics.
 * sscan:  Generates spatial windows and computes spatial cluster statistics.
 * satscan: Combines spatial and temporal scanning to evaluate space-time windows.
 *
 * After candidate generation, clusters are evaluated using log-likelihood
 * ratios (LLRs), assigned Monte Carlo p-values, ranked, and filtered to
 * produce the final result set.
 */


void tscan::temporal_scanning(
    T_Timespans*& timespans, UtmssMap*& utmssmap,
    PopData* const& pop, CasData* const& cas, AddrTable* const& addr_table,
    const char*& st_dtm, const char*& nd_dtm, const char*& temporal_step,
    const double& termrate = 0.5, const unsigned int& parallel_num = 1) {
    
    const time_t st_utm = Xtm(st_dtm, YMD_DASH).to_utm();
    const time_t nd_utm = Xtm(nd_dtm, YMD_DASH).to_utm();
    auto *data_pair = Xtm::from_rule(temporal_step);
    auto step_u = data_pair->get_first();
    auto step_a = abs(data_pair->get_second());

    auto &cascols = cas->cols, &popcols = pop->cols;
    auto &casvals = cas->vals, &popvals = pop->vals;

    size_t bg = aggregate_population(pop->vals), bz;
    size_t cg = aggregate_population(cas->vals), cz;
    size_t hg = calculate_time_step_index(nd_utm, st_dtm, nd_dtm, temporal_step) + 1, hz;
    time_t ag = encode_t(nd_utm, st_dtm, nd_dtm, temporal_step)
              - encode_t(st_utm, st_dtm, nd_dtm, temporal_step) + 1, az;
    cout << "AG = " << ag << endl;

    list<time_t>* utmss = new list<time_t>();
    Timespan* timespan;
    DemCols::iterator cascols_iter, popcols_iter;

    for (auto iter = addr_table->cbegin(); iter != addr_table->cend(); ++iter) {
        auto& bgnaddr = iter->first;
        auto& bgnutm = *(iter->second->cbegin());

        for (auto jter = iter; jter != addr_table->cend(); ++jter) {
            auto& endaddr = jter->first;
            auto& endutm = *(jter->second->crbegin());
            
            cz = 0;
            bz = 0;

            // Convert the window endpoints to zero-based temporal-step indices.
            // The +1 makes the window length inclusive of both endpoints.
            size_t bgn_idx = calculate_time_step_index(bgnutm, st_dtm, nd_dtm, temporal_step);
            size_t end_idx = calculate_time_step_index(endutm, st_dtm, nd_dtm, temporal_step);
            hz = end_idx - bgn_idx + 1;

            // Windows are extended in chronological order. Once the maximum
            // permitted duration is exceeded, later endpoints cannot produce
            // a shorter window, so the endpoint loop terminates.
            if (hz > static_cast<size_t>(hg * termrate))
                break;

            for (auto addr_iter = iter; addr_iter != std::next(jter); ++addr_iter) {
                const time_t addrx = addr_iter->first;

                auto cas_col_iter = cascols->find(addrx);
                if (cas_col_iter != cascols->end()) {
                    auto idx = cas_col_iter->second->begin()->second;
                    auto val = casvals->at(idx)[0];
                    cz += val;
                }

                for (auto utm_iter = addr_iter->second->cbegin(); utm_iter != addr_iter->second->cend(); ++utm_iter) {
                    const time_t utm = *utm_iter;
                    auto pop_col_iter = popcols->find(utm);
                    if (pop_col_iter != popcols->end()) {
                        auto idx = pop_col_iter->second->begin()->second;
                        bz += popvals->at(idx)[0];
                    }
                }
            }

            // if (cz * 2 <= cg)
            //     continue;

            timespan = new Timespan(bgnaddr, endaddr, cz, bz, hz);
            timespans->insert(make_pair(timespan->id, timespan));

            if (utmssmap != nullptr) {
                utmssmap->insert(make_pair(timespan->id, new list<time_t>{bgnutm, endutm}));
            }
        }
    }
}
 

void tscan::update_timespans(
    Clusters*& clusters, T_Timespans* const& timespans,
    const size_t& cg, const size_t& bg, const size_t& hg) {
    
    Cluster *cluster;
    for (auto iter = timespans->cbegin(); iter != timespans->cend(); ++iter) {
        auto tid = iter->first;
        auto &timespan = iter->second;
        auto &cz = timespan->cz;
        auto &bz = timespan->bz;
        auto &hz = timespan->hz;
        
        // here define some constant values as default to make the `circle` act like a `cluster`
        // cluster = new Cluster(0, tid, cz, cg, 1, 1, hz, hg);
        cluster = new Cluster(0, tid, cz, cg, bz, bg, hz, hz);
        clusters->push_back(cluster);
    }
}


void tscan::postprocess_clusters(
    Clusters*& mlc, T_Timespans* const& timespans,
    const double& cg, const double& bg, const double& hg, const char*& temporal_step) {
    
    for (auto iter = mlc->begin(); iter != mlc->end(); ++iter) {
        auto* cluster = *iter;

        cluster->rr = Cluster::calc_rr(cluster->cz, cg, cluster->mz);
        cluster->oer = Cluster::calc_oer(cluster->cz, cluster->mz);
        cluster->ann = static_cast<double>(cluster->cz) / static_cast<double>(cluster->bz) * 100000.0;
    }
}


void sscan::spatial_scanning(
    S_Circles*& circles, LocssMap*& locssmap,
    GeoData* const& geo, PopData* const& pop, CasData* const& cas, RadTable* const& rad_table,
    const bool& termcond = true, const double& termrate = 0.5,
    const unsigned int& parallel_num = 1) {

    auto &locations = geo->locations;
    auto &coordinates = geo->coordinates;
    auto &popvals = pop->vals, &casvals = cas->vals;
    auto &poprows = pop->rows, &casrows = cas->rows;
    
    // calculate both local population `Bz` and lcoal cases `Cz` in the same iterations,
    // because it means only one datetime in `Pop` but multiple datetimes in `Cas`, in this case,
    // we can calculate the `Bg` and figure out spatial circles at the beginning in once time
    size_t bg = aggregate_population(pop->vals), bz, bs;
    size_t cg = aggregate_population(cas->vals), cz, cs;
    // and meanwhile calculate total number of locations `Lg` for deciding when to terminate
    // circle expansian when the `-lnd` is selected as terminating condition;
    // note that here using length of `locations` or `coordinates` in `Geo`,
    // or length of `border_seq` in `rad_table` as `Lg` will get the same value
    unsigned int lg = geo->locations->size(), lz;

    set<unsigned int>* locss = new set<unsigned int>();
    Circle* circle;

    // loop i iterates each location as a centre of the circle window;
    for (auto iter = rad_table->begin(); iter != rad_table->end(); ++iter) {
        auto &centre = iter->first;
        auto &border_seq = iter->second;
        bz = 0;
        cz = 0;
        lz = 0;

        // loop j iterates each border of the centre in ascending order
        //   to accumulately add population of locations in the radiated range;
        //  (and it will break out of this loop once satisfying the terminal condition)
        for (auto jter = border_seq->cbegin(); jter != border_seq->cend(); ++jter) {
            auto &step = jter->first;
            auto &borders = jter->second;
            bs = 0;
            cs = 0;
            // Locations at the same distance from the centre form one border group.
            // Add the entire group together so that equal-distance locations are
            // included or excluded as a unit when constructing a spatial window.
            for (auto kter = borders->cbegin(); kter != borders->cend(); ++kter) {
                auto &border = *kter;
                auto &popcol = poprows->find(border)->second;
                auto popval = popvals->at(popcol->begin()->second)[0];
                bs += popval;

                // auto &cascol = casrows->find(border)->second;
                // cout << "3" << endl;
                // auto casval = casvals->at(cascol->begin()->second)[0];
                // cout << "4" << endl;
                // cs += casval;

                double casval = 0.0;
                auto cas_iter = casrows->find(border);
                if (cas_iter != casrows->end()) {
                    auto* cascol = cas_iter->second;
                    if (cascol == nullptr || cascol->empty()) throw runtime_error("Empty case column: " + border);
                    auto idx = cascol->begin()->second;
                    if (idx >= casvals->size()) throw runtime_error("Case row index out of range: " + border);
                    const auto& row = casvals->at(idx);
                    if (row == nullptr) throw runtime_error("Empty case value row: " + border);
                    casval = row[0];
                }
                cs += casval;
            }
            bz += bs;
            cz += cs;
            ++lz;
            
            // `termcond` is true for `use_popmaxend`, false for `use_locmaxend`
            if ((termcond && bz > ((double) bg / 2)) || (!termcond && lz > ((double) lg / 2)))
                break;

            for (auto kter = borders->cbegin(); kter != borders->cend(); ++kter)
                locss->insert(locations->find(*kter)->second);

            circle = new Circle(locss, cz, bz);
            circle->centre = locations->find(centre)->second;
            circle->border = locations->find(borders->back())->second;
            circles->insert(pair<IdType, Circle*>(circle->id, circle));
            // * if `locssmap` is required to be returned (not null),
            //   then make another copy on `locss` and set it into `locssmap`
            locssmap->insert(pair<IdType, set<unsigned int>*>(
                circle->id, new set<unsigned int>(locss->cbegin(), locss->cend())));
        }

        locss->clear();
    }

    // free memory after copying all template `locss` into `Circle`s
    // that will be actually used in next steps
    delete locss;
    locss = nullptr;
}


void sscan::update_circles(
    Clusters*& clusters, S_Circles* const& circles, const size_t& cg, const size_t& bg) {
    
    Cluster *cluster;
    size_t cz;
    size_t bz;
    for (auto iter = circles->cbegin(); iter != circles->cend(); ++iter) {
        auto sid = iter->first;
        auto &circle = iter->second;
        cz = circle->cz;
        bz = circle->bz;
        
        // here define some constant values as default to make the `circle` act like a `cluster`
        cluster = new Cluster(sid, 0, cz, cg, bz, bg, 1, 1);
        clusters->push_back(cluster);
    }
}


void sscan::postprocess_clusters(
    Clusters*& mlc, S_Circles* const& circles, GeoData* const& geo, const bool& use_geo_coord) {
    
    auto &coordinates = geo->coordinates;
    for (auto iter = mlc->cbegin(); iter != mlc->cend(); ++iter) {
        auto &cluster = *iter;
        auto &circle = circles->find(cluster->sid)->second;
        auto &ccoord = coordinates->at(circle->centre);
        auto &bcoord = coordinates->at(circle->border);
        (*iter)->radius = use_geo_coord ? geographic_distance(ccoord, bcoord) : euclidean_distance(ccoord, bcoord);
        (*iter)->centre = ccoord->loc;
    }
}


void sscan::postprocess_clusters(
    Clusters*& mlc, S_Circles* const& circles, GeoData* const& geo,
    const double& cg, const double& bg, const double& hg, const bool& use_geo_coord) {
    
    auto &coordinates = geo->coordinates;
    for (auto iter = mlc->cbegin(); iter != mlc->cend(); ++iter) {
        auto &cluster = *iter;
        auto &circle = circles->find(cluster->sid)->second;
        auto &ccoord = coordinates->at(circle->centre);
        auto &bcoord = coordinates->at(circle->border);
        (*iter)->radius = use_geo_coord ? geographic_distance(ccoord, bcoord) : euclidean_distance(ccoord, bcoord);
        (*iter)->centre = ccoord->loc;
        (*iter)->mz = Cluster::calc_mz(cg, cluster->bz, bg, 1, 1);
        (*iter)->rr = Cluster::calc_rr(cluster->cz, cg, cluster->mz);
        (*iter)->oer = Cluster::calc_oer(cluster->cz, cluster->mz);
        // (*iter)->ann = Cluster::calc_ann(cluster->cz, cluster->bz, KULLDORFF_YDAY);
        (*iter)->ann = static_cast<double>(cluster->cz) / static_cast<double>(cluster->bz) / hg * 100000.0;
    }
}


void satscan::spatial_scanning_impl(
    S_Circles*& circles, const time_t& utm,
    GeoData* const& geo, CasData* const& cas, LocssMap* const& locssmap, S_Circles* const& template_circles) {

    auto &locations = geo->locations;
    auto &coordinates = geo->coordinates;
    auto &casvals = cas->vals;
    auto &cas_col = cas->cols->find(utm)->second;
    DemCol::iterator cas_col_iter;

    Circle* circle;
    size_t cz, cg;
    auto idx = 0;
    for (auto iter = template_circles->cbegin(); iter != template_circles->cend(); ++iter) {
        auto &template_circle = iter->second;
        auto &locss = locssmap->find(template_circle->id)->second;
        auto &bz = template_circle->bz;

        cz = 0;
        for (auto jter = locss->cbegin(); jter != locss->cend(); ++jter) {
            if ((cas_col_iter = cas_col->find(coordinates->at(*jter)->loc)) == cas_col->end())
                continue;
            cz += casvals->at(cas_col_iter->second)[0];
        }

        // Retain only windows containing at least one observed case.
        // Zero-case windows cannot be reported as observed clusters.
        if (cz == 0)
            continue;

        // * note that here `cg` is not input into the new `Circle` or not even have to be calculated
        //   since `cg` will be a single value that equals to summary of all locations in all datetimes
        // the new `Circle` will not create or copy a new set,
        // or even reference it from `locss` in template `Circle`s,
        // so the template circles should be deleted after all `locss`s have been used
        circle = new Circle();
        circle->id = template_circle->id;
        circle->cz = cz;
        circle->bz = bz;
        circle->centre = template_circle->centre;
        circle->border = template_circle->border;
        circles->insert(pair<IdType, Circle*>(circle->id, circle));  
    }
}

void satscan::spatial_scanning_2d(
    TS_Circles*& timeseries, LocssMap*& locssmap,
    GeoData* const& geo, PopData* const& pop, CasData* const& cas, RadTable* const& rad_table,
    const PopulationExposureIndex& exposure_index,
    const bool& termcond = true, const double& termrate = 0.5, const unsigned int& parallel_num = 1) {

    auto &locations = geo->locations;
    auto &coordinates = geo->coordinates;
    auto &popvals = pop->vals;
    auto &poprows = pop->rows;
    
    // firstly calculate global population `Bg` and leave `Cg`s empty for further calculation,
    // because it means only one datetime in `Pop` but multiple datetimes in `Cas`, in this case,
    // we can calculate the `Bg` and figure out spatial circles at the beginning in once time
    double bg = exposure_index.average_total_population;
    double bz;
    // and meanwhile calculate total number of locations `Lg` for deciding when to terminate
    // circle expansian when the `-lnd` is selected as terminating condition;
    // note that here using length of `locations` or `coordinates` in `Geo`,
    // or length of `border_seq` in `rad_table` as `Lg` will get the same value
    unsigned int lg = geo->locations->size(), lz;

    auto template_circles = new S_Circles();
    set<unsigned int>* locss = new set<unsigned int>();
    Circle* template_circle;

    // loop i iterates each location as a centre of the circle window;
    for (auto iter = rad_table->begin(); iter != rad_table->end(); ++iter) {
        auto &centre = iter->first;
        auto &border_seq = iter->second;
        bz = 0;
        lz = 0;

        // loop j iterates each border of the centre in ascending order
        //   to accumulately add population of locations in the radiated range;
        //  (and it will break out of this loop once satisfying the terminal condition)
        for (auto jter = border_seq->cbegin(); jter != border_seq->cend(); ++jter) {
            auto &step = jter->first;
            auto &borders = jter->second;
            double bs = 0;
            // loop k is to make sure that populations in multiple borders
            //   with same distances to the centre will be counted simultaneously;
            //  (which in most cases would iterate only once time)
            for (auto kter = borders->cbegin(); kter != borders->cend(); ++kter) {
                auto &border = *kter;
                // auto popcol = poprows->find(border)->second;
                // auto popval = popvals->at(popcol->begin()->second)[0];
                // bs += popval;
                const unsigned int location_id = locations->find(border)->second;
                bs += exposure_index.average_population.at(location_id);
            }
            bz += bs;
            ++lz;
            
            // `termcond` is true for `use_popmaxend`, false for `use_locmaxend`
            if ((termcond && bz > ((double) bg / 2)) || (!termcond && lz > ((double) lg / 2)))
                break;

            for (auto kter = borders->cbegin(); kter != borders->cend(); ++kter)
                locss->insert(locations->find(*kter)->second);

            // Construct spatial window templates once using the population
            // exposure aggregated across time. The same spatial windows are then
            // evaluated against case counts at each timestamp, avoiding repeated
            // construction of the spatial search windows for every timestamp.
            template_circle = new Circle(locss, 0, bz);
            template_circle->centre = locations->find(centre)->second;
            template_circle->border = locations->find(borders->back())->second;
            template_circles->insert(pair<IdType, Circle*>(template_circle->id, template_circle));
            // * if `locssmap` is required to be returned (not null),
            //   then make another copy on `locss` and set it into `locssmap`
            locssmap->insert(pair<IdType, set<unsigned int>*>(
                template_circle->id, new set<unsigned int>(locss->cbegin(), locss->cend())));
        }

        locss->clear();
    }

    // free memory after copying all template `locss` into `Circle`s that will be actually used in next steps
    delete locss;
    locss = nullptr;

    auto &cascols = cas->cols;
    auto length = cascols->size();

    // after figuring out global circles, calculate `Cg`s and `Cz`s for circles in each datetime;
    // note that the radiation part above did not deal with process stuff but left it here,
    // we may consider to implement the multi-process function for radiation part in further optimization
    S_Circles* circles;
    unordered_map<IdType, TS_Circles*>::iterator kter;

    if (parallel_num <= 1) {
        for (auto iter = cascols->cbegin(); iter != cascols->cend(); ++iter) {
            auto &utm = iter->first;
            auto &cascol = iter->second;
            circles = new S_Circles();
            spatial_scanning_impl(circles, utm, geo, cas, locssmap, template_circles);
            timeseries->insert(pair<time_t, S_Circles*>(utm, circles));
        }
        return;
    }

    cout << "Start spatial scanning with `parallel_num` > 1" << endl;
    auto utmss = new time_t[length];
    unsigned int i = 0;
    for (auto iter = cascols->cbegin(); iter != cascols->cend(); ++iter, ++i)
        utmss[i] = iter->first;
    time_t** utmsss = new time_t*[parallel_num];
    approx_bisect(utmsss, utmss);
    cout << "Completed `approx_bisect` process" << endl;

    TS_Circles*** resultsss = new TS_Circles**[parallel_num];
    for (unsigned int i = 0; i < parallel_num; ++i) {
        auto slice_size = get_size(utmsss[i]);
        TS_Circles** resultss = new TS_Circles*[slice_size];
        for (unsigned int j = 0; j < slice_size; ++j)
            resultss[j] = new TS_Circles();
        resultsss[i] = resultss;
    }

    omp_set_num_threads(parallel_num);
#pragma omp parallel for
    for (unsigned int i = 0; i < parallel_num; ++i) {
        auto &slice = utmsss[i];
        auto &resultss = resultsss[i];
        auto slice_size = get_size(slice);

        for (auto j = 0; j < slice_size; ++j) {
            auto &utm = slice[j];
            auto &results = resultss[j];
            cout << "  " << j << " / " << slice_size << endl;
            circles = new S_Circles();
            spatial_scanning_impl(circles, utm, geo, cas, locssmap, template_circles);
            results->insert(pair<time_t, S_Circles*>(utm, circles));
            cout << "Completed insert `utm` " << utm << " into `st_circles`" << endl;
        }
        cout << "Thread " << omp_get_thread_num() << ": ";
        cout << i + 1 << " / " << parallel_num << endl;
    }
    cout << "Completed spatial scanning process" << endl;

    for (unsigned int i = 0; i < parallel_num; ++i) {
        auto &resultss = resultsss[i];
        auto slice_size = get_size(resultss);
        for (unsigned int j = 0; j < slice_size; ++j) {
            auto &results = resultss[j];
            timeseries->insert(results->begin(), results->end());
        }
    }

    // free memory for `utmsss`
    for (auto i = 0; i < parallel_num; ++i) {
        auto& slice = utmsss[i];
        delete slice;
        slice = nullptr;
    }
    delete utmsss;
    utmsss = nullptr;

    // free memory for `utmss`
    delete utmss;
    utmss = nullptr;

    // free memory for `template_circles` whenever an instance of `locssmap` is input
    // to store `locss` for further use or not
    destroy(template_circles);
}


// Transpose the index structure from timestamp -> circle ID -> Circle
// to circle ID -> timestamp -> Circle, allowing temporal scanning to
// extend each spatial window across consecutive timestamps.
// Circle objects are shared by pointer rather than copied.
void satscan::transpose_clusters(ST_Circles*&s_timeseries_s, TS_Circles* const& timeseries) {
    ST_Circles::iterator kter;

    for (auto iter = timeseries->cbegin(); iter != timeseries->cend(); ++iter) {
        auto &utm = iter->first;
        auto &circles = iter->second;
        for (auto jter = circles->cbegin(); jter != circles->cend(); ++jter) {
            auto &circle_id = jter->first;
            // cout << "inserting " << Xtm(utm).to_dtm(YMD_DASH) << " " << circle_id << endl;
            Circle *circle = &*(jter->second);
            // cout << (s_timeseries_s->find(circle_id) == s_timeseries_s->end()) << endl;
            auto *t_circles = (
                (kter = s_timeseries_s->find(circle_id)) == s_timeseries_s->end()
            ) ? new T_Circles() : kter->second;
            t_circles->insert(pair<time_t, Circle*>(utm, circle));
            s_timeseries_s->insert(pair<IdType, T_Circles*>(circle_id, t_circles));
        }
    }
}


void satscan::temporal_scanning_impl(
    Clusters*& clusters, UtmssMap*& utmssmap,
    const IdType& sid, T_Circles* const& t_circles, AddrTable* const& addr_table,
    LocssMap* const& locssmap, const PopulationExposureIndex& exposure_index,
    const size_t& cg, const size_t& bg, const size_t& hg,
    const char*& st_dtm, const char*& nd_dtm, const char*& temporal_step,
    const double& termrate = 0.5) {
    
    auto tricky_table = unordered_map<IdType, Cluster*>();
    unordered_map<IdType, Cluster*>::iterator tt_iter;
    AddrTable::iterator at_iter;
    // time_t bgnutm, endutm;
    // size_t cz, bz, hz;
    // Cluster* cluster;
    // Timespan timespan;
    time_t bgnutm, endutm;
    size_t cz, bz, hz;
    double ez, mz, llr;
    size_t start_index, end_index;
    
    Cluster* cluster;
    Timespan timespan;

    //           T_CIRCLES
    //       addr,   addr,   ..., addr
    // sid   circle, circle, ..., circle
    // sid   circle, ------, ..., circle
    // ...                   ...
    // sid   circle, circle, ..., ------
    for (auto iter = t_circles->cbegin(); iter != t_circles->cend(); ++iter) {
        auto &bgnaddr = iter->first;
        if ((at_iter = addr_table->find(bgnaddr)) == addr_table->end())
            continue;
        // bgnutm = *(at_iter->second->cbegin());
        const size_t start_index = exposure_index.time_index.at(bgnaddr);
        cz = 0;

        for (auto jter = iter; jter != t_circles->cend(); ++jter) {
            auto &endaddr = jter->first;
            if ((at_iter = addr_table->find(endaddr)) == addr_table->end())
                continue;
            const size_t end_index = exposure_index.time_index.at(endaddr);
            hz = end_index - start_index + 1;
            
            bgnutm = *(addr_table->at(bgnaddr)->cbegin());
            endutm = *(addr_table->at(endaddr)->crbegin());

            // cout << "  " << sid << ": " << Xtm(bgnutm).to_dtm(YMD_DASH);
            // cout << " ~ " << Xtm(endutm).to_dtm(YMD_DASH);

            if (hz > static_cast<double>(hg) * termrate)
                break;

            auto &circle = jter->second;
            cz += circle->cz;
            bz = circle->bz;

            const auto loc_iter = locssmap->find(sid);
            
            if (loc_iter == locssmap->end() || loc_iter->second == nullptr)
                throw std::runtime_error("Missing locations for spatial window");
            
            // Compute the population exposure inside this space-time window.
            // Scale the global case count by the window's share of total exposure
            // to obtain the expected case count used by the likelihood-ratio test.
            ez = population_exposure_in_window(exposure_index, *loc_iter->second, start_index, end_index);
            
            if (exposure_index.total_exposure <= 0.0)
                throw std::runtime_error("Total population exposure must be positive");
            
            mz = static_cast<double>(cg) * ez / exposure_index.total_exposure;

            llr = Cluster::calc_llr(cz, cg, mz);
            
            if (!std::isfinite(llr))
                continue;

            time_t tid = simple_hash(bgnaddr, endaddr);
            if (utmssmap != nullptr)
                utmssmap->insert(pair<IdType, list<time_t>*>(tid, new list<time_t>{bgnutm, endutm}));

            if ((tt_iter = tricky_table.find(sid)) == tricky_table.end()) {
                cluster = new Cluster(sid, tid, cz, bz, hz);
                cluster->mz = mz;
                cluster->llr = llr;
                tricky_table.insert(pair<IdType, Cluster*>(sid, cluster));
                continue;
            }
            
            // cout << ", cz/cg = " << cz << " / " << cg;
            // cout << ", bz/bg = " << bz << " / " << bg;
            // cout << ", hz/hg = " << hz << " / " << hg;
            // cout << ", mz = " << Cluster::calc_mz(cg, bz, bg, hz, hg) << ", LLR = " << llr << endl;

            // Keep only the highest-LLR temporal window for each spatial window ID.
            // This reduces multiple temporal candidates for the same spatial window
            // to a single candidate before global ranking and Monte Carlo evaluation.
            auto*& ori_cluster = tt_iter->second;
            if (ori_cluster->llr < llr) {
                delete ori_cluster;
                ori_cluster = new Cluster(sid, tid, cz, bz, hz);
                ori_cluster->mz = mz;
                ori_cluster->llr = llr;
            }
        }
    }

    for (auto iter = tricky_table.cbegin(); iter != tricky_table.cend(); ++iter) {
        auto &sid = iter->first;
        auto &cluster = iter->second;
        clusters->push_back(cluster);
    }
}


void satscan::temporal_scanning(
    Clusters*& clusters, UtmssMap*& utmssmap,
    ST_Circles* const& st_circles, AddrTable* const& addr_table,
    LocssMap* const& locssmap, const PopulationExposureIndex& exposure_index,
    const size_t& cg, const size_t& bg, const size_t& hg,
    const char*& st_dtm, const char*& nd_dtm, const char*& temporal_step,
    const double& termrate = 0.5, const unsigned int& parallel_num = 1) {

    if (parallel_num <= 1) {
        for (auto iter = st_circles->cbegin(); iter != st_circles->cend(); ++iter) {
            auto &sid = iter->first;
            auto &t_circles = iter->second;
            temporal_scanning_impl(
                clusters, utmssmap, sid, t_circles, addr_table, locssmap, exposure_index,
                cg, bg, hg, st_dtm, nd_dtm, temporal_step, termrate);
        }
        return;
    }

    cout << "Start temporal scanning with `parallel_num` > 1" << endl;
    IdType* ss = new IdType[st_circles->size()];
    IdType** sss = new IdType*[parallel_num];
    
    unsigned int idx = 0;
    for (auto iter = st_circles->cbegin(); iter != st_circles->cend(); ++iter)
        ss[idx++] = iter->first;

    approx_bisect(sss, ss);
    cout << "Completed `approx_bisect` process" << endl;

    Clusters*** resultsss = new Clusters**[parallel_num];
    for (unsigned int i = 0; i < parallel_num; ++i) {
        auto slice_size = get_size(sss[i]);
        Clusters** resultss = new Clusters*[slice_size];
        for (unsigned int j = 0; j < slice_size; ++j)
            resultss[j] = new Clusters();
        resultsss[i] = resultss;
    }

omp_set_num_threads(parallel_num);
#pragma omp parallel for
    for (unsigned int i = 0; i < parallel_num; ++i) {
        auto &slice = sss[i];
        auto &resultss = resultsss[i];
        auto slice_size = get_size(slice);

        for (auto j = 0; j < slice_size; ++j) {
            auto &sid = slice[j];
            auto &t_circles = st_circles->find(sid)->second;
            auto &results = resultss[j];
            cout << "  " << j << " / " << slice_size << endl;
            temporal_scanning_impl(
                results, utmssmap, sid, t_circles, addr_table, locssmap, exposure_index,
                cg, bg, hg, st_dtm, nd_dtm, temporal_step, termrate);
            // temporal_scanning_impl(clusters, utmssmap, sid, t_circles, addr_table, cg, bg, hg, termrate);
            cout << "Completed insert `sid` " << sid << " into `clusters`" << endl;
        }
        cout << "Thread " << omp_get_thread_num() << ": ";
        cout << i + 1 << " / " << parallel_num << endl;
    }
    cout << "Completed temporal scanning process" << endl;

    for (unsigned int i = 0; i < parallel_num; ++i) {
        auto &resultss = resultsss[i];
        auto slice_size = get_size(resultss);
        for (unsigned int j = 0; j < slice_size; ++j) {
            auto &results = resultss[j];
            clusters->insert(clusters->end(), results->begin(), results->end());
        }
    }

    cout << "free memory" << endl;
    // free memory for `sss` and `resultsss`
    for (auto i = 0; i < parallel_num; ++i) {
        auto &slice = sss[i];
        auto &resultss = resultsss[i];
        delete resultss;
        resultss = nullptr;
        delete slice;
        slice = nullptr;
    }
    delete sss;
    sss = nullptr;

    // free memory for `ss`
    delete ss;
    ss = nullptr;

    cout << "done" << endl;
}


void satscan::postprocess_clusters(
    Clusters*& mlc, ST_Circles* const& st_circles, GeoData* const& geo, const bool& use_geo_coord) {
    
    auto &coordinates = geo->coordinates;
    for (auto iter = mlc->cbegin(); iter != mlc->cend(); ++iter) {
        auto &cluster = *iter;
        auto &circle = st_circles->find(cluster->sid)->second->cbegin()->second;
        auto &ccoord = coordinates->at(circle->centre);
        auto &bcoord = coordinates->at(circle->border);
        (*iter)->radius = use_geo_coord ? geographic_distance(ccoord, bcoord)
                                        : euclidean_distance(ccoord, bcoord);
        (*iter)->centre = ccoord->loc;
    }
}

void satscan::postprocess_clusters(
    Clusters*& mlc, ST_Circles* const& st_circles, GeoData* const& geo,
    const double& cg, const double& bg, const double& hg, const bool& use_geo_coord, const char*& temporal_step) {
    
    auto &coordinates = geo->coordinates;
    auto* datapair = Xtm::from_rule(temporal_step);
    const char unit = datapair->get_first();
    delete datapair;

    double fac;
    switch (unit) {
        case 'd':
            fac = 1;
            break;
        case 'm':
            fac = 30;
            break;
        case 'Y':
            fac = KULLDORFF_YDAY;
            break;
        default:
            throw std::invalid_argument("Unsupported temporal step unit");
    }

    for (auto iter = mlc->cbegin(); iter != mlc->cend(); ++iter) {
        auto &cluster = *iter;
        auto &circle = st_circles->find(cluster->sid)->second->cbegin()->second;
        auto &ccoord = coordinates->at(circle->centre);
        auto &bcoord = coordinates->at(circle->border);

        (*iter)->radius = use_geo_coord ? geographic_distance(ccoord, bcoord)
                                        : euclidean_distance(ccoord, bcoord);
        (*iter)->centre = ccoord->loc;
        // (*iter)->mz = Cluster::calc_mz(cg, cluster->bz, bg, cluster->hz, hg);
        (*iter)->rr = Cluster::calc_rr(cluster->cz, cg, cluster->mz);
        (*iter)->oer = Cluster::calc_oer(cluster->cz, cluster->mz);
        (*iter)->ann = Cluster::calc_ann(cluster->cz, cluster->bz, cluster->hz * fac);
    }
}



void simulation(
    double*& simvals, vector<Cluster*>* const& clusters,
    const size_t& cg, const size_t& bg, const size_t& hg, const size_t& n = 999) {
    // this is the implementation in another way, that generate poisson random series for
    // clusters directively; since it equals to compressing the original 3D matrix on its 
    // `loc` and `utm` dimensions and integrating them to `C` (cluster), only valuse on Z-axis
    // will remain and the cube collapse to a rectangular which will be like:
    //
    //  ^ Z
    //  |
    // 999 -cz--cz--cz-- ... --cz
    //  |   |   |   |          |
    // ...        ...        ...
    //  |
    //  3 -cz--cz--cz-- ... --cz
    //  |   |   |   |          |
    //  2 -cz--cz--cz-- ... --cz
    //  |   |   |   |          |
    //  1 -cz--cz--cz-- ... --cz
    //  |   |   |   |          |  U
    //  0--C1--C2--C3-- ... --Cm-->
    //
    // there are only two dimensions now, with
    // its horizontal U-axis represents clusteres and its width equals to number of clusters,
    // while its vertical Z-axis still represents simulations whose height equals to number
    // of monte carlo replication times
    double *simss;
    double simval;
    unsigned int idx = 0;
    auto length = clusters->size();
    // loop for `clusters` X
    for (auto iter = clusters->cbegin(); iter != clusters->cend(); ++iter) {
        auto &cluster = *iter;
        auto &bz = cluster->bz;
        auto &hz = cluster->hz;
        simss = new double[n];

        // For each candidate cluster, generate Poisson case-count replicates
        // using its expected count as the rate, then calculate the corresponding
        // simulated LLRs. For each replicate, retain the maximum LLR observed
        // across candidates in simvals, producing a replicate-level maximum
        // statistic for comparison with observed cluster LLRs.

        const double lam = cluster->mz;  // Cluster::calc_mz(cg, bz, bg, hz, hg);
        random_poisson(simss, lam, n);

        // loop for `simss` 999
        for (auto i = 0; i < n; ++i) {
            simval = Cluster::calc_llr(simss[i], cg, lam);
            if (simvals[i] < simval)
                simvals[i] = simval;
        }
        delete[] simss;
        simss = nullptr;

        if (idx++ % 10000 == 0)
            cout << idx << " / " << length << " done" << endl;
    }
}


void rank_clusters(Clusters*& clusters, double* const& simvals) {
    // Estimate the Monte Carlo p-value by counting simulated maximum
    // statistics at least as large as the observed cluster LLR.
    // The +1 correction in both numerator and denominator avoids a zero
    // estimated p-value when no simulated statistic exceeds the observation.

    const size_t n = get_size(simvals);

    for (auto* cluster : *clusters) {
        size_t exceed = 0;

        for (size_t i = 0; i < n; ++i) {
            if (simvals[i] >= cluster->llr)
                ++exceed;
        }

        cluster->pval = static_cast<double>(exceed + 1) / static_cast<double>(n + 1);
    }

    std::sort(clusters->begin(), clusters->end(), Cluster::compare);
}


void filter_clusters(
    Clusters*& most_likely_clusters, Clusters* const& clusters,
    LocssMap* const& locssmap, const size_t& top = 3) {
    // if `top` is 0, then find all filtered clusters
    // note that in this case clusters must be collated in descending order already
    if (top == 0)
        sort(clusters->begin(), clusters->end(), Cluster::compare);

    set<unsigned int> *prvss = new set<unsigned int>(), *mrgss;
    auto idx = 0;
    double pval = (clusters->size() == 0) ? 0 : clusters->at(0)->pval, threshold;
    auto iter = clusters->cbegin();
    while ((threshold = thresholding(pval)) <= 1 && iter != clusters->cend() && (top == 0 || idx < top)) {
        auto &cluster = *iter;
        auto &locss = locssmap->find(cluster->sid)->second;
        pval = cluster->pval;
        mrgss = new set<unsigned int>();
        set_intersection(
            prvss->begin(),
            prvss->end(),
            locss->begin(),
            locss->end(),
            inserter(*mrgss, mrgss->begin())
        );
        auto mrgsize = mrgss->size();

        mrgss->clear();
        delete mrgss;
        mrgss = nullptr;

        ++iter;

        // Suppress a candidate if its spatial footprint overlaps any
        // previously retained cluster. Only disjoint candidates are kept.
        if (mrgsize > 0)
            continue;

        ++idx;
        prvss->insert(locss->begin(), locss->end());
        most_likely_clusters->push_back(cluster);
    }
}

