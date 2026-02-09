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
//                      MULTISTAGE 2026
//
//        Particle & Exhaust Management (Mach Diamonds)
// ==============================================================

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE

#include <cmath>
#include <cstdio>
#include "Orbitersdk.h"
#include "Multistage2026.h"

// Update frequency constants
#define PARTICLEUPDATECYCLE 1.0     // Seconds between size updates
#define PARTICLEOVERLAPCYCLE 0.1    // Seconds to blend old/new streams

void Multistage2026::ManageParticles(double dt, bool prestep)
{
    // Only run logic if there is significant atmosphere (otherwise max expansion is assumed/static)
    // Note: You might want to remove this 'if' if you want expansion to stop cleanly in vacuum
    // but the original code stops updating below 0.1 Pa.
    if (GetAtmPressure() > 10e-5) 
    {
        // 1. Update Timer
        if (prestep) { 
            particlesdt += dt; 
        }

        // 2. Creation Cycle (Every 1.0 second)
        if (particlesdt > PARTICLEUPDATECYCLE)
        {
            for (int i = 0; i < nPsh; i++)
            {
                if (psg[i].growing)
                {
                    if (prestep) 
                    {
                        // Toggle buffer index (0 or 1)
                        psg[i].status = (psg[i].status == 0) ? 1 : 0;

                        // Calculate new size based on pressure
                        // Growth = Base + Rate * log10(Ref / Current)
                        double pressureRatio = RefPressure / GetAtmPressure();
                        if (pressureRatio < 1.0) pressureRatio = 1.0; // Prevent negative logs

                        psg[i].pss.growthrate = psg[i].baserate + psg[i].GrowFactor_rate * log10(pressureRatio);
                        double deltasize = psg[i].GrowFactor_size * log10(pressureRatio);
                        psg[i].pss.srcsize = psg[i].basesize + deltasize;

                        VECTOR3 thdir;

                        // Get Thruster Direction to offset the origin slightly
                        if (!psg[i].ToBooster) {
                            // Main Stage Thruster
                            if (currentStage <= psg[i].nItem && psg[i].nItem < stage.size()) {
                                GetThrusterDir(stage[psg[i].nItem].th_main_h[psg[i].nEngine], thdir);
                            } else {
                                continue; // Invalid stage index
                            }
                        } else {
                            // Booster Thruster
                            if (currentBooster <= psg[i].nItem && psg[i].nItem < booster.size()) {
                                GetThrusterDir(booster[psg[i].nItem].th_booster_h[psg[i].nEngine], thdir);
                            } else {
                                continue; // Invalid booster index
                            }
                        }

                        // Invert direction for exhaust placement
                        thdir *= -1.0;

                        // Calculate new position offset
                        psg[i].pos = psg[i].basepos + (thdir * deltasize * 0.5);

                        // Create the new stream in the current buffer slot
                        if (!psg[i].ToBooster) {
                             if (currentStage <= psg[i].nItem) {
                                psg[i].psh[psg[i].status] = AddExhaustStream(stage[psg[i].nItem].th_main_h[psg[i].nEngine], psg[i].pos, &psg[i].pss);
                             }
                        } else {
                             if (currentBooster <= psg[i].nItem) {
                                psg[i].psh[psg[i].status] = AddExhaustStream(booster[psg[i].nItem].th_booster_h[psg[i].nEngine], psg[i].pos, &psg[i].pss);
                             }
                        }
                        
                        psg[i].counting = TRUE;
                    }
                }
            }
            // Reset timer
            if (!prestep) { particlesdt = 0; }
        }

        // 3. Deletion Cycle (0.1s after creation)
        // This creates a smooth cross-fade between the old size and new size
        if (!prestep)
        {
            for (int i = 0; i < nPsh; i++)
            {
                if (psg[i].growing && psg[i].counting)
                {
                    psg[i].doublepstime += dt;
                    
                    if (psg[i].doublepstime >= PARTICLEOVERLAPCYCLE)
                    {
                        if (!psg[i].FirstLoop)
                        {
                            // Delete the OLD stream (the one we didn't just create)
                            int oldStatus = (psg[i].status == 0) ? 1 : 0;
                            if (psg[i].psh[oldStatus]) {
                                DelExhaustStream(psg[i].psh[oldStatus]);
                                psg[i].psh[oldStatus] = NULL;
                            }
                        }
                        else {
                            // Special case for first run (might have a default stream at index 2)
                            if (psg[i].psh[2]) {
                                DelExhaustStream(psg[i].psh[2]);
                                psg[i].psh[2] = NULL;
                            }
                            psg[i].FirstLoop = FALSE;
                        }
                        
                        psg[i].counting = FALSE;
                        psg[i].doublepstime = 0;
                    }
                }
            }
        }
    }
}
