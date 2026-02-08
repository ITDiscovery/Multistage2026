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
// Multistage2015_MFD.h
//
// 
// ==============================================================
#include "..//..//..//sources//Multistage2015//Multistage2015.h"
#ifndef __Multistage2015_MFD_H
#define __Multistage2015_MFD_H

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE

#define V_INIT 0
#define V_VEHICLE 1
#define V_GUIDANCE 2
#define V_PAYLOAD 3
#define V_MONITOR 5
#define V_CTRL 4
#define V_NOCLASS 6
#define V_BATTS 11
#define V_THRUST 12

#define VIEWDATA_ALT 0
#define VIEWDATA_SPEED 1
#define VIEWDATA_PITCH 2
#define VIEWDATA_THRUST 3
#define VIEWDATA_VV 4
#define VIEWDATA_ACC 5
#include "..//..//include//Orbitersdk.h"
class Multistage2015_MFD : public MFD2 {
public:
	Multistage2015_MFD(DWORD w, DWORD h, VESSEL* vessel);
	~Multistage2015_MFD();
	bool ConsumeButton(int bt, int event);
	char* ButtonLabel(int bt);
	int ButtonMenu(const MFDBUTTONMENU** menu) const;
	bool Update(oapi::Sketchpad* skp);
	static OAPI_MSGTYPE MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam);
	void StoreStatus(void) const;
	void RecallStatus(void);
	bool AddStep(char* str);
	bool MFDSetRange(char* str);
	bool MFDLoadTlmFile(char* str);
	bool InputPMCInterval(char* str);
	bool InputAltSteps(char* str);
	bool InputNewPitchLimit(char* str);
	bool InputNewRefVessel(char* str);

private:
	OBJHANDLE Hvessel;
	int CurrentView;
	oapi::Brush* mygreen;
	oapi::Brush* mydarkgreen;
	oapi::Brush* mydarkblue;
	oapi::Brush* myyellow;
	oapi::Brush* mydarkred;
	oapi::Brush* myblack;
	oapi::Pen* pengreen;
	oapi::Pen* pengreenstrong;
	oapi::Pen* penwhite;
	oapi::Pen* penwhitestrong;
	oapi::Pen* penwhitedotted;
	oapi::Pen* pengraydotted;
	oapi::Pen* pengray;
	oapi::Pen* pendarkgreen;
	oapi::Pen* penyellow;
	oapi::Pen* penyellowstrong;
	oapi::Pen* pendarkyellow;
	oapi::Pen* penblue;
	oapi::Pen* penbluestrong;
	oapi::Pen* pendarkblue;
	oapi::Pen* penred;
	oapi::Pen* penredstrong;
	oapi::Pen* pendarkred;
	oapi::Pen* penpurple;
	oapi::Pen* penpurplestrong;
	oapi::Pen* pendarkpurple;
	oapi::Pen* pencyan;
	oapi::Pen* pencyanstrong;
	oapi::Pen* pendarkcyan;

	// bool activestep[150];
	oapi::Font* smallfont;
	oapi::Font* verysmallfont;
	oapi::Font* smallquarterfont;
	Multistage2015* Ms;
	bool ValidVessel();
	int SelectedStep;
	int SelectedInfoItem;

	oapi::IVECTOR2 MFDtlmAlt[TLMSECS];
	oapi::IVECTOR2 MFDtlmSpeed[TLMSECS];
	oapi::IVECTOR2 MFDtlmPitch[TLMSECS];
	oapi::IVECTOR2 MFDtlmThrust[TLMSECS];
	oapi::IVECTOR2 MFDtlmMass[TLMSECS];
	oapi::IVECTOR2 MFDtlmVv[TLMSECS];
	oapi::IVECTOR2 MFDtlmAcc[TLMSECS];

	oapi::IVECTOR2 RefMFDtlmAlt[TLMSECS];
	oapi::IVECTOR2 RefMFDtlmSpeed[TLMSECS];
	oapi::IVECTOR2 RefMFDtlmPitch[TLMSECS];
	oapi::IVECTOR2 RefMFDtlmThrust[TLMSECS];
	oapi::IVECTOR2 RefMFDtlmMass[TLMSECS];
	oapi::IVECTOR2 RefMFDtlmVv[TLMSECS];
	oapi::IVECTOR2 RefMFDtlmAcc[TLMSECS];

	bool ViewData[6]; //Mass not viewed, 0=alt,1=speed,2=pitch,3=thrust,4=Vv,5=Acc

	double ArrayMin(oapi::IVECTOR2* Array, int uptopoint);
	double ArrayMax(oapi::IVECTOR2* Array, int uptopoint);

	oapi::IVECTOR2 ScalePoints(oapi::IVECTOR2 input[], double xscale, double yscale, int uptopoint, int MinY);

	void UpdateTlmValues();
	void ToggleViewData(int data);
	int SelectedTlm;

	bool AutoRange[6];
	double RangeMin[6];
	double RangeMax[6];

	bool outerVessel;
	char NewRefVesselName[128];
	char outTitle[128];
	//bool InputStep(void *id, char *str, void *usrdata);
//protected:
	//oapi::Font *smallfont;

	/* static struct globalvar{
	OBJHANDLE hvessel;
	int CV;
	}global;
	*/

};

#endif // !__Multistage2015_MFD_H