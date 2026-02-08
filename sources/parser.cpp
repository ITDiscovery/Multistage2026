/****************************************************************************
  This file is a derivative of Multistage2015 project, refactored to
  Multistage2026 to run on OpenOrbiter for Linux only.

  Copyright for Multistage2015 belogs to Fred18 for module
  implementation and its code. Biggest Credit goes to Vinka for his idea
  of Multistage.dll. None of his code was used here since his addons are
  all closed source. Credit goes to Face for having pointed me to the
  GetPrivateProfileString. Credit goes to Hlynkacg for his OrientForBurn
  function which was the basis on which I developed the Attitude Function.

  Multistage2015 and Multistage2026 is distributed FREEWARE. Its code is
  distributed with out the compiled library. Nobody is authorized to exploit
  the module or the code or parts of them commercially directly or indirectly.

  You CAN distribute the dll together with your addon but in this case you MUST:
  -     Include credit to the author in your addon documentation;
  -     Add to the addon documentation the official link of Orbit Hangar Mods for
        download and suggest to download the latest and updated version of the module.
  You CAN use parts of the code of Multistage2015, but in this case you MUST:
  -     Give credits in your copyright header and in your documentation for
        the part you used.
  -     Let your project be open source and its code available for at least
        visualization by other users.
  You CAN NOT use the entire code for making and distributing the very same module
  claiming it as your work entirely or partly.
  You CAN NOT claim that Multistage2015 is an invention of yourself or a work made
  up by yourself, or anyhow let intend that is not made and coded by the author.
  You install and use Multistage2015 or Multistage2026 at your own risk, authors
  will not be responsible for any claim or damage subsequent to its use or for 
  the use of part of it or of part of its code.

  ==== MultiStage2015 By Fred18
  ==== MultiStage2026 refactor by ITDiscovery.info
*/

#include <math.h>
#include <stdio.h>
#include <string>
#include <vector>
#include "Multistage2026.h"
#include "SimpleIni.h" // Ensure this path matches your project structure

// Helper to convert "x,y,z" strings from INI to VECTOR3
// (Assuming this helper exists in your project, if not we can add it)
extern VECTOR3 CharToVec(const std::string& str);
extern VECTOR4 CharToVec4(const std::string& str);

// ==============================================================
// String Parsing Helpers
// ==============================================================

VECTOR3 CharToVec(const std::string& str) {
    VECTOR3 v = {0,0,0};
    if (str.empty()) return v;
    sscanf(str.c_str(), "%lf,%lf,%lf", &v.x, &v.y, &v.z);
    return v;
}

VECTOR4 CharToVec4(const std::string& str) {
    VECTOR4 v = {0,0,0,0};
    if (str.empty()) return v;
    sscanf(str.c_str(), "%lf,%lf,%lf,%lf", &v.x, &v.y, &v.z, &v.t);
    return v;
}

// ==============================================================
// Interstage Parsing
// ==============================================================

void Multistage2026::parseInterstages(char* filename, int parsingstage) {
    CSimpleIniA ini;
    ini.SetUnicode();

    if (ini.LoadFile(filename) < 0) {
        oapiWriteLogV("%s: Failed to load INI configuration file: %s", GetName(), filename);
        return;
    }

    char sectionName[64];
    snprintf(sectionName, sizeof(sectionName), "SEPARATION_%i-%i", parsingstage, parsingstage + 1);

    // Check if section exists
    if (!ini.GetSection(sectionName)) {
        return; // No interstage for this stage
    }

    // Retrieve Meshname (Critical check)
    const char* meshname = ini.GetValue(sectionName, "meshname", "");
    if (strlen(meshname) > 0) {

        stage.at(parsingstage).wInter = TRUE;
        stage.at(parsingstage).IntIncremental = parsingstage + 1; // Logic from original code

        // Strings
        stage.at(parsingstage).interstage.meshname = std::string(meshname);
        stage.at(parsingstage).interstage.module = ini.GetValue(sectionName, "module", "Stage");

        // Vectors
        stage.at(parsingstage).interstage.off = CharToVec(ini.GetValue(sectionName, "off", "0,0,0"));
        stage.at(parsingstage).interstage.speed = CharToVec(ini.GetValue(sectionName, "speed", "0,0,0"));
        stage.at(parsingstage).interstage.rot_speed = CharToVec(ini.GetValue(sectionName, "rot_speed", "0,0,0"));

        // Floats
        stage.at(parsingstage).interstage.height = ini.GetDoubleValue(sectionName, "height", 0.0);
        stage.at(parsingstage).interstage.diameter = ini.GetDoubleValue(sectionName, "diameter", 0.0);
        stage.at(parsingstage).interstage.emptymass = ini.GetDoubleValue(sectionName, "emptymass", 0.0);
        stage.at(parsingstage).interstage.separation_delay = ini.GetDoubleValue(sectionName, "separation_delay", 0.0);

        // Initialize current delay
        stage.at(parsingstage).interstage.currDelay = stage.at(parsingstage).interstage.separation_delay;

        nInterstages++;
        oapiWriteLogV("%s: Interstage found for Stage %i (Mesh: %s)", GetName(), parsingstage + 1, meshname);
    }
}

