//////////////////////////////////////////////////////////////////////////////////////////
// ORBITERSOUND 4.0 (3D) SDK
// SDK to add sound to Orbiter's VESSEL (c) Daniel Polli aka DanSteph
// For ORBITER ver:2010 with patch P1
//////////////////////////////////////////////////////////////////////////////////////////
//
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!! IMPORTANT INFORMATION							       							!!!!
// !!!!																					!!!!
// !!!! ORBITERSOUND SDK IS VERY USEFULL TO PLAY SPECIAL EVENTS	SOUNDS BUT IF YOU		!!!!
// !!!! WANT JUST TO REPLACE THE 43 DEFAULT SOUNDS, CHANGE THE 22 OPTIONS AND/OR		!!!!
// !!!! PLAY ANIMATION SOUNDS, THE NEW CONFIGURATION FILE INTRODUCED IN 4.0 VERSION		!!!! 
// !!!! IS MUCH MORE PRACTICALL AND USER-FRIENDLY 										!!!!
// !!!! (You can use a mix of both: use configuration for stock/animation sounds and	!!!!
// !!!! the SDK only for your special events sounds; you'll save a lot of time)			!!!!
// !!!!										       										!!!!
// !!!! See first "Doc/OrbiterSound Documentation".										!!!!
// !!!! Then "Sound/_CustomVesselSounds" folder	(OrbiterSoundSDKDemo.cfg)				!!!!
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
////////////////////////////////////////////////////////////////////////////////
#ifndef __ORBITERSOUND40_SDK
#define __ORBITERSOUND40_SDK
// those are some definitions; watch below for usefull instructions for you
#pragma comment(lib,"OrbiterSoundSDK40.lib")


////////////////////////////////////////////
// KEYWORDS
////////////////////////////////////////////

//These are the keywords used by SoundOptionOnOff()
#define PLAYCOUNTDOWNWHENTAKEOFF		1	// The countdown when you take-off
#define PLAYWHENATTITUDEMODECHANGE		3	// Play "rotation" "linear" sound 
#define PLAYGPWS						4	// The GPWS sound
#define PLAYMAINTHRUST					5	// Main thrust sound
#define PLAYHOVERTHRUST					6	// The hover thrust sound
#define PLAYATTITUDETHRUST				7	// The attitude thrust sound
#define PLAYDOCKINGSOUND				8	// The docking sound and radio
#define PLAYRADARBIP					9	// The radar 'bip' when near another vessel
#define PLAYWINDAIRSPEED				10	// The wind airspeed when atmosphere
#define PLAYDOCKLANDCLEARANCE			11	// The landing clearance sound.
#define PLAYLANDINGANDGROUNDSOUND		12	// Rolling, landing, speedbrake, crash sound
#define PLAYCABINAIRCONDITIONING		13	// Play the air conditionning sound
#define PLAYCABINRANDOMAMBIANCE			14	// Play the random pump and rumble sound
#define PLAYWINDAMBIANCEWHENLANDED		15	// Play the wind sound when landed
#define PLAYRADIOATC					16	// Play the atc radio sound 
#define DISPLAYTIMER					17	// Display the timer text at the bottom of the screen
#define DISABLEAUTOPILOTWHENTIMEWARP 	18	// The auto-disable of pilot if you time warp
#define ALLOWRADIOBLACKOUT				19	// Allows the radio blackout when reentry
#define MUTEORBITERSOUND				20	// Stop all sound (but doesn't forbid further play)
#define PLAYRETROTHRUST					21	// --- NEW OrbiterSound 4.0 
#define PLAYUSERTHRUST					22	// --- NEW OrbiterSound 4.0, the aux or user group thrust. (eg: atlantis SRB)
#define PLAYWINDCOCKPITOPEN				23	// --- NEW OrbiterSound 4.0 if TRUE play planet wind and base sound even in internal view.
#define PLAYREENTRYAIRSPEED				24	// --- NEW OrbiterSound 4.0 the reentry sound, see demo scenario "reentry sound"

