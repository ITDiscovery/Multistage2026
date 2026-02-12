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

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm> // For std::min, std::max

#include "Orbitersdk.h"
#include "Multistage2026.h"

using namespace std;

const double mu = 398600.4418e9;

// Helper for Sign
template <typename T> int SignOf(T val) {
    return (T(0) < val) - (val < T(0));
}

// ==============================================================
// Orbital Calculation Helpers
// ==============================================================

double Multistage2026::finalv(double Abside, double Apo, double Peri) {
    return sqrt(mu * (2 / (Abside + rt) - 1 / (0.5 * (Apo + Peri + 2 * rt))));
}

double Multistage2026::CalcAzimuth() {
    double heading;
    double lon, lat, rad;
    GetEquPos(lon, lat, rad);
    const double wearth = 2 * PI / oapiGetPlanetPeriod(GetSurfaceRef());
    
    // Calculate Earth Rotation speed at current latitude
    double EastVel = fabs((oapiGetSize(GetSurfaceRef()) + GetAltitude()) * wearth * cos(lat));
    
    VECTOR3 hspd;
    GetGroundspeedVector(FRAME_HORIZON, hspd);
    EastVel += (hspd.x);
    double NorthVel = hspd.z;

    if (GetAltitude() > 5000) {
        // Check for the particular ambiguity in inclination
        if (SignOf(tgtinc) != SignOf(NorthVel)) {
            tgtinc = -tgtinc;
        }
    }

    double num = (finalv(tgtabside, tgtapo, tgtperi)) * (sin(asin(cos(tgtinc) / cos(lat)))) - EastVel;
    double den = (finalv(tgtabside, tgtapo, tgtperi)) * (cos(asin(cos(tgtinc) / cos(lat)))) - fabs(NorthVel);
    
    heading = atan2(num, den);
    heading += (90 * RAD - heading) * (1 - SignOf(tgtinc));

    return heading;
}

double Multistage2026::GetProperHeading() {
    double heading;
    
    if (!runningPeg) {
        if (GetAltitude() < 3000) { 
            // Safety fallback if roll hasn't happened yet
            heading = VinkaAzimuth;
        } else {
            oapiGetHeading(GetHandle(), &heading);
        }
    } else {
        double lon, lat, rad;
        GetEquPos(lon, lat, rad);

        if (fabs(tgtinc) < fabs(lat)) {
            if (lat < 0) tgtinc = fabs(lat);
            else tgtinc = -fabs(lat);
        }
        heading = CalcAzimuth();
    }
    return heading;
}

double Multistage2026::GetVPerp() {
    OBJHANDLE hearth = GetSurfaceRef();
    VECTOR3 relvel, loc_rvel, horvel, glob_vpos;
    GetRelativeVel(hearth, relvel);
    GetGlobalPos(glob_vpos);
    Global2Local((relvel + glob_vpos), loc_rvel);
    HorizonRot(loc_rvel, horvel);
    return horvel.y;
}

double Multistage2026::GetProperRoll(double RequestedRoll) {
    if (RequestedRoll < 0) { RequestedRoll += 2 * PI; }
    double GB = GetBank();
    if (GB < 0) { GB += 2 * PI; }
    
    double roll = GB - RequestedRoll;
    if (roll > PI) { roll -= 2 * PI; }
    return roll;
}

// ==============================================================
// Attitude Control (PID Logic)
// ==============================================================