// ==============================================================
// Launch Escape System (LES) Parsing
// ==============================================================

void Multistage2026::parseLes(char* filename) {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(filename) < 0) return;

    const char* sectionName = "LES";

    if (!ini.GetSection(sectionName)) return;

    const char* meshname = ini.GetValue(sectionName, "meshname", "");
    if (strlen(meshname) > 0) {
        wLes = TRUE;
        Les.meshname = std::string(meshname);
        Les.module = ini.GetValue(sectionName, "module", "Stage");

        Les.off = CharToVec(ini.GetValue(sectionName, "off", "0,0,0"));
        Les.speed = CharToVec(ini.GetValue(sectionName, "speed", "0,0,0"));
        Les.rot_speed = CharToVec(ini.GetValue(sectionName, "rot_speed", "0,0,0"));

        Les.height = ini.GetDoubleValue(sectionName, "height", 0.0);
        Les.diameter = ini.GetDoubleValue(sectionName, "diameter", 0.0);
        Les.emptymass = ini.GetDoubleValue(sectionName, "emptymass", 0.0);

        oapiWriteLogV("%s: LES System Found (Mesh: %s)", GetName(), meshname);
    }
}

// ==============================================================
// Adapter Parsing
// ==============================================================

void Multistage2026::parseAdapter(char* filename) {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(filename) < 0) return;

    // Adapters can be named "SEPARATION_N-N+1" (top stage) or just "ADAPTER"
    char sectionName[64];
    snprintf(sectionName, sizeof(sectionName), "SEPARATION_%i-%i", nStages, nStages + 1);

    const char* meshname = ini.GetValue(sectionName, "meshname", "");

    // Fallback check
    if (strlen(meshname) == 0) {
        strcpy(sectionName, "ADAPTER");
        meshname = ini.GetValue(sectionName, "meshname", "");
    }

    if (strlen(meshname) > 0) {
        wAdapter = TRUE;
        Adapter.meshname = std::string(meshname);

        Adapter.off = CharToVec(ini.GetValue(sectionName, "off", "0,0,0")); 
        Adapter.height = ini.GetDoubleValue(sectionName, "height", 0.0); 
        Adapter.diameter = ini.GetDoubleValue(sectionName, "diameter", 0.0); 
        Adapter.emptymass = ini.GetDoubleValue(sectionName, "emptymass", 0.0);

        oapiWriteLogV("%s: Adapter Found (Mesh: %s)", GetName(), meshname);
    }
}

// ==============================================================
// Stage Parsing (The Core Rocket)
// ==============================================================

