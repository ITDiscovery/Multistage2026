# Multistage2026 Flight Manual & Configuration Reference
Updated Multistage Vessel Module for OpenOrbiter - Linux Only

This is "forked" from [Matias Saibene's Github] (https://github.com/MatiasSaibene/Multistage2015_for_OpenOrbiter) and using Gemini Pro, ported
to OpenOrbiter for Linux. All the developer tools were stripped out do to the graphics nature would have made it difficult. My goal is to fly the
Artemis II mission using OpenOrbiter for Linux from thegondors.

This document outlines the control inputs, guidance automation, and staging logic for Multistage2026 vessels in OpenOrbiter.

## 1. File Layout
Multistage is a vessel, and is called by the SCN file. 
```text
BEGIN SHIPS
SLS_Artemis_II:Multistage2026
  STATUS Landed Earth
  POS -80.6209100 28.6271780
  HEADING 180.00
  ALT 50.0
  AROT 150.00 -10.00 0
  AFCMODE 7
  PRPLEVEL 0:1.000 1:1.000 2:1.000 3:1.000
  NAVFREQ 0 0
  CONFIG_FILE Config/Multistage2026/SLS_Block1.ini
  GUIDANCE_FILE Config/Multistage2026/Guidance/SLS_Lunar.txt
END
END_SHIPS
```

---

## 1. Key Bindings (Manual Control)

The following key commands are defined in the module's input buffer (`clbkConsumeBufferedKey`) and are available when the vessel has focus.

### Flight Controls

| Key | Action | Logic / Behavior |
| :--- | :--- | :--- |
| **J** | **Jettison / Staging** | **Sequential Logic:**<br>1. **Boosters:** If active boosters remain, they are jettisoned.<br>2. **Stages:** If boosters are gone, the active stage (or Interstage) is jettisoned.<br>3. **Payload:** If stages are depleted, the Payload is released.<br>**Constraint:** Payload jettison is *blocked* until the Fairing/LES is gone (`wFairing == 0`). You must press **F** before **J** works for the final payload release. |
| **F** | **Jettison LES / Fairing** | **Priority Logic:**<br>1. **LES:** If a Launch Escape System (`wLes`) is defined, it jettisons first.<br>2. **Fairing:** If the LES is gone (or undefined), it jettisons the Fairing (`wFairing`). |
| **P** | **Toggle Autopilot** | Toggles the internal Gravity Turn / Pitch Program (`APstat`). Disabling this allows for full manual control. |
| **G** | **Landing Gear** | Toggles landing legs or gear animations (if defined). |

### Ground & Hangar Operations

| Key | Modifiers | Action | Logic / Notes |
| :--- | :--- | :--- | :--- |
| **L** | **CTRL** | **Hangar Light** | Sends signal to Hangar vessel (if `HangarMode` active). |
| **D** | **CTRL** | **Hangar Door** | Sends signal to Hangar vessel. Also attempts to mechanically attach the rocket to the `MS_Pad` vessel. |

### Developer Mode (Debug)

*Note: Windows-dependent dialogs are disabled in Linux ports, but the hot-reload function may still operate.*

* **CTRL + SPACE**: Enable Developer Mode (Toggles on-screen annotation).
* **SPACE**: **Hot Reload**. Triggers `ResetVehicle`, instantly reloading the `.ini` configuration file without restarting the simulator.

---

## 2. Guidance File Format (`.txt`)

The autopilot reads a space-delimited text file (e.g., `Config/Multistage2026/Guidance/SLS_Lunar.txt`) to control the ascent pitch and heading.

**Syntax:**
The parser expects four columns of floating-point numbers. Lines starting with `;`, `/`, or `SPACE` are ignored as comments.

```text
; TIME    PITCH    HEADING    THROTTLE
  -5.0    90.0     90.0       1.0
  10.0    90.0     90.0       1.0
  130.0   45.0     90.0       1.0
  480.0   0.0      90.0       1.0
```
Columns:

TIME (MET): Mission Elapsed Time in seconds.

Note: Use negative values (e.g., -5.0) to set the initial state on the pad.

PITCH (Deg): Target pitch angle relative to the Horizon.

90.0 = Vertical (Up).

0.0 = Horizontal (Level flight).

HEADING (Deg): Target compass heading (Launch Azimuth).

90.0 = East.

0.0 = North.

THROTTLE (0.0 - 1.0): Main engine thrust setting.

1.0 = 100%.

0.65 = 65% (e.g., for Max-Q).

0.0 = Engine Cutoff.

Behavior:

The autopilot linearly interpolates between the defined points.

This file DOES NOT control staging events.

3. Staging Logic (Event-Driven)
Multistage2026 does not use "Time-to-Stage" commands. Staging is strictly event-driven based on propellant depletion.

The Sequence:

Fuel Depletion: The active stage burns until its propellant mass reaches 0.0.

Engine Cutoff (MECO): The engine stops automatically.

Separation Event: The module detects the empty tank and triggers separation.

The empty stage is jettisoned using the SPEED vector defined in the .ini.

The next stage (or Interstage) becomes active.

Ignition: The next stage ignites (unless a delay is defined).

Interstages: If an [INTERSTAGE] section exists, the sequence pauses after the lower stage drops. The Interstage ring remains attached to the upper stage until a specific separation command (manual J or auto-sequence) jettisons the ring, allowing the upper engine to fire.


---
## Multistage2026 INI Configuration Reference

This document outlines the standard configuration sections and parameters for defining a multistage vessel (e.g., SLS) in OpenOrbiter.

### 1. [MISC]
*General vessel configuration and initial state.*

* **`COG`**: `(float)`
    * Vertical position of the Center of Gravity (offset from the mesh center).
* **`G_TURN_PITCH`**: `(float)`
    * The initial pitch angle for the gravity turn (e.g., `89.5` for a slight initial tilt).
* **`PAD_MODULE`**: `(0 or 1)`
    * Legacy setting. `1` implies a separate launch pad vessel is used. (Modern configs often rely on `TOUCHDOWN_POINTS` instead).

---

### 2. [TEXTURE_LIST]
*Pre-loads textures into memory so other sections can reference them by index number.*

* **`TEX_1`**: `(string)`
    * Filename of the texture (do not include `.dds`).
    * *Example:* `TEX_1=Exhaust_atsme`
* **`TEX_2`**: `(string)`
    * Filename for the second texture, etc.

---

### 3. [TOUCHDOWN_POINTS]
*Defines the physics legs. Essential for keeping the rocket sitting on the pad rather than sinking ("Submarine" issue).*

* **`POS_1`**: `(Vector: x, y, z)`
    * Front/Center contact point.
    * *Example:* `(0, 65.0, -10.0)` — `Y` is the vertical length of the "leg".
* **`POS_2`**: `(Vector: x, y, z)`
    * Rear-Left contact point.
* **`POS_3`**: `(Vector: x, y, z)`
    * Rear-Right contact point.

---

### 4. [STAGE_n]
*Defines a core stage. Replace `n` with the stage number (e.g., `[STAGE_1]`, `[STAGE_2]`).*

* **`MESHNAME`**: `(string)`
    * Path to the visual mesh file (relative to `Meshes/`).
* **`OFF`**: `(Vector: x, y, z)`
    * **CRITICAL**: The visual offset of the mesh relative to the vessel's Center of Gravity.
    * *Note:* Incorrect values here cause the "Exploded Rocket" look.
* **`HEIGHT`**: `(float)`
    * Stage height (used for aerodynamic drag calculations).
* **`DIAMETER`**: `(float)`
    * Stage width (used for aerodynamic drag calculations).
* **`MASS`**: `(float)`
    * Dry mass of the stage in kg.
* **`FUELMASS`**: `(float)`
    * Mass of the fuel in kg.
* **`THRUST`**: `(float)`
    * Total thrust in Newtons.
* **`BURNTIME`**: `(float)`
    * Burn duration in seconds. (Auto-calculated if Thrust/Fuel are set, but can be forced here).
* **`ENG_1`**, **`ENG_2`**...: `(Vector: x, y, z)`
    * Location of the engine nozzle(s) where thrust is applied.
* **`ENG_DIAMETER`**: `(float)`
    * Visual diameter of the engine bells.
* **`ENG_PSTREAM1`**: `(integer)`
    * ID of the `[PARTICLESTREAM_n]` to use for this stage's exhaust.
* **`SPEED`**: `(Vector: x, y, z)`
    * Separation velocity vector (how fast the stage is pushed away when jettisoned).

---

### 5. [BOOSTER_n]
*Defines strap-on boosters (SRBs).*

* **`MESHNAME`**: `(string)`
    * Path to the visual mesh.
* **`OFF`**: `(Vector: x, y, z)`
    * Position relative to the Core.
* **`MASS`, `FUELMASS`, `THRUST`, `BURNTIME`**:
    * (Same definitions as `[STAGE_n]`).
* **`ENG_1`**: `(Vector: x, y, z)`
    * **CRITICAL**: The coordinate where the exhaust flame starts.
    * *Troubleshooting:* If exhaust floats in mid-air, adjust the `Z` value here (or in the `PARTICLESTREAM` source).
* **`ENG_PSTREAM1`**: `(integer)`
    * Link to the particle definition.

---

### 6. [PARTICLESTREAM_n]
*Defines the look of the exhaust plume.*

* **`NAME`**: `(string)`
    * Friendly name for reference.
* **`SRCSIZE`**: `(float)`
    * Initial size of the flame at the nozzle.
    * *Tuning:* Reduce this (e.g., `2.0` - `3.0`) to fix "Marshmallow" effects.
* **`GROWTHRATE`**: `(float)`
    * How fast the smoke expands as it trails behind.
    * *Tuning:* Reduce this (e.g., `8.0`) for a tighter, faster-looking jet.
* **`ATEX`**: `(integer)`
    * Texture ID from `[TEXTURE_LIST]` (e.g., `1`).
* **`SRC`**: `(Vector: x, y, z)`
    * Hardcoded offset for the smoke origin. Use this to align the plume exactly with the SRB nozzles.
* **`V0`**: `(float)`
    * Ejection velocity (speed of the smoke particles).

---

### 7. [PAYLOAD_n]
*Defines the cargo (e.g., Orion).*

* **`MESHNAME`**: `(string)`
    * Visual mesh of the payload.
* **`OFF`**: `(Vector: x, y, z)`
    * Position where it sits on top of the stack.
* **`MASS`**: `(float)`
    * Payload mass.
* **`MODULE`**: `(string)`
    * The DLL name for the vessel to be spawned.
    * *Example:* `Spacecraft\Orion` launches the Orion vessel logic on separation.
* **`NAME`**: `(string)`
    * The name the new vessel will have when it spawns (e.g., "Orion").

---

### 8. [LES]
*Launch Escape System (The tower on top).*

* **`MESHNAME`**: `(string)`
    * Mesh file for the tower (e.g., `Orion-MPCV\orion-las`).
* **`OFF`**: `(Vector: x, y, z)`
    * Offset relative to the **Payload**, not the ground.
* **`ARM_TIME`**: `(float)`
    * Time (MET) when the system becomes active.
* **`JETTISON_TIME`**: `(float)`
    * Time (MET) when the tower is automatically jettisoned.

---

### 9. [ADAPTER]
*Connecting hardware (e.g., Stage Adapter).*

* **`MESHNAME`**: `(string)`
    * Visual mesh.
* **`OFF`**: `(Vector: x, y, z)`
    * Position relative to the stack.
* **`MASS`**: `(float)`
    * Adapter mass.
* **`SEPARATION_DELAY`**: `(float)`
    * Delay in seconds between main staging and adapter release.
