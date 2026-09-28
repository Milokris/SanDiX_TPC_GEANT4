#include "tracking.hh"

MyTrackingAction::MyTrackingAction(MySteppingAction *steppingAction) : fSteppingAction(steppingAction)
{}

void MyTrackingAction::PreUserTrackingAction(const G4Track *track)   // ADDED
{
    G4int id = track->GetTrackID();
	G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

	fSteppingAction->ClearStagnationData(eventID, id);}

void MyTrackingAction::PostUserTrackingAction(const G4Track *track)
{
	G4int id = track->GetTrackID();
	G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

	fSteppingAction->ClearStagnationData(eventID, id);
}