void Multistage2026::parseStages(char* filename) {
    CSimpleIniA ini;
    ini.SetUnicode();

    if (ini.LoadFile(filename) < 0) {
        oapiWriteLogV("%s: Error loading INI file in parseStages: %s", GetName(), filename);
        return;
    }

    char sectionName[64];

    // Iterate through potential stages (Limit 10)
    for (int i = 0; i < 10; i++) {
        // 1. Check for Interstages attached to this stage index
        parseInterstages(filename, i);

        // 2. Setup Stage Section Name
        snprintf(sectionName, sizeof(sectionName), "STAGE_%i", i + 1);

        // If section doesn't exist, we assume we've reached the end of the rocket
        if (!ini.GetSection(sectionName)) {
            nStages = i;
            oapiWriteLogV("%s: Config loaded. Total Stages found: %i", GetName(), nStages);
            break;
        }

        // 3. Read Basic Parameters
        stage.at(i).meshname = std::string(ini.GetValue(sectionName, "meshname", ""));
        stage.at(i).module = std::string(ini.GetValue(sectionName, "module", "Stage"));
        stage.at(i).off = CharToVec(ini.GetValue(sectionName, "off", "0,0,0"));
        stage.at(i).speed = CharToVec(ini.GetValue(sectionName, "speed", "0,0,0"));
        stage.at(i).rot_speed = CharToVec(ini.GetValue(sectionName, "rot_speed", "0,0,0"));
        stage.at(i).height = ini.GetDoubleValue(sectionName, "height", 0.0);
        stage.at(i).diameter = ini.GetDoubleValue(sectionName, "diameter", 0.0);
        stage.at(i).thrust = ini.GetDoubleValue(sectionName, "thrust", 0.0);
        stage.at(i).emptymass = ini.GetDoubleValue(sectionName, "emptymass", 0.0);
        stage.at(i).fuelmass = ini.GetDoubleValue(sectionName, "fuelmass", 0.0);
        stage.at(i).burntime = ini.GetDoubleValue(sectionName, "burntime", 0.0);
        stage.at(i).ignite_delay = ini.GetDoubleValue(sectionName, "ignite_delay", 0.0);
        stage.at(i).currDelay = stage.at(i).ignite_delay; // Initialize runtime counter

        // 4. Read Aerodynamics / Control
        stage.at(i).pitchthrust = 2.0 * ini.GetDoubleValue(sectionName, "pitchthrust", 0.0);
        if (stage.at(i).pitchthrust == 0) stage.at(i).defpitch = TRUE;

        stage.at(i).yawthrust = 2.0 * ini.GetDoubleValue(sectionName, "yawthrust", 0.0);
        if (stage.at(i).yawthrust == 0) stage.at(i).defyaw = TRUE;

        stage.at(i).rollthrust = 2.0 * ini.GetDoubleValue(sectionName, "rollthrust", 0.0);
        if (stage.at(i).rollthrust == 0) stage.at(i).defroll = TRUE;

        // 5. Read Engines
        int neng = 0;
        char engKey[32];
        for (int e = 0; e < 32; e++) {
            snprintf(engKey, sizeof(engKey), "ENG_%i", e + 1);
            const char* engVal = ini.GetValue(sectionName, engKey, "");
            if (strlen(engVal) == 0) break; // Stop if no more engines defined

            // Parse as Vec4 (x,y,z, throttle_delay) or Vec3
            VECTOR4 ev4 = CharToVec4(engVal); // You'll need CharToVec4 helper
            stage.at(i).engV4.at(e) = ev4; 
            stage.at(i).eng.at(e) = _V(ev4.x, ev4.y, ev4.z);

            // Safety check for throttle delay
            if ((stage.at(i).engV4.at(e).t <= 0) || (stage.at(i).engV4.at(e).t > 10)) {
                stage.at(i).engV4.at(e).t = 1.0;
            }
            neng++;
        }
        stage.at(i).nEngines = neng;

        // 6. Engine Visuals
        stage.at(i).eng_diameter = ini.GetDoubleValue(sectionName, "eng_diameter", 0.0);
        if (stage.at(i).eng_diameter == 0) {
            stage.at(i).eng_diameter = 0.5 * stage.at(i).diameter;
        }

        stage.at(i).eng_dir = CharToVec(ini.GetValue(sectionName, "eng_dir", "0,0,1"));
        stage.at(i).eng_tex = std::string(ini.GetValue(sectionName, "eng_tex", ""));

        // Particle Streams
        stage.at(i).eng_pstream1 = std::string(ini.GetValue(sectionName, "eng_pstream1", ""));
        stage.at(i).wps1 = (stage.at(i).eng_pstream1.length() > 0);

        stage.at(i).eng_pstream2 = std::string(ini.GetValue(sectionName, "eng_pstream2", ""));
        stage.at(i).wps2 = (stage.at(i).eng_pstream2.length() > 0);

        stage.at(i).ParticlesPackedToEngine = ini.GetLongValue(sectionName, "particles_packed_to_engine", 0);

        // 7. Advanced Features (Boiloff, Battery, Ullage)
        stage.at(i).reignitable = ini.GetBoolValue(sectionName, "reignitable", false);
        stage.at(i).wBoiloff = ini.GetBoolValue(sectionName, "boiloff", false);

        double batt_hours = ini.GetDoubleValue(sectionName, "battery", 0.0);
        if (batt_hours > 0) {
            stage.at(i).batteries.wBatts = TRUE;
            stage.at(i).batteries.MaxCharge = batt_hours * 3600.0;
            stage.at(i).batteries.CurrentCharge = stage.at(i).batteries.MaxCharge;
        } else {
            stage.at(i).batteries.wBatts = FALSE;
            stage.at(i).batteries.MaxCharge = 12 * 3600; // Default
        }

        // Ullage Motors
        stage.at(i).ullage.thrust = ini.GetDoubleValue(sectionName, "ullage_thrust", 0.0);
        if (stage.at(i).ullage.thrust > 0) {
            stage.at(i).ullage.wUllage = TRUE;
            stage.at(i).ullage.anticipation = ini.GetDoubleValue(sectionName, "ullage_anticipation", 0.0);
            stage.at(i).ullage.overlap = ini.GetDoubleValue(sectionName, "ullage_overlap", 0.0);
            stage.at(i).ullage.N = ini.GetLongValue(sectionName, "ullage_N", 0);
            stage.at(i).ullage.angle = ini.GetDoubleValue(sectionName, "ullage_angle", 0.0);
            stage.at(i).ullage.diameter = ini.GetDoubleValue(sectionName, "ullage_diameter", 0.0);
            stage.at(i).ullage.length = ini.GetDoubleValue(sectionName, "ullage_length", 0.0);
            if (stage.at(i).ullage.length == 0) stage.at(i).ullage.length = 10 * stage.at(i).ullage.diameter;

            stage.at(i).ullage.dir = CharToVec(ini.GetValue(sectionName, "ullage_dir", "0,0,1"));
            stage.at(i).ullage.pos = CharToVec(ini.GetValue(sectionName, "ullage_pos", "0,0,0"));
            stage.at(i).ullage.tex = std::string(ini.GetValue(sectionName, "ullage_tex", ""));
            stage.at(i).ullage.rectfactor = ini.GetDoubleValue(sectionName, "ullage_rectfactor", 1.0);
        }

        // Explosive Bolts (Separation)
        stage.at(i).expbolt.pos = CharToVec(ini.GetValue(sectionName, "expbolts_pos", "0,0,0"));
        if (length(stage.at(i).expbolt.pos) > 0) { // If pos is defined
            stage.at(i).expbolt.wExpbolt = TRUE;
            stage.at(i).expbolt.pstream = std::string(ini.GetValue(sectionName, "expbolts_pstream", ""));
            stage.at(i).expbolt.dir = _V(0,0,1);
            stage.at(i).expbolt.anticipation = ini.GetDoubleValue(sectionName, "expbolts_anticipation", 1.0);
        }
    }
}

