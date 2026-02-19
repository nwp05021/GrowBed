# GrowBed v0.6.0-platform (refactor scaffold)

Added platform capability envelope foundation:
- src/platform/envelope: Envelope model + BedLinkBinaryCodec
- src/platform/capability: capId/msgId constants
- src/product/growbed/GrowBedNode: capId routing skeleton

MotionController remains unchanged.
Next: add RS485 adapter + map motion.linear commands to controller operations.
