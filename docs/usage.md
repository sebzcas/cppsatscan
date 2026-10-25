# Usage Guide

This document describes the build environment, execution procedure, parameter-file format, input data schemas, and output files supported by `cppsatscan`.

## 1. Requirements

The current development setup uses:

- Windows
- GCC through the MSYS2 UCRT64 toolchain
- OpenMP support
- Visual Studio Code for building and running the program

The build configuration below reflects the current Windows development environment. Other operating systems and toolchains have not been documented as supported configurations.

## 2. Building the Program

### 2.1 Compiler Configuration

The project currently uses GCC with C++11 and OpenMP. The following is the relevant VS Code build-task configuration from the development environment:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "build",
            "type": "shell",
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "presentation": {
                "echo": true,
                "reveal": "always",
                "focus": false,
                "panel": "shared"
            },
            "windows": {
                "command": "g++",
                "args": [
                    "-fopenmp",
                    "-ggdb",
                    "${file}",
                    "--std=c++11",
                    "-o",
                    "${fileDirname}\\${fileBasenameNoExtension}.exe"
                ]
            },
            "problemMatcher": [
                "$gcc"
            ]
        }
    ]
}
```

The task compiles the currently active source file. Ensure that the selected source file and resulting executable path match the executable expected by the run script.

In the current setup, the run script expects the executable at `src\satscanner.exe`. If the executable is produced elsewhere, either select the appropriate build target or adjust the output path.

The compiler path is specific to the current machine and should be changed if GCC is installed elsewhere.

## 3. Running the Program

The program is launched using a Windows batch script:

```bat
@echo off
setlocal

rem Adjust this path to your MSYS2 UCRT64 installation.
set "MSYS2_UCRT64=C:\msys64\ucrt64"
set "PATH=%MSYS2_UCRT64%\bin;%PATH%"

cd /d "%~dp0"

"%~dp0src\satscanner.exe" -p "%~dp0inst.prm"

set "EXIT_CODE=%ERRORLEVEL%"
echo.
echo SaTScan exited with code %EXIT_CODE%.
pause