//These are the keywords used by ReplaceStockSound()
#define REPLACE_MAIN_THRUST					10	// Replace the main thrust sound
#define REPLACE_HOVER_THRUST				11	// Replace the hover sound
#define REPLACE_RCS_THRUST_ATTACK			12	// Replace the rcs attack thrust
#define REPLACE_RCS_THRUST_SUSTAIN			13	// Replace the rcs sustain thrust
#define REPLACE_AIR_CONDITIONNING	 		14	// Replace the air conditionning sound
#define REPLACE_COCKPIT_AMBIENCE_1	 		15	// Replace the wave 1 used for cockpit ambience
#define REPLACE_COCKPIT_AMBIENCE_2			16	// Replace the wave 2 used for cockpit ambience
#define REPLACE_COCKPIT_AMBIENCE_3			17	// Replace the wave 3 used for cockpit ambience
#define REPLACE_COCKPIT_AMBIENCE_4			18	// Replace the wave 4 used for cockpit ambience
#define REPLACE_COCKPIT_AMBIENCE_5	 		19	// Replace the wave 5 used for cockpit ambience
#define REPLACE_COCKPIT_AMBIENCE_6			20	// Replace the wave 6 used for cockpit ambience
#define REPLACE_COCKPIT_AMBIENCE_7			21	// Replace the wave 7 used for cockpit ambience
#define REPLACE_COCKPIT_AMBIENCE_8			22	// Replace the wave 8 used for cockpit ambience
#define REPLACE_COCKPIT_AMBIENCE_9			23	// Replace the wave 9 used for cockpit ambience
#define REPLACE_MODE_ROTATION				24	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_MODE_TRANSLATION			25	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_MODE_ATTITUDEOFF			26	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_WIND_AIRSPEED				27	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_REENTRY_AIRSPEED			28	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_LAND_TOUCHDOWN				29	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_GROUND_ROLL					30	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_WHEELBRAKE		   			31	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_CRASH_SOUND					32	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_DOCKING						33	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_UNDOCKING					34	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_RADIOLANDCLEARANCE	 		35	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_DOCKING_RADIO				36	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_UNDOCKING_RADIO		 		37	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_RADAR_APPROACH		 		38	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_RADAR_CLOSE					39	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_RETRO_THRUST		 		40	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_USER_THRUST					41	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACE_COUNTDOWN_WHENTAKEOFF 		42	// --- NEW OrbiterSound 4.0 Self explanatory
#define REPLACEALLGPWSSOUND_README_FOR_USE	43	// see below warning!
//
// WARNING for "REPLACEALLGPWSSOUND_README_FOR_USE"
// Set parm to "yes" instead of WAV filename. You must provide all the 12 GPWS sound in your 
// vessel's config folder. To find this folder's name without headache, simply watch 
// OrbiterSound_log.txt after trying your vessel; you'll have a load error with the full path.
// Example: 
// 'unable to load 3D wave: Sound\_CustomVesselsSounds\ShuttlePBSDKDemo\-10.wav'

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
// ConnectToOrbiterSoundDLL
// -------------------------
// This must be called previous to any other OrbiterSound call
// You must call this in your "clbkPostCreation" (new version of "ovcPostCreation" which is now obsolete)
// You must keep in your class the ID it returns and provide it to all other functions.
//
// PARAMETER:
// ----------
// Obj    : The Vessel handle using this : MySoundID=ConnectToOrbiterSoundDLL(GetHandle());
// return : An ID that you must keep and provide to others function (or -1 if it fails to connect).
///////////////////////////////////////////////////
int ConnectToOrbiterSoundDLL(OBJHANDLE Obj);


///////////////////////////////////////////////////
// SetMyDefaultWaveDirectory - optional path utility
// -------------------------
// This one allows you to define the default path that will be used by all other
// load functions. 
//
// Instead of defining the complete path at each sound function:
// Example: 'RequestLoadWave(MyID,1,"Sound\\_CustomVesselsSounds\\ShuttlePBSDKDemo\\myFirst.wav"'
// You define it only >>one<< time just after ConnectToOrbiterSound():
// Example: 'SetMyDefaultWaveDirectory("Sound\\_CustomVesselsSounds\\ShuttlePBSDKDemo\\");'
// and then you can forget it in all other functions, giving only the wave's filename:
// Example: 'RequestLoadWave(MyID,1,"myFirst.wav"); RequestLoadWave(MyID,2,"mySecond.wav");'
//
// If you don't call this function, you should still give the complete path to all other functions.
//
// RETURN
// ------
// FALSE: If the path is wrong (it doesn't start by "Sound\\_CustomVesselsSounds\\" for example)
//	  In this case the other functions will not use this path (load sound, replace waves etc. etc.).
// TRUE:  The path is valid, all other functions will use it.
//
// EXAMPLES OF VALID CALLING:
// --------------------------
// SetMyDefaultWaveDirectory("Sound\\_CustomVesselsSounds\\Atlantis\\);
// SetMyDefaultWaveDirectory("Sound\\_CustomVesselsSounds\\OdysseyProject\\); 
// SetMyDefaultWaveDirectory("Sound\\_CustomVesselsSounds\\ShuttleAReworked\\);
//
// Example of INVALID CALLING:
//            -------
// SetMyDefaultWaveDirectory("Atlantis");
// SetMyDefaultWaveDirectory("C:\\MyFolder\\Atlantis\\");
// SetMyDefaultWaveDirectory("Sound\\Atlantis\\");
///////////////////////////////////////////////////
BOOL SetMyDefaultWaveDirectory(char *MySoundDirectory);

