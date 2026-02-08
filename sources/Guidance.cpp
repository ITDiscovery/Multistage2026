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
//						MultiStage2015
//                  
//					       By Fred18
//                  
//
// Guidance.cpp
// ==============================================================


#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE

#include <math.h>
#include <stdio.h>
#include "orbitersdk.h"
//############################################################################//
#include "Multistage2015.h"






double Multistage2015::finalv(double Abside, double Apo,double Peri){
	return sqrt(mu*(2/(Abside+rt)-1/(0.5*(Apo+Peri+2*rt))));
}

double Multistage2015::CalcAzimuth()
{double heading;

double lon,lat,rad;
GetEquPos(lon,lat,rad);
const double wearth=2*PI/oapiGetPlanetPeriod(GetSurfaceRef()); 
	double EastVel=abs((oapiGetSize(GetSurfaceRef())+GetAltitude())*wearth*cos(lat));
	VECTOR3 hspd;
//	GetHorizonAirspeedVector(hspd);
	GetGroundspeedVector(FRAME_HORIZON,hspd);
	EastVel+=(hspd.x);
	double NorthVel=hspd.z;
	
	if(GetAltitude()>5000)
	{
		if(SignOf(tgtinc)!=SignOf(NorthVel)) ///CHECK FOR THE PARTICULAR AMBIGUITY
		{
			tgtinc=-tgtinc;
		}	
	}

	heading=atan2((finalv(tgtabside,tgtapo,tgtperi))*(sin(asin(cos(tgtinc)/cos(lat))))-EastVel,((finalv(tgtabside,tgtapo,tgtperi))*(cos(asin(cos(tgtinc)/cos(lat))))-abs(NorthVel)));
	
	heading+=(90*RAD-heading)*(1-SignOf(tgtinc));
	

	return heading;
}
double Multistage2015::GetProperHeading(){

	double heading;
	if(!runningPeg)
	{
		if(GetAltitude()<3000){ //JUST A SAFETY IN CASE ROLL HAS SOMETHING BAD
		heading=VinkaAzimuth;
		}else{
		oapiGetHeading(GetHandle(),&heading);
		}
	}else{
	
	double lon,lat,rad;
	GetEquPos(lon,lat,rad);

	if(abs(tgtinc)<abs(lat)){
		if(lat<0){	tgtinc=abs(lat);	}else{	tgtinc=-abs(lat);	}
							}
	
		heading=CalcAzimuth();
	}
	return heading;
}

double Multistage2015::GetVPerp(){

	
	OBJHANDLE hearth=GetSurfaceRef();
	VECTOR3 relvel,loc_rvel,horvel,glob_vpos;
	GetRelativeVel(hearth,relvel);
	GetGlobalPos(glob_vpos);
	Global2Local((relvel + glob_vpos), loc_rvel);
 
	HorizonRot(loc_rvel,horvel);
	//horvel.x=0;
	//horvel.z=0;
	
	return horvel.y;
}



double Multistage2015::GetProperRoll(double RequestedRoll=0){
	if(RequestedRoll<0){RequestedRoll+=2*PI;}
	double GB=GetBank();
	if(GB<0){GB+=2*PI;}
	double roll;
	roll=GB-RequestedRoll;
	if(roll>PI){roll-=2*PI;}
	return roll;
}

//Attitude Function! Got the inspiration from the OrientForBurn of Hlynkacg

void Multistage2015::Attitude(double pitch,double roll, double heading,double pitchrate=2, double rollrate=8, double yawrate=5){
	if(AttCtrl){
	double pdb;								//Deadband for rates, way higher if Time Acceleration is applied
//	if(oapiGetTimeAcceleration()>1){
	//	pdb=0.1;
	//}else{
		pdb=0.1;
	//}


VECTOR3 PMI;
	GetPMI(PMI);
	double ReqTime=oapiGetSimStep()*oapiGetTimeAcceleration();
	VECTOR3 ReqAngularAcc=_V(0,0,0);
	VECTOR3 ReqTorque=_V(0,0,0);
	
	VECTOR3 thetain=_V(cos(pitch)*sin(heading),sin(pitch),cos(pitch)*cos(heading));  // Getting the proper input vector from pitch and heading
	
	VECTOR3 tgtVector;
	HorizonInvRot(thetain,tgtVector);


	normalise (tgtVector);
	VECTOR3 ThrustVector = {0,0,1}; 
	VECTOR3 input = crossp(tgtVector,ThrustVector);
	
	input.z=GetProperRoll(roll);

	
	if(abs(input.z)>1){
		//input.z=input.z/abs(input.z);
		input.z=SignOf(input.z);
	}
	VECTOR3 aVel; 
	GetAngularVel(aVel);
	//aVel=operator/(aVel,oapiGetTimeAcceleration());

	double PitchV = aVel.x*DEG;	
	double RollV = aVel.z*DEG;	
	double YawV = aVel.y*DEG;	
	
	// PITCH
	double CurrentRate = PitchV;
	double CommandedRate = asin(input.x)*DEG;
	CommandedRate=((min(abs(CommandedRate),pitchrate))*SignOf(CommandedRate));
	double pDelta = (CommandedRate - CurrentRate); 
	

	VECTOR3 tgtrotlevel;
	GetAttitudeRotLevel(tgtrotlevel);


	if(PitchCtrl){
	if(abs(pDelta)>pdb){
					
			ReqAngularAcc.x=pDelta*RAD/ReqTime;
			ReqTorque.x=PMI.x*GetMass()*ReqAngularAcc.x;
			tgtrotlevel.x=min(abs(ReqTorque.x/MaxTorque.x),1)*SignOf(ReqTorque.x);
	
		}else{tgtrotlevel.x=0;}
		SetAttitudeRotLevel(0,tgtrotlevel.x);
	}
	
	// ROLL
	CurrentRate = RollV;
	CommandedRate = asin(input.z)*DEG;
	CommandedRate=((min(abs(CommandedRate),rollrate))*SignOf(CommandedRate));
	double rDelta = (CommandedRate - CurrentRate); 
	
	if(RollCtrl){
	

if(abs(rDelta)>pdb){
			ReqAngularAcc.z=rDelta*RAD/ReqTime;
			ReqTorque.z=PMI.z*GetMass()*ReqAngularAcc.z;
			tgtrotlevel.z=min(abs(ReqTorque.z/MaxTorque.z),1)*SignOf(ReqTorque.z);
		}else{tgtrotlevel.z=0;}
		SetAttitudeRotLevel(2,tgtrotlevel.z);
	}
	
	// YAW
	CurrentRate = YawV; 
	CommandedRate = asin(input.y)*DEG;
	CommandedRate=((min(abs(CommandedRate),yawrate))*SignOf(CommandedRate));
	double yDelta = (CommandedRate - CurrentRate); 
	
	if(YawCtrl){
	
		if(abs(yDelta)>pdb){
		ReqAngularAcc.y=yDelta*RAD/ReqTime;
		ReqTorque.y=PMI.y*GetMass()*ReqAngularAcc.y;
		tgtrotlevel.y=min(abs(ReqTorque.y/MaxTorque.y),1)*SignOf(ReqTorque.y);
		}else{tgtrotlevel.y=0;}
		SetAttitudeRotLevel(1,tgtrotlevel.y);
	}



	}
	

	return;
}

