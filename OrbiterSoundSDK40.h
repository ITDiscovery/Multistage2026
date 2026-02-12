//////////////////////////////////////////////////////////////////////////////////////////
// ORBITERSOUND 4.0 (3D) SDK
// SDK to add sound to Orbiter's VESSEL (c) Daniel Polli aka DanSteph
// For ORBITER ver:2010 with patch P1
//////////////////////////////////////////////////////////////////////////////////////////
//
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!! IMPORTANT INFORMATION							       		!!!!
// !!!!												!!!!
// !!!! ORBITERSOUND SDK IS VERY USEFULL TO PLAY SPECIAL EVENTS	SOUNDS BUT IF YOU		!!!!
// !!!! WANT JUST TO REPLACE THE 43 DEFAULT SOUNDS, CHANGE THE 22 OPTIONS AND/OR		!!!!
// !!!! PLAY ANIMATION SOUNDS, THE NEW CONFIGURATION FILE INTRODUCED IN 4.0 VERSION		!!!! 
// !!!! IS MUCH MORE PRACTICALL AND USER-FRIENDLY 						!!!!
// !!!! (You can use a mix of both: use configuration for stock/animation sounds and	!!!!
// !!!! the SDK only for your special events sounds; you'll save a lot of time)			!!!!
// !!!!										       		!!!!
// !!!! See first "Doc/OrbiterSound Documentation".						!!!!
// !!!! Then "Sound/_CustomVesselSounds" folder	(OrbiterSoundSDKDemo.cfg)			!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// INFO:
// -----
//
// This SDK allows loading and playing sounds for vessels. Sounds are loaded and played 
// throught OrbiterSound.dll so you must specify in your doc that the player must have 
// OrbiterSound installed.
// Several test are done internally to avoid problems if either OrbiterSound isn't loaded, 
// the WAV doesn't exist or you give a wrong parameter; in this either case, no sound is 
// played and orbiter doesn't crash.
// You don't need to bother destroying sounds; they are destroyed automatically if:
//   - You reload a WAV in the same slot (WavNumber)
//   - User exits Orbiter
//
// NEW FEATURES OF 4.0
// ------------------
// - You can load and play 3D WAV.
// - You can replace new default sounds (wind, roll ,radio clearance, dock, GPWS etc); see 
//   ReplaceStockSound()'s parameters.
// - YOU CAN DESIGN VESSEL SOUNDS & OPTIONS BY CONFIGURATION FILE - This can save you a lot 
//   of time and add flexibility; see 'read me' in "Sound/_CustomVesselsSounds"
//
// NEW FEATURE OF 3.5
// ------------------
// - Can load dynamically another sound to one slot and play it immediattely. 
//   (ie: "RequestLoadVesselWave" than immediattely "PlayVesselWave" DO NOT DO THAT EACH 
//	  FRAME !!!)
// - Reentry sound reworked
// - Added a "GetUserOrbiterSoundVersion()" which will tell you if orbiter 4.0
//   (or previous) is installed in user's orbiter (see below for usage)
// - MFD Sound SDK so MFDs can also play sound.
//
//
// Usage example:
// --------------
// SEE 'Sound/VESSELSOUND_SDK/ShuttlePB_project' example. It's a fully functional
// and well commented example!
//
//
// Functions available (see below in the header how to use each function):
// -------------------
// int  ConnectToOrbiterSoundDLL		// To do before any other functions
// float GetUserOrbiterSoundVersion
// BOOL IsOrbiterSound3D				- NEW 4.0
// BOOL SetMyDefaultWaveDirectory		- NEW 4.0
// BOOL RequestLoad3DWaveMono			- NEW 4.0
// BOOL RequestLoad3DWaveStereo			- NEW 4.0		
// BOOL Set3dWaveParameters				- NEW 4.0
// BOOL RequestLoadVesselWave
// BOOL PlayVesselWave
// BOOL StopVesselWave
// BOOL IsPlaying
// BOOL PlayVesselRadioExclusiveWave
// BOOL IsRadioPlaying
// BOOL ReplaceStockSound				- 4.0 Please read warning in function's header below!
// BOOL SoundOptionOnOff				- 4.0 Please read warning in function's header below!
// BOOL SetRadioFrequency
//
// IMPORTANT NOTE:
// ---------------
//
// Sound Folder!!!  Keep it clear for users
// ------------------------------------------
// You must put your sounds in a "Sound//_CustomVesselsSounds/[YourFolder]" so users 
// will not end with dozen of sound folders throwed everywhere in Orbiter's directory. 
// This new folder '_CustomVesselsSounds' introduced in 4.0 version will also help to 
// clean up the main "Sound" directory. SEE HELPER FUNCTION "SetMyDefaultWaveDirectory" 
// below.
//
//
// "Sliding" sound
// --------------
// If you want to make sliding sound (smoothly change frequency or volume) you must call
// the play function each game loop with the LOOP parameter. So you can make sounds that 
// has 'fade in', 'fade out' or 'slide in' frequency. In this case you must set them to
// "LOOP" anyway - ! DO NOT CALL A SOUND EACH FRAME WITH "NOLOOP" AS PARAMETER !
//
// Wave Format
// -----------
// OrbiterSound plays by default at 22050 hz for good performance, so it's useless
// to load sounds that have higher frequency. 
// Sound must be WAV 16 bits PCM format (uncompressed)
//
// Linux/Port Safe Header
//
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
// ORBITERSOUND 4.0 (3D) SDK
// Linux/Port Safe Header
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef ORBITERSOUNDSDK40_H
#define ORBITERSOUNDSDK40_H

