// ==============================================================
//                 ORBITER MODULE: stage
//                 
// stage.cpp
// Control module for stage vessel class
//
// ==============================================================

#define STRICT
#define ORBITER_MODULE

#include "..//..//include//Orbitersdk.h"

// ==============================================================
// Some vessel parameters
// ==============================================================




// ==============================================================
// stage class interface
// ==============================================================

class stage : public VESSEL4 {
public:
	stage(OBJHANDLE hVessel, int flightmodel)
		: VESSEL4(hVessel, flightmodel) {
	}
	void clbkSetClassCaps(FILEHANDLE cfg)override;
	void clbkPreStep(double SimT, double SimDT, double MJD)override;
	void clbkPostCreation()override;


};

void HLiftCoeff(VESSEL* v, double beta, double M, double Re, void* context, double* cl, double* cm, double* cd)
{
	// no lift, no moment, only drag
	*cl = 0;
	*cm = 0;
	*cd = 1;
}

void VLiftCoeff(VESSEL* v, double aoa, double M, double Re, void* context, double* cl, double* cm, double* cd)
{
	// no lift, no moment, only drag
	*cl = 0;
	*cm = 0;
	*cd = 1;
}

// ==============================================================
// Overloaded callback functions
// ==============================================================

// --------------------------------------------------------------
// Set the capabilities of the vessel class
// --------------------------------------------------------------

void stage::clbkSetClassCaps(FILEHANDLE cfg)
{

	// physical specs
	SetCW(1, 1, 0.5, 0.5);
	SetRotDrag(_V(0.5, 0.5, 0.5));
	SetSize(20);
	SetEmptyMass(1000);
	SetPMI(_V(20, 20, 10));
	SetCrossSections(_V(20, 20, 5));
	SetCameraOffset(_V(0, 2, -5));
	SetTouchdownPoints(_V(0, 1, -0.5), _V(1, 0, -0.5), _V(-1, 0, -0.5));


	CreateAirfoil3(LIFT_HORIZONTAL, _V(0, 0, 0), HLiftCoeff, NULL, 0.00001, 0.00001, 0.00001);
	CreateAirfoil3(LIFT_VERTICAL, _V(0, 0, 0), VLiftCoeff, NULL, 0.00001, 0.00001, 0.00001);
	// using a larger aerofoil creates too much lift in the +x direction!
	// I don't understand why, though ;-)

}

// ==============================================================
// API callback interface
// ==============================================================

// --------------------------------------------------------------
// Vessel initialisation
// --------------------------------------------------------------
DLLCLBK VESSEL* ovcInit(OBJHANDLE hvessel, int flightmodel)
{
	return new stage(hvessel, flightmodel);
}
// --------------------------------------------------------------
// Vessel cleanup
// --------------------------------------------------------------
DLLCLBK void ovcExit(VESSEL* vessel)
{
	if (vessel) delete (stage*)vessel;
}
// ==============================================================
// Overloaded clbkPostCreation callback functions
// ==============================================================

void stage::clbkPostCreation()
{
	// multistage2.dll gives the vessel the same name as the mesh
	// so we can use the vessel name to load the mesh
	char name[256];
	strcpy(name, GetName());
	SetMeshVisibilityMode((AddMesh(name)), MESHVIS_EXTERNAL);

}

void stage::clbkPreStep(double SimT, double SimDT, double MJD)
{

	// self delete when at the surface
	if (GetAltitude() < 10) oapiDeleteVessel(GetHandle());


	// extra drag
	VECTOR3 drag;
	GetDragVector(drag);

	double dfactor = 2000;
	VECTOR3 dragx;
	dragx = drag * (dfactor);
	AddForce(dragx, _V(0, 0, 0));


}