// ==============================================================
// Booster Parsing
// ==============================================================

void Multistage2026::parseBoosters(char* filename) {
    CSimpleIniA ini;
    ini.SetUnicode();

    if (ini.LoadFile(filename) < 0) {
        oapiWriteLogV("%s: Error loading INI file in parseBoosters: %s", GetName(), filename);
        return;
    }

    nBoosters = 0;
    char sectionName[64];

    // Iterate through booster groups (Limit 10)
    for (int b = 0; b < 10; b++) {
        snprintf(sectionName, sizeof(sectionName), "BOOSTER_%i", b + 1);

        // Check if section exists
        if (!ini.GetSection(sectionName)) {
            nBoosters = b;
            oapiWriteLogV("%s: Config loaded. Total Booster Groups: %i", GetName(), nBoosters);
            break;
        }

        // 1. Basic Parameters
        booster.at(b).N = ini.GetLongValue(sectionName, "N", 0);
        booster.at(b).meshname = std::string(ini.GetValue(sectionName, "meshname", ""));
        booster.at(b).module = std::string(ini.GetValue(sectionName, "module", "Stage"));
        booster.at(b).off = CharToVec(ini.GetValue(sectionName, "off", "0,0,0.001"));
        booster.at(b).speed = CharToVec(ini.GetValue(sectionName, "speed", "3,0,0"));
        booster.at(b).rot_speed = CharToVec(ini.GetValue(sectionName, "rot_speed", "0,-0.1,0"));
        booster.at(b).height = ini.GetDoubleValue(sectionName, "height", 0.0);
        booster.at(b).diameter = ini.GetDoubleValue(sectionName, "diameter", 0.0);
        booster.at(b).angle = ini.GetDoubleValue(sectionName, "angle", 0.0);
        booster.at(b).thrust = ini.GetDoubleValue(sectionName, "thrust", 0.0);
        booster.at(b).emptymass = ini.GetDoubleValue(sectionName, "emptymass", 0.0);
        booster.at(b).fuelmass = ini.GetDoubleValue(sectionName, "fuelmass", 0.0);
        booster.at(b).burntime = ini.GetDoubleValue(sectionName, "burntime", 0.0);
        booster.at(b).burndelay = ini.GetDoubleValue(sectionName, "burndelay", 0.0);
        booster.at(b).currDelay = booster.at(b).burndelay; // Initialize runtime delay

        // 2. Engines & Visuals
        booster.at(b).eng_diameter = ini.GetDoubleValue(sectionName, "eng_diameter", 0.5);
        booster.at(b).eng_tex = std::string(ini.GetValue(sectionName, "eng_tex", ""));
        booster.at(b).eng_dir = CharToVec(ini.GetValue(sectionName, "eng_dir", "0,0,1"));

        // Booster Engine Offset Loop
        for (int nbeng = 0; nbeng < 5; nbeng++) {
            char engKey[32];
            snprintf(engKey, sizeof(engKey), "ENG_%i", nbeng + 1);
            const char* val = ini.GetValue(sectionName, engKey, "");
            if (strlen(val) == 0) {
                booster.at(b).nEngines = nbeng;
                break;
            }
            booster.at(b).eng[nbeng] = CharToVec(val);
            booster.at(b).nEngines = nbeng + 1;
        }

        // Particle Streams
        booster.at(b).eng_pstream1 = std::string(ini.GetValue(sectionName, "eng_pstream1", ""));
        booster.at(b).wps1 = (booster.at(b).eng_pstream1.length() > 0);

        booster.at(b).eng_pstream2 = std::string(ini.GetValue(sectionName, "eng_pstream2", ""));
        booster.at(b).wps2 = (booster.at(b).eng_pstream2.length() > 0);

        // 3. Thrust Curves (CURVE_1 to CURVE_10)
        // Format: (Time, ThrustLevel, Unused) e.g. (0, 100, 0)
        for (int cc = 0; cc < 10; cc++) {
            char curveKey[32];
            snprintf(curveKey, sizeof(curveKey), "CURVE_%i", cc + 1);
            // Default: "High endurance" if undefined (9000000 sec, 100% thrust)
            const char* curveVal = ini.GetValue(sectionName, curveKey, "9000000,100,0");
            booster.at(b).curve.at(cc) = CharToVec(curveVal);
            booster.at(b).curve.at(cc).z = 0; // Ensure Z is clean
        }

        // 4. Explosive Bolts
        booster.at(b).expbolt.pos = CharToVec(ini.GetValue(sectionName, "expbolts_pos", "0,0,0"));
        if (length(booster.at(b).expbolt.pos) > 0) {
            booster.at(b).expbolt.wExpbolt = TRUE;
            booster.at(b).expbolt.pstream = std::string(ini.GetValue(sectionName, "expbolts_pstream", ""));
            booster.at(b).expbolt.dir = _V(0,0,1);
            booster.at(b).expbolt.anticipation = ini.GetDoubleValue(sectionName, "expbolts_anticipation", 1.0);
        } else {
            booster.at(b).expbolt.wExpbolt = FALSE;
        }
    }
}