void Multistage2015::ToggleAP()
{
	if(APstat){
	APstat=FALSE;
	}else{
	APstat=TRUE;
	}
	return;
}

void Multistage2015::ToggleAttCtrl(bool Pitch, bool Yaw, bool Roll)
{
	if((Pitch)&&(Yaw)&&(Roll)){
	if(AttCtrl)
	{
		AttCtrl=FALSE;
	}else{
		AttCtrl=TRUE;
	}
	}else if((Pitch)&&(!Yaw)&&(!Roll))
	{
		if(PitchCtrl)
		{
			PitchCtrl=FALSE;
		}else{
			PitchCtrl=TRUE;
		}
	}else if((!Pitch)&&(Yaw)&&(!Roll))
	{
		if(YawCtrl)
		{
			YawCtrl=FALSE;
		}else{
			YawCtrl=TRUE;
		}
	}else if((!Pitch)&&(!Yaw)&&(Roll))
	{
		if(RollCtrl)
		{
			RollCtrl=FALSE;
		}else{
			RollCtrl=TRUE;
		}
	}


}

void Multistage2015::killAP(){
	SetThrusterGroupLevel(THGROUP_MAIN,0);
	SetThrusterGroupLevel(THGROUP_ATT_PITCHUP,0);
	SetThrusterGroupLevel(THGROUP_ATT_PITCHDOWN,0);
	SetThrusterGroupLevel(THGROUP_ATT_YAWLEFT,0);
	SetThrusterGroupLevel(THGROUP_ATT_YAWRIGHT,0);
	SetThrusterGroupLevel(THGROUP_ATT_BANKRIGHT,0);
	SetThrusterGroupLevel(THGROUP_ATT_BANKLEFT,0);
	ActivateNavmode(NAVMODE_KILLROT);
	return;
}

