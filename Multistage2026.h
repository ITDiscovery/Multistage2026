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
  -	Include credit to the author in your addon documentation;
  -	Add to the addon documentation the official link of Orbit Hangar Mods for
        download and suggest to download the latest and updated version of the module.
  You CAN use parts of the code of Multistage2015, but in this case you MUST:
  -	Give credits in your copyright header and in your documentation for
        the part you used.
  -	Let your project be open source and its code available for at least
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

#ifndef _MULTISTAGE2026_H
#define _MULTISTAGE2026_H

// ==============================================================
// INCLUDES
// ==============================================================
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <array>
#include <cmath>
#include <map>
#include <unistd.h>
#include <sstream>
#include <cstdint> // Required for uint32_t

// Include SDK
#include "Orbitersdk.h"
#include "OrbiterSoundSDK40.h"

// Only define what Orbitersdk.h specifically missed
typedef uint32_t      DWORD;
typedef void* HINSTANCE;

#define TLMSECS 600
#define MAXLEN 256

// Handshake & Command IDs
#define VMSG_MS26_IDENTIFY       0x2015
#define VMSG_MS26_GETDATA        0x2026
#define VMSG_MS26_TOGGLE_AP      0x3001
#define VMSG_MS26_TOGGLE_COMPLEX 0x3002
#define VMSG_MS26_DELETE_STEP    0x3005
#define VMSG_MS26_WRITE_GNC      0x3006
#define VMSG_MS26_ATT_CTRL       0x3007
#define VMSG_MS26_WRITE_TLM      0x3008
#define VMSG_MS26_JETTISON_FAI   0x3009
#define VMSG_MS26_ADD_STEP       0x5001
#define VMSG_MS26_LOAD_TLM       0x5002
#define VMSG_MS26_SET_PMC        0x5003
#define VMSG_MS26_SET_ALT        0x5004
#define VMSG_MS26_SET_PITCHLIMIT 0x5005

// Data structure for spying
struct MS26_DATA_INTERFACE {
    int nStages, currentStage;
    int nBoosters, currentBooster;
    int nPayloads, currentPayload;
    double MET, totalMass;
    bool APstat, Complex, AttCtrl, runningPeg;
    int nsteps, tlmidx, loadedtlmlines, NN;
    double altsteps[4], TgtPitch, TgtHeading, TMeco, GT_IP_Calculated, PegMajorCycleInterval, PegPitchLimit;
    char name[64];

    // Arrays must be in the struct for the MFD to see them
    oapi::IVECTOR2 tlmAlt[TLMSECS], tlmAcc[TLMSECS], tlmVv[TLMSECS], tlmSpeed[TLMSECS], tlmThrust[TLMSECS], tlmPitch[TLMSECS];
    oapi::IVECTOR2 ReftlmAlt[TLMSECS], ReftlmSpeed[TLMSECS], ReftlmAcc[TLMSECS], ReftlmVv[TLMSECS], ReftlmThrust[TLMSECS], ReftlmPitch[TLMSECS];
};

using namespace std;

// ==============================================================
// TYPE POLYFILLS (Force Definitions)
// ==============================================================

// Force DWORD definition if not present
#ifndef DWORD
typedef uint32_t DWORD;
#endif

// Force BOOL definition
#ifndef BOOL
typedef int BOOL;
#endif

// Force Windows Handles/Types
#ifndef _WINDOWS
    #define _WINDOWS
    typedef void* HINSTANCE;
    typedef long LONG;
    typedef char* LPSTR;
    typedef const char* LPCSTR;
    typedef unsigned long long UINT64;
    
    #ifndef TRUE
        #define TRUE true
    #endif
    #ifndef FALSE
        #define FALSE false
    #endif
    #define MAX_PATH 260
    
    #define HIWORD(l) ((unsigned short)((((unsigned long)(l)) >> 16) & 0xFFFF))
    #define LOWORD(l) ((unsigned short)(((unsigned long)(l)) & 0xFFFF))
    #define _stricmp strcasecmp
    #define _strnicmp strncasecmp
    #define stricmp strcasecmp
    #define strnicmp strncasecmp
    #define GetCurrentDirectory(x,y) getcwd(y,x)
#endif

