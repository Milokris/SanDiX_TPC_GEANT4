#include "run.hh"

MyRunAction::MyRunAction()
{
    G4AnalysisManager *man = G4AnalysisManager::Instance();

    man->CreateNtuple("Hits", "Hits");
    man->CreateNtupleDColumn("fX");
    man->CreateNtupleDColumn("fY");
    man->CreateNtupleDColumn("fZ");
    man->CreateNtupleDColumn("Time");
    man->FinishNtuple(0);
    

    const G4int bins = 1000; //2500

    const double tmin = -5.; //microseconds 
	const double tmax = 20; //microsends 
    const double posMin = -35.; //mm
    const double posMax = 35.; //mm

    man->CreateH1("posX", " ", bins, posMin, posMax); 
    man->CreateH1("posY", " ", bins, posMin, posMax); 
    man->CreateH1("posZ", " ", bins, -30, 30); 
    man->CreateH1("Time", " ", bins, tmin, tmax); 


    man->CreateNtuple("Events", "Golden paramter");
    man->CreateNtupleDColumn("logS2");
    man->CreateNtupleDColumn("S1");
    man->CreateNtupleIColumn("recoilType"); // 0=ER, 1=NR
    man->FinishNtuple(); //(1)


}

MyRunAction::~MyRunAction()
{}

void MyRunAction::BeginOfRunAction(const G4Run* run)
{
    nS1Events = 0;
    nS2Events = 0;
    totalS1Photons = 0;
    totalS2Photons = 0;

    DriftTrackIDs.clear();

    G4AnalysisManager *man = G4AnalysisManager::Instance();

    G4int runID = run->GetRunID();

    std::stringstream strRunID;
    strRunID << runID;

    man->OpenFile("output"+strRunID.str()+".root");

}

void MyRunAction::EndOfRunAction(const G4Run*)
{
    DriftTrackIDs.clear();

	G4cout << "Number of S1 events: " << nS1Events << G4endl;
	G4cout << "Number of S2 events: " << nS2Events << G4endl;

    G4cout << "Total S1 Photons: " << totalS1Photons << G4endl;
	G4cout << "Total S2 Photons: " << totalS2Photons << G4endl;


    G4AnalysisManager *man = G4AnalysisManager::Instance();

    man->Write();
    man->CloseFile();

}