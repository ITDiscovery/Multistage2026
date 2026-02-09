/*********************************************************************************************
This file is part of Multistage2015 project
Copyright belogs to Fred18 for module implementation and its code
Biggest Credit goes to Vinka for his idea of Multistage.dll. None of his code was used here since his addons are all closed source.
Credit goes to Face for having pointed me to the GetPrivateProfileString
Credit goes to Hlynkacg for his OrientForBurn function which was the basis on which I developed the Attitude Function.

Multistage2015 is distributed FREEWARE. Its code is distributed along with the dll. Nobody is authorized to exploit the module or the code or parts of them commercially directly or indirectly.
You CAN distribute the dll together with your addon but in this case you MUST:
-	Include credit to the author in your addon documentation;
-	Add to the addon documentation the official link of Orbit Hangar Mods for download and suggest to download the latest and updated version of the module.
You CAN use parts of the code of Multistage2015, but in this case you MUST:
-	Give credits in your copyright header and in your documentation for the part you used.
-	Let your project be open source and its code available for at least visualization by other users.
You CAN NOT use the entire code for making and distributing the very same module claiming it as your work entirely or partly.
You CAN NOT claim that Multistage2015 is an invention of yourself or a work made up by yourself, or anyhow let intend that is not made and coded by the author.
You install and use Multistage2015 at your own risk, author will not be responsible for any claim or damage subsequent to its use or for the use of part of it or of part of its code.
*********************************************************************************************/


// ==============================================================




//              
//
// Multistage2015_MFD.cpp
//




// ==============================================================

#define STRICT
#define ORBITER_MODULE
#include "Multistage2015_MFD.h"


#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
// ==============================================================
// Global variables

int g_MFDmode; // identifier for new MFD mode




// ==============================================================
// API interface


DLLCLBK void InitModule(HINSTANCE hDLL)
{
	static char* name = const_cast<char*>("Multistage2015_MFD");   // MFD mode name
	MFDMODESPECEX spec;
	spec.name = name;
	spec.key = OAPI_KEY_T;                // MFD mode selection key
	spec.context = NULL;
	spec.msgproc = Multistage2015_MFD::MsgProc;  // MFD mode callback function

	// Register the new MFD mode with Orbiter
	g_MFDmode = oapiRegisterMFDMode(spec);


}

DLLCLBK void ExitModule(HINSTANCE hDLL)
{
	// Unregister the custom MFD mode when the module is unloaded
	oapiUnregisterMFDMode(g_MFDmode);

}




/*DLLCLBK void opcFocusChanged  ( OBJHANDLE New, OBJHANDLE Old )
{
	//v=oapiGetVesselInterface(New);

}*/


bool InputStep(void* id, char* str, void* usrdata)
{
	return (((Multistage2015_MFD*)usrdata)->AddStep(str));
}

bool SetRange(void* id, char* str, void* usrdata)
{
	return (((Multistage2015_MFD*)usrdata)->MFDSetRange(str));
}

bool LoadTlmFile(void* id, char* str, void* usrdata)
{
	return (((Multistage2015_MFD*)usrdata)->MFDLoadTlmFile(str));
}

bool InputInterval(void* id, char* str, void* usrdata)
{
	return (((Multistage2015_MFD*)usrdata)->InputPMCInterval(str));
}

bool SetAltSteps(void* id, char* str, void* usrdata)
{
	return (((Multistage2015_MFD*)usrdata)->InputAltSteps(str));
}

bool SetNewPitchLimit(void* id, char* str, void* usrdata)
{
	return (((Multistage2015_MFD*)usrdata)->InputNewPitchLimit(str));
}

bool NewRefVessel(void* id, char* str, void* usrdata)
{
	return (((Multistage2015_MFD*)usrdata)->InputNewRefVessel(str));
}

// ==============================================================
// MFD class implementation

// Constructor
Multistage2015_MFD::Multistage2015_MFD(DWORD w, DWORD h, VESSEL* vessel)
	: MFD2(w, h, vessel)
{
	smallfont = oapiCreateFont(h / 21, FALSE, const_cast<char*>("Courier"), FONT_NORMAL);
	verysmallfont = oapiCreateFont(h / 42, FALSE, const_cast<char*>("Courier"), FONT_NORMAL);
	smallquarterfont = oapiCreateFont(h / 24, FALSE, const_cast<char*>("Courier"), FONT_NORMAL);
	mygreen = oapiCreateBrush(0x00FF00);
	mydarkgreen = oapiCreateBrush(0x336633);
	mydarkblue = oapiCreateBrush(0x800000);
	myyellow = oapiCreateBrush(0x00FFFF);
	mydarkred = oapiCreateBrush(0x660000AA);
	myblack = oapiCreateBrush(0x000000);

	pengreen = oapiCreatePen(1, 1, 0x00FF00);
	pengreenstrong = oapiCreatePen(1, 2, 0x00FF00);
	penwhite = oapiCreatePen(1, 1, 0xFFFFFF);
	pengraydotted = oapiCreatePen(2, 1, 0x9c9c9c);
	pengray = oapiCreatePen(1, 1, 0x9c9c9c);
	penwhitestrong = oapiCreatePen(1, 2, 0xFFFFFF);
	penwhitedotted = oapiCreatePen(2, 1, 0xFFFFFF);
	pendarkgreen = oapiCreatePen(1, 1, 0x336633);
	penyellow = oapiCreatePen(1, 1, 0x30FFFF);
	penyellowstrong = oapiCreatePen(1, 2, 0x30FFFF);
	pendarkyellow = oapiCreatePen(1, 1, 0x005252);
	penblue = oapiCreatePen(1, 1, 0xFF0000);
	penbluestrong = oapiCreatePen(1, 2, 0xFF0000);
	pendarkblue = oapiCreatePen(1, 1, 0x290000);
	penred = oapiCreatePen(1, 1, 0x0000FF);
	penredstrong = oapiCreatePen(1, 2, 0x0000FF);
	pendarkred = oapiCreatePen(1, 1, 0x000066);
	penpurple = oapiCreatePen(1, 1, 0xFF00FF);
	penpurplestrong = oapiCreatePen(1, 2, 0xFF00FF);
	pendarkpurple = oapiCreatePen(1, 1, 0x660066);
	pencyan = oapiCreatePen(1, 1, 0xFFFF00);
	pencyanstrong = oapiCreatePen(1, 2, 0xFFFF00);
	pendarkcyan = oapiCreatePen(1, 1, 0x666600);











	SelectedStep = 1;
	SelectedTlm = 1;
	SelectedInfoItem = 1;

	for (int i = 0; i < 6; i++)
	{
		ViewData[i] = TRUE;
		AutoRange[i] = TRUE;
	}

	for (int q = 0; q < 128; q++)
	{
		NewRefVesselName[q] = 0;
		outTitle[q] = 0;
	}

	if (ValidVessel()) {
		CurrentView = V_INIT;
	}
	else {
		CurrentView = V_NOCLASS;
	}

	outerVessel = FALSE;


	// Add MFD initialisation here
}

// Destructor
Multistage2015_MFD::~Multistage2015_MFD()
{
	oapiReleaseFont(smallfont);
	oapiReleaseFont(verysmallfont);
	oapiReleaseFont(smallquarterfont);
	oapiReleaseBrush(mygreen);
	oapiReleaseBrush(mydarkgreen);
	oapiReleaseBrush(myyellow);
	oapiReleaseBrush(mydarkred);
	oapiReleaseBrush(myblack);





	oapiReleasePen(pengreen);
	oapiReleasePen(pengreenstrong);
	oapiReleasePen(penwhite);
	oapiReleasePen(pengraydotted);
	oapiReleasePen(pengray);
	oapiReleasePen(penwhitestrong);
	oapiReleasePen(penwhitedotted);
	oapiReleasePen(pendarkgreen);
	oapiReleasePen(penyellow);
	oapiReleasePen(penyellowstrong);
	oapiReleasePen(pendarkyellow);
	oapiReleasePen(penblue);
	oapiReleasePen(penbluestrong);
	oapiReleasePen(pendarkblue);
	oapiReleasePen(penred);
	oapiReleasePen(penredstrong);
	oapiReleasePen(pendarkred);
	oapiReleasePen(penpurple);
	oapiReleasePen(penpurplestrong);
	oapiReleasePen(pendarkpurple);
	oapiReleasePen(pencyan);
	oapiReleasePen(pencyanstrong);
	oapiReleasePen(pendarkcyan);









}

bool Multistage2015_MFD::ValidVessel()
{
	if (!outerVessel)
		Hvessel = oapiGetFocusObject();
	else
		Hvessel = oapiGetVesselByName(NewRefVesselName);

	VESSEL* v = oapiGetVesselInterface(Hvessel);
	if (!v) return false;

	int testversion = v->Version();
	if (testversion < 2) {
		CurrentView = V_NOCLASS;
		return false;
	}

	int test = ((VESSEL3*)v)->clbkGeneric(2015, 2015, 0);
	if (test != 2015) {
		CurrentView = V_NOCLASS;
		return false;
	}

	Ms = dynamic_cast<Multistage2015*>((VESSEL3*)v);
	if (!Ms) {
		CurrentView = V_NOCLASS;
		return false;
	}

	return true;
}