// --- CONSTANTS ---
#define MAXLEN 256
#define TBOOSTER 0
#define TSTAGE 1
#define TFAIRING 2
#define TLES 3
#define TPAYLOAD 4
#define TINTERSTAGE 5
#define FXMACH 0
#define FXVENT 1
#define CM_NOLINE 0
#define CM_ENGINE 1
#define CM_ROLL 2
#define CM_PITCH 3
#define CM_FAIRING 4
#define CM_LES 5
#define CM_JETTISON 6
#define CM_TARGET 7
#define CM_AOA 8
#define CM_ATTITUDE 9
#define CM_SPIN 10
#define CM_INVERSE 11
#define CM_ENGINEOUT 12
#define CM_ORBIT 13
#define CM_DEFAP 14
#define CM_GLIMIT 15
#define CM_DESTROY 16
#define CM_EXPLODE 17
#define CM_DISABLE_PITCH 18
#define CM_DISABLE_ROLL 19
#define CM_DISABLE_JETTISON 20
#define CM_PLAY 21
#define VMSG_JETTISON 100
#define VMSG_AUTOPILOT 101
#define STAGE_WAITING 0
#define STAGE_IGNITED 1
#define STAGE_SHUTDOWN 2

// ==========================================
// APOLLO DSKY PROGRAM CODES (NASSP Standard)
// ==========================================
#define PROG_IDLE          0   // P00: CMC Idling (Awaiting Commands)
#define PROG_PRELAUNCH     1   // P01: Prelaunch Initialization
#define PROG_ENGINEIGNIT   4   // Not an AGC Code
#define PROG_TOWERJET      5   // Not an AGC Code
#define PROG_FAIRINGJET    6   // Not an AGC Code
#define PROG_ASCENTC       10  // Not an AGC Code (Tower Clear)
#define PROG_ASCENTR       11  // P11: Earth Orbit Insertion Monitor (Launch)
#define PROG_ASCENTP       12  // Not an AGC Code (Pitch)
#define PROG_ASCENTPO      13  // Not an AGC Code (Pitch Over)
#define PROG_ASCENTGT      14  // Not an AGC Code (Gravity Turn)
#define PROG_PEG_INSERT    15  // P15: TLI / Powered Explicit Guidance 
#define PROG_MECO          16  // Not an AGC Code (MECO)
#define PROG_RENDEZVOUS    20  // P20: Universal Tracking / Rendezvous
#define PROG_EXT_DELTAV    30  // P30: External Delta-V Targeting
#define PROG_ORBIT_BURN    40  // P40: SPS/OMS Thrust Maneuver
#define PROG_ENTRY_PREP    61  // P61: Entry Preparation (Attitude setup)
#define PROG_JETTISON      62  // P62: CM/SM Separation (Staging)
#define PROG_AERO_GUID     66  // P66: Atmospheric/Aero Control
#define PROG_BOOM          99  // Not an AGC Code (Destroy)

// ==============================================================
// STRUCTS
// ==============================================================

struct VECTOR4F { double x,y,z,t; VECTOR4F() {x=0;y=0;z=0;t=0;} };
struct VECTOR2 { double x,y; };

struct TEX {
    char TextureName[50][256];
    SURFHANDLE hTex[50];
    TEX() { for(int i=0;i<50;i++) { TextureName[i][0]='\0'; hTex[i]=NULL; } }
};

struct EXPBOLT {
    bool wExpbolt;
    THRUSTER_HANDLE threxp_h;
    VECTOR3 pos, dir;
    char pstream[256];
    double anticipation;
    EXPBOLT() {
        memset(this, 0, sizeof(EXPBOLT));
        anticipation = 1.0; 
    }
};

struct ULLAGE {
    bool wUllage;
    bool ignited;
    THRUSTER_HANDLE th_ullage;
    double thrust, anticipation, overlap, angle, diameter, length, rectfactor;
    int N;
    VECTOR3 dir, pos;
    char tex[256];
    ULLAGE() { memset(this, 0, sizeof(ULLAGE)); }
};

struct BATTS { bool wBatts; double MaxCharge; double CurrentCharge; BATTS() { wBatts=FALSE; MaxCharge=0; CurrentCharge=0; } };