void Multistage2026::Attitude(double pitch, double roll, double heading, double pitchrate, double rollrate, double yawrate) {
    if (AttCtrl) {
        double pdb = 0.1; // Deadband

        VECTOR3 PMI;
        GetPMI(PMI);
        double ReqTime = oapiGetSimStep() * oapiGetTimeAcceleration();
        VECTOR3 ReqAngularAcc = _V(0, 0, 0);
        VECTOR3 ReqTorque = _V(0, 0, 0);

        // Calculate target vector from Pitch/Heading
        VECTOR3 thetain = _V(cos(pitch) * sin(heading), sin(pitch), cos(pitch) * cos(heading));
        VECTOR3 tgtVector;
        HorizonInvRot(thetain, tgtVector);
        normalise(tgtVector);

        VECTOR3 ThrustVector = {0, 0, 1};
        VECTOR3 input = crossp(tgtVector, ThrustVector);
        input.z = GetProperRoll(roll);

        if (fabs(input.z) > 1) {
            input.z = SignOf(input.z);
        }

        VECTOR3 aVel;
        GetAngularVel(aVel);

        double PitchV = aVel.x * DEG;
        double RollV = aVel.z * DEG;
        double YawV = aVel.y * DEG;

        // --- PITCH ---
        double CurrentRate = PitchV;
        double CommandedRate = asin(input.x) * DEG;
        CommandedRate = ((min(fabs(CommandedRate), pitchrate)) * SignOf(CommandedRate));
        double pDelta = (CommandedRate - CurrentRate);

        VECTOR3 tgtrotlevel;
        GetAttitudeRotLevel(tgtrotlevel);

        if (PitchCtrl) {
            if (fabs(pDelta) > pdb) {
                ReqAngularAcc.x = pDelta * RAD / ReqTime;
                ReqTorque.x = PMI.x * GetMass() * ReqAngularAcc.x;
                tgtrotlevel.x = min(fabs(ReqTorque.x / MaxTorque.x), 1.0) * SignOf(ReqTorque.x);
            } else { tgtrotlevel.x = 0; }
            SetAttitudeRotLevel(0, tgtrotlevel.x);
        }

        // --- ROLL ---
        CurrentRate = RollV;
        CommandedRate = asin(input.z) * DEG;
        CommandedRate = ((min(fabs(CommandedRate), rollrate)) * SignOf(CommandedRate));
        double rDelta = (CommandedRate - CurrentRate);

        if (RollCtrl) {
            if (fabs(rDelta) > pdb) {
                ReqAngularAcc.z = rDelta * RAD / ReqTime;
                ReqTorque.z = PMI.z * GetMass() * ReqAngularAcc.z;
                tgtrotlevel.z = min(fabs(ReqTorque.z / MaxTorque.z), 1.0) * SignOf(ReqTorque.z);
            } else { tgtrotlevel.z = 0; }
            SetAttitudeRotLevel(2, tgtrotlevel.z);
        }

        // --- YAW ---
        CurrentRate = YawV;
        CommandedRate = asin(input.y) * DEG;
        CommandedRate = ((min(fabs(CommandedRate), yawrate)) * SignOf(CommandedRate));
        double yDelta = (CommandedRate - CurrentRate);

        if (YawCtrl) {
            if (fabs(yDelta) > pdb) {
                ReqAngularAcc.y = yDelta * RAD / ReqTime;
                ReqTorque.y = PMI.y * GetMass() * ReqAngularAcc.y;
                tgtrotlevel.y = min(fabs(ReqTorque.y / MaxTorque.y), 1.0) * SignOf(ReqTorque.y);
            } else { tgtrotlevel.y = 0; }
            SetAttitudeRotLevel(1, tgtrotlevel.y);
        }
    }
}

// ==============================================================
// Autopilot State Toggles
// ==============================================================

void Multistage2026::ToggleAP() {
    APstat = !APstat;
}

void Multistage2026::ToggleAttCtrl(bool Pitch, bool Yaw, bool Roll) {
    if (Pitch && Yaw && Roll) {
        AttCtrl = !AttCtrl;
    } else if (Pitch && !Yaw && !Roll) {
        PitchCtrl = !PitchCtrl;
    } else if (!Pitch && Yaw && !Roll) {
        YawCtrl = !YawCtrl;
    } else if (!Pitch && !Yaw && Roll) {
        RollCtrl = !RollCtrl;
    }
}

void Multistage2026::killAP() {
    SetThrusterGroupLevel(THGROUP_MAIN, 0);
    SetThrusterGroupLevel(THGROUP_ATT_PITCHUP, 0);
    SetThrusterGroupLevel(THGROUP_ATT_PITCHDOWN, 0);
    SetThrusterGroupLevel(THGROUP_ATT_YAWLEFT, 0);
    SetThrusterGroupLevel(THGROUP_ATT_YAWRIGHT, 0);
    SetThrusterGroupLevel(THGROUP_ATT_BANKRIGHT, 0);
    SetThrusterGroupLevel(THGROUP_ATT_BANKLEFT, 0);
    ActivateNavmode(NAVMODE_KILLROT);
}

