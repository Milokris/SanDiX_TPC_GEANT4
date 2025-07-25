// #ifndef ELECTRIC_FIELD_HH
// #define ELECTRIC_FIELD_HH

// #include "G4ElectricField.hh"
// #include "G4UniformElectricField.hh"
// #include "G4EqMagElectricField.hh"
// #include "G4DormandPrince745.hh"
// #include "G4FieldManager.hh"
// #include "G4ChordFinder.hh"
// #include "G4TransportationManager.hh"
// #include "globals.hh"

// class ElectricField {
// public:
//     ElectricField();                       // installs 1 kV/cm in +z by default
//     ~ElectricField();

//     void            SetFieldValue(const G4ThreeVector& fieldVector);
//     G4ElectricField* GetField() const { return fElectricField; }

// private:
//     G4ElectricField*        fElectricField;
//     G4EqMagElectricField*   fEquation;
//     G4DormandPrince745*     fStepper;
//     G4ChordFinder*          fChordFinder;
// };

// #endif