///////////////////////////////////////////////////
// GetUserOrbiterSoundVersion()
// -------------------------
// Returns exact version of orbitersound installed in >>>user's directory<<<< 
// Version number returned by this function is in float example: 
// returned value 3.0f = version 3.0
// returned value 3.5f = version 3.5
// returned value 4.0f = version 4.0
// For previous versions than 3.0, it returns always 0, as the version is 
// so outdated that you might warn the user anyway.
// This may be usefull if you want special new features
// only existing in 3.5 or 4.0
///////////////////////////////////////////////////
float GetUserOrbiterSoundVersion(void);

///////////////////////////////////////////////////
// IsOrbiterSound3D
// -------------------------
// Return TRUE if the user's OrbiterSound plays in 3D and FALSE if sound 
// plays in 2D only (User's settings or previous versions of OrbiterSound).
//
// This information might be very useful if you use advanced 
// 3D effects as sound cone. As they don't work in 2D, sounds that should play 
// in different areas would overlapp in a bad mix. You can hear this bad effect 
// with the scenario: "_OrbiterSound 4.0 demo/SDK C++ and config demo.scn"
// Deactivate 3D with SoundConfig.exe and launch this scenario.
// 
// !!!!! WARNING !!!!! !!!!! WARNING !!!!! !!!!! WARNING !!!!!
// -----------------------------------------------------------
// All vessels are initialised BEFORE OrbiterSound initialise DirectSound.
// If you call this in "clbkPostCreation" the function will always return
// FALSE. A good place to call it is when you play the sound.
// 
// GOOD EXAMPLE:
// -------------
// In clbkPresStep:
// if(IsOrbiterSound3D()==TRUE)
// {
//		PlayVesselWave(MyID,MYSOUND3DWITHCONE);
// }
//
// NON-WORKING EXAMPLE:
// --------------------
// in clbkPostCreation:
// MySoundID=ConnectToOrbiterSoundDLL(GetHandle());
// if(IsOrbiterSound3D()==TRUE) // This will never return true, in clbkPostCreation DirectSound is not up. 
/////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IsOrbiterSound3D(void);

///////////////////////////////////////////////////////////////////////////////////////
// RequestLoad3DWaveMono
// ----------------------
// Load a sound that will play in 3D at the given position
//
// NOTE: If 3D is not available in user's configuration, this function internally reverts
// to 2D "RequestLoadVesselWave"
//
// PARAMETERS:
// -----------
// MyID       : The number returned by ConnectToOrbiterSoundDLL
// WavNumber  : from 1 to 60 or #defined name from 1 to 60 
//              example: "#define APPLAUSE 1" "#define EXPLOSION 2"
// SoundName  : your WAV's name and path
//              example: "Sound\\_CustomVesselSound\\atlantis\\separate.wav"
// Extended   : This parameter will define how your sound will play,
//              in 'cockpit only', in 'external view', etc etc. (see below)
// v3Position : 3D Position in local vessel coordinate
//
// This is the keyword used for the extended parameter:
//  INTERNAL_ONLY			Sound will play in internal view only (usefull for internal cockpit sound)
//  BOTHVIEW_FADED_CLOSE	Sound will play as the RCS thrust, faded by close distance and pressure
//							in external view and full volume in internal view
//  BOTHVIEW_FADED_MEDIUM	Sound will play as the airspeed wind, faded by medium distance and pressure
//							in external view and full volume in internal view
//  BOTHVIEW_FADED_FAR		Sound will play as the main thrust,faded by far distance and pressure
//							in external view and full volume in internal view
//
// NOTE: - Stereo sound will be converted to mono in 3D (and stay stereo in 2D)
//		   (convenient for most uses as 3D is natively in mono)
//		 - If the user disables the 3D OR if it has only OrbiterSound 3.5,
//		   your sound will load & play, but it will in 2D.
//       - Distance for fading is roughly set by the extended parameter.
//		   The distances are as follows: 
//			min/max in meters where - min is the distance at which the sound is maximum
//									- max the distance at which the sound stops
//			INTERNAL_ONLY			5	100
//			BOTHVIEW_FADED_CLOSE	10	1000
//			BOTHVIEW_FADED_MEDIUM	50	8000
//			BOTHVIEW_FADED_FAR		80	18000
//
// RETURN
// ------
// FALSE if request not taken in account (wrong parameters, OrbiterSound disabled or wave failed to load).
// If a wave wasn't found, a log will be created in Orbiter's directory with the name of the 
// missing wave
//
// EXAMPLES OF CALLING: (Using the new 'SetMyDefaultWaveDirectory()' previously)
// --------------------
// RequestLoad3DWaveMono(MyID,APPLAUSE,"a.wav",INTERNAL_ONLY,&_V(-1,2,10));			// internal sound
// RequestLoad3DWaveMono(MyID,APPLAUSE,"b.wav",BOTHVIEW_FADED_FAR,&_V(-1,2,-10));	// internal/external sound
// RequestLoad3DWaveMono(MyID,APPLAUSE,"c.wav",BOTHVIEW_FADED_CLOSE,&_V(-5,5,-10));	// internal/external sound
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL RequestLoad3DWaveMono(int iMyID,int iWavNumber,char* cSoundName,EXTENDEDPLAY extended,VECTOR3* v3Position);

