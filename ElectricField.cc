// #include "ElectricField.hh"
// using namespace CLHEP;


// ElectricField::ElectricField()
// : fElectricField(nullptr), fEquation(nullptr),
//   fStepper(nullptr),   fChordFinder(nullptr)
// {
//     // default: 1 kV / cm in +z
//     SetFieldValue(G4ThreeVector(0.0, 0.0, 10.0 * kilovolt / cm));
// }

// ElectricField::~ElectricField()
// {
//     delete fChordFinder;
//     delete fStepper;
//     delete fEquation;
//     delete fElectricField;
// }

// void ElectricField::SetFieldValue(const G4ThreeVector& fieldVector)
// {
//     // clean up any existing components
//     delete fChordFinder;  fChordFinder  = nullptr;
//     delete fStepper;      fStepper      = nullptr;
//     delete fEquation;     fEquation     = nullptr;
//     delete fElectricField;fElectricField= nullptr;

//     // new uniform field
//     fElectricField = new G4UniformElectricField(fieldVector);

//     // equation of motion, stepper, driver, chord‑finder
//     fEquation = new G4EqMagElectricField(fElectricField);
//     fStepper  = new G4DormandPrince745(fEquation, 8);

//     const G4double minStep = 0.1 * mm;          // 10 µm
//     auto driver = new G4IntegrationDriver<G4DormandPrince745>(
//                     minStep, fStepper, 8);

//     fChordFinder = new G4ChordFinder(driver);

//     // install in the global field manager
//     auto fm = G4TransportationManager::GetTransportationManager()
//                 ->GetFieldManager();
//     fm->SetDetectorField(fElectricField);
//     fm->SetChordFinder  (fChordFinder);
// }