void Multistage2015::VinkaUpdateRollTime(){

	Gnc_step[VinkaFindRoll()].time_fin=Gnc_step[VinkaFindFirstPitch()].time-1;

	return;
}
void Multistage2015::VinkaComposeGNCSteps(){
	for(int i=0;i<=nsteps;i++){
		Gnc_step[i].executed=FALSE;
		 for(int z=0;z<128;z++){
		  Gnc_step[i].Comand[z]=tolower(Gnc_step[i].Comand[z]);
														}
		
		if(strcmp(Gnc_step[i].Comand,"engine")==0){
			Gnc_step[i].GNC_Comand=CM_ENGINE;
		}else if(strcmp(Gnc_step[i].Comand,"roll")==0){
			Gnc_step[i].GNC_Comand=CM_ROLL;
		}else if(strcmp(Gnc_step[i].Comand,"pitch")==0){
			Gnc_step[i].GNC_Comand=CM_PITCH;
		}else if(strcmp(Gnc_step[i].Comand,"fairing")==0){
			Gnc_step[i].GNC_Comand=CM_FAIRING;
		}else if(strcmp(Gnc_step[i].Comand,"les")==0){
			Gnc_step[i].GNC_Comand=CM_LES;
		}else if(strcmp(Gnc_step[i].Comand,"disablepitch")==0){
			Gnc_step[i].GNC_Comand=CM_DISABLE_PITCH;
		}else if(strcmp(Gnc_step[i].Comand,"disableroll")==0){
			Gnc_step[i].GNC_Comand=CM_DISABLE_ROLL;
		}else if(strcmp(Gnc_step[i].Comand,"disablejettison")==0){
			Gnc_step[i].GNC_Comand=CM_DISABLE_JETTISON;
		}else if(strcmp(Gnc_step[i].Comand,"playsound")==0){
			Gnc_step[i].GNC_Comand=CM_PLAY;
		}else if(strcmp(Gnc_step[i].Comand,"jettison")==0){
			Gnc_step[i].GNC_Comand=CM_JETTISON;
		}else if(strcmp(Gnc_step[i].Comand,"target")==0){
			Gnc_step[i].GNC_Comand=CM_TARGET;
		}else if(strcmp(Gnc_step[i].Comand,"aoa")==0){
			Gnc_step[i].GNC_Comand=CM_AOA;
		}else if(strcmp(Gnc_step[i].Comand,"attitude")==0){
			Gnc_step[i].GNC_Comand=CM_ATTITUDE;
		}else if(strcmp(Gnc_step[i].Comand,"spin")==0){
			Gnc_step[i].GNC_Comand=CM_SPIN;
		}else if(strcmp(Gnc_step[i].Comand,"inverse")==0){
			Gnc_step[i].GNC_Comand=CM_INVERSE;
		}else if(strcmp(Gnc_step[i].Comand,"engineout")==0){
			Gnc_step[i].GNC_Comand=CM_ENGINEOUT;
		}else if(strcmp(Gnc_step[i].Comand,"orbit")==0){
			Gnc_step[i].GNC_Comand=CM_ORBIT;
		}else if(strcmp(Gnc_step[i].Comand,"defap")==0){
			Gnc_step[i].GNC_Comand=CM_DEFAP;
		}else if(strcmp(Gnc_step[i].Comand,"glimit")==0){
			Gnc_step[i].GNC_Comand=CM_GLIMIT;
		}else if(strcmp(Gnc_step[i].Comand,"destroy")==0){
			Gnc_step[i].GNC_Comand=CM_DESTROY;
		}else if(strcmp(Gnc_step[i].Comand,"explode")==0){
			Gnc_step[i].GNC_Comand=CM_EXPLODE;
		}else if(strcmp(Gnc_step[i].Comand,"noline")==0){
			Gnc_step[i].GNC_Comand=CM_NOLINE;
		}

//CM_ROLL,CM_PITCH,CM_ENGINE,CM_FAIRING,CM_DISABLE,CM_PLAY,CM_JETTISON,CM_TARGET,CM_AOA,CM_ATTITUDE,CM_SPIN

		Gnc_step[0].time_fin=-10000;

		switch(Gnc_step[i].GNC_Comand){
		
		case CM_ENGINE:
			Gnc_step[i].val_init=Gnc_step[i].trval1;
			Gnc_step[i].val_fin=Gnc_step[i].trval2;
			if(Gnc_step[i].val_fin==0){Gnc_step[i].val_fin=-1;}
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].duration=Gnc_step[i].trval3;
			if(Gnc_step[i].duration==0){Gnc_step[i].duration=0.01;}
			Gnc_step[i].time_fin=Gnc_step[i].time_init+Gnc_step[i].duration;
			break;

		case CM_ROLL:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].val_init=Gnc_step[i].trval2;
			Gnc_step[i].val_fin=Gnc_step[i].trval4;
			Gnc_step[i].duration=60;//Gnc_step[i].trval1;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+Gnc_step[i].duration;
			
			//VinkaAzimuth=Gnc_step[i].trval3*RAD;
			//VinkaMode=Gnc_step[i].trval5;
		
			
			break;
		
		case CM_PITCH:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].val_init=Gnc_step[i].trval1;
			Gnc_step[i].val_fin=Gnc_step[i].trval2;
			Gnc_step[i].duration=Gnc_step[i].trval3;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+Gnc_step[i].duration;
			break;

			
		case CM_FAIRING:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].val_init=Gnc_step[i].trval1*1000;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+1000;
			break;
		case CM_LES:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].val_init=Gnc_step[i].trval1*1000;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+1000;
			break;
		case CM_DISABLE_JETTISON:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+2;
			break;
		case CM_DISABLE_PITCH:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+2;
			break;
		case CM_DISABLE_ROLL:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+2;
			break;
		case CM_JETTISON:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+2;
			break;
		case CM_AOA:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].duration=Gnc_step[i].trval2;
			
			if(Gnc_step[i].duration<=0){
		
				if(i<nsteps){
					Gnc_step[i].time_fin=Gnc_step[i+1].time;
				}else{
					Gnc_step[i].time_fin=Gnc_step[i].time_init+60;
				}
			}else{
				Gnc_step[i].time_fin=Gnc_step[i].time_init+Gnc_step[i].duration;
			}

			
			Gnc_step[i].val_init=Gnc_step[i].trval1*RAD;
			
			
			break;
		case CM_ATTITUDE:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].duration=Gnc_step[i].trval4;
			if(Gnc_step[i].duration<=0){
				
				if(i<nsteps){
					Gnc_step[i].time_fin=Gnc_step[i+1].time;
				}else{
					Gnc_step[i].time_fin=Gnc_step[i].time_init+60;
				}
			}else{
				Gnc_step[i].time_fin=Gnc_step[i].time_init+Gnc_step[i].duration;
			}
			break;
		case CM_SPIN:
			Gnc_step[i].time_init=Gnc_step[i].time;

			if(i<nsteps){
					Gnc_step[i].time_fin=Gnc_step[i+1].time;
				}else{
					Gnc_step[i].time_fin=Gnc_step[i].time_init+60;
				}
			Gnc_step[i].val_init=Gnc_step[i].trval1;
			break;
		case CM_TARGET:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+5*60;
			Gnc_step[i].val_init=Gnc_step[i].trval1;
			break;
		case CM_INVERSE:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+2;
			break;
		case CM_ENGINEOUT:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+1;
			Gnc_step[i].val_init=Gnc_step[i].trval1;
			break;
		case CM_ORBIT:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+10000;
			tgtapo=Gnc_step[i].trval2*1000;
			tgtperi=Gnc_step[i].trval1*1000;
			tgtinc=Gnc_step[i].trval3*RAD;
			VinkaMode=Gnc_step[i].trval4;
			if(VinkaMode==0){VinkaMode=1;}
			GT_InitPitch=Gnc_step[i].trval5*RAD;
			tgtabside=Gnc_step[i].trval6*1000;
			wPeg=TRUE;
			CalculateTargets();
			sprintf(logbuff,"%s: Orbit Call Found! Targets: Apogee:%.1f Perigee:%.1f Inclination:%.1f Mode:%.1f GT initial Pitch: %.1f Abside:%.1f",GetName(),tgtapo,tgtperi,tgtinc*DEG,VinkaMode,GT_InitPitch*DEG,tgtabside);
			oapiWriteLog(logbuff);
			break;
		case CM_PLAY:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+2;
			for(int s=0;s<57;s++){
				if(Ssound.GncStepSound[s]==-5){
					Ssound.GncStepSound[s]=i;
					break;
				}
			}
			sprintf(logbuff,"%s: Sound found @step %i",GetName(),i);
			oapiWriteLog(logbuff);
			break;
		case CM_DEFAP:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+2;
			break;
		case CM_GLIMIT:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+250;
			break;
		case CM_DESTROY:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+2;
			break;
		case CM_EXPLODE:
			Gnc_step[i].time_init=Gnc_step[i].time;
			Gnc_step[i].time_fin=Gnc_step[i].time_init+2;
			break;
		case CM_NOLINE:
			
			break;

		}
	}
	if(!wPeg){
	VinkaUpdateRollTime();
	}
	return;
}
double Multistage2015::VinkaFindEndTime(){
	double EndTime=0;
	
	for(int q=0;q<=nsteps;q++){
		if(Gnc_step[q].time_fin>EndTime){
			EndTime=Gnc_step[q].time_fin;
		}
	}
	
	return EndTime;
}

