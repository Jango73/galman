# Galman

Galman is an image gallery manager. It lets you browse folders, compare sets of files, preview images, and sort or move selections. The interface focuses on fast navigation and visual comparison.

The app also supports automation scripts, and you can add your own JavaScript scripts by dropping them into the `scripts/` folder.

## Technical

### Prerequisites
- Qt 6 (Quick, Concurrent, Network)
- CMake and Ninja
- A C++17 compiler

On Debian or Ubuntu, `./deps.sh` installs the required dependencies.
On Windows, ensure Qt 6, CMake, and Ninja are in `PATH`.

### Build
```bash
./build.sh
```

Useful options:
- `--debug` or `--release` (default is release)
- `-c Debug` or `-c Release`
- `--verbose` for verbose build output

Windows:
```bat
build.bat
```

### Run
```bash
./run.sh
```

`run.sh` triggers a build if the matching build folder (debug or release) is missing. Output is logged to `temp/run.log`.

Windows:
```bat
run.bat
```

`run.bat` triggers a build if the matching build folder (debug or release) is missing. Output is logged to `temp/run.log`.

### Package (Debian)
```bash
./package.sh
```

Version source (single source of truth):
```bash
./set-version.sh 1.2.3
```

`package.sh` uses the version from `VERSION`. Generated packages are copied to `dist/`.

## Architecture
- **User interface (QML)**: `qml/App/Main.qml` drives the main screen. Reusable components live in `qml/Components`. Visual theme is defined in `qml/Theme.qml`.
- **Models and logic (C++)**: `src` contains the QML-exposed models (`FolderBrowserModel`, `FolderCompareModel`, `VolumeModel`) and copy operations through `CopyWorker`. `ScriptEngine` and `ScriptManager` handle script execution and discovery.
- **Integration**: `ComfyClient` and `ComfyWorkflowParser` handle external workflows related to image processing.
- **Application entry**: `src/main.cpp` initializes Qt, loads the QML scene, and exposes C++ services to the interface.
