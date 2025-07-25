#ifndef STEPPING_HH
#define STEPPING_HH

#include "G4UserSteppingAction.hh"
#include"G4Step.hh"
#include "G4Track.hh"
#include "globals.hh"
#include "G4RunManager.hh"
#include <map>
#include <cmath>

#include "construction.hh"
#include "tracking.hh"
#include "event.hh"
#include "G4Electron.hh"  // For G4Electron definition
#include "G4OpticalPhoton.hh"

class MySteppingAction : public G4UserSteppingAction
{
public:
	MySteppingAction(MyEventAction* eventAction);
	~MySteppingAction();
	
	virtual void UserSteppingAction(const G4Step*);
	
	void ClearStagnationData(G4int trackID);
	
private:
	MyEventAction *fEventAction;
	
	std::map<G4int, G4double> previousEnergy ;
	std::map<G4int, G4int> stagnationCounter;
};

#endif