# Algorithm Notes

## 1. Overview

This project implements spatial, temporal, and space-time scan workflows in C++.

The main scanning modes are:

- **PT — Purely Temporal:** evaluates temporal windows over the study period.
- **PS — Purely Spatial:** evaluates spatial windows around candidate centers.
- **ST — Space-Time:** combines spatial footprints with temporal windows.

The general workflow consists of candidate generation, likelihood-ratio calculation, Monte Carlo ranking, candidate filtering, and postprocessing.

The ST workflow additionally builds a population-exposure index and evaluates spatial footprints across time buckets.

These notes describe the implementation in this repository. They do not imply complete numerical or statistical equivalence with the reference SaTScan implementation.

## 2. Cluster Statistics

The `Cluster` structure stores candidate-window statistics and derived measures.

| Field | Description |
|---|---|
| `cz` | Observed cases in the candidate window |
| `cg` | Total observed cases in the study region |
| `bz` | Population-related quantity for the candidate window |
| `bg` | Population-related quantity for the study region |
| `hz` | Number of time buckets in the candidate window |
| `hg` | Number of time buckets in the study period |
| `mz` | Expected cases in the candidate window |
| `llr` | Log-likelihood ratio |
| `pval` | Monte Carlo p-value |

The precise interpretation of population-related quantities depends on the scanning mode and the code path that constructs or updates a cluster.

### 2.1 Expected Case Counts

The general `Cluster` constructor computes the expected case count as:

$$
m_z = \frac{c_g}{b_g} b_z \frac{h_z}{h_g}
$$

Here, $$b_z$$ and $$b_g$$ represent the population-related quantities supplied to the constructor, while $$h_z$$ and $$h_g$$ represent the candidate and study-period time-bucket counts.

For the PS postprocessing path, the same formula is used with $$h_z=h_g=1$$, giving:

$$
m_z = \frac{c_g b_z}{b_g}
$$

The ST temporal-scanning path computes expected cases separately, using population exposure:

$$
m_z = c_g \frac{E_z}{E}
$$

where:

- $$E_z$$ is the population exposure within the candidate spatial footprint and temporal window.
- $$E$$ is the total population exposure across the study region and study period.

The ST implementation therefore uses its exposure-based calculation directly rather than relying on `Cluster::calc_mz()` for this step.

### 2.2 Log-Likelihood Ratio

`Cluster::calc_llr()` computes the log-likelihood ratio using the observed cases $$c_z$$, global cases $$c_g$$, and expected cases $$m_z$$.

For a candidate with $$c_z>m_z$$, the implemented calculation is:

$$
\mathrm{LLR} = c_z \ln\left(\frac{c_z}{m_z}\right) + (c_g-c_z) \ln\left(\frac{c_g-c_z}{c_g-m_z}\right)
$$

The second term is omitted when $$c_z=c_g$$.

The function returns zero when $$c_z\leq m_z$$. It returns NaN when its input checks fail, including invalid case counts, a non-positive global case count, or an expected count outside the accepted range.

Candidates with non-finite LLR values are skipped in the ST temporal-scanning implementation.

### 2.3 Other Derived Measures

The implementation also provides helpers for:

- Relative risk (`calc_rr`)
- Observed-to-expected ratio (`calc_oer`)
- Annualized rate (`calc_ann`)

These measures are calculated during postprocessing. Their precise interpretation depends on the arguments supplied by the relevant scanning mode.

## 3. Purely Temporal Scanning (PT)

The PT workflow enumerates temporal windows using the ordered time information in `AddrTable`.

For each candidate window, the implementation:

1. Enumerates candidate start and end positions.
2. Maps the selected endpoints to temporal-step indices.
3. Calculates the inclusive window length:

$$
h_z = \mathrm{end\ index} - \mathrm{start\ index} + 1
$$

4. Accumulates the relevant case and population-related quantities.
5. Applies the configured maximum-window constraint.
6. Constructs temporal candidates and calculates their statistics.

The scan stops extending a window when its duration exceeds the configured fraction of the study-period duration.

