# CTRL Mixbus

### Native Stereo Mixing and Processing Plugin for Akai MPC

**Project:** CTRL Mixbus\
**Developer:** Tony Alonso Tech\
**Plugin type:** MPC Compatible VST audio effect\
**Primary platform:** Akai MPC standalone\
**Build environment:** Ubuntu Linux with the Open MPC toolchain\
**Distribution:** [Open MPC Plugin
Catalog](https://open-mpc.github.io/)\
**Status:** Released

**Versions:**
V1.0.0 - Input and Output stage created. Tested local installation on MPC One G2 successfully.

V1.0.1 - Initial build of all control device pages tested and working on MPC One G2. CPU Load and Memory Load below 1% when used on Tube Synth Plugin with Air Compressor, Air Maximizer, and Air Delay in the FX Chain.

V1.0.2 - Adjusted knob placement to reflect Q Link Mapping. Adjusted appearance to match layout. Grouped pages in a more logical manner.

V1.0.3 - Corrected Filter representation to better match knob values. Corrected an issue where an audible pop is heard when changing the Filter slope.

V1.0.4 - Adjusted UI to match Tony Alonso Tech color scheme. Fixed an issue where filter slope display did not always match Q Link assigned values.

V1.0.5 - Fixed an issue where On/Off switches would not always reflect assigned values, when adjusted using Q-Links. 

V1.0.6 - Updated the skin to have more contrast for controls. 

V1.0.7 - Fixed a Metering Issue where the input meters were not correctly reflecting the page stages.

V2.0.0 - This is the first release version. No major updates to the plugin itself. Plugin has been tested on MPC One G2 and MPC Live III, both running MPC OS 3.10. Package version changed to reflect plugin moving from beta into production. 

V2.0.1 - On/Off switches are now touchscreen only and are no longer assigned to Q-Links. The bottom On/Off switches on the INPUT and FILTER pages now sit at the same height. The DJ EQ knobs (High, Mid, Low) now use a centre-origin ring.

------------------------------------------------------------------------

## Description

CTRL Mixbus is a professional stereo mixing and processing effect designed to streamline your workflow across three focused pages. Take control of input gain, stereo width, equalization, precision filtering, dynamic gating, and loudness maximization.

Featuring a versatile three-band DJ EQ with selectable EQ and Isolator modes, adjustable low-pass and high-pass filters with resonance and curve shaping, bass width control, stereo manipulation, phase inversion, and independent processing stages, CTRL Mixbus provides the tools needed for detailed mix shaping and signal control. Fine-tune your stereo image with width, bass width, and balance adjustments, then refine your output with configurable gating and maximization for precise dynamic control and enhanced loudness.

Dedicated input and output level meters, maximizer gain reduction monitoring, and visual filter displays provide real-time feedback throughout your signal chain. Integrated clipping indicators flash red to warn of excessive signal levels, helping you maintain control over your mix.

Built natively for MPC, CTRL Mixbus brings essential mixing, stereo processing, tonal shaping, and dynamic control together in a streamlined interface designed for efficient workflow and hands-on operation, without relying on external hardware or software.



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




## 1. Project Overview

CTRL Mixbus is a professional stereo mixing and processing effect designed to bring essential mix-bus processing tools together in a streamlined interface.

The plugin provides input gain, phase inversion, stereo width, bass mono control, balance, a three-band isolator EQ, adjustable low-pass and high-pass filters, noise gating, output level control, and loudness maximization.

CTRL Mixbus is available in three production distributions. The MPC distribution is at version **2.0.1**; the desktop VST3 distributions are at version **2.0.0**:

| Distribution | Target | Architecture / Format |
|---|---|---|
| MPC Gen 1 | Akai MPC VST plugin | ARMHF |
| MPC Gen 2 | Akai MPC VST plugin | AArch64 |
| Linux | Desktop plugin | VST3 |
| Windows | Desktop plugin | VST3 |

The MPC distribution includes separate architecture-specific builds for Gen 1 and Gen 2 hardware. The desktop distributions provide VST3 versions for compatible Linux and Windows digital audio workstations.

The plugin uses a shared DSP and parameter architecture across its supported platforms. Platform-specific wrappers, build configurations, and interface implementations accommodate the requirements of each environment.

### Project Priorities

- Native operation within MPC standalone firmware.
- Efficient CPU and memory consumption, particularly on Gen 1 hardware.
- Full touchscreen operation on MPC hardware.
- Hardware Q-Link integration for rotary parameters.
- Integration with the MPC preset menu and project save/recall.
- Consistent processing behavior across supported platforms.
- Separate architecture-specific builds for MPC Gen 1 and Gen 2.
- Desktop VST3 compatibility on supported Windows and Linux systems.
- Reproducible builds and documented release artifacts.

---

## 2. Development and Build Architecture

CTRL Mixbus uses a shared processing architecture with platform-specific plugin integrations.

The native MPC implementation follows the Open MPC plugin architecture. The desktop versions use VST3 plugin builds for Windows and Linux.

### 2.1 Shared Architecture

The DSP engine and parameter model are maintained independently of platform-specific interface and plugin-host requirements wherever practical.

This separation allows the same core processing behavior to be used across:

- MPC Gen 1 ARMHF builds.
- MPC Gen 2 AArch64 builds.
- Linux VST3 builds.
- Windows VST3 builds.

Platform-specific code is used where required for plugin hosting, audio processing interfaces, user interface integration, packaging, and installation.

### 2.2 MPC Build Environment

MPC plugin builds are performed on a development computer rather than on the MPC itself.

The Open MPC build scripts use Docker containers to provide the required build toolchains.

The primary MPC development environment is Ubuntu Linux with:

- Git
- Python 3
- Docker
- The Open MPC `mpc-vst-plugins` repository
- The CTRL Mixbus source repository

The Open MPC build guide remains the reference for MPC-specific setup and build commands:

https://open-mpc.github.io/build.html

### 2.3 Open MPC Framework

The native MPC implementation follows the plugin architecture and conventions maintained by the Open MPC project:

https://github.com/open-mpc/mpc-vst-plugins

The MPC plugin provides a block-rendering C/C++ engine that the Open MPC tools wrap as a native MPC VST2 plugin.

The engine interface is governed by the checked-out Open MPC wrapper and its documentation. The documented contract includes:

- Engine creation and destruction.
- MIDI input handling where applicable.
- Setting and retrieving parameters by key.
- Rendering 128 stereo frames at 44.1 kHz.

The exact declarations in the checked-out Open MPC `wrapper/engine.h` and current porting documentation remain authoritative. Wrapper interfaces must not be duplicated or independently redefined.

### 2.4 MPC Build Description

The MPC plugin is described using the Open MPC `vst.json` configuration and its associated parameter and layout files.

The build description identifies:

- Plugin name, vendor, UID, and version.
- Shared-library filename.
- Parameter definition file.
- MPC page layout.
- Engine source files and include paths.
- Compiler definitions and required libraries.

The Open MPC build tooling generates the MPC plugin artifacts, including the ARM shared library, parameter table, skin resources, and plugin-list metadata.

### 2.5 Desktop VST3 Builds

The desktop distributions use the shared CTRL Mixbus processing architecture with platform-specific VST3 integration.

Separate desktop builds are maintained for:

- Linux VST3.
- Windows VST3.

The desktop plugin builds are distinct from the native MPC plugin packages. They are not MPC VST2 builds and do not use the MPC touchscreen skin as their desktop interface.

Desktop build configurations must preserve the shared processing behavior and parameter definitions while accommodating the requirements of the relevant operating system, compiler, plugin SDK, and host environment.

---

## 3. Supported MPC Hardware and Firmware

CTRL Mixbus provides separate architecture-specific builds for MPC Gen 1 and Gen 2 hardware.

| Device | Hardware Generation | Architecture | Build Target |
|---|---|---|---|
| Akai Force | MPC Gen 1 | ARM32 | ARMHF Build |
| Akai Force G2 | MPC Gen 2 | ARM64 | AARCH64 Build |
| Akai MPC Key 37 | MPC Gen 1 | ARM32 | ARMHF Build |
| Akai MPC Key 37 G2 | MPC Gen 2 | ARM64 | AARCH64 Build |
| Akai MPC Key 61 | MPC Gen 1 | ARM32 | ARMHF Build |
| Akai MPC Live | MPC Gen 1 | ARM32 | ARMHF Build |
| Akai MPC Live II | MPC Gen 1 | ARM32 | ARMHF Build |
| Akai MPC Live III | MPC Gen 2 | ARM64 | AARCH64 Build |
| Akai MPC One | MPC Gen 1 | ARM32 | ARMHF Build |
| Akai MPC One+ | MPC Gen 1 | ARM32 | ARMHF Build |
| Akai MPC One G2 | MPC Gen 2 | ARM64 | AARCH64 Build |
| Akai MPC X | MPC Gen 1 | ARM32 | ARMHF Build |
| Akai MPC X Special Edition | MPC Gen 1 | ARM32 | ARMHF Build |
| Akai MPC XL | MPC Gen 2 | ARM64 | AARCH64 Build |

### 3.1 Compatibility Policy

A successful cross-compile is not proof of device compatibility.

Each hardware generation and supported firmware family must be validated independently. Release documentation and catalog information must accurately identify the devices and firmware versions tested.

The supported hardware families and build targets are documented here. Individual device and firmware verification should be determined from the corresponding release test records rather than inferred from architecture compatibility.

Current Open MPC compatibility guidance:

https://github.com/open-mpc/mpc-vst-plugins/blob/main/docs/COMPATIBILITY.md

Gen 2 guidance:

https://github.com/open-mpc/mpc-vst-plugins/blob/main/docs/GEN2.md

OS 2.x skin guidance:

https://github.com/open-mpc/mpc-vst-plugins/blob/main/docs/OS2_SKINS.md

---

## 4. Signal Processing Architecture

CTRL Mixbus combines stereo mixing, equalization, filtering, dynamics, and loudness processing in one plugin.

The core processing chain is organized around the following stages:

1. Input Gain
2. Phase Inversion
3. Stereo Width
4. Bass Mono
5. Balance
6. Three-Band Isolator EQ
7. Low-Pass Filter
8. High-Pass Filter
9. Noise Gate
10. Maximizer

Every applicable processing stage has an independent ON/OFF bypass control.

The following sections describe the processing controls and their intended behavior.

### 4.1 Input Gain

- Adjustable gain: -∞ dB to +24 dB
- Independent bypass
- Stereo input metering

Input Gain controls the signal level entering the processing chain

### 4.2 Phase Inversion

- One switch inverts both left and right channels simultaneously
- Independent bypass


### 4.3 Stereo Width

- Adjustable from normal stereo width through to mono
- Mid/Side-based processing
- Independent bypass

Stereo Width adjusts the relationship between the mid and side components of the stereo signal.

### 4.4 Bass Mono

- Adjustable crossover frequency
- Low-frequency stereo information is summed to mono
- Stereo information above the crossover is retained
- Independent bypass

Bass Mono provides control over low-frequency stereo content while retaining stereo information above the selected crossover.

### 4.5 Balance

- Adjustable left/right balance
- Center position maintains equal channel levels
- Independent bypass

Balance adjusts the relative levels of the left and right channels.

### 4.6 Three-Band DJ EQ

The Isolator mode utilizes three bands split by phase-aligned LR4 Crossovers, cutting to full kill or boosting to +6DB.

- LOW
-- X-Over - 250Hz
- MID/HIGH
-- X-Over - 2,500Hz


The EQ mode utilizes a low shelf, mid bell, and high shelf, cutting to -26DB or boosting to +6DB.

- LOW
-- X-Over - 250Hz
- MID
-- Bell, Q0.5 - 1,000Hz
- HIGH
-- X-Over - 2,500Hz



### 4.7 Low-Pass Filter

The adjustable low-pass filter provides:

- Cutoff frequency
- Resonance
- Selectable curve:
  - 12 dB/octave
  - 24 dB/octave
  - 36 dB/octave
  - 48 dB/octave
  - 96 dB/octave
  - Brickwall
- Independent bypass


### 4.8 High-Pass Filter

The adjustable high-pass filter provides:

- Cutoff frequency
- Resonance
- Selectable curve:
  - 12 dB/octave
  - 24 dB/octave
  - 36 dB/octave
  - 48 dB/octave
  - 96 dB/octave
  - Brickwall
- Independent bypass


### 4.9 Noise Gate

Configurable stereo noise gate with:

- Threshold
- Attack
- Hold
- Release

Stereo-linked detection is used to help maintain stereo image stability.


### 4.10 Maximizer

CTRL Mixbus includes a Maximizer for loudness maximization with:

- Threshold
- Attack
- Release
- Output

The Maximizer provides visual activity and gain-reduction feedback through the plugin interface.

---

## 5. MPC User Interface

CTRL Mixbus includes a dedicated native MPC touchscreen interface designed around the MPC display and Q-Link layout.

The production interface is organized into three pages:

1. INPUT
2. FILTER
3. OUTPUT


### 5.1 Q-Link Rules

- No more than 16 rotary controls on any individual page.
- All rotary parameters are controlled using the Q-Links.
- ON/OFF switches are touchscreen only and are not assigned to Q-Links (their Q-Link slots are left empty).
- Q-Link assignments follow the currently selected page.
- Displayed values and visual states update when parameters change.

Meters, graphs, labels, and touchscreen switches do not consume rotary assignments.

The vertical arrangement of four controls applies to rotary control groups. It does not require touchscreen switches or other interface elements to follow the same arrangement.

On the INPUT and FILTER pages, the bottom ON/OFF switch of every column sits at the same height.


### 5.2 Visual Feedback

The interface includes visual feedback for the plugin's processing and signal levels.

Visual elements include:

- Stereo input level meter.
- Stereo output level meter.
- Live filter response visualization.
- Visual indication of filter cutoff and resonance.
- Maximizer gain-reduction metering.
- Clearly visible ON/OFF state for processing stages.
- Current parameter values.
- Clip indication where applicable.

Visual updates must not block or disrupt the audio-rendering thread.

Where the MPC skin system has limitations, the implementation must use supported native page elements and artwork rather than assume arbitrary desktop-style drawing is available.

### 5.3 Skin Appearance

The CTRL Mixbus interface uses the Tony Alonso Tech visual theme.

The established color palette is:

| Interface Element | Color |
|---|---|
| Background | `#370038` |
| Text | `#feb9ff` |
| Borders | `#ffffff` |
| Knob bodies | `#8a52e8` |
| Knob indicators | `#ffffff` |

The DJ EQ knobs (High, Mid, Low) use a centre-origin ring: a dot at 12 o'clock marks 0 dB, and the ring grows to the left for a cut and to the right for a boost.

The theme should remain consistent across the MPC skin assets and other plugin interface assets where applicable.


### 5.74Skin Compatibility

The MPC skin is a first-class part of the plugin release.

The project accounts for the different skin requirements of MPC OS 2.x and MPC OS 3.x. If one skin package cannot support both, the release must include the appropriate variants or clearly state the firmware limitation.

Skin pages are developed and previewed using Open MPC Skin Studio and its documented tools.

---

## 6. Parameter Model and Project Recall

The parameter model is a core part of the plugin architecture.

The Open MPC parameter definition determines the parameter ordering used by MPC projects. Once a plugin has shipped, existing parameter positions must never be reordered or reused. New parameters may only be appended.

Requirements:

- Assign stable, unique parameter keys.
- Define ranges, defaults, units, and display behavior.
- Keep parameter ordering stable after release.
- Commit the generated `params.lock` file where required by the Open MPC workflow.
- Test parameter changes, automation, save, and restore.
- Ensure multiple plugin instances maintain independent state.

The plugin must restore its state correctly when an MPC project is saved, closed, and reopened.

Changes to the parameter model must preserve compatibility with existing projects and presets wherever possible.

---


## 7. Dependencies and Packaging

Required runtime dependencies are included in the plugin source and build packages wherever licensing and platform requirements permit.

Third-party engine and DSP source dependencies should be vendored into the repository rather than downloaded dynamically during a build. This supports reproducible builds and avoids network-dependent build steps.

The MPC plugin must not require users to independently install:

- Additional DSP libraries.
- External runtime libraries not already provided by the target environment.
- Background services.
- Separate processing applications.
- Additional audio-processing frameworks.

Any dependency that cannot legally or technically be bundled must be documented, including its version, license, and target-platform availability.

### 7.1 MPC Release Artifacts

The MPC release package follows the current Open MPC plugin packaging conventions.

Expected artifacts include:

- Native MPC VST2 shared library (`.so`).
- Plugin metadata and catalog information.
- MPC touchscreen skin resources.
- Parameter definitions and Q-Link/page configuration.
- Required plugin assets and vendored dependencies.
- Preset support and factory Init preset.
- Installation and removal scripts generated or required by the release workflow.
- Installation instructions.
- Release checksums.

The exact package structure follows the current Open MPC toolchain rather than a custom structure invented by CTRL Mixbus.

### 7.2 Desktop Release Artifacts

The desktop distributions are packaged separately from the MPC releases.

The production desktop targets are:

- Linux VST3.
- Windows VST3.

Desktop release packages must contain the appropriate VST3 plugin binary and any required supporting assets or installation documentation.

Desktop packaging must not be confused with the native MPC VST2 package structure.

### 7.3 Release Versions

The current production release is version **2.0.1** for the MPC distribution and **2.0.0** for the desktop VST3 distributions.

| Release Package | Version |
|---|---|
| MPC Gen 1 and Gen 2 | 2.0.1 |
| Linux VST3 | 2.0.0 |
| Windows VST3 | 2.0.0 |

The MPC distribution contains architecture-specific builds for the two MPC hardware generations.

---

## 8. Build, Test, Benchmark, and Release Workflow

CTRL Mixbus uses separate build and packaging workflows for the native MPC plugin and desktop VST3 distributions.

### 8.1 MPC Plugin Description

The MPC build configuration maintains:

- `vst.json`
- Parameter definition file.
- Layout file.
- Engine source and headers.
- Build configuration and compiler definitions.

The plugin UID, shared-library name, and parameter ordering must remain stable after release.

### 8.2 MPC Build

Use the Open MPC build script with the CTRL Mixbus `vst.json`:
