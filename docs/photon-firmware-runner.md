# Photon Firmware Runner

The `Build Photon Firmwares` workflow builds the MeshSmith Photon nRF52 and ESP32-C6 firmware variants on a self-hosted Linux GitHub Actions runner.

## Runner Labels

Register the runner in this repository with these labels:

- `self-hosted`
- `linux`
- `photon-firmware`

GitHub automatically adds `self-hosted` and the operating-system label. Add `photon-firmware` as the custom label so the workflow can pick the runner.

## Workflow

Run `.github/workflows/build-photon-firmwares.yml` manually from the Actions tab. The optional `firmware_version` input controls the artifact filenames. If it is omitted, the workflow falls back to the latest tag and then to `manual-<sha>`.

By default, each successful build publishes two GitHub Releases:

- nRF52: `photon-nrf52-<branch>-<version>`
- ESP32-C6: `photon-esp32-c6-<branch>-<version>`

Override `release_tag` to set a shared base tag, which the workflow publishes as `<release_tag>-nrf52` and `<release_tag>-esp32-c6`. Override `release_name` to set a shared base title, which the workflow publishes with `nRF52` and `ESP32-C6` appended. Disable `publish_release` from the manual workflow form when needed.

The workflow targets the `meshsmith_photon_nrf52...` and `meshsmith_photon_esp32c6...` PlatformIO environments and uploads artifacts from `out/`:

- `Photon-nRF52-Companion-BLE-<version>.uf2`
- `Photon-nRF52-Companion-BLE-<version>.zip`
- `Photon-nRF52-Companion-USB-<version>.uf2`
- `Photon-nRF52-Companion-USB-<version>.zip`
- `Photon-nRF52-Repeater-<version>.uf2`
- `Photon-nRF52-Repeater-<version>.zip`
- `Photon-nRF52-Repeater-<version>-logging.uf2`
- `Photon-nRF52-Repeater-<version>-logging.zip`
- `Photon-nRF52-Room-Server-<version>.uf2`
- `Photon-nRF52-Room-Server-<version>.zip`
- `Photon-nRF52-Room-Server-<version>-logging.uf2`
- `Photon-nRF52-Room-Server-<version>-logging.zip`
- `Photon-ESP32-C6-Companion-BLE-<version>.bin`
- `Photon-ESP32-C6-Companion-BLE-<version>-merged.bin`
- `Photon-ESP32-C6-Companion-USB-<version>.bin`
- `Photon-ESP32-C6-Companion-USB-<version>-merged.bin`
- `Photon-ESP32-C6-Repeater-<version>.bin`
- `Photon-ESP32-C6-Repeater-<version>-merged.bin`
- `Photon-ESP32-C6-Repeater-<version>-logging.bin`
- `Photon-ESP32-C6-Repeater-<version>-logging-merged.bin`
- `Photon-ESP32-C6-Room-Server-<version>.bin`
- `Photon-ESP32-C6-Room-Server-<version>-merged.bin`
- `Photon-ESP32-C6-Room-Server-<version>-logging.bin`
- `Photon-ESP32-C6-Room-Server-<version>-logging-merged.bin`
- `photon-nrf52-firmware-manifest.json`
- `photon-esp32-c6-firmware-manifest.json`

The manifests are intended for tools such as a web flasher. A flasher can query GitHub Releases, filter tags that start with `photon-nrf52-` or `photon-esp32-c6-`, and read the matching manifest asset to display available firmware files.

## Runner Host Notes

Use a Linux host with enough disk space for PlatformIO build caches. The workflow installs Python 3.11 and PlatformIO through the shared `setup-build-environment` action, so the host mainly needs a working shell, GitHub Actions runner service, Python setup support, and internet access for dependency downloads.