The resulting candidates are subsequently ranked and filtered.

## 4. Purely Spatial Scanning (PS)

The PS workflow evaluates spatial windows around candidate centers.

For each center, locations are organized into border groups ordered by distance. Locations at the same distance are processed together.

The scanning procedure accumulates the cases and population-related quantities for the growing spatial footprint. Its stopping condition depends on the configured spatial-window constraint, which may be based on accumulated population or the number of border groups.

The border group that triggers termination is not emitted as a candidate.

The generated spatial windows retain information about their center, border, included locations, and accumulated quantities. These windows are converted into cluster objects for subsequent ranking and postprocessing.

For the PS path, expected cases are calculated during postprocessing using the spatial population-related quantities and the special case $$h_z=h_g=1$$.

## 5. Space-Time Scanning (ST)

The ST workflow combines spatial footprints with temporal windows.

Its main stages are:

1. Build a population-exposure index.
2. Generate spatial templates.
3. Evaluate spatial templates at individual time buckets.
4. Transpose the intermediate candidate structure.
5. Expand candidates across temporal windows.
6. Calculate expected cases and LLR values.

### 5.1 Population-Exposure Index

`build_population_exposure_index()` constructs an index for the selected study period.

It selects time buckets that contain at least one original timestamp within the study-period bounds. For each selected bucket, it retains the corresponding original timestamps.

For each geographic location, the function builds a prefix-sum array of population values across the selected time buckets. These prefix sums support efficient exposure calculations over temporal intervals.

The index stores:

- Ordered time buckets and their indices
- Per-location population prefix sums
- Per-location average population
- Total population exposure
- Average total population

For a location with a single population record, that value is reused across the selected timestamps. When multiple records exist, population values are matched to the original timestamps. Missing population records or unmatched timestamps cause an exception.

The implementation also checks that the selected period contains time buckets and that total population exposure is positive.

### 5.2 Exposure Within a Candidate Window

`population_exposure_in_window()` calculates the exposure for a set of locations over an inclusive interval of time-bucket indices.

For a spatial footprint $$S$$, with start index $$s$$ and end index $$e$$, the calculation is:

$$
E_z =
\sum_{i\in S}
\left(P_i[e+1]-P_i[s]\right)
$$

where $$P_i$$ is the population prefix-sum array for location $$i$$.

The function validates the interval bounds and checks that each requested location has a corresponding prefix-sum array.

### 5.3 Spatial Template Generation

`spatial_scanning_2d()` generates candidate spatial footprints using the population-exposure information and the spatial scanning procedure.

These footprints provide the spatial templates evaluated during the subsequent time-bucket scan.

### 5.4 Spatial Evaluation by Time Bucket

`spatial_scanning_impl()` evaluates each spatial template at individual time buckets by accumulating cases within the template's location set.

Candidates with zero observed cases are excluded from this intermediate candidate set.

### 5.5 Transposition

`transpose_clusters()` reorganizes the intermediate candidates from a time-oriented structure:

- Time bucket → spatial-window ID → candidate

into a spatial-window-oriented structure:

- Spatial-window ID → time bucket → candidate

This makes it possible to evaluate temporal intervals for each spatial footprint.

The transformation reorganizes the existing candidate references rather than independently recalculating their spatial statistics.

### 5.6 Temporal Expansion

`temporal_scanning_impl()` enumerates temporal intervals for each spatial footprint.

For each interval, it:

1. Calculates the inclusive duration in time buckets.
2. Stops extending the interval when the configured maximum-window constraint is exceeded.
3. Accumulates observed cases across the selected time buckets.
4. Calculates the population exposure of the spatial footprint over the interval.
5. Calculates expected cases from the proportion of total exposure:

$$
m_z = c_g \frac{E_z}{E} 
$$

6. Calculates the LLR and skips candidates with non-finite values.
7. Retains the highest-LLR temporal interval for each spatial footprint.

The resulting ST candidates therefore represent spatial footprints paired with their selected temporal intervals.

## 6. Monte Carlo Simulation

The simulation routine generates Poisson-distributed case counts for candidate clusters.