struct MISC {
    double COG;
    int GNC_Debug;
    bool telemetry;
    int Focus;
    bool thrustrealpos;
    double VerticalAngle;
    double drag_factor;
    char PadModule[MAXLEN];
    VECTOR3 td_points[3];
    bool has_custom_td;
    MISC() {
       COG=0;
       GNC_Debug=0;
       telemetry=FALSE;
       Focus=0;
       thrustrealpos=FALSE;
       VerticalAngle=0;
       drag_factor=0;
       PadModule[0]='\0';
       has_custom_td = false;
        for(int i = 0; i < 3; i++) td_points[i] = _V(0, 0, 0);
       }
};

struct PARTICLE {
    char ParticleName[MAXLEN];
    PARTICLESTREAMSPEC Pss;
    double GrowFactor_size, GrowFactor_rate;
    bool Growing;
    char TexName[256];
    PARTICLE() {
        ParticleName[0]='\0'; GrowFactor_size=0; GrowFactor_rate=0; Growing=FALSE;
        TexName[0]='\0'; memset(&Pss,0,sizeof(Pss)); 
    }
};

struct PSGROWING { PSTREAM_HANDLE psh[3]; PARTICLESTREAMSPEC pss; double GrowFactor_size, GrowFactor_rate; THRUSTER_HANDLE th; VECTOR3 pos; bool growing; int status; bool counting; double doublepstime; double basesize, baserate; VECTOR3 basepos; bool ToBooster; int nItem, nEngine; bool FirstLoop; PSGROWING() { memset(&pss,0,sizeof(pss)); psh[0]=0; psh[1]=0; psh[2]=0; GrowFactor_size=0; GrowFactor_rate=0; th=NULL; pos=_V(0,0,0); growing=FALSE; status=0; counting=FALSE; doublepstime=0; basesize=0; baserate=0; basepos=_V(0,0,0); ToBooster=FALSE; nItem=0; nEngine=0; FirstLoop=TRUE; } };

struct BOOSTER {
    string meshname; 
    VECTOR3 off; 
    double height, diameter, thrust, emptymass, fuelmass, burntime, isp;
    double angle, burndelay; VECTOR3 speed, rot_speed;
    int N; bool wps1, wps2; 
    int nEngines; VECTOR3 eng[4]; double engine_phase[32], engine_amp[32], freq[32];
    double eng_diameter; VECTOR3 eng_dir; string eng_tex, eng_pstream1, eng_pstream2;
    string module;
    bool Ignited, PSSdefined;
    array<MESHHANDLE, 5> msh_h; array<int, 5> msh_idh; PROPELLANT_HANDLE tank; array<THRUSTER_HANDLE, 4> th_booster_h; THGROUP_HANDLE Thg_boosters_h;
    double currDelay, IgnitionTime, volume;
    bool ParticlesPacked; int ParticlesPackedToEngine;
    VECTOR3 curve[10]; EXPBOLT expbolt;
    BOOSTER() :
        off(_V(0,0,0)), height(0), diameter(0), thrust(0), emptymass(0), 
        fuelmass(0), burntime(0), isp(0), angle(0), burndelay(0), 
        speed(_V(0,0,0)), rot_speed(_V(0,0,0)), N(0), wps1(false), wps2(false),
        nEngines(0), eng_diameter(0), eng_dir(_V(0,0,1)), Ignited(false), 
        PSSdefined(false), tank(nullptr), Thg_boosters_h(nullptr), 
        currDelay(0), IgnitionTime(0), volume(0), ParticlesPacked(false), 
        ParticlesPackedToEngine(0)
        {
        // Removed memset(this, 0, sizeof(BOOSTER));
        msh_idh.fill(0);
        th_booster_h.fill(nullptr);
        }
};

struct INTERSTAGE { string meshname; VECTOR3 off; double height, diameter, emptymass; string module; VECTOR3 speed, rot_speed; double separation_delay; double currDelay; MESHHANDLE msh_h; int msh_idh; double volume; INTERSTAGE() { meshname=""; off=_V(0,0,0); height=0; diameter=0; emptymass=0; module=""; speed=_V(0,0,0); rot_speed=_V(0,0,0); separation_delay=0; currDelay=0; msh_h=NULL; msh_idh=0; volume=0; } };

