#include "electricField2.hh"

ElectricField2::ElectricField2()
{}

ElectricField2::~ElectricField2()
{}

void ElectricField2::GetFieldValue(const G4double Point[4], G4double *field) const
{
    static nestPart detector;
    static NEST::NESTcalc nestCalc(&detector);

    G4double x = Point[0] / 10;
    G4double y = Point[1] / 10;
    G4double z = Point[2] / 10;

    // Get electric field at this position from nestDetector
    double Efield = detector.get_ElectricField(x, y, z);
    double r = std::sqrt(x*x + y*y);
    double x_part = (r > 0 ? x / r : 0.);
    double y_part = (r > 0 ? y / r : 0.);

    field[0] = 0.;
    field[1] = 0.;
    field[2] = 0;
    field[3] = x_part * Efield *volt/cm;
    field[4] = y_part * Efield *volt/cm;

    field[5] = 0; 
}

G4bool ElectricField2::DoesFieldChangeEnergy() const
{
    return true;
}