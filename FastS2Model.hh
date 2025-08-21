#ifndef FAST_S2_MODEL_HH
#define FAST_S2_MODEL_HH

#include "G4VFastSimulationModel.hh"
#include "G4ThreeVector.hh"
#include "G4FastTrack.hh"
#include "G4FastStep.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include "nestFile.hh"
#include "DriftElectronInfo.hh"
#include "G4Electron.hh"
#include "G4OpticalPhoton.hh"
#include "G4DynamicParticle.hh"
#include "G4Track.hh"
#include "G4ParticleDefinition.hh"
#include "NEST.hh"
#include "stepping.hh"
#include "run.hh"
#include "DynamicUserLimits.hh"


class LXeElectronDriftModel : public G4VFastSimulationModel {
public:
    LXeElectronDriftModel(const G4String& modelName, G4Region* envelope, nestPart* nptr)
        : G4VFastSimulationModel(modelName, envelope), nestCalc(nptr) {}

    ~LXeElectronDriftModel() override {}

    G4bool IsApplicable(const G4ParticleDefinition& particle) override {
        return &particle == G4Electron::ElectronDefinition();
    }

    G4bool ModelTrigger(const G4FastTrack& fastTrack) override {
        const DriftElectronInfo* info =
            dynamic_cast<const DriftElectronInfo*>(fastTrack.GetPrimaryTrack()->GetUserInformation());
        return info && info->IsDrift(); 
    }


    void DoIt(const G4FastTrack& fastTrack, G4FastStep& fastStep) override;

private:
    nestPart* nestCalc;

    bool CheckGainRegion(double x, double y, double z, double Efield) const {
        return (Efield > ElThreshold);
    }

    double ElThreshold = 412000.; 
    nestPart* nestDetector;  // pointer to the nestPart detector

};

#endif
