# GrowBed v0.6.0-platform (refactor scaffold)

Non-blocking reciprocal motion baseline for GrowBed.

## Hardware
- Raspberry Pi Pico (Arduino framework in PlatformIO)
- DRV8825 (STEP/DIR/EN)
- NEMA17
- Hall endstops Left/Right (active-low)

## Run
1) Edit `src/config/PinMap.h`
2) Flash
3) Serial monitor @115200

## Notes
- DRV8825 EN assumed active-low (enabled => EN=LOW)
- Set current limit (Vref) before long runs

Added platform capability envelope foundation:
- src/platform/envelope: Envelope model + BedLinkBinaryCodec
- src/platform/capability: capId/msgId constants
- src/product/growbed/GrowBedNode: capId routing skeleton

MotionController remains unchanged.
Next: add RS485 adapter + map motion.linear commands to controller operations.
