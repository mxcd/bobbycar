# bob-e-car logger

Unified onboard tool for the bob-e-car: data logging, live telemetry, and
firmware OTA for the system controller. Runs on a Raspberry Pi wired to the
system controller (Ethernet for telemetry, USB to the Nucleo's ST-LINK), or
on a laptop for development.

## What it does

- Listens for the system controller's telemetry broadcasts (`:4242`,
  SO_REUSEPORT — coexists with the telemetry-server on the same host)
- Writes every received packet unmodified as varint-length-delimited protobuf
  `LogRecord`s (`proto/logfile.proto`) to `.bblog` files — no resampling on write
- Log metadata (filename, start/end, size, record count, trigger) in SQLite
- Dashboard on `:8090`: connection/stream status, trigger config, log list
  (rename/download/delete), log viewer with dynamic server-side resampling
  (zoom/pan refetches the visible window at ~2 points per pixel, fsw-dl style)
- **Flags**: the steering wheel "R" button (`steeringAccelerationCommand`)
  marks moments in the log. Markers are derived at read time from rising
  edges — a held press counts once, and old logs get flags retroactively —
  and drawn as ⚑ lines in the viewer regardless of channel selection. In dms
  mode a flag press also triggers recording, pre-roll included.
- **Live telemetry** at `/live/` (same dashboard as the telemetry-server):
  every packet is re-broadcast as protojson on the `/ws` websocket. Decoding
  happens on its own goroutine — a slow viewer never stalls logging.
- **Firmware OTA** for the system controller via the Nucleo's onboard ST-LINK
  on the Pi's USB port (`st-flash` from stlink-tools)

## Firmware OTA

Dashboard: probe ST-LINK, pick a `.bin`, flash. Or from `system-controller/`:

```bash
just ota            # pio run + POST firmware.bin to http://logger:8090
just ota bob-pi     # other host (Tailscale name)
```

API: `GET /api/ota/probe`, `POST /api/ota/flash` with the raw `.bin` as
body. The flash is refused with 409 while telemetry shows the vehicle
outside IDLE — append `?force=true` to override. Requires
`apt install stlink-tools` on the Pi (ships the udev rules for non-root USB
access; re-plug the Nucleo after installing).

**The ST-LINK can reset the running controller.** Probe therefore only
checks USB presence via sysfs and never attaches to SWD (`st-info --probe`
halts/resets the target); only an actual flash touches the MCU. The Pi's
boot-time USB power cycle still restarts the ST-LINK, which yanks the
target's NRST — remove solder bridge **SB111** on the Nucleo-144 (MB1137,
UM1974) to disconnect ST-LINK reset from the target for good; `st-flash`
then resets via SWD (AIRCR) after flashing instead.

## Trigger modes (configurable in the dashboard)

- **always** — one file per run, recording starts with the first packet
- **dms** — ring buffer holds the last n seconds (default 10); DMS press opens
  the file pre-filled with the buffer, and it closes only after n seconds
  without a DMS press — a brief accidental release never splits the log

## Usage

```bash
just run        # logger on the laptop (udp :4242, http :8090)
just sim        # fake system controller incl. DMS cycle with 1s blip
just build      # bin/logger + bin/simulator
just build-pi   # CGO-free linux/arm (ARMv7) build for the Pi 2
just deploy     # build-pi + scp to the Pi + restart bob-logger.service
just test       # trigger + resampling tests
just gen        # regenerate protobuf (after editing proto/logfile.proto)
```

Flags (env fallback): `-log-dir` (`LOGGER_LOG_DIR`, ./logs), `-db`
(`LOGGER_DB`, ./logs.sqlite), `-udp` (`LOGGER_UDP`, :4242), `-http`
(`LOGGER_HTTP`, :8090). A `.env` file in the working directory is loaded
into the environment on startup (mxcd/go-config).

## Login

`LOGGER_USERNAME` / `LOGGER_PASSWORD` (`.env`, see `.env.example`; on the Pi
in `/etc/bob-logger.env`) enable the dashboard login — form login with a
session cookie persisted in SQLite (30 days; survives the Pi's power cycle
with the car), logout in the header of both pages. API clients can use HTTP
basic auth with the same credentials instead (`just ota` sends
`$LOGGER_AUTH` as `user:pass`). Both unset = auth disabled for local dev.

## Pi setup

The logger Pi (reachable via `ssh logger`) runs `bob-logger.service` (unit in
`deploy/`, binary at `/usr/local/bin/bob-logger`, data in
`/var/lib/bob-logger`). `just deploy` (Pi 2, ARMv7) or `just deploy-pi4`
(Pi 4, arm64) builds and ships a new binary.

eth0 has static `10.42.10.223/24` — the system controller (`10.42.10.130`)
broadcasts to `10.42.10.255:4242` over a direct cable. The Pi 4 uses
systemd-networkd (`/etc/systemd/network/30-eth0.network`): static address
with `ConfigureWithoutCarrier=yes` (logging-ready before the link is). The
vehicle LAN is deliberately its own subnet, distinct from any home/workshop
WiFi — with overlapping subnets the eth0 prefix route hijacks replies to
WiFi-side peers (including tailscale's direct path). eth0 on plain
`DHCP=ipv4` never gets an address on this link — there is no DHCP server in
the car.

### Pi 4 + Tailscale (remote access)

The Pi 4 additionally joins the internet over built-in WiFi (or a phone
hotspot, via wpa_supplicant + systemd-networkd) and exposes the dashboard
through Tailscale:

```bash
sudo apt install stlink-tools                       # OTA flashing
curl -fsSL https://tailscale.com/install.sh | sh
sudo tailscale up                                    # auth once, key survives reboots
```

As soon as the Pi has internet, `http://<tailscale-name>:8090` serves logs,
live telemetry, and OTA from anywhere in the tailnet. eth0 keeps its static
vehicle IP; WiFi is only the uplink.

### Pi 2 boot tuning (legacy)

Boot is tuned for fast availability: cloud-init, console/keyboard-setup,
bluetooth/wpa_supplicant (no such hardware), avahi, udisks2,
rpi-eeprom-update and NetworkManager-wait-online are disabled; `config.txt`
has `disable_splash=1`, `boot_delay=0`, `initial_turbo=30`; kernel cmdline
has `quiet`.
