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
// BOOL ReplaceStockSound				- 4.0 Please read warning in function
's header below!
// BOOL SoundOptionOnOff				- 4.0 Please read warning in function
's header below!
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

#ifndef __ORBITERSOUND40_SDK
#define __ORBITERSOUND40_SDK

// Ensure standard Orbiter types (VECTOR3, DWORD, BOOL) are defined
#include "orbitersdk.h" 

// NOTE: The original #pragma comment(lib...) has been removed for cross-platform compatibility.
// Linking must be handled by the build system (CMake/Makefile).

////////////////////////////////////////////
// KEYWORDS
////////////////////////////////////////////

//These are the keywords used by SoundOptionOnOff()
#define PLAYCOUNTDOWNWHENTAKEOFF		1
#define PLAYWHENATTITUDEMODECHANGE		3
#define PLAYGPWS						4
#define PLAYMAINTHRUST					5
#define PLAYHOVERTHRUST					6
#define PLAYATTITUDETHRUST				7
#define PLAYDOCKINGSOUND				8
#define PLAYRADARBIP					9
#define PLAYWINDAIRSPEED				10
#define PLAYDOCKLANDCLEARANCE			11
#define PLAYLANDINGANDGROUNDSOUND		12
#define PLAYCABINAIRCONDITIONING		13
#define PLAYCABINRANDOMAMBIANCE			14
#define PLAYWINDAMBIANCEWHENLANDED		15
#define PLAYRADIOATC					16
#define DISPLAYTIMER					17
#define DISABLEAUTOPILOTWHENTIMEWARP 	18
#define ALLOWRADIOBLACKOUT				19
#define MUTEORBITERSOUND				20
#define PLAYRETROTHRUST					21
#define PLAYUSERTHRUST					22
#define PLAYWINDCOCKPITOPEN				23
#define PLAYREENTRYAIRSPEED				24

//These are the keywords used by ReplaceStockSound()
#define REPLACE_MAIN_THRUST					10
#define REPLACE_HOVER_THRUST				11
#define REPLACE_RCS_THRUST_ATTACK			12
#define REPLACE_RCS_THRUST_SUSTAIN			13
#define REPLACE_AIR_CONDITIONNING	 		14
#define REPLACE_COCKPIT_AMBIENCE_1	 		15
#define REPLACE_COCKPIT_AMBIENCE_2			16
#define REPLACE_COCKPIT_AMBIENCE_3			17
#define REPLACE_COCKPIT_AMBIENCE_4			18
#define REPLACE_COCKPIT_AMBIENCE_5	 		19
#define REPLACE_COCKPIT_AMBIENCE_6			20
#define REPLACE_COCKPIT_AMBIENCE_7			21
#define REPLACE_COCKPIT_AMBIENCE_8			22
#define REPLACE_COCKPIT_AMBIENCE_9			23
#define REPLACE_MODE_ROTATION				24
#define REPLACE_MODE_TRANSLATION			25
#define REPLACE_MODE_ATTITUDEOFF			26
#define REPLACE_WIND_AIRSPEED				27
#define REPLACE_REENTRY_AIRSPEED			28
#define REPLACE_LAND_TOUCHDOWN				29
#define REPLACE_GROUND_ROLL					30
#define REPLACE_WHEELBRAKE		   			31
#define REPLACE_CRASH_SOUND					32
#define REPLACE_DOCKING						33
#define REPLACE_UNDOCKING					34
#define REPLACE_RADIOLANDCLEARANCE	 		35
#define REPLACE_DOCKING_RADIO				36
#define REPLACE_UNDOCKING_RADIO		 		37
#define REPLACE_RADAR_APPROACH		 		38
#define REPLACE_RADAR_CLOSE					39
#define REPLACE_RETRO_THRUST		 		40
#define REPLACE_USER_THRUST					41
#define REPLACE_COUNTDOWN_WHENTAKEOFF 		42
#define REPLACEALLGPWSSOUND_README_FOR_USE	43

// This is the structure used by Set3dWaveParameters
typedef struct OS3DCONE {
    DWORD dwInsideConeAngle;
    DWORD dwOutsideConeAngle;
    VECTOR3 vConeOrientation;
    double lConeOutsideVolume;
} OS3DCONE;

// These are the keywords used by PlayVesselWave
#define NOLOOP	0
#define LOOP	1

// These are the keywords used by "RequestLoadWave***()"
typedef enum{
		DEFAULT,
		INTERNAL_ONLY,
		BOTHVIEW_FADED_CLOSE,
		BOTHVIEW_FADED_MEDIUM,
		BOTHVIEW_FADED_FAR,
		EXTERNAL_ONLY_FADED_CLOSE,
		EXTERNAL_ONLY_FADED_MEDIUM,
		EXTERNAL_ONLY_FADED_FAR,
		RADIO_SOUND,
}EXTENDEDPLAY;


///////////////////////////////////////////////////
// FUNCTION DECLARATIONS
///////////////////////////////////////////////////

// Connection
int  ConnectToOrbiterSoundDLL(OBJHANDLE Obj);

// Utility
BOOL SetMyDefaultWaveDirectory(char *MySoundDirectory);
float GetUserOrbiterSoundVersion(void);
BOOL IsOrbiterSound3D(void);

// Loading (3D)
BOOL RequestLoad3DWaveMono(int iMyID,int iWavNumber,char* cSoundName,EXTENDEDPLAY extended,VECTOR3* v3Position);
BOOL RequestLoad3DWaveStereo(int iMyID,int iWavNumber,char* cSoundName,EXTENDEDPLAY extended,VECTOR3* v3LeftPos,VECTOR3* v3RightPos);
BOOL Set3dWaveParameters(int iMyID,int WavNumber,VECTOR3 *vLeftPos=NULL,VECTOR3 *vRightPos=NULL,float *flMinDistance=NULL,float *flMaxDistance=NULL,OS3DCONE *soundConeParm=NULL);

// Loading (2D)
BOOL RequestLoadVesselWave(int MyID,int WavNumber,char* SoundName,EXTENDEDPLAY extended);

// Playback
BOOL PlayVesselWave(int MyID,int WavNumber,int Loop=NOLOOP,int Volume=255,int Frequency=0);
BOOL StopVesselWave(int MyID,int WavNumber);
BOOL IsPlaying(int MyID,int WavNumber);
BOOL PlayVesselRadioExclusiveWave(int MyID,int WavNumber,int Volume=255);

// Customization
BOOL ReplaceStockSound(int iMyId,char *cMyCustomWavName,int iWhichSoundToReplace);
BOOL SoundOptionOnOff(int MyID,int Option, BOOL Status=TRUE);
BOOL IsRadioPlaying(void);
BOOL SetRadioFrequency(char* Frequency);

#endif //__ORBITERSOUND40_SDK