// ==============================================================
// Payload Helpers
// ==============================================================

// Helper: Split "Mesh1;Mesh2;Mesh3" string into vector names
void Multistage2026::ArrangePayloadMeshes(std::string data, int pnl) {
    std::string segment;
    std::stringstream ss(data);
    int count = 0;

    // Split string by semicolon
    while(std::getline(ss, segment, ';') && count < 5) {
        // Remove whitespace from start/end
        segment.erase(0, segment.find_first_not_of(" \t\n\r\f\v"));
        segment.erase(segment.find_last_not_of(" \t\n\r\f\v") + 1);

        if (count == 0) payload.at(pnl).meshname0 = segment;
        else if (count == 1) payload.at(pnl).meshname1 = segment;
        else if (count == 2) payload.at(pnl).meshname2 = segment;
        else if (count == 3) payload.at(pnl).meshname3 = segment;
        else if (count == 4) payload.at(pnl).meshname4 = segment;
        count++;
    }
    payload.at(pnl).nMeshes = count;
}

// Helper: Split "0,0,1; 0,1,0" offset string into vector positions
void Multistage2026::ArrangePayloadOffsets(std::string data, int pnl) {
    std::string segment;
    std::stringstream ss(data);
    int count = 0;

    while(std::getline(ss, segment, ';') && count < payload.at(pnl).nMeshes) {
        // We need CharToVec defined for this to work!
        payload.at(pnl).off.at(count) = CharToVec(segment);
        count++;
    }
}

void Multistage2026::parsePayload(char* filename) {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(filename) < 0) {
        oapiWriteLogV("%s: Error loading INI file in parsePayload: %s", GetName(), filename);
        return;
    }

    char sectionName[64];

    // Iterate through Payloads (Limit 10)
    for (int pnl = 0; pnl < 10; pnl++) {
        snprintf(sectionName, sizeof(sectionName), "PAYLOAD_%i", pnl + 1);

        if (!ini.GetSection(sectionName)) {
            nPayloads = pnl;
            oapiWriteLogV("%s: Config loaded. Total Payloads: %i", GetName(), nPayloads);
            break;
        }

        // 1. Mesh Parsing (Handles multiple meshes separated by ;)
        std::string meshlist = std::string(ini.GetValue(sectionName, "meshname", ""));
        if (meshlist.length() > 0) {
            ArrangePayloadMeshes(meshlist, pnl);
            // 2. Offsets (Matches the mesh count)
            std::string offlist = std::string(ini.GetValue(sectionName, "off", "0,0,0"));
            ArrangePayloadOffsets(offlist, pnl);
            payload.at(pnl).meshname = meshlist; // Store full string for reference
        } else {
            payload.at(pnl).nMeshes = 0;
        }

        // 3. Physical Parameters
        payload.at(pnl).name = std::string(ini.GetValue(sectionName, "name", "Payload"));
        payload.at(pnl).module = std::string(ini.GetValue(sectionName, "module", "Stage"));
        payload.at(pnl).mass = ini.GetDoubleValue(sectionName, "mass", 0.0);
        payload.at(pnl).diameter = ini.GetDoubleValue(sectionName, "diameter", 0.0);
        payload.at(pnl).height = ini.GetDoubleValue(sectionName, "height", 0.0);
        // 4. Motion Parameters
        payload.at(pnl).speed = CharToVec(ini.GetValue(sectionName, "speed", "0,0,0"));
        payload.at(pnl).rot_speed = CharToVec(ini.GetValue(sectionName, "rot_speed", "0,0,0"));
        // Rotation (Orientation on stack)
        VECTOR3 rotDeg = CharToVec(ini.GetValue(sectionName, "rotation", "0,0,0"));
        payload.at(pnl).Rotation = operator*(rotDeg, RAD); // Convert to Radians
        payload.at(pnl).rotated = (length(payload.at(pnl).Rotation) > 0);

        // 5. Render & Logic Flags
        payload.at(pnl).render = ini.GetLongValue(sectionName, "render", 0);
        if (payload.at(pnl).render != 1) payload.at(pnl).render = 0;

        int liveVal = ini.GetLongValue(sectionName, "live", 0);
        payload.at(pnl).live = (liveVal == 1);
    }
}

// ==============================================================
// Fairing Parsing
// ==============================================================

void Multistage2026::parseFairing(char* filename) {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(filename) < 0) return;

    const char* sectionName = "FAIRING";

    if (!ini.GetSection(sectionName)) {
        hasFairing = false;
        return;
    }

    fairing.N = ini.GetLongValue(sectionName, "N", 0);

    if (fairing.N > 0) {
        hasFairing = true;
        fairing.meshname = std::string(ini.GetValue(sectionName, "meshname", ""));
        fairing.module = std::string(ini.GetValue(sectionName, "module", "Stage"));
        fairing.off = CharToVec(ini.GetValue(sectionName, "off", "0,0,0"));
        fairing.speed = CharToVec(ini.GetValue(sectionName, "speed", "0,-3,0"));
        fairing.rot_speed = CharToVec(ini.GetValue(sectionName, "rot_speed", "0,0,0"));
        fairing.height = ini.GetDoubleValue(sectionName, "height", 0.0);
        fairing.diameter = ini.GetDoubleValue(sectionName, "diameter", 0.0);
        fairing.angle = ini.GetDoubleValue(sectionName, "angle", 0.0);
        fairing.emptymass = ini.GetDoubleValue(sectionName, "emptymass", 0.0);
        oapiWriteLogV("%s: Fairing Configured. N=%i Mesh=%s", GetName(), fairing.N, fairing.meshname.c_str());
    } else {
        hasFairing = false;
    }
}

