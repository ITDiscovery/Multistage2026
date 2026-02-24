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
// ==============================================================
//                      MultiStage2026
//                  Linux Port & Refactor
// ==============================================================

#define ORBITER_MODULE
#include <math.h>
#include <stdio.h>
#include <sstream>
#include <algorithm>
#include <stdarg.h>
#include "Orbitersdk.h"
#include "Multistage2026.h"

const double mu = 398600.4418e9;

// ==============================================================
// Creation & Destruction
// This is the "Birth" and "Death" of the rocket. It initializes
// every variable (mass, stage count, fuel, flags) to zero or
// default values so you don't start with garbage data. The
// destructor frees the memory used by arrays (like psg,
// payloadrotatex).
// ==============================================================

Multistage2026::Multistage2026(OBJHANDLE hObj, int fmodel) :VESSEL3(hObj, fmodel) {
    bVesselInitialized = false;
    DeveloperMode = false;
    HangarMode = false;
    GetCurrentDirectory(MAXLEN, OrbiterRoot);
    nStages = 0;
    nBoosters = 0;
    nInterstages = 0;
    nTextures = 0;
    nParticles = 0;
    currentStage = 0;
    currentBooster = 0;
    currentInterstage = 0;
    CogElev = 0;
    Misc.VerticalAngle = 0;
    Misc.drag_factor = 1;
    OrbiterRoot[0] = '\0';

    wBoosters = false;
    wFairing = 0;
    wLes = false;
    wAdapter = false;
    wMach = false;
    wVent = false;
    wLaunchFX = false;
    Complex = false;
    stage_ignition_time = 0;
    currentDelta = 0.0;
    perror = 0.0;
    yerror = 0.0;
    rerror = 0.0;

   // Resize vectors so they actually have slots 0 through 9
    stage.resize(10);
    booster.resize(10);
    payload.resize(10);
    Particle.resize(20);

    // NEW: Initialize them to prevent GetProperPS from reading garbage
    for (unsigned int i = 0; i < Particle.size(); i++) {
        Particle[i].ParticleName[0] = '\0'; // Empty string
        // Initialize other members if necessary, but name is critical for lookups
    }

    // -------------------------------------------------------------------------
    // 1. Initialize STAGES
    // -------------------------------------------------------------------------
    currentStage = 0;
    for (unsigned int i = 0; i < 10; i++) {
        stage.at(i) = STAGE();
        stage.at(i).Ignited = false;
        stage.at(i).reignitable = true;
        stage.at(i).batteries.wBatts = false;
        stage.at(i).waitforreignition = 0;
        stage.at(i).StageState = STAGE_SHUTDOWN;
        stage.at(i).DenyIgnition = false;
        stage.at(i).ParticlesPacked = false;
        stage.at(i).ParticlesPackedToEngine = 0;
        stage.at(i).defpitch = false;
        stage.at(i).defroll = false;
        stage.at(i).defyaw = false;
        stage.at(i).wps1 = false;
        stage.at(i).wps2 = false;
        stage.at(i).eng_tex[0] = '\0';
        stage.at(i).tank = NULL;
        payload[i] = PAYLOAD();
    }

    // -------------------------------------------------------------------------
    // 2. Initialize BOOSTERS
    // -------------------------------------------------------------------------
    // Resize to safe defaults
    if (booster.size() < 10) booster.resize(10);

    for (unsigned int i = 0; i < booster.size(); i++) {
        booster.at(i) = BOOSTER(); // Reset to defaults
        // Safety: Nullify Pointers
        booster.at(i).tank = NULL;
        // Boosters use a Fixed Array (std::array), so we .fill() it
        booster.at(i).th_booster_h.fill(NULL);
        // Safety: Clear Flags
        booster.at(i).Ignited = false;
        booster.at(i).wps1 = false;
        booster.at(i).wps2 = false;

        // Safety: Empty Strings
        booster.at(i).eng_pstream1 = "";
        booster.at(i).eng_pstream2 = "";
        booster.at(i).ParticlesPacked = false;
        booster.at(i).ParticlesPackedToEngine = 0;
    }

    MET = 0.0;
    APstat = false;
    AJdisabled = false;
    rolldisabled = false;
    pitchdisabled = false;
    Gnc_running = 0;
    spinning = false;
    lvl = 1.0;

    for (unsigned int i = 0; i < 57; i++) {
        Ssound.GncStepSound[i] = -5;
    }

    PegDesPitch = 35 * RAD;
    PegPitchLimit = 35 * RAD;
    DeltaUpdate = 0;
    GT_InitPitch = 89.5 * RAD;
    UpdatePegTimer = 0.0;
    UpdateComplex = 0.0;
    wPeg = false;
    PegMajorCycleInterval = 0.1;
    runningPeg = false;
    AttCtrl = true;
    PitchCtrl = true;
    YawCtrl = true;
    RollCtrl = true;
    TgtPitch = 0;
    eps = -9000000000000;
    failureProbability = -1000;
    timeOfFailure = -10000000;
    wFailures = false;
    failed = false;
    killDMD = false;
    stepsloaded = false;

    for (unsigned int i = 0; i < 150; i++) {
        Gnc_step[i].Cmd = CM_NOLINE;
    }

    updtlm = 0.0;
    tlmidx = 0;
    writetlmTimer = 0.0;
    tlmnlines = 0;
    wReftlm = false;

    for (int i = 0; i < TLMSECS; i++) {
        tlmAlt[i].x = 0; tlmAlt[i].y = 0;
        tlmSpeed[i].x = 0; tlmSpeed[i].y = 0;
        tlmPitch[i].x = 0; tlmPitch[i].y = 0;
        tlmThrust[i].x = 0; tlmThrust[i].y = 0;
        tlmMass[i].x = 0; tlmMass[i].y = 0;
        tlmVv[i].x = 0; tlmVv[i].y = 0;
        tlmAcc[i].x = 0; tlmAcc[i].y = 0;
    }

    updtboiloff = 0.0;
    for (unsigned int i = 0; i < 100; i++) {
        coeff[i] = 0.0;
    }

    avgcoeff = 0.0;
    MECO_Counter = 0;
    MECO_TEST = 0.0;
    TMeco = 600.0;
    nPsg = 0;
    particlesdt = 0.0;
    GrowingParticles = false;
    RefPressure = 101400.0;
    wRamp = false;
    NoMoreRamp = false;
    
    col_d.a = 0; col_d.b = 1; col_d.g = 0.8; col_d.r = 0.9;
    col_s.a = 0; col_s.b = 1; col_s.g = 0.8; col_s.r = 1.9;
    col_a.a = 0; col_a.b = 0; col_a.g = 0; col_a.r = 0;
    col_white.a = 0; col_white.b = 1; col_white.g = 1; col_white.r = 1;

    th_main_level = 0.0;
    launchFx_level = 0.0;
    hangaranims = _V(1.3, -10, 57.75);
    wCrawler = false;
    wCamera = false;
    AttToMSPad = false;
    MsPadZ = _V(0, 0, -50);

    for (int i = 0; i < 10; i++) A[i] = 0.0;
    AttToCrawler = nullptr;
    AttToHangar = nullptr;
    AttToRamp = nullptr;
    Azimuth = 0.0;
    for (int i = 0; i < 10; i++) B[i] = 0.0;
    CamDLat = 0.0;
    CamDLng = 0.0;
    Configuration = 0;
    for (int i = 0; i < 10; i++) DeltaA[i] = 0.0;
    for (int i = 0; i < 10; i++) DeltaB[i] = 0.0;
    GT_IP_Calculated = 0.0;
    J = 0;
    Ku = 0.0;
    MaxTorque = _V(0, 0, 0);
    MyID = 0.0;
    NN = 0;
    for (int i = 0; i < 10; i++) OmegaS[i] = 0.0;
    PadHangar = nullptr;

    for (int i = 0; i < 7200; i++) {
        ReftlmAcc[i] = {0,0}; ReftlmAlt[i] = {0,0}; ReftlmMass[i] = {0,0};
        ReftlmSpeed[i] = {0,0}; ReftlmPitch[i] = {0,0}; ReftlmThrust[i] = {0,0}; ReftlmVv[i] = {0,0};
    }

    RotMatrix = {};
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 5; j++) {
            RotatePayloadAnim_x[i][j] = 0;
            RotatePayloadAnim_y[i][j] = 0;
            RotatePayloadAnim_z[i][j] = 0;
        }
    }

    ShipSpeed = _V(0, 0, 0);
    for (int i = 0; i < 10; i++) T[i] = 0.0;
    TotalHeight = 0.0;
    Tu = 0.0;
    VertVel = 0.0;
    VinkaStatus = PROG_IDLE;
    VinkaAzimuth = 0.0;
    VinkaMode = 0.0;
    for (int i = 0; i < 10; i++) VthetaAtStaging[i] = 0.0;
    for (int i = 0; i < 10; i++) a_[i] = 0.0;
    for (int i = 0; i < 10; i++) a_fin[i] = 0.0;
    for (int i = 0; i < 4; i++) altsteps[i] = 0.0;

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 5; j++) {
            anim_x[i][j] = nullptr;
            anim_y[i][j] = nullptr;
            anim_z[i][j] = nullptr;
        }
    }

    cent = 0.0; cent_term = 0.0; coc = {}; config = nullptr; currentPayload = 0;
    for (int i = 0; i < 10; i++) delta_r[i] = 0.0;
    for (int i = 0; i < 10; i++) delta_rdot[i] = 0.0;
    el = {}; epsfin = 0.0;
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 32; j++) exhaustN[i][j] = 0;
    }

    fhat = _V(0, 0, 0); g = 0.0; g0 = 0.0; g_term = 0.0; h = 0.0;
    hCrawler = nullptr; hasFairing = false; hhangar = nullptr;
    hhat = _V(0, 0, 0); hramp = nullptr; hvec = _V(0, 0, 0);
    initlat = 0.0; initlong = 0.0; kd = 0.0; ki = 0.0; kp = 0.0;
    for (int i = 0; i < 10; i++) live_a[i] = nullptr;

    loadedConfiguration = 0; loadedCurrentBooster = 0; loadedCurrentInterstage = 0;
    loadedCurrentPayload = 0; loadedCurrentStage = 0; loadedGrowing = false;
    loadedMET = 0.0; loadedtlmlines = 0; loadedwFairing = 0;
    mass = 0.0; nPayloads = 0; nPsh = 0; note = nullptr; nsteps = 0;
    omega = 0.0; padramp = nullptr;

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 5; j++) {
            payloadrotatex[i][j] = nullptr;
            payloadrotatey[i][j] = nullptr;
            payloadrotatez[i][j] = nullptr;
        }
    }

    pintegral = 0.0; psg = {}; r = 0.0;
    for (int i = 0; i < 10; i++) r_T[i] = 0.0;
    for (int i = 0; i < 10; i++) r_in[i] = 0.0;
    rdot = 0.0;
    for (int i = 0; i < 10; i++) rdot_T[i] = 0.0;
    for (int i = 0; i < 10; i++) rdot_in[i] = 0.0;
    rhat = _V(0, 0, 0); rt = 0.0; rvec = _V(0, 0, 0);
    for (int i = 0; i < 10; i++) tau_[i] = 0.0;
    tgtabside = 0.0; tgtinc = 0.0; tgtapo = 0.0; tgtperi = 0.0; tgtvv = 0.0;
    thetahat = _V(0, 0, 0); thg_h_main = nullptr; thrust = 0; v = 0.0; vCrawler = nullptr;
    for (int i = 0; i < 10; i++) v_e[i] = 0.0;
    vhangar = nullptr; vramp = nullptr; vs2 = {}; vshangar = {}; vsramp = {};
    vtheta = 0.0; vvec = _V(0, 0, 0); z = 0.0;

    buffreset[0] = '\0'; dataparsed[0] = '\0'; fileini[0] = '\0';
    guidancefile[0] = '\0'; logbuff[0] = '\0'; tex = TEX(); tlmfile[0] = '\0';
}

Multistage2026::~Multistage2026() {
    // 1. psg is a std::vector. .clear() is okay, but not strictly needed 
    // as the vector's own destructor will handle its internal memory.
    psg.clear();

    // 2. FIXED: Payload rotation handles. 
    // In your code, you use 'new MGROUP_ROTATE'.
    // We must ensure we don't delete nullptrs or double-free.
    for (int i = 0; i < 5; i++) { 
        for (int j = 0; j < 10; j++) {
            if (payloadrotatex[i][j]) {
                delete payloadrotatex[i][j];
                payloadrotatex[i][j] = nullptr; // Prevent invalid pointer access
            }
            if (payloadrotatey[i][j]) {
                delete payloadrotatey[i][j];
                payloadrotatey[i][j] = nullptr;
            }
            if (payloadrotatez[i][j]) {
                delete payloadrotatez[i][j];
                payloadrotatez[i][j] = nullptr;
            }
        }
    }
}

// ==============================================================
// Utility Functions
// These are utility functions used throughout the code to calculate
// positions and parse numbers from the Config file.
// ==============================================================

VECTOR4F Multistage2026::_V4(double x, double y, double z, double t) {
    VECTOR4F v4; v4.x = x; v4.y = y; v4.z = z; v4.t = t; return v4;
}

VECTOR2 Multistage2026::_V2(double x, double y) {
    VECTOR2 v2; v2.x = x; v2.y = y; return v2;
}

int Multistage2026::SignOf(double X) {
    return (int)(X / abs(X));
}

bool Multistage2026::IsOdd(int integer) {
    return (integer % 2 == 0);
}

// Transform string to VECTOR3 (Linux compatible)
VECTOR3 Multistage2026::CharToVec(const std::string& str) {
    std::string s = str;
    s.erase(std::remove(s.begin(), s.end(), '('), s.end());
    s.erase(std::remove(s.begin(), s.end(), ')'), s.end());
    std::replace(s.begin(), s.end(), ';', ',');
    std::stringstream ss(s);
    double x = 0.0, y = 0.0, z = 0.0;
    char comma;
    ss >> x >> comma >> y >> comma >> z;
    return _V(x, y, z);
}

VECTOR3 Multistage2026::RotateVecZ(VECTOR3 input, double Angle) {
    return _V(input.x * cos(Angle) - input.y * sin(Angle), input.x * sin(Angle) + input.y * cos(Angle), input.z);
}

MATRIX3 Multistage2026::RotationMatrix(VECTOR3 angles) {
    MATRIX3 RM_X = _M(1, 0, 0, 0, cos(angles.x), -sin(angles.x), 0, sin(angles.x), cos(angles.x));
    MATRIX3 RM_Y = _M(cos(angles.y), 0, sin(angles.y), 0, 1, 0, -sin(angles.y), 0, cos(angles.y));
    MATRIX3 RM_Z = _M(cos(angles.z), -sin(angles.z), 0, sin(angles.z), cos(angles.z), 0, 0, 0, 1);
    return mul(RM_Z, mul(RM_Y, RM_X));
}

void oapiWriteLogV(const char* format, ...) {
    char va_buff[1024];
    va_list va;
    va_start(va, format);
    vsnprintf(va_buff, sizeof(va_buff), format, va);
    va_end(va);
    oapiWriteLog((char*)va_buff);
    fflush(stdout);
}

// ==============================================================
// Core Logic: Reset & Update
// This is a "Soft Reboot" for the rocket. It clears all meshes,
// thrusters, and propellant, then re-reads the .ini file and
// rebuilds the rocket. It was used heavily in Dev Mode (hitting
// Spacebar to reload configs without restarting Orbiter), and is
// retained here as it is used during the initial load.
// ==============================================================

void Multistage2026::ResetVehicle(VECTOR3 hangaranimsV, bool Ramp) {
    ClearMeshes();
    ClearThrusterDefinitions();
    ClearPropellantResources();
    ClearAttachments();

    initGlobalVars();
    wRamp = Ramp;
    if (wRamp) {
        oapiDeleteVessel(hramp);
        wRamp = false;
    }

    char tempFile[MAXLEN];
    strcpy(tempFile, OrbiterRoot);
    strcat(tempFile, "/"); // Linux Path
    strcat(tempFile, fileini);

    // SAFE STRING FORMATTING
    parseinifile(tempFile);

    currentBooster = loadedCurrentBooster;
    currentInterstage = loadedCurrentInterstage;
    currentStage = loadedCurrentStage;
    currentPayload = loadedCurrentPayload;
    wFairing = loadedwFairing;
    Configuration = loadedConfiguration;
    GrowingParticles = loadedGrowing;
    MET = loadedMET;

    if ((currentInterstage > currentStage) || (currentInterstage > nInterstages) || (currentInterstage >= stage.at(currentStage).IntIncremental)) { stage.at(currentStage).wInter = false; }
    if ((wFairing == 1) && (hasFairing == false)) { wFairing = 0; }
    if (Configuration == 0) {
        if (hasFairing == TRUE) wFairing = 1;
        currentStage = 0;
        currentPayload = 0;
        currentBooster = 0;
        currentInterstage = 0;
    }

    parseGuidanceFile(guidancefile);
    VehicleSetup();
    LoadMeshes();

    vs2.rvel = _V(0, 0, 0);
    clbkSetStateEx(&vs2);
    hangaranims = hangaranimsV;
    clbkPostCreation();
}

// ==============================================================
// Physics Getters
// These calculate flight data (Delta-V, Burn time remaining) for
// the MFD and the autopilot.
// ==============================================================

double Multistage2026::GetHeading() {
    double Heading;
    oapiGetHeading(GetHandle(), &Heading);
    return Heading;
}

double Multistage2026::GetOS() {
    OBJHANDLE hearth = GetSurfaceRef();
    VECTOR3 relvel;
    GetRelativeVel(hearth, relvel);
    return length(relvel);
}

double Multistage2026::GetMassAtStage(int MassStage, bool empty) {
    double Mass = 0;
    int add = (empty == TRUE) ? 1 : 0;
    for (int i = MassStage; i < nStages; i++) Mass += stage.at(i).emptymass;
    for (int j = MassStage + add; j < nStages; j++) Mass += stage[j].fuelmass;
    for (int k = 0; k < nPayloads; k++) Mass += payload[k].mass;
    if (hasFairing) Mass += fairing.emptymass;
    if (wAdapter) Mass += Adapter.emptymass;
    if (wLes) Mass += Les.emptymass;
    return Mass;
}

double Multistage2026::StageDv(int dvstage) {
    return stage[dvstage].isp * log(GetMassAtStage(dvstage, false) / GetMassAtStage(dvstage, TRUE));
}

double Multistage2026::DvAtStage(int dvatstage) {
    double rdvas = 0;
    for (int i = dvatstage; i < nStages; i++) rdvas += StageDv(i);
    return rdvas;
}

double Multistage2026::CurrentStageRemDv() {
    return stage.at(currentStage).isp * log((GetMassAtStage(currentStage, TRUE) + GetPropellantMass(stage.at(currentStage).tank)) / GetMassAtStage(currentStage, TRUE));
}

double Multistage2026::StageRemDv(int dvstage) {
    return stage[dvstage].isp * log((GetMassAtStage(dvstage, TRUE) + GetPropellantMass(stage[dvstage].tank)) / GetMassAtStage(dvstage, TRUE));
}

double Multistage2026::RemBurnTime(int rbtstage, double level) {
    if (stage[rbtstage].thrust <= 0) return 0;
    return stage[rbtstage].isp * GetPropellantMass(stage[rbtstage].tank) / (stage[rbtstage].thrust * level);
}

double Multistage2026::BoosterRemBurnTime(int rbtbooster, double level) {
    if (booster.at(rbtbooster).thrust <= 0) return 0;
    return booster.at(rbtbooster).isp * GetPropellantMass(booster.at(rbtbooster).tank) / ((booster.at(rbtbooster).thrust * booster.at(rbtbooster).N) * level);
}

