# SanDiX TPC Geant4 Simulation

This repository contains a Geant4 simulation of the SanDiX single-phase liquid-xenon time projection chamber. It models particle interactions, xenon scintillation and ionization yields, electron drift and diffusion, proportional electroluminescence near the anode, and optical-photon detection.

The current version expands the original 2025 REU implementation with event-level signal accounting, additional source configurations, trajectory export for browser-based visualization, and updated analysis tools.

## Model overview

The simulated detector is a cylindrical liquid-xenon target with a central anode wire, a cylindrical cathode-wire array, PTFE reflectors, steel structures, and eight photosensor volumes. The electric field follows the cylindrical-wire approximation implemented in `src/nestFile.cc`.

The simulation uses:

- Geant4 for particle transport, detector geometry, optical processes, and ROOT output
- NEST for liquid-xenon scintillation and ionization yields, drift velocity, and diffusion
- A fast-simulation model for electron transport and S2 production close to the anode
- ROOT macros for plotting and fitting generated results

## Repository layout

```text
analysis/   ROOT analysis macro
include/    C++ headers
macros/     Geant4 run and visualization macros
src/        Simulation implementation and entry point
```

Generated build products and simulation outputs are deliberately excluded from version control.

## Requirements

- CMake 3.16 or newer
- A C++17 compiler
- Geant4 with UI and visualization support
- NEST with its Geant4 integration target (`NEST::NESTG4`)
- ROOT for using `analysis/graphing.c`

The exact Geant4 and NEST data paths depend on how those packages were installed. If CMake cannot locate NEST, pass its build or installation directory with `-DNEST_DIR=/path/to/NEST`.

Some local Geant4/NEST builds also expose transitive dependencies such as Qt 5 or gcem from nonstandard locations. Add those package prefixes when needed, for example:

```bash
cmake -S . -B build \
  -DNEST_DIR=/path/to/NEST \
  -DCMAKE_PREFIX_PATH="/path/to/gcem;/path/to/Qt5"
```

## Build

```bash
cmake -S . -B build -DNEST_DIR=/path/to/NEST
cmake --build build -j
```

CMake copies the files in `macros/` and the ROOT analysis macro into the build directory.

## Run

For the interactive Geant4 visualization:

```bash
cd build
./sandix
```

This executes `vis.mac`, runs the one event currently configured in `run.mac`, and then opens the interactive session.

For a batch run using a specific macro:

```bash
cd build
./sandix run.mac
```

The supplied macros are starting configurations:

- `run.mac`: configurable gamma, neutron, or electron source
- `spectrum.mac`: tabulated AmBe neutron-energy configuration snippet
- `radon.mac`: radon-220 ion-source configuration snippet
- `vis.mac`: interactive particle-trajectory visualization

Review particle type, energy, position, event count, and output naming before a production run.

The two configuration snippets do not contain `/run/beamOn`; include one from another macro and issue that command afterward. The current physics list does not register Geant4 radioactive-decay physics, so the radon source is not yet a validated decay workflow.

## Outputs

Depending on the selected macro, a run can produce:

- ROOT files containing hit and event-level quantities
- `output.csv` containing the most recently completed event's detected S1 and S2 values
- `event.json` containing sampled particle trajectories and electron-cluster origins for the web interface

The CSV and JSON files are intentionally lightweight interface products. Multi-event physics studies should use the ROOT output rather than treating those files as complete run summaries.

## Current assumptions and validation status

This is an active research simulation rather than a general-purpose detector package. Important model choices include fixed detector gains (`g1 = 0.13` and `g2 = 0.7`), a nominal 17 electroluminescence photons per electron, simplified analytic electric-field geometry, and custom transport-step settings.

The liquid-xenon user-limits implementation computes a maximum allowed step with a 0.1 mm lower bound on that cap; this is not a minimum Geant4 step length. The construction code also creates a 1 mm electron production-cut object but does not currently attach it to the liquid-xenon region, so the inherited/default Geant4 production cuts remain in effect. These settings should be resolved and revalidated before interpreting precision optical or charge-transport results.

Every new executable invocation currently writes `output3.root`, replacing an existing file of that name. Rename or move production output between invocations until output naming is made configurable.

## Related interface

The separate `CALIX-SANDIX-Web-Interface` project provides a local browser interface and Three.js event display for this simulation and a separately attributed CALIX simulation.

## Acknowledgments

This simulation began during the 2025 UCLA Summer REU program under the guidance of Professor Jay Hauser and Professor Alvine Kamaha.