// ==============================================================
// Vinka Script / Guidance Logic
// ==============================================================

void Multistage2026::VinkaUpdateRollTime() {
    // Finds the end time of the pitch program to set the roll duration
    int pitchIdx = VinkaFindFirstPitch();
    if (pitchIdx > 0 && pitchIdx < 150) {
        int rollIdx = VinkaFindRoll();
        if (rollIdx > 0 && rollIdx < 150) {
             Gnc_step[rollIdx].time_fin = Gnc_step[pitchIdx].time - 1;
        }
    }
}

void Multistage2026::VinkaComposeGNCSteps() {
    char logbuff[256];

    for (int i = 0; i <= nsteps; i++) {
        Gnc_step[i].executed = FALSE;
        
        // Lowercase command
        for (int z = 0; z < 128; z++) {
            Gnc_step[i].Comand[z] = tolower(Gnc_step[i].Comand[z]);
        }

        // Command Mapping
        if (strcmp(Gnc_step[i].Comand, "engine") == 0) Gnc_step[i].GNC_Comand = CM_ENGINE;
        else if (strcmp(Gnc_step[i].Comand, "roll") == 0) Gnc_step[i].GNC_Comand = CM_ROLL;
        else if (strcmp(Gnc_step[i].Comand, "pitch") == 0) Gnc_step[i].GNC_Comand = CM_PITCH;
        else if (strcmp(Gnc_step[i].Comand, "fairing") == 0) Gnc_step[i].GNC_Comand = CM_FAIRING;
        else if (strcmp(Gnc_step[i].Comand, "les") == 0) Gnc_step[i].GNC_Comand = CM_LES;
        else if (strcmp(Gnc_step[i].Comand, "disablepitch") == 0) Gnc_step[i].GNC_Comand = CM_DISABLE_PITCH;
        else if (strcmp(Gnc_step[i].Comand, "disableroll") == 0) Gnc_step[i].GNC_Comand = CM_DISABLE_ROLL;
        else if (strcmp(Gnc_step[i].Comand, "disablejettison") == 0) Gnc_step[i].GNC_Comand = CM_DISABLE_JETTISON;
        else if (strcmp(Gnc_step[i].Comand, "playsound") == 0) Gnc_step[i].GNC_Comand = CM_PLAY;
        else if (strcmp(Gnc_step[i].Comand, "jettison") == 0) Gnc_step[i].GNC_Comand = CM_JETTISON;
        else if (strcmp(Gnc_step[i].Comand, "target") == 0) Gnc_step[i].GNC_Comand = CM_TARGET;
        else if (strcmp(Gnc_step[i].Comand, "aoa") == 0) Gnc_step[i].GNC_Comand = CM_AOA;
        else if (strcmp(Gnc_step[i].Comand, "attitude") == 0) Gnc_step[i].GNC_Comand = CM_ATTITUDE;
        else if (strcmp(Gnc_step[i].Comand, "spin") == 0) Gnc_step[i].GNC_Comand = CM_SPIN;
        else if (strcmp(Gnc_step[i].Comand, "inverse") == 0) Gnc_step[i].GNC_Comand = CM_INVERSE;
        else if (strcmp(Gnc_step[i].Comand, "engineout") == 0) Gnc_step[i].GNC_Comand = CM_ENGINEOUT;
        else if (strcmp(Gnc_step[i].Comand, "orbit") == 0) Gnc_step[i].GNC_Comand = CM_ORBIT;
        else if (strcmp(Gnc_step[i].Comand, "defap") == 0) Gnc_step[i].GNC_Comand = CM_DEFAP;
        else if (strcmp(Gnc_step[i].Comand, "glimit") == 0) Gnc_step[i].GNC_Comand = CM_GLIMIT;
        else if (strcmp(Gnc_step[i].Comand, "destroy") == 0) Gnc_step[i].GNC_Comand = CM_DESTROY;
        else if (strcmp(Gnc_step[i].Comand, "explode") == 0) Gnc_step[i].GNC_Comand = CM_EXPLODE;
        else if (strcmp(Gnc_step[i].Comand, "noline") == 0) Gnc_step[i].GNC_Comand = CM_NOLINE;

        Gnc_step[0].time_fin = -10000;

        switch (Gnc_step[i].GNC_Comand) {
        case CM_ENGINE:
            Gnc_step[i].val_init = Gnc_step[i].trval1;
            Gnc_step[i].val_fin = Gnc_step[i].trval2;
            if (Gnc_step[i].val_fin == 0) { Gnc_step[i].val_fin = -1; }
            Gnc_step[i].time_init = Gnc_step[i].time;
            Gnc_step[i].duration = Gnc_step[i].trval3;
            if (Gnc_step[i].duration == 0) { Gnc_step[i].duration = 0.01; }
            Gnc_step[i].time_fin = Gnc_step[i].time_init + Gnc_step[i].duration;
            break;

        case CM_ROLL:
            Gnc_step[i].time_init = Gnc_step[i].time;
            Gnc_step[i].val_init = Gnc_step[i].trval2;
            Gnc_step[i].val_fin = Gnc_step[i].trval4;
            Gnc_step[i].duration = 60;
            Gnc_step[i].time_fin = Gnc_step[i].time_init + Gnc_step[i].duration;
            break;

        case CM_PITCH:
            Gnc_step[i].time_init = Gnc_step[i].time;
            Gnc_step[i].val_init = Gnc_step[i].trval1;
            Gnc_step[i].val_fin = Gnc_step[i].trval2;
            Gnc_step[i].duration = Gnc_step[i].trval3;
            Gnc_step[i].time_fin = Gnc_step[i].time_init + Gnc_step[i].duration;
            break;

        case CM_ORBIT:
            Gnc_step[i].time_init = Gnc_step[i].time;
            Gnc_step[i].time_fin = Gnc_step[i].time_init + 10000;
            tgtapo = Gnc_step[i].trval2 * 1000;
            tgtperi = Gnc_step[i].trval1 * 1000;
            tgtinc = Gnc_step[i].trval3 * RAD;
            VinkaMode = Gnc_step[i].trval4;
            if (VinkaMode == 0) { VinkaMode = 1; }
            GT_InitPitch = Gnc_step[i].trval5 * RAD;
            tgtabside = Gnc_step[i].trval6 * 1000;
            wPeg = TRUE;
            CalculateTargets();
            
            snprintf(logbuff, sizeof(logbuff), "%s: Orbit Call Found! Targets: Apogee:%.1f Perigee:%.1f Inclination:%.1f", GetName(), tgtapo, tgtperi, tgtinc * DEG);
            oapiWriteLog(logbuff);
            break;

        // Default handler for single events (Jettison, etc.)
        default:
            Gnc_step[i].time_init = Gnc_step[i].time;
            Gnc_step[i].time_fin = Gnc_step[i].time_init + 2;
            
            if (Gnc_step[i].GNC_Comand == CM_AOA) {
                Gnc_step[i].duration = Gnc_step[i].trval2;
                if (Gnc_step[i].duration <= 0) Gnc_step[i].time_fin = Gnc_step[i].time_init + 60;
                else Gnc_step[i].time_fin = Gnc_step[i].time_init + Gnc_step[i].duration;
                Gnc_step[i].val_init = Gnc_step[i].trval1 * RAD;
            }
            if (Gnc_step[i].GNC_Comand == CM_ATTITUDE) {
                Gnc_step[i].duration = Gnc_step[i].trval4;
                if (Gnc_step[i].duration <= 0) Gnc_step[i].time_fin = Gnc_step[i].time_init + 60;
                else Gnc_step[i].time_fin = Gnc_step[i].time_init + Gnc_step[i].duration;
            }
            break;
        }
    }

    if (!wPeg) {
        VinkaUpdateRollTime();
    }
}