void Multistage2026::initGlobalVars() {
        nStages = 0;
        nBoosters = 0;
        nInterstages = 0;
        nTextures = 0;
        nParticles = 0;
        currentStage = 0;
        currentBooster = 0;
        currentInterstage = 0;
        CogElev = 0;
        Misc.VerticalAngle = 0;
        Misc.drag_factor = 1;

        // Initialize Orbiter with the Current Directory
        GetCurrentDirectory(MAXLEN, OrbiterRoot);

        wBoosters = false;
        wFairing = 0;
        wLes = false;
        wAdapter = false;
        wMach = false;
        wVent = false;
        wLaunchFX = false;
        Complex = false;
        stage_ignition_time = 0;
        currentDelta = 0;
        perror = 0;
        yerror = 0;
        rerror = 0;
        int i;
        for (i = 0; i < 10; i++) {
                stage.at(i).Ignited = false;
                stage.at(i).reignitable = true;
                stage.at(i).batteries.wBatts = false;
                stage.at(i).waitforreignition = 0;
                stage.at(i).StageState = STAGE_SHUTDOWN;
                stage.at(i).DenyIgnition = false;
                stage.at(i).ParticlesPacked = false;
                stage.at(i).ParticlesPackedToEngine = 0;
                stage.at(i).defpitch = false;
                stage.at(i).defroll = false;
                stage.at(i).defyaw = false;
                payload[i] = PAYLOAD();
        }

        int ii;
        for (ii = 0; ii < 10; ii++) {
                booster.at(ii) = BOOSTER();
                booster.at(ii).Ignited = false;
                booster.at(ii).ParticlesPacked = false;
                booster.at(ii).ParticlesPackedToEngine = 0;
        }
        MET = 0;
        APstat = false;
        AJdisabled = false;
        rolldisabled = false;
        pitchdisabled = false;
        Gnc_running = 0;
        spinning = false;
        lvl = 1;
        for (int s = 0; s < 57; s++) {
                Ssound.GncStepSound[s] = -5;
        }
        PegDesPitch = 35 * RAD;
        PegPitchLimit = 35 * RAD;
        DeltaUpdate = 0;
        GT_InitPitch = 89.5 * RAD;
        UpdatePegTimer = 0;
        UpdateComplex = 0;
        wPeg = false;
        PegMajorCycleInterval = 0.1;
        runningPeg = false;
        //g0=9.80655;
        AttCtrl = TRUE;
        PitchCtrl = TRUE;
        YawCtrl = TRUE;
        RollCtrl = TRUE;
        TgtPitch = 0;
        //tgtapo=200000;
        //tgtperi=200000;
        eps = -9000000000000;
        failureProbability = -1000;
        timeOfFailure = -10000000;
        wFailures = false;
        failed = false;
        DMD = 0;
        killDMD = false;
        stepsloaded = false;

        for (int q = 0; q < 150; q++)
        {
                Gnc_step[q].GNC_Comand = CM_NOLINE;
        }

        updtlm = 0;
        tlmidx = 0;
        writetlmTimer = 0;
        tlmnlines = 0;
        wReftlm = false;
        for (int q = 0; q < TLMSECS; q++)
        {
                tlmAlt[q].x = 0;
                tlmSpeed[q].x = 0;
                tlmPitch[q].x = 0;
                tlmThrust[q].x = 0;
                tlmMass[q].x = 0;
                tlmVv[q].x = 0;
                tlmAcc[q].x = 0;

                tlmAlt[q].y = 0;
                tlmSpeed[q].y = 0;
                tlmPitch[q].y = 0;
                tlmThrust[q].y = 0;
                tlmMass[q].y = 0;
                tlmVv[q].y = 0;
                tlmAcc[q].y = 0;
        }

        updtboiloff = 0;
        for (int h = 0; h < 100; h++)
        {
                coeff[h] = 0;
        }
        avgcoeff = 0;
        MECO_Counter = 0;

        MECO_TEST = 0;
        TMeco = 600;
        nPsg = 0;
        particlesdt = 0;
        GrowingParticles = false;
        //ParticleFirstLoop=TRUE;
        RefPressure = 101400;
        //RampCreated=false;
        //AttachedToRamp=false;
        //RampDeleted=false;
        wRamp = false;
        NoMoreRamp = false;
        col_d.a = 0;
        col_d.b = 1;
        col_d.g = 0.8;
        col_d.r = 0.9;
        col_s.a = 0;
        col_s.b = 1;
        col_s.g = 0.8;
        col_s.r = 1.9;
        col_a.a = 0;
        col_a.b = 0;
        col_a.g = 0;
        col_a.r = 0;
        col_white.a = 0;
        col_white.b = 1;
        col_white.g = 1;
        col_white.r = 1;

        th_main_level = 0;
        launchFx_level = 0;
        hangaranims = _V(1.3, -10, 57.75);
        wCrawler = false;
        wCamera = false;
        AttToMSPad = false;
        MsPadZ = _V(0, 0, -50);
        return;
}

// ==============================================================
// Thruster and Particle Setup
// These read the config data and actually tell Orbiter "Put an
// engine here with this much power" and "Make smoke come out here."
// ==============================================================
PSTREAM_HANDLE Multistage2026::AddExhaustStreamGrowing(THRUSTER_HANDLE  th, const VECTOR3& pos, PARTICLESTREAMSPEC* pss, bool growing, double growfactor_size, double growfactor_rate, bool count, bool ToBooster, int N_Item, int N_Engine) {
    PSTREAM_HANDLE psh = AddExhaustStream(th, pos, pss);

    psg[nPsg].pss = *pss;
    psg[nPsg].psh[2] = psh;
    psg[nPsg].th = th;
    psg[nPsg].pos = pos;
    psg[nPsg].GrowFactor_rate = growfactor_rate;
    psg[nPsg].GrowFactor_size = growfactor_size;
    psg[nPsg].growing = growing;
    psg[nPsg].baserate = psg[nPsg].pss.growthrate;
    psg[nPsg].basesize = psg[nPsg].pss.srcsize;
    psg[nPsg].basepos = psg[nPsg].pos;
    psg[nPsg].ToBooster = ToBooster;
    psg[nPsg].nItem = N_Item;
    psg[nPsg].nEngine = N_Engine;
    if (count) nPsg++;
    return psh;
}

// ==============================================================
// Thruster Creation
// ==============================================================
void Multistage2026::CreateMainThruster()
{
    // Use stage.size() only if 'stage' is still a vector; 
    // if you switched to a fixed array, use 'nStages'.
    if (currentStage >= stage.size()) return;

    STAGE& S = stage[currentStage]; // Safe indexing
    char logbuff[512];

    // 1. Handle 0 Engines Edge Case
    if (S.nEngines == 0) {
        S.nEngines = 1;
        S.eng[0] = _V(0, 0, -S.height / 2.0);
    }

    // Default Pressure check
    if ((S.ispref >= 0) && (S.pref == 0.0)) {
        S.pref = 101400.0;
    }

    // 2. Create Thrusters
    if (Misc.thrustrealpos) {
        for (int i = 0; i < S.nEngines; i++) {
            S.th_main_h[i] = CreateThruster(S.eng[i], _V(0,0,1), S.thrust / S.nEngines, S.tank, S.isp, S.isp, S.pref);
        }
    } else {
        for (int i = 0; i < S.nEngines; i++) {
            S.th_main_h[i] = CreateThruster(_V(0, 0, 0), _V(0,0,1), S.thrust / S.nEngines, S.tank, S.isp, S.isp, S.pref);
        }
    }

    // 3. Create Group
    if (S.nEngines > 0) {
        // Arrays decay to pointers, so S.th_main_h is equivalent to .data()
        thg_h_main = CreateThrusterGroup(S.th_main_h, S.nEngines, THGROUP_MAIN);
    }

    if (S.tank) SetDefaultPropellantResource(S.tank);

    // 4. Add Exhausts
    for (int i = 0; i < S.nEngines; i++) {
        AddExhaust(S.th_main_h[i], 20.0, 1.0);

        if (!S.ParticlesPacked) {
            char nameBuffer[256];

            if (S.wps1) {
                strncpy(nameBuffer, S.eng_pstream1, 255);
                nameBuffer[255] = '\0'; 
                PARTICLE P1 = GetProperPS(nameBuffer); 
                AddExhaustStreamGrowing(S.th_main_h[i], _V(0,0,0), &P1.Pss, true, 8.0, 1.0, false, false, currentStage, i);
            }
            if (S.wps2) {
                strncpy(nameBuffer, S.eng_pstream2, 255);
                nameBuffer[255] = '\0'; 
                PARTICLE P2 = GetProperPS(nameBuffer);
                AddExhaustStreamGrowing(S.th_main_h[i], _V(0,0,0), &P2.Pss, true, 8.0, 1.0, false, false, currentStage, i);
            }
        }
    }
    // 5. Deny Ignition
    if (S.DenyIgnition) {
        for (int i = 0; i < S.nEngines; i++) {
            SetThrusterResource(S.th_main_h[i], NULL);
        }
    }

    // 6. Engine Light
    if (S.nEngines > 0) {
        COLOUR4 col_w = {1.0, 1.0, 1.0, 0.0};
        LightEmitter* le = AddPointLight(S.eng[0], 200, 1e-3, 0, 1e-3, col_w, col_w, col_w);

        th_main_level = 0.0;
        le->SetIntensityRef(&th_main_level);
    }
}
// ==============================================================
// Reaction Control System Creation - Stabilized Version
// ==============================================================
void Multistage2026::CreateRCS() {
    // Reference to the current stage to keep code clean
    STAGE &S = stage[currentStage];

    // 1. Calculate default thrusts if not provided
    if (S.pitchthrust == 0) S.pitchthrust = 0.25 * S.thrust * S.diameter; 
    if (S.yawthrust == 0)   S.yawthrust = 0.25 * S.thrust * S.diameter;
    if (S.rollthrust == 0)  S.rollthrust = 0.1 * S.thrust * S.diameter;

    // Use a local temporary array to register groups to avoid overwriting 
    // class members and causing Lua nil/crash errors
    THRUSTER_HANDLE hTemp[2];

    // --- Rotational RCS ---

    // Pitch Up
    hTemp[0] = CreateThruster(_V(0, 0, 1), _V(0, 1, 0), 2 * S.pitchthrust, S.tank, S.isp);
    hTemp[1] = CreateThruster(_V(0, 0, -1), _V(0, -1, 0), 2 * S.pitchthrust, S.tank, S.isp);
    CreateThrusterGroup(hTemp, 2, THGROUP_ATT_PITCHUP);
    MaxTorque.x = 2 * GetThrusterMax(hTemp[0]);

    // Pitch Down
    hTemp[0] = CreateThruster(_V(0, 0, 1), _V(0, -1, 0), 2 * S.pitchthrust, S.tank, S.isp);
    hTemp[1] = CreateThruster(_V(0, 0, -1), _V(0, 1, 0), 2 * S.pitchthrust, S.tank, S.isp);
    CreateThrusterGroup(hTemp, 2, THGROUP_ATT_PITCHDOWN);

    // Yaw Left
    hTemp[0] = CreateThruster(_V(0, 0, 1), _V(-1, 0, 0), 2 * S.yawthrust, S.tank, S.isp);
    hTemp[1] = CreateThruster(_V(0, 0, -1), _V(1, 0, 0), 2 * S.yawthrust, S.tank, S.isp);
    CreateThrusterGroup(hTemp, 2, THGROUP_ATT_YAWLEFT);
    MaxTorque.y = 2 * GetThrusterMax(hTemp[0]);

    // Yaw Right
    hTemp[0] = CreateThruster(_V(0, 0, 1), _V(1, 0, 0), 2 * S.yawthrust, S.tank, S.isp);
    hTemp[1] = CreateThruster(_V(0, 0, -1), _V(-1, 0, 0), 2 * S.yawthrust, S.tank, S.isp);
    CreateThrusterGroup(hTemp, 2, THGROUP_ATT_YAWRIGHT);

    // Roll Left
    hTemp[0] = CreateThruster(_V(1, 0, 0), _V(0, 1, 0), 2 * S.rollthrust, S.tank, S.isp);
    hTemp[1] = CreateThruster(_V(-1, 0, 0), _V(0, -1, 0), 2 * S.rollthrust, S.tank, S.isp);
    CreateThrusterGroup(hTemp, 2, THGROUP_ATT_BANKLEFT);
    MaxTorque.z = 2 * GetThrusterMax(hTemp[0]);

    // Roll Right
    hTemp[0] = CreateThruster(_V(1, 0, 0), _V(0, -1, 0), 2 * S.rollthrust, S.tank, S.isp);
    hTemp[1] = CreateThruster(_V(-1, 0, 0), _V(0, 1, 0), 2 * S.rollthrust, S.tank, S.isp);
    CreateThrusterGroup(hTemp, 2, THGROUP_ATT_BANKRIGHT);

    // --- Linear Translation Thrusters ---
    if (S.linearthrust > 0) {
        if (S.linearisp <= 0) S.linearisp = S.isp * 100; 

        // Forward/Back
        hTemp[0] = CreateThruster(_V(0, 0, 0), _V(0, 0, 1), S.linearthrust, S.tank, S.linearisp);
        hTemp[1] = CreateThruster(_V(0, 0, 0), _V(0, 0, 1), S.linearthrust, S.tank, S.linearisp);
        CreateThrusterGroup(hTemp, 2, THGROUP_ATT_FORWARD);

        hTemp[0] = CreateThruster(_V(0, 0, 0), _V(0, 0, -1), S.linearthrust, S.tank, S.linearisp);
        hTemp[1] = CreateThruster(_V(0, 0, 0), _V(0, 0, -1), S.linearthrust, S.tank, S.linearisp);
        CreateThrusterGroup(hTemp, 2, THGROUP_ATT_BACK);

        // Left/Right
        hTemp[0] = CreateThruster(_V(0, 0, 0), _V(-1, 0, 0), S.linearthrust, S.tank, S.linearisp);
        hTemp[1] = CreateThruster(_V(0, 0, 0), _V(-1, 0, 0), S.linearthrust, S.tank, S.linearisp);
        CreateThrusterGroup(hTemp, 2, THGROUP_ATT_LEFT);

        hTemp[0] = CreateThruster(_V(0, 0, 0), _V(1, 0, 0), S.linearthrust, S.tank, S.linearisp);
        hTemp[1] = CreateThruster(_V(0, 0, 0), _V(1, 0, 0), S.linearthrust, S.tank, S.linearisp);
        CreateThrusterGroup(hTemp, 2, THGROUP_ATT_RIGHT);

        // Up/Down
        hTemp[0] = CreateThruster(_V(0, 0, 0), _V(0, 1, 0), S.linearthrust, S.tank, S.linearisp);
        hTemp[1] = CreateThruster(_V(0, 0, 0), _V(0, 1, 0), S.linearthrust, S.tank, S.linearisp);
        CreateThrusterGroup(hTemp, 2, THGROUP_ATT_UP);

        hTemp[0] = CreateThruster(_V(0, 0, 0), _V(0, -1, 0), S.linearthrust, S.tank, S.linearisp);
        hTemp[1] = CreateThruster(_V(0, 0, 0), _V(0, -1, 0), S.linearthrust, S.tank, S.linearisp);
        CreateThrusterGroup(hTemp, 2, THGROUP_ATT_DOWN);
    }
}

// ==============================================================
// Helper Functions: Positions & Rotations
// Get Boosters Position given group number and booster number 
// inside the group
// ==============================================================
VECTOR3 Multistage2026::GetBoosterPos(int nb, int n) {
    VECTOR3 b_off = booster.at(nb).off; // Mesh attachment (-12.5, 0, -3)
    VECTOR3 e_off = booster.at(nb).eng[n]; // Engine local (0, 0, -27)
    double angle = booster.at(nb).angle * RAD;
    VECTOR3 final_pos;
    // X and Y handle rotation
    final_pos.x = b_off.x + (e_off.x * cos(angle) - e_off.y * sin(angle));
    final_pos.y = b_off.y + (e_off.x * sin(angle) + e_off.y * cos(angle));
    // Ensure Z is added! Currently, it's likely just returning b_off.z
    final_pos.z = b_off.z + e_off.z; 
    return final_pos;
}

char* Multistage2026::GetProperPayloadMeshName(int pnl, int n) {
    if (n == 0) return (char*)payload[pnl].meshname0.c_str();
    else if (n == 1) return (char*)payload[pnl].meshname1.c_str();
    else if (n == 2) return (char*)payload[pnl].meshname2.c_str();
    else if (n == 3) return (char*)payload[pnl].meshname3.c_str();
    else if (n == 4) return (char*)payload[pnl].meshname4.c_str();
    else return (char*)payload[pnl].meshname0.c_str();
}

void Multistage2026::RotatePayload(int pns, int nm, VECTOR3 anglesrad) {
    VECTOR3 state = _V(anglesrad.x / (2 * PI), anglesrad.y / (2 * PI), anglesrad.z / (2 * PI));
    VECTOR3 reference;

    if (nm == 0) reference = _V(0, 0, 0);
    else reference = operator-(payload[pns].off[nm], payload[pns].off[0]);

    if (!DeveloperMode) {
        payloadrotatex[pns][nm] = new MGROUP_ROTATE(payload[pns].msh_idh[nm], 0, 0, reference, _V(1, 0, 0), (float)2 * PI);
        RotatePayloadAnim_x[pns][nm] = CreateAnimation(0);
        anim_x[pns][nm] = AddAnimationComponent(RotatePayloadAnim_x[pns][nm], 0.0f, 1.0f, payloadrotatex[pns][nm]);

        payloadrotatey[pns][nm] = new MGROUP_ROTATE(payload[pns].msh_idh[nm], 0, 0, reference, _V(0, 1, 0), (float)2 * PI);
        RotatePayloadAnim_y[pns][nm] = CreateAnimation(0);
        anim_y[pns][nm] = AddAnimationComponent(RotatePayloadAnim_y[pns][nm], 0.0f, 1.0f, payloadrotatey[pns][nm]);

        payloadrotatez[pns][nm] = new MGROUP_ROTATE(payload[pns].msh_idh[nm], 0, 0, reference, _V(0, 0, 1), (float)2 * PI);
        RotatePayloadAnim_z[pns][nm] = CreateAnimation(0);
        anim_z[pns][nm] = AddAnimationComponent(RotatePayloadAnim_z[pns][nm], 0.0f, 1.0f, payloadrotatez[pns][nm]);
    } else {
        SetAnimation(RotatePayloadAnim_z[pns][nm], 0);
        SetAnimation(RotatePayloadAnim_y[pns][nm], 0);
        SetAnimation(RotatePayloadAnim_x[pns][nm], 0);
    }

    SetAnimation(RotatePayloadAnim_x[pns][nm], state.x);
    SetAnimation(RotatePayloadAnim_y[pns][nm], state.y);
    SetAnimation(RotatePayloadAnim_z[pns][nm], state.z);
}

VECTOR3 Multistage2026::RotateVector(const VECTOR3& input, double angle, const VECTOR3& rotationaxis)
{
    MATRIX3 rMatrix;
    double c = cos(angle);
    double s = sin(angle);
    double t = 1.0 - c;
    double x = rotationaxis.x;
    double y = rotationaxis.y;
    double z = rotationaxis.z;

    rMatrix.m11 = (t * x * x + c);
    rMatrix.m12 = (t * x * y - s * z);
    rMatrix.m13 = (t * x * z + s * y);
    rMatrix.m21 = (t * x * y + s * z);
    rMatrix.m22 = (t * y * y + c);
    rMatrix.m23 = (t * y * z - s * x);
    rMatrix.m31 = (t * x * z - s * y);
    rMatrix.m32 = (t * y * z + s * x);
    rMatrix.m33 = (t * z * z + c);

    return mul(rMatrix, input);
}

// ==============================================================
// Mesh Loading Logic
// ==============================================================
// This function loads the 3D models for every part of the rocket.
// It populates the mesh handles (msh_idh) that the MFD reads to display status.

