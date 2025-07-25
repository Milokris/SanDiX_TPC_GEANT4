#ifndef CONSTRUCTION_HH
#define CONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "G4VPhysicalVolume.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Tubs.hh"
#include "G4VisAttributes.hh"
//#include "ElectricField.hh"
#include "G4FieldManager.hh"
#include "G4EqMagElectricField.hh"
#include "G4ClassicalRK4.hh"
#include "G4ChordFinder.hh"
#include "G4SubtractionSolid.hh"
#include <cmath>
#include "detector.hh"
#include "electricField2.hh"
#include "G4DormandPrince745.hh"
#include "G4TransportationManager.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4Material.hh"

const double pi = 3.14159265;
class MyDetectorConstruction : public G4VUserDetectorConstruction
{
public:
    MyDetectorConstruction();
    ~MyDetectorConstruction();

    G4ElectroMagneticField *electricField2;
    G4EqMagElectricField *equation;
    G4ChordFinder *chordFinder2;
    G4LogicalVolume* GetScoringVolume() const;

    virtual G4VPhysicalVolume *Construct();

private:
    G4LogicalVolume *logicDetector;
    G4LogicalVolume *fLogicWorld = nullptr;
    G4LogicalVolume* fScoringVolume = nullptr;
    virtual void ConstructSDandField();
};


#endif