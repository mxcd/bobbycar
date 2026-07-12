# PRODUCT.md — bobbycar onboard tool

## Product purpose

Unified onboard tool for a converted Bobby Car (children's ride-on toy) with a
VESC dual-motor drivetrain: data logging (.bblog), live telemetry, firmware
OTA for the STM32 system controller. Served by a Raspberry Pi in the vehicle,
reachable over Tailscale.

## Users

One builder/driver/mechanic (MaPa, 15+ years engineering) plus friends and
family who drive the car. Primary usage: standing next to the car with a
phone (driveway, daylight) checking state of charge, grabbing the log of the
last run, flashing firmware; secondary: desk review of logs in the evening.

## Register

product

## Tone

Serious engineering wrapped around an absurd, joyful object. The tension IS
the identity: a children's toy with a 50V pack, CRC-checked serial protocols
and a dead man's switch. Precise, technical, never corporate. Lowercase
"bobbycar" wordmark.

## Brand

The car itself is bobby-car red. Neutrals are warm (tinted toward the red
hue), the accent is the car's red — used for identity, primary actions and
destructive/danger semantics (they coincide here: flashing firmware IS the
dangerous primary action). Status semantics stay green/amber.

## Anti-references

- Generic SaaS admin templates, hero-metric tiles, gradient dashboards
- "Telemetry tool = neon on dark blue" category reflex
- Anything that hides data behind decoration; density is a feature