// ==============================================================
// Step Consumption (Runtime)
// ==============================================================

void Multistage2026::VinkaConsumeStep(int step) {
    if (Gnc_step[step].time_fin <= MET) { Gnc_step[step].executed = TRUE; }

    if (Gnc_step[step].executed == FALSE) {
        switch (Gnc_step[step].GNC_Comand) {
        case CM_ENGINE:
            VinkaEngine(step);
            break;
        case CM_ROLL:
            VinkaRoll(step);
            break;
        case CM_PITCH:
            VinkaPitch(step);
            break;
        case CM_FAIRING:
            if ((wFairing == 1) && (GetAltitude() >= Gnc_step[step].val_init)) {
                Jettison(TFAIRING, 0);
                Gnc_step[step].time_fin = MET;
                Gnc_step[step].executed = TRUE;
            }
            break;
        case CM_LES:
            if ((wLes == TRUE) && (GetAltitude() >= Gnc_step[step].val_init)) {
                Jettison(TLES, 0);
                Gnc_step[step].time_fin = MET;
                Gnc_step[step].executed = TRUE;
            }
            break;
        case CM_JETTISON:
            // Simulate J Keypress or direct jettison
            // SendBufferedKey(OAPI_KEY_J, true, kstate); -> Replaced with direct call
            // Using generic Jettison logic:
            if (currentBooster < nBoosters) Jettison(TBOOSTER, currentBooster);
            else if (currentStage < nStages - 1) Jettison(TSTAGE, currentStage);
            Gnc_step[step].executed = TRUE;
            break;
            
        case CM_ORBIT:
            if (Configuration == 1) { // Running as Main Rocket
                runningPeg = TRUE;
                if (GetAltitude() < altsteps[0]) {
                    TgtPitch = 90 * RAD - Misc.VerticalAngle;
                    Attitude(TgtPitch, GetBank(), GetHeading(), 8, 0, 5);
                }
                else if ((GetAltitude() >= altsteps[0]) && (GetAltitude() < altsteps[1])) {
                    TgtPitch = 89.9 * RAD - Misc.VerticalAngle;
                    Attitude(TgtPitch, (0.5 * (1 - VinkaMode) * PI), GetProperHeading(), 8, 20, 5);
                }
                else if ((GetAltitude() >= altsteps[1]) && (GetAltitude() < altsteps[2])) {
                    TgtPitch = GT_InitPitch;
                    // Linear interpolation logic
                    double deltaAltitude = altsteps[2] - GetAltitude();
                    double deltaTime = sqrt((2 * deltaAltitude) / (1.5 * getabsacc() - g0));
                    double PitchRate = fabs((GetPitch() - GT_InitPitch) / deltaTime);
                    Attitude(GT_InitPitch, (0.5 * (1 - VinkaMode) * PI), GetProperHeading(), PitchRate * DEG, 20, PitchRate * DEG);
                }
                else if ((GetAltitude() >= altsteps[2]) && (GetAltitude() < altsteps[3])) {
                    double DesiredPitch = GetPitch() - VinkaMode * GetAOA() + (-0.7 * RAD);
                    TgtPitch = DesiredPitch;
                    Attitude(DesiredPitch, (0.5 * (1 - VinkaMode) * PI), GetProperHeading(), 8, 5, 8);
                }
                else {
                    if (GetThrusterGroupLevel(THGROUP_MAIN) > 0.1) {
                        PEG(); // PEG Guidance Call
                        if (CutoffCheck() == TRUE) {
                            Gnc_step[step].time_fin = MET + 1;
                            Gnc_step[step].executed = TRUE;
                        }
                    }
                }
            }
            break;

        case CM_DESTROY:
            Gnc_step[step].executed = TRUE;
            oapiDeleteVessel(GetHandle());
            break;
        }
    }
}

