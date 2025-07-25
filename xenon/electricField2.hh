#ifndef ELECTRICFIELD2_HH
#define ELECTRICFIELD2_HH

#include "G4ElectroMagneticField.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include <cmath>

class ElectricField2 : public G4ElectroMagneticField
{
public:
    ElectricField2();
    virtual ~ElectricField2();

    virtual void GetFieldValue(const G4double Point[4], G4double *field) const override;
    virtual G4bool DoesFieldChangeEnergy() const override;
};

#endif