///////////////////////////////////////////////////////////////////////////////////////
// RequestLoad3DWaveStereo
// ----------------------
// Load a stereo sound that will play in two separate channels in 3D
//
// Same as "RequestLoad3DWaveMono" except that stereo sound will be splitted in two channels
// that can be placed separately in 3D.
// This is very usefull for sounds that needs dynamic or are emitted by an area 
// (for example air conditionning, multiples engines, hovers)
//
// NOTE: If 3D is not available in user's configuration this function internally
// reverts to 2D "RequestLoadVesselWave"
//
// PARAMETERS:
// -----------
// MyID      : The number returned by ConnectToOrbiterSoundDLL
// WavNumber : From 1 to 60  or #defined name from 1 to 60 (example: "#define APPLAUSE 1" "#define EXPLOSION 2")
// v3LeftPos : Left Position in local vessel coordinates
// v3RightPos: Right Position in local vessel coordinates
//
// NOTE: - Mono WAV will stay mono, this function doesn't convert them in stereo,
//			nor does it create two channels with mono.
//       - This function uses in fact two 3D channels (As 3D is natively in mono)
//
// RETURN
// ------
// FALSE if request not taken in account (wrong parameters, OrbiterSound 
// disabled or wave failed to load). If a wave wasn't found, a log will be
// created in Orbiter's directory with the name of the missing wave
//
// EXAMPLES OF CALLING: (Using the new 'SetMyDefaultWaveDirectory()' previously)
// --------------------
//  RequestLoad3DWaveStereo(MyID,APPLAUSE,"a.wav",INTERNAL_ONLY,&_V(2,0,6),&_V(-2,0,6));		 // internal sound (example aircond)
//  RequestLoad3DWaveStereo(MyID,APPLAUSE,"b.wav",BOTHVIEW_FADED_FAR,&_V(-3,2,-10)&_V(3,2,-10)); // internal/external example thruster
///////////////////////////////////////////////////////////////////////////////////////
BOOL RequestLoad3DWaveStereo(int iMyID,int iWavNumber,char* cSoundName,EXTENDEDPLAY extended,VECTOR3* v3LeftPos,VECTOR3* v3RightPos);

///////////////////////////////////////////////////////////////////////////////////////
// Set3dWaveParameters
// ----------------------
// Set all 3D parameters for a previously 3D loaded sound.
//
// This gives you the full control of a sound's 3D parameters, you'll rarely need it but if you do,
// here it is. Internally this function calls the IDirectSound3DBuffer8::SetAllParameters
// So you can refer to DirectSound's documentation and google about this function and it's parameters.
//
// PARAMETERS:
// -----------
// MyID					: The number returned by ConnectToOrbiterSoundDLL.
// WavNumber			: From 1 to 60 or #defined name from 1 to 60 (example: "#define APPLAUSE 1" "#define EXPLOSION 2")
// vLeftPos vRightPos	: Position of sound in LOCAL VESSEL COORDINATES. For mono sound, leave vRightPos=NULL.
// flMinDistance		: Min distance with maximum volume.
// flMaxDistance		: Distance at which the sound is minimum and stop.
// soundConeParm		: The 3D cone parameters (Try scenario "_OrbiterSound demo/3d sound's cone projection" to see this effect)
//
// RETURN
// ------
// FALSE if request not taken in account (wrong parameters, OrbiterSound disabled or not in 3D).
//
// EXAMPLES OF CALLING:
// --------------------
// Set3dWavParameters(MyID,APPLAUSE,&_V(2,0,10));						  Reposition a mono WAV
// Set3dWavParameters(MyID,APPLAUSE,&_V(2,0,10),&_V(-2,0,10));			  Reposition a stereo WAV
// Set3dWavParameters(MyID,APPLAUSE,NULL,NULL,&fMyValueMin,&fMyValueMax); Change only the Min/max value 
//																		  (Pos left to NULL, other takes default value NULL)
//
//	OS3DCONE MyCone={0};				// init all struct value to zero
//	MyCone.dwInsideConeAngle=30;		// the cone angle where the sound is at max (255)
//	MyCone.dwOutsideConeAngle=360;		// the cone angle where the sound is lower (progressive)
//	MyCone.vConeOrientation=_V(-1,0,0);	// normalized vector, orientation of cone, here 'left'
//	MyCone.lConeOutsideVolume=230;		// outside cone volume (max 255); usually a 230, 240 value is more realistic for engine.
//	Set3dWaveParameters(MyID,MYLEFTSOUND,NULL,NULL,NULL,NULL,&MyCone); 
//
// Set3dWavParameters(MyID,APPLAUSE,&_V(2,0,10),&_V(-2,0,10),&fMyValueMin,&fMyValueMax,&MyCone); //Set all parameters above in one call
////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Set3dWaveParameters(int iMyID,int WavNumber,VECTOR3 *vLeftPos=NULL,VECTOR3 *vRightPos=NULL,float *flMinDistance=NULL,float *flMaxDistance=NULL,OS3DCONE *soundConeParm=NULL);

