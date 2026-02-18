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
#include "simpleini/SimpleIni.h" // Linux method to read ini files

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
    // Linux safety: Remove ( and ) before parsing
    std::string clean = str;
    clean.erase(std::remove(clean.begin(), clean.end(), '('), clean.end());
    clean.erase(std::remove(clean.begin(), clean.end(), ')'), clean.end());
    // Now sscanf can find the numbers regardless of the original formatting
    sscanf(clean.c_str(), "%lf,%lf,%lf", &v.x, &v.y, &v.z);
    return v;
}

VECTOR4F Multistage2026::CharToVec4(const std::string& str)
{
    VECTOR4F v;
    if (sscanf(str.c_str(), "%lf,%lf,%lf,%lf", &v.x, &v.y, &v.z, &v.t) != 4) {
        // Handle error or default
        v.x = v.y = v.z = v.t = 0.0;
    }
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
        // --- PHASE 1 DEBUG: INTERSTAGE ---
        oapiWriteLogV("PARSER: Interstage %d | Mesh: %s | Offset Z: %.3f",
            parsingstage + 1, meshname,stage.at(parsingstage).interstage.off.z
        );
        // ---------------------------------
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
            oapiWriteLogV("PARSER: %s Config loaded. Total Stages found: %i", GetName(), nStages);
            break;
        }

        // 3. Read Basic Parameters - Using explicit strncpy with null termination
        strncpy(stage[i].meshname, ini.GetValue(sectionName, "meshname", ""), 255);
        stage[i].meshname[255] = '\0';

        strncpy(stage[i].module, ini.GetValue(sectionName, "module", "Stage"), 255);
        stage[i].module[255] = '\0';

        stage[i].off = CharToVec(ini.GetValue(sectionName, "off", "0,0,0"));
        stage[i].speed = CharToVec(ini.GetValue(sectionName, "speed", "0,0,0"));
        stage[i].rot_speed = CharToVec(ini.GetValue(sectionName, "rot_speed", "0,0,0"));
        stage[i].height = ini.GetDoubleValue(sectionName, "height", 0.0);
        stage[i].diameter = ini.GetDoubleValue(sectionName, "diameter", 0.0);
        stage[i].thrust = ini.GetDoubleValue(sectionName, "thrust", 0.0);
        stage[i].emptymass = ini.GetDoubleValue(sectionName, "emptymass", 0.0);
        stage[i].fuelmass = ini.GetDoubleValue(sectionName, "fuelmass", 0.0);
        stage[i].burntime = ini.GetDoubleValue(sectionName, "burntime", 0.0);
        stage[i].ignite_delay = ini.GetDoubleValue(sectionName, "ignite_delay", 0.0);
        stage[i].currDelay = stage[i].ignite_delay; 

        // 4. Read Aerodynamics / Control
        stage[i].pitchthrust = 2.0 * ini.GetDoubleValue(sectionName, "pitchthrust", 0.0);
        stage[i].defpitch = (stage[i].pitchthrust == 0);

        stage[i].yawthrust = 2.0 * ini.GetDoubleValue(sectionName, "yawthrust", 0.0);
        stage[i].defyaw = (stage[i].yawthrust == 0);

        stage[i].rollthrust = 2.0 * ini.GetDoubleValue(sectionName, "rollthrust", 0.0);
        stage[i].defroll = (stage[i].rollthrust == 0);

        // 5. Read Engines - Hardened for Linux Case Sensitivity
        int neng = 0;
        char engKey[32];
        for (int e = 0; e < 32; e++) {
            // Check for both cases explicitly to prevent "0 Engines Found" errors
            snprintf(engKey, sizeof(engKey), "eng_%i", e + 1);
            const char* engVal = ini.GetValue(sectionName, engKey, "");

            if (strlen(engVal) == 0) {
                snprintf(engKey, sizeof(engKey), "ENG_%i", e + 1);
                engVal = ini.GetValue(sectionName, engKey, "");
            }

            if (strlen(engVal) == 0) break;

            // Use the verified CharToVec to clean up formatting
            stage[i].eng[e] = CharToVec(engVal);

            // --- PHASE 1 DEBUG: STAGE ---
            oapiWriteLogV("PARSER: Stage %d Configured | eng_off %d X,Y,Z: (%.3f, %.3f, %.3f)", 
            i + 1,e + 1, stage[i].eng[e].x,stage[i].eng[e].y, stage[i].eng[e].z);
            // ----------------------------
            neng++;
        }
        stage[i].nEngines = neng;

        // 6. Engine Visuals
        stage[i].eng_diameter = ini.GetDoubleValue(sectionName, "eng_diameter", 0.0);
        if (stage[i].eng_diameter == 0) {
            stage[i].eng_diameter = 0.5 * stage[i].diameter;
        }

        stage[i].eng_dir = CharToVec(ini.GetValue(sectionName, "eng_dir", "0,0,1"));
        strncpy(stage[i].eng_tex, ini.GetValue(sectionName, "eng_tex", ""), 255);
        stage[i].eng_tex[255] = '\0';

        // Particle Streams
        strncpy(stage[i].eng_pstream1, ini.GetValue(sectionName, "eng_pstream1", ""), 255);
        stage[i].eng_pstream1[255] = '\0';
        stage[i].wps1 = (strlen(stage[i].eng_pstream1) > 0);

        strncpy(stage[i].eng_pstream2, ini.GetValue(sectionName, "eng_pstream2", ""), 255);
        stage[i].eng_pstream2[255] = '\0';
        stage[i].wps2 = (strlen(stage[i].eng_pstream2) > 0);

        // 7. Advanced Features
        stage[i].reignitable = ini.GetBoolValue(sectionName, "reignitable", false);
        stage[i].wBoiloff = ini.GetBoolValue(sectionName, "boiloff", false);

        double batt_hours = ini.GetDoubleValue(sectionName, "battery", 0.0);
        if (batt_hours > 0) {
            stage[i].batteries.wBatts = TRUE;
            stage[i].batteries.MaxCharge = batt_hours * 3600.0;
            stage[i].batteries.CurrentCharge = stage[i].batteries.MaxCharge;
        } else {
            stage[i].batteries.wBatts = FALSE;
        }

        // Ullage Motors
        stage[i].ullage.thrust = ini.GetDoubleValue(sectionName, "ullage_thrust", 0.0);
        if (stage[i].ullage.thrust > 0) {
            stage[i].ullage.wUllage = TRUE;
            stage[i].ullage.anticipation = ini.GetDoubleValue(sectionName, "ullage_anticipation", 0.0);
            stage[i].ullage.overlap = ini.GetDoubleValue(sectionName, "ullage_overlap", 0.0);
            stage[i].ullage.N = ini.GetLongValue(sectionName, "ullage_N", 0);
            stage[i].ullage.angle = ini.GetDoubleValue(sectionName, "ullage_angle", 0.0);
            stage[i].ullage.diameter = ini.GetDoubleValue(sectionName, "ullage_diameter", 0.0);
            stage[i].ullage.length = ini.GetDoubleValue(sectionName, "ullage_length", 0.0);
            stage[i].ullage.dir = CharToVec(ini.GetValue(sectionName, "ullage_dir", "0,0,1"));
            stage[i].ullage.pos = CharToVec(ini.GetValue(sectionName, "ullage_pos", "0,0,0"));
            strncpy(stage[i].ullage.tex, (const char*)ini.GetValue(sectionName, "ullage_tex", ""), 255);
            stage[i].ullage.tex[255] = '\0';
            stage[i].ullage.rectfactor = ini.GetDoubleValue(sectionName, "ullage_rectfactor", 1.0);
        }

        // Explosive Bolts
        stage[i].expbolt.pos = CharToVec(ini.GetValue(sectionName, "expbolts_pos", "0,0,0"));
        if (length(stage[i].expbolt.pos) > 0) {
           stage[i].expbolt.wExpbolt = TRUE;
           strncpy(stage[i].expbolt.pstream, (const char*)ini.GetValue(sectionName, "expbolts_pstream", ""), 255);
           stage[i].expbolt.pstream[255] = '\0';
           stage[i].expbolt.dir = _V(0,0,1);
           stage[i].expbolt.anticipation = ini.GetDoubleValue(sectionName, "expbolts_anticipation", 1.0);
        }
        // --- PHASE 1 DEBUG: STAGE ---
        oapiWriteLogV("PARSER: Stage %d Configured | Mesh: %s | Offset Z: %.3f", 
            i + 1, stage[i].meshname, stage[i].off.z);
        // ----------------------------
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
            oapiWriteLogV("PARSER %s Config loaded. Total Booster Groups: %i", GetName(), nBoosters);
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

        // --- CRITICAL DEBUG: CAPTURE MESH OFFSET ---
        oapiWriteLogV("PARSER: Booster Group %d INI Read -> Offset (X,Y,Z): (%.3f, %.3f, %.3f)", 
            b + 1, booster.at(b).off.x, booster.at(b).off.y, booster.at(b).off.z);
        // -------------------------------------------

        // 2. Engines & Visuals
        booster.at(b).eng_diameter = ini.GetDoubleValue(sectionName, "eng_diameter", 0.5);
        booster.at(b).eng_tex = std::string(ini.GetValue(sectionName, "eng_tex", ""));
        booster.at(b).eng_dir = CharToVec(ini.GetValue(sectionName, "eng_dir", "0,0,1"));

        // Booster Engine Offset Loop - 32 Engine Limit for Falcon Heavy
        for (int nbeng = 0; nbeng < 32; nbeng++) {
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
            booster.at(b).curve[cc] = CharToVec(curveVal);
            booster.at(b).curve[cc].z = 0; // Ensure Z is clean
        }

        // 4. Explosive Bolts
        booster.at(b).expbolt.pos = CharToVec(ini.GetValue(sectionName, "expbolts_pos", "0,0,0"));
        if (length(booster.at(b).expbolt.pos) > 0) {
            booster.at(b).expbolt.wExpbolt = TRUE;
            strncpy(booster.at(b).expbolt.pstream, (const char*)ini.GetValue(sectionName, "expbolts_pstream", ""), 255);
            booster.at(b).expbolt.pstream[255] = '\0';
            booster.at(b).expbolt.dir = _V(0,0,1);
            booster.at(b).expbolt.anticipation = ini.GetDoubleValue(sectionName, "expbolts_anticipation", 1.0);
        } else {
            booster.at(b).expbolt.wExpbolt = FALSE;
        }
    }
}
// ==============================================================
// Payload Parsing
// ==============================================================
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
            oapiWriteLogV("PARSER %s Config loaded. Total Payloads: %i", GetName(), nPayloads);
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

            // SAFETY CHECK: Only print if the vector actually has data
            if (!payload.at(pnl).off.empty()) {
                oapiWriteLogV("PARSER: Payload %d Configured | Mesh: %s | X,Y,Z: (%.3f , %.3f , %.3f)",
                    pnl + 1, payload.at(pnl).meshname.c_str(),payload.at(pnl).off[0].x,
                   payload.at(pnl).off[0].y, payload.at(pnl).off[0].z);
            } else {
                oapiWriteLogV("PARSER: Payload %d Configured | WARNING: No Offset Found!", pnl + 1);
            }
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

        // --- PHASE 1 DEBUG: LES ---
        oapiWriteLogV("PARSER: LES System Configured | Mesh: %s | X,Y,Z: (%.3f, %.3f, %.3f)",
           meshname, Les.off.x, Les.off.y, Les.off.z);
        // --------------------------
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

        // --- PHASE 1 DEBUG: LES ---
        oapiWriteLogV("PARSER: Adpater System Configured | Mesh: %s | X,Y,Z: (%.3f, %.3f, %.3f)",
           meshname, Adapter.off.x, Adapter.off.y, Adapter.off.z);
        // --------------------------
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
        // Standard String/Vector Parsing
        fairing.meshname = std::string(ini.GetValue(sectionName, "meshname", ""));
        fairing.module = std::string(ini.GetValue(sectionName, "module", "Stage"));
        // --- PHASE 1 DEBUG: CAPTURE THE OFFSET ---
        fairing.off = CharToVec(ini.GetValue(sectionName, "off", "0,0,0"));
        fairing.speed = CharToVec(ini.GetValue(sectionName, "speed", "0,-3,0"));
        fairing.rot_speed = CharToVec(ini.GetValue(sectionName, "rot_speed", "0,0,0"));
        fairing.height = ini.GetDoubleValue(sectionName, "height", 0.0);
        fairing.diameter = ini.GetDoubleValue(sectionName, "diameter", 0.0);
        fairing.angle = ini.GetDoubleValue(sectionName, "angle", 0.0);
        fairing.emptymass = ini.GetDoubleValue(sectionName, "emptymass", 0.0);

        // --- IMPROVED LOGGING ---
        oapiWriteLogV("PARSER: Fairing Configured | N=%i Mesh=%s | X,Y,Z: (%.3f, %.3f, %.3f)",
            fairing.N, fairing.meshname.c_str(),fairing.off.x, fairing.off.y,fairing.off.z);
    } else {
        hasFairing = false;
    }
}

