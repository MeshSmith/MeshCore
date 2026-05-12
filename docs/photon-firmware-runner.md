# Photon Firmware Runner

The `Build Photon Firmwares` workflow builds the MeshSmith Photon ESP32-C6 firmware variants on a self-hosted Linux GitHub Actions runner.

## Runner Labels

Register the runner in this repository with these labels:

- `self-hosted`
- `linux`
- `photon-firmware`

GitHub automatically adds `self-hosted` and the operating-system label. Add `photon-firmware` as the custom label so the workflow can pick the runner.

## Workflow

Run `.github/workflows/build-photon-firmwares.yml` manually from the Actions tab. The optional `firmware_version` input controls the artifact filenames. If it is omitted, the workflow falls back to the latest tag and then to `manual-<sha>`.

By default, each successful build also publishes a GitHub Release. The release tag defaults to `photon-esp32-c6-<branch>-<version>`, so builds from separate Photon ESP32-C6 release branches do not collide. Override `release_tag` or disable `publish_release` from the manual workflow form when needed.

The workflow targets the `meshsmith_photon_esp32c6...` PlatformIO environments and uploads BIN and merged BIN artifacts from `out/`:

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
- `photon-esp32-c6-firmware-manifest.json`

The manifest is intended for tools such as a web flasher. A flasher can query GitHub Releases, filter tags that start with `photon-esp32-c6-`, and read this manifest asset to display available firmware files.

## Runner Host Notes

Use a Linux host with enough disk space for PlatformIO build caches. The workflow installs Python 3.11 and PlatformIO through the shared `setup-build-environment` action, so the host mainly needs a working shell, GitHub Actions runner service, Python setup support, and internet access for dependency downloads.