int Multistage2015::VinkaGetStep(double met){
	int n=0;
	for(int i=1;i<=nsteps;i++){
		if((met>=Gnc_step[i].time)&&(Gnc_step[i].GNC_Comand!=CM_NOLINE)){
			n+=1;
		}
	}
	return n;
}
void Multistage2015::VinkaCheckInitialMet(){
	if(Configuration==0){
		MET=Gnc_step[1].time;
	}
	if(nsteps==0)
	{
		MET=-1;
	}
	return;

}
int Multistage2015::VinkaFindRoll(){
	int n=0;
	for(int i=1;i<=nsteps;i++){
	n+=1;
		if(Gnc_step[i].GNC_Comand==CM_ROLL){
			
			break;
		}
}
return n;
}
int Multistage2015::VinkaFindFirstPitch(){
int n=0;
for(int i=1;i<=nsteps;i++){
	n+=1;
		if(Gnc_step[i].GNC_Comand==CM_PITCH){
			
			break;
		}
}
return n;

}
void Multistage2015::VinkaConsumeStep(int step){

	if(Gnc_step[step].time_fin<=MET){Gnc_step[step].executed=TRUE;}

	if(Gnc_step[step].executed==FALSE){

		switch(Gnc_step[step].GNC_Comand){
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
			if((wFairing==1)&&(GetAltitude()>=Gnc_step[step].val_init)){
			Jettison(TFAIRING,0);
			Gnc_step[step].time_fin=MET;
			Gnc_step[step].executed=TRUE;
			}
			
		break;
		case CM_LES:
			if((wLes==TRUE)&&(GetAltitude()>=Gnc_step[step].val_init)){
			Jettison(TLES,0);
			Gnc_step[step].time_fin=MET;
			Gnc_step[step].executed=TRUE;
			}
			
		break;
		case CM_DISABLE_JETTISON:
			AJdisabled=TRUE;
			Gnc_step[step].executed=TRUE;
		break;
		case CM_DISABLE_ROLL:
			rolldisabled=TRUE;
			Gnc_step[step].executed=TRUE;
		break;
		case CM_DISABLE_PITCH:
			pitchdisabled=TRUE;
			Gnc_step[step].executed=TRUE;
		break;
		case CM_JETTISON:
		char kstate[256];
		for(int i=0;i<256;i++) kstate[i]=0x00;
		kstate[OAPI_KEY_J]=0x80;
		SendBufferedKey(OAPI_KEY_J,true,kstate);
		Gnc_step[step].executed=TRUE;
		break;
		case CM_AOA:
		double DesiredPitch;
		DesiredPitch=GetPitch()-VinkaMode*GetAOA()+VinkaMode*Gnc_step[step].val_init;
		TgtPitch=DesiredPitch;
		Attitude(DesiredPitch,(0.5*(1-VinkaMode)*PI),GetProperHeading());
		break;
		case CM_ATTITUDE:
		TgtPitch=Gnc_step[step].trval1*RAD;
		Attitude(Gnc_step[step].trval1*RAD,-(Gnc_step[step].trval3*RAD),Gnc_step[step].trval2*RAD);
		break;
		case CM_SPIN:
			spinning=TRUE;
			VECTOR3 AngVel;
			GetAngularVel(AngVel);
		
			if((fabs(fabs(AngVel.z)-fabs(Gnc_step[step].val_init*RAD))>0.1)||(AngVel.z/Gnc_step[step].val_init<0)){
				TgtPitch=GetPitch();
				Attitude(GetPitch(),GetBank()-Gnc_step[step].val_init,GetProperHeading(),0,fabs((GetBank()*DEG+Gnc_step[step].val_init)),0);
			}else{
				spinning=FALSE;
				Gnc_step[step].executed=TRUE;}
		break;
			case CM_TARGET:
				double ApR,Rt,ApD;
				GetApDist(ApR);
				Rt=oapiGetSize(GetSurfaceRef());
				ApD=ApR-Rt;
				ApD=ApD/1000;
				if(ApD>=Gnc_step[step].val_init){
					Gnc_step[step].time_fin=MET;
					Gnc_step[step].executed=TRUE;
				}
		break;
			case CM_INVERSE:
		VinkaMode*=-1;
		Gnc_step[step].executed=TRUE;
		break;
			case CM_ENGINEOUT:
		if((Gnc_step[step].val_init>0)&&(Gnc_step[step].val_init<=(double)stage.at(currentStage).nEngines))
		{
			SetThrusterLevel(stage.at(currentStage).th_main_h[(int)Gnc_step[step].val_init-1],0);
		}else{
			SetThrusterLevel(stage.at(currentStage).th_main_h[stage.at(currentStage).nEngines-1],0);
		}
		Gnc_step[step].executed=TRUE;
		break;
			case CM_PLAY:
//		PlayVesselWave(MyID,step);
		//PlaySound(TEXT(Gnc_step[step].trchar),NULL,SND_FILENAME|SND_ASYNC);
		Gnc_step[step].executed=TRUE;
		break;
			case CM_ORBIT:
				if(Configuration==1)
				{
					runningPeg=TRUE;
		if(GetAltitude()<altsteps[0]){
			TgtPitch=90*RAD-Misc.VerticalAngle;
			Attitude(90*RAD-Misc.VerticalAngle,GetBank(),GetHeading(),8,0,5);
			//SetAngularVel(_V(0,0,0));

		}else if((GetAltitude()>=altsteps[0])&&(GetAltitude()<altsteps[1])){
		//Attitude(90*RAD,0.5*PI,VinkaAzimuth*(0.5*(1-VinkaMode)),8,20,5);

		TgtPitch=89.9*RAD-Misc.VerticalAngle;
		Attitude(89.9*RAD-Misc.VerticalAngle,(0.5*(1-VinkaMode)*PI),GetProperHeading(),8,20,5);
		}else if((GetAltitude()>=altsteps[1])&&(GetAltitude()<altsteps[2])){
		TgtPitch=GT_InitPitch;

		double deltaAltitude=altsteps[2]-GetAltitude();
		double deltaTime=sqrt((2*deltaAltitude)/(1.5*getabsacc()-g0));

		double PitchRate=abs((GetPitch()-GT_InitPitch)/deltaTime);
		Attitude(GT_InitPitch,(0.5*(1-VinkaMode)*PI),GetProperHeading(),PitchRate*DEG,20,PitchRate*DEG);








		}else if((GetAltitude()>=altsteps[2])&&(GetAltitude()<altsteps[3])){
		DesiredPitch=GetPitch()-VinkaMode*GetAOA()+(-0.7*RAD);
		TgtPitch=DesiredPitch;
		Attitude(DesiredPitch,(0.5*(1-VinkaMode)*PI),GetProperHeading(),8,5,8);
		}else{
		if(GetThrusterGroupLevel(THGROUP_MAIN)>0.1)
				{
				PEG();
				CutoffCheck();
					if(CutoffCheck()==TRUE)
					{
						Gnc_step[step].time_fin=MET+1;
						Gnc_step[step].executed=TRUE;
					}
				}		
			}
				}

		break;
			case CM_DEFAP:
				ActivateNavmode((int)Gnc_step[step].trval1);
				Gnc_step[step].executed=TRUE;
				break;
			case CM_GLIMIT:
				if(getacc()>Gnc_step[step].trval1*g0)
				{
					double lev=GetThrusterGroupLevel(THGROUP_MAIN);
					SetThrusterGroupLevel(THGROUP_MAIN,lev-0.001);
				}
				break;
			case CM_DESTROY:
				Gnc_step[step].executed=TRUE;
				oapiDeleteVessel(GetHandle());
				break;
			case CM_EXPLODE:
				Gnc_step[step].executed=TRUE;
				boom();
				break;
			case CM_NOLINE:
		break;

		}
	
	
	}
	return;
}