struct STAGE {
    char meshname[256];
    VECTOR3 off; 
    double height, diameter, thrust, emptymass, fuelmass, burntime, isp;
    bool wps1, wps2;
    int nEngines; 
    VECTOR3 eng[32]; 
    double engine_phase[32], engine_amp[32], freq[32]; 
    array<VECTOR4F, 32> engV4;
    double eng_diameter; 
    VECTOR3 eng_dir; 
    char eng_tex[256], eng_pstream1[256], eng_pstream2[256], module[256];
    VECTOR3 speed, rot_speed;
    BATTS batteries; 
    double ignite_delay;
    double pitchthrust, rollthrust, yawthrust; 
    bool defpitch, defyaw, defroll;
    double linearthrust, linearisp;
    ULLAGE ullage; EXPBOLT expbolt;
    bool Ignited, reignitable, DenyIgnition, wBoiloff; 
    int StageState;
    double currDelay, IgnitionTime, volume, IntIncremental;
    double ispref, pref;
    bool ParticlesPacked; int ParticlesPackedToEngine;
    bool wInter;
    INTERSTAGE interstage;
    THRUSTER_HANDLE th_main_h[32];
    THRUSTER_HANDLE th_att_h[32];
    THGROUP_HANDLE thg_main_h;
    PROPELLANT_HANDLE tank;
    MESHHANDLE msh_h; int msh_idh;
    double waitforreignition;
    STAGE() :
        off(_V(0,0,0)), height(0), diameter(0), thrust(0), emptymass(0), 
        fuelmass(0), burntime(0), isp(0), wps1(false), wps2(false), 
        nEngines(0), eng_diameter(0), eng_dir(_V(0,0,1)), ignite_delay(0),
        pitchthrust(0), rollthrust(0), yawthrust(0), defpitch(false), 
        defyaw(false), defroll(false), linearthrust(0), linearisp(0),
        Ignited(false), reignitable(true), DenyIgnition(false), wBoiloff(false),
        StageState(0), currDelay(0), IgnitionTime(0), volume(0), 
        IntIncremental(0), ispref(0), pref(0), ParticlesPacked(false), 
        ParticlesPackedToEngine(0), wInter(false), waitforreignition(0),
        thg_main_h(nullptr), tank(nullptr), msh_h(nullptr), msh_idh(0)
        {
        // Removed memset(this, 0, sizeof(STAGE));
        // Arrays and Strings initialize themselves automatically!
        for(int i=0; i<32; i++) {
            eng[i] = _V(0,0,0);
            th_main_h[i] = nullptr;
            th_att_h[i] = nullptr;
        }
    }
};