double Multistage2015_MFD::ArrayMin(oapi::IVECTOR2* Array, int uptopoint)
{
	int min = Array[0].y;

	for (int i = 0; i < uptopoint; i++)
	{
		if (Array[i].y < min)
		{
			min = Array[i].y;
		}
	}

	return min;
}

double Multistage2015_MFD::ArrayMax(oapi::IVECTOR2* Array, int uptopoint)
{
	int max = Array[0].y;

	for (int i = 0; i < uptopoint; i++)
	{
		if (Array[i].y > max)
		{
			max = Array[i].y;
		}
	}

	return max;
}

void Multistage2015_MFD::ToggleViewData(int data)
{
	if (ViewData[data]) { ViewData[data] = FALSE; }
	else { ViewData[data] = TRUE; }
}

// Return button labels
char* Multistage2015_MFD::ButtonLabel(int bt)
{

	// The labels for the two buttons used by our MFD mode
/*	static char *label[2] = {"UP", "DN"};
	return (bt < 2 ? label[bt] : 0);*/
	switch (CurrentView) {
	case V_INIT:
	{
		if (!outerVessel) {
			static const char* label[10] = { "FST","VEH","GNC","PLD","CTRL","MNT","ALT","PMC","PLM","COM" };
			return (bt < 10 ? const_cast<char*>(label[bt]) : 0);
		}
		else {
			static const char* label[12] = { "FST","VEH","GNC","PLD","CTRL","MNT","ALT","PMC","PLM","COM",0,"RST" };
			return (bt < 12 ? const_cast<char*>(label[bt]) : 0);

		}
		break;
	}
	case V_PAYLOAD:
	{
		static const char* label[8] = { "FST","VEH","GNC","PLD","CTRL","MNT","FAI","JET" };
		return (bt < 8 ? const_cast<char*>(label[bt]) : 0);
		break;
	}
	case V_VEHICLE:
	case V_BATTS:
	case V_THRUST:
	{
		static const char* label[9] = { "FST","VEH","GNC","PLD","CTRL","MNT","FL","THR","BAT" };
		return (bt < 9 ? const_cast<char*>(label[bt]) : 0);
		break;
	}
	case V_CTRL:
	{
		static const char* label[10] = { "FST","VEH","GNC","PLD","CTRL","MNT","ATT","PIT","YAW","ROL" };
		return (bt < 10 ? const_cast<char*>(label[bt]) : 0);
		break;
	}
	case V_GUIDANCE:
	{
		static const char* label[12] = { "FST","VEH","GNC","PLD","CTRL","MNT","UP","DN","ADD","DEL","SAV","AP" };
		return (bt < 12 ? const_cast<char*>(label[bt]) : 0);
		break;
	}
	case V_MONITOR:
	{
		static const char* label[12] = { "FST","VEH","GNC","PLD","CTRL","MNT","UP","DN","TOG","SET","LD","SAV" };
		return (bt < 12 ? const_cast<char*>(label[bt]) : 0);
		break;
	}






	case V_NOCLASS:
	{
		static const char* label[1] = { "SEL" };
		return (bt < 1 ? const_cast<char*>(label[bt]) : 0);
		break;

	}
	}
	return 0;
}

// Return button menus
int Multistage2015_MFD::ButtonMenu(const MFDBUTTONMENU** menu) const
{

	// The menu descriptions for the two buttons
	//switch(CurrentView){
	//case V_INIT:
	static const MFDBUTTONMENU genmnu[6] = {
		{"Flight Settings", 0, 0},
		{"Fuel Display", 0, 0},
		{"Guidance Display", 0, 0},
		{"Payload Display", 0, 0},
		{"Control Display", 0, 0},
		{"Performance Monitor", 0, 0}
	};

	static const MFDBUTTONMENU initmnu[10] = {
		{"Flight Settings", 0, 0},
		{"Fuel Display", 0, 0},
		{"Guidance Display", 0, 0},
		{"Payload Display", 0, 0},
		{"Control Display", 0, 0},
		{"Performance Monitor", 0, 0},
		{"Set Altitude","Steps",0},
		{"Set Peg Major","Cycle Interval",0},
		{"Set Peg Pitch","Limit",0},
		{"Toggle Complex","Flight",0}
	};

	static const MFDBUTTONMENU initmnuOV[12] = {
		{"Flight Settings", 0, 0},
		{"Fuel Display", 0, 0},
		{"Guidance Display", 0, 0},
		{"Payload Display", 0, 0},
		{"Control Display", 0, 0},
		{"Performance Monitor", 0, 0},
		{"Set Altitude","Steps",0},
		{"Set Peg Major","Cycle Interval",0},
		{"Set Peg Pitch","Limit",0},
		{"Toggle Complex","Flight",0},
		{0,0,0},
		{"Reset Vessel","to Focus"}
	};

	//if (menu) *menu = mnu;
	//return 6; // return the number of buttons used
	//break;
	//case V_FUEL:
	/*static const MFDBUTTONMENU genmnu[6] = {
		{"MFD Menu", 0, 0},
		{"Fuel Display", 0, 0},
		{"Guidance Display", 0, 0},
		{"Payload Display", 0, 0},
		{"Control Display", 0, 0},
		{"Performance Monitor", 0, 0}
	};*/
	//if (menu) *menu = mnu;
	//return 6; // return the number of buttons used
	//break;
	//case V_CTRL:
	static const MFDBUTTONMENU ctrmnu[10] = {
		{"Flight Settings", 0, 0},
		{"Fuel Display", 0, 0},
		{"Guidance Display", 0, 0},
		{"Payload Display", 0, 0},
		{"Control Display", 0, 0},
		{"Performance Monitor", 0, 0},
		{"Toggle Attitude", "Control", 0},
		{"Toggle Pitch", "Control", 0},
		{"Toggle Yaw", "Control", 0},
		{"Toggle Roll", "Control", 0}
	};

	static const MFDBUTTONMENU gncmnu[12] = {
		{"Flight Settings", 0, 0},
		{"Fuel Display", 0, 0},
		{"Guidance Display", 0, 0},
		{"Payload Display", 0, 0},
		{"Control Display", 0, 0},
		{"Performance Monitor", 0, 0},
		{"Select Previous", "Step", 0},
		{"Select Next", "Step", 0},
		{"Add New", "Step", 0},
		{"Delete Selected", "Step", 0},
		{"Save Guidance", "File", 0},
		{"Toggle Autopilot", 0, 0}
	};

	static const MFDBUTTONMENU mntmnu[12] = {
		{"Flight Settings", 0, 0},
		{"Fuel Display", 0, 0},
		{"Guidance Display", 0, 0},
		{"Payload Display", 0, 0},
		{"Control Display", 0, 0},
		{"Performance Monitor", 0, 0},
		{"Select Previous", "Item", 0},
		{"Select Next", "Item", 0},
		{"Toggle Selected", "Item ON/OFF", 0},
		{"Set Range of", "Selected Item", 0},
		{"Load Reference", "File", 0},
		{"Save Current", "Telemetry", 0}
	};

	static const MFDBUTTONMENU vehmnu[9] = {
		{"Flight Settings", 0, 0},
		{"Fuel Display", 0, 0},
		{"Guidance Display", 0, 0},
		{"Payload Display", 0, 0},
		{"Control Display", 0, 0},
		{"Performance Monitor", 0, 0},
		{"Fuel Monitor", 0, 0},
		{"Thrust Monitor", 0, 0},
		{"Batteries Monitor", 0, 0}
	};

	static const MFDBUTTONMENU pldmnu[8] = {
		{"Flight Settings", 0, 0},
		{"Fuel Display", 0, 0},
		{"Guidance Display", 0, 0},
		{"Payload Display", 0, 0},
		{"Control Display", 0, 0},
		{"Performance Monitor", 0, 0},
		{"Fairing/LES","Jettison",0},
		{"Stage/Payload","Jettison",0}
	};

	static const MFDBUTTONMENU noclassmnu[1] = {
		{"Select a Target","Vessel for MFD",0}
	};



	//if (menu) *menu = mnu;
	//return 7; // return the number of buttons used
	//break;
	//}

	if (menu) {
		if (CurrentView == V_INIT)
		{
			if (!outerVessel) {
				*menu = initmnu;
				return 10;
			}
			else {
				*menu = initmnuOV;
				return 12;
			}
		}
		else if (CurrentView == V_CTRL)
		{
			*menu = ctrmnu;
			return 10;
		}
		else if (CurrentView == V_GUIDANCE)
		{
			*menu = gncmnu;
			return 12;
		}
		else if ((CurrentView == V_VEHICLE) || (CurrentView == V_BATTS) || (CurrentView == V_THRUST))
		{
			*menu = vehmnu;
			return 9;
		}
		else if (CurrentView == V_MONITOR)
		{
			*menu = mntmnu;
			return 12;
		}
		else if (CurrentView == V_PAYLOAD)
		{
			*menu = pldmnu;
			return 8;
		}
		else if (CurrentView == V_NOCLASS)
		{
			*menu = noclassmnu;
			return 1;
		}
		else {
			*menu = genmnu;
			return 6;
		}
	}
	return 0;
}

