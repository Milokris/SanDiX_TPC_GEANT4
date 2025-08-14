// #ifndef FAST_S2_MODEL_HH
// #define FAST_S2_MODEL_HH

// #include "G4VFastSimulationModel.hh"
// #include "G4Electron.hh"
// #include "G4OpticalPhoton.hh"
// #include "DriftElectronInfo.hh"
// #include "G4ThreeVector.hh"
// #include "G4Material.hh"
// #include "G4ParticleTable.hh"
// #include "globals.hh"
// #include "Randomize.hh"
// #include "electricField2.hh"



// class FastS2Model : public G4VFastSimulationModel {
// public:
//     FastS2Model(const G4String& modelName, G4Region* envelope, ElectricField2* field);
//     virtual ~FastS2Model();

//     // Mandatory overrides
//     virtual G4bool IsApplicable(const G4ParticleDefinition& particle) override;
//     virtual G4bool ModelTrigger(const G4FastTrack& fastTrack) override;
//     virtual void DoIt(const G4FastTrack& fastTrack, G4FastStep& fastStep) override;

//     // Configurable parameters
//     void SetYieldParams(G4double p3, G4double Eth_kVcm) {
//         fP3 = p3;
//         fEth_kVcm = Eth_kVcm;
//     }
//     void SetStepSize(G4double step_um) { fStep_um = step_um; }

// private:
//     ElectricField2* fField;
//     G4double fP3;        // photons/(electron·(kV/cm)·µm)
//     G4double fEth_kVcm;  // threshold field in kV/cm
//     G4double fStep_um;   // step size in micrometers
// };

// #endif