// ==============================================================
// Step Helpers (Engine, Roll, Pitch)
// ==============================================================

void Multistage2026::VinkaEngine(int step) {
    double progress = (MET - Gnc_step[step].time_init) / (Gnc_step[step].time_fin - Gnc_step[step].time_init);
    double DesiredEngineLevel = (Gnc_step[step].val_init + (Gnc_step[step].val_fin - Gnc_step[step].val_init) * progress) / 100.0;
    SetThrusterGroupLevel(THGROUP_MAIN, DesiredEngineLevel);
}

void Multistage2026::VinkaRoll(int step) {
    double progress = (MET - Gnc_step[step].time_init) / (Gnc_step[step].time_fin - Gnc_step[step].time_init);
    double DesiredPitch = (Gnc_step[step].val_init + (Gnc_step[step].val_fin - Gnc_step[step].val_init) * progress) * RAD;
    
    TgtPitch = DesiredPitch;
    double RollRate = 180 / ((Gnc_step[step].time_fin - 1) - Gnc_step[step].time_init);
    
    VinkaAzimuth = Gnc_step[step].trval3 * RAD;
    VinkaMode = Gnc_step[step].trval5;
    
    Attitude(DesiredPitch, (0.5 * (1 - VinkaMode) * PI), VinkaAzimuth, 2, RollRate, 5);
}