bool Multistage2015_MFD::ConsumeButton(int bt, int event)
{
	if (!(event & PANEL_MOUSE_LBDOWN)) return false;
	if ((bt < 6) && (CurrentView != V_NOCLASS)) {
		CurrentView = bt;
		InvalidateButtons();


	}
	else
	{


		switch (CurrentView)
		{
		case V_INIT:
			if (bt == 6)
			{
				oapiOpenInputBox(const_cast<char*>("Set New Altitude Steps [m]- step1,step2,step3,step4"), SetAltSteps, 0, 35, (void*)this);
			}
			else if (bt == 7)
			{
				oapiOpenInputBox(const_cast<char*>("Set New Interval [s]"), InputInterval, 0, 35, (void*)this);
				//Ms->SetPegMajorCycleInterval(1);
			}
			else if (bt == 8)
			{
				oapiOpenInputBox(const_cast<char*>("Set New Pitch Limit [deg]"), SetNewPitchLimit, 0, 35, (void*)this);
			}
			else if (bt == 9)
			{
				Ms->ToggleComplexFlight();


			}
			else if (bt == 11)
			{
				if (outerVessel)
				{
					outerVessel = FALSE;
					CurrentView = V_NOCLASS;
					InvalidateButtons();

				}
			}
			break;
		case V_VEHICLE:
			if (bt == 7)
			{
				CurrentView = V_THRUST;
			}
			else if (bt == 8)
			{
				CurrentView = V_BATTS;
			}
			break;
		case V_THRUST:
			if (bt == 6)
			{
				CurrentView = V_VEHICLE;
			}
			else if (bt == 8)
			{
				CurrentView = V_BATTS;
			}
			break;
		case V_BATTS:
			if (bt == 6)
			{
				CurrentView = V_VEHICLE;
			}
			else if (bt == 7)
			{
				CurrentView = V_THRUST;
			}
			break;

		case V_CTRL:
			if (bt == 6) {
				Ms->ToggleAttCtrl(TRUE, TRUE, TRUE);
			}
			else if (bt == 7) {
				Ms->ToggleAttCtrl(TRUE, FALSE, FALSE);
			}
			else if (bt == 8) {
				Ms->ToggleAttCtrl(FALSE, TRUE, FALSE);
			}
			else if (bt == 9) {
				Ms->ToggleAttCtrl(FALSE, FALSE, TRUE);
			}
			break;
		case V_GUIDANCE:
			if (bt == 6) {
				SelectedStep -= 1;
				if (SelectedStep < 1) { SelectedStep = 1; }
			}
			else if (bt == 7) {
				SelectedStep += 1;
				if (SelectedStep > Ms->nsteps) { SelectedStep = Ms->nsteps; }
			}
			else if (bt == 9) {
				Ms->VinkaDeleteStep(SelectedStep);
			}
			else if (bt == 8) {
				oapiOpenInputBox(const_cast<char*>("Add Guidance Step"), InputStep, 0, 35, (void*)this);
			}
			else if (bt == 10) {
				Ms->WriteGNCFile();
			}
			else if (bt == 11) {
				Ms->ToggleAP();
			}
			break;
		case V_MONITOR:
			if (bt == 6)
			{
				SelectedTlm -= 1;
				if (SelectedTlm < 1) { SelectedTlm = 1; }
			}
			else if (bt == 7)
			{
				SelectedTlm += 1;
				if (SelectedTlm > 6) { SelectedTlm = 6; }
			}
			else if (bt == 8)
			{
				ToggleViewData(SelectedTlm - 1);
			}
			else if (bt == 9)
			{
				oapiOpenInputBox(const_cast<char*>("Set Range 'MIN,MAX' - A for automatic"), SetRange, 0, 35, (void*)this);
			}
			else if (bt == 10)
			{
				oapiOpenInputBox(const_cast<char*>("Load Telemetry Reference File"), LoadTlmFile, 0, 35, (void*)this);
			}
			else if (bt == 11)
			{
				Ms->WriteTelemetryFile(0);
			}

			break;
		case V_PAYLOAD:
			if (bt == 6)

			{
				char kstate[256];
				for (int i = 0; i < 256; i++) kstate[i] = 0x00;
				kstate[OAPI_KEY_F] = 0x80;
				Ms->SendBufferedKey(OAPI_KEY_F, TRUE, kstate);
			}
			else if (bt == 7)
			{
				char kstate[256];
				for (int i = 0; i < 256; i++) kstate[i] = 0x00;
				kstate[OAPI_KEY_J] = 0x80;
				Ms->SendBufferedKey(OAPI_KEY_J, TRUE, kstate);
			}
			break;


		case V_NOCLASS:
			if (bt == 0)
			{
				oapiOpenInputBox(const_cast<char*>("New Reference Vessel"), NewRefVessel, 0, 35, (void*)this);
			}
			break;

		}
	}

	return true;
}


void Multistage2015_MFD::UpdateTlmValues()
{
	for (int i = 0; i < Ms->tlmidx; i++)
	{
		MFDtlmAlt[i].x = Ms->tlmAlt[i].x;
		MFDtlmAlt[i].y = Ms->tlmAlt[i].y;
		MFDtlmAcc[i].x = Ms->tlmAcc[i].x;
		MFDtlmAcc[i].y = Ms->tlmAcc[i].y * 100;
		MFDtlmVv[i].x = Ms->tlmVv[i].x;
		MFDtlmVv[i].y = Ms->tlmVv[i].y * 10;
		MFDtlmSpeed[i].x = Ms->tlmSpeed[i].x;
		MFDtlmSpeed[i].y = Ms->tlmSpeed[i].y * 10;
		MFDtlmThrust[i].x = Ms->tlmThrust[i].x;
		MFDtlmThrust[i].y = Ms->tlmThrust[i].y;
		MFDtlmPitch[i].x = Ms->tlmPitch[i].x;
		MFDtlmPitch[i].y = Ms->tlmPitch[i].y * 100;

	}
	for (int j = 0; j < TLMSECS; j++)
	{
		RefMFDtlmAlt[j].x = Ms->ReftlmAlt[j].x;
		RefMFDtlmAlt[j].y = Ms->ReftlmAlt[j].y;
		RefMFDtlmSpeed[j].x = Ms->ReftlmSpeed[j].x;
		RefMFDtlmSpeed[j].y = Ms->ReftlmSpeed[j].y * 10;
		RefMFDtlmAcc[j].x = Ms->ReftlmAcc[j].x;
		RefMFDtlmAcc[j].y = Ms->ReftlmAcc[j].y * 100;
		RefMFDtlmVv[j].x = Ms->ReftlmVv[j].x;
		RefMFDtlmVv[j].y = Ms->ReftlmVv[j].y * 10;
		RefMFDtlmThrust[j].x = Ms->ReftlmThrust[j].x;
		RefMFDtlmThrust[j].y = Ms->ReftlmThrust[j].y;
		RefMFDtlmPitch[j].x = Ms->ReftlmPitch[j].x;
		RefMFDtlmPitch[j].y = Ms->ReftlmPitch[j].y * 100;
	}
}

oapi::IVECTOR2 Multistage2015_MFD::ScalePoints(oapi::IVECTOR2 input[], double xscale, double yscale, int uptopoint, int MinY = 0)
{
	for (int i = 0; i < uptopoint; i++)
	{

		input[i].x = input[i].x / xscale;
		input[i].y = -(input[i].y - MinY) / yscale;

	}

	return *input;
}


