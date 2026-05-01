# Photon Firmware Runner

The `Build Photon Firmwares` workflow builds the MeshSmith Photon firmware variants on a self-hosted Linux GitHub Actions runner.

## Runner Labels

Register the runner in this repository with these labels:

- `self-hosted`
- `linux`
- `washtastic-firmware`

GitHub automatically adds `self-hosted` and the operating-system label. Add `washtastic-firmware` as the custom label so the workflow can pick the runner.

## Workflow

Run `.github/workflows/build-photon-firmwares.yml` manually from the Actions tab. The optional `firmware_version` input controls the artifact filenames. If it is omitted, the workflow falls back to the latest tag and then to `manual-<sha>`.

The workflow uploads UF2 and ZIP artifacts from `out/`:

- `Photon-Companion-BLE-<version>.uf2`
- `Photon-Companion-BLE-<version>.zip`
- `Photon-Companion-USB-<version>.uf2`
- `Photon-Companion-USB-<version>.zip`
- `Photon-Repeater-<version>.uf2`
- `Photon-Repeater-<version>.zip`
- `Photon-Repeater-<version>-logging.uf2`
- `Photon-Repeater-<version>-logging.zip`
- `Photon-Room-Server-<version>.uf2`
- `Photon-Room-Server-<version>.zip`
- `Photon-Room-Server-<version>-logging.uf2`
- `Photon-Room-Server-<version>-logging.zip`

## Runner Host Notes

Use a Linux host with enough disk space for PlatformIO build caches. The workflow installs Python 3.11 and PlatformIO through the shared `setup-build-environment` action, so the host mainly needs a working shell, GitHub Actions runner service, Python setup support, and internet access for dependency downloads.