struct PAYLOAD { string meshname, meshname0, meshname1, meshname2, meshname3, meshname4; array<VECTOR3, 5> off; double height, diameter, mass; string module, name; char MultiOffset[128]; VECTOR3 speed, rot_speed; double volume; int render, nMeshes; MESHHANDLE msh_h[5]; int msh_idh[5]; VECTOR3 Rotation; bool rotated, live; PAYLOAD() { for(int i=0;i<5;i++){ off[i]=_V(0,0,0); msh_h[i]=NULL; msh_idh[i]=0; } height=0; diameter=0; mass=0; module=""; name=""; MultiOffset[0]='\0'; speed=_V(0,0,0); rot_speed=_V(0,0,0); volume=0; render=0; nMeshes=0; Rotation=_V(0,0,0); rotated=FALSE; live=FALSE; } };
struct FAIRING { string meshname; VECTOR3 off; double height, diameter, emptymass; int N; double angle; string module; VECTOR3 speed, rot_speed; MESHHANDLE msh_h[4]; int msh_idh[4]; double volume; FAIRING() { meshname=""; off=_V(0,0,0); height=0; diameter=0; emptymass=0; N=0; angle=0; module=""; speed=_V(0,0,0); rot_speed=_V(0,0,0); for(int i=0;i<4;i++){msh_h[i]=NULL; msh_idh[i]=0;} volume=0;} };
struct LES { string meshname; VECTOR3 off; double height, diameter, emptymass; string module; VECTOR3 speed, rot_speed; MESHHANDLE msh_h; int msh_idh; double volume; LES() { meshname=""; off=_V(0,0,0); height=0; diameter=0; emptymass=0; module=""; speed=_V(0,0,0); rot_speed=_V(0,0,0); msh_h=NULL; msh_idh=0; volume=0;} };
struct ADAPTER { string meshname; VECTOR3 off; double height, diameter, emptymass; MESHHANDLE msh_h; int msh_idh; ADAPTER() { meshname=""; off=_V(0,0,0); height=0; diameter=0; emptymass=0; msh_h=NULL; msh_idh=0;} };
struct FX_MACH { double mach_min, mach_max; string pstream; bool added; array<VECTOR3, 10> off; VECTOR3 dir; PSTREAM_HANDLE ps_h[10]; int nmach; FX_MACH() { mach_min=0; mach_max=0; added=FALSE; dir=_V(0,0,0); nmach=0; off.fill(_V(0,0,0)); } };
struct FX_VENT { array<double, 11> time_fin; string pstream; array<VECTOR3, 11> off; array<VECTOR3, 11> dir; PSTREAM_HANDLE ps_h[11]; array<bool, 11> added; int nVent; FX_VENT() { nVent=0; for(int i=0;i<=10;i++) { time_fin[i]=0; off[i]=_V(0,0,0); dir[i]=_V(0,0,0); added[i]=FALSE; } }; };
struct FX_LAUNCH { int N; double H, Angle, Distance, CutoffAltitude; string Ps1, Ps2; PSTREAM_HANDLE ps_h, ps2_h; FX_LAUNCH() { N=0; H=0; Angle=0; Distance=0; CutoffAltitude=0; Ps1=""; Ps2=""; ps_h=NULL; ps2_h=NULL; } };
struct GNC_STEP { double time; char Comand[32]; int GNC_Comand; double val_init; double trval1, trval2, trval3, trval4, trval5, trval6; bool executed; double time_fin; int Cmd; double val_fin, duration, time_init; GNC_STEP() { time=0; Comand[0]='\0'; GNC_Comand=0; val_init=0; trval1=0; trval2=0; trval3=0; trval4=0; trval5=0; trval6=0; executed=FALSE; time_fin=0; Cmd=0; val_fin=0; duration=0; time_init=0;} };
struct SSOUND { bool Main, Hover, RCS_ta, RCS_ts, RCS_tr, Stage, Fairing, Booster, Gear, Umbilical, Lock, Click, Warning; char Main_wav[256], Hover_wav[256], RCS_ta_wav[256], RCS_ts_wav[256], RCS_tr_wav[256]; char Stage_wav[256], Fairing_wav[256], Booster_wav[256], Gear_wav[256], Umbilical_wav[256], Lock_wav[256], Click_wav[256], Warning_wav[256]; int GncStepSound[500]; SSOUND() { Main=FALSE; Hover=FALSE; RCS_ta=FALSE; RCS_ts=FALSE; RCS_tr=FALSE; Stage=FALSE; Fairing=FALSE; Booster=FALSE; Gear=FALSE; Umbilical=FALSE; Lock=FALSE; Click=FALSE; Warning=FALSE; for(int i=0; i<500; i++) GncStepSound[i]=0;} };
typedef std::map<int, int> SoundMap;

class Multistage2026 : public VESSEL3 {
public:
    Multistage2026(OBJHANDLE hObj, int fmodel);
    ~Multistage2026();

    std::string ConfigFile;

    // Callbacks
    bool bVesselInitialized;
    void clbkSetClassCaps(FILEHANDLE cfg) override;
    void clbkPostStep(double simt, double simdt, double mjd) override;
    void clbkPreStep(double simt, double simdt, double mjd) override;
    void clbkLoadStateEx(FILEHANDLE scn, void *vs) override;
    void clbkSaveState(FILEHANDLE scn) override;
    
    // SDK Typedefs should handle DWORD now
    // Changed DWORD -> int to match Linux signature, removed override to bypass strict check
    int clbkConsumeBufferedKey(int key, bool down, char* kstate);

    void clbkPostCreation() override;
    int clbkGeneric(int msgid, int prm, void *context) override;
    virtual bool clbkLoadGenericCockpit();

//#ifdef _DEBUG_HUD
    virtual bool clbkDrawHUD(int mode, const HUDPAINTSPEC *hps, oapi::Sketchpad *skp) override;
//#endif

    void clbkVisualCreated(VISHANDLE vis, int refcount) override;
    void clbkVisualDestroyed(VISHANDLE vis, int refcount) override;
    int clbkConsumeDirectKey(char* kstate) override;
    
