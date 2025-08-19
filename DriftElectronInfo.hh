#ifndef DRIFT_ELECTRON_INFO_HH
#define DRIFT_ELECTRON_INFO_HH

#include "G4VUserTrackInformation.hh"

class DriftElectronInfo : public G4VUserTrackInformation {
public:
    DriftElectronInfo(G4bool isDrift = false, G4bool fromDrift = false) 
        : fIsDrift(isDrift), fIsFromDrift(fromDrift) {}
    virtual ~DriftElectronInfo() {}

    inline void SetIsDrift(G4bool val) { fIsDrift = val; }
    inline G4bool IsDrift() const { return fIsDrift; }

    inline void SetIsFromDrift(G4bool val) { fIsFromDrift = val; }
    inline G4bool IsFromDrift() const { return fIsFromDrift; }

private:
    G4bool fIsDrift;
    G4bool fIsFromDrift; // true for ALL descendants of a drift e-

};

#endif