///////////////////////////////////////////////////////////////////////////////////////
// RequestLoadVesselWave
// ----------------------
// Request to Load one 2D sound in slot "WavNumber"
//
// PARAMETERS:
// -----------
// MyID      : The number returned by ConnectToOrbiterSoundDLL
// WavNumber : From 1 to 60 or #defined name from 1 to 60 (example: "#define APPLAUSE 1" "#define EXPLOSION 2")
// SoundName : Your WAV's name and path. Example: "Sound\\atlantisMartin\\separate.wav"
// Extended  : This parameter will define how your sound will play, in 'cockpit only', in 'external view', etc etc.
//
// These are the keywords used for the extended parameter:
//		DEFAULT						DEFAULT NOT RECOMMENDED! sound will always play in all views and will not
//	 								 fade by distance, pressure or view change
//  	INTERNAL_ONLY				Sound will play in internal view only (usefull for internal cockpit sound)
//  	BOTHVIEW_FADED_CLOSE		Sound will play as the RCS thrust, faded by close distance and pressure in external view
//									 and full volume in internal view
//		BOTHVIEW_FADED_MEDIUM		Sound will play as the airspeed wind, faded by medium distance and pressure
//									 in external view and full volume in internal view
//		BOTHVIEW_FADED_FAR			Sound will play as the main thrust, faded by far distance and pressure in external view
//									 and full volume in internal view
//		EXTERNAL_ONLY_FADED_CLOSE   Sound will play in external view only (faded with close distance and pressure)
//  	EXTERNAL_ONLY_FADED_MEDIUM  Sound will play in external view only (faded with medium distance and pressure)
//  	EXTERNAL_ONLY_FADED_FAR     Sound will play in external view only (faded with far distance and pressure)
//  	RADIO_SOUND	   				Sound is a radio sound that will be played by PlayVesselRadioExclusiveWave3()
//
//
// RETURN
// ------
// FALSE if request not taken in account (wrong parameters, OrbiterSound disabled or wave failed to load).
// If a wave wasn't found, a log will be created in Orbiter's directory with the name of the 
// missing wave
//
// NOTE:
// -----
// You can submit a new loading at same "slot" anytime during simulation; the new
// one will erase the old one. Take care not to ask a loading every frame, otherwise
// the framerate will suffer. Take care that if the user wants to save it's situation
// and come back later you can define some flags and record that time, so when the player
// comes back you reload the appropriate WAV for the corresponding time.
//
////////////////////////////////////////////////////////////////////////////////////////
BOOL RequestLoadVesselWave(int MyID,int WavNumber,char* SoundName,EXTENDEDPLAY extended);

///////////////////////////////////////////////
// PlayVesselWave
// --------------
// Play a wave loaded before (2D or 3D)
//
// PARAMETERS:
// -----------
// MyID      : The number returned by ConnectToOrbiterSoundDLL
// WavNumber : From 1 to 60 or #defined name from 1 to 60 (example: "#define APPLAUSE 1" "#define EXPLOSION 2")
// Loop	     : NOLOOP= sound not looped LOOP= sound looped.
// Volume    : From 0 to 255
// Frequency : From 3000 to 44100 (OrbiterSound's plays anyway at 22050 so your sound will be resampled at run time)
//
// IMPORTANT NOTICE: FOR >>>CONSTANT<<< SOUND (LOOPED) IN >>>EXTERNAL VIEW OR BOTHVIEW<<< YOU MUST CALL THE SOUND 
// EACH FRAME OTHERWISE IT WILL NOT FADE PROPERLY BY DISTANCE AND PRESSURE. SEE "SHUTLEPB" EXAMPLE.
//
// RETURN
// ------
// FALSE if wave not played/loaded OR the connection to orbitersound's dll failed.
//
// EXAMPLES OF CALLING:
// --------------------
//  PlayVesselWave(MyID,APPLAUSE);					// play with no loop full volume and WAV's default frequency
//  PlayVesselWave(MyID,APPLAUSE,LOOP,210);			// play a looped sound with specified volume and WAV's default frequency
//  PlayVesselWave(MyID,APPLAUSE,NOLOOP,128,22050); // play no loop with specified volume and frequency
////////////////////////////////////////////////////////////////////////////
BOOL PlayVesselWave(int MyID,int WavNumber,int Loop=NOLOOP,int Volume=255,int Frequency=NULL);