void Multistage2026::LoadMeshes() {

    // Log the limits BEFORE the loop to see if nStages is garbage
    //oapiWriteLogV("MESHES: Start - currentStage: %d, nStages: %d, Vector Size: %ld",
    //        currentStage, nStages, stage.size());

    // ---------------------------------------------------------
    // 1. Load Main Stages
    // ---------------------------------------------------------
    for (int q = currentStage; q < nStages; q++) {

        VECTOR3 pos = stage[q].off;

        // FIX: Load directly into the stage struct, not a temporary variable
        stage[q].msh_h = oapiLoadMeshGlobal(stage[q].meshname);

        // Now AddMesh sees the valid handle we just loaded
        stage[q].msh_idh = AddMesh(stage[q].msh_h, &pos);

        // Log the final addition with coordinates
        //oapiWriteLogV("MESHES: %s Stage n.%i Mesh Added Mesh: %s @ x:%.3f y:%.3f z:%.3f", 
        //    GetName(), q + 1, stage[q].meshname, pos.x, pos.y, pos.z);

        // Load Interstage (if present for this stage)
        if (stage[q].wInter == TRUE) {
            VECTOR3 inspos = stage[q].interstage.off;
            // Note: The interstage logic below was already correct in your snippet
            stage[q].interstage.msh_h = oapiLoadMeshGlobal((char*)stage[q].interstage.meshname.c_str());
            //oapiWriteLogV("MESHES: s Interstage Mesh Preloaded for Stage %i", GetName(), q + 1);
            stage[q].interstage.msh_idh = AddMesh(stage[q].interstage.msh_h, &inspos);
            //oapiWriteLogV("MESHES: %s Interstage Mesh Added: %s @ x:%.3f y:%.3f z:%.3f", 
            //    GetName(), stage[q].interstage.meshname.c_str(), inspos.x, inspos.y, inspos.z);
        }
    }

    // ---------------------------------------------------------
    // 2. Load Payloads
    // ---------------------------------------------------------
    for (int pns = currentPayload; pns < nPayloads; pns++) {

        // Only load if it's a "Static" payload (mesh only), not a "Live" vessel attachment
        if (!payload[pns].live) {
            for (int nm = 0; nm < payload[pns].nMeshes; nm++) {

                VECTOR3 pos = payload[pns].off[nm];
                // Get proper name handles multi-part payloads
                payload[pns].msh_h[nm] = oapiLoadMeshGlobal(GetProperPayloadMeshName(pns, nm));
                //oapiWriteLogV("MESHES: %s Payload Mesh Preloaded %i", GetName(), pns + 1);
                payload[pns].msh_idh[nm] = AddMesh(payload[pns].msh_h[nm], &pos);
                //oapiWriteLogV("MESHES: %s: Payload n.%i Mesh Added: %s @ x:%.3f y:%.3f z:%.3f", GetName(), pns + 1, GetProperPayloadMeshName(pns, nm), pos.x, pos.y, pos.z);

                // Hide payload if inside fairing?
                if ((payload[pns].render == 0) && (wFairing == 1)) {
                    SetMeshVisibilityMode(payload[pns].msh_idh[nm], MESHVIS_NEVER);
                }

                // Apply Initial Rotation if defined
                RotatePayload(pns, nm, payload[pns].Rotation);
            }
        }

        // Setup Attachment Point (Used for re-attaching or referencing later)
        VECTOR3 direction, normal;
        if (!payload[pns].rotated) {
            direction = _V(0, 0, 1);
            normal = _V(0, 1, 0);
        } else {
            // Complex rotation logic for attachment points
            direction = payload[pns].Rotation;
            VECTOR3 rotation = payload[pns].Rotation;
            direction = mul(RotationMatrix(rotation), _V(0, 0, 1));
            normal = mul(RotationMatrix(rotation), _V(0, 1, 0));
            normalise(normal);
            normalise(direction);
        }

        live_a[pns] = CreateAttachment(false, payload[pns].off[0], direction, normal, (char*)"MS2015", false);
    }

    // ---------------------------------------------------------
    // 3. Load Boosters
    // ---------------------------------------------------------
    for (int qb = currentBooster; qb < nBoosters; qb++) {
        VECTOR3 bpos = booster.at(qb).off;
        VECTOR3 bposxy = bpos;
        bposxy.z = 0;
        double bro = length(bposxy); // Distance from center

        // Loop through N boosters in this group
        for (int NN = 1; NN < booster.at(qb).N + 1; NN++) {
            char boosbuff[32], boosmhname[MAXLEN];

            // Construct filename with suffix (e.g., Booster_1, Booster_2)
            snprintf(boosbuff, sizeof(boosbuff), "_%i", NN);
            strcpy(boosmhname, booster.at(qb).meshname.c_str());
            strcat(boosmhname, boosbuff);

            // Calculate radial position
            double arg = booster.at(qb).angle * RAD + (NN - 1) * 2 * PI / booster.at(qb).N;
            VECTOR3 bposdef = _V(bpos.x * cos(arg) - bpos.y * sin(arg), bpos.x * sin(arg) + bpos.y * cos(arg), bpos.z);

            booster.at(qb).msh_h[NN] = oapiLoadMeshGlobal(boosmhname);
            //oapiWriteLogV("MESHES: %s Booster Mesh Preloaded: %s", GetName(), boosmhname);
            booster.at(qb).msh_idh[NN] = AddMesh(booster.at(qb).msh_h[NN], &bposdef);
            //oapiWriteLogV("MESHES: %s Booster Mesh Added Mesh: %s @ x:%.3f y:%.3f z:%.3f", GetName(), boosmhname, bposdef.x, bposdef.y, bposdef.z);
        }
    }

    // ---------------------------------------------------------
    // 4. Load Fairings
    // ---------------------------------------------------------
    if (wFairing == 1) {
        VECTOR3 fpos = fairing.off;
        VECTOR3 fposxy = fpos;
        fposxy.z = 0;
        double fro = length(fposxy);

        for (int NF = 1; NF < fairing.N + 1; NF++) {
            char fairbuff[32], fairmshname[MAXLEN];
            snprintf(fairbuff, sizeof(fairbuff), "_%i", NF);
            strcpy(fairmshname, fairing.meshname.c_str());
            strcat(fairmshname, fairbuff);

            VECTOR3 fposdef = _V(fro * cos(fairing.angle * RAD + (NF - 1) * 2 * PI / fairing.N), fro * sin(fairing.angle * RAD + (NF - 1) * 2 * PI / fairing.N), fpos.z);

            fairing.msh_h[NF] = oapiLoadMeshGlobal(fairmshname);
            //oapiWriteLogV("MESHES: %s Fairing Mesh Preloaded: %s", GetName(), fairmshname);
            fairing.msh_idh[NF] = AddMesh(fairing.msh_h[NF], &fposdef);
            //oapiWriteLogV("MESHES: %s: Fairing Mesh Added Mesh: %s @ x:%.3f y:%.3f z:%.3f", GetName(), fairmshname, fposdef.x, fposdef.y, fposdef.z);
        }
    }

    // ---------------------------------------------------------
    // 5. Load Adapter (Optional)
    // ---------------------------------------------------------
    if (wAdapter == TRUE) {
        VECTOR3 adappos = Adapter.off;
        Adapter.msh_h = oapiLoadMeshGlobal((char*)Adapter.meshname.c_str());
        //oapiWriteLogV("MESHES: %s Adapter Mesh Preloaded", GetName());
        Adapter.msh_idh = AddMesh(Adapter.msh_h, &adappos);
        //oapiWriteLogV("MESHES: %s Adapter Mesh Added: %s @ x:%.3f y:%.3f z:%.3f", GetName(), Adapter.meshname.c_str(), adappos.x, adappos.y, adappos.z);
    }

    // ---------------------------------------------------------
    // 6. Load Launch Escape System (Optional)
    // ---------------------------------------------------------
    if (wLes == TRUE) {
        VECTOR3 LesPos = Les.off;
        Les.msh_h = oapiLoadMeshGlobal((char*)Les.meshname.c_str());
        //oapiWriteLogV("MESHES %s Les Mesh Preloaded", GetName());
        Les.msh_idh = AddMesh(Les.msh_h, &LesPos);
        //oapiWriteLogV("MESHES: %s Les Mesh Added %s @ x:%.3f y:%.3f z:%.3f", GetName(), Les.meshname.c_str(), LesPos.x, LesPos.y, LesPos.z);
    }
}

// ==============================================================
// Physics & Mass Management
// ==============================================================

// Inertia Calculations
void Multistage2026::UpdatePMI() {
    double TotalVolume = 0;
    TotalHeight = 0;
    for (int i = currentStage; i < nStages; i++) {
        TotalHeight += stage.at(i).height;
        stage.at(i).volume = stage.at(i).height * 0.25 * PI * stage.at(i).diameter * stage.at(i).diameter;
        TotalVolume += stage.at(i).volume;
        stage.at(i).interstage.volume = stage.at(i).interstage.height * 0.25 * PI * stage.at(i).interstage.diameter * stage.at(i).interstage.diameter;
        TotalVolume += stage.at(i).interstage.volume;
    }

    double CSBoosters = 0;
    for (int q = currentBooster; q < nBoosters; q++) {
        booster.at(q).volume = booster.at(q).N * booster.at(q).height * 0.25 * PI * booster.at(q).diameter * booster.at(q).diameter;
        CSBoosters += 0.25 * PI * booster.at(q).diameter * booster.at(q).diameter;
    }

    for (int k = currentPayload; k < nPayloads; k++) {
        payload[k].volume = payload[k].height * 0.25 * PI * payload[k].diameter * payload[k].diameter;
        TotalVolume += payload[k].volume;
    }

    if (wFairing == 1) {
        fairing.volume = fairing.height * 0.25 * PI * fairing.diameter * fairing.diameter;
        TotalVolume += fairing.volume;
    }
    if (wLes == TRUE) {
        Les.volume = Les.height * 0.25 * PI * Les.diameter * Les.diameter;
        TotalVolume += Les.volume;
    }

    double PhiEq = sqrt(4 * TotalVolume / (PI * TotalHeight));
    double CSX = TotalHeight * PhiEq;
    double CSY = CSX;
    double CSZ = PI * 0.25 * PhiEq * PhiEq + CSBoosters;
    SetCrossSections(_V(CSX, CSY, CSZ));

    double IZ = (PhiEq * 0.5) * (PhiEq * 0.5) * 0.5;
    double IX = (3 * (PhiEq * 0.5) * (PhiEq * 0.5) + TotalHeight * TotalHeight) / 12;
    double IY = IX;
    SetPMI(_V(IX, IY, IZ));

    if (Configuration == 0) {
        SetSize(stage[0].height * 0.5 + Misc.COG);
    } else {
        SetSize(TotalHeight);
    }
}

// Fuel and stage mass summing
void Multistage2026::UpdateMass() {
    double EM = stage.at(currentStage).emptymass;

    for (int s = currentStage + 1; s < nStages; s++) {
        EM += stage[s].emptymass;
        if (stage[s].wInter == TRUE) {
            EM += stage[s].interstage.emptymass;
        }
    }

    for (int bs = currentBooster; bs < nBoosters; bs++) {
        EM += (booster.at(bs).emptymass * booster.at(bs).N);
    }

    for (int pns = currentPayload; pns < nPayloads; pns++) {
        EM += payload[pns].mass;
    }
    if (wFairing == 1) { EM += 2 * fairing.emptymass; }
    if (wAdapter == TRUE) { EM += Adapter.emptymass; }
    if (wLes == TRUE) { EM += Les.emptymass; }
    SetEmptyMass(EM);
}

//  Hndling attached vessels
void Multistage2026::UpdateLivePayloads() {
    for (int pns = currentPayload; pns < nPayloads; pns++) {
        if (payload[pns].live) {
            VESSELSTATUS2 vslive;
            memset(&vslive, 0, sizeof(vslive));
            vslive.version = 2;
            OBJHANDLE checklive = oapiGetVesselByName((char*)payload[pns].name.c_str()); // Cast for Linux

            if (oapiIsVessel(checklive)) {
                ATTACHMENTHANDLE liveatt;
                VESSEL3* livepl = (VESSEL3*)oapiGetVesselInterface(checklive);
                liveatt = livepl->CreateAttachment(TRUE, _V(0, 0, 0), _V(0, 0, -1), _V(0, 1, 0), (char*)"MS2015", false);
                AttachChild(checklive, live_a[pns], liveatt);
                if (payload[pns].mass <= 0) payload[pns].mass = livepl->GetMass();
                if (payload[pns].height <= 0) {
                    payload[pns].height = livepl->GetSize();
                    payload[pns].diameter = payload[pns].height * 0.1;
                }
            } else {
                VESSEL3* v;
                OBJHANDLE hObj;
                ATTACHMENTHANDLE liveatt;
                GetStatusEx(&vslive);
                hObj = oapiCreateVesselEx(payload[pns].name.c_str(), payload[pns].module.c_str(), &vslive);

                if (oapiIsVessel(hObj)) {
                    v = (VESSEL3*)oapiGetVesselInterface(hObj);
                    liveatt = v->CreateAttachment(TRUE, _V(0, 0, 0), _V(0, 0, -1), _V(0, 1, 0), (char*)"MS2015", false);
                    AttachChild(hObj, live_a[pns], liveatt);
                    if (payload[pns].mass <= 0) payload[pns].mass = v->GetMass();
                    if (payload[pns].height <= 0) {
                        payload[pns].height = v->GetSize();
                        payload[pns].diameter = payload[pns].height * 0.1;
                    }
                }
            }
        }
    }
    UpdateMass();
    UpdatePMI();
}

// ==============================================================
// Lookups & Systems
// ==============================================================

//  Returns the particlestream specification to use or the empty one if not found
PARTICLE Multistage2026::GetProperPS(char name[MAXLEN]) {
    // 1. Create a local copy to prevent buffer overruns and const-cast violations
    char local_name[MAXLEN];
    strncpy(local_name, name, MAXLEN - 1);
    local_name[MAXLEN - 1] = '\0';
    // 2. Safely lowercase ONLY the actual length of the target string
    size_t len = strlen(local_name);
    for (size_t z = 0; z < len; z++) {
        local_name[z] = tolower(local_name[z]);
    }
    char checktxt[MAXLEN];
    for (int nt = 0; nt < 16; nt++) {
        // 3. Safely copy the reference name
        strncpy(checktxt, Particle[nt].ParticleName, MAXLEN - 1);
        checktxt[MAXLEN - 1] = '\0';
        size_t check_len = strlen(checktxt);
        for (size_t z = 0; z < check_len; z++) {
            checktxt[z] = tolower(checktxt[z]);
        }
        // 4. Compare safely
        if (len > 0 && strncmp(local_name, checktxt, MAXLEN - 5) == 0) {
            return Particle[nt];
        }
    }
    // Fallback if not found
    return Particle[15];
}

//  Returns the texture to be used or the empty one
SURFHANDLE Multistage2026::GetProperExhaustTexture(char name[MAXLEN]) {
    char checktxt[MAXLEN];
    for (int nt = 0; nt < nTextures; nt++) {
        for (int k = 0; k < MAXLEN; k++) checktxt[k] = tex.TextureName[k][nt];
        if (strncmp(name, checktxt, MAXLEN - 5) == 0) return tex.hTex[nt];
    }
    return NULL;
}


// Separation Logic: creates Ullage engine and exhausts
void Multistage2026::CreateUllageAndBolts() {
    // 1. Create a reference to the current stage for clarity and safety
    STAGE &S = stage[currentStage]; 

    // 2. Handle Ullage Motor Creation
    if (S.ullage.wUllage) {
        // Fix: Use the new 'ignited' boolean member [cite: 2, 15]
        S.ullage.ignited = false; 

        // Fix: Use the new 'th_ullage' handle member [cite: 3, 16]
        // Note: Creating the thruster with its specific thrust, tank, and ISP
        S.ullage.th_ullage = CreateThruster(_V(0, 0, 0), _V(0, 0, 1), S.ullage.thrust, S.tank, 100000);

        for (int i = 0; i < S.ullage.N; i++) {
            double ull_angle;
            // Angle calculation logic (preserved from original)
            if (IsOdd((int)S.ullage.N)) {
                if (i < (int)(S.ullage.N * 0.5)) {
                    ull_angle = S.ullage.angle * RAD + (i) * ((2 * PI / S.ullage.N));
                } else if (i == (int)(S.ullage.N * 0.5)) {
                    ull_angle = PI + S.ullage.angle * RAD;
                } else {
                    ull_angle = S.ullage.angle * RAD + PI + (i - (int)(S.ullage.N * 0.5)) * (2 * PI / S.ullage.N);
                }
            } else {
                ull_angle = S.ullage.angle * RAD + (i) * (2 * PI / S.ullage.N);
            }

            VECTOR3 ulldir = RotateVecZ(S.ullage.dir, ull_angle);
            VECTOR3 ullpos = RotateVecZ(S.ullage.pos, ull_angle);

            // Fix: S.ullage.tex is now a char[256], so we remove .data() [cite: 7]
            AddExhaust(S.ullage.th_ullage, S.ullage.length, S.ullage.diameter, ullpos, ulldir, GetProperExhaustTexture(S.ullage.tex));
        }
    }

    // 3. Handle Explosive Separation Bolts
    if (S.expbolt.wExpbolt) {
        // Fix: S.expbolt.pstream is now a char[256], so we remove .data() [cite: 8]
        PARTICLESTREAMSPEC Pss3 = GetProperPS(S.expbolt.pstream).Pss;

        // Fix: Use the new 'threxp_h' handle member [cite: 9, 22]
        S.expbolt.threxp_h = CreateThruster(S.expbolt.pos, S.expbolt.dir, 0, S.tank, 100000, 100000);
        
        AddExhaustStream(S.expbolt.threxp_h, &Pss3);
    }
}