void Multistage2026::VinkaPitch(int step) {
    double progress = (MET - Gnc_step[step].time_init) / (Gnc_step[step].time_fin - Gnc_step[step].time_init);
    double DesiredPitch = (Gnc_step[step].val_init + (Gnc_step[step].val_fin - Gnc_step[step].val_init) * progress) * RAD;
    
    TgtPitch = DesiredPitch;
    if (spinning) {
        Attitude(DesiredPitch, GetBank(), GetProperHeading(), 0, 0, 0);
    } else {
        Attitude(DesiredPitch, (0.5 * (1 - VinkaMode) * PI), GetProperHeading(), 10, 10, 10);
    }
}

void Multistage2026::VinkaAutoPilot() {
    for (int q = 1; q <= VinkaGetStep(MET); q++) {
        VinkaConsumeStep(q);
    }
}

// ==============================================================
// Guidance File Parsing Utilities
// ==============================================================

int Multistage2026::VinkaGetStep(double met) {
    int n = 0;
    for (int i = 1; i <= nsteps; i++) {
        if ((met >= Gnc_step[i].time) && (Gnc_step[i].GNC_Comand != CM_NOLINE)) {
            n += 1;
        }
    }
    return n;
}

void Multistage2026::VinkaCheckInitialMet() {
    if (Configuration == 0) {
        MET = Gnc_step[1].time;
    }
    if (nsteps == 0) {
        MET = -1;
    }
}

int Multistage2026::VinkaFindRoll() {
    for (int i = 1; i <= nsteps; i++) {
        if (Gnc_step[i].GNC_Comand == CM_ROLL) return i;
    }
    return 0;
}

int Multistage2026::VinkaFindFirstPitch() {
    for (int i = 1; i <= nsteps; i++) {
        if (Gnc_step[i].GNC_Comand == CM_PITCH) return i;
    }
    return 0;
}

double Multistage2026::VinkaFindEndTime() {
    double EndTime = 0;
    for (int q = 0; q <= nsteps; q++) {
        if (Gnc_step[q].time_fin > EndTime) {
            EndTime = Gnc_step[q].time_fin;
        }
    }
    return EndTime;
}

int Multistage2026::VinkaCountSteps() {
    int q = 0;
    for (int i = 1; i < 150; i++) {
        if (Gnc_step[i].GNC_Comand != CM_NOLINE) {
            q += 1;
        }
    }
    return q;
}

void Multistage2026::VinkaRearrangeSteps() {
    int index = 1;
    GNC_STEP trans[150];
    GNC_STEP temp;

    // Reset buffer
    for (int q = 0; q < 150; q++) {
        trans[q].GNC_Comand = CM_NOLINE;
        trans[q].time_fin = -10000;
    }

    // Filter valid steps
    for (int i = 1; i < 150; i++) {
        if (Gnc_step[i].GNC_Comand != CM_NOLINE) {
            trans[index] = Gnc_step[i];
            index++;
        }
    }

    // Copy back
    for (int q = 1; q < 150; q++) {
        Gnc_step[q] = trans[q];
    }

    // Sort by time
    for (int k = 1; k < index - 1; k++) {
        for (int j = k + 1; j < index; j++) {
            if (Gnc_step[k].time > Gnc_step[j].time) {
                temp = Gnc_step[k];
                Gnc_step[k] = Gnc_step[j];
                Gnc_step[j] = temp;
            }
        }
    }
}