///////////////////////////////////////////////////////////////
// StopVesselWave
// --------------
// Stop a wave that plays
//
// PARAMETERS:
// -----------
// MyID      : The number returned by ConnectToOrbiterSoundDLL
// WavNumber : from 1 to 60 or #defined name from 1 to 60 (example: "#define APPLAUSE 1" "#define EXPLOSION 2")
//
// RETURN
// ------
// FALSE if wave not played/loaded OR the connection to orbitersound's dll failed.
//
// EXAMPLES OF CALLING:
// --------------------
// StopVesselWave(MyID,APPLAUSE);		
///////////////////////////////////////////////
BOOL StopVesselWave(int MyID,int WavNumber);

///////////////////////////////////////////////
// IsPlaying
// ---------
// Return TRUE if the sound specified in WaveNumber is currently playing otherwise it returns FALSE.
//
// PARAMETERS:
// -----------
// MyID	     : The number returned by ConnectToOrbiterSoundDLL
// WavNumber : From 1 to 60 or #defined name from 1 to 60 (example: "#define APPLAUSE 1" "#define EXPLOSION 2")
//
// RETURN
// ------
// FALSE if wave does not play OR the connection to orbitersound's dll failed.
//
// This is useful to avoid playing two sounds in same time
//
// EXAMPLES OF CALLING:
// --------------------
//  if(IsPlaying(MyID,APPLAUSE)==FALSE)
//  {
//     PlayVesselWave(MyID,EXPLOSION);		
//  }
///////////////////////////////////////////////
BOOL IsPlaying(int MyID,int WavNumber);

///////////////////////////////////////////////
// PlayVesselRadioExclusiveWave
// ----------------------------
// This function is used to play your radio sound
//
// What it does:
// 1- STOP the ATC RADIO sound that OrbiterSound is currently playing
// 2- FORBID OrbiterSound to play radio atc while this sound is playing (auto test).
// 
// As soon as your sound ends, OrbiterSound will continue to play random ATC radio Wave.
// This is perfect to play your radio sound or an important sound without being disturbed by
// OrbiterSound's atc sound.
//
// For parameters see PlayVesselWave
//
// Note:
// It is perhaps better to test with IsRadioPlaying() if an atc WAV is
// currently playing before calling this as the Orbiter's atc sound might
// stop in the middle and that would look strange.
//
///////////////////////////////////////////////
BOOL PlayVesselRadioExclusiveWave(int MyID,int WavNumber,int Volume=255);