void Multistage2015::VinkaEngine(int step){
	double DesiredEngineLevel=(Gnc_step[step].val_init+(Gnc_step[step].val_fin-Gnc_step[step].val_init)*(MET-Gnc_step[step].time_init)/(Gnc_step[step].time_fin-Gnc_step[step].time_init))/100;
	SetThrusterGroupLevel(THGROUP_MAIN,DesiredEngineLevel);
	return;
}


void Multistage2015::VinkaRoll(int step){
	//double DesiredPitch=(Gnc_step[step].val_init+(Gnc_step[step].val_fin-Gnc_step[step].val_init)*(MET-Gnc_step[step].time_init)/((Gnc_step[VinkaFindFirstPitch()].time-1)-Gnc_step[step].time_init))*RAD;//88*RAD;
	double DesiredPitch=(Gnc_step[step].val_init+(Gnc_step[step].val_fin-Gnc_step[step].val_init)*(MET-Gnc_step[step].time_init)/((Gnc_step[step].time_fin)-Gnc_step[step].time_init))*RAD;//88*RAD;
	TgtPitch=DesiredPitch;
	double RollRate=180/((Gnc_step[step].time_fin-1)-Gnc_step[step].time_init);
	VinkaAzimuth=Gnc_step[step].trval3*RAD;
	VinkaMode=Gnc_step[step].trval5;
	Attitude(DesiredPitch,(0.5*(1-VinkaMode)*PI),VinkaAzimuth,2,RollRate,5);

	return;
}

void Multistage2015::VinkaPitch(int step){
	double DesiredPitch=(Gnc_step[step].val_init+(Gnc_step[step].val_fin-Gnc_step[step].val_init)*(MET-Gnc_step[step].time_init)/(Gnc_step[step].time_fin-Gnc_step[step].time_init))*RAD;//Gnc_step[step].val_fin*RAD;
	TgtPitch=DesiredPitch;
	if(spinning){
		Attitude(DesiredPitch,GetBank(),GetProperHeading());
	}else{
		Attitude(DesiredPitch,(0.5*(1-VinkaMode)*PI),GetProperHeading(),10,10,10);}
	return;
}

void Multistage2015::VinkaAutoPilot(){
	for(int q=1;q<=VinkaGetStep(MET);q++){
	VinkaConsumeStep(q);
		}
	return;
}


void Multistage2015::VinkaDeleteStep(int q)
{
	if(Gnc_step[q].GNC_Comand==CM_ORBIT){wPeg=FALSE;}
	if(Gnc_step[q].GNC_Comand==CM_DISABLE_ROLL){rolldisabled=FALSE;}
	if(Gnc_step[q].GNC_Comand==CM_DISABLE_PITCH){pitchdisabled=FALSE;}
	if(Gnc_step[q].GNC_Comand==CM_DISABLE_JETTISON){AJdisabled=FALSE;}
	Gnc_step[q].GNC_Comand=CM_NOLINE;
	sprintf(Gnc_step[q].Comand,"\0");
	VinkaRearrangeSteps();
	nsteps=VinkaCountSteps();
	VinkaCheckInitialMet();
	return;
}

