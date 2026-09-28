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


    const G4int bins = 5000; //2500 //1000

    const double tmin = -5.; //microseconds
	const double tmax = 20; //microsends
    const double posMin = -35.; //mm
    const double posMax = 35.; //mm

    man->CreateH1("posX", " ", bins, posMin, posMax);
    man->CreateH1("posY", " ", bins, posMin, posMax);
    man->CreateH1("posZ", " ", bins, -30, 30);
    man->CreateH1("Time", " ", bins, tmin, tmax);
    man->CreateH1("drift times", "Drift Times", 50, -1, 10);


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
    G4cout << "==BEGIN_USER_OUTPUT==" << G4endl;

    startTime = std::chrono::high_resolution_clock::now();
    G4AnalysisManager* man = G4AnalysisManager::Instance();
    G4int runID = run->GetRunID() + 3; //add previous run # here to preserve root files. 0 for gammas, 1 for neutrons, 2 for drift time
    std::stringstream strRunID;
    strRunID << runID;
    man->OpenFile("output" + strRunID.str() + ".root");
}

void MyRunAction::EndOfRunAction(const G4Run* run)
{
    G4int runID = run->GetRunID();


    G4cout << "Run Number: " << runID << G4endl;

    G4AnalysisManager *man = G4AnalysisManager::Instance();

    man->Write();
    man->CloseFile();

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;

    double totalSeconds = elapsed.count();
    int hours   = (int)totalSeconds / 3600;
    int minutes = ((int)totalSeconds % 3600) / 60;
    int seconds = (int)totalSeconds % 60;
    if (hours > 0)
        G4cout << "Run " << runID << " elapsed time: " << hours << "h " << minutes << "m " << seconds << "s" << G4endl;
    else if (minutes > 0)
        G4cout << "Run " << runID << " elapsed time: " << minutes << "m " << seconds << "s" << G4endl;
    else
        G4cout << "Run " << runID << " elapsed time: " << seconds << "s" << G4endl;
    //G4cout << "Run " << runID << " elapsed time: " << elapsed.count() << " seconds" << G4endl;

    G4cout << "==END_USER_OUTPUT==" << G4endl;
}