# JED-Light-Baker
JED/SED Light Baker Plugin. Bakes lighting using simple GPU accelerated ray tracing.

<img width="717" height="617" alt="image" src="https://github.com/user-attachments/assets/a323245a-a864-4fa4-93bc-290bdca37dfa" />

# Requirements:
- JED/NJED/SED level editor of choice (not some of them have bugs with RGB lighting)
- A copy of Jedi Knight/Mysteries of the Sith
- A DX11 Capable PC

# Usage:

Sunlight: place a light, set flag "0x2" (sun flag). Controls sun direction, color and intensity. Move it around the world origin (or anchor, see below) to control the direction of the sun light. Range has no effect.
Skylight: place a light, set flag "0x4" (sky flag). Controls sky color and intensity. Position and range have no effect.
Anchor: place a light, set flag "0x8" (sun anchor). The sun light will orbit this light instead of the world origin. All settings except position do nothing.
