### Offline Installation

## Prerequisites

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

### Manual Uninstallation

In some cases, Plugin Manager may not recognize the uninstaller or available updates for plugins installed offline. Fortunately, there is a workaround that allows you to manually uninstall the plugin using its installation script.

## Uninstallation Steps

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