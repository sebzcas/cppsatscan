# Validation Against SaTScan Reference Results

## 1. Overview

This document compares the output of `cppsatscan` with reference results produced by SaTScan v10.3.3 (released in September 2025) using the New Mexico acute lymphoblastic leukemia (ALL) dataset (`nm-ars`). The comparison covers purely temporal (PT), purely spatial (PS), and space-time (ST) scanning.

The evaluation focuses on observed scan statistics, identified clusters, case counts, expected counts, population estimates, and estimated statistical significance. The reference results serve as a comparison baseline; this validation does not claim complete numerical or methodological equivalence with SaTScan.

## 2. Temporal Scanning (PT)

Both implementations identify the period from 1985 through 1991 as the leading temporal cluster.

| Metric | SaTScan reference | `cppsatscan` |
|---|---:|---:|
| Study period | 1973–1991 | 1973–1991 |
| Total cases | 1,175 | 1,175 |
| Cluster period | 1985–1991 | 1985–1991 |
| Observed cases | 552 | 552 |
| Expected cases | 478.77 | 478.99 |
| Relative risk | 1.29 | 1.29 |
| Observed/expected ratio | 1.15 | 1.15 |
| Log-likelihood ratio (LLR) | 9.329155 | 9.27155 |
| Estimated p-value | 0.002 | 0.031 |

The implementations agree on the leading cluster's time interval and observed case count. The expected counts, relative risk, and observed/expected ratio are also close, as are the observed LLR values.

The estimated p-values differ. Therefore, agreement in the identified period and observed scan statistic does not establish equivalence in statistical significance estimation.

## 3. Purely Spatial Scanning (PS)

Both implementations identify the same three leading spatial clusters: the regional cluster comprising Bernalillo, Sandoval, Valencia, Torrance, Santa Fe, Los Alamos, Socorro, and San Miguel counties; Sierra County; and Chaves County.

### 3.1 Leading spatial clusters

| Metric | SaTScan reference | `cppsatscan` |
|---|---:|---:|
| Regional cluster cases | 642 | 642 |
| Regional cluster expected cases | 586.58 | 586.58 |
| Regional cluster population | 672,787 | 672,730 |
| Regional cluster LLR | 5.235401 | 5.23571 |
| Regional cluster p-value | 0.045 | 0.564 |
| Sierra County cases | 17 | 17 |
| Sierra County expected cases | 7.80 | 7.80 |
| Sierra County population | 8,941 | 8,940 |
| Sierra County LLR | 4.086094 | 4.0867 |
| Sierra County p-value | 0.144 | 0.864 |
| Chaves County cases | 64 | 64 |
| Chaves County expected cases | 45.83 | 45.83 |
| Chaves County population | 52,564 | 52,560 |
| Chaves County LLR | 3.350054 | 3.34985 |
| Chaves County p-value | 0.266 | 0.970 |

The three leading clusters have matching county membership and observed case counts. Their expected counts and LLR values are very close to the reference values. The reported population estimates differ slightly.

The p-values differ substantially for all three clusters. This is a clear distinction between the close agreement in the observed scan statistics and the differences in estimated statistical significance.

### 3.2 Additional candidates

The implementation also reports other spatial candidates, including clusters involving Harding, Quay, Union, Rio Arriba, Roosevelt, Guadalupe, San Juan, Catron, Grant, and Colfax. Several of these have LLR values close to zero and estimated p-values of 1.

These candidates are included here as implementation output, not as evidence that their ranking or selection is identical to the reference analysis. The comparison above is limited to the three leading spatial clusters for which corresponding reference results are available.

## 4. Space-Time Scanning (ST)

Both implementations identify the same eight-county regional cluster as the leading space-time cluster. They also identify Chaves, Roosevelt, and Sierra county clusters among the reported candidates.

### 4.1 Leading space-time cluster

| Metric | SaTScan reference | `cppsatscan` |
|---|---:|---:|
| Regional cluster membership | Same eight counties | Same eight counties |
| Cluster period | 1985–1989 | 1985–1989 |
| Observed cases | 232 | 232 |
| Expected cases | 170.86 | 170.9 |
| Population | 672,787 | 672,733 |
| LLR | 11.725380 | 11.7107 |
| Estimated p-value | 0.010 | 0.002 |
| Relative risk | 1.45 | 1.45 |
| Observed/expected ratio | 1.36 | 1.36 |
| Annualized rate per 100,000 | 6.2 | 6.9 |