endlocal
exit /b %EXIT_CODE%
```

The script:

1. Sets the runtime `PATH` for the GCC toolchain.
2. Runs `src\satscanner.exe` with `inst.prm` as its parameter file.
3. Prints the process exit code.
4. Pauses the console before returning the exit code.

The parameter file is referenced relative to the script location. Input and output paths inside the parameter file are specified separately.

To run the program, ensure that the executable exists at the expected location and that the input files and output directory are accessible.

## 4. Parameter File

The program reads its configuration from a text file passed using `-p`.

Each parameter is specified on a separate line, followed by its value.

### 4.1 Example

The following example configures a purely spatial scan using annual data:

```text
-gp data\input\nm.geo
-pp data\input\nm-ars.pop
-cp data\input\nm-ars.cas
-rp data\output\
-typ PS
-stm 1973-01-01
-ntm 1991-01-01
-ts %(1)Y
-mcn 999
-tcn 10
-pn 1
-pnd 0.5
-lnd 0
-tnd 0.5
-gc 0
-wtmp 1
```

Replace the example paths with the actual locations of your input files and output directory.

### 4.2 Parameter Reference

| Parameter | Description |
|---|---|
| `-gp` | Path to the geographic location file (`.geo`). |
| `-pp` | Path to the population file (`.pop`). |
| `-cp` | Path to the case file (`.cas`). |
| `-rp` | Output directory. |
| `-typ` | Scanning mode: `PT`, `PS`, or `ST`. |
| `-stm` | Start date of the study period, formatted as `YYYY-MM-DD`. |
| `-ntm` | End date of the study period, formatted as `YYYY-MM-DD`. |
| `-ts` | Temporal step format, such as `%(1)Y` for one-year steps. |
| `-mcn` | Number of Monte Carlo replicates used to estimate empirical p-values. |
| `-tcn` | Top-cluster count parameter. |
| `-pn` | Parallelism parameter. |
| `-pnd` | Population-based spatial-window expansion limit, expressed as a proportion of the global population-related quantity. |
| `-lnd` | Location-count-based spatial-window expansion limit, expressed as a proportion of the total number of locations. |
| `-tnd` | Temporal-window expansion limit, expressed as a proportion of the total number of time buckets. |
| `-gc` | Coordinate system selection: `0` for Cartesian coordinates and `1` for geographic coordinates. |
| `-wtmp` | Whether to write intermediate files such as `all.cls.txt` and `sim.llr.txt`: `1` enables output and `0` disables it. |

The window-expansion parameters control candidate-generation limits. Their effects depend on the scanning mode and the corresponding implementation path.

The exact operational effects of `-tcn` and `-pn` should be interpreted in conjunction with the code paths that consume these parameters.

### 4.3 Temporal Step Format

The temporal-step parameter uses the format:

```text
%(N)U
```

where `N` is an integer interval and `U` is a supported time unit, such as `d`, `m`, or `Y`.

Examples:

- `%(1)d` — one-day steps
- `%(1)m` — one-month steps
- `%(1)Y` — one-year steps

The study-period boundaries and temporal-step configuration jointly determine the time buckets used by the scan.

## 5. Input Data Formats

All three input files are comma-separated files without header rows. Each row represents one record.

### 5.1 Geographic Locations (`.geo`)

The geographic location file contains three columns:

| Column | Description |
|---|---|
| 1 | Location name or identifier |
| 2 | Longitude or first Cartesian coordinate |
| 3 | Latitude or second Cartesian coordinate |

Example:

```csv
LocationA,-106.6504,35.0844
LocationB,-103.2052,32.3140
LocationC,-104.2288,32.8998
```

The interpretation of the coordinate columns depends on `-gc`:

- `-gc 1`: geographic coordinates, with longitude in column 2 and latitude in column 3.
- `-gc 0`: Cartesian coordinates, with the first and second coordinate components in columns 2 and 3.

The example illustrates the file layout only; the values are illustrative.

### 5.2 Case Counts (`.cas`)

The case file contains three columns:

| Column | Description |
|---|---|
| 1 | Location name or identifier |
| 2 | Year |
| 3 | Case count |

Example:

```csv
LocationA,1973,12
LocationA,1974,15
LocationB,1973,8
LocationB,1974,10
```

Each row specifies the number of cases recorded for a location in a particular year.

### 5.3 Population Counts (`.pop`)

The population file contains three columns:

| Column | Description |
|---|---|
| 1 | Location name or identifier |
| 2 | Year |
| 3 | Population count |

Example:

```csv
LocationA,1973,12500
LocationA,1974,12700
LocationB,1973,8400
LocationB,1974,8500
```

A population value must be available for every required location-year combination. Missing population records can cause the program to fail during data processing, particularly when constructing the population-exposure index for ST scanning.

### 5.4 Input Consistency

The input files should satisfy the following requirements:

- Location identifiers must be consistent across the geographic, case, and population files.
- Case and population records must use compatible time values and temporal granularity.
- Every required location-year combination must have a population value.
- Coordinates must be consistent with the coordinate system selected by `-gc`.
- The configured study period and temporal step must be compatible with the supplied data.

The current implementation is designed around location-based case counts, population counts, and coordinates. It does not support arbitrary additional covariates.

## 6. Output Files

The output directory is specified using `-rp`.

The program produces final cluster results and can optionally write intermediate files for algorithm inspection.

### 6.1 Final Cluster Results

The final cluster output uses the following comma-separated columns:

```text
id,sid,tid,locations,timespan,cz,bz,hz,llr,p_val,centre,radius,cg,bg,hg,mz,rr,oer,ann
```

| Field | Description |
|---|---|
| `id` | Candidate or cluster identifier |
| `sid` | Spatial identifier |
| `tid` | Temporal identifier |
| `locations` | Locations included in the candidate |
| `timespan` | Temporal interval associated with the candidate |
| `cz` | Observed cases in the candidate window |
| `bz` | Population-related quantity for the candidate window |
| `hz` | Number of time buckets in the candidate window |
| `llr` | Log-likelihood ratio |
| `p_val` | Monte Carlo p-value |
| `centre` | Reported spatial center |
| `radius` | Reported spatial radius |
| `cg` | Total cases in the study region |
| `bg` | Global population-related quantity |
| `hg` | Number of time buckets in the study period |
| `mz` | Expected cases |
| `rr` | Relative risk |
| `oer` | Observed-to-expected ratio |
| `ann` | Annualized rate |

The meaning of some population-related quantities and the availability of derived statistics depend on the scanning mode and processing stage.

### 6.2 Intermediate Candidate Output: `all.cls.txt`

Set `-wtmp 1` to enable intermediate output.

`all.cls.txt` uses the same column schema as the final cluster output:

```text
id,sid,tid,locations,timespan,cz,bz,hz,llr,p_val,centre,radius,cg,bg,hg,mz,rr,oer,ann
```

Unlike the final output, which contains all fields populated where applicable for the reported clusters, `all.cls.txt` exposes the collection of candidate clusters at the intermediate stage. Some fields may be empty or contain sentinel values because the corresponding statistics have not yet been calculated or assigned.

Example:

```csv
0,1192206971,2025440425,0,"('Bernalillo', 'LosAlamos', 'Sandoval', 'Socorro', 'Torrance', 'Valencia')","()",536,567641,1,2.9223,-1,,-1,0,0,0,494.95,-1,-1,-1
1,97747505,49925179,0,"('Bernalillo', 'Sandoval', 'Socorro', 'Torrance', 'Valencia')","()",517,550408,1,2.40401,-1,,-1,0,0,0,479.92,-1,-1,-1
```

For example, `p_val` may be `-1`, `centre` may be empty, and derived measures may contain placeholder values at this stage.

This file is useful for examining the candidate set beyond the clusters ultimately reported in the final output. It provides a way to inspect intermediate scanning results and investigate how candidate generation and subsequent processing affect the final results.

### 6.3 Monte Carlo Output: `sim.llr.txt`

When intermediate output is enabled, `sim.llr.txt` stores the values in the simulation routine's `simvals` array.

The file contains one numeric value per line:

```text
<simulated maximum LLR for replicate 1>
<simulated maximum LLR for replicate 2>
<simulated maximum LLR for replicate 3>
...
```

Each value represents the maximum simulated LLR retained for a Monte Carlo replicate by the implementation's simulation procedure.

The number of lines corresponds to the number of simulation entries written by the program. With `-mcn 999`, the expected number of entries is 999 when the full array is written successfully.

These values are used by the ranking procedure to estimate empirical p-values for observed candidates.

### 6.4 Intermediate Output and Reproducibility

Intermediate files are intended for inspection and debugging of the scanning process. They are particularly useful when comparing candidate generation, LLR values, and Monte Carlo results with those of a reference implementation.

The contents of an intermediate file reflect the stage at which it is written. A missing or placeholder value should not automatically be interpreted as a missing value in the original input data.

## 7. Supported Scope

The current implementation focuses on basic purely temporal, purely spatial, and space-time scanning using a Poisson model.

It does not implement the full feature set of SaTScan, including alternative probability models, covariate adjustments, and other advanced analysis options.

The input schema is limited to location identifiers, coordinates, case counts, population counts, and time values.

For details of the implemented scanning procedures, see [Algorithm Notes](algorithm-notes.md). For comparisons with the reference software, see [Validation](validation.md).

## 8. References

- SaTScan. [Official website](https://www.satscan.org/).
- SaTScan. [Technical Documentation](https://www.satscan.org/techdoc.html).
- Kulldorff, M. (1997). A spatial scan statistic. *Communications in Statistics: Theory and Methods*, 26(6), 1481–1496. [https://doi.org/10.1080/03610929708831995](https://doi.org/10.1080/03610929708831995).

The reference software used for comparison in this project is SaTScan v10.3.3. The references provide background on the statistical method and the reference software; they do not imply that this implementation reproduces all of its features or numerical behavior.