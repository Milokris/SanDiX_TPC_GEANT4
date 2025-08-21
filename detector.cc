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

    const G4VTouchable *touchable = aStep->GetPreStepPoint()->GetTouchable();

    G4int copyNo = touchable->GetCopyNumber();

    G4VPhysicalVolume *physVol = touchable->GetVolume();
    G4ThreeVector posDetector = physVol->GetTranslation();

    G4double time = preStepPoint->GetGlobalTime(); // in ns

    G4int evt = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    G4AnalysisManager *man = G4AnalysisManager::Instance();

	man->FillH1(0, posDetector[0]/mm);
    man->FillH1(1, posDetector[1]/mm);
	man->FillH1(2, posDetector[2]/mm);
	man->FillH1(3, time/us);

    return true;

}