// ==============================================================
// Vehicle Setup
// ==============================================================
void Multistage2026::VehicleSetup() {

    oapiWriteLogV("SETUP: Misc.thrustrealpos is %s at VehicleSetup", Misc.thrustrealpos ? "TRUE" : "FALSE");

    // 1. Aerodynamics (Standard Vinka values)
    SetRotDrag(_V(0.7, 0.7, 0.06));

    // 2. Initialize Propellant
    // Boosters
    for (int bk = currentBooster; bk < nBoosters; bk++) {
        booster.at(bk).tank = CreatePropellantResource(booster.at(bk).fuelmass * booster.at(bk).N);
        oapiWriteLogV("SETUP: %s Booster Group %i Tank Added: %.3f kg", GetName(), bk + 1, booster.at(bk).fuelmass * booster.at(bk).N);
    }
    // Stages
    for (int k = nStages - 1; k > currentStage - 1; k--) {
        stage[k].tank = CreatePropellantResource(stage[k].fuelmass);
        oapiWriteLogV("SETUP: %s Stage %i Tank Added: %.3f kg", GetName(), k + 1, stage[k].fuelmass);
    }

    // 3. Calculate ISP
    for (int r = currentStage; r < nStages; r++) {
        if (stage[r].fuelmass > 0)
            stage[r].isp = stage[r].thrust * stage[r].burntime / stage[r].fuelmass;
    }
    for (int br = currentBooster; br < nBoosters; br++) {
        if (booster.at(br).fuelmass > 0)
            booster.at(br).isp = (booster.at(br).thrust * booster.at(br).N) * booster.at(br).burntime / (booster.at(br).fuelmass * booster.at(br).N);
    }

    // 4. Initialize Particle System Array
    nPsh = 0;
    for (int pp = 0; pp < nStages; pp++) nPsh += stage[pp].nEngines;
    for (int pb = 0; pb < nBoosters; pb++) nPsh += (booster.at(pb).N * booster.at(pb).nEngines);
    nPsh *= 2; 

    psg.clear();
    psg.resize(nPsh);
    for (int ps = 0; ps < nPsh; ps++) {
        psg[ps] = PSGROWING();
        psg[ps].pss = Particle[15].Pss;
    }

    // 5. Create Core Thrusters
    CreateMainThruster();
    CreateRCS();

   // A. CORE STAGE DUMP
   for (int i = 0; i < stage[currentStage].nEngines; i++) {
       VECTOR3 corePos = stage[currentStage].eng[i];
       oapiWriteLogV("SETUP: Core Stage %d | Engine %d | Local: X=%.2f Y=%.2f Z=%.2f", 
                  currentStage + 1, i + 1, corePos.x, corePos.y, corePos.z);
   }

   // B. BOOSTER GROUP DUMP
   for (int bi = 0; bi < nBoosters; bi++) {
       oapiWriteLogV("SETUP: Booster %d Tank Handle CREATED at: %p", bi, booster.at(bi).tank);
       oapiWriteLogV("SETUP: Booster Group %d | Total N: %d | Mesh Off: X=%.2f Y=%.2f Z=%.2f", 
                  bi + 1, booster.at(bi).N, booster.at(bi).off.x, booster.at(bi).off.y, booster.at(bi).off.z);
       for (int bn = 0; bn < booster.at(bi).nEngines; bn++) {
            VECTOR3 bLocalPos = booster.at(bi).eng[bn];
            // Calculate what the code THINKS is the global position
            VECTOR3 bGlobalPos = GetBoosterPos(bi, bn);
            oapiWriteLogV("  -> Eng %d | Local: X=%.2f Y=%.2f Z=%.2f | Global Target: X=%.2f Y=%.2f Z=%.2f", 
                      bn + 1, bLocalPos.x, bLocalPos.y, bLocalPos.z, bGlobalPos.x, bGlobalPos.y, bGlobalPos.z);
       }
    }

    // 6. MASTER HANDLE CONSOLIDATION (Unified THGROUP_MAIN)
    THRUSTER_HANDLE hAllMain[32]; 
    int totalMainCount = 0;

    // Add Core Stage handles
    for (int i = 0; i < stage[currentStage].nEngines; i++) {
        if (stage[currentStage].th_main_h[i] != NULL) {
            hAllMain[totalMainCount++] = stage[currentStage].th_main_h[i];
        }
    }

    for (int bi = currentBooster; bi < nBoosters; bi++) {
        // A. Create Booster Thruster Handles
        for (int bn = 0; bn < booster.at(bi).N; bn++) {
            if (bn >= 4) break; 
            VECTOR3 pos = (Misc.thrustrealpos) ? GetBoosterPos(bi, bn) : _V(0,0,0);
            VECTOR3 dir = (Misc.thrustrealpos) ? booster.at(bi).eng_dir : _V(0,0,1);

            booster.at(bi).th_booster_h.at(bn) = CreateThruster(pos, dir, booster.at(bi).thrust, booster.at(bi).tank, booster.at(bi).isp);
            if (booster.at(bi).th_booster_h.at(bn) != NULL) {
                hAllMain[totalMainCount++] = booster.at(bi).th_booster_h.at(bn);
            }
        } // <-- FIX: Engine loop closes HERE now!

        // Initialize the individual curve handle safely outside the engine loop
        if (booster.at(bi).N > 0) {
            booster.at(bi).Thg_boosters_h = CreateThrusterGroup(booster.at(bi).th_booster_h.data(), booster.at(bi).N, THGROUP_USER);
        }

        // B. Hardened Visuals (Prevents GetProperPS crash)
        for (int bii = 0; bii < booster.at(bi).N; bii++) {
            if (bii >= 4) break;

            THRUSTER_HANDLE hB = booster.at(bi).th_booster_h.at(bii);
            if (hB != NULL) {
                AddExhaust(hB, 10 * booster.at(bi).eng_diameter, 1.0);

                // Ensure pstream name is valid before calling GetProperPS
                if (booster.at(bi).wps1 && !booster.at(bi).eng_pstream1.empty()) {
                    PARTICLE P1 = GetProperPS((char*)booster.at(bi).eng_pstream1.c_str());
                    VECTOR3 nozzlePos = booster.at(bi).eng[bii];
                    AddExhaustStreamGrowing(hB, nozzlePos, &P1.Pss, true, 8.0, 1.0, false, true, bi, bii);
                }
            }
        }
    }
    // 7. Register UNIFIED group
    if (totalMainCount > 0) {
        thg_h_main = CreateThrusterGroup(hAllMain, totalMainCount, THGROUP_MAIN);
    }

    // 8. Final Systems
    CreateUllageAndBolts();
    SetCW(0.2 * Misc.drag_factor, 0.5, 1.5, 1.5);

    // 9. Touchdown Points
    if (Misc.has_custom_td) {
        TOUCHDOWNVTX td[3];
        for (int i = 0; i < 3; i++) {
            td[i].pos = Misc.td_points[i];
            td[i].stiffness = 1e7; td[i].damping = 1e6; td[i].mu = 3.0; td[i].mu_lng = 3.0;
        }
        SetTouchdownPoints(td, 3);
    }

    if (currentBooster < nBoosters) wBoosters = true;
    UpdateMass();
    UpdatePMI();

    // ==========================================
    // THE LUA DATA BUS (Phantom Registers)
    // ==========================================
    // Positioned at Y=999.0 so Lua can find them. No fuel, no physical thrust.
    hBus_AP     = CreateThruster(_V(1.0, 999.0, 0.0), _V(0,1,0), 0.0, NULL, 0.0);
    hBus_Config = CreateThruster(_V(2.0, 999.0, 0.0), _V(0,1,0), 0.0, NULL, 0.0);
    hBus_Prog   = CreateThruster(_V(3.0, 999.0, 0.0), _V(0,1,0), 0.0, NULL, 0.0);

    bVesselInitialized = true;

    oapiWriteLog((char*)"SETUP: VehicleSetup Complete. Systems Online.");
}

// ==============================================================
// Separation Logic (Spawning Debris/Vessels)
// ==============================================================

void Multistage2026::Spawn(int type, int current) {

    char mn[MAXLEN];
    char flat_name[256];
    VESSELSTATUS2 vs;
    memset(&vs, 0, sizeof(vs));
    vs.version = 2;
    GetStatusEx(&vs);

    VECTOR3 ofs, rofs;
    VECTOR3 rvel = { vs.rvel.x, vs.rvel.y, vs.rvel.z };
    VECTOR3 vrot = { vs.vrot.x, vs.vrot.y, vs.vrot.z };
    // VECTOR3 arot = { vs.arot.x, vs.arot.y, vs.arot.z }; // Unused in original, kept for ref
    VECTOR3 vel;

    switch (type) {

    case TBOOSTER:
        for (int i = 1; i < booster.at(current).N + 1; i++) {
            GetMeshOffset(booster.at(current).msh_idh[i], ofs);

            // Calculate separation velocity relative to rotation
            vel = RotateVecZ(booster.at(current).speed, booster.at(current).angle * RAD + (i - 1) * 2 * PI / booster.at(current).N);

            Local2Rel(ofs, vs.rpos);
            GlobalRot(vel, rofs);

            vs.rvel.x = rvel.x + rofs.x;
            vs.rvel.y = rvel.y + rofs.y;
            vs.rvel.z = rvel.z + rofs.z;

            double arg = booster.at(current).angle * RAD + (i - 1) * 2 * PI / booster.at(current).N;
            vs.vrot.x = vrot.x + booster.at(current).rot_speed.x * cos(arg) - booster.at(current).rot_speed.y * sin(arg);
            vs.vrot.y = vrot.y + booster.at(current).rot_speed.x * sin(arg) + booster.at(current).rot_speed.y * cos(arg);
            vs.vrot.z = vrot.z + booster.at(current).rot_speed.z;

            char mn2[32];
            snprintf(mn2, sizeof(mn2), "_%i", i);
            strcpy(mn, booster.at(current).meshname.c_str());
            strcat(mn, mn2);

            // Spawn using the pre-generated config file
            strncpy(flat_name, mn, 255);
            flat_name[255] = '\0';
            for (int k = 0; flat_name[k] != '\0'; k++) {
                if (flat_name[k] == '/' || flat_name[k] == '\\') flat_name[k] = '_';
            }
            oapiCreateVesselEx(mn, flat_name, &vs);

            oapiWriteLogV("%s: Booster n.%i jettisoned name: %s @%.3f", GetName(), current + 1, mn, MET);
        }
        break;

    case TSTAGE:
        // 1. Use a reference to avoid calling .at() repeatedly
        { STAGE &S = stage[current];

        GetMeshOffset(S.msh_idh, ofs);
        // 2. Direct vector assignment is cleaner
        vel = S.speed;

        Local2Rel(ofs, vs.rpos);
        GlobalRot(vel, rofs);

        // 3. Coordinate math remains the same
        vs.rvel = rvel + rofs;
        vs.vrot = vrot + S.rot_speed;

        // 4. FIX: meshname is a char array
        strncpy(mn, S.meshname, 255);
        mn[255] = '\0'; // Safety termination

        // --- FLATTEN NAME AND SPAWN ---
        strncpy(flat_name, mn, 255);
        flat_name[255] = '\0';
        for (int k = 0; flat_name[k] != '\0'; k++) {
            if (flat_name[k] == '/' || flat_name[k] == '\\') flat_name[k] = '_';
        }

        // 5. FIX: Call oapiCreateVesselEx with the flattened config name
        oapiCreateVesselEx(mn, flat_name, &vs);
        // ------------------------------

        oapiWriteLogV("%s: Stage n.%i jettisoned name: %s @%.3f", GetName(), current + 1, mn, MET);

        stage_ignition_time = MET;
        break;}

    case TPAYLOAD:
        // Case 1: Static Payload (Mesh inside the rocket)
        if (!payload[current].live) {
            GetMeshOffset(payload[current].msh_idh[0], ofs);
            vel = _V(payload[current].speed.x, payload[current].speed.y, payload[current].speed.z);

            Local2Rel(ofs, vs.rpos);
            GlobalRot(vel, rofs);

            vs.rvel.x = rvel.x + rofs.x;
            vs.rvel.y = rvel.y + rofs.y;
            vs.rvel.z = rvel.z + rofs.z;
            vs.vrot.x = vrot.x + payload[current].rot_speed.x;
            vs.vrot.y = vrot.y + payload[current].rot_speed.y;
            vs.vrot.z = vrot.z + payload[current].rot_speed.z;

            if (payload[current].rotated) {
                MATRIX3 RotMatrixVal, RotMatrix_Def;
                GetRotationMatrix(RotMatrixVal);
                VECTOR3 rotation = payload[current].Rotation;
                RotMatrix_Def = mul(RotMatrixVal, RotationMatrix(rotation));
                vs.arot.x = atan2(RotMatrix_Def.m23, RotMatrix_Def.m33);
                vs.arot.y = -asin(RotMatrix_Def.m13);
                vs.arot.z = atan2(RotMatrix_Def.m12, RotMatrix_Def.m11);
            }
            OBJHANDLE hpl = oapiCreateVesselEx(payload[current].name.c_str(), payload[current].module.c_str(), &vs);

            if (currentPayload + 1 == Misc.Focus) {
                oapiSetFocusObject(hpl);
            }
        }
        // Case 2: Live Payload (Already a vessel attached to us)
        else {
            if (GetAttachmentStatus(live_a[current])) {
                OBJHANDLE live = GetAttachmentStatus(live_a[current]);
                VESSEL3* v = (VESSEL3*)oapiGetVesselInterface(live);

                VECTOR3 dir, rot;
                GetAttachmentParams(live_a[current], ofs, dir, rot);
                DetachChild(live_a[current], 0);

                vel = _V(payload[current].speed.x, payload[current].speed.y, payload[current].speed.z);
                Local2Rel(ofs, vs.rpos);
                GlobalRot(vel, rofs);

                vs.rvel.x = rvel.x + rofs.x;
                vs.rvel.y = rvel.y + rofs.y;
                vs.rvel.z = rvel.z + rofs.z;
                vs.vrot.x = vrot.x + payload[current].rot_speed.x;
                vs.vrot.y = vrot.y + payload[current].rot_speed.y;
                vs.vrot.z = vrot.z + payload[current].rot_speed.z;

                if (payload[current].rotated) {
                    MATRIX3 RotMatrixVal, RotMatrix_Def;
                    GetRotationMatrix(RotMatrixVal);
                    VECTOR3 rotation = payload[current].Rotation;
                    RotMatrix_Def = mul(RotMatrixVal, RotationMatrix(rotation));
                    vs.arot.x = atan2(RotMatrix_Def.m23, RotMatrix_Def.m33);
                    vs.arot.y = -asin(RotMatrix_Def.m13);
                    vs.arot.z = atan2(RotMatrix_Def.m12, RotMatrix_Def.m11);
                }
                v->clbkSetStateEx(&vs);

                if (currentPayload + 1 == Misc.Focus) {
                    oapiSetFocusObject(live);
                }
            }
        }
        oapiWriteLogV("%s: Payload n.%i jettisoned name: %s @%.3f", GetName(), current + 1, payload[current].name.c_str(), MET);
        break;

    case TFAIRING:
        for (int ii = 1; ii < fairing.N + 1; ii++) {
            GetMeshOffset(fairing.msh_idh[ii], ofs);
            vel = RotateVecZ(fairing.speed, fairing.angle * RAD + (ii - 1) * 2 * PI / fairing.N);

            Local2Rel(ofs, vs.rpos);
            GlobalRot(vel, rofs);

            vs.rvel.x = rvel.x + rofs.x;
            vs.rvel.y = rvel.y + rofs.y;
            vs.rvel.z = rvel.z + rofs.z;

            double arg = (ii - 1) * 2 * PI / fairing.N;
            vs.vrot.x = vrot.x + fairing.rot_speed.x * cos(arg) - fairing.rot_speed.y * sin(arg);
            vs.vrot.y = vrot.y + fairing.rot_speed.x * sin(arg) + fairing.rot_speed.y * cos(arg);
            vs.vrot.z = vrot.z + fairing.rot_speed.z;

            char mn2[32];
            snprintf(mn2, sizeof(mn2), "_%i", ii);
            strcpy(mn, fairing.meshname.c_str());
            strcat(mn, mn2);

            // Spawn using the pre-generated config file
            strncpy(flat_name, mn, 255);
            flat_name[255] = '\0';
            for (int k = 0; flat_name[k] != '\0'; k++) {
                if (flat_name[k] == '/' || flat_name[k] == '\\') flat_name[k] = '_';
            }
            oapiCreateVesselEx(mn, flat_name, &vs);

            oapiWriteLogV("%s: Fairing jettisoned: name %s @%.3f", GetName(), mn, MET);
        }
        break;

    case TINTERSTAGE:
        GetMeshOffset(stage.at(current).interstage.msh_idh, ofs);
        vel = _V(stage.at(current).interstage.speed.x, stage.at(current).interstage.speed.y, stage.at(current).interstage.speed.z);

        Local2Rel(ofs, vs.rpos);
        GlobalRot(vel, rofs);

        vs.rvel.x = rvel.x + rofs.x;
        vs.rvel.y = rvel.y + rofs.y;
        vs.rvel.z = rvel.z + rofs.z;
        vs.vrot.x = vrot.x + stage.at(current).interstage.rot_speed.x;
        vs.vrot.y = vrot.y + stage.at(current).interstage.rot_speed.y;
        vs.vrot.z = vrot.z + stage.at(current).interstage.rot_speed.z;

        strcpy(mn, stage.at(current).interstage.meshname.c_str());

        // Spawn using the pre-generated config file
        strncpy(flat_name, mn, 255);
        flat_name[255] = '\0';
        for (int k = 0; flat_name[k] != '\0'; k++) {
            if (flat_name[k] == '/' || flat_name[k] == '\\') flat_name[k] = '_';
        }

        // Spawn using the pre-generated config file
        oapiCreateVesselEx(mn, flat_name, &vs);

        oapiWriteLogV("%s: Interstage of stage %i jettisoned name: %s @%.3f", GetName(), current + 1, mn, MET);
        break;

    case TLES:
        GetMeshOffset(Les.msh_idh, ofs);
        vel = _V(Les.speed.x, Les.speed.y, Les.speed.z);

        Local2Rel(ofs, vs.rpos);
        GlobalRot(vel, rofs);

        vs.rvel.x = rvel.x + rofs.x;
        vs.rvel.y = rvel.y + rofs.y;
        vs.rvel.z = rvel.z + rofs.z;
        vs.vrot.x = vrot.x + Les.rot_speed.x;
        vs.vrot.y = vrot.y + Les.rot_speed.y;
        vs.vrot.z = vrot.z + Les.rot_speed.z;

        strcpy(mn, Les.meshname.c_str());

        // Spawn using the pre-generated config file
        strncpy(flat_name, mn, 255);
        flat_name[255] = '\0';
        for (int k = 0; flat_name[k] != '\0'; k++) {
            if (flat_name[k] == '/' || flat_name[k] == '\\') flat_name[k] = '_';
        }

        oapiCreateVesselEx(mn, flat_name, &vs);
        oapiWriteLogV("%s: Les jettisoned name: %s @%.3f", GetName(), mn, MET);
        break;
    }
}

// ==============================================================
// Jettison Logic (Cleaning up the old vessel)
// ==============================================================

void Multistage2026::Jettison(int type, int current) {
    switch (type) {
    case TBOOSTER:
        oapiWriteLogV("DEBUG JETTISON: Initiating TBOOSTER %d separation.", current);
        Spawn(type, current);
        oapiWriteLogV("DEBUG JETTISON: Spawn() completed successfully.");

        // 1. Delete the Meshes
        oapiWriteLogV("DEBUG JETTISON: Deleting %d meshes...", booster.at(current).N);
        for (int i = 1; i < booster.at(current).N + 1; i++) {
            oapiWriteLogV("   -> Deleting Mesh Index %d (Handle: %p)", i, booster.at(current).msh_idh[i]);
            DelMesh(booster.at(current).msh_idh[i]);
        }

        // 2. Delete the Thrusters
        oapiWriteLogV("DEBUG JETTISON: Deleting %d Thrusters...", booster.at(current).N);
        for (int bn = 0; bn < booster.at(current).N; bn++) {
            if (booster.at(current).th_booster_h.at(bn) != NULL) {
                oapiWriteLogV("   -> Deleting Thruster Handle %p", booster.at(current).th_booster_h.at(bn));
                DelThruster(booster.at(current).th_booster_h.at(bn));
                booster.at(current).th_booster_h.at(bn) = NULL;
            } else {
                oapiWriteLogV("   -> Thruster Handle %d was already NULL. Skipping.", bn);
            }
        }

        // 3. Delete the Thruster Group
        oapiWriteLogV("DEBUG JETTISON: Checking Thruster Group...");
        if (booster.at(current).Thg_boosters_h != NULL) {
            oapiWriteLogV("   -> Deleting Thruster Group Handle %p", booster.at(current).Thg_boosters_h);
            DelThrusterGroup(booster.at(current).Thg_boosters_h, false);
            booster.at(current).Thg_boosters_h = NULL;
        }

        // 4. Delete the Propellant Tank
        oapiWriteLogV("DEBUG JETTISON: Checking Propellant Tank...");
        if (booster.at(current).tank != NULL) {
            oapiWriteLogV("   -> Deleting Tank Handle %p", booster.at(current).tank);
            DelPropellantResource(booster.at(current).tank);
            booster.at(current).tank = NULL;
        }

        currentBooster += 1;
        oapiWriteLogV("DEBUG JETTISON: Updating Mass and PMI...");
        UpdateMass();
        UpdatePMI();

        if (currentBooster >= nBoosters) wBoosters = false;
        oapiWriteLogV("DEBUG JETTISON: TBOOSTER separation routine complete.");
        break;
    case TSTAGE:
        oapiWriteLogV("DEBUG JETTISON: Initiating TSTAGE %d separation.", current);
        Spawn(type, current);
        oapiWriteLogV("DEBUG JETTISON: Spawn() completed successfully.");

        oapiWriteLogV("DEBUG JETTISON: Clearing Thruster Definitions for entire vessel...");
        ClearThrusterDefinitions();

        oapiWriteLogV("DEBUG JETTISON: Checking Propellant Tank...");
        if (stage.at(current).tank != NULL) {
            oapiWriteLogV("   -> Deleting Tank Handle %p", stage.at(current).tank);
            DelPropellantResource(stage.at(current).tank);
            stage.at(current).tank = NULL;
        }

        oapiWriteLogV("DEBUG JETTISON: Deleting Stage Mesh...");
        DelMesh(stage.at(current).msh_idh);

        currentStage += 1;
        oapiWriteLogV("DEBUG JETTISON: Updating Mass and PMI...");
        UpdateMass();
        UpdatePMI();

        // --- REBUILD THE ROCKET PHYSICS ---
        oapiWriteLogV("DEBUG JETTISON: Initializing engines for new Stage %d", currentStage);
        CreateMainThruster();
        CreateRCS();
        hBus_AP     = CreateThruster(_V(1.0, 999.0, 0.0), _V(0,1,0), 0.0, NULL, 0.0);
        hBus_Config = CreateThruster(_V(2.0, 999.0, 0.0), _V(0,1,0), 0.0, NULL, 0.0);
        hBus_Prog   = CreateThruster(_V(3.0, 999.0, 0.0), _V(0,1,0), 0.0, NULL, 0.0);

        oapiWriteLogV("DEBUG JETTISON: TSTAGE separation routine complete.");
        break;

    case TPAYLOAD:
        Spawn(type, current);

        if (!payload[current].live) {
            for (int ss = 0; ss < payload[current].nMeshes; ss++) {
                oapiWriteLogV("%s: Deleting payload[%i] mesh number %i", GetName(), current, ss);
                DelMesh(payload[current].msh_idh[ss]);
            }
        }
        currentPayload += 1;
        UpdateMass();
        UpdatePMI();
        break;

    case TFAIRING:
        Spawn(type, current);

        // Make payload meshes visible now that fairing is gone
        for (int pns = currentPayload; pns < nPayloads; pns++) {
            if (!payload[pns].live) {
                for (int s = 0; s < payload[pns].nMeshes; s++) {
                    SetMeshVisibilityMode(payload[pns].msh_idh[s], MESHVIS_EXTERNAL);
                }
            }
        }

        for (int ii = 1; ii < fairing.N + 1; ii++) {
            DelMesh(fairing.msh_idh[ii]);
        }

        wFairing = 0;
        UpdateMass();
        UpdatePMI();
        break;

    case TLES:
        oapiWriteLogV("Jettison: Tower Jet");
        Spawn(type, current);
        DelMesh(Les.msh_idh);
        wLes = false;
        UpdateMass();
        UpdatePMI();
        break;

    case TINTERSTAGE:
        Spawn(type, current);
        DelMesh(stage.at(current).interstage.msh_idh);
        currentInterstage += 1;
        stage.at(current).wInter = false;
        UpdateMass();
        UpdatePMI();
        break;
    }
}