///////////////////////////////////////////////////
// ReplaceStockSound
// ------------------------
// This function will instruct to permanently replace one or more of OrbiterSound's 
// stock sound with your own. You might call this just after ConnectToOrbiterSoundDLL 
// to customise your vesel's sounds.
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!! YOU CAN DO THAT NOW BY CONFIGURATION FILE, WHICH IS MAYBE MORE PRACTICALL   !!!!
// !!!! YOU CAN ALSO SET DEFAUT SOUND 3D POSITION IN CONFIGURATION FILES			!!!!
// !!!! SEE Sound/_CustomVesselSounds FOLDER										!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// PARAMETERS:
// -----------
// iMyID				: The number returned by ConnectToOrbiterSoundDLL
// cMyCustomWavName		: Your WAV's name OR "yes" for 'REPLACEALLGPWSSOUND_README_FOR_USE', see below.
// iWhichSoundToReplace	: a keyword as defined below:
//	REPLACE_MAIN_THRUST		 			10	// Replace the main thrust sound
//	REPLACE_HOVER_THRUST				11	// Replace the hover sound
//	REPLACE_RCS_THRUST_ATTACK			12	// Replace the rcs attack thrust
//	REPLACE_RCS_THRUST_SUSTAIN			13	// Replace the rcs sustain thrust
//	REPLACE_AIR_CONDITIONNING			14	// Replace the air conditionning sound
//	REPLACE_COCKPIT_AMBIENCE_1			15	// Replace the wave 1 used for cockpit ambience
//	REPLACE_COCKPIT_AMBIENCE_2			16	// Replace the wave 2 used for cockpit ambience
//	REPLACE_COCKPIT_AMBIENCE_3			17	// Replace the wave 3 used for cockpit ambience
//	REPLACE_COCKPIT_AMBIENCE_4			18	// Replace the wave 4 used for cockpit ambience
//	REPLACE_COCKPIT_AMBIENCE_5			19	// Replace the wave 5 used for cockpit ambience
//	REPLACE_COCKPIT_AMBIENCE_6	 		20	// Replace the wave 6 used for cockpit ambience
//	REPLACE_COCKPIT_AMBIENCE_7			21	// Replace the wave 7 used for cockpit ambience
//	REPLACE_COCKPIT_AMBIENCE_8			22	// Replace the wave 8 used for cockpit ambience
//	REPLACE_COCKPIT_AMBIENCE_9			23	// Replace the wave 9 used for cockpit ambience
//	REPLACE_MODE_ROTATION				24	// --- NEW OrbiterSound 4.0 Self explanatory
//	REPLACE_MODE_TRANSLATION			25	// --- NEW OrbiterSound 4.0 Self explanatory
//	REPLACE_MODE_ATTITUDEOFF			26	// --- NEW OrbiterSound 4.0 Self explanatory
//	REPLACE_WIND_AIRSPEED				27	// --- NEW OrbiterSound 4.0 Self explanatory
//  REPLACE_REENTRY_AIRSPEED			28	// --- NEW OrbiterSound 4.0 Self explanatory
//	REPLACE_LAND_TOUCHDOWN			 	29	// --- NEW OrbiterSound 4.0 Self explanatory
//	REPLACE_GROUND_ROLL		 			30	// --- NEW OrbiterSound 4.0 Self explanatory
//	REPLACE_WHEELBRAKE					31	// --- NEW OrbiterSound 4.0 Self explanatory
//	REPLACE_CRASH_SOUND					32	// --- NEW OrbiterSound 4.0 Self explanatory
//	REPLACE_DOCKING						33	// --- NEW OrbiterSound 4.0 Self explanatory
//	REPLACE_UNDOCKING		 			34	// --- NEW OrbiterSound 4.0 Self explanatory
//	REPLACE_RADIOLANDCLEARANCE	 		35	// --- NEW OrbiterSound 4.0 Self explanatory
//	REPLACE_DOCKING_RADIO		 		36	// --- NEW OrbiterSound 4.0 Self explanatory
//	REPLACE_UNDOCKING_RADIO		 		37	// --- NEW OrbiterSound 4.0 Self explanatory
//	REPLACE_RADAR_APPROACH		 		38	// --- NEW OrbiterSound 4.0 Self explanatory
// 	REPLACE_RADAR_CLOSE					39	// --- NEW OrbiterSound 4.0 Self explanatory
//	REPLACE_RETRO_THRUST				40	// --- NEW OrbiterSound 4.0 Self explanatory
// 	REPLACE_USER_THRUST					41	// --- NEW OrbiterSound 4.0 Self explanatory
// 	REPLACE_COUNTDOWN_WHENTAKEOFF 	 	42	// --- NEW OrbiterSound 4.0 Self explanatory
// 	REPLACEALLGPWSSOUND_README_FOR_USE	43	// See warning below!
//
// WARNING for "REPLACEALLGPWSSOUND_README_FOR_USE":
// Set parm to "yes" instead of WAV filename. You must provide all the 12 GPWS sounds in your 
// vessel's config folder. To find this folder's name without headache, simply watch 
// OrbiterSound_log.txt after trying your vessel; you'll have a load error with the full path.
// Example: 
// 'unable to load 3D wave: Sound\_CustomVesselsSounds\ShuttlePBSDKDemo\-10.wav'
//
//
// RETURN
// ------
// FALSE if request not taken in account (wrong parameters or OrbiterSound disabled).
// If a wave wasn't found, a log will be created in Orbiter's directory with the name of the 
// missing wave.
//
// EXAMPLES OF CALLING:  (Using the new 'SetMyDefaultWaveDirectory()' previously)
// --------------------
// ReplaceStockSound(MyId,"Main.wav",REPLACE_MAIN_THRUST);
// ReplaceStockSound(MyId,"roll.wav",REPLACE_GROUND_ROLL); 
// ReplaceStockSound(MyId,"amb1.wav",REPLACE_COCKPIT_AMBIENCE_1);
//
///////////////////////////////////////////////////
BOOL ReplaceStockSound(int iMyId,char *cMyCustomWavName,int iWhichSoundToReplace);

