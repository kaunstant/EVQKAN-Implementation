# QCS Quantum KAN Benchmark

This project uses the exported QCS quantum circuit simulator as a backend for a small regression experiment. It compares:

- **QNN**: one parameterized quantum circuit used as a nonlinear regressor.
- **QKAN**: a sum of quantum activation functions with trainable edge parameters and classical mixing weights.
- **EVQKAN**: a research-inspired enhanced, two-stage quantum KAN with hidden and output quantum activations.

All models learn the same continuous target, `f(x) = sin(x)`, from the same training points and are evaluated on the same held-out points.

## Build and run

Requirements: Windows, CMake 3.16 or newer, and MSVC with C++17 support. Install Visual Studio 2022 or the Visual Studio Build Tools with the **Desktop development with C++** workload and the Windows 10/11 SDK. `run.bat` downloads a local CMake copy when CMake is unavailable and can locate an installed MSVC toolchain through `vswhere`.

```powershell
cmake -S . -B build/debug
cmake --build build/debug --config Debug
.\build\debug\Debug\QuantumSimulationStudio.exe
```

Use `run.bat debug` or `run.bat release` for a non-interactive Windows build, or run `run.bat` for the interactive choice. Debug links `qcs/debug`; Release links `qcs/release`.

The bundled QCS libraries are Windows MSVC `.lib` binaries, so this checkout cannot run on Linux or macOS without replacing them with platform-native QCS builds and adapting the package layout. CMake stops with a clear message when another compiler is used.

The program prints bordered metric and prediction tables, final trained parameters, train/test MSE, test MAE, test $R^2$, maximum error, parameter count, and training time. Runtime depends on the simulator and build configuration, so the terminal output is the experiment result to record.

## Layout

The bundled QCS 0.3.5 installs live under `qcs/debug` and `qcs/release`. New project code is isolated under `include/KAN` and `src/KAN`; the simulator backend is consumed through the matching configuration-specific QCS artifact.

See [docs/manual.md](docs/manual.md) for the mathematical model, training method, interpretation, and implementation details.

## Scope

This is an educational, simulator-backed comparison. EVQKAN is implemented as a two-stage enhanced composition for experimentation; it should not be presented as an exact reproduction of any particular paper without matching that paper's circuit, data, optimizer, and hyperparameters.

The LICENSE file is intentionally left for the project owner.

## Bundled QCS files

The `.gitignore` ignores generated build directories and compiler products. The versioned `qcs/debug` and `qcs/release` directories contain the QCS 0.3.5 headers, libraries, and exported CMake package files required by another checkout.
