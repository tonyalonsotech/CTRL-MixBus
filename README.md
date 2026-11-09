# CTRL Mixbus

### Native Stereo Mixing and Processing Plugin for Akai MPC

**Project:** CTRL Mixbus\
**Developer:** Tony Alonso Tech\
**Plugin type:** MPC Compatible VST audio effect\
**Primary platform:** Akai MPC standalone\
**Build environment:** Ubuntu Linux with the Open MPC toolchain\
**Distribution:** [Open MPC Plugin
Catalog](https://open-mpc.github.io/)\
**Status:** Feasibility test completed

**Versions:**
V1.0.0 - Input and Output stage created. Tested local installation on MPC One G2 successfully.

V1.0.1 - Initial build of all control device pages tested and working on MPC One G2. CPU Load and Memory Load below 1% when used on Tube Synth Plugin with Air Compressor, Air Maximizer, and Air Delay in the FX Chain.

V1.0.2 - Adjusted knob placement to reflect Q Link Mapping. Adjusted appearance to match layout. Grouped pages in a more logical manner.

V1.0.3 - Corrected Filter representation to better match knob values. Corrected an issue where an audible pop is heard when changing the Filter slope.

V1.0.4 - Adjust UI to match Tony Alonso Tech color scheme. Fixed an issue where filter slope display did not always match Q Link assigned values.

------------------------------------------------------------------------

## Description

CTRL Mixbus is a professional stereo mixing and processing effect designed to streamline your workflow across three focused pages. Take control of input gain, stereo width, equalization, precision filtering, dynamic gating, and loudness maximization.

Featuring a versatile three-band isolator EQ, adjustable low-pass and high-pass filters, bass mono control, stereo manipulation, and independent processing stages, CTRL Mixbus puts the tools you need for detailed mix shaping and signal control right at your fingertips. Built natively for MPC, it delivers a comprehensive processing experience without relying on external hardware or software.



## Offline Installation

### Prerequisites

- Plugin Manager must already be installed on your Akai device.
- A PC with access to the MPC User drive.

### Installation Steps

1. Download the ZIP archive corresponding to your MPC generation.
2. Connect your Akai device to your PC.
3. Put your device into **Controller Mode**.
4. Open File Explorer and locate the **MPC User** drive.
5. If it does not already exist, create a folder named `local-plugins` in the root directory of the MPC User drive.
6. Copy the downloaded ZIP archive into the `local-plugins` folder.
7. Before disconnecting your device, safely eject any SD cards, USB flash drives, or external storage devices connected to your Akai device.
8. On your device, tap the **Standalone** button to return to Standalone Mode.
9. Start an empty project.
10. Add a new plugin and select **Plugin Manager**.
11. Locate **CTRL MixBus** in the plugin list.
12. Tap **Install**.
13. Tap **Apply**.
14. Tap **Restart and Apply**.
15. When the confirmation popup appears, tap **Restart and Apply** again.
16. Your device will restart to complete the installation.
17. Open a project.
18. Navigate to the **Mixer**.
19. Add an effect to any mixer channel.
20. **CTRL MixBus** should now appear as an available effect under the **VST** section.

---

## Manual Uninstallation

In some cases, Plugin Manager may not recognize the uninstaller or available updates for plugins installed offline. Fortunately, there is a workaround that allows you to manually uninstall the plugin using its installation script.

### Uninstallation Steps

1. Connect your Akai device to your PC.
2. Put your device into **Controller Mode**.
3. Open File Explorer and locate the **MPC User** drive.
4. Navigate to the `local-plugins` directory.
5. Using WinRAR, open the `CTRL-Mixbus` ZIP archive.
6. Locate `install.sh` and rename it to `tempinstall.sh`.
7. Locate `uninstall.sh` and rename it to `install.sh`.
8. Before disconnecting your device, safely eject any SD cards, USB flash drives, or external storage devices connected to your Akai device.
9. On your device, tap the **Standalone** button to return to Standalone Mode.
10. Start an empty project.
11. Add a new plugin and select **Plugin Manager**.
12. Locate **CTRL MixBus** in the plugin list.
13. Tap **Install**.
14. Tap **Apply**.
15. Tap **Restart and Apply**.
16. When the confirmation popup appears, tap **Restart and Apply** again.
17. Your device will restart to complete the process.
18. **CTRL MixBus** should now be uninstalled.

### Manually Updating the Plugin

To manually update CTRL MixBus, first complete the **Manual Uninstallation** procedure above. Once the existing installation has been removed, follow the [Offline Installation](#offline-installation) instructions to install the updated version.


## Initial Implementation

The MPC implementation is the first and primary development target. The
DSP engine, parameter model, preset format, and processing behavior will
be designed so they can be reused when desktop versions for Windows,
macOS, and Linux are developed later.

Project priorities:

-   Native operation within MPC standalone firmware.
-   Low CPU and memory consumption, particularly on Gen 1 hardware.
-   Full touchscreen operation.
-   Hardware Q-Link integration.
-   Integration with the MPC preset menu and project save/recall.
-   Portable user presets that can be transferred between MPC and future
    desktop versions.
-   Separate, validated builds for MPC Gen 1 and Gen 2.
-   Reproducible builds and documented release artifacts.

CTRL Mixbus is intended to run as a native MPC effect. It must not
require a companion application, background service, or separately
installed runtime on the MPC.

## 2. Development and Build Strategy

### MPC first

Development and release work will proceed in this order:

1.  Implement the CTRL Mixbus engine against the Open MPC engine
    interface.
2.  Define and lock the plugin parameters and their permanent ordering.
3.  Build the native MPC VST2 plugin using the Open MPC toolchain.
4.  Develop the MPC touchscreen pages and Q-Link layout.
5.  Run the Open MPC PC-side test suite.
6.  Benchmark and validate on physical Gen 1 and Gen 2 devices.
7.  Package and publish the MPC release through the Open MPC catalog.
8.  After the MPC implementation is stable, develop desktop plugin
    wrappers for Windows, macOS, and Linux using the shared DSP engine.

The desktop versions are future ports, not the first development target.

### Open MPC build environment

MPC plugin builds are performed on a development computer, not on the
MPC itself. The Open MPC build scripts use Docker containers to provide
the required build toolchains.

The primary development environment is Ubuntu Linux with:

-   Git
-   Python 3
-   Docker
-   The Open MPC `mpc-vst-plugins` repository
-   The CTRL Mixbus source repository

The Open MPC build guide is the authority for current setup and build
commands:

https://open-mpc.github.io/build.html

### Open MPC framework

CTRL Mixbus will follow the native plugin architecture and conventions
in:

https://github.com/open-mpc/mpc-vst-plugins

The plugin will provide a block-rendering C/C++ engine that the Open MPC
tools wrap as a native MPC VST2 plugin.

The engine must implement the interface required by the current Open MPC
wrapper. The current documented contract includes:

-   Engine creation and destruction.
-   MIDI input handling where applicable.
-   Setting and retrieving parameters by key.
-   Rendering 128 stereo frames at 44.1 kHz.

The exact declarations in the checked-out Open MPC `wrapper/engine.h`
and the current porting documentation must be treated as authoritative
during implementation. Do not duplicate or invent wrapper interfaces.

### Build description

The MPC plugin will be described using the Open MPC `vst.json`
configuration and its associated parameter and layout files.

The build description will identify:

-   Plugin name, vendor, UID, and version.
-   Shared-library filename.
-   Parameter definition file.
-   MPC page layout.
-   Engine source files and include paths.
-   Compiler definitions and required libraries.

The Open MPC build tooling generates the MPC plugin artifacts, including
the ARM shared library, parameter table, skin resources, and plugin-list
metadata.

## 3. Supported MPC Hardware and Firmware

CTRL Mixbus will have separate architecture-specific builds.

  -----------------------------------------------------------------------
  Hardware generation     Architecture            Build target
  ----------------------- ----------------------- -----------------------
  MPC Gen 1               ARM32 / ARMHF           ARMHF, built against
                                                  the Open MPC-compatible
                                                  glibc 2.31 baseline

  MPC Gen 2               ARM64 / AArch64         Separate AArch64 build
  -----------------------------------------------------------------------

### Gen 1 devices

Target devices include:

-   MPC Live
-   MPC Live II
-   MPC One
-   MPC X
-   MPC Key 61
-   Akai Force

Gen 1 compatibility includes MPC OS 2.x and 3.x, subject to the plugin's
compiled library and skin compatibility. OS 2.x and OS 3.x have
different skin requirements; compatibility must be verified for each
release.

### Gen 2 devices

Target devices include:

-   MPC Live III
-   MPC One G2
-   MPC Key G2 models

Gen 2 requires a separate AArch64 build and Gen 2-specific installation
and validation.

### Compatibility policy

A successful cross-compile is not proof of device compatibility.

Each hardware generation and supported firmware family must be validated
independently. The release documentation must state exactly which
devices and firmware versions were tested.

Current Open MPC compatibility guidance:

https://github.com/open-mpc/mpc-vst-plugins/blob/main/docs/COMPATIBILITY.md

Gen 2 guidance:

https://github.com/open-mpc/mpc-vst-plugins/blob/main/docs/GEN2.md

OS 2.x skin guidance:

https://github.com/open-mpc/mpc-vst-plugins/blob/main/docs/OS2_SKINS.md

## 4. Signal Processing Architecture

CTRL Mixbus uses a fixed processing chain:

1.  Input Gain
2.  Phase Inversion
3.  Stereo Width
4.  Bass Mono
5.  Balance
6.  Three-Band Isolator EQ
7.  Low-Pass Filter
8.  High-Pass Filter
9.  Noise Gate
10. Output Level

Every processing stage has an independent ON/OFF bypass control. Bypass
controls are operated through the MPC touchscreen, not Q-Links. Bypass
transitions should be smoothed where appropriate to avoid clicks.

### 4.1 Input Gain

-   Adjustable gain: -∞ dB to +24 dB.
-   Independent bypass.
-   Stereo input metering.

### 4.2 Phase Inversion

-   One switch inverts both left and right channels simultaneously.
-   Independent bypass.
-   Touchscreen operation.

### 4.3 Stereo Width

-   Adjustable from normal stereo width through to mono.
-   Mid/Side-based processing.
-   Independent bypass.

### 4.4 Bass Mono

-   Adjustable crossover frequency.
-   Low-frequency stereo information is summed to mono.
-   Stereo information above the crossover is retained.
-   Independent bypass.

### 4.5 Balance

-   Adjustable left/right balance.
-   Center position maintains equal channel levels.
-   Independent bypass.

### 4.6 Three-Band Isolator EQ

The EQ is based on the isolator behavior of the Pioneer DJM-900NXS2.

Three independently adjustable bands:

-   LOW
-   MID
-   HIGH

Each band has independent level adjustment and full-kill capability. The
complete EQ section has its own bypass control.

Crossover frequencies, gain range, kill behavior, and response curves
must be verified against reliable DJM-900NXS2 documentation or
measurements before the DSP implementation is finalized. The design must
not assume that a generic three-band EQ is an accurate reproduction.

### 4.7 Low-Pass Filter

Adjustable low-pass filter with:

-   Cutoff frequency.
-   Resonance.
-   Selectable curve:
    -   12 dB/octave
    -   24 dB/octave
    -   36 dB/octave
    -   48 dB/octave
    -   96 dB/octave
    -   Brickwall
-   Independent bypass.

The brickwall option must use a practical real-time implementation. Its
response, latency, ringing, stability, and CPU cost must be measured and
documented.

### 4.8 High-Pass Filter

Adjustable high-pass filter with:

-   Cutoff frequency.
-   Resonance.
-   Selectable curve:
    -   12 dB/octave
    -   24 dB/octave
    -   36 dB/octave
    -   48 dB/octave
    -   96 dB/octave
    -   Brickwall
-   Independent bypass.

The brickwall option must use a practical real-time implementation. Its
response, latency, ringing, stability, and CPU cost must be measured and
documented.

### 4.9 Noise Gate

Configurable stereo noise gate with:

-   Threshold.
-   Attack.
-   Hold.
-   Release.
-   Independent bypass.

Stereo-linked detection will be used to help maintain stereo image
stability.

### 4.10 Output Level

-   Adjustable output gain: -∞ dB to +12 dB.
-   Independent bypass.
-   Stereo output metering.

## 5. MPC User Interface

CTRL Mixbus will include a dedicated native MPC touchscreen interface,
designed around the MPC display and Q-Link layout.

### Q-Link rules

-   No more than 16 rotary controls on any individual page.
-   Rotary parameters are controlled using the Q-Links.
-   ON/OFF switches and bypass controls are operated through the
    touchscreen.
-   Phase inversion is operated through the touchscreen.
-   Q-Link assignments follow the currently selected page.
-   Displayed values and visual states update when parameters change.

The 16-control limit applies to Q-Link rotary parameters. Meters,
graphs, labels, and touchscreen switches do not consume rotary
assignments.

The final page layout must also respect the actual widget and page
capabilities of the Open MPC skin system.

### Initial interface pages

#### MAIN / MIX

-   Input Gain
-   Stereo Width
-   Bass Mono Crossover
-   Balance
-   Output Level
-   Input Meter
-   Output Meter
-   Touchscreen bypass controls

#### EQ / FILTER

-   LOW EQ
-   MID EQ
-   HIGH EQ
-   Low-Pass Cutoff
-   Low-Pass Resonance
-   Low-Pass Curve
-   High-Pass Cutoff
-   High-Pass Resonance
-   High-Pass Curve
-   Touchscreen EQ and filter bypass controls

#### GATE

-   Threshold
-   Attack
-   Hold
-   Release
-   Touchscreen gate bypass
-   Gate activity / gain-reduction visualization

Additional pages may be introduced as needed, while retaining the
maximum of 16 rotary controls per page.

### Visual feedback

Required visual elements:

-   Stereo input level meter.
-   Stereo output level meter.
-   Live filter response visualization.
-   Visual indication of filter cutoff and resonance.
-   Gate activity or gain-reduction indication.
-   Clearly visible ON/OFF state for every processing stage.
-   Current parameter values.

Visual updates must not block or disrupt the audio-rendering thread.
Where the MPC skin system has limitations, the implementation must use
supported native page elements and artwork rather than assume arbitrary
desktop-style drawing is available.

### Skin compatibility

The MPC skin is a first-class part of the plugin release.

The project must account for the different skin requirements of MPC OS
2.x and MPC OS 3.x. If one skin package cannot support both, the release
must include the appropriate variants or clearly state the firmware
limitation.

Skin pages will be developed and previewed using the Open MPC Skin
Studio and its documented tools.

## 6. Parameter Model and Project Recall

The parameter model must be designed before the full DSP implementation.

The Open MPC parameter definition determines the parameter ordering used
by MPC projects. Once a plugin has shipped, existing parameter positions
must never be reordered or reused. New parameters may only be appended.

Requirements:

-   Assign stable, unique parameter keys.
-   Define ranges, defaults, units, and display behavior.
-   Keep parameter ordering stable after release.
-   Commit the generated `params.lock` file where required by the Open
    MPC workflow.
-   Test parameter changes, automation, save, and restore.
-   Ensure multiple plugin instances maintain independent state.

The plugin must restore its state correctly when an MPC project is
saved, closed, and reopened.

## 7. Preset Management

CTRL Mixbus will include a portable, versioned preset format shared by
the MPC implementation and future desktop ports.

Presets must not depend on operating-system-specific paths or binary
serialization. A preset created on MPC must be usable by future Windows,
macOS, and Linux versions without conversion, and vice versa.

### Factory preset

The initial release will contain exactly one factory preset:

**Init**

Init restores every parameter to its defined default value.

### User presets

Users must be able to:

-   Save current settings as a new preset.
-   Load saved presets.
-   Rename user presets.
-   Delete user presets.
-   Restore Init.

### MPC integration

CTRL Mixbus must integrate with the MPC preset workflow, including the
plugin's PRESET menu where supported by the Open MPC wrapper and host.

Preset state and project state are related but distinct requirements:

-   Presets allow users to save and recall named processing setups.
-   MPC project state must restore the settings used by each plugin
    instance when the project is reopened.

The portable preset schema must be versioned and documented. Loading an
older preset version must either migrate it safely or report a clear
incompatibility.

## 8. Dependencies and Packaging

All required runtime dependencies must be included in the plugin
source/build package wherever licensing and platform requirements
permit.

Third-party engine and DSP source dependencies must be vendored into the
repository rather than downloaded dynamically during a build. This
supports reproducible builds and avoids network-dependent build steps.

The plugin must not require MPC users to independently install:

-   Additional DSP libraries.
-   External runtime libraries not already provided by the target
    environment.
-   Background services.
-   Separate processing applications.
-   Additional audio-processing frameworks.

Any dependency that cannot legally or technically be bundled must be
documented, including its version, license, and target-platform
availability.

### MPC release artifacts

The release package will follow the current Open MPC plugin packaging
conventions. Expected artifacts include:

-   Native MPC VST2 shared library (`.so`).
-   Plugin metadata and catalog information.
-   MPC touchscreen skin resources.
-   Parameter definitions and Q-Link/page configuration.
-   Required plugin assets and vendored dependencies.
-   Preset support and factory Init preset.
-   Installation and removal scripts generated or required by the
    release workflow.
-   Installation instructions.
-   Release checksums.

The exact package structure must follow the current Open MPC toolchain
rather than a custom structure invented by CTRL Mixbus.

## 9. Build, Test, Benchmark, and Release Workflow

The Open MPC workflow is the primary build and release process for the
MPC target.

### 9.1 Describe the plugin

Create and maintain:

-   `vst.json`
-   Parameter definition file.
-   Layout file.
-   Engine source and headers.
-   Build configuration and compiler definitions.

The plugin UID, shared-library name, and parameter ordering must remain
stable after release.

### 9.2 Build

Use the Open MPC build script with the CTRL Mixbus `vst.json`:

``` bash
MPC_VST=/path/to/mpc-vst-plugins
"$MPC_VST/tools/build_port.sh" vst/vst.json
```

The toolchain builds the native target library and generates the
associated plugin and skin artifacts.

Build each hardware architecture using the documented Open MPC target
configuration.

### 9.3 Design the skin

Use Open MPC Skin Studio to create and refine the touchscreen pages.

Preview every page and retain screenshots as release artifacts.

### 9.4 Test on the development computer

Run the Open MPC port test:

``` bash
"$MPC_VST/tools/test_port.sh" vst/vst.json
```

The PC-side test validates the engine and wrapper in a host test
environment. It is a required development gate, but it does not replace
physical MPC testing.

The test must pass before proceeding to device benchmarking.

### 9.5 Benchmark and test on hardware

Use the Open MPC benchmark tooling and test on actual target hardware.

Validate:

-   Plugin loading and unloading.
-   Audio input and output.
-   CPU use and audio stability.
-   Touchscreen operation.
-   Q-Link assignments.
-   Bypass controls.
-   Parameter automation.
-   Preset save and load.
-   Project save and recall.
-   Multiple plugin instances.
-   Long-duration operation.
-   Meter and visualization performance.

Gen 1 and Gen 2 must be tested separately.

### 9.6 Release and catalog publication

Use the current Open MPC release workflow to build, test, preview, and
package the release.

The release must be tested on the intended hardware before publication.

To list CTRL Mixbus in the Open MPC catalog, follow the current catalog
contribution process, including the required plugin catalog metadata and
release information.

## 10. Installation and Distribution

CTRL Mixbus will be distributed through the Open MPC ecosystem:

https://open-mpc.github.io/

The intended user installation routes are:

### On-device Open MPC Plugin Manager

Where supported, users can browse the catalog and install, update, or
remove CTRL Mixbus directly on the MPC using the Open MPC Plugin Manager
VST.

The current Open MPC documentation identifies the Plugin Manager as
supporting Gen 1 and Gen 2 devices.

### Open MPC desktop installer

The Open MPC installer app for Windows, macOS, and Linux can connect to
an MPC and install compatible plugin release packages.

This is an installation utility for MPC plugins. It does not mean the
CTRL Mixbus desktop plugin ports are available at initial MPC release.

### Other documented installation routes

Open MPC also documents shell-based and USB installation routes. These
may be used where appropriate for a supported device or firmware
configuration.

The release's `INSTALL.md` and the current Open MPC installation guide
must provide the authoritative steps.

Official installation guide:

https://open-mpc.github.io/install.html

## 11. Real-Time Audio and Performance Requirements

CTRL Mixbus must operate reliably alongside other MPC instruments,
effects, and audio tracks.

Requirements:

-   No dynamic memory allocation in the real-time audio-rendering path.
-   No blocking filesystem or network operations in the audio thread.
-   No mutex waits or other unbounded blocking in the audio thread.
-   No unnecessary background processing.
-   Efficient stereo DSP implementations.
-   Parameter smoothing to minimize clicks and zipper noise.
-   Correct operation at the render format and block size required by
    the Open MPC engine interface.
-   Efficient filter and gate processing.
-   Stable operation across ARMHF and AArch64 builds.
-   Independent state for multiple plugin instances.

Gen 1 performance is a particular concern. CPU use must be measured on
physical hardware and evaluated in the context of the complete MPC
project, not just an isolated plugin instance.

## 12. Testing and Quality Assurance

### DSP testing

-   Frequency response.
-   Filter slope accuracy.
-   Filter resonance.
-   Isolator crossover behavior.
-   Phase integrity.
-   Stereo width accuracy.
-   Bass mono operation.
-   Gate attack, hold, and release.
-   Input and output gain accuracy.
-   Clipping and headroom behavior.
-   Bypass transitions.
-   Parameter smoothing.
-   Sample-rate and block-processing correctness.

### MPC integration testing

-   Plugin loading and unloading.
-   Audio input and output.
-   Touchscreen interface.
-   Q-Link assignments.
-   Touchscreen bypass controls.
-   Parameter automation.
-   Preset loading and saving.
-   Project state save and restore.
-   Project reopening.
-   Multiple plugin instances.
-   Interface page switching.
-   Meter and visualization performance.

### Hardware validation

Separate validation is required for:

-   Gen 1 ARMHF hardware.
-   Gen 2 AArch64 hardware.
-   Supported MPC OS versions.
-   Any supported Hakai firmware configuration.

A release must not be marked hardware-verified until the relevant
physical device and firmware have been tested. The catalog's
hardware-test status must accurately reflect the results.

## 13. Release Documentation and Reproducibility

Every release must include or reference:

-   Complete CTRL Mixbus source code.
-   All required headers and submodules.
-   Vendored third-party dependencies and their licenses.
-   Build scripts and `vst.json`.
-   Parameter definitions and locked parameter ordering.
-   Architecture-specific compiler flags and target details.
-   Compiler and toolchain versions.
-   Plugin metadata and UID.
-   MPC touchscreen assets and skin previews.
-   Q-Link/page configuration.
-   Preset format documentation and Init preset.
-   Installation and removal instructions.
-   Release screenshots.
-   Exact source Git commit and release tag.
-   SHA-256 checksums.
-   Test and benchmark results.
-   Hardware and firmware compatibility results.

Builds must be reproducible from a documented source commit and pinned
toolchain.

## 14. Future Desktop Ports

The MPC implementation is the first release target. Desktop development
begins after the MPC engine and behavior are stable.

The DSP engine, parameter definitions, and portable preset format should
remain independent of the MPC-specific wrapper and skin.

The intended future desktop targets are:

  Platform   Planned format
  ---------- -----------------------------------------------------
  Windows    Desktop plugin format to be selected during porting
  macOS      Desktop plugin format to be selected during porting
  Linux      Desktop plugin format to be selected during porting

The Open MPC project documents a VST3 desktop port path for local DAW
use. That path may be evaluated when desktop development begins. Desktop
format selection and host compatibility will be documented separately.

Desktop ports must preserve the same DSP behavior, parameter meanings,
and portable preset compatibility as the MPC version.

## 15. Development Roadmap

  -----------------------------------------------------------------------
  Milestone                           Description
  ----------------------------------- -----------------------------------
  0.1                                 Open MPC project integration,
                                      engine skeleton, and parameter
                                      architecture

  0.2                                 Complete DSP processing chain

  0.3                                 Portable preset engine and MPC
                                      preset/state integration

  0.4                                 MPC touchscreen interface, Q-Link
                                      mapping, meters, and visualizations

  0.5                                 Open MPC PC-side testing and DSP
                                      validation

  0.6                                 Gen 1 ARMHF build, benchmarking,
                                      and physical-device testing

  0.7                                 Gen 2 AArch64 build, benchmarking,
                                      and physical-device testing

  0.8                                 Firmware compatibility, release
                                      packaging, and catalog preparation

  1.0                                 Initial MPC production release

  2.x                                 Windows, macOS, and Linux desktop
                                      ports
  -----------------------------------------------------------------------

Milestones may be adjusted as development and hardware testing progress.

## 16. Project References

**Open MPC Plugin Catalog**\
https://open-mpc.github.io/

**Open MPC Build Guide**\
https://open-mpc.github.io/build.html

**Open MPC Plugin Development Repository**\
https://github.com/open-mpc/mpc-vst-plugins

**Open MPC Porting Documentation**\
https://github.com/open-mpc/mpc-vst-plugins/blob/main/docs/PORTING.md

**Open MPC Skin Studio Documentation**\
https://github.com/open-mpc/mpc-vst-plugins/blob/main/docs/SKIN_STUDIO.md

**Open MPC Compatibility Documentation**\
https://github.com/open-mpc/mpc-vst-plugins/blob/main/docs/COMPATIBILITY.md

**Open MPC Gen 2 Documentation**\
https://github.com/open-mpc/mpc-vst-plugins/blob/main/docs/GEN2.md

**Open MPC Release Documentation**\
https://github.com/open-mpc/mpc-vst-plugins/blob/main/docs/RELEASING.md

These references guide the native MPC plugin architecture, build and
test process, touchscreen skin development, and future desktop-port
evaluation.

------------------------------------------------------------------------

## 17. Project Status

**CTRL Mixbus is in development. The first target is native operation on
Akai MPC standalone hardware.**

The initial implementation will use the Open MPC toolchain to build and
validate the native MPC VST2 plugin. Gen 1 ARMHF and Gen 2 AArch64
builds will be treated as separate targets and tested on their
respective hardware.

The desktop versions for Windows, macOS, and Linux will follow after the
MPC implementation is stable. The shared DSP engine and portable preset
format will be designed from the beginning to support those future
ports.

The ultimate goal is to deliver a self-contained stereo mixing and
processing solution that users can discover and install through the Open
MPC Plugin Catalog or the Open MPC Plugin Manager.

**CTRL Projects**\
**CTRL Mixbus**
