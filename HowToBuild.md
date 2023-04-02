# How to build RisohEditor

## Requirements

- Visual Studio
- MSYS2
- CMake
- Inno Setup

ATL support is for Visual C++ only.

## Pre-task

- Load `src/RisohEditor_res.rc` by RisohEditor and save it as UTF-16 if you are using Visual Studio.

## Build task

- Execute a `cmake` command line using CMake.
    - Please specify `-G "(generator)"`. To get the generator list, please specify `-G` only.
    - If ATL support is needed, then add `-DATL_SUPPORT=ON`.
    - If you want XP support, use Visual C++ and `v141_xp` toolset.
    - `-A "Win32"` might be needed if your VS uses `x64` as default.
- Build it by the solution file, Makefile or Ninja.
- Execute `pack.sh` shell script.
- Execute `packportable.sh` shell script.

## Making an installer

- Open `installer.iss` with Inno Setup.
- Build it.
