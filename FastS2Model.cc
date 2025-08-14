// #include "FastS2Model.hh"
// #include "electricField2.hh"      // for field queries
// #include "G4SystemOfUnits.hh"
// #include "G4RandomDirection.hh"
// #include "G4ParticleTable.hh"
// #include "G4FastTrack.hh"
// #include "G4FastStep.hh"
// #include "G4OpticalPhoton.hh"
// #include "Randomize.hh"

// FastS2Model::FastS2Model(const G4String& modelName, G4Region* envelope, ElectricField2* field)
//     : G4VFastSimulationModel(modelName, envelope),
//     fField(field),
//     fP3(2.09e-2),     // photons / (e·kV/cm·µm)
//     fEth_kVcm(412.0), // kV/cm
//     fStep_um(0.1)     // micrometers
//     {}

// FastS2Model::~FastS2Model() {}

// G4bool FastS2Model::IsApplicable(const G4ParticleDefinition& particle) {
//     return (&particle == G4Electron::ElectronDefinition());
// }

// G4bool FastS2Model::ModelTrigger(const G4FastTrack& fastTrack) {
//     const G4Track* track = fastTrack.GetPrimaryTrack();
//     auto info = dynamic_cast<const DriftElectronInfo*>(track->GetUserInformation());
//     return (info && info->IsDrift());
// }

// void FastS2Model::DoIt(const G4FastTrack& fastTrack, G4FastStep& fastStep) {
//     G4ThreeVector pos = fastTrack.GetPrimaryTrack()->GetPosition();
//     G4ThreeVector dir = fastTrack.GetPrimaryTrack()->GetMomentumDirection();

//     // Photon energy ~ 7 eV
//     G4double photonEnergy = 7.0 * eV;
//     auto photonDef = G4OpticalPhoton::OpticalPhotonDefinition();

//     // Electric field query
//     ElectricField2* field = fField;
//     G4ThreeVector eFieldVec;
//     field->GetFieldValue((G4double*)&pos, (G4double*)&eFieldVec);
//     G4double E_Vcm = eFieldVec.mag();
//     G4double E_kVcm = E_Vcm / 1.0e3;

//     // Simple straight-line drift for demonstration
//     G4ThreeVector stepVec = dir.unit() * (fStep_um * micrometer);

//     // Accumulate photons along the step until out of envelope
//     G4ThreeVector currentPos = pos;
//     for (int i = 0; i < 10000; ++i) { // hard stop for safety
//         field->GetFieldValue((G4double*)&currentPos, (G4double*)&eFieldVec);
//         E_Vcm = eFieldVec.mag();
//         E_kVcm = E_Vcm / 1.0e3;

//         if (E_kVcm > fEth_kVcm) {
//             G4double yieldPerUm = fP3 * (E_kVcm - fEth_kVcm);
//             G4int nPhotons = CLHEP::RandPoisson::shoot(yieldPerUm * fStep_um);
//             for (int n = 0; n < nPhotons; ++n) {
//                 G4ThreeVector photDir = G4RandomDirection();
//                 G4DynamicParticle* dynPhoton = new G4DynamicParticle(photonDef, photDir, photonEnergy);
//                 fastStep.CreateSecondaryTrack(*dynPhoton, currentPos, photDir, 0.0, true);
//             }
//         }

//         currentPos += stepVec;

//         // TODO: break if outside logicChamber bounds (user can implement)
//         if (!fastTrack.GetEnvelopeSolid()->Inside(currentPos)) break;
//     }

//     fastStep.KillPrimaryTrack();
//     fastStep.ProposeTotalEnergyDeposited(0.);
// }
