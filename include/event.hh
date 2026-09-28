#ifndef EVENT_HH
#define EVENT_HH

#include "G4UserEventAction.hh"
#include "G4Event.hh"
#include "G4AnalysisManager.hh"
#include <unordered_set>
#include "G4Types.hh"
#include <fstream>
#include <vector>
#include <string>
#include <map>

struct TrajectoryPoint { float x, y, z; };
struct TrajectoryTrack {
    int id = 0, parentID = 0;
    std::string particle;
    std::vector<TrajectoryPoint> pts;
};

struct DriftCluster { float x, y, z; int count; };

class MySteppingAction;

class MyEventAction : public G4UserEventAction
{
public:
    MyEventAction(MySteppingAction* steppingAction);
    ~MyEventAction();

    virtual void BeginOfEventAction(const G4Event*);
    virtual void EndOfEventAction(const G4Event*);

    // Accumulator methods called from SteppingAction
    void AddS1Photons(G4int n)  { totalS1Photons += n; nS1Events++; }
    void AddS2Photons(G4int n)  { totalS2Photons += n; nS2Events++; }
    void AddElectrons(G4int n)  { createdElectrons += n; }
    void AddDriftTrackID(G4int id) { DriftTrackIDs.insert(id); }
    bool IsDriftTrack(G4int id) const { return DriftTrackIDs.count(id) > 0; }
    void ClearDriftTracks()     { DriftTrackIDs.clear(); }

    void RecordStep(int trkID, int parentID, const std::string& pname, float x, float y, float z);
    void AddDriftCluster(float x, float y, float z, int count);

    // Read accessors
    G4int GetTotalS1Photons()   const { return totalS1Photons; }
    G4int GetTotalS2Photons()   const { return totalS2Photons; }
    G4int GetCreatedElectrons() const { return createdElectrons; }
    G4double GetDriftTime()     const { return driftTime; }
    void SetDriftTime(G4double t)     { driftTime = t; }

    G4int  incidentRecoilType;
    G4int  photPerE = 17;

private:
    G4int    totalS1Photons;
    G4int    totalS2Photons;
    G4int    nS1Events;
    G4int    nS2Events;
    G4int    createdElectrons;
    G4double driftTime;
    G4double fEdep;
    std::unordered_set<G4int> DriftTrackIDs;
    std::map<int, TrajectoryTrack> fTracks;
    std::vector<DriftCluster> fDriftClusters;
	MySteppingAction* fSteppingAction;
};

#endif