    // ... (Functions same as before, preserving decls)
    void parseInterstages(char* filename, int parsingstage);
    void parseLes(char* filename);
    void parseAdapter(char* filename);
    void parseStages(char* filename);
    void parseBoosters(char* filename);
    void ArrangePayloadMeshes(std::string data, int pnl);
    void ArrangePayloadOffsets(std::string data, int pnl);
    void parsePayload(char* filename);
    void parseFairing(char* filename);
    void parseParticle(char* filename);
    void parseTexture(char* filename);
    void parseMisc(char* filename);
    void parseFXMach(char* filename);
    void parseFXVent(char* filename);
    void parseFXLaunch(char* filename);
    void parseSound(char* filename);
    void parseGuidanceFile(char* filename);
    bool parseinifile(char* filename);
    void initGlobalVars();
    
    VECTOR4F _V4(double x, double y, double z, double t);
    VECTOR2 _V2(double x, double y);
    int SignOf(double X);
    bool IsOdd(int integer);
    VECTOR3 CharToVec(const std::string& str);
    VECTOR4F CharToVec4(const std::string &str);
    VECTOR3 RotateVecZ(VECTOR3 input, double Angle);
    MATRIX3 RotationMatrix(VECTOR3 angles);
    double GetHeading();
    double GetOS();
    double StageDv(int dvstage);
    double DvAtStage(int dvatstage);
    double finalv(double Abside, double Apo, double Peri);
    double CalcAzimuth();
    double GetVPerp();
    double GetProperRoll(double RequestedRoll);
    VECTOR3 RotateVector(const VECTOR3& input, double angle, const VECTOR3& rotationaxis);
    
    void ParseCfgFile(char* filename);
    void CreateMainThruster();
    void CreateBoosterThruster();
    void CreateAttitudeThruster();
    void CreateRCS(); 
    void CreateUllageAndBolts(); 
    void InitializeDelays(); 
    void Guidance_Debug(); 
    void ComplexFlight(); 
    void Boiloff(); 
    void EvaluateComplexFlight(); 
    void FailuresEvaluation(); 
    double CalculateFullMass(); 
    double GetProperNforCGTE(double time); 
    bool CGTE(double psi0); 
    void CheckForAdditionalThrust(int pns); 
    void Failure(); 
    void Telemetry(); 
    void CalculateAltSteps(double planetmass); 
    void FLY(double simtime, double simdtime, double mjdate); 
    void LoadMeshes();
    void UpdateOffsets();
    void UpdateMass();
    void UpdatePMI();
    void DefineAnimations();
    void Jettison();
    void Jettison(int type, int current); 
    void AutoJettison();
    void Separation(int stg);
    void BoosterSeparation(int bst);
    void FairingSeparation();
    void LesSeparation();
    void PayloadSeparation(int pld);
    void AdapterSeparation();
    void CheckForFX(int mode, double dt);
    bool CheckForDetach();
    bool CheckForFailure(double dt);
    void boom();
    void VehicleSetup();
    void ResetVehicle(VECTOR3 ofs, bool shift);
    void Spawn(int item, int mode);
    void UpdateLivePayloads();
    void AttachToMSPad(OBJHANDLE hPad);
    void Ramp(bool alreadyramp=false); 
    void CreateLaunchFX();
    void CreateHangar();
    void CreateCamera();
    double GetMET() const; 
    int GetAutopilotStatus() const; 
    int GetMSVersion() { return 2015; }

    int VinkaStatus;
    void VinkaRearrangeSteps();
    void VinkaComposeGNCSteps();
    void VinkaConsumeStep(int step);
    int VinkaFindRoll();
    int VinkaFindFirstPitch();
    virtual int VinkaCountSteps();
    virtual int VinkaGetStep(double met);
    void VinkaAddStep(char* str);
    void VinkaDeleteStep(int step);

    void Attitude(double pitch, double roll, double heading, double pitchrate, double rollrate, double yawrate);
    void ExecuteAttitude(double dt);
    void VinkaPitch(int step);
    void KillRot();
    void ToggleAttCtrl(bool p, bool y, bool r);
    void ToggleAP();
    void InitPEG();
    void CalculateTargets();
    bool CutoffCheck();
    void CalcMECO();
    void Navigate();
    void FEstimate();
    void FStaging();
    void MajorCycle();
    void PEG();
    void WriteGNCFile();
    