void Multistage2015::VinkaRearrangeSteps()
{
	int index=1;
	GNC_STEP trans[150];
	GNC_STEP temp;
	//initialize trans;
	for(int q=0;q<150;q++)
	{
		trans[q].GNC_Comand=CM_NOLINE;
		sprintf(trans[q].Comand,"\0");
		trans[0].time_fin=-10000; // this should be unnecessary but who knows...
			
	}
	for(int i=1;i<150;i++)
	{
		if(Gnc_step[i].GNC_Comand!=CM_NOLINE)
		{
			trans[index]=Gnc_step[i];
			index++;
		}

	}

	for(int q=1;q<150;q++)
	{
		Gnc_step[q]=trans[q];
	}

	for(int k=1;k<index-1;k++)
	{
		for(int j=k+1;j<index;j++)
		{
			if(Gnc_step[k].time>Gnc_step[j].time)
			{
				temp=Gnc_step[k];
				Gnc_step[k]=Gnc_step[j];
				Gnc_step[j]=temp;
			}
		}
	}

return;
}

int Multistage2015::VinkaCountSteps()
{
	int q=0;
	for(int i=1;i<150;i++)
	{
		if(Gnc_step[i].GNC_Comand!=CM_NOLINE)
		//if(Gnc_step[i].GNC_Comand==CM_ROLL||CM_PITCH||CM_ENGINE||CM_FAIRING||CM_LES||CM_DISABLE_PITCH||CM_DISABLE_ROLL||CM_DISABLE_JETTISON||CM_PLAY||CM_JETTISON||CM_TARGET||CM_AOA||CM_ATTITUDE||CM_SPIN||CM_INVERSE||CM_ORBIT)//,CM_NOLINE
		{
			q+=1;
		}
	}

	return q;
}