// ==============================================================
// Flight Logic & Automation
// ==============================================================
void Multistage2026::InitializeDelays() {
    // Initialize booster burn delays (only if stage 0 and MET > 0)
    if ((currentStage == 0) && (MET > 0)) {
        for (int kb = currentBooster; kb < nBoosters; kb++) {
            if (booster.at(kb).currDelay > 0) {
                booster.at(kb).currDelay -= MET;
            }
        }
    }
    // Initialize stages (accounting for previous staging times)
    else if (currentStage > 0) {
        double delta = MET - stage_ignition_time;
        if (delta < stage.at(currentStage).currDelay) {
            stage.at(currentStage).currDelay -= delta;
        }
    }
}

void Multistage2026::AutoJettison() {
    if (Configuration == 0) return; // Safety: Ground vessels don't jettison

// 1. Boosters
    for (int i = 0; i < nBoosters; i++) {
        if (booster[i].tank != NULL) { 
            // ==========================================
            // BULLETPROOF POINTER VERIFICATION
            // ==========================================
            bool validHandle = false;
            int pCount = GetPropellantCount();
            for (int k = 0; k < pCount; k++) {
                // If our booster tank matches a real Orbiter tank, it's safe!
                if (GetPropellantHandleByIndex(k) == booster[i].tank) {
                    validHandle = true;
                    break;
                }
            }
            // Only query the mass if the handle is 100% verified
            if (validHandle) {
                double mass = GetPropellantMass(booster[i].tank);
                //oapiWriteLogV("JETTISON: Booster %d Mass= %.1f", i, mass);
                if (mass <= 10.0) { 
                    oapiWriteLogV("JETTISON: FIRING EXPLOSIVE BOLTS FOR BOOSTER %d!", i);
                    Jettison(TBOOSTER, i); 
                    booster[i].tank = NULL;
                    currentBooster++; 
                    return; // EXIT FRAME
                }
            } else {
                // We caught the garbage pointer! Neutralize it so it never checks again.
                oapiWriteLogV("JETTISON: Caught garbage pointer in Booster %d. Value: %p", i, booster[i].tank);
                booster[i].tank = NULL; 
            }
        }
    }

    // 2. Main Stages
    if (currentStage < nStages - 1) {
        // Block stage jettison until boosters are gone
        if ((currentStage == 0) && (currentBooster < nBoosters)) { return; }

        if (stage.at(currentStage).tank != nullptr) {
            if (GetPropellantMass(stage.at(currentStage).tank) <= 0.1) {
                oapiWriteLogV("JETTISON: Jettisoning Stage %d", currentStage);
                Jettison(TSTAGE, currentStage);
                return; // EXIT FRAME
            }
        }
    }

    // 3. Interstages (Safe logic)
    // Check wInter for the current active stage AFTER handling stage jettison above
    if (stage.at(currentStage).wInter == true && stage.at(currentStage).interstage.currDelay <= 0) {
        oapiWriteLogV("AUTO: Jettisoning Interstage for Stage %d", currentStage);
        Jettison(TINTERSTAGE, currentStage);
        return; // EXIT FRAME
    }
}

// This writes raw text to Orbiter's global debug line 
// (usually the bottom-left of the screen) using oapiDebugString().
// It tells you exactly what the autopilot is "thinking" (Target Pitch 
// vs. Current Pitch, current Step index, etc.).
void Multistage2026::Guidance_Debug() {
    int step = VinkaGetStep(MET);
    double DesiredPitch;
    char* debug_ptr = oapiDebugString();

    if (Gnc_step[step].Cmd == CM_ROLL) {
        DesiredPitch = (Gnc_step[step].val_init + (Gnc_step[step].val_fin - Gnc_step[step].val_init) * (MET - Gnc_step[step].time_init) / ((Gnc_step[VinkaFindFirstPitch()].time - 1) - Gnc_step[step].time_init)) * RAD;
        double heading;
        oapiGetHeading(GetHandle(), &heading);
        snprintf(debug_ptr, 256, "MET: %.1f Step: %i P: %.2f (%.2f) H: %.2f (%.2f)", MET, step, GetPitch() * DEG, DesiredPitch * DEG, heading * DEG, VinkaAzimuth * DEG);
    }
    else if (Gnc_step[step].Cmd == CM_PITCH) {
        DesiredPitch = (Gnc_step[step].val_init + (Gnc_step[step].val_fin - Gnc_step[step].val_init) * (MET - Gnc_step[step].time_init) / (Gnc_step[step].time_fin - Gnc_step[step].time_init)) * RAD;
        snprintf(debug_ptr, 256, "MET: %.1f Step: %i P: %.2f (%.2f) Delta: %.1f", MET, step, GetPitch() * DEG, DesiredPitch * DEG, GetPitch() * DEG - DesiredPitch * DEG);
    }
    else {
        snprintf(debug_ptr, 256, "MET: %.1f Step: %i", MET, step);
    }
}

// ==============================================================
// Simulation Features: Oscillations, Boiloff, & Failures
// ==============================================================

// --------------------------------------------------------------
// ComplexFlight()
// Simulates "Pogo Oscillation" (thrust fluctuation) and aerodynamic stress.
// It actively varies the max thrust of engines based on a sine wave pattern
// defined in EvaluateComplexFlight(). This creates vibration and
// performance unpredictability during ascent.
// --------------------------------------------------------------
void Multistage2026::ComplexFlight() {
    UpdateComplex += oapiGetSimStep();
    if (UpdateComplex >= 1) {
        UpdateComplex = 0;

        // Reference to avoid repetitive indexing
        STAGE &S = stage[currentStage];

        // Oscillate Main Engines
        for (int i = 0; i < S.nEngines; i++) {
            // Replaced .at() with direct array access
            double oscillation = 1.0 + (S.engine_amp[i] * sin(2 * PI / S.freq[i] * oapiGetSimTime() + S.engine_phase[i]));
            double newMax = (S.thrust / S.nEngines) * oscillation;
            // Set max thrust using the fixed array th_main_h
            SetThrusterMax0(S.th_main_h[i], newMax);
        }

        // Oscillate Booster Engines
        if (wBoosters) {
            BOOSTER &B = booster[currentBooster]; // Assumes booster is now a fixed array/struct
            for (int j = 0; j < B.N; j++) {
                double oscillation = 1.0 + (B.engine_amp[j] * sin(2 * PI / B.freq[j] * oapiGetSimTime() + B.engine_phase[j]));
                double newMax = B.thrust * oscillation;
                // Replaced .at(j) with [j]
                SetThrusterMax0(B.th_booster_h[j], newMax);
            }
        }
    }
}
// --------------------------------------------------------------
// Boiloff()
// Simulates the slow evaporation of cryogenic propellants (LOX/LH2)
// while the vehicle is sitting on the pad or coasting.
// It removes 1kg of fuel every hour (3600 seconds) if enabled in config.
// --------------------------------------------------------------
void Multistage2026::Boiloff() {
    updtboiloff += oapiGetSimStep();
    if (updtboiloff >= 3600) {
        updtboiloff = 0;
        for (int i = currentStage; i < nStages; i++) {
            if (stage.at(i).wBoiloff) {
                double propmass = GetPropellantMass(stage.at(i).tank);
                if (propmass > 1.0) {
                    propmass -= 1.0;
                    SetPropellantMass(stage.at(i).tank, propmass);
                }
            }
        }
    }
}

// --------------------------------------------------------------
// EvaluateComplexFlight()
// Initializes the random parameters for Thrust Oscillation (ComplexFlight).
// It generates random Amplitudes, Phases, and Frequencies for each engine
// so that every flight feels slightly different.
// --------------------------------------------------------------
void Multistage2026::EvaluateComplexFlight() {
    srand((unsigned)time(NULL));

    // Calculate parameters for Main Stages
    for (int i = 0; i < nStages; i++) {
        for (int q = 0; q < stage.at(i).nEngines; q++) {
            int amplitude = rand() % 1500;
            stage.at(i).engine_amp[q] = (double)amplitude / 100000.0; // 0.0% to 1.5% variation
            int transval = rand() % 180;
            stage.at(i).engine_phase[q] = (double)transval * RAD;
            int frequency = rand() % 60;
            stage.at(i).freq[q] = 30.0 + (double)frequency; // 30-90 Hz hum
            oapiWriteLogV("%s Complex Flight-> Stage %i Engine %i: Amp=%.4f Phase=%.2f Freq=%.2f", 
                GetName(), i + 1, q + 1, stage.at(i).engine_amp[q], stage.at(i).engine_phase[q], stage.at(i).freq[q]);
        }
    }

    // Calculate parameters for Boosters
    for (int j = 0; j < nBoosters; j++) {
        for (int z = 0; z < booster.at(j).N; z++) {
            int amplitude = rand() % 1500;
            booster.at(j).engine_amp[z] = (double)amplitude / 100000.0;
            int transval = rand() % 180;
            booster.at(j).engine_phase[z] = (double)transval * RAD;
            int frequency = rand() % 60;
            booster.at(j).freq[z] = 30.0 + (double)frequency;
            oapiWriteLogV("%s Complex Flight-> Booster %i Engine %i: Amp=%.4f Phase=%.2f Freq=%.2f", 
                GetName(), j + 1, z + 1, booster.at(j).engine_amp[z], booster.at(j).engine_phase[z], booster.at(j).freq[z]);
        }
    }
}

// --------------------------------------------------------------
// FailuresEvaluation()
// "Rolls the dice" at simulation start to decide if a catastrophic
// failure will occur during this mission.
// - failureProbability: integer from Config (0-1000 range usually)
// - timeOfFailure: The MET second when the boom happens.
// --------------------------------------------------------------
void Multistage2026::FailuresEvaluation() {
    srand((unsigned)time(NULL));
    int check = rand() % 1000;

    // If the random roll is lower than the probability setting...
    if (check < 10 * failureProbability) {
        // ... Schedule a failure between T+0 and T+300 seconds
        timeOfFailure = rand() % 300;
        oapiWriteLogV("%s: FAILURE SCHEDULED! Time: T+ %i seconds", GetName(), timeOfFailure);
    } else {
        oapiWriteLogV("%s: Systems nominal. No failure scheduled.", GetName());
    }
}

// ==============================================================
// Fuel Mass Calculations
// ==============================================================

double Multistage2026::CalculateFullMass()
{
    double FM = 0.0;

    // 1. Stages (Empty + Fuel + Interstage)
    for (int i = 0; i < nStages; i++) {
        FM += stage.at(i).emptymass;
        FM += stage.at(i).fuelmass;
        if (stage.at(i).wInter) { 
            FM += stage.at(i).interstage.emptymass; 
        }
    }

    // 2. Payloads
    for (int j = 0; j < nPayloads; j++) {
        FM += payload[j].mass;
    }

    // 3. Boosters
    for (int q = 0; q < nBoosters; q++) {
        FM += booster.at(q).fuelmass * booster.at(q).N;
        FM += booster.at(q).emptymass * booster.at(q).N;
    }

    // 4. Optional Hardware
    // Note: 'fairing.emptymass' usually accounts for the total system in MS configs,
    // but some Vinka configs might treat it per-panel. Keeping original logic safe.
    if (hasFairing) {
        FM += fairing.emptymass; 
    }
    if (wAdapter) {
        FM += Adapter.emptymass;
    }
    if (wLes) {
        FM += Les.emptymass;
    }

    return FM;
}

// ==============================================================
// Utilities: Time Formats & Physics Prediction
// ==============================================================

// Converts seconds into a Vector3 (Hours, Minutes, Seconds)
VECTOR3 Multistage2026::hms(double time) {
    VECTOR3 met;
    if (time == 0) {
        met = _V(0, 0, 0);
    } else {
        // Handle negative time (T-Minus) logic
        time = abs(time + 0.5 * (time / abs(time) - 1));
        met.x = floor(time / 3600) - 0.5 * (time / abs(time) - 1);
        met.y = floor((time - met.x * 3600 * (time / abs(time))) / 60) - 0.5 * (time / abs(time) - 1);
        met.z = floor(time - met.x * 3600 * (time / abs(time)) - met.y * 60 * (time / abs(time)));
    }
    return met;
}

// Calculates Acceleration (in Gs) at a specific time 't'
// Used by the Gravity Turn solver to predict flight path.
double Multistage2026::GetProperNforCGTE(double time) {
    double n;
    double Thrust = stage[0].thrust;
    double BoosterFlow = 0;
    double BoosterFuelMassBurnt = 0;

    // Check which boosters are active at this virtual time
    for (int i = 0; i < nBoosters; i++) {
        if ((booster.at(i).burndelay < time) && (time < (booster.at(i).burndelay + booster.at(i).burntime))) {
            Thrust += booster.at(i).thrust * booster.at(i).N;
            BoosterFlow = ((booster.at(i).fuelmass * booster.at(i).N) / booster.at(i).burntime);
            BoosterFuelMassBurnt += BoosterFlow * (time - booster.at(i).burndelay);
        }
    }

    double mass = CalculateFullMass();
    double FirstStageFlow = stage[0].fuelmass / stage[0].burntime;

    // Estimate mass loss (Simplified: assumes linear burn)
    double substr = time * (FirstStageFlow) + BoosterFuelMassBurnt; 
    mass -= substr;

    // F = ma -> a = F/m.  Result is normalized to Gs (g0).
    n = Thrust / (mass * g0);

    // DEBUGGING ONLY - Keep commented to prevent massive log flooding
    // oapiWriteLogV("CGTE Debug: Thrust %.3f fsflow %.3f boostersflow %.3f mass %.3f n %.3f", Thrust, FirstStageFlow, BoosterFlow, mass, n);
    return n;
}

// ==============================================================
// Gravity Turn Solver (CGTE)
// ==============================================================
// Simulates the ascent path to calculate if a specific initial
// pitch angle (psi0) allows the rocket to reach target altitude.
// Returns TRUE if the trajectory reaches the final altitude step.

bool Multistage2026::CGTE(double psi0) {
    double t0, v0, x0, y0;
    double psi, deltaT, deltax, deltay, x, y;

    // 1. Initial State Setup
    // Calculates the "Hover" phase or initial vertical rise until the rocket 
    // clears the tower/terrain before beginning the turn.

    y0 = Misc.COG;
    t0 = 0;
    v0 = 0;
    double dtt = 0.1; // Time step for vertical rise

    // Simulate vertical rise until first altitude step is reached
    while ((y0 < altsteps[1]) && (y0 > 0)) {
        double acceleration = GetProperNforCGTE(t0) * g0;
        y0 += 0.5 * (acceleration - g0) * dtt * dtt + v0 * dtt;
        t0 += dtt;
        v0 += (acceleration - g0) * dtt;
    }

    // 2. Gravity Turn Integration
    double hvel = 0;    // Horizontal Velocity
    double vvel = v0;   // Vertical Velocity
    double vel = v0;    // Total Velocity
    deltaT = 1.0;       // 1-second steps for coarse simulation

    double absacc, hacc, vacc, grel;
    double modspost, normx, normy;

    // Run simulation loop (Limit to 500s duration or pitch reaching 80 deg)
    while ((t0 < 500) && (psi0 < 80 * RAD)) {

        // Calculate Gravity at current altitude
        grel = mu / ((rt + y0) * (rt + y0)) - (vel * vel / (rt + y0));

        // Get Acceleration from Thrust/Mass curve
        double n = GetProperNforCGTE(t0);
        absacc = n * g0;

        // Split acceleration vectors based on current Pitch (psi0)
        hacc = absacc * sin(psi0);
        vacc = absacc * cos(psi0) - grel;

        // Integreate Position
        x = x0 + hvel * deltaT + 0.5 * hacc * deltaT * deltaT;
        y = y0 + vvel * deltaT + 0.5 * vacc * deltaT * deltaT;

        // Integrate Velocity
        hvel += hacc * deltaT;
        vvel += vacc * deltaT;
        vel = sqrt(hvel * hvel + vvel * vvel);

        // Update State
        t0 += deltaT;
        deltax = x - x0;
        deltay = y - y0;
        x0 = x;
        y0 = y;
        v0 = vel;

        // Calculate New Pitch Angle (Gravity Turn Effect)
        // The velocity vector naturally bends; the rocket follows it.
        modspost = sqrt(deltax * deltax + deltay * deltay);
        if (modspost > 0) {
            normx = deltax / modspost;
            normy = deltay / modspost;
            psi = 0.5 * PI - atan2(normy, normx);

            // Ensure pitch only increases (nose drops), never rises
            if (psi > psi0) {
                psi0 = psi;
            }
        }

        // Debug Logging (Keep commented out for speed)
        // oapiWriteLogV("CGTE: t=%.0f x=%.0f y=%.0f v=%.0f psi=%.1f", t0, x, y, v0, psi0 * DEG);

        // Success Condition: If we cleared the final altitude step
        if (y0 > altsteps[3]) {
            return true;
        }
    }

    return false;
}

// ==============================================================
// Visual Effects & Payload Physics
// ==============================================================

void Multistage2026::CheckForAdditionalThrust(int pns) {
    if (GetAttachmentStatus(live_a[pns])) {
        OBJHANDLE live = GetAttachmentStatus(live_a[pns]);
        VESSEL3* v = (VESSEL3*)oapiGetVesselInterface(live);
        // If the attached payload has its own engines active,
        // add that thrust vector to the main stack.
        VECTOR3 TotalThrustVecPL;
        v->GetThrustVector(TotalThrustVecPL);
        TotalThrustVecPL = mul(RotationMatrix(payload[pns].Rotation), TotalThrustVecPL);
        AddForce(TotalThrustVecPL, payload[pns].off[0]);
    }
}