// ==============================================================
// Misc Parse
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

    oapiWriteLogV("PARSER DEBUG: Misc.thrustrealpos value is: %s", Misc.thrustrealpos ? "TRUE" : "FALSE");

    Misc.VerticalAngle = ini.GetDoubleValue(sectionName, "VERTICAL_ANGLE", 0.0) * RAD;
    //added by rcraig42 to retrieve drag_factor from ini
    Misc.drag_factor = ini.GetDoubleValue(sectionName, "drag_factor", 1.0);
    strncpy(Misc.PadModule, ini.GetValue(sectionName, "PAD_MODULE", "EmptyModule"), MAXLEN - 1);

    // Search for Touchdown Points Section
    if (ini.GetSection("TOUCHDOWN_POINTS")) {
        char key[32];
        for (int i = 0; i < 3; i++) {
            sprintf(key, "point_%d", i + 1);
            const char* val = ini.GetValue("TOUCHDOWN_POINTS", key, "");
            if (val && strlen(val) > 0) {
                // Route to the Misc structure where we already store COG and drag
                Misc.td_points[i] = CharToVec(val);
                Misc.has_custom_td = true;

                // --- DEBUG: Log the exact coordinate parsed ---
                oapiWriteLogV("PARSER: TP[%d] Parsed: X=%.3f Y=%.3f Z=%.3f",
                    i + 1, Misc.td_points[i].x, Misc.td_points[i].y, Misc.td_points[i].z);
            }
        }
        oapiWriteLog("DEBUG: Custom Touchdown Points Parsed from INI");
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
        // Ensure the vector has room before we write to it!
        if (i >= Particle.size()) {
            Particle.resize(i + 1);
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
        Particle[i].Pss.tex = NULL;

        // Growth Factor
        Particle[i].GrowFactor_size = ini.GetDoubleValue(sectionName, "GrowFactor_size", 0.0);
        Particle[i].GrowFactor_rate = ini.GetDoubleValue(sectionName, "GrowFactor_rate", 0.0);
        Particle[i].Growing = (Particle[i].GrowFactor_rate != 0 || Particle[i].GrowFactor_size != 0);
    }
}

