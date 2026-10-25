# cppsatscan

A C++ implementation of basic purely temporal, purely spatial, and space-time scan workflows using a Poisson model.

The project explores the implementation of spatial and temporal cluster detection, likelihood-ratio calculations, Monte Carlo ranking, and intermediate-result inspection. Its outputs have been compared with those produced by the reference SaTScan software.

## Features

- **PT — Purely Temporal Scanning:** evaluates temporal windows.
- **PS — Purely Spatial Scanning:** evaluates spatial windows around candidate centers.
- **ST — Space-Time Scanning:** evaluates spatial footprints across temporal windows.
- **Statistical calculations:** computes expected case counts and log-likelihood ratios, with additional cluster statistics produced during postprocessing.
- **Monte Carlo ranking:** estimates empirical p-values using simulated maximum LLR values.
- **Intermediate outputs:** supports writing selected intermediate result files for inspecting candidate clusters and simulation results.
- **Reference comparison:** includes a validation report comparing selected outputs with SaTScan.

## Build and Run

The current development setup uses GCC with OpenMP on Windows, with compilation and execution configured through Visual Studio Code tasks and a batch script.

For the exact compiler configuration, parameter-file syntax, input schemas, and execution instructions, see [Usage Guide](docs/usage.md).

## Input Data

The program reads three comma-separated files without header rows:

- `.geo` — location identifiers and coordinates.
- `.cas` — location identifiers, time values, and case counts.
- `.pop` — location identifiers, time values, and population counts.

The population input must provide a population value for every required location-year combination.

See the [Usage Guide](docs/usage.md) for column definitions, examples, coordinate settings, and parameter descriptions.

## Outputs and Algorithm Inspection

In addition to the final cluster results, the program can write intermediate files when temporary-output generation is enabled.

The current implementation includes support for writing `all.cls.txt` and `sim.llr.txt`. These files can help inspect intermediate cluster information and Monte Carlo simulation results.

For algorithm details, see [Algorithm Notes](docs/algorithm-notes.md). Refer to the Usage Guide for the output-file configuration and the exact contents of each file.

## Scope and Limitations

The current implementation focuses on basic PT, PS, and ST scanning under a Poisson model. It does not implement the full set of models, covariate adjustments, or other features available in SaTScan.

The population input also has a strict completeness requirement: every required location-year combination must have a corresponding population value.

The project is an independent implementation, not an official SaTScan product. Its numerical and statistical differences from the reference software are documented in [Validation](docs/validation.md).

## Documentation

- [Usage Guide](docs/usage.md) — build configuration, execution, parameters, input formats, and outputs.
- [Algorithm Notes](docs/algorithm-notes.md) — scanning procedures, statistical calculations, simulation, ranking, and filtering.
- [Validation](docs/validation.md) — comparison with reference SaTScan results.

## References

- Kulldorff, M. (1997). *A spatial scan statistic*. Communications in Statistics—Theory and Methods, 26(6), 1481–1496. [https://doi.org/10.1080/03610929708831995](https://doi.org/10.1080/03610929708831995)
- Kulldorff, M. *SaTScan User Guide, Version 10.1* (July 2022). [Official user guide](https://www.satscan.org/cgi-bin/satscan/register.pl/SaTScan_Users_Guide.pdf?todo=process_userguide_download)
- SaTScan. [Methodological references](https://www.satscan.org/references.html)

The cited references describe the background methodology and reference software. They should not be interpreted as a claim that this implementation reproduces every SaTScan feature or algorithmic detail.