///////////////////////////////////////////////
// SoundOptionOnOff
// -----------------
// With this function you can put ON/OFF every sound
// in OrbiterSound the same way SOUNDCONFIG.EXE does
// but in a temporary manner and only for your ship. 
// (start soundconfig.exe to test the sound related to each option so you have an idea what it does)
// 
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!! YOU CAN DO THAT NOW BY CONFIGURATION FILE WHICH IS MAYBE MORE PRACTICALL	!!!!
// !!!! YOU CAN ALSO SET DEFAUT SOUND 3D POSITION IN CONFIGURATION FILES			!!!!
// !!!! SEE Sound/_CustomVesselSounds FOLDER										!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// PARAMETERS:
// -----------
// MyID   : The number returned by ConnectToOrbiterSoundDLL
// Option : Keyword as defined below:
//
// PLAYCOUNTDOWNWHENTAKEOFF			// The countdown of Apollo 11 when you take-off
// PLAYWHENATTITUDEMODECHANGE		// Play "rotation" "linear" sound when you change mode
// PLAYGPWS							// The GPWS sound
// PLAYMAINTHRUST					// Main thrust sound
// PLAYHOVERTHRUST					// The hover thrust sound
// PLAYATTITUDETHRUST				// The attitude thrust sound
// PLAYDOCKINGSOUND					// The docking sound and radio
// PLAYRADARBIP						// The radar bip when near another vessel
// PLAYWINDAIRSPEED					// The wind airspeed when atmosphere
// PLAYDOCKLANDCLEARANCE			// The landing clearance 
// PLAYLANDINGANDGROUNDSOUND		// Rolling, landing, speedbrake, crash sound
// PLAYCABINAIRCONDITIONING			// Play the air conditionning sound
// PLAYCABINRANDOMAMBIANCE			// Play the random pump and rumble sound
// PLAYWINDAMBIANCEWHENLANDED		// Play the wind sound when landed
// PLAYRADIOATC						// Play the atc radio sound 
// DISPLAYTIMER						// Display the timer text when you take-off
// DISABLEAUTOPILOTWHENTIMEWARP		// The auto-disable of pilot if you time warp
// ALLOWRADIOBLACKOUT				// Allow the radio blackout when reentry
// MUTEORBITERSOUND					// Mute all the orbiter sound
// PLAYRETROTHRUST					// --- NEW OrbiterSound 4.0 
// PLAYUSERTHRUST					// --- NEW OrbiterSound 4.0 
// PLAYWINDCOCKPITOPEN				// --- NEW OrbiterSound 4.0 if TRUE play planet wind and base sound even in internal view.
// PLAYREENTRYAIRSPEED				// --- NEW OrbiterSound 4.0 the reentry sound, see demo scenario "reentry sound"
//
// Status:	TRUE if you want to allow the sound 
//			FALSE if you want to disable it (sound stops immediately when you call it)
//
// RETURN
// ------
//  FALSE if the connection to orbitersound's dll failed
//  or the keyword used is false
//
// EXAMPLES OF CALLING:
// --------------------
//  SoundOptionOnOff(MyID,PLAYMAINTHRUST,FALSE)	// Disable the main thrust sound in orbitersound
//  SoundOptionOnOff(MyID,PLAYGPWS,TRUE)		// Enable the GPWS sound in OrbiterSound
///////////////////////////////////////////////////////////////////////
BOOL SoundOptionOnOff(int MyID,int Option, BOOL Status=TRUE);

///////////////////////////////////////////////
// IsRadioPlaying
// --------------
// Returns TRUE if an OrbiterSound's atc radio WAV is currently playing
// otherwise it returns FALSE.
// This is usefull to avoid playing YOUR radio WAV during
// and OrbiterSound's atc one.
//
// RETURN
// ------
// FALSE if radio atc does not play OR the connection to orbitersound's dll failed.
//
// EXAMPLES OF CALLING:
// --------------------
//  if(IsRadioPlaying()==FALSE)
//  {
//     PlayVesselWave(MyID,MYRADIODOCKED);		
//  }
///////////////////////////////////////////////
BOOL IsRadioPlaying(void);

///////////////////////////////////////////////
// SetRadioFrequency
// -----------------
// With this function you can set the main radio frequency 
// used in OrbiterSound.
// 
// PARAMETERS:
// -----------
// Frequency: a char string in the form "nnn.nnn"
//			  (a directory with the same name must exist in the sound/radio folder)
//
// RETURN
// ------
// FALSE if the connection to orbitersound's dll failed
//
// EXAMPLES OF CALLING:
// --------------------
// SetRadioFrequency("296.800");  // Set the frequency 296.800 and radio will play the sound contained in this folder
/////////////////////////////////////////////////////////////////////////
BOOL SetRadioFrequency(char* Frequency);

#endif //__ORBITERSOUND40_SDK