// ==============================================================
// Particle & FX Parsing
// ==============================================================

void Multistage2026::parseParticle(char* filename) {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(filename) < 0) return;

    char sectionName[64];

    // Iterate through particle definitions (Limit 13)
    for (int i = 0; i < 13; i++) {
        snprintf(sectionName, sizeof(sectionName), "PARTICLESTREAM_%i", i + 1);

        if (!ini.GetSection(sectionName)) {
            nParticles = i;
            break;
        }

        const char* name = ini.GetValue(sectionName, "name", "Particle");
        strncpy(Particle[i].ParticleName, name, MAXLEN - 1); // Safe copy

        Particle[i].Pss.srcsize = ini.GetDoubleValue(sectionName, "srcsize", 1.0);
        Particle[i].Pss.srcrate = ini.GetDoubleValue(sectionName, "srcrate", 10.0);
        Particle[i].Pss.v0 = ini.GetDoubleValue(sectionName, "v0", 10.0);
        Particle[i].Pss.srcspread = ini.GetDoubleValue(sectionName, "srcspread", 0.1);
        Particle[i].Pss.lifetime = ini.GetDoubleValue(sectionName, "lifetime", 1.0);
        Particle[i].Pss.growthrate = ini.GetDoubleValue(sectionName, "growthrate", 0.0);
        Particle[i].Pss.atmslowdown = ini.GetDoubleValue(sectionName, "atmslowdown", 0.0);

        // Light Type
        std::string ltype = ini.GetValue(sectionName, "ltype", "EMISSIVE");
        if (ltype == "DIFFUSE") Particle[i].Pss.ltype = PARTICLESTREAMSPEC::DIFFUSE;
        else Particle[i].Pss.ltype = PARTICLESTREAMSPEC::EMISSIVE;

        // Level Map
        std::string lvlmap = ini.GetValue(sectionName, "levelmap", "LVL_LIN");
        if (lvlmap == "LVL_FLAT") Particle[i].Pss.levelmap = PARTICLESTREAMSPEC::LVL_FLAT;
        else if (lvlmap == "LVL_SQRT") Particle[i].Pss.levelmap = PARTICLESTREAMSPEC::LVL_SQRT;
        else if (lvlmap == "LVL_PLIN") Particle[i].Pss.levelmap = PARTICLESTREAMSPEC::LVL_PLIN;
        else if (lvlmap == "LVL_PSQRT") Particle[i].Pss.levelmap = PARTICLESTREAMSPEC::LVL_PSQRT;
        else Particle[i].Pss.levelmap = PARTICLESTREAMSPEC::LVL_LIN;

        Particle[i].Pss.lmin = ini.GetDoubleValue(sectionName, "lmin", 0.0);
        Particle[i].Pss.lmax = ini.GetDoubleValue(sectionName, "lmax", 1.0);

        // Atmosphere Map
        std::string atmmap = ini.GetValue(sectionName, "atmsmap", "ATM_PLIN");
        if (atmmap == "ATM_FLAT") Particle[i].Pss.atmsmap = PARTICLESTREAMSPEC::ATM_FLAT;
        else if (atmmap == "ATM_PLOG") Particle[i].Pss.atmsmap = PARTICLESTREAMSPEC::ATM_PLOG;
        else Particle[i].Pss.atmsmap = PARTICLESTREAMSPEC::ATM_PLIN;

        Particle[i].Pss.amin = ini.GetDoubleValue(sectionName, "amin", 0.0);
        Particle[i].Pss.amax = ini.GetDoubleValue(sectionName, "amax", 1.0);

        // Texture
        const char* tex = ini.GetValue(sectionName, "tex", "Contrail3");
        strncpy(Particle[i].TexName, tex, MAXLEN - 1);
        Particle[i].Pss.tex = oapiRegisterParticleTexture((char*)tex);

        // Growth Factor
        Particle[i].GrowFactor_size = ini.GetDoubleValue(sectionName, "GrowFactor_size", 0.0);
        Particle[i].GrowFactor_rate = ini.GetDoubleValue(sectionName, "GrowFactor_rate", 0.0);
        Particle[i].Growing = (Particle[i].GrowFactor_rate != 0 || Particle[i].GrowFactor_size != 0);
    }
}

void Multistage2026::parseTexture(char* filename) {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(filename) < 0) return;

    const char* sectionName = "TEXTURE_LIST";
    if (!ini.GetSection(sectionName)) return;

    char keyName[32];
    for (int i = 0; i < 16; i++) {
        snprintf(keyName, sizeof(keyName), "TEX_%i", i + 1);
        const char* texVal = ini.GetValue(sectionName, keyName, "");
        if (strlen(texVal) == 0) {
            nTextures = i;
            break;
        }
        strncpy(tex.TextureName[i], texVal, MAXLEN - 1);
        tex.hTex[i] = oapiRegisterExhaustTexture((char*)texVal);
    }
}

