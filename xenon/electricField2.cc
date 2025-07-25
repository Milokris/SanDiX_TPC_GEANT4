#include "electricField2.hh"

ElectricField2::ElectricField2()
{}

ElectricField2::~ElectricField2()
{}

void ElectricField2::GetFieldValue(const G4double Point[4], G4double *field) const
{
    G4double x = Point[0];
    G4double y = Point[1];
    G4double z = Point[2];

    field[0] = 0.;
    field[1] = 0.;
    field[2] = 0;
    field[3] = 0.;
    field[4] = 0.;

    field[5] = -5 * kilovolt/cm;
}

G4bool ElectricField2::DoesFieldChangeEnergy() const
{
    return true;
}