GNC_STEP Multistage2015::VinkaComposeSpecificGNCSteps(GNC_STEP gnc)
{
	gnc.executed=FALSE;
		 for(int z=0;z<128;z++){
		  gnc.Comand[z]=tolower(gnc.Comand[z]);
														}
		
		if(strcmp(gnc.Comand,"engine")==0){
			gnc.GNC_Comand=CM_ENGINE;
		}else if(strcmp(gnc.Comand,"roll")==0){
			gnc.GNC_Comand=CM_ROLL;
		}else if(strcmp(gnc.Comand,"pitch")==0){
			gnc.GNC_Comand=CM_PITCH;
		}else if(strcmp(gnc.Comand,"fairing")==0){
			gnc.GNC_Comand=CM_FAIRING;
		}else if(strcmp(gnc.Comand,"les")==0){
			gnc.GNC_Comand=CM_LES;
		}else if(strcmp(gnc.Comand,"disablepitch")==0){
			gnc.GNC_Comand=CM_DISABLE_PITCH;
		}else if(strcmp(gnc.Comand,"disableroll")==0){
			gnc.GNC_Comand=CM_DISABLE_ROLL;
		}else if(strcmp(gnc.Comand,"disablejettison")==0){
			gnc.GNC_Comand=CM_DISABLE_JETTISON;
		}else if(strcmp(gnc.Comand,"playsound")==0){
			gnc.GNC_Comand=CM_PLAY;
		}else if(strcmp(gnc.Comand,"jettison")==0){
			gnc.GNC_Comand=CM_JETTISON;
		}else if(strcmp(gnc.Comand,"target")==0){
			gnc.GNC_Comand=CM_TARGET;
		}else if(strcmp(gnc.Comand,"aoa")==0){
			gnc.GNC_Comand=CM_AOA;
		}else if(strcmp(gnc.Comand,"attitude")==0){
			gnc.GNC_Comand=CM_ATTITUDE;
		}else if(strcmp(gnc.Comand,"spin")==0){
			gnc.GNC_Comand=CM_SPIN;
		}else if(strcmp(gnc.Comand,"inverse")==0){
			gnc.GNC_Comand=CM_INVERSE;
		}else if(strcmp(gnc.Comand,"engineout")==0){
			gnc.GNC_Comand=CM_ENGINEOUT;
		}else if(strcmp(gnc.Comand,"orbit")==0){
			gnc.GNC_Comand=CM_ORBIT;
		}else if(strcmp(gnc.Comand,"defap")==0){
			gnc.GNC_Comand=CM_DEFAP;
		}else if(strcmp(gnc.Comand,"glimit")==0){
			gnc.GNC_Comand=CM_GLIMIT;
		}else if(strcmp(gnc.Comand,"destroy")==0){
			gnc.GNC_Comand=CM_DESTROY;
		}else if(strcmp(gnc.Comand,"explode")==0){
			gnc.GNC_Comand=CM_EXPLODE;
		}else if(strcmp(gnc.Comand,"noline")==0){
			gnc.GNC_Comand=CM_NOLINE;
		}

//CM_ROLL,CM_PITCH,CM_ENGINE,CM_FAIRING,CM_DISABLE,CM_PLAY,CM_JETTISON,CM_TARGET,CM_AOA,CM_ATTITUDE,CM_SPIN

		Gnc_step[0].time_fin=-10000;

		switch(gnc.GNC_Comand){
		
		case CM_ENGINE:
			gnc.val_init=gnc.trval1;
			gnc.val_fin=gnc.trval2;
			if(gnc.val_fin==0){gnc.val_fin=-1;}
			gnc.time_init=gnc.time;
			gnc.duration=gnc.trval3;
			if(gnc.duration==0){gnc.duration=0.01;}
			gnc.time_fin=gnc.time_init+gnc.duration;
			break;

		case CM_ROLL:
			gnc.time_init=gnc.time;
			gnc.val_init=gnc.trval2;
			gnc.val_fin=gnc.trval4;
			gnc.duration=60;//gnc.trval1;
			gnc.time_fin=gnc.time_init+gnc.duration;
			
			//VinkaAzimuth=gnc.trval3*RAD;
		//	VinkaMode=gnc.trval5;
		
			break;
		
		case CM_PITCH:
			gnc.time_init=gnc.time;
			gnc.val_init=gnc.trval1;
			gnc.val_fin=gnc.trval2;
			gnc.duration=gnc.trval3;
			gnc.time_fin=gnc.time_init+gnc.duration;
			break;

			
		case CM_FAIRING:
			gnc.time_init=gnc.time;
			gnc.val_init=gnc.trval1*1000;
			gnc.time_fin=gnc.time_init+1000;
			break;
		case CM_LES:
			gnc.time_init=gnc.time;
			gnc.val_init=gnc.trval1*1000;
			gnc.time_fin=gnc.time_init+1000;
			break;
		case CM_DISABLE_JETTISON:
			gnc.time_init=gnc.time;
			gnc.time_fin=gnc.time_init+2;
			break;
		case CM_DISABLE_PITCH:
			gnc.time_init=gnc.time;
			gnc.time_fin=gnc.time_init+2;
			break;
		case CM_DISABLE_ROLL:
			gnc.time_init=gnc.time;
			gnc.time_fin=gnc.time_init+2;
			break;
		case CM_JETTISON:
			gnc.time_init=gnc.time;
			gnc.time_fin=gnc.time_init+2;
			break;
		case CM_AOA:
			gnc.time_init=gnc.time;
			gnc.duration=gnc.trval2;
			
			if(gnc.duration<=0){
		
			//	if(i<nsteps){
			//		gnc.time_fin=Gnc_step[i+1].time;
			//	}else{
					gnc.time_fin=gnc.time_init+60;
				//}
			}else{
				gnc.time_fin=gnc.time_init+gnc.duration;
			}

			
			gnc.val_init=gnc.trval1*RAD;
			
			
			break;
		case CM_ATTITUDE:
			gnc.time_init=gnc.time;
			gnc.duration=gnc.trval4;
			if(gnc.duration<=0){
				
			//	if(i<nsteps){
				//	gnc.time_fin=Gnc_step[i+1].time;
				//}else{
					gnc.time_fin=gnc.time_init+60;
				//}
			}else{
				gnc.time_fin=gnc.time_init+gnc.duration;
			}
			break;
		case CM_SPIN:
			gnc.time_init=gnc.time;

			//if(i<nsteps){
					//gnc.time_fin=Gnc_step[i+1].time;
				//}else{
					gnc.time_fin=gnc.time_init+60;
				//}
			gnc.val_init=gnc.trval1;
			break;
		case CM_TARGET:
			gnc.time_init=gnc.time;
			gnc.time_fin=gnc.time_init+5*60;
			gnc.val_init=gnc.trval1;
			break;
		case CM_INVERSE:
			gnc.time_init=gnc.time;
			gnc.time_fin=gnc.time_init+2;
			break;
		case CM_ENGINEOUT:
			gnc.time_init=gnc.time;
			gnc.time_fin=gnc.time_init+1;
			gnc.val_init=gnc.trval1;
			break;
		case CM_ORBIT:
			gnc.time_init=gnc.time;
			gnc.time_fin=gnc.time_init+10000;
			tgtapo=gnc.trval2*1000;
			tgtperi=gnc.trval1*1000;
			tgtinc=gnc.trval3*RAD;
			VinkaMode=gnc.trval4;
			if(VinkaMode==0){VinkaMode=1;}
			GT_InitPitch=gnc.trval5*RAD;
			if(GT_InitPitch==0){GT_InitPitch=GT_IP_Calculated;}
			tgtabside=gnc.trval6*1000;
			wPeg=TRUE;
			CalculateTargets();
			sprintf(logbuff,"%s: Orbit Call Found! Targets: Apogee:%.1f Perigee:%.1f Inclination:%.1f Mode:%.1f GT initial Pitch: %.1f Abside:%.1f",GetName(),tgtapo,tgtperi,tgtinc*DEG,VinkaMode,GT_InitPitch*DEG,tgtabside);
			oapiWriteLog(logbuff);
			break;
		case CM_PLAY:
			gnc.time_init=gnc.time;
			gnc.time_fin=gnc.time_init+2;
			for(int s=0;s<57;s++){
				if(Ssound.GncStepSound[s]==-5){
					Ssound.GncStepSound[s]=nsteps+1;
					break;
				}
			}
		//	sprintf(logbuff,"%s: Sound found @step %i",GetName(),i);
			//oapiWriteLog(logbuff);
			break;
		case CM_DEFAP:
			gnc.time_init=gnc.time;
			gnc.time_fin=gnc.time_init+2;
			break;
		case CM_GLIMIT:
			gnc.time_init=gnc.time;
			gnc.time_fin=gnc.time_init+250;
			break;
		case CM_DESTROY:
			gnc.time_init=gnc.time;
			gnc.time_fin=gnc.time_init+2;
			break;
		case CM_EXPLODE:
			gnc.time_init=gnc.time;
			gnc.time_fin=gnc.time_init+2;
		case CM_NOLINE:
			
			break;

		}
	
	if(!wPeg){
	VinkaUpdateRollTime();
	}

	return gnc;
}

