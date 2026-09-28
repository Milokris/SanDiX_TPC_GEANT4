#include "event.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "stepping.hh"
#include <cmath>
#include <cstdio>

MyEventAction::MyEventAction(MySteppingAction* steppingAction)
    : fSteppingAction(steppingAction)
{}

MyEventAction::~MyEventAction()
{}

void MyEventAction::AddDriftCluster(float x, float y, float z, int count)
{
    if (std::isfinite(x) && std::isfinite(y) && std::isfinite(z))
        fDriftClusters.push_back({x, y, z, count});
}

void MyEventAction::RecordStep(int trkID, int parentID, const std::string& pname, float x, float y, float z)
{
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return;
    auto& trk = fTracks[trkID];
    if (trk.pts.empty()) {
        trk.id = trkID;
        trk.parentID = parentID;
        trk.particle = pname;
    }
    if ((int)trk.pts.size() < 500)
        trk.pts.push_back({x, y, z});
}

void MyEventAction::BeginOfEventAction(const G4Event* event)
{
    // Reset ALL per-event state here
    totalS1Photons  = 0;
    totalS2Photons  = 0;
    nS1Events       = 0;
    nS2Events       = 0;
    createdElectrons = 0;
    driftTime       = 0.0;
    fEdep           = 0.0;
    incidentRecoilType = -1;
    DriftTrackIDs.clear();
    fTracks.clear();
    fDriftClusters.clear();
    fSteppingAction->ClearAllStagnationData();

    G4PrimaryParticle* primary = event->GetPrimaryVertex(0)->GetPrimary(0);
    G4int pdg = primary->GetPDGcode();
    if      (pdg == 22 || pdg == 11) incidentRecoilType = 0; // ER
    else if (pdg == 2112)            incidentRecoilType = 1; // NR
}

void MyEventAction::EndOfEventAction(const G4Event*)
{
    const double g1 = 0.13;
    const double g2 = 0.7;
    const double detectedS1 = g1 * totalS1Photons;
    const double detectedS2 = g2 * totalS2Photons / photPerE;

    if (nS1Events > 0)
    {
        G4cout << "Incident Recoil Type (0=ER, 1=NR): " << incidentRecoilType << G4endl;
        //G4cout << "S1 Photons this event: " << totalS1Photons << G4endl;
        //G4cout << "S2 Photons this event: " << totalS2Photons << G4endl;
        G4cout << "S1 Events: " << nS1Events << G4endl;
        G4cout << "S2 Events: " << nS2Events << G4endl;

        G4AnalysisManager* man = G4AnalysisManager::Instance();
        G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
        G4int totalEvents = G4RunManager::GetRunManager()->GetCurrentRun()->GetNumberOfEventToBeProcessed();

        // From the paper, S2 = g2 * #e; dividing by photPerE converts
        // the simulated S2-photon total back to its electron equivalent.
        //because this 0.7 gain for S2 is so low compared to typical detectors, our S2 vs S1 graph is much lower than others

        G4cout << "Detected S1 photons: " << detectedS1 << G4endl;
        G4cout << "Detected S2 photons: " << detectedS2 << G4endl;
        G4cout << "Drift Time is: " << driftTime << " us" << G4endl;
        G4cout << "Event Number: " << eventID << G4endl;
        G4cout << "Percent of Events Completed: " << (1000*eventID/totalEvents)/10.0 << "%" << G4endl;
        const MyRunAction* runAction = static_cast<const MyRunAction*>(
            G4RunManager::GetRunManager()->GetUserRunAction());
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - runAction->GetStartTime();
        double totalSeconds = elapsed.count();
        int hours   = (int)totalSeconds / 3600;
        int minutes = ((int)totalSeconds % 3600) / 60;
        int seconds = (int)totalSeconds % 60;
        if (hours > 0)
            G4cout << "Elapsed Time: " << hours << "h " << minutes << "m " << seconds << "s" << G4endl;
        else if (minutes > 0)
            G4cout << "Elapsed Time: " << minutes << "m " << seconds << "s" << G4endl;
        else
            G4cout << "Elapsed Time: " << seconds << "s" << G4endl;
        G4cout << " " << G4endl;
        G4cout << " " << G4endl;

        if (detectedS2 > 0.) {
            man->FillNtupleDColumn(1, 0, std::log10(detectedS2)); //detectedS2 or detectedS2/detectedS1
            man->FillNtupleDColumn(1, 1, detectedS1);
            man->FillNtupleIColumn(1, 2, incidentRecoilType);
            man->AddNtupleRow(1);
        }
    }

    // Always replace the lightweight interface result, including for an event
    // with no recorded signal, so a previous event cannot be mistaken for this one.
    std::ofstream csvFile("output.csv");
    csvFile << "S1,S2\n";
    csvFile << detectedS1 << "," << detectedS2 << "\n";
    csvFile.close();

    // Write trajectory JSON for Three.js event display (always, regardless of signal)
    std::ofstream jf("event.json");
    jf << "{\"tracks\":[";
    bool firstTrack = true;
    for (auto& kv : fTracks) {
        const TrajectoryTrack& trk = kv.second;
        if (trk.pts.size() < 2) continue;
        if (!firstTrack) jf << ",";
        firstTrack = false;
        jf << "{\"id\":" << trk.id
           << ",\"parent\":" << trk.parentID
           << ",\"particle\":\"" << trk.particle << "\""
           << ",\"pts\":[";
        for (size_t i = 0; i < trk.pts.size(); ++i) {
            if (i) jf << ",";
            jf << "[" << trk.pts[i].x << "," << trk.pts[i].y << "," << trk.pts[i].z << "]";
        }
        jf << "]}";
    }
    jf << "],\"driftClusters\":[";
    bool firstDC = true;
    for (const auto& dc : fDriftClusters) {
        if (!firstDC) jf << ",";
        firstDC = false;
        jf << "{\"x\":" << dc.x << ",\"y\":" << dc.y << ",\"z\":" << dc.z << ",\"n\":" << dc.count << "}";
    }
    jf << "]}";
    jf.close();
}