void Multistage2026::parseTexture(char* filename) {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadFile(filename) < 0) {
        oapiWriteLogV("%s: Warning: Could not load texture file %s", GetName(), filename);
        return;
    }

    const char* sectionName = "TEXTURE_LIST";
    if (!ini.GetSection(sectionName)) return;

    // Reset count before parsing
    nTextures = 0;
    char keyName[32];

    // Safely determine the limit based on your header's array size
    // Assuming 'tex' is a struct containing TextureName[16] and hTex[16]
    const int MAX_TEXTURES = 16; 

    for (int i = 0; i < MAX_TEXTURES; i++) {
        snprintf(keyName, sizeof(keyName), "TEX_%i", i + 1);
        const char* texVal = ini.GetValue(sectionName, keyName, "");
        // Break early if key is missing or empty
        if (texVal == nullptr || strlen(texVal) == 0) {
            break;
        }
        // 1. Safe String Copy: Prevent buffer overflow on the name array
        strncpy(tex.TextureName[i], texVal, MAXLEN - 1);
        tex.TextureName[i][MAXLEN - 1] = '\0'; // Explicit null termination

        // 2. Increment current valid count
        nTextures = i + 1;
    }
    oapiWriteLogV("%s: Successfully loaded %d textures from INI.", GetName(), nTextures);
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