void Multistage2015::VinkaAddStep(char input[MAXLEN])
{
	string line(input);
	
		for(int i=0;i<6;i++){Gnc_step[nsteps+1].wValue[i]=FALSE;}
		
		std::size_t findEqual=line.find_first_of("=");
		if(findEqual!=line.npos){
		string mettime=line.substr(0,findEqual);
		
		Gnc_step[nsteps+1].time=atof(&mettime[0]);

		std::size_t findLineEnd=line.find_first_of(")");
		if(findLineEnd!=line.npos){
	
		std::size_t findOpenP=line.find_first_of("(");
		if(findOpenP!=line.npos){
			string comand=line.substr(findEqual+1,findOpenP-findEqual-1);
			sprintf(Gnc_step[nsteps+1].Comand,&comand[0]);
			
			string values=line.substr(findOpenP+1,findLineEnd-findOpenP-1);
				Gnc_step[nsteps+1].wValue[0]=TRUE;			
			std::size_t findFirstComma=values.find_first_of(",");
			if(findFirstComma!=values.npos){
				value1=values.substr(0,findFirstComma);
			Gnc_step[nsteps+1].wValue[1]=TRUE;
			

			std::size_t findSecondComma=values.find_first_of(",",findFirstComma+1);
						
			if(findSecondComma!=values.npos){
				
				value2=values.substr(findFirstComma+1,findSecondComma-findFirstComma-1);
				Gnc_step[nsteps+1].wValue[2]=TRUE;

			std::size_t findThirdComma=values.find_first_of(",",findSecondComma+1);
				
			if(findThirdComma!=values.npos){
				
				value3=values.substr(findSecondComma+1,findThirdComma-findSecondComma-1);
				Gnc_step[nsteps+1].wValue[3]=TRUE;

			std::size_t findFourthComma=values.find_first_of(",",findThirdComma+1);
			if(findFourthComma!=values.npos){	
				value4=values.substr(findThirdComma+1,findFourthComma-findThirdComma-1);
				Gnc_step[nsteps+1].wValue[4]=TRUE;
			std::size_t findFifthComma=values.find_first_of(",",findFourthComma+1);
			if(findFifthComma!=values.npos){
				value5=values.substr(findFourthComma+1,findFifthComma-findFourthComma-1);
				value6=values.substr(findFifthComma+1,string::npos);
				Gnc_step[nsteps+1].wValue[5]=TRUE;
			}else{ value5=values.substr(findFourthComma+1,string::npos);}
			}else{ value4=values.substr(findThirdComma+1,values.npos);}
			}else{ value3=values.substr(findSecondComma+1,values.npos);}
			}else{ value2=values.substr(findFirstComma+1,values.npos);}
			}else{ value1=values.substr(0,values.npos);}
		}}
		if(Gnc_step[nsteps+1].wValue[0]){	Gnc_step[nsteps+1].trval1=atof(&value1[0]);}else{Gnc_step[nsteps+1].trval1=0;}
		if(Gnc_step[nsteps+1].wValue[1]){	Gnc_step[nsteps+1].trval2=atof(&value2[0]);}else{Gnc_step[nsteps+1].trval2=0;}
		if(Gnc_step[nsteps+1].wValue[2]){	Gnc_step[nsteps+1].trval3=atof(&value3[0]);}else{Gnc_step[nsteps+1].trval3=0;}
		if(Gnc_step[nsteps+1].wValue[3]){	Gnc_step[nsteps+1].trval4=atof(&value4[0]);}else{Gnc_step[nsteps+1].trval4=0;}
		if(Gnc_step[nsteps+1].wValue[4]){	Gnc_step[nsteps+1].trval5=atof(&value5[0]);}else{Gnc_step[nsteps+1].trval5=0;}
		if(Gnc_step[nsteps+1].wValue[5]){	Gnc_step[nsteps+1].trval6=atof(&value6[0]);}else{Gnc_step[nsteps+1].trval6=0;}

		}else{
		sprintf(Gnc_step[nsteps+1].Comand,"noline");	
		Gnc_step[nsteps+1].GNC_Comand=CM_NOLINE;
		Gnc_step[nsteps+1].time_fin=-10000;
		Gnc_step[nsteps+1].trval1=0;
		Gnc_step[nsteps+1].trval2=0;
		Gnc_step[nsteps+1].trval3=0;
		Gnc_step[nsteps+1].trval4=0;
		Gnc_step[nsteps+1].trval5=0;
		Gnc_step[nsteps+1].trval6=0;
		}

	
		
		for(UINT l=0;l<line.size();l++){
			line[l]=tolower(line[l]);
		}
		
		std::size_t foundDisable=line.find("disable");
		std::size_t foundPlay=line.find("playsound");

		if(foundDisable!=std::string::npos){
		std::size_t foundDisPitch=line.find("pitch");
		std::size_t foundDisRoll=line.find("roll");
		std::size_t foundDisJett=line.find("jettison");

			if(foundDisPitch!=std::string::npos){
				sprintf(Gnc_step[nsteps+1].Comand,"disablepitch");
			}else if(foundDisRoll!=std::string::npos){
				sprintf(Gnc_step[nsteps+1].Comand,"disableroll");
			}else if(foundDisJett!=std::string::npos){
				sprintf(Gnc_step[nsteps+1].Comand,"disablejettison");
			}

		}else if(foundPlay!=std::string::npos){
		std::size_t findopen=line.find_first_of("(");
		std::size_t findclose=line.find_first_of(")");
		string filename=line.substr(findopen+1,findclose-findopen-1);
			sprintf(Gnc_step[nsteps+1].Comand,"playsound");
			filename.copy(Gnc_step[nsteps+1].trchar,MAXLEN,0);
		}
		
		Gnc_step[nsteps+1]=VinkaComposeSpecificGNCSteps(Gnc_step[nsteps+1]);
	
	VinkaRearrangeSteps();
	nsteps=VinkaCountSteps();	
	VinkaCheckInitialMet();
}

void Multistage2015::WriteGNCFile()
{
FILEHANDLE GncFile;
char filenmbuff[MAXLEN];
sprintf(filenmbuff,"Multistage2015\\Guidance\\%s_%.2f_GNC.txt",GetName(),oapiGetSysMJD());
GncFile=oapiOpenFile(filenmbuff,FILE_OUT,CONFIG);
char buffer[MAXLEN];
sprintf(buffer,"Multistage 2015 Automatically Generated Guidance File");
oapiWriteLine(GncFile,buffer);
sprintf(buffer,"Vehicle: %s",GetName());
oapiWriteLine(GncFile,buffer);

for(int i=1;i<=nsteps;i++)
{
	sprintf(buffer,"%.3f =%s(%.2f,%.2f,%.2f,%.2f,%.2f,%.2f)",Gnc_step[i].time_init,Gnc_step[i].Comand,Gnc_step[i].trval1,Gnc_step[i].trval2,Gnc_step[i].trval3,Gnc_step[i].trval4,Gnc_step[i].trval5,Gnc_step[i].trval6);
	oapiWriteLine(GncFile,buffer);
}


oapiCloseFile(GncFile,FILE_OUT);
	return;
}