    double GetMassAtStage(int stg, bool empty);
    double CurrentStageRemDv();
    double StageRemDv(int stg);
    double RemBurnTime(int stg, double level=1.0);
    double BoosterRemBurnTime(int bst, double level=1.0);
    double GetPropellantMass(PROPELLANT_HANDLE ph);
    double GetPropellantMaxMass(PROPELLANT_HANDLE ph);
    PARTICLE GetProperPS(char* name);
    void SetDefaultParticle(int idx, const char* name, const char* texName, double srcSize, double srcRate, double v0);
    void* GetProperExhaustTexture(char* name); 
    VECTOR3 hms(double t);
    void GetRotLevel(VECTOR3 &lvl);
    void GetAttitudeRotLevel(VECTOR3 &lvl);
    double GetProperHeading();
    void ToggleComplexFlight();
    
    double b0(int j, double t_j);
    double b_(int n, int j, double t_j);
    double c0(int j, double t_j);
    double c_(int n, int j, double t_j);
    double a(int j, double t_j);
    double getacc();
    double getabsacc();
    VECTOR3 GetAPTargets();
    void SetPegMajorCycleInterval(double val);
    void SetPegPitchLimit(double val);
    void SetNewAltSteps(double s1, double s2, double s3, double s4);
    
    void killAP();
    void VinkaUpdateRollTime();
    void VinkaEngine(int step);
    void VinkaRoll(int step);
    void VinkaAutoPilot();
    void VinkaCheckInitialMet();
    double VinkaFindEndTime();
    
    PSTREAM_HANDLE AddExhaustStreamGrowing(THRUSTER_HANDLE th, const VECTOR3& pos, PARTICLESTREAMSPEC* pss, bool growing, double growfactor_size, double growfactor_rate, bool count, bool ToBooster, int N_Item, int N_Engine);
    VECTOR3 GetBoosterPos(int nBooster, int N);
    char* GetProperPayloadMeshName(int pnl, int n);
    void RotatePayload(int pns, int nm, VECTOR3 anglesrad);
    
    // Variables
    vector<STAGE> stage;
    vector<BOOSTER> booster;
    vector<PAYLOAD> payload;
    FAIRING fairing;
    INTERSTAGE interstage; // Temp holder
    LES Les;
    ADAPTER Adapter;
    MISC Misc;
    BATTS Batteries;
    FX_MACH FX_Mach;
    FX_VENT FX_Vent;
    FX_LAUNCH FX_Launch;
    
    int currentStage, currentBooster, currentPayload;
    int nStages, nBoosters, nPayloads;
    double MET;
    bool APstat;
    bool wFairing, wLes, wAdapter, wBoosters, wLaunchFX;

    GNC_STEP Gnc_step[500];
    int nsteps;
    double DesiredPitch, DesiredRoll;
    bool AttCtrl, PitchCtrl, YawCtrl, RollCtrl;

    double tgtapo, tgtperi, tgtinc, tgtabside;
    bool runningPeg;
    double PegMajorCycleInterval, PegPitchLimit;
    double PegDesPitch, TgtPitch;
    double TMeco;
    int NN; // PEG stages
    double altsteps[4];
    double GT_IP_Calculated;
    
    double v_e[10], a_[10], a_fin[10], tau_[10];
    double A[10], B[10], T[10], OmegaS[10], delta_rdot[10], rdot_in[10], delta_r[10], r_in[10], r_T[10], rdot_T[10];
    double VthetaAtStaging[10], DeltaA[10], DeltaB[10];
    struct PEG_COC { double T; int stage; double remBT[10]; int rem_stages; } coc;
    double coeff[100];
    double MECO_TEST;
    int MECO_Counter;
    double avgcoeff;
    double UpdatePegTimer;
    int J; // PEG current stage index
    double epsfin, tgtvv, eps;
    VECTOR3 ShipSpeed;
    double VertVel;
    VECTOR3 rvec, vvec, hvec;
    double r, v, h, thrust, mass, omega;
    double g, cent, g_term, cent_term;
    
    VECTOR2 ReftlmAlt[TLMSECS], ReftlmSpeed[TLMSECS], ReftlmPitch[TLMSECS], ReftlmThrust[TLMSECS];
    VECTOR2 ReftlmMass[TLMSECS], ReftlmVv[TLMSECS], ReftlmAcc[TLMSECS];
    VECTOR2 tlmAlt[TLMSECS], tlmSpeed[TLMSECS], tlmPitch[TLMSECS], tlmThrust[TLMSECS];
    VECTOR2 tlmMass[TLMSECS], tlmVv[TLMSECS], tlmAcc[TLMSECS];
    int tlmidx, loadedtlmlines;
    