void Multistage2026::parseTelemetryFile(char* name)
{
    FILE* fVal = fopen(name, "r");
    if (fVal == NULL) {
        // Try looking in Config/ folder if not found directly
        char path[256];
        sprintf(path, "Config/%s", name);
        fVal = fopen(path, "r");
    }

    if (fVal != NULL) {
        char line[512];
        int i = 0;
        
        // Skip header if present (optional, but good practice)
        // fgets(line, 512, fVal); 

        while (fgets(line, 512, fVal) != NULL && i < TLMSECS) {
            double t, alt, spd, pch, th, m, vv, acc;
            
            // Parse the line: Time, Alt, Speed, Pitch, Thrust, Mass, VV, Acc
            int count = sscanf(line, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", 
                               &t, &alt, &spd, &pch, &th, &m, &vv, &acc);

            if (count >= 8) {
                // Populate Reference Arrays (IVECTOR2: x=Time, y=Value)
                ReftlmAlt[i].x = t;     ReftlmAlt[i].y = alt;
                ReftlmSpeed[i].x = t;   ReftlmSpeed[i].y = spd;
                ReftlmPitch[i].x = t;   ReftlmPitch[i].y = pch;
                ReftlmThrust[i].x = t;  ReftlmThrust[i].y = th;
                ReftlmMass[i].x = t;    ReftlmMass[i].y = m;
                ReftlmVv[i].x = t;      ReftlmVv[i].y = vv;
                ReftlmAcc[i].x = t;     ReftlmAcc[i].y = acc;
                
                i++;
            }
        }
        
        fclose(fVal);
        loadedtlmlines = i;
        wReftlm = true;
        oapiWriteLogV((char*)"Telemetry Loaded: %d lines", i);
    } else {
        wReftlm = false;
        oapiWriteLogV((char*)"[ERROR] Telemetry File Not Found: %s", name);
    }
}

