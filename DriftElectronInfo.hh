#ifndef DRIFT_ELECTRON_INFO_HH
#define DRIFT_ELECTRON_INFO_HH

#include "G4VUserTrackInformation.hh"

class DriftElectronInfo : public G4VUserTrackInformation {
public:
    DriftElectronInfo(G4bool isDrift = false) 
        : fIsDrift(isDrift) {}
    virtual ~DriftElectronInfo() {}

    inline void SetIsDrift(G4bool val) { fIsDrift = val; }
    inline G4bool IsDrift() const { return fIsDrift; }

private:
    G4bool fIsDrift;
};

#endif