    vector<PARTICLE> Particle;
    int nParticles;
    vector<PSGROWING> psg;
    int nPsh;
    double particlesdt;
    
    SSOUND Ssound;
    int soundID;
    
    char configFilename[256];
    char OrbiterRoot[MAXLEN];
    bool ConfigLoaded;
    
    bool Complex;
    bool HangarMode, wCrawler, wCamera, GrowingParticles, stepsloaded, wRamp, AttToMSPad, hasFairing, loadedbatts, AJdisabled, wReftlm;
    int Gnc_running, AJVal;
    
    double stage_ignition_time;
    int failureProbability;
    double timeOfFailure;
    bool wFailures, failed, killDMD;
    double currentDelta, perror, yerror, rerror, totalHeight, Tu;
    bool rolldisabled, pitchdisabled, spinning;
    double lvl, DeltaUpdate, GT_InitPitch, UpdateComplex, wPeg, updtlm, writetlmTimer;
    int tlmnlines; double updtboiloff, RefPressure;
    bool NoMoreRamp; 
    COLOUR4 col_d, col_s, col_a, col_white;
    double th_main_level, launchFx_level;
    VECTOR3 hangaranims, MsPadZ, MaxTorque; 
    ATTACHMENTHANDLE AttToCrawler;
    double Azimuth, CamDLat, CamDLng;
    int Configuration; double Ku, MyID;
    OBJHANDLE PadHangar;
    MATRIX3 RotMatrix;
    
    double RotatePayloadAnim_x[5][10], RotatePayloadAnim_y[5][10], RotatePayloadAnim_z[5][10];
    ANIMATIONCOMPONENT_HANDLE anim_x[5][10]; 
    ANIMATIONCOMPONENT_HANDLE anim_y[5][10]; 
    ANIMATIONCOMPONENT_HANDLE anim_z[5][10];
    
    PSTREAM_HANDLE exhaustN[10][32];
    VECTOR3 fhat, hhat, thetahat, rhat;
    double g0, initlat, initlong, kd, ki, kp;
    ATTACHMENTHANDLE live_a[10];
    int loadedConfiguration, loadedCurrentBooster, loadedCurrentInterstage;
    int loadedCurrentPayload, loadedCurrentStage;
    bool loadedGrowing; double loadedMET; int loadedwFairing;
    double mul_val; 
    char* note;
    ELEMENTS el;
    THGROUP_HANDLE thg_h_main;
    VESSEL *vCrawler, *vhangar, *vramp;
    VESSELSTATUS2 vs2; VESSELSTATUS vshangar, vsramp;
    double vtheta, z;
    char buffreset[MAXLEN], dataparsed[MAXLEN], fileini[MAXLEN];
    char guidancefile[MAXLEN], logbuff[MAXLEN], tlmfile[MAXLEN];
    TEX tex;
    double pintegral, rdot, rt;
    double TotalHeight, VinkaAzimuth, VinkaMode;
    MGROUP_ROTATE* payloadrotatex[5][10]; MGROUP_ROTATE* payloadrotatey[5][10]; MGROUP_ROTATE* payloadrotatez[5][10]; 
    VECTOR3 hangaranimsV;
    int nInterstages, currentInterstage;
    int nTextures;
    double CogElev;
    bool wMach, wVent;
    bool DeveloperMode;
    double DMD; 
    int nGuidanceSteps; 
    int nReftlm; 
    int nPsg; 
    FILEHANDLE config; 
    vector<TOUCHDOWNVTX> TouchdownPoints;

    ATTACHMENTHANDLE liveatt;
    ATTACHMENTHANDLE AttToRamp, AttToHangar;
    ATTACHMENTHANDLE hhangar; 
    ATTACHMENTHANDLE padramp;
    OBJHANDLE hCrawler; 
    OBJHANDLE hramp; 
    
    void InitParticles();
    void ManageParticles(double dt, bool prestep);
    void parseTelemetryFile(char* name);
    int WriteTelemetryFile(int mode);

    // ==========================================
    // LUA DATA BUS HANDLES
    // ==========================================
    THRUSTER_HANDLE hBus_AP;
    THRUSTER_HANDLE hBus_Config;
    THRUSTER_HANDLE hBus_Prog;
};
#endif