// -- LINUX STUB FOR ORBITERSOUND --
#include "Orbitersdk.h"

// Define types used in the main code to prevent errors
typedef int BOOL;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// Define Constants expected by parser
#define SI_MOVE 1
#define SI_cockpit 2
#define SI_mech 3
#define DYNAMIC_LOAD_VER 1
#define STATIC_LOAD_VER 2
#define BOTH 0
#define RADIO_ONLY 1
#define SOUND_ONLY 2
#define LOOP 1
#define NOLOOP 0
#define LOG_IN_ORBITER_LOG 1
#define DISPLAY_ON_SCREEN 2

// Dummy Class
class OrbiterSoundSDK40 {
public:
    OrbiterSoundSDK40(VESSEL *v) {}
    ~OrbiterSoundSDK40() {}
};

// Dummy Functions (All return success/noop)
inline int ConnectToOrbiterSound(OBJHANDLE h, int m) { return 0; }
inline bool RequestLoadVesselWave(int i, int w, char* f, int t) { return true; }
inline bool PlayVesselWave(int i, int w, int l=0, int v=255, int f=0) { return true; }
inline bool StopVesselWave(int i, int w) { return true; }
inline bool IsPlaying(int i, int w) { return false; }
inline bool SetVesselWaveVolume(int i, int w, int v) { return true; }
inline bool SetVesselWaveFrequency(int i, int w, int f) { return true; }
inline bool SoundOptionOnOff(int i, int o, bool s) { return true; }
inline bool PlayVesselRadioExclusiveWave(int i, int w, int v=255) { return true; }
inline bool IsRadioPlaying() { return false; }
inline bool SetRadioFrequency(char* f) { return true; }
inline bool ReplaceStockSound(int i, char* c, int w) { return true; }

// 3D Sound Stubs (The compiler was complaining about these missing)
typedef struct { float fMin; float fMax; } OS3DCONE; // Dummy struct
typedef int EXTENDEDPLAY; 
inline bool RequestLoad3DWaveMono(int i, int w, char* n, EXTENDEDPLAY e, VECTOR3* p) { return true; }
inline bool RequestLoad3DWaveStereo(int i, int w, char* n, EXTENDEDPLAY e, VECTOR3* l, VECTOR3* r) { return true; }
inline bool Set3dWaveParameters(int i, int w, VECTOR3* l=0, VECTOR3* r=0, float* min=0, float* max=0, OS3DCONE* c=0) { return true; }

#endif