The implementations agree on the leading cluster's geographic membership, time interval, and observed case count. The expected counts, relative risk, observed/expected ratio, and LLR are also close.

The reported population estimates and annualized rates differ. The estimated p-value also differs, with the implementation producing a smaller value than the reference result.

### 4.2 Other reported space-time clusters

| Cluster | SaTScan reference period | `cppsatscan` period | Reference LLR | Implementation LLR | Reference p-value | Implementation p-value |
|---|---|---|---:|---:|---:|---:|
| Chaves | 1988–1989 | 1988–1989 | 7.245863 | 7.26 | 0.281 | 0.027 |
| Roosevelt | 1982–1983 | 1982–1983 | 3.855069 | 3.84553 | 0.997 | 0.615 |
| Sierra | 1976–1984 | 1976–1984 | 3.812186 | 3.80997 | 0.998 | 0.631 |

For these clusters, the reported periods and LLR values are close to the reference results. Their estimated p-values, however, differ.

The implementation also reports additional candidates, including Quay, Guadalupe, Rio Arriba, Curry, Union, and Doña Ana. These do not appear in the four reference space-time clusters listed above. Their presence in the implementation output does not, by itself, establish a defect: the available comparison does not determine whether the difference arises from candidate generation, ranking, filtering, or other implementation details.

## 5. Monte Carlo Significance Estimation

The implementation estimates significance using 999 Monte Carlo replicates. For each candidate cluster, it samples a count from a Poisson distribution parameterized by the cluster's expected count, calculates the corresponding LLR, and retains the maximum simulated LLR across candidates for each replicate.

The observed LLR values are generally close to those reported by the reference implementation, while the estimated p-values differ in several cases. The comparison establishes that the two sets of results do not yield matching significance estimates for all examined clusters. It does not, by itself, identify the precise cause of those differences or establish whether the significance-estimation procedures are statistically equivalent.

## 6. Observed Differences

The comparison identifies the following differences:

- **Population estimates:** Several cluster population estimates differ slightly from the reference values.
- **Annualized rates:** The leading space-time cluster has a reported annualized rate of 6.9 per 100,000 in the implementation, compared with 6.2 in the reference result.
- **Estimated p-values:** The estimated p-values differ for the leading temporal, spatial, and space-time clusters, as well as for several secondary space-time clusters.
- **Reported candidate clusters:** The implementation reports additional candidates that are not among the reference clusters listed in this comparison.
- **Time representation:** The implementation represents temporal intervals using date strings, whereas the reference output may display intervals using year-level or inclusive end-date conventions. The displayed intervals should therefore be interpreted according to each output format.

These differences are reported as observations. The current comparison does not isolate their underlying causes.

## 7. Conclusions

The comparison with SaTScan reference results supports the following conclusions:

1. **Agreement in leading-cluster identification:** The implementation identifies the same leading temporal interval and the same leading spatial and space-time regional cluster memberships in the examined dataset.
2. **Close agreement in observed statistics:** Observed case counts match for the leading clusters, while expected counts, relative risks, observed/expected ratios, and LLR values are generally close to the reference values where these metrics are available.
3. **Differences in statistical significance:** Estimated p-values differ across the examined scan types. Close agreement in observed LLR values should not be interpreted as proof that the significance estimates are equivalent.
4. **Remaining numerical differences:** Population estimates, at least one reported annualized rate, and the sets of reported secondary candidates are not identical to the reference output.
5. **Scope of validation:** These results demonstrate agreement in several important aspects of cluster identification and observed scan statistics on the examined dataset. They do not establish complete numerical equivalence with SaTScan or resolve every difference in statistical significance and secondary-cluster reporting.

Further validation would require targeted examination of the specific calculations and processing stages associated with the observed differences.

## References

1. SaTScan. Official website and technical documentation.
   https://www.satscan.org/
   https://www.satscan.org/techdoc.html

2. Kulldorff, M. (1997). A spatial scan statistic.
   Communications in Statistics: Theory and Methods, 26(6), 1481–1496.
   https://doi.org/10.1080/03610929708831995

3. SaTScan. New Mexico Brain Cancer Dataset.
   https://www.satscan.org/datasets/nmbrain/index.html
