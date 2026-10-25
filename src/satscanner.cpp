#include "core/scanning.h"

/*
 * Program workflow:
 * 1. Read and parse the parameter file.
 * 2. Load input data according to the scanning mode (PT, PS, or ST).
 * 3. Prepare temporal or spatial indexes and generate candidate clusters.
 * 4. Run Monte Carlo simulations and assign empirical p-values.
 * 5. Rank and filter candidate clusters, then reconstruct output details.
 * 6. Write the final most-likely clusters and optional intermediate results.
 *
 * PT performs temporal scanning, PS performs spatial scanning, and ST
 * combines spatial-window generation with space-time scanning.
 */

using namespace std;


int main(int argc, char *argv[]) {
    auto global_start_time = Xtm();
    
    // 1. Read and parse the parameter file.
    if (argc != 3) {
        cout << "command must be in format:" << endl;
        cout << "  satscanner.exe -p \"`path of .prm`\"";
        return 1;
    }
    auto prm_filepath = argv[2];
    auto kwargs = unordered_map<string, string>();
    read_parameters(prm_filepath, kwargs);

    // set 1 as initial returning status
    int stat = 1;
    
    const char* geo_path = kwargs.find("-gp")->second.c_str();   // -gp: geo filepath
    const char* pop_path = kwargs.find("-pp")->second.c_str();   // -pp: pop filepath
    const char* cas_path = kwargs.find("-cp")->second.c_str();   // -cp: cas filepath
    const char* res_path = kwargs.find("-rp")->second.c_str();   // -rp: res directory

    // -typ: scanning type in format "PT" (Purely Temporal), "PS" (Purely Spatial), or "ST" (Space-Time)
    auto& typ = kwargs.find("-typ")->second;
    SCANNING_TYPE scanning_type;
    if (typ == string("PT"))
        scanning_type = PT;
    else if (typ == string("PS"))
        scanning_type = PS;
    else if (typ == string("ST"))
        scanning_type = ST;
    else {
        const char* err = "error: `-typ` must be \"PT\", \"PS\", or \"ST\", but not ";
        cout << err << "\"" << typ << "\"" << endl;
        return -1;
    }

    // -stm: start datetime in format "%Y-%m-%d"
    const char* st_dtm = kwargs.find("-stm")->second.c_str();
    // -ntm: end datetime in format "%Y-%m-%d"
    const char* nd_dtm = kwargs.find("-ntm")->second.c_str();
    // -ts: temporal step in format "%" + "(" + `int` + ")" + ("d" | "m" | "Y"), eg. %(1)Y
    const char* temporal_step = kwargs.find("-ts")->second.c_str();
    
    // -mcn: number of Monte Carlo replicates used to estimate empirical p-values.
    unsigned int monte_carlo_num = (unsigned int) atoi(kwargs.find("-mcn")->second.c_str());
    // -tcn: top cluster num
    unsigned int top_cluster_num = (unsigned int) atoi(kwargs.find("-tcn")->second.c_str());
    // -pn: parallel num
    unsigned int parallel_num = (unsigned int) atoi(kwargs.find("-pn")->second.c_str());

    // -pnd: circle expansian stops when `bz` > `rate` * `bg`
    double use_popmaxend = (double) atof(kwargs.find("-pnd")->second.c_str());
    // -lnd: circle expansian stops when `loc_num_in_circle` > `rate` * `total_loc_num`
    double use_locmaxend = (double) atof(kwargs.find("-lnd")->second.c_str());
    // -tnd: timespan expansian stops when `utm_num_in_timespan` > `rate` * `total_utm_num`
    double use_tspmaxend = (double) atof(kwargs.find("-tnd")->second.c_str());

    // -gc: if use geographic coordinate system or cartesian coordinate system (with distance measure)
    bool use_geo_coord = (bool) atoi(kwargs.find("-gc")->second.c_str());
    // -wtmp: if write temporary files (`all.cls.txt`, `sim.llr.txt`) into result path
    bool save_tmp_files = (bool) atoi(kwargs.find("-wtmp")->second.c_str());

    cout << "geo_path = " << geo_path << endl;
    cout << "pop_path = " << pop_path << endl;
    cout << "cas_path = " << cas_path << endl;
    cout << "res_path = " << res_path << endl;
    cout << "scanning_type = " << scanning_type << endl;
    cout << "st_dtm = " << st_dtm << endl;
    cout << "nd_dtm = " << nd_dtm << endl;
    cout << "temporal_step = " << temporal_step << endl;
    cout << "monte_carlo_num = " << monte_carlo_num << endl;
    cout << "top_cluster_num = " << top_cluster_num << endl;
    cout << "parallel_num = " << parallel_num << endl;
    cout << "use_popmaxend = " << use_popmaxend << endl;
    cout << "use_locmaxend = " << use_locmaxend << endl;
    cout << "use_tspmaxend = " << use_tspmaxend << endl;
    cout << "use_geo_coord = " << use_geo_coord << endl;
    cout << "save_tmp_files = " << save_tmp_files << endl;
    cout << endl;

    // **** 0. Here calculation begins, including data loading process ****
    time_t st_utm = Xtm(st_dtm, "%Y-%m-%d").to_utm();
    time_t nd_utm = Xtm(nd_dtm, "%Y-%m-%d").to_utm();
    
    auto *geo = new GeoData();            //    | PS | ST
    auto *pop = new PopData();            // PT | PS | ST
    auto *cas = new CasData();            // PT | PS | ST
    // `rad_table` is a distance matrix composed of `loc`'s distances from each other
    auto *rad_table = new RadTable();     //    | PS | ST
    // `addr_table` maintains `addrx` as keys and original `utmss`s as values,
    // where `addrx` is id of group that composed by original datetimes after temporal compression
    auto *addr_table = new AddrTable();   // PT |    | ST
    // init global `locssmap` and `utmssmap` to store tables for mapping `sid` and `tid`
    // in clusters to `locss` and `utmss` respectively when specific `loc`s and `utm`s are required
    // {`sid`: `locss`, ...} and {`tid`: `utmss`, ...}
    LocssMap *glob_locssmap = nullptr;    //    | PS | ST
    UtmssMap *glob_utmssmap = nullptr;    // PT |    | ST
    // `clusters` stores all clusters scanned, including p-values
    // and note that `locss` and `utmss` are not set into a `Cluster`
    auto *clusters = new Clusters();      // PT | PS | ST
    // `S_Circles`, `T_Timespans`, `TS_Circles`, and `ST_Circles`
    // are some temporary containers for different scanning types
    auto *circles = new S_Circles();      //    | PS |
    auto *timespans = new T_Timespans();  // PT |    |
    // these two containers are transposition matrix to each other
    auto *ts_circles = new TS_Circles();  //    |    | ST
    auto *st_circles = new ST_Circles();  //    |    | ST

    // simvals[i] stores the maximum simulated LLR for replicate i,
    // used as the reference distribution when estimating empirical p-values.
    auto *simvals = new double[monte_carlo_num]();
    // `mlc` stores top n most likely clusters that filtered by `filter_clusters`
    auto *mlc = new Clusters();

    try {
    // if (1) {
        auto running_start_time = Xtm();

        // the supposed population values are in only one datetime (all `scanning_type`s below),
        // so here `Pop` should only have `loc` and `pop` columns and shall be load in `ps` mode
        if (scanning_type == PT) {
            load_pt_pop_from_csv(pop, pop_path);
            load_pt_cas_from_csv(cas, cas_path);
            glob_utmssmap = new UtmssMap();
        } else if (scanning_type == PS) {
            load_geo_from_csv(geo, geo_path);
            load_ps_pop_from_csv(pop, pop_path);
            load_ps_cas_from_csv(cas, cas_path);
            glob_locssmap = new LocssMap();
        } else if (scanning_type == ST) {
            load_geo_from_csv(geo, geo_path);
            load_pop_from_csv(pop, pop_path);
            load_cas_from_csv(cas, cas_path);
            glob_locssmap = new LocssMap();
            glob_utmssmap = new UtmssMap();
        } else {
            const char* err = "error: `-typ` must be `T`, `S`, or `ST`, but not ";
            cout << err << "`" << scanning_type << "`" << endl;
            throw err;
        }

        cout.precision(20);
        cout << *geo << endl;
        cout << *pop << endl;
        
        auto scanning_start_time = Xtm();
        if (scanning_type == PT || scanning_type == ST) {
            // Aggregate case counts into the configured temporal intervals.
            // addr_table preserves the mapping from compressed time indexes to
            // their original timestamps for later result reconstruction.
            compress_temporal_cases(cas, addr_table, st_dtm, nd_dtm, temporal_step);
            cout << cas->as_type('T') << endl;
        }
        if (scanning_type == PS || scanning_type == ST) {
            // Precompute pairwise location distances for spatial-window expansion.
            geography_spatial_radiation(geo, rad_table, use_geo_coord);
            if (scanning_type == PS)
                cout << *cas << endl;
        }

        for (auto iter = addr_table->cbegin(); iter != addr_table->cend(); ++iter) {
            auto &addrx = iter->first;
            auto &sss = iter->second;
            cout << addrx << ": (";
            for (auto jter = sss->cbegin(); jter != sss->cend(); ++jter) {
                auto &addrxe = *jter;
                cout << Xtm(addrxe).to_dtm(YMD_DASH) << ", ";
            } cout << ")" << endl;
        }

        size_t cg = aggregate_population(cas->vals);
        size_t bg = aggregate_population(pop->vals);
        size_t hg = calculate_time_step_index(nd_utm, st_dtm, nd_dtm, temporal_step) + 1;
        cout << "CG = " << cg << ", BG = " << bg << ", HG = " << hg << endl;

        if (scanning_type == PT) {
            double txtermrate = use_tspmaxend;
            
            auto t_start_time = Xtm();
            tscan::temporal_scanning(
                timespans, glob_utmssmap, pop, cas, addr_table, st_dtm, nd_dtm, temporal_step, txtermrate, 1);
            cout << "Running time: " << Xtm().to_utm() - t_start_time.to_utm() << "s" << endl;
            cout << "Completed temporal scanning" << endl;
        
            auto upg_start_time = Xtm();
            // Convert temporal windows into Cluster objects and calculate their
            // cluster-level statistics using the global case, population, and
            // temporal totals.
            tscan::update_timespans(clusters, timespans, cg, bg, hg);
            cout << "Running time: " << Xtm().to_utm() - upg_start_time.to_utm() << "s" << endl;
            cout << "Completed `timespan`s to `cluster`s upgrading" << endl;
            cout << "The total number of clusters (or timespans) is: " << clusters->size() << endl;
        } else if (scanning_type == PS) {
            // Use the population-exposure limit when enabled; otherwise use
            // the location-count limit to control spatial-window expansion.
            bool sxtermcond = use_popmaxend != 0;
            double sxtermrate = sxtermcond ? use_popmaxend : use_locmaxend;
            
            auto s_start_time = Xtm();
            sscan::spatial_scanning(circles, glob_locssmap, geo, pop, cas, rad_table, sxtermcond, sxtermrate, 1);
            cout << "Running time: " << Xtm().to_utm() - s_start_time.to_utm() << "s" << endl;
            cout << "Completed spatial scanning" << endl;

            auto upg_start_time = Xtm();
            sscan::update_circles(clusters, circles, cg, bg);
            cout << "Running time: " << Xtm().to_utm() - upg_start_time.to_utm() << "s" << endl;
            cout << "Completed `circle`s to `cluster`s upgrading" << endl;
            cout << "The total number of clusters (or circles) is: " << clusters->size() << endl;
        } else {
            bool sxtermcond = use_popmaxend != 0;
            double sxtermrate = sxtermcond ? use_popmaxend : use_locmaxend;
            double txtermrate = use_tspmaxend;
            auto s_start_time = Xtm();

            const time_t st_utm = Xtm(st_dtm, YMD_DASH).to_utm();
            const time_t nd_utm = Xtm(nd_dtm, YMD_DASH).to_utm();
            // Build a time-indexed population-exposure structure used to evaluate
            // spatial and space-time windows consistently across the study period.
            const auto exposure_index = build_population_exposure_index(geo, pop, cas, addr_table, st_utm, nd_utm);
            const size_t hg = exposure_index.times.size();

            // Generate spatial windows once and organize them by timestamp.
            // The same spatial-window definitions are then reused during
            // space-time evaluation instead of rebuilding them for every time window.
            satscan::spatial_scanning_2d(
                ts_circles, glob_locssmap, geo, pop, cas, rad_table, exposure_index, sxtermcond, sxtermrate, 1);
            cout << "Running time: " << Xtm().to_utm() - s_start_time.to_utm() << "s" << endl;
            cout << "Completed spatial scanning" << endl;
        
            auto cnt = 0;
            for (auto iter = ts_circles->cbegin(); iter != ts_circles->cend(); ++iter)
                cnt += iter->second->size();
            cout << "The total number of circles is: " << cnt << endl;

            auto cvs_start_time = Xtm();
            // Reverse the index organization from timestamp -> spatial window
            // to spatial window -> timestamp, enabling temporal expansion of
            // each spatial candidate.
            satscan::transpose_clusters(st_circles, ts_circles);
            cout << "Running time: " << Xtm().to_utm() - cvs_start_time.to_utm() << "s" << endl;
            cout << "Completed transposition from `ts_circles` to `st_circles` " << endl;
            cout << "and get size: " << st_circles->size() << " x " << ts_circles->size() << endl;

            auto *datapair = Xtm::from_rule(temporal_step);
            auto step_u = datapair->get_first();
            auto step_a = abs(datapair->get_second());
            delete datapair; datapair = nullptr;
            
            auto t_start_time = Xtm();
            // Evaluate temporal windows for the spatial candidates, producing
            // space-time clusters and their scan statistics.
            satscan::temporal_scanning(
                clusters, glob_utmssmap, st_circles, addr_table, glob_locssmap, exposure_index, cg, bg, hg,
                st_dtm, nd_dtm, temporal_step, txtermrate, parallel_num);
            cout << "Runnnig time: " << Xtm().to_utm() - t_start_time.to_utm() << "s" << endl;
            cout << "Completed temporal scanning" << endl;
            // cout << "The total number of timespans is: " << timespans->size() << endl;
            cout << "The total number of cluster is: " << clusters->size() << endl;
        }
        // for (auto iter = glob_utmssmap->cbegin(); iter != glob_utmssmap->cend(); ++iter) {
        //     auto &bgnutm = iter->second->front();
        //     auto &endutm = iter->second->back();
        //     cout << Xtm(bgnutm).to_dtm(YMD_DASH) << " ~ " << Xtm(endutm).to_dtm(YMD_DASH) << endl;
        // }

        time_t scanning_time = Xtm().to_utm() - scanning_start_time.to_utm();
        cout << "actual clusters scanning done and spent " << scanning_time << "s" << endl;
        
        if (save_tmp_files) {
            string act_path = (str(res_path) + str("all.cls.txt")).to_string();
            cout << "Start writing `act` data into file \"" << act_path << "\"" << endl;
            write_clusters_to_csv(act_path.c_str(), clusters, geo, glob_locssmap, glob_utmssmap);
            cout << "Complete writing `act` data" << endl;
        }
        // here roughly calculated scanning time for all monte carlo simulations
        // by multiply n times of simulations with time usage of the scanning on actual clusters above
        // and a conservative factor which is defined empirically
        int estimated_time = monte_carlo_num * scanning_time * 0.8;
        cout << "estimate that there are nearly " << estimated_time << "s" << endl;
        write_message(
            (res_path + str("est.msg.txt")).to_string(),
            (str("estimation: totally ") + str(estimated_time) + str("s")).to_string());
        
        auto sim_start_time = Xtm();
        // Generate Monte Carlo replicate statistics used to estimate empirical
        // p-values for the observed candidate clusters.
        simulation(simvals, clusters, cg, bg, hg, monte_carlo_num);
        cout << "Runnnig time: " << Xtm().to_utm() - sim_start_time.to_utm() << "s" << endl;
        cout << "Completed monte carlo simulations" << endl;
        // this code block aim to write simulated `llr` series into file
        if (save_tmp_files) {
            string llrstr("");
            for (auto i = 0; i < get_size(simvals); ++i)
                llrstr += std::isfinite(simvals[i])? (str(simvals[i]) + "\n").to_string() : "inf\n";
            write_message((str(res_path) + str("sim.llr.txt")).to_string(), llrstr);
        }

        auto r_start_time = Xtm();
        // distribution to assign empirical p-values.
        rank_clusters(clusters, simvals);
        cout << "Runnnig time: " << Xtm().to_utm() - r_start_time.to_utm() << "s" << endl;
        cout << "Completed p-value ranking" << endl;
        
        auto f_start_time = Xtm();
        if (scanning_type == PT) {
            // PT has a dedicated postprocessing path for temporal clusters.
            mlc->push_back(clusters->at(0));
            tscan::postprocess_clusters(mlc, timespans, cg, bg, hg, temporal_step);
        } else {
            // Rank candidates by the cluster comparator, then retain the
            // requested number of non-overlapping spatial or space-time clusters.
            sort(clusters->begin(), clusters->end(), Cluster::compare);
            filter_clusters(mlc, clusters, glob_locssmap, top_cluster_num);
            if (scanning_type == PS)
                sscan::postprocess_clusters(mlc, circles, geo, cg, bg, hg, use_geo_coord);
            else
                satscan::postprocess_clusters(mlc, st_circles, geo, cg, bg, hg, use_geo_coord, temporal_step);
        }
        cout << "Runnnig time: " << Xtm().to_utm() - f_start_time.to_utm() << "s" << endl;
        cout << "Completed MLC filtering" << endl;
        write_clusters_to_csv(
            (str(res_path) + str("mlc.cls.txt")).to_string(), mlc, geo,
            glob_locssmap, glob_utmssmap, cg, bg, hg);
        
        cout << "Running time: " << Xtm().to_utm() - running_start_time.to_utm() << "s" << endl;
        cout << "Completed all calculation" << endl;

        // free memory here for containers that defined in the beginning
        // note that it would better be free in reverse order of definitions
        destroy(mlc);
        delete[] simvals; simvals = nullptr;
        destroy(timespans);
        destroy(clusters);
        destroy(circles);
        destroy(st_circles);
        destroy(ts_circles, true);
        destroy(glob_utmssmap);
        destroy(glob_locssmap);
        destroy(addr_table);
        destroy(rad_table);
        delete cas; cas = nullptr;
        delete pop; pop = nullptr;
        delete geo; geo = nullptr;

        // return 0 for success
        stat = 0;
    } catch (const char *e) {
        cout << "Error: " << e << endl;
    // } else {
        destroy(mlc);
        delete[] simvals; simvals = nullptr;
        destroy(timespans);
        destroy(clusters);
        destroy(circles);
        destroy(st_circles);
        destroy(ts_circles, true);
        destroy(glob_utmssmap);
        destroy(glob_locssmap);
        destroy(addr_table);
        destroy(rad_table);
        delete cas; cas = nullptr;
        delete pop; pop = nullptr;
        delete geo; geo = nullptr;

        // return -1 for failure
        stat = -1;
    }

    cout << "ND: " << Xtm().to_utm() - global_start_time.to_utm() << "s" << endl;

    // system("pause");
    return stat;
}
