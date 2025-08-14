#include "tracking.hh"

MyTrackingAction::MyTrackingAction(MySteppingAction *steppingAction) : fSteppingAction(steppingAction)
{}

void MyTrackingAction::PostUserTrackingAction(const G4Track *track)
{
	G4int id = track->GetTrackID();
	fSteppingAction->ClearStagnationData(id);
}