// Repaint the MFD
bool Multistage2015_MFD::Update(oapi::Sketchpad* skp)
{
	ValidVessel();



	int len;
	char buff[MAXLEN];
	int line = H / 20;
	int margin = W / 20;
	int middleW = W / 2;
	int middleH = H / 2;

	switch (CurrentView) {
	case V_NOCLASS:
		//skp->SetFont(font);
		Title(skp, "Multistage2015 MFD");
		skp->SetTextAlign(oapi::Sketchpad::CENTER, oapi::Sketchpad::BOTTOM);
		skp->SetTextColor(0x00FFFF);
		len = sprintf_s(buff, "This Vessel is not a Multistage2015");
		skp->Text(W / 2, H / 2, buff, len);
		break;
	case V_INIT:
	{
		if (!outerVessel) {
			Title(skp, "Multistage2015 MFD - MENU");
		}
		else {
			len = sprintf_s(outTitle, "MS15-MENU RefVessel:%s", NewRefVesselName);
			skp->SetTextColor(0x00FF00);
			//Title (skp,outTitle);
			skp->Text(margin, 0, outTitle, len);
			skp->SetTextColor(0xFFFFFF);
		}
		skp->SetTextAlign(oapi::Sketchpad::CENTER, oapi::Sketchpad::BOTTOM);
		skp->SetPen(penwhite);
		skp->Line(0, line + 3, W, line + 3);
		skp->SetPen(NULL);
		len = sprintf_s(buff, "Welcome to the Multistage2015 MFD");
		skp->Text(middleW, 2 * line + 3, buff, len);
		skp->SetTextAlign(oapi::Sketchpad::LEFT, oapi::Sketchpad::BOTTOM);
		skp->SetTextColor(0x9c9c9c);
		len = sprintf_s(buff, "Vehicle Information:");
		skp->Text(margin, line * 4, buff, len);
		len = sprintf_s(buff, "Name:%s", Ms->GetName());
		skp->Text(margin, line * 5, buff, len);
		len = sprintf_s(buff, "Current Mass: %.0f kg", Ms->GetMass());
		skp->Text(margin, line * 6, buff, len);
		len = sprintf_s(buff, "N. of stages: %i", Ms->nStages);
		skp->Text(margin, line * 7, buff, len);
		len = sprintf_s(buff, "N. of boosters: %i", Ms->nBoosters);
		skp->Text(margin, line * 8, buff, len);
		len = sprintf_s(buff, "N. of payloads: %i", Ms->nPayloads);
		skp->Text(margin, line * 9, buff, len);

		double PLtotWeight = 0;
		for (int i = 0; i < Ms->nPayloads; i++)
		{
			PLtotWeight += Ms->payload[i].mass;
		}

		len = sprintf_s(buff, "Total Payload Mass: %.0f kg", PLtotWeight);
		skp->Text(margin, line * 10, buff, len);

		len = sprintf_s(buff, "Alt Steps: %.0f,%.0f,%.0f,%.0f m", Ms->altsteps[0], Ms->altsteps[1], Ms->altsteps[2], Ms->altsteps[3]);
		skp->Text(margin, line * 11, buff, len);
		len = sprintf_s(buff, "Grav Turn Calc Init Pitch:%.2f deg", Ms->GT_IP_Calculated * DEG);
		skp->Text(margin, line * 12, buff, len);

		len = sprintf_s(buff, "PEG Major Cycle Interval:%.1f s", Ms->PegMajorCycleInterval);
		skp->Text(margin, line * 13, buff, len);
		len = sprintf_s(buff, "PEG Pitch Limit: %.1f deg", Ms->PegPitchLimit * DEG);
		skp->Text(margin, line * 14, buff, len);
		if (Ms->Complex) { len = sprintf_s(buff, "Complex Flight Active"); }
		else { len = sprintf_s(buff, "Complex Flight Not Active"); }
		skp->Text(margin, line * 15, buff, len);
		skp->SetPen(penwhite);
		skp->Rectangle(margin * 0.5, line * 3 - 5, W - margin * 0.5, line * 15 + 5);
		break;
	}
	case V_VEHICLE:
	{
		if (!outerVessel) {
			skp->SetTextColor(0x00FFFF);
			Title(skp, "Multistage2015 MFD - FUEL DISPLAY");
		}
		else {
			len = sprintf_s(outTitle, "MS15-FUEL RefVessel:%s", NewRefVesselName);
			skp->SetTextColor(0x00FF00);
			//Title (skp,outTitle);
			skp->Text(margin, 0, outTitle, len);
			skp->SetTextColor(0xFFFFFF);
		}
		skp->SetTextAlign(oapi::Sketchpad::LEFT, oapi::Sketchpad::BOTTOM);
		len = sprintf_s(buff, "Boosters:");
		skp->Text(margin, line * 2 + 5, buff, len);
		len = sprintf_s(buff, "Stages:");
		//int deltalines=Ms->nBoosters-Ms->currentBooster;
		skp->Text(margin, line * (11), buff, len);

		len = sprintf_s(buff, "AAAAAAAAA");
		DWORD ltw = skp->GetTextWidth(buff, len);
		ltw += 3;


		for (int i = Ms->currentBooster; i < Ms->nBoosters; i++)
		{
			skp->SetBrush(NULL);
			double fl = Ms->GetPropellantMass(Ms->booster[i].tank) / Ms->GetPropellantMaxMass(Ms->booster[i].tank);
			VECTOR3 Rbt = Ms->hms(Ms->BoosterRemBurnTime(i, 1));
			len = sprintf_s(buff, "Grp%i:%.0f%%", i + 1, fl * 100);
			skp->Text(margin, line * (i + 3) + 5, buff, len);
			len = sprintf_s(buff, "%02.0f:%02.0f", Rbt.y, Rbt.z);
			DWORD rtw = skp->GetTextWidth(buff, len);
			rtw += 3;
			skp->Text(W - margin - rtw, line * (i + 3) + 5, buff, len);
			//skp->SetPen(pengreen);
			//skp->Rectangle(margin+ltw,line*(i+2)+1,W-margin-rtw-3,line*(i+3)-1);
			skp->SetBrush(mydarkgreen);
			skp->SetPen(NULL);
			skp->Rectangle(margin + ltw, line * (i + 2) + 5 + 2, fl * (W - 2 * margin - rtw - ltw - 3) + margin + ltw, line * (i + 3) - 2 + 5);
			skp->SetBrush(NULL);

		}
		for (int i = Ms->currentStage; i < Ms->nStages; i++)
		{
			skp->SetBrush(NULL);
			double fl = Ms->GetPropellantMass(Ms->stage[i].tank) / Ms->GetPropellantMaxMass(Ms->stage[i].tank);
			VECTOR3 Rbt = Ms->hms(Ms->RemBurnTime(i, 1));
			len = sprintf_s(buff, "Stg%i:%.0f%%", i + 1, fl * 100);
			skp->Text(margin, line * (i + 12), buff, len);
			len = sprintf_s(buff, "%02.0f:%02.0f", Rbt.y, Rbt.z);
			DWORD rtw = skp->GetTextWidth(buff, len);
			rtw += 3;
			skp->Text(W - margin - rtw, line * (i + 12), buff, len);
			//skp->Rectangle(margin+ltw,line*(i+11)+1,W-margin-rtw-3,line*(i+12)-1);
			skp->SetPen(NULL);
			skp->SetBrush(mydarkblue);
			skp->Rectangle(margin + ltw, line * (i + 11) + 2, fl * (W - 2 * margin - rtw - ltw - 3) + margin + ltw, line * (i + 12) - 2);
			skp->SetBrush(NULL);
			skp->SetPen(penwhite);
			skp->Line(0, middleH - 5, W, middleH - 5);
			skp->Line(0, line + 3, W, line + 3);
			skp->Line(0, 19 * line - 5, W, 19 * line - 5);
			skp->SetPen(NULL);


		}

		VECTOR3 met = Ms->hms(Ms->MET);

		if (Ms->MET >= 0) {
			len = sprintf_s(buff, "MET: %03.0f:%02.0f:%02.0f", met);
		}
		else {
			len = sprintf_s(buff, "T-: %03.0f:%02.0f:%02.0f", met);
		}
		skp->Text(margin, line * 20, buff, len);

		if (Ms->APstat) { len = sprintf_s(buff, "AP: ON"); }
		else { len = sprintf_s(buff, "AP: OFF"); }
		skp->Text(W - margin - skp->GetTextWidth(buff, len), line * 20, buff, len);
		break;
	}


	case V_THRUST:
	{



		if (!outerVessel) {
			Title(skp, "Multistage2015 MFD - THRUST DISPLAY");

		}
		else {

			len = sprintf_s(outTitle, "MS15-THRUST RefVessel:%s", NewRefVesselName);
			skp->SetTextColor(0x00FF00);
			//Title (skp,outTitle);
			skp->Text(margin, 0, outTitle, len);
			skp->SetTextColor(0xFFFFFF);
		}
		//	skp->SetTextColor (0x00FFFF);
		skp->SetTextAlign(oapi::Sketchpad::LEFT, oapi::Sketchpad::BOTTOM);

		VECTOR3 met = Ms->hms(Ms->MET);

		if (Ms->MET >= 0) {
			len = sprintf_s(buff, "MET: %03.0f:%02.0f:%02.0f", met);
		}
		else {
			len = sprintf_s(buff, "T-: %03.0f:%02.0f:%02.0f", met);
		}
		skp->Text(margin, line * 20, buff, len);

		if (Ms->APstat) { len = sprintf_s(buff, "AP: ON"); }
		else { len = sprintf_s(buff, "AP: OFF"); }
		skp->Text(W - margin - skp->GetTextWidth(buff, len), line * 20, buff, len);

		len = sprintf_s(buff, "Stage: %i", Ms->currentStage + 1);
		skp->Text(margin, line * 3, buff, len);

		len = sprintf_s(buff, "Booster: %i", Ms->currentBooster + 1);
		skp->Text(margin, line * 11, buff, len);

		skp->SetPen(penwhite);
		//skp->Line(0,middleH-5,W,middleH-5);
		skp->Line(0, line + 3, W, line + 3);
		skp->Line(0, 19 * line - 5, W, 19 * line - 5);
		skp->SetPen(NULL);


		len = sprintf_s(buff, "Eng 1:");
		DWORD lm = skp->GetTextWidth(buff, len);

		for (int i = 0; i < Ms->stage[Ms->currentStage].nEngines; i++)
		{

			skp->SetFont(smallfont);

			double Thrust, Throttle, Performance;
			Throttle = Ms->GetThrusterLevel(Ms->stage[Ms->currentStage].th_main_h[i]);
			Thrust = (Ms->GetThrusterMax0(Ms->stage[Ms->currentStage].th_main_h[i]) * Throttle);
			Performance = Thrust / ((Ms->stage[Ms->currentStage].thrust / Ms->stage[Ms->currentStage].nEngines) * Throttle);

			skp->SetBrush(NULL);
			skp->SetPen(penwhite);
			skp->Rectangle(lm + 2, line * (i + 3), W - margin, line * (i + 4));
			skp->SetPen(NULL);
			skp->SetBrush(mydarkred);
			skp->Rectangle(lm + 4, line * (i + 3) + 1, (W - margin - lm - 6) * Throttle + lm + 4, line * (i + 4) - 1);
			skp->SetPen(NULL);
			skp->SetBrush(NULL);


			len = sprintf_s(buff, "Eng%i  Th: %.2f MN Tle:%.0f%% Prm:%.0f%%", i + 1, Thrust / 1000000, Throttle * 100, Performance * 100);
			skp->Text(margin + 3, line * (i + 4), buff, len);

			/*len=sprintf_s(buff,"Throttle: %.1f%%",Throttle*100);
			skp->Text(margin,line*(i+5*i),buff,len);
			len=sprintf_s(buff,"Performance: %.1f%%",Performance*100);
			skp->Text(margin,line*(i+6*i),buff,len);*/

		}


		for (int q = 0; q < Ms->booster[Ms->currentBooster].N; q++)
		{
			skp->SetFont(smallfont);

			double Thrust, Throttle, Performance;
			Throttle = Ms->GetThrusterLevel(Ms->booster[Ms->currentBooster].th_booster_h[q]);
			Thrust = (Ms->GetThrusterMax0(Ms->booster[Ms->currentBooster].th_booster_h[q]) * Throttle);
			Performance = Thrust / ((Ms->booster[Ms->currentBooster].thrust) * Throttle);

			skp->SetBrush(NULL);
			skp->SetPen(penwhite);
			skp->Rectangle(lm + 2, line * (q + 11), W - margin, line * (q + 12));
			skp->SetPen(NULL);
			skp->SetBrush(mydarkred);
			skp->Rectangle(lm + 4, line * (q + 11) + 1, (W - margin - lm - 6) * Throttle + lm + 4, line * (q + 12) - 1);
			skp->SetPen(NULL);
			skp->SetBrush(NULL);

			len = sprintf_s(buff, "Eng%i  Th: %.2f MN Tle:%.0f%% Prm:%.0f%%", q + 1, Thrust / 1000000, Throttle * 100, Performance * 100);
			skp->Text(margin + 3, line * (q + 12), buff, len);
		}



		break;
	}
	case V_BATTS:
	{



		if (!outerVessel) {
			Title(skp, "Multistage2015 MFD - BATTS DISPLAY");
		}
		else {
			len = sprintf_s(outTitle, "MS15-BATTS RefVessel:%s", NewRefVesselName);
			skp->SetTextColor(0x00FF00);
			//Title (skp,outTitle);
			skp->Text(margin, 0, outTitle, len);
			skp->SetTextColor(0xFFFFFF);
		}
		//	skp->SetTextColor (0x00FFFF);
		skp->SetTextAlign(oapi::Sketchpad::LEFT, oapi::Sketchpad::BOTTOM);
		VECTOR3 met = Ms->hms(Ms->MET);

		if (Ms->MET >= 0) {
			len = sprintf_s(buff, "MET: %03.0f:%02.0f:%02.0f", met);
		}
		else {
			len = sprintf_s(buff, "T-: %03.0f:%02.0f:%02.0f", met);
		}
		skp->Text(margin, line * 20, buff, len);

		if (Ms->APstat) { len = sprintf_s(buff, "AP: ON"); }
		else { len = sprintf_s(buff, "AP: OFF"); }
		skp->Text(W - margin - skp->GetTextWidth(buff, len), line * 20, buff, len);
		skp->SetPen(NULL);






		len = sprintf_s(buff, "AAAAAAAAA");
		DWORD ltw = skp->GetTextWidth(buff, len);
		ltw += 3;

		for (int i = Ms->currentStage; i < Ms->nStages; i++)
		{

			skp->SetBrush(NULL);
			double bl = Ms->stage[i].batteries.CurrentCharge / Ms->stage[i].batteries.MaxCharge;
			VECTOR3 Rbat = Ms->hms(Ms->stage[i].batteries.CurrentCharge);
			len = sprintf_s(buff, "Stg%i:%.0f%%", i + 1, bl * 100);
			skp->Text(margin, line * (i + 3), buff, len);
			len = sprintf_s(buff, "%03.0f:%02.0f:%02.0f", Rbat.x, Rbat.y, Rbat.z);
			DWORD rtw = skp->GetTextWidth(buff, len);
			rtw += 3;
			skp->Text(W - margin - rtw, line * (i + 3), buff, len);
			//skp->Rectangle(margin+ltw,line*(i+11)+1,W-margin-rtw-3,line*(i+12)-1);
			skp->SetPen(NULL);
			skp->SetBrush(myyellow);
			skp->Rectangle(margin + ltw, line * (i + 2) + 2, bl * (W - 2 * margin - rtw - ltw - 3) + margin + ltw, line * (i + 3) - 2);
			skp->SetBrush(NULL);
			skp->SetPen(penwhite);
			//skp->Line(0,middleH-5,W,middleH-5);
			skp->Line(0, line + 3, W, line + 3);
			skp->Line(0, 19 * line - 5, W, 19 * line - 5);


		}



		break;
	}
	case V_GUIDANCE:
	{

		if (!outerVessel) {
			Title(skp, "Multistage2015 MFD- GUIDANCE DISPLAY");

		}
		else {
			len = sprintf_s(outTitle, "MS15-GNC RefVessel:%s", NewRefVesselName);
			skp->SetTextColor(0x00FF00);
			//Title (skp,outTitle);
			skp->Text(margin, 0, outTitle, len);
			skp->SetTextColor(0xFFFFFF);
		}
		//	skp->SetTextColor (0x00FFFF);

		VECTOR3 met = Ms->hms(Ms->MET);

		if (Ms->MET >= 0) {
			len = sprintf_s(buff, "MET: %03.0f:%02.0f:%02.0f", met);
		}
		else {
			len = sprintf_s(buff, "T-: %03.0f:%02.0f:%02.0f", met);
		}
		skp->SetTextAlign(oapi::Sketchpad::LEFT, oapi::Sketchpad::BOTTOM);
		skp->Text(margin, line * 20, buff, len);
		if (Ms->APstat) { len = sprintf_s(buff, "AP: ON"); }
		else { len = sprintf_s(buff, "AP: OFF"); }
		skp->Text(W - margin - skp->GetTextWidth(buff, len), line * 20, buff, len);
		skp->SetBrush(NULL);
		skp->SetPen(penwhite);
		skp->Line(0, line + 3, W, line + 3);
		skp->Line(0, 19 * line - 5, W, 19 * line - 5);
		skp->SetPen(NULL);
		len = sprintf_s(buff, "MET  Comand  params");
		skp->Text(margin, line * 3, buff, len);
		skp->SetPen(pengraydotted);
		skp->Line(0, line * 3, W, line * 3);
		skp->SetPen(NULL);


		for (int i = 1; i <= Ms->nsteps; i++)
		{
			skp->SetFont(smallfont);
			if ((Ms->MET > Ms->Gnc_step[i].time) && (!Ms->Gnc_step[i].executed))
			{
				skp->SetTextColor(0xFFFFFF);

			}
			else {
				skp->SetTextColor(0x9c9c9c);
			}
			if (Ms->Gnc_step[i].executed) { skp->SetTextColor(0x333333); }
			if (SelectedStep == i) { skp->SetTextColor(0x00FFFF); }
			if ((Ms->Gnc_step[i].time < 0) && (Ms->Gnc_step[i].GNC_Comand != CM_NOLINE))
			{
				VECTOR3 tmr = Ms->hms(Ms->Gnc_step[i].time + 1);
				len = sprintf_s(buff, "-%02.0f:%02.0f %s %.1f %.1f %.1f %.1f %.1f %.1f", tmr.y, tmr.z, Ms->Gnc_step[i].Comand, Ms->Gnc_step[i].trval1, Ms->Gnc_step[i].trval2, Ms->Gnc_step[i].trval3, Ms->Gnc_step[i].trval4, Ms->Gnc_step[i].trval5, Ms->Gnc_step[i].trval6);
			}
			else if (Ms->Gnc_step[i].time > 0) {
				VECTOR3 tmr = Ms->hms(Ms->Gnc_step[i].time);
				len = sprintf_s(buff, "%02.0f:%02.0f %s %.1f %.1f %.1f %.1f %.1f %.1f", tmr.y, tmr.z, Ms->Gnc_step[i].Comand, Ms->Gnc_step[i].trval1, Ms->Gnc_step[i].trval2, Ms->Gnc_step[i].trval3, Ms->Gnc_step[i].trval4, Ms->Gnc_step[i].trval5, Ms->Gnc_step[i].trval6);
			}
			else {
				VECTOR3 tmr = _V(0, 0, 0);
				len = sprintf_s(buff, "%02.0f:%02.0f %s %.1f %.1f %.1f %.1f %.1f %.1f", tmr.y, tmr.z, Ms->Gnc_step[i].Comand, Ms->Gnc_step[i].trval1, Ms->Gnc_step[i].trval2, Ms->Gnc_step[i].trval3, Ms->Gnc_step[i].trval4, Ms->Gnc_step[i].trval5, Ms->Gnc_step[i].trval6);
			}
			skp->Text(3, line * (i + 4) + 5, buff, len);

		}



		break;
	}
	case V_CTRL:
	{

		if (!outerVessel) {
			Title(skp, "Multistage2015 MFD - CONTROL DISPLAY");

		}
		else {
			len = sprintf_s(outTitle, "MS15-CTRL RefVessel:%s", NewRefVesselName);
			skp->SetTextColor(0x00FF00);
			//Title (skp,outTitle);
			skp->Text(margin, 0, outTitle, len);
			skp->SetTextColor(0xFFFFFF);
		}
		//	skp->SetTextColor (0x00FFFF);
		VECTOR3 met = Ms->hms(Ms->MET);

		if (Ms->MET >= 0) {
			len = sprintf_s(buff, "MET: %03.0f:%02.0f:%02.0f", met);
		}
		else {
			len = sprintf_s(buff, "T-: %03.0f:%02.0f:%02.0f", met);
		}
		skp->SetTextAlign(oapi::Sketchpad::LEFT, oapi::Sketchpad::BOTTOM);
		skp->Text(margin, line * 20, buff, len);
		if (Ms->APstat) { len = sprintf_s(buff, "AP: ON"); }
		else { len = sprintf_s(buff, "AP: OFF"); }
		skp->Text(W - margin - skp->GetTextWidth(buff, len), line * 20, buff, len);
		skp->SetBrush(NULL);
		skp->SetPen(penwhite);
		skp->Line(0, line + 3, W, line + 3);
		skp->Line(0, 19 * line - 5, W, 19 * line - 5);
		skp->SetPen(NULL);

		VECTOR3 rotlvl;
		Ms->GetAttitudeRotLevel(rotlvl);
		if (rotlvl.x != 0) {
			if (rotlvl.x > 0) {
				skp->SetPen(pengray);
				skp->SetBrush(mydarkgreen);
				skp->Rectangle(W * 0.25, middleH - (int)(rotlvl.x * 0.5 * middleH), W * 0.75, middleH);
				skp->SetPen(NULL);
				skp->SetBrush(NULL);
			}
			else {
				skp->SetPen(pengray);
				skp->SetBrush(mydarkgreen);
				skp->Rectangle(W * 0.25, middleH, W * 0.75, middleH - (int)(rotlvl.x * 0.5 * middleH));
				skp->SetPen(NULL);
				skp->SetBrush(NULL);
			}
		}

		if (rotlvl.y != 0) {
			if (rotlvl.y > 0) {
				skp->SetPen(pengray);
				skp->SetBrush(mydarkblue);
				skp->Rectangle(middleW - (int)(rotlvl.y * 0.5 * middleW), H * 0.25, middleW, H * 0.75);
				skp->SetPen(NULL);
				skp->SetBrush(NULL);
			}
			else {
				skp->SetPen(pengray);
				skp->SetBrush(mydarkblue);
				skp->Rectangle(middleW, H * 0.25, middleW - (int)(rotlvl.y * 0.5 * middleW), H * 0.75);
				skp->SetPen(NULL);
				skp->SetBrush(NULL);
			}
		}

		if (rotlvl.z != 0) {
			if (rotlvl.z > 0) {
				skp->SetPen(pengray);
				skp->SetBrush(mydarkred);
				skp->Rectangle(middleW, H * 0.25, middleW + (int)(rotlvl.z * 0.5 * middleW), middleH);
				skp->Rectangle(middleW - (int)(rotlvl.z * 0.5 * middleW), middleH, middleW, H * 0.75);
				skp->SetPen(NULL);
				skp->SetBrush(NULL);
			}
			else {
				skp->SetPen(pengray);
				skp->SetBrush(mydarkred);
				skp->Rectangle(middleW + (int)(rotlvl.z * 0.5 * middleW), H * 0.25, middleW, middleH);
				skp->Rectangle(middleW, middleH, middleW - (int)(rotlvl.z * 0.5 * middleW), H * 0.75);
				skp->SetPen(NULL);
				skp->SetBrush(NULL);
			}
		}

		skp->SetPen(penwhite);
		skp->Rectangle(margin, 2 * line, W - margin, 18 * line);
		skp->SetPen(NULL);

		skp->SetPen(pengraydotted);
		skp->Line(margin + 1, middleH, W - margin - 1, middleH);
		skp->Line(middleW, 2 * line + 1, middleW, 18 * line - 1);
		skp->Line(0.25 * W, 0.25 * H, 0.75 * W, 0.25 * H);
		skp->Line(0.75 * W, 0.25 * H, 0.75 * W, 0.75 * H);
		skp->Line(0.25 * W, 0.75 * H, 0.75 * W, 0.75 * H);
		skp->Line(0.25 * W, 0.25 * H, 0.25 * W, 0.75 * H);

		int cnv = H / 90;
		double desiredHeading = Ms->GetProperHeading();
		if (desiredHeading < 0) { desiredHeading += 2 * PI; }
		if (Ms->APstat)
		{
			skp->SetPen(pengreenstrong);
			int deltaPitch = (Ms->GetPitch() - Ms->TgtPitch) * DEG * cnv + middleH;

			int deltaHeading = (-Ms->GetHeading() + desiredHeading) * DEG * cnv + middleW;
			skp->Line(deltaHeading - 15, deltaPitch, deltaHeading + 15, deltaPitch);
			skp->Line(deltaHeading, deltaPitch - 15, deltaHeading, deltaPitch + 15);
			skp->SetPen(NULL);
		}

		skp->SetTextColor(0xC4C5C5);
		len = sprintf_s(buff, "Pitch");
		skp->Text(margin + 5, 7 * line, buff, len);
		len = sprintf_s(buff, "Target:%.1f", Ms->TgtPitch * DEG);
		skp->Text(margin + 5, 8 * line, buff, len);
		len = sprintf_s(buff, "Delta:%.1f", (Ms->GetPitch() - Ms->TgtPitch) * DEG);
		skp->Text(margin + 5, 9 * line, buff, len);
		len = sprintf_s(buff, "Heading");
		skp->Text(margin + 5, 12 * line, buff, len);
		len = sprintf_s(buff, "Target:%.0f", desiredHeading * DEG);
		skp->Text(margin + 5, 13 * line, buff, len);
		len = sprintf_s(buff, "Delta:%.0f", (desiredHeading - Ms->GetHeading()) * DEG);
		skp->Text(margin + 5, 14 * line, buff, len);


		if ((Ms->runningPeg) && (Ms->GetAltitude() > Ms->altsteps[3]) && (Ms->currentStage == Ms->NN - 1))
		{
			VECTOR3 PMECO = Ms->hms(Ms->TMeco);
			len = sprintf_s(buff, "Predicted MECO: %02.0f:%02.0f", PMECO.y, PMECO.z);
			skp->SetTextColor(0x00FFFF);
			skp->Text(margin + 10, 16 * line, buff, len);
			skp->SetTextColor(0xFFFFFF);
		}

		if (Ms->AttCtrl)
		{
			len = sprintf_s(buff, "Att Ctrl ON");
		}
		else {
			len = sprintf_s(buff, "Att Ctrl OFF");
		}
		skp->Text(W - margin - 5 - skp->GetTextWidth(buff, len), line * 7, buff, len);
		if (!Ms->PitchCtrl)
		{
			len = sprintf_s(buff, "Pitch Ctrl OFF");
			skp->Text(W - margin - 5 - skp->GetTextWidth(buff, len), line * 8, buff, len);
		}
		if (!Ms->YawCtrl)
		{
			len = sprintf_s(buff, "Yaw Ctrl OFF");
			skp->Text(W - margin - 5 - skp->GetTextWidth(buff, len), line * 9, buff, len);
		}
		if (!Ms->RollCtrl)
		{
			len = sprintf_s(buff, "Roll Ctrl OFF");
			skp->Text(W - margin - 5 - skp->GetTextWidth(buff, len), line * 10, buff, len);
		}


		break;
	}
	case V_PAYLOAD:
	{





		if (!outerVessel) {
			Title(skp, "Multistage2015 MFD - PAYLOAD DISPLAY");
		}
		else {
			len = sprintf_s(outTitle, "MS15-PLD RefVessel:%s", NewRefVesselName);
			skp->SetTextColor(0x00FF00);
			//Title (skp,outTitle);
			skp->Text(margin, 0, outTitle, len);
			skp->SetTextColor(0xFFFFFF);
		}
		//		skp->SetTextColor (0x00FFFF);
		VECTOR3 met = Ms->hms(Ms->MET);

		if (Ms->MET >= 0) {
			len = sprintf_s(buff, "MET: %03.0f:%02.0f:%02.0f", met);
		}
		else {
			len = sprintf_s(buff, "T-: %03.0f:%02.0f:%02.0f", met);
		}
		skp->SetTextAlign(oapi::Sketchpad::LEFT, oapi::Sketchpad::BOTTOM);
		skp->Text(margin, line * 20, buff, len);
		if (Ms->APstat) { len = sprintf_s(buff, "AP: ON"); }
		else { len = sprintf_s(buff, "AP: OFF"); }
		skp->Text(W - margin - skp->GetTextWidth(buff, len), line * 20, buff, len);
		skp->SetPen(penwhite);
		skp->Line(0, line + 3, W, line + 3);
		skp->Line(0, 19 * line - 5, W, 19 * line - 5);
		skp->SetPen(NULL);

		for (int i = Ms->currentPayload; i < Ms->nPayloads; i++)
		{
			skp->SetFont(smallfont);
			len = sprintf_s(buff, "Pld %i - %s - Mass:%.0fkg", i + 1, Ms->payload[i].name, Ms->payload[i].mass);
			skp->Text(margin, line * (i + 4), buff, len);

		}

		break;
	}
	case V_MONITOR:
	{

		UpdateTlmValues();
		skp->SetFont(smallfont);
		len = sprintf_s(buff, "Alt");
		skp->SetPen(pengreen);
		if (SelectedTlm == 1) { skp->SetTextColor(0x00FFFF); }
		else { if (ViewData[VIEWDATA_ALT]) { skp->SetTextColor(0xFFFFFF); skp->Rectangle(W - 2 * margin, line * 3, W - 2 * margin + skp->GetTextWidth(buff, len), line * 4); } else { skp->SetTextColor(0x333333); } }
		skp->Text(W - 2 * margin, line * 3, buff, len);

		skp->SetPen(penpurple);
		len = sprintf_s(buff, "Spd");
		if (SelectedTlm == 2) { skp->SetTextColor(0x00FFFF); }
		else { if (ViewData[VIEWDATA_SPEED]) { skp->SetTextColor(0xFFFFFF); skp->Rectangle(W - 2 * margin, line * 4, W - 2 * margin + skp->GetTextWidth(buff, len), line * 5); } else { skp->SetTextColor(0x333333); } }
		skp->Text(W - 2 * margin, line * 4, buff, len);
		skp->SetPen(penred);
		len = sprintf_s(buff, "Ptc");
		if (SelectedTlm == 3) { skp->SetTextColor(0x00FFFF); }
		else { if (ViewData[VIEWDATA_PITCH]) { skp->SetTextColor(0xFFFFFF); skp->Rectangle(W - 2 * margin, line * 5, W - 2 * margin + skp->GetTextWidth(buff, len), line * 6); } else { skp->SetTextColor(0x333333); } }
		skp->Text(W - 2 * margin, line * 5, buff, len);
		skp->SetPen(penblue);
		len = sprintf_s(buff, "Thr");
		if (SelectedTlm == 4) { skp->SetTextColor(0x00FFFF); }
		else { if (ViewData[VIEWDATA_THRUST]) { skp->SetTextColor(0xFFFFFF); skp->Rectangle(W - 2 * margin, line * 6, W - 2 * margin + skp->GetTextWidth(buff, len), line * 7); } else { skp->SetTextColor(0x333333); } }
		skp->Text(W - 2 * margin, line * 6, buff, len);
		skp->SetPen(pencyan);
		len = sprintf_s(buff, "Vvt");
		if (SelectedTlm == 5) { skp->SetTextColor(0x00FFFF); }
		else { if (ViewData[VIEWDATA_VV]) { skp->SetTextColor(0xFFFFFF); skp->Rectangle(W - 2 * margin, line * 7, W - 2 * margin + skp->GetTextWidth(buff, len), line * 8); } else { skp->SetTextColor(0x333333); } }
		skp->Text(W - 2 * margin, line * 7, buff, len);
		skp->SetPen(penyellow);
		len = sprintf_s(buff, "Acc");
		if (SelectedTlm == 6) { skp->SetTextColor(0x00FFFF); }
		else { if (ViewData[VIEWDATA_ACC]) { skp->SetTextColor(0xFFFFFF); skp->Rectangle(W - 2 * margin, line * 8, W - 2 * margin + skp->GetTextWidth(buff, len), line * 9); } else { skp->SetTextColor(0x333333); } }
		skp->Text(W - 2 * margin, line * 8, buff, len);



		int y0 = line + 6;
		int y1 = 19 * line - 8;


		int hh = y1 - y0;

		skp->SetBrush(NULL);
		int nlines = 5;
		skp->SetPen(pengraydotted);
		for (int i = 1; i < nlines; i++)
		{
			skp->Line(0, y0 + hh / nlines * i, W, y0 + hh / nlines * i);
			skp->Line(W / nlines * i, y0, W / nlines * i, y1);
		}



		double MetStep = Ms->MET / 600;
		skp->SetOrigin(-(int)MetStep * W, y1);
		double cnvy;
		double cnvx = 600 / (double)W;

		if (ViewData[VIEWDATA_ALT]) {
			double maxY1 = ArrayMax(RefMFDtlmAlt, Ms->loadedtlmlines);
			double maxY2 = ArrayMax(MFDtlmAlt, Ms->tlmidx);
			double	maxY;
			if (maxY1 > maxY2) { maxY = maxY1; }
			else { maxY = maxY2; }
			if (maxY > 0) { maxY *= 1.05; }
			else { maxY *= 0.95; }
			double minY1 = ArrayMin(RefMFDtlmAlt, Ms->loadedtlmlines);
			double minY2 = ArrayMin(MFDtlmAlt, Ms->tlmidx);
			double minY;
			if (minY1 < minY2) { minY = minY1; }
			else { minY = minY2; }
			if (minY < 0) { minY *= 1.05; }
			else { minY *= 0.95; }

			if (!AutoRange[VIEWDATA_ALT])
			{
				maxY = RangeMax[VIEWDATA_ALT];
				minY = RangeMin[VIEWDATA_ALT];
			}

			cnvy = (maxY - minY) / (double)hh;
			skp->SetPen(pendarkgreen);
			ScalePoints(RefMFDtlmAlt, cnvx, cnvy, Ms->loadedtlmlines, minY);
			skp->Polyline(RefMFDtlmAlt, Ms->loadedtlmlines);

			skp->SetPen(pengreenstrong);
			ScalePoints(MFDtlmAlt, cnvx, cnvy, Ms->tlmidx, minY);
			skp->Polyline(MFDtlmAlt, Ms->tlmidx);

		}


		if (ViewData[VIEWDATA_SPEED]) {
			double maxY1 = ArrayMax(RefMFDtlmSpeed, Ms->loadedtlmlines);
			double maxY2 = ArrayMax(MFDtlmSpeed, Ms->tlmidx);
			double	maxY;
			if (maxY1 > maxY2) { maxY = maxY1; }
			else { maxY = maxY2; }
			if (maxY > 0) { maxY *= 1.05; }
			else { maxY *= 0.95; }
			double minY1 = ArrayMin(RefMFDtlmSpeed, Ms->loadedtlmlines);
			double minY2 = ArrayMin(MFDtlmSpeed, Ms->tlmidx);
			double minY;
			if (minY1 < minY2) { minY = minY1; }
			else { minY = minY2; }
			if (minY < 0) { minY *= 1.05; }
			else { minY *= 0.95; }

			if (!AutoRange[VIEWDATA_SPEED])
			{
				maxY = RangeMax[VIEWDATA_SPEED] * 10;
				minY = RangeMin[VIEWDATA_SPEED] * 10;
			}

			//cnvy=(maxY-minY)/(double)H;
			cnvy = (maxY - minY) / (double)hh;
			skp->SetPen(pendarkpurple);
			ScalePoints(RefMFDtlmSpeed, cnvx, cnvy, Ms->loadedtlmlines, minY);
			skp->Polyline(RefMFDtlmSpeed, Ms->loadedtlmlines);

			skp->SetPen(penpurplestrong);
			ScalePoints(MFDtlmSpeed, cnvx, cnvy, Ms->tlmidx, minY);
			skp->Polyline(MFDtlmSpeed, Ms->tlmidx);

		}


		if (ViewData[VIEWDATA_PITCH]) {
			double maxY1 = ArrayMax(RefMFDtlmPitch, Ms->loadedtlmlines);
			double maxY2 = ArrayMax(MFDtlmPitch, Ms->tlmidx);
			double	maxY;
			if (maxY1 > maxY2) { maxY = maxY1; }
			else { maxY = maxY2; }
			if (maxY > 0) { maxY *= 1.05; }
			else { maxY *= 0.95; }
			double minY1 = ArrayMin(RefMFDtlmPitch, Ms->loadedtlmlines);
			double minY2 = ArrayMin(MFDtlmPitch, Ms->tlmidx);
			double minY;
			if (minY1 < minY2) { minY = minY1; }
			else { minY = minY2; }
			if (minY < 0) { minY *= 1.05; }
			else { minY *= 0.95; }

			if (!AutoRange[VIEWDATA_PITCH])
			{
				maxY = RangeMax[VIEWDATA_PITCH] * 100;
				minY = RangeMin[VIEWDATA_PITCH] * 100;
			}
			//cnvy=(maxY-minY)/(double)H;
			cnvy = (maxY - minY) / (double)hh;

			skp->SetPen(pendarkred);
			ScalePoints(RefMFDtlmPitch, cnvx, cnvy, Ms->loadedtlmlines, minY);
			skp->Polyline(RefMFDtlmPitch, Ms->loadedtlmlines);

			skp->SetPen(penredstrong);
			ScalePoints(MFDtlmPitch, cnvx, cnvy, Ms->tlmidx, minY);
			skp->Polyline(MFDtlmPitch, Ms->tlmidx);

		}

		if (ViewData[VIEWDATA_VV]) {
			double maxY1 = ArrayMax(RefMFDtlmVv, Ms->loadedtlmlines);
			double maxY2 = ArrayMax(MFDtlmVv, Ms->tlmidx);
			double	maxY;
			if (maxY1 > maxY2) { maxY = maxY1; }
			else { maxY = maxY2; }
			if (maxY > 0) { maxY *= 1.05; }
			else { maxY *= 0.95; }
			double minY1 = ArrayMin(RefMFDtlmVv, Ms->loadedtlmlines);
			double minY2 = ArrayMin(MFDtlmVv, Ms->tlmidx);
			double minY;
			if (minY1 < minY2) { minY = minY1; }
			else { minY = minY2; }
			if (minY < 0) { minY *= 1.05; }
			else { minY *= 0.95; }

			if (!AutoRange[VIEWDATA_VV])
			{
				maxY = RangeMax[VIEWDATA_VV] * 10;
				minY = RangeMin[VIEWDATA_VV] * 10;
			}
			//cnvy=(maxY-minY)/(double)H;
			cnvy = (maxY - minY) / (double)hh;

			skp->SetPen(pendarkcyan);
			ScalePoints(RefMFDtlmVv, cnvx, cnvy, Ms->loadedtlmlines, minY);
			skp->Polyline(RefMFDtlmVv, Ms->loadedtlmlines);

			skp->SetPen(pencyanstrong);
			ScalePoints(MFDtlmVv, cnvx, cnvy, Ms->tlmidx, minY);
			skp->Polyline(MFDtlmVv, Ms->tlmidx);

		}

		if (ViewData[VIEWDATA_THRUST]) {
			double maxY1 = ArrayMax(RefMFDtlmThrust, Ms->loadedtlmlines);
			double maxY2 = ArrayMax(MFDtlmThrust, Ms->tlmidx);
			double	maxY;
			if (maxY1 > maxY2) { maxY = maxY1; }
			else { maxY = maxY2; }
			if (maxY > 0) { maxY *= 1.05; }
			else { maxY *= 0.95; }
			double minY1 = ArrayMin(RefMFDtlmThrust, Ms->loadedtlmlines);
			double minY2 = ArrayMin(MFDtlmThrust, Ms->tlmidx);
			double minY;
			if (minY1 < minY2) { minY = minY1; }
			else { minY = minY2; }
			if (minY < 0) { minY *= 1.05; }
			else { minY *= 0.95; }

			if (!AutoRange[VIEWDATA_THRUST])
			{
				maxY = RangeMax[VIEWDATA_THRUST];
				minY = RangeMin[VIEWDATA_THRUST];
			}
			//cnvy=(maxY-minY)/(double)H;
			cnvy = (maxY - minY) / (double)hh;

			skp->SetPen(pendarkblue);
			ScalePoints(RefMFDtlmThrust, cnvx, cnvy, Ms->loadedtlmlines, minY);
			skp->Polyline(RefMFDtlmThrust, Ms->loadedtlmlines);

			skp->SetPen(penbluestrong);
			ScalePoints(MFDtlmThrust, cnvx, cnvy, Ms->tlmidx, minY);
			skp->Polyline(MFDtlmThrust, Ms->tlmidx);

		}

		if (ViewData[VIEWDATA_ACC]) {
			double maxY1 = ArrayMax(RefMFDtlmAcc, Ms->loadedtlmlines);
			double maxY2 = ArrayMax(MFDtlmAcc, Ms->tlmidx);
			double	maxY;
			if (maxY1 > maxY2) { maxY = maxY1; }
			else { maxY = maxY2; }
			if (maxY > 0) { maxY *= 1.05; }
			else { maxY *= 0.95; }
			double minY1 = ArrayMin(RefMFDtlmAcc, Ms->loadedtlmlines);
			double minY2 = ArrayMin(MFDtlmAcc, Ms->tlmidx);
			double minY;
			if (minY1 < minY2) { minY = minY1; }
			else { minY = minY2; }
			if (minY < 0) { minY *= 1.05; }
			else { minY *= 0.95; }


			if (!AutoRange[VIEWDATA_ACC])
			{
				maxY = RangeMax[VIEWDATA_ACC] * 100;
				minY = RangeMin[VIEWDATA_ACC] * 100;
			}
			//cnvy=(maxY-minY)/(double)H;
			cnvy = (maxY - minY) / (double)hh;

			skp->SetPen(pendarkyellow);
			ScalePoints(RefMFDtlmAcc, cnvx, cnvy, Ms->loadedtlmlines, minY);
			skp->Polyline(RefMFDtlmAcc, Ms->loadedtlmlines);

			skp->SetPen(penyellowstrong);
			ScalePoints(MFDtlmAcc, cnvx, cnvy, Ms->tlmidx, minY);
			skp->Polyline(MFDtlmAcc, Ms->tlmidx);

		}


		skp->SetOrigin(0, 0);
		skp->SetPen(NULL);
		skp->SetBrush(myblack);
		skp->Rectangle(0, 19 * line - 5, W, H);
		skp->Rectangle(0, 0, W, line + 3);
		skp->SetBrush(NULL);



		if (!outerVessel) {
			Title(skp, "Multistage2015 MFD - MONITOR DISPLAY");
		}
		else {
			len = sprintf_s(outTitle, "MS15-MNT RefVessel:%s", NewRefVesselName);
			skp->SetTextColor(0x00FF00);
			//skp->SetFont(NULL);
				//Title (skp,outTitle);
			skp->Text(margin, 0, outTitle, len);
			skp->SetTextColor(0xFFFFFF);
		}
		//	skp->SetTextColor (0x00FFFF);
		VECTOR3 met = Ms->hms(Ms->MET);

		if (Ms->MET >= 0) {
			len = sprintf_s(buff, "MET: %03.0f:%02.0f:%02.0f", met);
		}
		else {
			len = sprintf_s(buff, "T-: %03.0f:%02.0f:%02.0f", met);
		}
		skp->SetTextAlign(oapi::Sketchpad::LEFT, oapi::Sketchpad::BOTTOM);
		skp->Text(margin, line * 20, buff, len);
		if (Ms->APstat) { len = sprintf_s(buff, "AP: ON"); }
		else { len = sprintf_s(buff, "AP: OFF"); }
		skp->Text(W - margin - skp->GetTextWidth(buff, len), line * 20, buff, len);
		skp->SetPen(penwhite);
		skp->Line(0, line + 3, W, line + 3);
		skp->Line(0, 19 * line - 5, W, 19 * line - 5);
		skp->SetPen(NULL);




		break;



































































































	}






































	}






































	return true;
}

