// ==============================================================
// Misc & FX Parsing
// ==============================================================

void Multistage2026::parseMisc(char* filename) {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(filename) < 0) return;

    const char* sectionName = "MISC";
    if (!ini.GetSection(sectionName)) return;

    Misc.COG = ini.GetDoubleValue(sectionName, "COG", 10.0);
    Misc.GNC_Debug = ini.GetLongValue(sectionName, "GNC_DEBUG", 0);
    Misc.telemetry = ini.GetBoolValue(sectionName, "TELEMETRY", false);
    Misc.Focus = ini.GetLongValue(sectionName, "FOCUS", 0);
    Misc.thrustrealpos = ini.GetBoolValue(sectionName, "THRUST_REAL_POS", false);
    Misc.VerticalAngle = ini.GetDoubleValue(sectionName, "VERTICAL_ANGLE", 0.0) * RAD;
    //added by rcraig42 to retrieve drag_factor from ini 
    Misc.drag_factor = ini.GetDoubleValue(sectionName, "drag_factor", 1.0);
    strncpy(Misc.PadModule, ini.GetValue(sectionName, "PAD_MODULE", "EmptyModule"), MAXLEN - 1);
}

void Multistage2026::parseFXMach(char* filename) {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(filename) < 0) return;

    const char* sectionName = "FX_MACH";
    if (ini.GetSection(sectionName)) {
        FX_Mach.pstream = std::string(ini.GetValue(sectionName, "pstream", ""));
        wMach = (FX_Mach.pstream.length() > 0);
        FX_Mach.mach_min = ini.GetDoubleValue(sectionName, "mach_min", 0.0);
        FX_Mach.mach_max = ini.GetDoubleValue(sectionName, "mach_max", 0.0);
        FX_Mach.dir = CharToVec(ini.GetValue(sectionName, "dir", "0,0,0"));

        for (int i = 0; i < 10; i++) {
            char key[32];
            snprintf(key, sizeof(key), "off_%i", i + 1);
            std::string val = ini.GetValue(sectionName, key, "");
            if (val.length() == 0) {
                FX_Mach.nmach = i;
                break;
            }
            FX_Mach.off[i] = CharToVec(val);
        }
    }
}

void Multistage2026::parseFXVent(char* filename) {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(filename) < 0) return;

    const char* sectionName = "FX_VENT";
    if (ini.GetSection(sectionName)) {
        FX_Vent.pstream = std::string(ini.GetValue(sectionName, "pstream", ""));
        wVent = (FX_Vent.pstream.length() > 0);

        for (int i = 1; i <= 10; i++) {
            char keyOff[32], keyDir[32], keyTime[32];
            snprintf(keyOff, sizeof(keyOff), "off_%i", i);
            snprintf(keyDir, sizeof(keyDir), "dir_%i", i);
            snprintf(keyTime, sizeof(keyTime), "time_fin_%i", i);

            std::string valOff = ini.GetValue(sectionName, keyOff, "");
            if (valOff.length() == 0) {
                FX_Vent.nVent = i - 1;
                break;
            }

            FX_Vent.off[i] = CharToVec(valOff);
            FX_Vent.dir[i] = CharToVec(ini.GetValue(sectionName, keyDir, "0,0,1"));
            FX_Vent.time_fin[i] = ini.GetDoubleValue(sectionName, keyTime, 0.0);
            FX_Vent.added[i] = false;
        }
    }
}

void Multistage2026::parseFXLaunch(char* filename) {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(filename) < 0) return;

    const char* sectionName = "FX_LAUNCH";
    if (ini.GetSection(sectionName)) {
        FX_Launch.N = ini.GetLongValue(sectionName, "N", 0);
        if (FX_Launch.N >= 1) wLaunchFX = TRUE;

        FX_Launch.H = ini.GetDoubleValue(sectionName, "Height", 0.0);
        FX_Launch.Angle = ini.GetDoubleValue(sectionName, "Angle", 0.0);
        FX_Launch.Distance = ini.GetDoubleValue(sectionName, "Distance", 100.0);
        FX_Launch.CutoffAltitude = ini.GetDoubleValue(sectionName, "CutoffAltitude", 200.0);
    }
}

// ==============================================================
// Sound Parsing (Complete)
// ==============================================================