void Multistage2026::CheckForFX(int fxtype, double param) {
    switch (fxtype) {

    // 1. Mach Cone (Transonic/Supersonic condensation clouds)
    case FXMACH:
        if ((param > FX_Mach.mach_min) && (param < FX_Mach.mach_max)) {
            if (!FX_Mach.added) {
                FX_Mach.added = TRUE;
                for (int nmach = 0; nmach < FX_Mach.nmach; nmach++) {
                    PARTICLESTREAMSPEC Pss9 = GetProperPS((char*)FX_Mach.pstream.c_str()).Pss;
                    FX_Mach.ps_h[nmach] = AddParticleStream(&Pss9, FX_Mach.off[nmach], FX_Mach.dir, &lvl);
                }
            }
        } else {
            if (FX_Mach.added == TRUE) {
                for (int nmach = 0; nmach < FX_Mach.nmach; nmach++) {
                    DelExhaustStream(FX_Mach.ps_h[nmach]);
                }
                FX_Mach.added = false;
            }
        }
        break;

    // 2. Venting (Cryogenic vapors on the pad)
    case FXVENT:
        for (int fv = 1; fv <= FX_Vent.nVent; fv++) {
            if (param < FX_Vent.time_fin[fv]) {
                if (!FX_Vent.added[fv]) {
                    PARTICLESTREAMSPEC Pss10 = GetProperPS((char*)FX_Vent.pstream.c_str()).Pss;
                    FX_Vent.ps_h[fv] = AddParticleStream(&Pss10, FX_Vent.off[fv], FX_Vent.dir[fv], &lvl);
                    FX_Vent.added[fv] = TRUE;

                    oapiWriteLogV("Venting Effect Added @: %.3f,%.3f,%.3f dir: %.3f,%.3f,%.3f", 
                        FX_Vent.off[fv].x, FX_Vent.off[fv].y, FX_Vent.off[fv].z, 
                        FX_Vent.dir[fv].x, FX_Vent.dir[fv].y, FX_Vent.dir[fv].z);
                }
            } else {
                if (FX_Vent.added[fv] == TRUE) {
                    DelExhaustStream(FX_Vent.ps_h[fv]);
                    FX_Vent.added[fv] = false;
                }
            }
        }
        break;
    }
}

// Helper to check if the rocket has lifted off the pad
bool Multistage2026::CheckForDetach() {
    VECTOR3 Thrust, horThrust;
    GetThrustVector(Thrust);
    double Mass = GetMass();
    HorizonRot(Thrust, horThrust);

    // If upward thrust exceeds weight (Mass * G), we are flying.
    // Note: 9.81 is hardcoded here as a "generic G", but usually sufficient for detach logic.
    if (horThrust.y > Mass * 9.81) {
        return TRUE;
    } else {
        return false;
    }
}

// ==============================================================
// Failure & Destruction Logic
// ==============================================================

void Multistage2026::boom() {
    // 1. Detach from Pad/Hangar if connected
    if ((wRamp) || (HangarMode)) {
        DelAttachment(AttToRamp);
    }

    // 2. Get current state to spawn wreckage at exact location
    VESSELSTATUS2 vs;
    memset(&vs, 0, sizeof(vs));
    vs.version = 2;
    GetStatusEx(&vs);

    VECTOR3 ofs = _V(0, 0, 0);
    VECTOR3 rofs, rvel = { vs.rvel.x, vs.rvel.y, vs.rvel.z };
    VECTOR3 vel = { 0,0,0 };

    Local2Rel(ofs, vs.rpos);
    GlobalRot(vel, rofs);

    vs.rvel.x = rvel.x + rofs.x;
    vs.rvel.y = rvel.y + rofs.y;
    vs.rvel.z = rvel.z + rofs.z;

    // 3. Spawn the Wreckage Vessel
    // Note: Requires a "wreck" config or vessel class to exist in Orbiter
    OBJHANDLE hwreck = oapiCreateVesselEx("wreck", "boom", &vs);

    oapiWriteLogV("%s: CATASTROPHIC FAILURE! Vehicle destroyed.", GetName());

    // 4. Move Camera focus to wreckage so user sees the aftermath
    oapiSetFocusObject(hwreck);
    oapiCameraScaleDist(20);

    // 5. Cleanup
    if (wRamp) {
        oapiDeleteVessel(hramp);
    }

    // Goodbye, cruel world
    oapiDeleteVessel(GetHandle());
}

void Multistage2026::Failure() {
    srand((unsigned)time(NULL));
    int check = rand() % 1000;

    // Scenario A: Total Destruction (Boom)
    if ((currentStage == 0) && (check < 250)) {
        boom();
    }
    // Scenario B: Engine Failure (Shut down one engine)
    else {
        // Direct index access instead of .at()
        int engineout = rand() % stage[currentStage].nEngines;

        // Kill thrust resource using fixed array indexing
        SetThrusterResource(stage[currentStage].th_main_h[engineout], NULL);
        oapiWriteLogV("%s: ENGINE FAILURE! Stage %i, Engine %i shutdown.", 
                      GetName(), currentStage + 1, engineout);
    }
}

bool Multistage2026::CheckForFailure(double met) {
    // If we hit the pre-determined failure second...
    if ((floor(met) == timeOfFailure) && (!failed)) {
        failed = TRUE; // Ensure it only happens once
        Failure();     // Trigger the event
        return TRUE;
    }
    return false;
}

// ==============================================================
// Telemetry & Configuration
// ==============================================================

int Multistage2026::WriteTelemetryFile(int initline) {
    FILEHANDLE TlmFile;
    char filenmbuff[MAXLEN];

    // Create unique filename based on Sim Time
    snprintf(filenmbuff, sizeof(filenmbuff), "Multistage2026/Telemetry/%s_%.2f_TLM.txt", GetName(), oapiGetSysMJD());

    char buffer[MAXLEN];

    // Open Mode: Create New or Append
    if (initline == 0) {
        TlmFile = oapiOpenFile(filenmbuff, FILE_OUT, CONFIG);
        oapiWriteLine(TlmFile, (char*)"<--!Multistage 2026 Automatically Generated Telemetry File!-->");
        oapiWriteLine(TlmFile, (char*)"MET,Altitude,Speed,Pitch,Thrust,Mass,V-Speed,Acceleration");
    } else {
        TlmFile = oapiOpenFile(filenmbuff, FILE_APP, ROOT);
    }

    // Write Data Lines
    for (int i = initline; i < tlmidx; i++) {
        snprintf(buffer, sizeof(buffer), "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f", 
            tlmAlt[i].x, tlmAlt[i].y, tlmSpeed[i].y, tlmPitch[i].y, 
            tlmThrust[i].y, tlmMass[i].y, tlmVv[i].y, tlmAcc[i].y);
        oapiWriteLine(TlmFile, buffer);
    }

    if (initline == 0) oapiCloseFile(TlmFile, FILE_OUT);
    else oapiCloseFile(TlmFile, FILE_APP);

    return tlmidx - 1;
}

void Multistage2026::Telemetry() {
    // Record data once per second (approx)
    if ((updtlm >= 1.0) && (MET < TLMSECS - 1)) {
        VECTOR3 ThrustVec;
        GetThrustVector(ThrustVec);

        // X = Time (MET)
        tlmAlt[tlmidx].x = MET;
        tlmSpeed[tlmidx].x = MET;
        tlmPitch[tlmidx].x = MET;
        tlmThrust[tlmidx].x = MET;
        tlmMass[tlmidx].x = MET;
        tlmVv[tlmidx].x = MET;
        tlmAcc[tlmidx].x = MET;

        // Y = Data Value
        tlmAlt[tlmidx].y = GetAltitude();
        tlmSpeed[tlmidx].y = GetOS();
        tlmPitch[tlmidx].y = GetPitch() * DEG;
        tlmThrust[tlmidx].y = length(ThrustVec);
        tlmMass[tlmidx].y = GetMass();
        tlmVv[tlmidx].y = GetVPerp();
        tlmAcc[tlmidx].y = getabsacc();

        tlmidx++;
        updtlm = 0;
    }
    updtlm += oapiGetSimStep();

    // Write to disk every 60 seconds if enabled
    if (Misc.telemetry) {
        writetlmTimer += oapiGetSimStep();
        if (writetlmTimer >= 60.0) {
            writetlmTimer = 0;
            tlmnlines = WriteTelemetryFile(tlmnlines);
        }
    }
}

void Multistage2026::CalculateAltSteps(double planetmass) {
    // Scale guidance milestones based on planet gravity relative to Earth
    double altref[4] = { 100, 350, 1400, 35000 };
    double earthmass = 5.973698968e24; 

    for (int i = 0; i < 4; i++) {
        altsteps[i] = altref[i] * planetmass / earthmass;
    }
}

void Multistage2026::SetNewAltSteps(double newstep1, double newstep2, double newstep3, double newstep4) {
    altsteps[0] = newstep1;
    altsteps[1] = newstep2;
    altsteps[2] = newstep3;
    altsteps[3] = newstep4;
}

void Multistage2026::ToggleComplexFlight() {
    if (Complex) {
        Complex = false;
        oapiWriteLogV("%s: Complex Flight Disabled", GetName());
    } else {
        Complex = true;
        EvaluateComplexFlight();
        oapiWriteLogV("%s: Complex Flight Enabled", GetName());
    }
}

// ==============================================================
// Main Flight Loop (Called every timestep)
// ==============================================================
void Multistage2026::FLY(double simtime, double simdtime, double mjdate) {

    // 1. Handle Staging Delays (Autopilot Active)
    if (APstat) {
        // Reference to the current stage to replace .at()
        STAGE &S = stage[currentStage];
        // Handle main stage ignition delay
        if (S.currDelay > 0) {
            S.currDelay -= simdtime;
            S.StageState = STAGE_WAITING;
        }

        // Handle interstage jettison delay
        if (S.interstage.currDelay > 0) {
            S.interstage.currDelay -= simdtime;
        }
    }

    // 2. Handle Booster Delays
    if (wBoosters) {
        for (int nb = 0; nb < nBoosters; nb++) {
            if (booster.at(nb).currDelay > 0) {
                booster.at(nb).currDelay -= simdtime;
            }
        }
    }

    // 3. Auto-Jettison Logic
    if (!AJdisabled) AutoJettison();

    // 4. Main Stage Ignition
    if ((!stage.at(currentStage).Ignited) && (stage.at(currentStage).currDelay <= 0) && (stage.at(currentStage).StageState == STAGE_WAITING)) {
        SetThrusterGroupLevel(THGROUP_MAIN, 1);
        stage.at(currentStage).Ignited = TRUE;
        stage.at(currentStage).IgnitionTime = MET;
        stage.at(currentStage).StageState = STAGE_IGNITED;
        oapiWriteLogV("%s Stage n: %i ignited @%.1f", GetName(), currentStage + 1, stage.at(currentStage).IgnitionTime);
    }

    // 5. Booster Thrust Curve Logic (Ignition now handled by VinkaConsumeStep)
    if (wBoosters) {
        for (int kb = currentBooster; kb < nBoosters; kb++) {
            // Only process boosters that Otto has already ignited
            if (booster.at(kb).Ignited == true) {
                // Calculate time elapsed since this specific booster's ignition
                double btime = MET - booster.at(kb).IgnitionTime;
                double Level = 1.0;

                // Linear interpolation of thrust curve points
                for (int qq = 0; qq < 10; qq++) {
                    if (btime > booster.at(kb).curve[qq].x) {
                        double m, q_const;
                        if (qq < 9) {
                            if (btime < booster.at(kb).curve[qq + 1].x) {
                                m = (booster.at(kb).curve[qq + 1].y - booster.at(kb).curve[qq].y) / (booster.at(kb).curve[qq + 1].x - booster.at(kb).curve[qq].x);
                                q_const = booster.at(kb).curve[qq].y - m * booster.at(kb).curve[qq].x;
                                Level = (m * btime + q_const) / 100.0;
                            }
                        } else {
                            // Last curve segment interpolation
                            m = (booster.at(kb).curve[qq].y - booster.at(kb).curve[qq - 1].y) / (booster.at(kb).curve[qq].x - booster.at(kb).curve[qq - 1].x);
                            q_const = booster.at(kb).curve[qq].y - m * booster.at(kb).curve[qq].x;
                            Level = (m * btime + q_const) / 100.0;
                        }
                    }
                }

                // Safety Guard: Ensure the level never drops below 0 due to curve math
                if (Level < 0) Level = 0;

                // Apply the calculated thrust level to the specific booster group
                if (booster.at(kb).Thg_boosters_h != NULL) {
                    SetThrusterGroupLevel(booster.at(kb).Thg_boosters_h, Level);
                }
            }
        }
    }

    // 6. Guidance (Vinka Autopilot)
    if (APstat) {
        VinkaAutoPilot();
        if (Misc.GNC_Debug == 1) Guidance_Debug();
        double endTime = VinkaFindEndTime();
        // Add a spy log right here to see what the computer thinks the end time is
        static double lastEndTimeLog = 0;
        //if (oapiGetSimTime() - lastEndTimeLog > 5.0) {
        //    oapiWriteLogV("OTTO DIAGNOSTIC: AP is ALIVE. MET: %.1f | EndTime: %.1f", MET, endTime);
        //    lastEndTimeLog = oapiGetSimTime();
        //}
        // GUARDED SHUTDOWN: Only kill the AP if the end time is actually valid (> 0)
        if (MET > endTime && endTime > 0.1) {
            oapiWriteLogV("OTTO: Mission Complete. AP Shutdown at MET %.1f (EndTime: %.1f)", MET, endTime);
            killAP();
            APstat = false;
        }
    }

    // 7. Prevent Re-ignition of non-restartable stages
    if ((!stage.at(currentStage).reignitable) && (stage.at(currentStage).Ignited)) {
        if (GetThrusterGroupLevel(THGROUP_MAIN) == 0) {
            stage.at(currentStage).waitforreignition += simdtime;
            if (stage.at(currentStage).waitforreignition >= 3) {
                for (int i = 0; i < stage.at(currentStage).nEngines; i++) {
                    SetThrusterResource(stage[currentStage].th_main_h[i], NULL);
                }
                stage.at(currentStage).DenyIgnition = TRUE;
            }
        }
    }

    // 8. Telemetry & Systems Update
    //if (tlmidx < TLMSECS) Telemetry();
    Telemetry();

    // 9. Battery Drain Logic
    STAGE &S = stage[currentStage]; // Use direct indexing 
    if (S.batteries.wBatts) {
        // Drain charge based on simulation step time
        S.batteries.CurrentCharge -= oapiGetSimStep();
        // Power Failure Scenario
        if (S.batteries.CurrentCharge <= 0) {
            S.batteries.CurrentCharge = 0;
            // Critical System Failure: Cut all engines if power is lost
            ClearThrusterDefinitions();
            oapiWriteLogV("%s: BLACKOUT! Battery depleted on Stage %i.",GetName(), currentStage + 1);
        }
    }

    // 10. Complex Flight (Thrust Oscillations / Failure Simulation)
    if ((Complex) && (GetDrag() > 1000)) {
        ComplexFlight();
        // Add aerodynamic stress forces
        AddForce(_V(0, 2 * GetDrag() * sin(GetAOA()), 0), _V(0, 0, TotalHeight));
        AddForce(_V(2 * GetDrag() * sin(GetSlipAngle()), 0, 0), _V(0, 0, TotalHeight));

        // Structural Failure Check (High Q + High AoA)
        if (GetDrag() > 500000) { 
            if ((abs(GetAOA()) > 45 * RAD) || (abs(GetSlipAngle()) > 45 * RAD)) { 
                boom(); 
            } 
        }
    }

    // 11. Ullage Motors Logic
    if (stage[currentStage].ullage.wUllage) {
        // Reference for cleaner code
        STAGE &S = stage[currentStage];
        // Check if we are within the lead-time window before main engine ignition
        bool in_anticipation_window = (S.currDelay < S.ullage.anticipation);

        // A. Ignition Trigger
        if (!S.ullage.ignited && in_anticipation_window) {
            SetThrusterLevel(S.ullage.th_ullage, 1.0);
            S.ullage.ignited = true;
            oapiWriteLogV("%s: Ullage Motors Ignited. Stage: %i", GetName(), currentStage + 1);
        }
        // B. Shutdown Logic
        // Shut down if ignited AND main engines have been running longer than the 'overlap' period
        else if (S.ullage.ignited && (S.IgnitionTime != 0) && 
            (MET - S.IgnitionTime > S.ullage.overlap)) {
        SetThrusterLevel(S.ullage.th_ullage, 0.0);
        // Optional: Reset ignited to false if you need to prevent re-entry into this block
        // S.ullage.ignited = false; 
        }
    }

    // 12. Explosive Bolts (Separation Motors)
    // A. Main Stage Separation Bolts
    if (stage[currentStage].expbolt.wExpbolt) {
        STAGE &S = stage[currentStage];
        if (RemBurnTime(currentStage) < S.expbolt.anticipation) {
            // Fix: Use direct array indexing and the newly added threxp_h member
            SetThrusterLevel(S.expbolt.threxp_h, 1.0);
        }
    }

    // B. Booster Separation Bolts
    if (wBoosters && booster[currentBooster].expbolt.wExpbolt) {
        BOOSTER &B = booster[currentBooster];
        if (BoosterRemBurnTime(currentBooster) < B.expbolt.anticipation) {
            // Fix: Replace booster.at() with booster[]
            SetThrusterLevel(B.expbolt.threxp_h, 1.0);
        }
    }
    MET += simdtime;
}

void Multistage2026::clbkSetClassCaps(FILEHANDLE cfg) {
    // 1. Initialize Variables
    // (Only keeping this if it resets simple booleans/ints. 
    //  Do NOT use this to alloc memory!)
    initGlobalVars(); 

    // DELETE THIS: if (Particle.size() < 16) { Particle.resize(20); }
    // DELETE THIS: auto safe_copy = ... (It is unused here and clutters the function)

    // 2. Physical Parameters (The actual job of this function)
    SetSurfaceFrictionCoeff(0.7, 0.7);  // Avoid vibrations on pad
    SetCW(0.2, 0.5, 1.5, 1.5);          // Standard Vinka Drag
    EnableTransponder(true);
    InitNavRadios(4);
    
    // 3. Camera Defaults
    SetCameraOffset(_V(0, 0, 100));     // Default external camera position
}

// ==============================================================
// State Loading (Scenario)
// ==============================================================

