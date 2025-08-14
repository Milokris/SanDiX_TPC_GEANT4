#include "detector.hh"

MySensitiveDetector::MySensitiveDetector(G4String name) : G4VSensitiveDetector(name)
{}

MySensitiveDetector::~MySensitiveDetector()
{}

G4bool MySensitiveDetector::ProcessHits(G4Step *aStep, G4TouchableHistory 
    *ROhist)
{
    G4Track *track = aStep->GetTrack();

    if (track->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition())
    {
        return false;
    }
    
    track->SetTrackStatus(fStopAndKill);

    G4StepPoint *preStepPoint = aStep->GetPreStepPoint();
    G4StepPoint *postStepPoint = aStep->GetPostStepPoint();

    G4ThreeVector posPhoton = preStepPoint->GetPosition();

    //G4cout << "Photon position: " << posPhoton << G4endl;

    const G4VTouchable *touchable = aStep->GetPreStepPoint()->GetTouchable();

    G4int copyNo = touchable->GetCopyNumber();

    //G4cout << "Copy Number: " << copyNo << G4endl;

    G4VPhysicalVolume *physVol = touchable->GetVolume();
    G4ThreeVector posDetector = physVol->GetTranslation();

    G4double time = preStepPoint->GetGlobalTime(); // in ns
    //G4cout << time << G4endl;


    //G4cout << "Detector position: " << posDetector << G4endl;

    G4int evt = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    G4AnalysisManager *man = G4AnalysisManager::Instance();
    man->FillNtupleIColumn(0, evt);
    man->FillNtupleDColumn(1, posDetector[0]); //posPhoton[0]);
    man->FillNtupleDColumn(2, posDetector[1]); //posPhoton[1]);
    man->FillNtupleDColumn(3, posDetector[2]); //posPhoton[2]);
    //man->FillNtupleDColumn(4, 0.0); // photons don't deposit energy
    man->FillNtupleDColumn(4, time);
    man->AddNtupleRow(0);

    return true;

}