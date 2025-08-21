#include "event.hh"

MyEventAction::MyEventAction()
{}

MyEventAction::~MyEventAction()
{}

void MyEventAction::BeginOfEventAction(const G4Event*)
{}

void MyEventAction::EndOfEventAction(const G4Event*)
{
	
	G4AnalysisManager *man = G4AnalysisManager::Instance();
	    
    if(totalS2Photons > 0) // avoid log10(0)
    {  
        man->FillNtupleDColumn(1, 0, std::log10(totalS2Photons));
    } 
    else 
    {
        man->FillNtupleDColumn(1, 0, -10);
    }
    man->FillNtupleDColumn(1, 1, totalS1Photons);
    man->FillNtupleIColumn(1, 2, incidentRecoilType);
	man->AddNtupleRow(1);


}