void Multistage2026::clbkLoadStateEx(FILEHANDLE scn, void* vs)
{
    oapiWriteLogV("Multistage Version: %i", GetMSVersion());
    oapiWriteLogV("Load State Started");

    char* line;
    double batt_trans = 0;
    bool loadedbatts = false;
    stepsloaded = false;

    // 1. Default to the Class Name (e.g., "SLS_Block1")
    // This handles 99% of cases where the INI matches the vessel class.
    ConfigFile = std::string(GetClassName());

    while (oapiReadScenario_nextline(scn, line)) {
        // 2. Override if the scenario explicitly points elsewhere
        if (!_strnicmp(line, "CONFIG_FILE", 11)) {
             char cfg[256];
             if (sscanf(line + 11, "%s", cfg) == 1) {
                 ConfigFile = std::string(cfg);
             }
        }
        else if (!strnicmp(line, "MET", 3)) {
            sscanf(line + 3, "%lf", &MET);
        }
        else if (!strnicmp(line, "GNC_RUN", 7)) {
            sscanf(line + 7, "%i", &Gnc_running);
            if (Gnc_running == 1) { APstat = TRUE; }
        }
        else if (!strnicmp(line, "BATTERY", 7)) {
            sscanf(line + 7, "%lf", &batt_trans);
            loadedbatts = TRUE;
        }
        else if (!strnicmp(line, "STAGE_IGNITION_TIME", 19)) {
            sscanf(line + 19, "%lf", &stage_ignition_time);
        }
        else if (!strnicmp(line, "FAILURE_PROB", 12)) {
            sscanf(line + 12, "%i", &failureProbability);
        }
        else if (!strnicmp(line, "GNC_AUTO_JETTISON", 17)) {
            int AJVal;
            sscanf(line + 17, "%i", &AJVal);
            if (AJVal == 0) { AJdisabled = TRUE; }
        }
        else if (!strnicmp(line, "GUIDANCE_FILE", 13)) {
            sscanf(line + 13, "%s", guidancefile);
            parseGuidanceFile(guidancefile);
            if (Gnc_running != 1) {
                VinkaCheckInitialMet();
            }
        }
        else if (!strnicmp(line, "TELEMETRY_FILE", 14)) {
            sscanf(line + 14, "%s", tlmfile);
            parseTelemetryFile(tlmfile);
            wReftlm = TRUE;
        }
        else if (!strnicmp(line, "CURRENT_BOOSTER", 15)) {
            sscanf(line + 15, "%i", &currentBooster);
            currentBooster -= 1;
        }
        else if (!strnicmp(line, "CURRENT_INTERSTAGE", 18)) {
            sscanf(line + 18, "%i", &currentInterstage);
            currentInterstage -= 1;
        }
        else if (!strnicmp(line, "CURRENT_STAGE", 13)) {
            sscanf(line + 13, "%i", &currentStage);
            currentStage -= 1;
            if (currentStage > 0) { currentBooster = 11; } 
            // 11 means "All boosters gone" (assuming max 10 groups)
        }
        else if (!strnicmp(line, "STAGE_STATE", 11)) {
            sscanf(line + 11, "%i", &stage.at(currentStage).StageState);
            if (stage.at(currentStage).StageState == STAGE_IGNITED) { stage.at(currentStage).Ignited = TRUE; }
        }
        else if (!strnicmp(line, "CURRENT_PAYLOAD", 15)) {
            sscanf(line + 15, "%i", &currentPayload);
            currentPayload -= 1;
        }
        else if (!strnicmp(line, "FAIRING", 7)) {
            int tmpFairing = 0;
            if (sscanf(line + 7, "%i", &tmpFairing) == 1) {
                wFairing = (tmpFairing != 0);
            }
        }
        else if (!strnicmp(line, "CONFIGURATION", 13)) {
            sscanf(line + 13, "%i", &Configuration);
            if ((Configuration < 0) || (Configuration > 1)) { Configuration = 0; }
        }
        else if (!strnicmp(line, "COMPLEX", 7)) {
            Complex = TRUE;
        }
        else if (!strnicmp(line, "HANGAR", 6)) {
            HangarMode = TRUE;
        }
        else if (!strnicmp(line, "CRAWLER", 7)) {
            wCrawler = TRUE;
        }
        else if (!strnicmp(line, "CAMERA", 6)) {
            sscanf(line + 6, "%lf %lf", &CamDLat, &CamDLng);
            if ((CamDLat == 0) && (CamDLng == 0)) { CamDLat = 0.01; CamDLng = 0.03; }
            wCamera = TRUE;
        }
        else if (!strnicmp(line, "GROWING_PARTICLES", 17)) {
            GrowingParticles = TRUE;
        }
        else if (!strnicmp(line, "DENY_IGNITION", 13)) {
            stage.at(currentStage).DenyIgnition = TRUE;
        }
        else if (!strnicmp(line, "ALT_STEPS", 9)) {
            sscanf(line + 9, "%lf,%lf,%lf,%lf", &altsteps[0], &altsteps[1], &altsteps[2], &altsteps[3]);
            stepsloaded = TRUE;
        }
        else if (!strnicmp(line, "PEG_PITCH_LIMIT", 15)) {
            sscanf(line + 15, "%lf", &PegPitchLimit);
            PegPitchLimit *= RAD;
        }
        else if (!strnicmp(line, "PEG_MC_INTERVAL", 15)) {
            sscanf(line + 15, "%lf", &PegMajorCycleInterval);
        }
        else if (!strnicmp(line, "RAMP", 4)) {
            wRamp = TRUE;
        }
        else if (!strnicmp(line, "ATT_TO_MSPAD", 12)) {
            sscanf(line + 12, "%lf", &MsPadZ.z);
            AttToMSPad = TRUE;
        }
        else if (!strnicmp(line, "ATTMSPAD", 8)) {
            sscanf(line + 8, "%lf", &MsPadZ.z);
            AttToMSPad = TRUE;
        }        else {
            ParseScenarioLineEx(line, vs);
        }
    }

    if (!ConfigFile.empty()) {
        strncpy(fileini, ConfigFile.c_str(), MAXLEN - 1);
        fileini[MAXLEN - 1] = '\0'; // Safety termination
    }

    char tempFile[MAXLEN];
    strcpy(tempFile, OrbiterRoot);
    strcat(tempFile, "/"); // Linux path separator
    strcat(tempFile, fileini);

    oapiWriteLogV("%s: Config File: %s", GetName(), tempFile);
    parseinifile(tempFile);

    if ((currentInterstage > currentStage) || (currentInterstage > nInterstages) || (currentInterstage >= stage.at(currentStage).IntIncremental)) { 
        stage.at(currentStage).wInter = false; 
    }

    if ((wFairing == 1) && (hasFairing == false)) { wFairing = 0; }
    if (Configuration == 0) {
        if (hasFairing == TRUE) {
            wFairing = 1;
        }
        currentStage = 0;
        currentPayload = 0;
        currentBooster = 0;
        currentInterstage = 0;
    }

    LoadMeshes();

    if (Gnc_running == 1) {
        InitializeDelays();
    }

    if (loadedbatts) {
        stage.at(currentStage).batteries.CurrentCharge = batt_trans;
    }

    loadedCurrentBooster = currentBooster;
    loadedCurrentInterstage = currentInterstage;
    loadedCurrentStage = currentStage;
    loadedCurrentPayload = currentPayload;
    loadedwFairing = wFairing;
    loadedConfiguration = Configuration;
    loadedMET = MET;
    loadedGrowing = GrowingParticles;

    oapiWriteLogV("Load State Completed");
}

// ==============================================================
// Input Handling (Keyboard)
// ==============================================================
int Multistage2026::clbkConsumeBufferedKey(int key, bool down, char* kstate) {
    // Only trigger on the key-down event
    if (!down) return 0;

    switch (key) {

        // ------------------------------------------------------
        // 'P' Key: Toggle Autopilot
        // ------------------------------------------------------
        case OAPI_KEY_P:
            if (!KEYMOD_CONTROL(kstate) && !KEYMOD_SHIFT(kstate)) {
                APstat = !APstat;
                oapiWriteLogV("DEBUG: P-Key Pressed! Autopilot is now: %s", APstat ? "ON" : "OFF");
                return 1;
            }
            break;

        // ------------------------------------------------------
        // 'J' Key: Jettison Logic (Manual Staging)
        // ------------------------------------------------------
        case OAPI_KEY_J:
            if (!KEYMOD_CONTROL(kstate) && !KEYMOD_SHIFT(kstate)) {
                oapiWriteLogV("DEBUG: J-Key Pressed! Evaluating Staging...");
                
                if (currentBooster < nBoosters) {
                    Jettison(TBOOSTER, currentBooster);
                    return 1;
                } 
                else if (currentStage < nStages - 1) {
                    if (stage.at(currentStage).wInter == TRUE) {
                        Jettison(TINTERSTAGE, currentStage);
                    } else {
                        Jettison(TSTAGE, currentStage);
                    }
                    return 1;
                } 
                else if ((currentStage == nStages - 1) && (stage.at(currentStage).wInter)) {
                    Jettison(TINTERSTAGE, currentStage);
                    return 1;
                } 
                else if ((currentPayload < nPayloads) && (wFairing == 0)) {
                    Jettison(TPAYLOAD, currentPayload);
                    return 1;
                }
                
                oapiWriteLogV("DEBUG: J-Key Ignored. Nothing left to jettison.");
                return 0;
            }
            break;

        // ------------------------------------------------------
        // 'F' Key: Fairing / LES Jettison
        // ------------------------------------------------------
        case OAPI_KEY_F:
            if (!KEYMOD_CONTROL(kstate) && !KEYMOD_SHIFT(kstate)) {
                if (wLes) {
                    oapiWriteLogV("DEBUG: F-Key Pressed! Jettisoning LES.");
                    Jettison(TLES, 0);
                    return 1;
                } 
                else if (wFairing == 1) { 
                    oapiWriteLogV("DEBUG: F-Key Pressed! Jettisoning Fairing.");
                    Jettison(TFAIRING, 0); 
                    return 1;
                }
                return 0; // Nothing to jettison
            }
            break;

        // ------------------------------------------------------
        // 'CTRL+L': Hangar Logic (Launch)
        // ------------------------------------------------------
        case OAPI_KEY_L:
            if (KEYMOD_CONTROL(kstate) && !KEYMOD_ALT(kstate)) {
                if (HangarMode) {
                    char kstate_send[256];
                    for (int i = 0; i < 256; i++) kstate_send[i] = 0x00;
                    kstate_send[OAPI_KEY_L] = 0x80;
                    kstate_send[OAPI_KEY_LCONTROL] = 0x80;
                    vhangar->SendBufferedKey(OAPI_KEY_L, TRUE, kstate_send);
                    return 1;
                }
            }
            break;

        // ------------------------------------------------------
        // 'CTRL+D': Hangar Logic (Detach/Attach Pad)
        // ------------------------------------------------------
        case OAPI_KEY_D:
            if (KEYMOD_CONTROL(kstate) && !KEYMOD_ALT(kstate)) {
                if (HangarMode) {
                    char kstate_send[256];
                    for (int i = 0; i < 256; i++) kstate_send[i] = 0x00;
                    kstate_send[OAPI_KEY_D] = 0x80;
                    kstate_send[OAPI_KEY_LCONTROL] = 0x80;
                    vhangar->SendBufferedKey(OAPI_KEY_D, TRUE, kstate_send);

                    if (!AttToMSPad) {
                        OBJHANDLE hPad = oapiGetObjectByName((char*)"MS_Pad");
                        if (oapiIsVessel(hPad)) {
                            AttachToMSPad(hPad);
                            AttToMSPad = TRUE;
                        }
                    }
                    return 1;
                }
            }
            break;
    }

    // If the key pressed wasn't any of the cases above, let Orbiter handle it.
    return 0;
}

// AttachToMSPad is exclusively related to Hangar/Pad operations,
// It's not a callback but is part of "Hangar Functions".
void Multistage2026::AttachToMSPad(OBJHANDLE hPad) {
    VESSEL3* vPad;
    vPad = (VESSEL3*)oapiGetVesselInterface(hPad);
    padramp = vPad->CreateAttachment(false, _V(0, 0, 0), _V(0, 1, 0), _V(0, 0, 1), (char*)"Pad", false);
    AttToRamp = CreateAttachment(TRUE, MsPadZ, _V(0, 0, -1), _V(0, 1, 0), (char*)"Pad", false);
    vPad->AttachChild(GetHandle(), padramp, AttToRamp);
}

int Multistage2026::clbkConsumeDirectKey(char* kstate) {
    if (HangarMode) {
        //vhangar->clbkConsumeDirectKey(kstate);
        if (AttToMSPad) {
            VECTOR3 pos, dir, rot;
            GetAttachmentParams(AttToRamp, pos, dir, rot);
            const double delta = 0.1;
            // Adjust height on Hangar Pad using Shift+Arrows
            if (KEYDOWN(kstate, OAPI_KEY_UP) && (!KEYMOD_CONTROL(kstate)) && (KEYMOD_SHIFT(kstate) && (!KEYMOD_ALT(kstate)))) {
                pos.z -= delta;
                SetAttachmentParams(AttToRamp, pos, dir, rot);
            }
            if (KEYDOWN(kstate, OAPI_KEY_DOWN) && (!KEYMOD_CONTROL(kstate)) && (KEYMOD_SHIFT(kstate) && (!KEYMOD_ALT(kstate)))) {
                pos.z += delta;
                SetAttachmentParams(AttToRamp, pos, dir, rot);
            }
        }
    }
    return 0;
}

// ==============================================================
// State Saving (Scenario)
// ==============================================================

void Multistage2026::clbkSaveState(FILEHANDLE scn) {

    oapiWriteLog((char*)"DEBUG: clbkSaveState Called!");

    char savebuff[256], savevalbuff[256];
    if (HangarMode) Configuration = 0;
    SaveDefaultState(scn);
    oapiWriteScenario_string(scn, (char*)"CONFIG_FILE", fileini);
    if (guidancefile[0] != '\0') {
        oapiWriteScenario_string(scn, (char*)"GUIDANCE_FILE", guidancefile);
    }
    oapiWriteScenario_int(scn, (char*)"CONFIGURATION", Configuration);
    if (Complex) {
        oapiWriteScenario_string(scn, (char*)"COMPLEX", (char*)"");
    }
    if (HangarMode) {
        oapiWriteScenario_string(scn, (char*)"HANGAR", (char*)"");
    }
    oapiWriteScenario_int(scn, (char*)"CURRENT_BOOSTER", currentBooster + 1);
    oapiWriteScenario_int(scn, (char*)"CURRENT_STAGE", currentStage + 1);
    oapiWriteScenario_int(scn, (char*)"CURRENT_INTERSTAGE", currentInterstage + 1);
    oapiWriteScenario_int(scn, (char*)"CURRENT_PAYLOAD", currentPayload + 1);
    oapiWriteScenario_int(scn, (char*)"FAIRING", wFairing);

    snprintf(savevalbuff, sizeof(savevalbuff), "%.3f", MET);
    oapiWriteScenario_string(scn, (char*)"MET", savevalbuff);

    if (APstat) {
        Gnc_running = 1;
        oapiWriteScenario_int(scn, (char*)"GNC_RUN", Gnc_running);
    }
    if (stage.at(currentStage).batteries.wBatts) {
        oapiWriteScenario_float(scn, (char*)"BATTERY", stage.at(currentStage).batteries.CurrentCharge);
    }
    if (stage.at(currentStage).DenyIgnition) {
        oapiWriteScenario_string(scn, (char*)"DENY_IGNITION", (char*)"");
    }
    if (GrowingParticles) {
        oapiWriteScenario_string(scn, (char*)"GROWING_PARTICLES", (char*)"");
    }
    if (AJdisabled) {
        oapiWriteScenario_int(scn, (char*)"GNC_AUTO_JETTISON", 1);
    }
    snprintf(savevalbuff, sizeof(savevalbuff), "%.6f", stage_ignition_time);
    oapiWriteScenario_string(scn, (char*)"STAGE_IGNITION_TIME", savevalbuff);
    oapiWriteScenario_int(scn, (char*)"STAGE_STATE", stage.at(currentStage).StageState);
    if (tlmfile[0] != '\0') {
        oapiWriteScenario_string(scn, (char*)"TELEMETRY_FILE", tlmfile);
    }
    snprintf(savevalbuff, sizeof(savevalbuff), "%.1f,%.1f,%.1f,%.1f", altsteps[0], altsteps[1], altsteps[2], altsteps[3]);
    oapiWriteScenario_string(scn, (char*)"ALT_STEPS", savevalbuff);
    snprintf(savevalbuff, sizeof(savevalbuff), "%.3f", PegPitchLimit * DEG);
    oapiWriteScenario_string(scn, (char*)"PEG_PITCH_LIMIT", savevalbuff);
    snprintf(savevalbuff, sizeof(savevalbuff), "%.3f", PegMajorCycleInterval);
    oapiWriteScenario_string(scn, (char*)"PEG_MC_INTERVAL", savevalbuff);
    if (wRamp) {
        oapiWriteScenario_string(scn, (char*)"RAMP", (char*)"");
    }
    if ((HangarMode) && (AttToMSPad)) {
        VECTOR3 pos, dir, rot;
        GetAttachmentParams(AttToRamp, pos, dir, rot);
        oapiWriteScenario_float(scn, (char*)"ATTMSPAD", pos.z);
    }
}

// ==============================================================
// Simulation Steps (Pre Frame Updates)
// ==============================================================
void Multistage2026::clbkPreStep(double simt, double simdt, double mjd) {

    // 1. Autopilot Execution (Must run EVERY frame if engaged)
    if (APstat) {
        VinkaAutoPilot(); // Otto is now free to fly in Config 0 AND Config 1
        if (Misc.GNC_Debug == 1) Guidance_Debug();
    }

    // 1b. Pre-Launch Clock & Delay Sync (Only while on pad)
    if ((APstat) && (Configuration == 0)) {
        MET += simdt;
        stage[0].currDelay = -MET; // Sync delays while waiting on pad
    }
    // 1c. Broadcast Telemetry to the Lua MFD Data Bus
    if (hBus_AP) SetThrusterMax0(hBus_AP, APstat ? 1.0 : 0.0);
    if (hBus_Config) SetThrusterMax0(hBus_Config, (double)Configuration);
    if (hBus_Prog) SetThrusterMax0(hBus_Prog, (double)VinkaStatus);

    // ==========================================
    // THE C++ BROADCAST SPY
    // ==========================================
    //static double last_bus_spy = 0;
    //if (oapiGetSimTime() - last_bus_spy > 1.0) {
    //    oapiWriteLogV("SPY [C++ BUS]: AP=%d | Config=%d | Prog=%d", 
    //                  APstat, Configuration, VinkaStatus);
    //    last_bus_spy = oapiGetSimTime();
    //}

    // 2. Pad Detachment Logic (Launchpad Object)
    if ((wRamp) && (!HangarMode)) {
        if (CheckForDetach()) {
            if (GetAttachmentStatus(AttToRamp) != NULL) {
                vramp->DetachChild(padramp, 0);
                DelAttachment(AttToRamp);
                oapiWriteLogV("Detached from Launchpad");
            }
            // Cleanup Pad Mesh if high enough
            if ((!wLaunchFX) || (GetAltitude() > FX_Launch.CutoffAltitude)) {
                bool deleted = oapiDeleteVessel(hramp);
                if (deleted) {
                    wRamp = false;
                    NoMoreRamp = TRUE;
                    oapiWriteLogV("LaunchPad Deleted from Scenery");
                }
            }
        }
    }
    // 3. Hangar Detachment Logic
    else if ((HangarMode) && (AttToMSPad)) {
        if (CheckForDetach()) {
            DelAttachment(AttToRamp);
            AttToMSPad = false;
            oapiWriteLogV("Detached from Hangar/Pad");
        }
    }

    // 4. Flight Physics
    if (Configuration == 1) {
        FLY(simt, simdt, mjd);
        if (GrowingParticles) ManageParticles(simdt, TRUE);
    }

    // 5. Visual Effects Checks
    if (wMach) CheckForFX(FXMACH, GetMachNumber());
    if ((wVent) && (MET <= 5)) CheckForFX(FXVENT, MET);
    if (wFailures) {
        if (CheckForFailure(MET)) Failure();
    }

    // 6. Logging only once per second
    // static double last_log_time = 0;
    //if (simt - last_log_time > 1.0) {
    //    last_log_time = simt;
    //    VECTOR3 pos, vel, rot;
    //    GetGlobalPos(pos);       // Global position
    //    GetGlobalVel(vel);       // Global velocity
    //    double alt = GetAltitude();
    //    double pitch = GetPitch() * DEG;
    //    double bank = GetBank() * DEG;
    //    bool ground = GroundContact();

        // LOG FORMAT: Time | Alt | Pitch | Bank | GroundContact | Vertical Speed
    //    oapiWriteLogV("TELEMETRY [T+%.0f]: Alt: %.2f m | Pitch: %.2f | Bank: %.2f | Ground: %s | VS: %.2f m/s", 
    //        simt,alt, pitch, bank, ground ? "YES" : "NO", vel.y 
    // Rough vertical speed in global frame (approx)
    //    );
    //}
}

