#include "stepping.hh"

MySteppingAction::MySteppingAction(MyEventAction *eventAction)
{
	fEventAction = eventAction;
}

MySteppingAction::~MySteppingAction()
{}

void MySteppingAction::UserSteppingAction(const G4Step *step)
{		
	G4LogicalVolume *volume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume();
	
	const MyDetectorConstruction *detectorConstruction = static_cast<const MyDetectorConstruction*> (G4RunManager::GetRunManager()->GetUserDetectorConstruction());
	
	G4LogicalVolume *fScoringVolume = detectorConstruction->GetScoringVolume();
	
	G4Track *track = step->GetTrack();
	G4int id = track->GetTrackID();
	G4double energy = track->GetKineticEnergy();
	
	const G4double tol = 1e-6 * MeV;
	const int maxSteps = 100;

// Only apply stagnation checks to non-electron particles
	const G4ParticleDefinition* pd = track->GetParticleDefinition();
	if (pd && pd != G4Electron::ElectronDefinition())
	{
    	if(previousEnergy.count(id))
    	{
    		G4double prevE = previousEnergy[id];
        
        	if(std::abs(energy - prevE) < tol)
        	{
            	stagnationCounter[id]++;
        	}
        	else
        	{
            	stagnationCounter[id] = 0;
        	}
        
        	if(stagnationCounter[id] >= maxSteps)
        	{
            	track->SetTrackStatus(fStopAndKill);
        	}
    	}
    	else
    	{
        	stagnationCounter[id] = 0;
    	}
	}
	
	
	previousEnergy[id] = energy;
	
	if(volume != fScoringVolume) {
		//G4cout << volume->GetName() << G4endl;
		//G4cout << fScoringVolume->GetName() << G4endl;
		return;
	}
	
	G4double edep = step->GetTotalEnergyDeposit();
	fEventAction->AddEdep(edep);
}

void MySteppingAction::ClearStagnationData(G4int trackID)
{
	previousEnergy.erase(trackID);
	stagnationCounter.erase(trackID);
}