For each candidate, it uses the candidate's expected case count as the Poisson parameter:

$$
X \sim \mathrm{Poisson}(mz)
$$

For each simulated count, the implementation calculates an LLR using the simulated count, global case count, and expected count.

For each Monte Carlo replicate, it retains the maximum LLR encountered across the candidates processed by the routine. The resulting array therefore stores replicate-level maximum LLR values.

The default simulation count in the routine is 999, unless another value is supplied.

This describes the implemented candidate-level simulation procedure. It does not establish that the procedure is statistically equivalent to generating a complete simulated dataset and rerunning the entire scan on each replicate.

## 7. Ranking and Filtering

### 7.1 Ranking

`rank_clusters()` sorts candidates by LLR in descending order and calculates Monte Carlo p-values by comparing each observed candidate LLR with the simulated replicate-level maximum LLR values.

The implementation counts simulated values greater than or equal to the observed candidate LLR and applies the following correction:

$$
p = \frac{r+1}{n+1}
$$

where:

- $$r$$ is the number of simulated maximum LLR values greater than or equal to the observed candidate LLR.
- $$n$$ is the number of Monte Carlo replicates.

This calculation avoids returning a zero p-value solely because no simulated value reaches the observed LLR.

### 7.2 P-Value Binning

`thresholding()` maps p-values to the upper bounds of predefined intervals:

| P-value interval   | Returned value |
|--------------------|---:|
| $$0 < p \leq 0.001$$ | 0.001 |
| $$0.001 < p \leq 0.005$$ | 0.005 |
| $$0.005 < p \leq 0.01$$ | 0.01 |
| $$0.01 < p \leq 0.05$$ | 0.05 |
| $$0.05 < p \leq 0.1$$ | 0.1 |
| $$0.1 < p \leq 0.5$$ | 0.5 |
| $$0.5 < p \leq 1$$  | 1.0 |

Negative values return zero. Values that do not match any interval, including zero and NaN, fall through to the final return value of 1.0.

This function performs p-value binning; it does not independently implement a conventional significance test. The effect of the returned value on candidate retention depends on how `filter_clusters()` uses it.

### 7.3 Candidate Filtering

`filter_clusters()` applies the configured thresholding logic and a spatial-overlap filter.

The spatial-overlap filter checks whether a candidate's location set intersects the locations already retained. Candidates that overlap previously retained spatial footprints are skipped.

This is a spatial-footprint overlap check. It should not be described as an explicit spatiotemporal-overlap test unless additional code establishes such behavior.

## 8. Postprocessing and Output

Postprocessing derives additional statistics and presentation fields for retained candidates, including:

- Spatial center
- Radius
- Expected cases
- Relative risk
- Observed-to-expected ratio
- Annualized rate

The exact calculations depend on the scanning mode and the data supplied to the postprocessing functions.

The resulting statistics are then written to the project's output files.

## 9. Implementation Scope and Validation

The project implements separate PT, PS, and ST scanning paths, along with candidate ranking, filtering, simulation, and postprocessing.

Validation against the reference SaTScan outputs is documented separately in [`validation.md`](validation.md).

That comparison records similarities and differences in detected windows, observed and expected counts, LLRs, p-values, and related statistics.

The implementation notes describe what the code does; the validation results describe how its outputs compare with the reference software. Similarity in selected clusters or descriptive statistics does not, by itself, establish complete equivalence of the underlying algorithms or statistical procedures.


## References and Methodological Background

The implementation is informed by the spatial scan statistic literature
and is compared with the reference SaTScan software.

- Kulldorff, M. (1997). A spatial scan statistic.
  Communications in Statistics: Theory and Methods, 26(6), 1481–1496.
  https://doi.org/10.1080/03610929708831995

- SaTScan. Official website and technical documentation.
  https://www.satscan.org/
  https://www.satscan.org/techdoc.html

These references provide methodological and software context. The
implementation details described in this document are based on the
source code in this repository; they should not be interpreted as a
claim of complete algorithmic equivalence with SaTScan.