// ==============================================================
// Simulation Steps (Post Frame Updates)
// ==============================================================
void Multistage2026::clbkPostStep(double simt, double simdt, double mjd) {

    // 1. Detect Liftoff Transition
    // Once thrust is established and we are in launch config, restart MET
    if (GetThrusterGroupLevel(THGROUP_MAIN) > 0.05 && Configuration == 0) {
        oapiWriteLogV("Ignition Sequence Start - Configuration 1");
        Configuration = 1;
        MET = 0;
    }

    // 2. Forced Booster Thrust Synchronization
    // Ensures SRBs follow the unified group level for physics integration
    for (int i = 0; i < nBoosters; i++) {
        if (booster[i].Ignited && booster[i].Thg_boosters_h != NULL) {
            SetThrusterGroupLevel(booster[i].Thg_boosters_h, 1.0);
        }
    }

    // 3. Payload Thrust Physics
    // Accounts for any active engines on attached vessels
    for (int pns = currentPayload; pns < nPayloads; pns++) {
        if (payload[pns].live) CheckForAdditionalThrust(pns);
    }

    // 4. Particle Management
    // Updates exhaust stream growth based on atmospheric pressure
    if ((Configuration == 1) && (GrowingParticles)) {
        ManageParticles(simdt, false);
    }

    // 5. Calculate Launch Smoke Intensity
    // th_main_level is the intensity reference for launchpad effects
    th_main_level = GetThrusterGroupLevel(THGROUP_MAIN);

    if (wLaunchFX) {
        if (FX_Launch.CutoffAltitude > 0) {
            // Fade out the ground smoke as altitude increases
            launchFx_level = (-1.0 / FX_Launch.CutoffAltitude) * GetAltitude() + 1.0;

            // Clamp values between 0.0 and 1.0
            if (launchFx_level > 1.0) launchFx_level = 1.0;
            if (launchFx_level < 0.0) launchFx_level = 0.0;
        } else {
            launchFx_level = 1.0;
        }
    } else {
        launchFx_level = 0.0;
    }

    // Scale the smoke particle density by the actual engine throttle
    launchFx_level *= th_main_level;
}

// ==============================================================
// Heads Up Display (HUD) - Minimal Debug Version
// ==============================================================
bool Multistage2026::clbkDrawHUD(int mode, const HUDPAINTSPEC *hps, oapi::Sketchpad *skp)
{
    // ---------------------------------------------------------
    // GATEKEEPERS
    // ---------------------------------------------------------
    if (!bVesselInitialized) return false;
    if (stage.empty()) return false;
    if (currentStage < 0 || currentStage >= stage.size()) return false;

    // ---------------------------------------------------------
    // DRAWING VARIABLES
    // ---------------------------------------------------------
    int x = 10;
    int y = 200;
    int line_h = 16;
    char buff[256];

    // --- SECTION A: BASIC STATUS ---
    sprintf(buff, "--- MultiStage TELEMETRY ---");
    skp->Text(x, y, buff, strlen(buff));
    y += line_h;

    // MET
    sprintf(buff, "MET: %.1f s", oapiGetSimTime());
    skp->Text(x, y, buff, strlen(buff));
    y += line_h;

    // Stage State
    sprintf(buff, "STAGE: %d / %ld  (State: %d)", currentStage + 1, stage.size(), stage.at(currentStage).StageState);
    skp->Text(x, y, buff, strlen(buff));
    y += line_h;


    // --- SECTION B: FLIGHT DYNAMICS ---
    // Altitude
    double alt = GetAltitude();
    if (alt < 10000) sprintf(buff, "ALT:  %.1f m", alt);
    else             sprintf(buff, "ALT:  %.2f km", alt / 1000.0);
    skp->Text(x, y, buff, strlen(buff));
    y += line_h;

    // Vertical Speed
    VECTOR3 vel_air;
    GetAirspeedVector(FRAME_HORIZON, vel_air);
    sprintf(buff, "V/S:  %.1f m/s", vel_air.y);
    skp->Text(x, y, buff, strlen(buff));
    y += line_h;

    // Mach
    sprintf(buff, "MACH: %.2f", GetMachNumber());
    skp->Text(x, y, buff, strlen(buff));
    y += line_h * 1.5;


    // --- SECTION C: GUIDANCE ---
    sprintf(buff, "PITCH: %.1f deg", GetPitch() * DEG);
    skp->Text(x, y, buff, strlen(buff));
    y += line_h;

    sprintf(buff, "ROLL:  %.1f deg", GetBank() * DEG);
    skp->Text(x, y, buff, strlen(buff));
    y += line_h * 1.5;


    // --- SECTION D: ENGINES ---
    // 1. THRUST
    double th_level = 0.0;

    if (stage[currentStage].nEngines > 0) {
        THRUSTER_HANDLE hTh = stage[currentStage].th_main_h[0];
        if (hTh != NULL) {
             th_level = GetThrusterLevel(hTh);
        }
    }
    sprintf(buff, "THRUST: %.1f %%", th_level * 100.0);
    skp->Text(x, y, buff, strlen(buff));
    y += line_h;

    skp->Text(x, y, buff, strlen(buff));
    y += line_h;

    return false; 
}

// --------------------------------------------------------------
// Called at the end of spawn of the vehicle
// --------------------------------------------------------------
void Multistage2026::clbkPostCreation() {
    oapiWriteLog((char*)"DEBUG: clbkPostCreation Called!");

    // 1. THE CHECK: Has the scenario loader already populated the stages?
    // In the Constructor, nStages is set to 0.
    // If clbkLoadStateEx ran, it called parseinifile(), so nStages will be > 0.
    if (nStages > 0) {
        oapiWriteLog((char*)"DEBUG: Scenario State detected. Skipping redundant INI reload.");

        // Even if we skip loading meshes, we MUST ensure physics systems are ready.
        // These functions use the data loaded by the scenario.
        InitParticles();
        VehicleSetup(); 
        UpdateMass();
        UpdatePMI();

        // Actually spawn the pad if the scenario requested it
        if (wRamp) {
            Ramp(false);
        }

        // Setup Camera
        VESSEL* hFocus = oapiGetFocusInterface();
        if ((OBJHANDLE)hFocus == GetHandle()) {
            SetCameraOffset(_V(0, 0, 100));
        }
        return; // EXIT EARLY - Do not parse INI or load meshes again
    }

    // 2. DYNAMIC SPAWN PATH (Scenario Editor / Lua Spawn)
    // If we get here, nStages is 0. We are a blank slate and MUST load from config.
    oapiWriteLog((char*)"DEBUG: Dynamic Spawn detected. Initializing from Config.");

    char configPath[256];
    
    // Resolve Config Path
    if (ConfigFile.empty()) {
        ConfigFile = std::string(GetClassName());
    }

    if (ConfigFile.find("Config/") != std::string::npos || ConfigFile.find(".ini") != std::string::npos) {
        strncpy(configPath, ConfigFile.c_str(), 255);
    } 
    else {
        // Try Multistage specific folder first
        sprintf(configPath, "Config/Multistage2026/%s.ini", ConfigFile.c_str());
        FILE* fCheck = fopen(configPath, "r");
        if (!fCheck) {
            sprintf(configPath, "Config/%s.ini", ConfigFile.c_str());
        } else {
            fclose(fCheck);
        }
    }

    // Parse & Load
    if (!parseinifile(configPath)) {
        oapiWriteLogV("CRITICAL ERROR: Could not load INI file: %s", configPath);
    }

    LoadMeshes();
    InitParticles();
    VehicleSetup();
    // Spawn the pad if the INI requested it (unlikely for dynamic, but safe)
    if (wRamp) {
        Ramp(false);
    }

    // Finalize
    VESSEL* hFocus = oapiGetFocusInterface();
    if ((OBJHANDLE)hFocus == GetHandle()) {
        SetCameraOffset(_V(0, 0, 100));
    }
    
    // Attach to Pad if needed (Hangar mode)
    if (AttToMSPad) {
        OBJHANDLE hPad = oapiGetVesselByName(Misc.PadModule);
        if (hPad) {
            AttToRamp = CreateAttachment(false, _V(0,0,-66), _V(0,0,-1), _V(0,1,0), "PAD");
        }
    }

    if (wFailures) FailuresEvaluation();
    if (Complex) EvaluateComplexFlight();

    UpdateMass();
    UpdatePMI();
}

// --------------------------------------------------------------
// Visual Resource Loading (Mesh Hooks)
// --------------------------------------------------------------
void Multistage2026::clbkVisualCreated(VISHANDLE vis, int refcount) {

    oapiWriteLog((char*)"DEBUG: clbkVisualCreated Called!");

    // This is where we would attach dynamic lights or particle streams 
    // that depend on the visual mesh being present.
    // For now, standard Orbiter handling is sufficient.
    VESSEL3::clbkVisualCreated(vis, refcount);
}

void Multistage2026::clbkVisualDestroyed(VISHANDLE vis, int refcount) {
    VESSEL3::clbkVisualDestroyed(vis, refcount);
}

// ==============================================================
// Environmental Helpers (Smoke, Hangar, Camera)
// ==============================================================

void Multistage2026::CreateLaunchFX() {
    if (FX_Launch.N > 0) {
        // Defines the smoke trail on the ground
        PARTICLESTREAMSPEC launch_smoke = {
            0, 25.0, 5, 200, 0.15, 10.0, 4, 1.0, 
            PARTICLESTREAMSPEC::DIFFUSE, 
            PARTICLESTREAMSPEC::LVL_SQRT, 0, 1, 
            PARTICLESTREAMSPEC::ATM_FLAT, 1, 1, 
            NULL
        };

        // Register standard cloud texture
        launch_smoke.tex = oapiRegisterParticleTexture((char*)"Contrail1");

        // Create the particle stream
        FX_Launch.ps_h = AddParticleStream(&launch_smoke, _V(FX_Launch.Distance, 0, 0), _V(0, 1, 0), &launchFx_level);

        // Create the "flame trench" effect if defined
        if (FX_Launch.Angle != 0) {
            PARTICLESTREAMSPEC trench_flame = {
                0, 15.0, 8, 200, 0.1, 5.0, 4, 1.0, 
                PARTICLESTREAMSPEC::EMISSIVE, 
                PARTICLESTREAMSPEC::LVL_SQRT, 0, 1, 
                PARTICLESTREAMSPEC::ATM_FLAT, 1, 1, 
                NULL
            };
             trench_flame.tex = oapiRegisterParticleTexture((char*)"Exhaust");

             // Calculate vector for the angled flame trench
             VECTOR3 flame_dir = _V(sin(FX_Launch.Angle * RAD), cos(FX_Launch.Angle * RAD), 0);
             FX_Launch.ps2_h = AddParticleStream(&trench_flame, _V(FX_Launch.Distance, 0, 0), flame_dir, &launchFx_level);
        }
    }
}

void Multistage2026::InitParticles() {
    // 1. Memory Firewall: Resize NOW before tanks exist
    if (Particle.size() < 16) {
        Particle.resize(16); 
    }

    // 2. Data Definition: Fill the defaults safely
    // (Ensure you use the helper we fixed earlier)
    SetDefaultParticle(13, "Contrail", "Contrail1", 8, 5, 150);
    Particle[13].Pss.srcspread = 0.3;
    Particle[13].Pss.lifetime = 8;
    Particle[13].Pss.growthrate = 4;

    SetDefaultParticle(14, "Exhaust", "Contrail3", 4, 20, 150);
    Particle[14].Pss.srcspread = 0.1;
    Particle[14].Pss.lifetime = 0.3;
    Particle[14].Pss.growthrate = 12;

    SetDefaultParticle(15, "Clear", "", 0, 0, 0);

    // 3. Registration: Do it ONCE, right here.
    for (int i = 0; i < Particle.size(); i++) {
        // Guard against double-registration if run multiple times
        if (Particle[i].Pss.tex == NULL && strlen(Particle[i].TexName) > 0) {
            Particle[i].Pss.tex = oapiRegisterParticleTexture(Particle[i].TexName);
        }
    }
}

void Multistage2026::SetDefaultParticle(int idx, const char* name, const char* texName, double srcSize, double srcRate, double v0) {
    if (idx >= Particle.size()) return;

    strncpy(Particle[idx].ParticleName, name, MAXLEN - 1);
    Particle[idx].ParticleName[MAXLEN - 1] = '\0';

    strncpy(Particle[idx].TexName, texName, 255);
    Particle[idx].TexName[255] = '\0';

    // Clear the spec and set defaults
    memset(&Particle[idx].Pss, 0, sizeof(PARTICLESTREAMSPEC));
    Particle[idx].Pss.srcsize = srcSize;
    Particle[idx].Pss.srcrate = srcRate;
    Particle[idx].Pss.v0 = v0;
    Particle[idx].Pss.ltype = PARTICLESTREAMSPEC::EMISSIVE;
    Particle[idx].Pss.levelmap = PARTICLESTREAMSPEC::LVL_PSQRT;
    Particle[idx].Pss.atmsmap = PARTICLESTREAMSPEC::ATM_PLOG;
    Particle[idx].Pss.amin = 1e-6;
    Particle[idx].Pss.amax = 0.1;
    Particle[idx].Pss.lmax = 0.5;
}

void Multistage2026::CreateHangar() {
    // Requires "Multistage2015_Hangar" vessel class to be installed
    if (HangarMode) {
        VESSELSTATUS2 vs;
        memset(&vs, 0, sizeof(vs));
        vs.version = 2;
        GetStatusEx(&vs);

        OBJHANDLE hHangar = oapiCreateVesselEx("Hangar", "Multistage2015_Hangar", &vs);
        if (oapiIsVessel(hHangar)) {
            vhangar = (VESSEL3*)oapiGetVesselInterface(hHangar);

            // Setup attachment to hangar roof/crane
            hhangar = vhangar->CreateAttachment(false, _V(0, 26, 0), _V(0, 1, 0), _V(0, 0, 1), (char*)"Hangar", false);
            AttToHangar = CreateAttachment(TRUE, _V(0, 43, 0), _V(0, 1, 0), _V(0, 0, 1), (char*)"Hangar", false);
            vhangar->AttachChild(GetHandle(), hhangar, AttToHangar);
        } else {
            oapiWriteLogV("Error: Multistage2015_Hangar vessel class not found.");
            HangarMode = false; // Disable if missing
        }
    }
}

void Multistage2026::CreateCamera() {
    // Spawns a dedicated camera vessel (optional feature)
    if (wCamera) {
        VESSELSTATUS2 vs;
        memset(&vs, 0, sizeof(vs));
        vs.version = 2;
        GetStatusEx(&vs);

        // Offset camera position
        vs.rpos.x += CamDLat;
        vs.rpos.z += CamDLng;

        OBJHANDLE hCam = oapiCreateVesselEx("Camera", "Camera", &vs);
        if (oapiIsVessel(hCam)) {
            // Point camera at the rocket
            VESSEL3* vCam = (VESSEL3*)oapiGetVesselInterface(hCam);
            // (Simulated look-at logic would go here, kept simple for port)
        }
    }
}

// ==============================================================
// Pad Logic & Getters
// ==============================================================
void Multistage2026::Ramp(bool alreadyramp) {
    oapiWriteLog((char*)"DEBUG: Ramp() Called! Initializing Launch Clamp...");

    if ((wRamp) && (!HangarMode)) {
        VESSELSTATUS2 vs;
        memset(&vs, 0, sizeof(vs));
        vs.version = 2;
        GetStatusEx(&vs);

        hramp = oapiCreateVesselEx("Pad", Misc.PadModule, &vs);

        if (oapiIsVessel(hramp)) {
            vramp = (VESSEL3*)oapiGetVesselInterface(hramp);

            // 2. Create Attachment Points (The "Hard Clamp")
            padramp = vramp->CreateAttachment(false, _V(0, 0, 0), _V(0, 1, 0), _V(0, 0, 1), (char*)"Pad", false);
            AttToRamp = CreateAttachment(TRUE, MsPadZ, _V(0, 0, -1), _V(0, 1, 0), (char*)"Pad", false);

            // 3. Attach the Rocket to the Pad
            vramp->AttachChild(GetHandle(), padramp, AttToRamp);

            // 4. VISUAL MESH DISABLED
            // We intentionally skip loading "KLC39B" here so the physics clamp remains invisible,
            // allowing the scenario's highly detailed pad scenery to shine without overlapping.
            oapiWriteLog((char*)"DEBUG: Ramp physics engaged. Visual clone disabled.");
        } else {
             oapiWriteLog((char*)"ERROR: Failed to create Pad vessel.");
        }
    }
}

double Multistage2026::GetMET() const {
    return MET;
}

int Multistage2026::GetAutopilotStatus() const {
    return (APstat) ? 1 : 0;
}

// ==============================================================
// Generic Callback (MFD Communication)
// ==============================================================
int Multistage2026::clbkGeneric(int msgid, int prm, void* context) {
    switch (msgid) {
        // ==========================================
        // CORE SYSTEM HANDSHAKES & COMMANDS
        // ==========================================
        case VMSG_MS26_IDENTIFY:
            return VMSG_MS26_IDENTIFY;
        case VMSG_MS26_GETDATA:
            return (intptr_t)this;
        case VMSG_MS26_TOGGLE_AP:
            ToggleAP();
            return 1;
        case VMSG_MS26_DELETE_STEP:
            VinkaDeleteStep(prm);
            return 1;
        case VMSG_MS26_WRITE_GNC:
            WriteGNCFile();
            return 1;
        case VMSG_MS26_JETTISON_FAI:
            // Add jettison logic
            return 1;
        case VMSG_MS26_ADD_STEP:
            VinkaAddStep((char*)context); 
            return 1;
        case VMSG_MS26_LOAD_TLM:
            parseTelemetryFile((char*)context);
            return 1;
        case VMSG_MS26_SET_PMC:
            SetPegMajorCycleInterval(*(double*)context); 
            return 1;
        case VMSG_MS26_SET_ALT: {
            double* steps = (double*)context;
            SetNewAltSteps(steps[0], steps[1], steps[2], steps[3]);
            return 1;
        }
        case VMSG_MS26_SET_PITCHLIMIT:
            SetPegPitchLimit(*(double*)context); 
            return 1;
        // ==========================================
        // DEFAULT (UNHANDLED MESSAGES)
        // ==========================================
        default:
            // Safely ignore unknown messages. 
            // Returning 0 tells the calling script "I don't support this command."
            return 0;
    }
    return 0;
}

// Standard Orbiter Generic Cockpit Enabler
bool Multistage2026::clbkLoadGenericCockpit()
{
    // Debug print to PROVE Orbiter is calling this
    oapiWriteLog((char*)"DEBUG: clbkLoadGenericCockpit Called!");

    // 2. Tell Orbiter to use the standard 2-MFD layout
    return true;
}

// ==============================================================
// OPENORBITER MODULE REGISTRATION
// ==============================================================
DLLCLBK void ExitModule(HINSTANCE hModule)
{
    // 1. Unregister the MFD (If you haven't removed the registration yet)
    // Even if you deleted the class, the core might think it's still there
    // oapiUnregisterMFDMode(MFD_NAME); 

    // 2. Clean up global memory
    // If you used 'new' for any global variables or configuration buffers
    // delete MyGlobalObject;

    // 3. Clear any custom UI or GDI elements
    // This is often where "double free" happens if pointers aren't nulled
}

// OpenOrbiter Vessel Factory (Replaces oapiRegisterVesselClass)
DLLCLBK VESSEL *ovcInit (OBJHANDLE hvessel, int flightmodel)
{
    return new Multistage2026 (hvessel, flightmodel);
}

DLLCLBK void ovcExit (VESSEL *vessel)
{
    if (vessel) delete (Multistage2026*)vessel;
}

// Module Date Export
DLLCLBK const char *ModuleDate () { return __DATE__; }

// --- Linker Satisfaction Stubs ---
void Multistage2026::WriteGNCFile() {}
void Multistage2026::GetAttitudeRotLevel(VECTOR3& level) { level = _V(0,0,0); }
void dummy() {}

double Multistage2026::GetPropellantMass(PROPELLANT_HANDLE hProp) {
    // 1. Pointer Sanitation: Block NULL or obviously invalid addresses
    if (hProp == nullptr || (uintptr_t)hProp < 0x1000) {
       oapiWriteLogV("GPM called with a null pointer");
        return 0.0;
    }

    // 2. State Check: Ensure the vessel has finished VehicleSetup
    if (!bVesselInitialized) {
        return 0.0;
    }

    // 3. Core Call: Only pass to the engine once verified
    return VESSEL::GetPropellantMass(hProp);
}

double Multistage2026::GetPropellantMaxMass(PROPELLANT_HANDLE hProp) { 
    return VESSEL::GetPropellantMaxMass(hProp); 
}

// --- END OF FILE ---
