#ifndef MYTRACKINGACTION_HH
#define MYTRACKINGACTION_HH

#include "G4UserTrackingAction.hh"
#include "G4Track.hh"
#include "stepping.hh"

class MySteppingAction;

class MyTrackingAction : public G4UserTrackingAction
{
public:
	MyTrackingAction(MySteppingAction *steppingAction);
	~MyTrackingAction() = default;

	virtual void PreUserTrackingAction(const G4Track *track) override;   // ADDED
	virtual void PostUserTrackingAction(const G4Track *track) override;

private:
	MySteppingAction *fSteppingAction;
};

#endif