#ifndef STEPPING_HH
#define STEPPING_HH

#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "globals.hh"
#include "G4RunManager.hh"
#include <map>
#include <cmath>
#include "construction.hh"
#include "tracking.hh"
//#include "event.hh"
class MyEventAction; // forward declaration — no include needed in the header
#include "run.hh"
#include "G4Electron.hh"
#include "G4Neutron.hh"
#include "G4OpticalPhoton.hh"
#include "G4Gamma.hh"
#include "nestFile.hh"
#include "NEST.hh"
#include "DriftElectronInfo.hh"
#include "DynamicUserLimits.hh"
#include "G4VProcess.hh"
#include "CLHEP/Random/RandGaussZiggurat.h"
#include <unordered_set>
#include <tuple>

extern G4double gainArea;

class MySteppingAction : public G4UserSteppingAction
{
    public:
    MySteppingAction(MyEventAction* eventAction);
    ~MySteppingAction();

    virtual void UserSteppingAction(const G4Step*);
    void ClearStagnationData(G4int eventID, G4int trackID);
    void ClearAllStagnationData();
    void SetEventAction(MyEventAction* eventAction)
    {
        fEventAction = eventAction;
    }

    private:
    MyEventAction* fEventAction;
    struct TrackKey
    {
        G4int eventID;
        G4int trackID;
        bool operator<(const TrackKey& other)
        const {
            return std::tie(eventID, trackID) < std::tie(other.eventID, other.trackID);
        }
    };

    std::map<TrackKey, G4double> previousEnergy;
    std::map<TrackKey, G4int> stagnationCounter;
};

inline G4ThreeVector RandomUnitVector()
{
    double costheta = 2.*G4UniformRand() - 1.;
    double sintheta = std::sqrt(1. - costheta*costheta);
    double phi = 2.*CLHEP::pi*G4UniformRand();
    return G4ThreeVector(sintheta*cos(phi), sintheta*sin(phi), costheta);
}

inline G4double SampleLXePhotonEnergy_GaussEnergy()
{
    const double mean_eV = 1239.8419843320026/178.0;
    const double sigma_eV = 0.20;
    double E_eV = CLHEP::RandGauss::shoot(mean_eV, sigma_eV);
    if (E_eV < 6.5) E_eV = 6.5;
    if (E_eV > 8.2) E_eV = 8.2;
    return E_eV*eV;
}
#endif






















//-----------This code is for clustering S1 events together, which should be more physically accurate, but it doesn't seem to effect anything. keeping it just in case I need it.
//Make sure to change both stepping.hh and stepping.cc



// #ifndef STEPPING_HH
// #define STEPPING_HH

// #include "G4UserSteppingAction.hh"
// #include "G4Step.hh"
// #include "G4Track.hh"
// #include "globals.hh"
// #include "G4RunManager.hh"
// #include <map>
// #include <cmath>
// #include "construction.hh"
// #include "tracking.hh"
// //#include "event.hh"
// class MyEventAction;   // forward declaration — no include needed in the header

// #include "run.hh"
// #include "G4Electron.hh"
// #include "G4Neutron.hh"
// #include "G4OpticalPhoton.hh"
// #include "G4Gamma.hh"
// #include "nestFile.hh"
// #include "NEST.hh"
// #include "DriftElectronInfo.hh"
// #include "DynamicUserLimits.hh"
// #include "G4VProcess.hh"
// #include "CLHEP/Random/RandGaussZiggurat.h"
// #include <unordered_set>
// #include <tuple>

// extern double gainArea;

// class MySteppingAction : public G4UserSteppingAction
// {
// public:
//     MySteppingAction(MyEventAction* eventAction);
//     ~MySteppingAction();

//     virtual void UserSteppingAction(const G4Step*);
//     void ClearStagnationData(G4int eventID, G4int trackID);
//     void ClearAllStagnationData();
//     void SetEventAction(MyEventAction* eventAction) { fEventAction = eventAction; }


// private:
//     MyEventAction* fEventAction;
//     struct TrackKey {
//         G4int eventID;
//         G4int trackID;

//         bool operator<(const TrackKey& other) const {
//             return std::tie(eventID, trackID) < std::tie(other.eventID, other.trackID);
//         }
//     };

//     std::map<TrackKey, G4double> previousEnergy;
//     std::map<TrackKey, G4int>    stagnationCounter;

//     struct Cluster {
//     G4ThreeVector pos;
//     double edep = 0.0;
//     int trackID = -1;
//     bool active = false;
//     NEST::INTERACTION_TYPE recoilType = NEST::NoneType;
//     double beforeEnergy = 0.0;
//     };

//     std::map<TrackKey, Cluster> clusters;

//     void ProcessCluster(const Cluster& c, NEST::INTERACTION_TYPE recoilType, G4Track* track, double beforeEnergy, const G4Step* step);
// };

// inline G4ThreeVector RandomUnitVector()
// {
//     double costheta = 2.*G4UniformRand() - 1.;
//     double sintheta = std::sqrt(1. - costheta*costheta);
//     double phi      = 2.*CLHEP::pi*G4UniformRand();
//     return G4ThreeVector(sintheta*cos(phi), sintheta*sin(phi), costheta);
// }

// inline G4double SampleLXePhotonEnergy_GaussEnergy()
// {
//     const double mean_eV  = 1239.8419843320026/178.0;
//     const double sigma_eV = 0.20;
//     double E_eV = CLHEP::RandGauss::shoot(mean_eV, sigma_eV);
//     if (E_eV < 6.5) E_eV = 6.5;
//     if (E_eV > 8.2) E_eV = 8.2;
//     return E_eV*eV;
// }


// #endif
