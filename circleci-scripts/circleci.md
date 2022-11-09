# Compilation on Different Platforms

A short description of the processes involved on [Circle CI](https://circleci.com/) before resulting packages are uploaded to the test folder.

This is mostly for developers to remember processes when upgrading some dependencies.

## Common
The whole process running on Circle CI machines is always `build.sh`

This script calls three stages from subfolders, environment variables are configured in `.circleci/config.yml`.

This is separated into three stages, as described below.

Common scripts for inclusion are prefixed with a `_`.

### Setup (setup_environment.sh)
Gets dependencies, source code, and binaries for the machine. This includes Python for generating the manual.

### Compilation (compile.sh)
As it says: compiles the externals.

### Packaging (pack.sh)
Gets a clone of the Github wiki and uses [grip](https://github.com/joeyespo/grip) to compile markdown into HTML.

Creates a .zip of the result, renames it to .dek for later upload to [deken](https://deken.puredata.info/), the Puredata package repository.

## Linux amd64 (linux64)
Just a straightforward compilation with dependencies.

## Linux i386 and arm (linux32 and linuxarm)
Uses [QEMU](https://www.qemu.org/) to emulate a machine for the target environment on an amd64 machine.

Compilation is done after `chroot` into the emulated machine.

This uses `_debootstrap_setup_environment.sh` in the setup stage.

## Windows (windows and windows64)
Downloads pre-compiled [MXE](https://mxe.cc/) cross compiling environment on Linux and uses that.

The resulting dll are quite large, because dependencies are statically linked, and therefore included.

This uses `_win_setup_environment.sh` in the setup stage, `_win_compile.sh` in the compilation stage, and `_win_pack.sh` in the packaging stage.

**Notes for upgrading MXE packages:**
- Machine for MXE compilation and server must have compatible libc versions.
- Compilation and usage must be in the same folder (here: `/tmp/mxe`) on both machines.
- Compilation of MXE dependencies for both 32 and 64 bit: `make MXE_TARGETS='x86_64-w64-mingw32.static i686-w64-mingw32.static' curl liboauth pthreads json-c`

## OS X (osx)
Gets dependencies from [homebrew](https://brew.sh/), and uses hdiutil to attach the downloaded dmg file as a disc image.

Compilation is then straightforward.

Packaging includes moving all dependencies downloaded from homebrew into the current folder, changes binaries to point to the correct folder, and include that in the .dek. This is done recursively.