// MFD message parser
OAPI_MSGTYPE Multistage2015_MFD::MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case OAPI_MSG_MFD_OPENED:
		// Our new MFD mode has been selected, so we create the MFD and
		// return a pointer to it.
		return (OAPI_MSGTYPE)(new Multistage2015_MFD(LOWORD(wparam), HIWORD(wparam), (VESSEL*)lparam));
	}
	return 0;
}

bool Multistage2015_MFD::AddStep(char* str)
{

	Ms->VinkaAddStep(str);
	memset(str, '\0', sizeof(str));
	return TRUE;
}

bool Multistage2015_MFD::MFDSetRange(char* str)
{
	//memset( str, '\0', sizeof(str));


	if (str[0] == 'a' || str[0] == 'A') {
		AutoRange[SelectedTlm - 1] = TRUE;
		return true;
	}
	else {
		sscanf(str, "%lf,%lf", &RangeMin[SelectedTlm - 1], &RangeMax[SelectedTlm - 1]);
		AutoRange[SelectedTlm - 1] = FALSE;
		return true;
	}

	return false;
}

bool Multistage2015_MFD::MFDLoadTlmFile(char* str)
{
	Ms->parseTelemetryFile(str);
	return TRUE;
}

bool Multistage2015_MFD::InputPMCInterval(char* str)
{
	double newint;
	sscanf(str, "%lf", &newint);
	Ms->SetPegMajorCycleInterval(newint);

	return TRUE;
}
bool Multistage2015_MFD::InputAltSteps(char* str)
{
	double newsteps[4];
	sscanf(str, "%lf,%lf,%lf,%lf", &newsteps[0], &newsteps[1], &newsteps[2], &newsteps[3]);

	Ms->SetNewAltSteps(newsteps[0], newsteps[1], newsteps[2], newsteps[3]);

	return TRUE;
}

bool Multistage2015_MFD::InputNewPitchLimit(char* str)
{
	double newpitchlimit;
	sscanf(str, "%lf", &newpitchlimit);
	Ms->SetPegPitchLimit(newpitchlimit * RAD);

	return TRUE;
}



bool Multistage2015_MFD::InputNewRefVessel(char* str)
{
	sprintf_s(NewRefVesselName, str);
	if (oapiIsVessel(oapiGetVesselByName(NewRefVesselName)))
	{
		outerVessel = TRUE;
		if (ValidVessel()) {
			CurrentView = V_INIT;
			InvalidateButtons();
			return TRUE;
		}
		else {
			outerVessel = FALSE;
			return FALSE;
		}
	}
	else {
		return FALSE;
	}
}

void Multistage2015_MFD::StoreStatus(void) const
{

	return;
}
void Multistage2015_MFD::RecallStatus(void)
{

	return;
}

//AscentMFD::SavePrm AscentMFD::saveprm = {0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
//Multistage2015_MFD::globalvar Multistage2015_MFD::global ={NULL,0};

DLLCLBK void opcFocusChanged(OBJHANDLE New, OBJHANDLE Old)
{

}