void Multistage2026::parseGuidanceFile(char* name)
{
    FILE* fVal = fopen(name, "r");
    if (fVal == NULL) {
        char path[256];
        sprintf(path, "Config/%s", name);
        fVal = fopen(path, "r");
    }
    if (fVal != NULL) {
        char line[512];
        int i = 1; // 1-based index for Gnc_step matches legacy logic
        while (fgets(line, 512, fVal) != NULL && i < 500) {
            // Clear temporary vars
            double t = 0;
            char cmdStr[32] = {0};
            double v1=0, v2=0, v3=0, v4=0, v5=0, v6=0;

            // Scan: Time Command Val1 Val2 ...
            int count = sscanf(line, "%lf %s %lf %lf %lf %lf %lf %lf", 
                               &t, cmdStr, &v1, &v2, &v3, &v4, &v5, &v6);
            if (count >= 2) {
                Gnc_step[i].time = t;
                strcpy(Gnc_step[i].Comand, cmdStr); // Store string version
                // Map String to Integer ID (CM_ constants from header)
                if (_stricmp(cmdStr, "pitch") == 0) Gnc_step[i].GNC_Comand = CM_PITCH;
                else if (_stricmp(cmdStr, "roll") == 0) Gnc_step[i].GNC_Comand = CM_ROLL;
                else if (_stricmp(cmdStr, "yaw") == 0) Gnc_step[i].GNC_Comand = CM_ATTITUDE; // Often mapped to attitude
                else if (_stricmp(cmdStr, "engine") == 0) Gnc_step[i].GNC_Comand = CM_ENGINE;
                else if (_stricmp(cmdStr, "fairing") == 0) Gnc_step[i].GNC_Comand = CM_FAIRING;
                else if (_stricmp(cmdStr, "jettison") == 0) Gnc_step[i].GNC_Comand = CM_JETTISON;
                else if (_stricmp(cmdStr, "les") == 0) Gnc_step[i].GNC_Comand = CM_LES;
                else if (_stricmp(cmdStr, "target") == 0) Gnc_step[i].GNC_Comand = CM_TARGET;
                else if (_stricmp(cmdStr, "orbit") == 0) Gnc_step[i].GNC_Comand = CM_ORBIT;
                else if (_stricmp(cmdStr, "disable_pitch") == 0) Gnc_step[i].GNC_Comand = CM_DISABLE_PITCH;
                else if (_stricmp(cmdStr, "disable_roll") == 0) Gnc_step[i].GNC_Comand = CM_DISABLE_ROLL;
                else Gnc_step[i].GNC_Comand = CM_NOLINE; // Unknown or Comment

                // Store parameters
                Gnc_step[i].val_init = v1; // Primary value usually here
                Gnc_step[i].trval1 = v1;
                Gnc_step[i].trval2 = v2;
                Gnc_step[i].trval3 = v3;
                Gnc_step[i].trval4 = v4;
                Gnc_step[i].trval5 = v5;
                Gnc_step[i].trval6 = v6;
                Gnc_step[i].executed = false;
                i++;
            }
        }
        fclose(fVal);
        nsteps = i - 1;
        stepsloaded = true;
        oapiWriteLogV((char*)"Guidance File Read: %d lines", i);
    } else {
        stepsloaded = false;
        oapiWriteLogV((char*)"[ERROR] Guidance File Not Found: %s", name);
    }
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