void Multistage2026::parseSound(char* filename) {
    CSimpleIniA ini;
    ini.SetUnicode();
    // Attempt to load the file
    if (ini.LoadFile(filename) < 0) return;

    const char* sectionName = "SOUND";
    // If [SOUND] section exists, read all keys
    if (ini.GetSection(sectionName)) {
        // Helper Macro to reduce repetition
        #define READ_SOUND(var, key) \
            val = ini.GetValue(sectionName, key, ""); \
            if (strlen(val) > 1) { \
                Ssound.var = true; \
                strncpy(Ssound.var##_wav, val, 255); \
            } else { \
                Ssound.var = false; \
            }

        const char* val;

        // 1. Engines
        READ_SOUND(Main, "MAIN_THRUST");
        READ_SOUND(Hover, "HOVER_THRUST");
        // 2. RCS (Attitude Control)
        READ_SOUND(RCS_ta, "RCS_THRUST_ATTACK");
        READ_SOUND(RCS_ts, "RCS_THRUST_SUSTAIN");
        READ_SOUND(RCS_tr, "RCS_THRUST_RELEASE");

        // 3. Separation Events
        READ_SOUND(Stage, "STAGE_SEPARATION");
        READ_SOUND(Fairing, "FAIRING_SEPARATION");
        READ_SOUND(Booster, "BOOSTER_SEPARATION"); // Sometimes called Jettison

        // 4. Mechanical
        READ_SOUND(Gear, "GEAR");
        READ_SOUND(Umbilical, "UMBILICAL");

        // 5. Cockpit / Guidance
        READ_SOUND(Lock, "LOCK"); // Target lock or mode switch
        READ_SOUND(Click, "CLICK"); // Switch clicks
        READ_SOUND(Warning, "WARNING"); // Master Alarm

        #undef READ_SOUND
        oapiWriteLogV("%s: Sound Configuration Loaded.", GetName());
    }
}

// ==============================================================
// Telemetry & Guidance Parsing (Text/CSV Files)
// ==============================================================

void Multistage2026::parseTelemetryFile(char* filename) {
    // Telemetry files are CSVs: Time,Alt,Spd,Pitch,Thrust,Mass,Vv,Acc
    // We use standard fopen because SimpleIni expects Key=Value pairs.

    FILE* f = fopen(filename, "rt");
    if (!f) {
        // Try looking in Config/ if direct path fails
        char altpath[256];
        snprintf(altpath, sizeof(altpath), "Config/%s", filename);
        f = fopen(altpath, "rt");
        if (!f) {
            oapiWriteLogV("%s: Telemetry file not found: %s", GetName(), filename);
            return;
        }
    }

    char buffer[1024];
    // Skip Header Lines (Usually 2 lines in generated files)
    fgets(buffer, 1024, f); // "<--!Multistage 2026...>"
    fgets(buffer, 1024, f); // "MET,Altitude,Speed..."

    int i = 0;
    // Limit to 100,000 points or whatever your MAX_TLM is defined as
    while (fgets(buffer, 1024, f) && i < 100000) {
        double t, alt, spd, pch, th, m, vv, acc;
        // Parse CSV Line
        if (sscanf(buffer, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", 
            &t, &alt, &spd, &pch, &th, &m, &vv, &acc) == 8) {
            // Store in Reference Arrays (X=Time, Y=Value)
            ReftlmAlt[i] = _V(t, alt, 0);
            ReftlmSpeed[i] = _V(t, spd, 0);
            ReftlmPitch[i] = _V(t, pch, 0);
            ReftlmThrust[i] = _V(t, th, 0);
            ReftlmMass[i] = _V(t, m, 0);
            ReftlmVv[i] = _V(t, vv, 0);
            ReftlmAcc[i] = _V(t, acc, 0);
            i++;
        }
    }
    nReftlm = i;
    fclose(f);
    oapiWriteLogV("%s: Telemetry Loaded (%d points)", GetName(), nReftlm);
}

void Multistage2026::parseGuidanceFile(char* filename) {
    // Guidance files are simple text: "Time Pitch [Yaw]"
    // Example: 
    // 0 90
    // 10 85
    FILE* f = fopen(filename, "rt");
    if (!f) {
        char altpath[256];
        snprintf(altpath, sizeof(altpath), "Config/%s", filename);
        f = fopen(altpath, "rt");
        if (!f) {
            oapiWriteLogV("%s: Guidance file not found: %s", GetName(), filename);
            return;
        }
    }

    char buffer[1024];
    int i = 0;

    // Read until end of file
    while (fgets(buffer, 1024, f) && i < 2000) { // Limit to reasonable steps
        // Skip comments (lines starting with / or #)
        if (buffer[0] == '/' || buffer[0] == '#' || strlen(buffer) < 2) continue;

        double t, p, y = 0.0;
        int items = sscanf(buffer, "%lf %lf %lf", &t, &p, &y);
        if (items >= 2) {
            GuidanceTable[i].t = t;
            GuidanceTable[i].pitch = p * RAD; // Convert to Radians
            GuidanceTable[i].yaw = (items == 3) ? y * RAD : 0.0;
            i++;
        }
    }

    nGuidanceSteps = i;
    fclose(f);
    oapiWriteLogV("%s: Guidance Loaded (%d steps)", GetName(), nGuidanceSteps);
}


// ==============================================================
// Master Parse Function
// ==============================================================

bool Multistage2026::parseinifile(char* filename) {
    parseStages(filename);
    parseBoosters(filename);
    parsePayload(filename);
    parseLes(filename);
    parseFairing(filename);
    parseMisc(filename);
    parseTexture(filename);
    parseParticle(filename);
    parseFXMach(filename);
    parseFXVent(filename);
    parseFXLaunch(filename);
    parseAdapter(filename);
    parseSound(filename);
    return true;
}
