#include "construction.hh"

MyDetectorConstruction::MyDetectorConstruction()
{}

MyDetectorConstruction::~MyDetectorConstruction()
{}

G4VPhysicalVolume *MyDetectorConstruction::Construct()
{
    G4bool checkOverlaps = true;
    G4double overlapTolerance = 0.001*mm;  // 1 micron tolerance

    G4NistManager *nist = G4NistManager::Instance();

//World Volume
    G4Material *worldMat = nist->FindOrBuildMaterial("G4_Galactic");
    G4MaterialPropertiesTable* mptAir = new G4MaterialPropertiesTable();
    const G4int nEntries = 2;
    G4double photonEnergy[nEntries] = {1.55*eV, 10.0*eV}; 
    G4double rIndex[nEntries] = {1.0003, 1.0003};
    mptAir->AddProperty("RINDEX", photonEnergy, rIndex, nEntries);
    worldMat->SetMaterialPropertiesTable(mptAir);

    G4Box *solidWorld = new G4Box("solidWorld", 200*mm, 200*mm, 200*mm);

    G4LogicalVolume *logicWorld = new G4LogicalVolume(solidWorld, worldMat, "logicWorld");
    logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());
    fLogicWorld = logicWorld;
    G4VPhysicalVolume *physWorld = new G4PVPlacement(0, G4ThreeVector(0.,
    0.,0.), logicWorld, "physWorld", 0, false, 0, true);

//Xenon Chamber
    G4Material* xenon = new G4Material("xenon", 3.02*g/cm3, 1);
    G4Element* elXe = new G4Element("Xenon", "Xe", 54., 131.3 * g/mole);
    xenon->AddElement(elXe, 1);    
        //This is at STP, need special instructions to replicate TPC
    G4MaterialPropertiesTable* xenonMPT = new G4MaterialPropertiesTable();
    G4double xenonRIndex[nEntries] = {1.7, 1.7}; 
    xenonMPT->AddProperty("RINDEX", photonEnergy, xenonRIndex, nEntries);

    std::vector<G4double> ScintphotonEnergy = {7.0 * eV, 7.5 * eV};
    std::vector<G4double> scintYield = {1.0, 1.0};
    xenonMPT->AddProperty("SCINTILLATIONCOMPONENT1", ScintphotonEnergy, scintYield);
// Scintillation yield
    xenonMPT->AddConstProperty("SCINTILLATIONYIELD", 1000./MeV);  // or measured value
//include electric field into scintillation yield?
    xenonMPT->AddConstProperty("RESOLUTIONSCALE", 1.0);
    xenonMPT->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 27.*ns, true);
    xenonMPT->AddConstProperty("SCINTILLATIONYIELD1", 1.0);  // all light in 1st component
    xenonMPT->AddConstProperty("ELECTRONMOBILITY", 1.5e6 * mm2/volt/s, true);  
    xenonMPT->AddConstProperty("ELECTRONLIFETIME", 3.0 * ms, true);

    xenon->SetMaterialPropertiesTable(xenonMPT);


    G4double xenonRadius = 50*mm;
    G4double xenonHalfHeight = 50*mm;

    G4Tubs *xenonChamber = new G4Tubs("xenonChamber", 0, xenonRadius, xenonHalfHeight, 0.*deg, 360.*deg);
    G4LogicalVolume *logicChamber = new G4LogicalVolume(xenonChamber, xenon, "logicChamber");
    G4VPhysicalVolume *physChamber = new G4PVPlacement(0, G4ThreeVector(0, 0, 0), logicChamber, "physicalChamber", 
logicWorld, false, 0, checkOverlaps);
    auto xeVis = new G4VisAttributes(G4Colour(0.0, 0.0, 1.00, 0.75));  // blue, 75% opacity
    xeVis->SetForceSolid(true);
    logicChamber->SetVisAttributes(xeVis);


//Stainless Steel frame
    G4double steelDensity = 9.0*g/cm3;
    G4double steelHalfHeight = 59*mm;
    G4double steelHalfThickness = 4.5*mm;
    G4Material* steel304 = new G4Material("StainlessSteel", steelDensity, 3);
    steel304->AddElement(nist->FindOrBuildElement("Fe"), 70.*perCent);
    steel304->AddElement(nist->FindOrBuildElement("Cr"), 18.*perCent);
    steel304->AddElement(nist->FindOrBuildElement("Ni"), 12.*perCent);

    G4Tubs *steelFrame1 = new G4Tubs("steelFrame1", xenonRadius, xenonRadius + 10*mm,
steelHalfThickness, 0, 360*deg);
    G4Tubs *steelFrame2 = new G4Tubs("steelFrame2", xenonRadius, xenonRadius + 10*mm,
steelHalfThickness, 0, 360*deg);
    //I am estimating that the steel vessel has a radius 10 mm greater
    //than the Xenon but I do not have the exact measurement for this
    G4LogicalVolume *logicFrame1 = new G4LogicalVolume(steelFrame1, steel304, "logicFrame1");
    G4VPhysicalVolume *physFrame1 = new G4PVPlacement(0, G4ThreeVector(0, 0, steelHalfHeight - steelHalfThickness), 
logicFrame1, "physicalFrame1", logicWorld, false, 0, true);
    G4LogicalVolume *logicFrame2 = new G4LogicalVolume(steelFrame2, steel304, "logicFrame2");
    G4VPhysicalVolume *physFrame2 = new G4PVPlacement(0, G4ThreeVector(0, 0, -1*steelHalfHeight + steelHalfThickness), 
logicFrame2, "physicalFrame2", logicWorld, false, 0, true);
    auto steelVis = new G4VisAttributes(G4Colour(0, .8, 0, 0.5));  // green, 50% opacity
    steelVis->SetForceSolid(true);
    logicFrame1->SetVisAttributes(steelVis);
    logicFrame2->SetVisAttributes(steelVis);

//Teflon Container
    G4Material* teflon = nist->FindOrBuildMaterial("G4_TEFLON");
    G4Box *outerBox = new G4Box("outerBox", xenonRadius +10*mm, xenonRadius+10*mm, xenonHalfHeight);
    G4Tubs *innerCylinder = new G4Tubs("innerCylinder", 0, xenonRadius, xenonHalfHeight, 
0, 360*deg);

    G4double PmtHalfLengthHole = 12.85*mm;
    G4SubtractionSolid *teflonCutout = new G4SubtractionSolid("teflonCutout", outerBox, innerCylinder, 0,
G4ThreeVector(0, 0, 0));
    G4Box *PmtCutout1 = new G4Box("PmtCutout1", 15*mm, PmtHalfLengthHole, PmtHalfLengthHole);
    G4Box *PmtCutout2 = new G4Box("PmtCutout2", PmtHalfLengthHole, 15*mm, PmtHalfLengthHole);
    G4ThreeVector pos1(xenonRadius, 0, xenonHalfHeight/2);
    G4ThreeVector pos2(0, xenonRadius, xenonHalfHeight/2);
    G4ThreeVector pos3(-1*xenonRadius, 0, xenonHalfHeight/2);
    G4ThreeVector pos4(0, -1*xenonRadius, xenonHalfHeight/2);
    G4ThreeVector pos5(xenonRadius, 0, -1*xenonHalfHeight/2);
    G4ThreeVector pos6(0, xenonRadius, -1*xenonHalfHeight/2);
    G4ThreeVector pos7(-1*xenonRadius, 0, -1*xenonHalfHeight/2);
    G4ThreeVector pos8(0, -1*xenonRadius, -1*xenonHalfHeight/2);
    G4SubtractionSolid *currentSolid1 = new G4SubtractionSolid("Solid1", teflonCutout, PmtCutout1, 0, pos1);
    G4SubtractionSolid *currentSolid2 = new G4SubtractionSolid("Solid2", currentSolid1, PmtCutout2, 0, pos2);
    G4SubtractionSolid *currentSolid3 = new G4SubtractionSolid("Solid3", currentSolid2, PmtCutout1, 0, pos3);
    G4SubtractionSolid *currentSolid4 = new G4SubtractionSolid("Solid4", currentSolid3, PmtCutout2, 0, pos4);
    G4SubtractionSolid *currentSolid5 = new G4SubtractionSolid("Solid5", currentSolid4, PmtCutout1, 0, pos5);
    G4SubtractionSolid *currentSolid6 = new G4SubtractionSolid("Solid6", currentSolid5, PmtCutout2, 0, pos6);
    G4SubtractionSolid *currentSolid7 = new G4SubtractionSolid("Solid7", currentSolid6, PmtCutout1, 0, pos7);
    G4SubtractionSolid *currentSolid8 = new G4SubtractionSolid("Solid8", currentSolid7, PmtCutout2, 0, pos8);


    G4LogicalVolume *logicContainer = new G4LogicalVolume(currentSolid8, teflon, "logicContainer");
    G4VPhysicalVolume *physContainer = new G4PVPlacement(0, G4ThreeVector(0, 0, 0), logicContainer,
"physicalContainer", logicWorld, false, 0, checkOverlaps);
    G4VisAttributes* visTeflon = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5, 0.3)); // grey, 30% opaque
    visTeflon->SetForceSolid(true);
    logicContainer->SetVisAttributes(visTeflon);


//Teflon Caps
    G4Box *teflonCap1 = new G4Box("teflonCap1", xenonRadius+10*mm, xenonRadius+10*mm, 4.5*mm);
    G4Box *teflonCap2 = new G4Box("teflonCap2", xenonRadius+10*mm, xenonRadius+10*mm, 4.5*mm);
    G4LogicalVolume *logicCap1 = new G4LogicalVolume(teflonCap1, teflon, "logicCap1");
    G4VPhysicalVolume *physCap1 = new G4PVPlacement(0, G4ThreeVector(0, 0, xenonHalfHeight + 13.5*mm), logicCap1, 
"physicalCap1", logicWorld, false, 0, true);
    G4LogicalVolume *logicCap2 = new G4LogicalVolume(teflonCap2, teflon, "logicCap2");
    G4VPhysicalVolume *physCap2 = new G4PVPlacement(0, G4ThreeVector(0, 0, -1*xenonHalfHeight - 13.5*mm), logicCap2, 
"physicalCap2", logicWorld, false, 0, true);
    logicCap1->SetVisAttributes(visTeflon);
    logicCap2->SetVisAttributes(visTeflon);

//PMTs
    G4double PmtHalfLength = 10.25*mm;
    G4double PmtHalfWidth = 14.125*mm;

    G4ThreeVector Pos1(xenonRadius + PmtHalfWidth, 0, xenonHalfHeight/2);
    G4ThreeVector Pos2(0, xenonRadius + PmtHalfWidth, xenonHalfHeight/2);
    G4ThreeVector Pos3(-1*xenonRadius - PmtHalfWidth, 0, xenonHalfHeight/2);
    G4ThreeVector Pos4(0, -1*xenonRadius - PmtHalfWidth, xenonHalfHeight/2);
    G4ThreeVector Pos5(xenonRadius + PmtHalfWidth, 0, -1*xenonHalfHeight/2);
    G4ThreeVector Pos6(0, xenonRadius + PmtHalfWidth, -1*xenonHalfHeight/2);
    G4ThreeVector Pos7(-1*xenonRadius - PmtHalfWidth, 0, -1*xenonHalfHeight/2);
    G4ThreeVector Pos8(0, -1*xenonRadius - PmtHalfWidth, -1*xenonHalfHeight/2);
    G4RotationMatrix* rot90 = new G4RotationMatrix();
    rot90->rotateZ(90 * deg);
    G4Box *solidDetector = new G4Box("solidDetector", PmtHalfWidth, PmtHalfLength, PmtHalfLength);
    logicDetector = new G4LogicalVolume(solidDetector, worldMat, "logicDetector");
    G4VPhysicalVolume *physDetector = new G4PVPlacement(0, Pos1, logicDetector, "physDetector", logicWorld, false, 1, checkOverlaps);
    physDetector = new G4PVPlacement(rot90, Pos2, logicDetector, "physDetector", logicWorld, false, 2, checkOverlaps);
    physDetector = new G4PVPlacement(0, Pos3, logicDetector, "physDetector", logicWorld, false, 3, checkOverlaps);
    physDetector = new G4PVPlacement(rot90, Pos4, logicDetector, "physDetector", logicWorld, false, 4, checkOverlaps);
    physDetector = new G4PVPlacement(0, Pos5, logicDetector, "physDetector", logicWorld, false, 5, checkOverlaps);
    physDetector = new G4PVPlacement(rot90, Pos6, logicDetector, "physDetector", logicWorld, false, 6, checkOverlaps);
    physDetector = new G4PVPlacement(0, Pos7, logicDetector, "physDetector", logicWorld, false, 7, checkOverlaps);
    physDetector = new G4PVPlacement(rot90, Pos8, logicDetector, "physDetector", logicWorld, false, 8, checkOverlaps);

    G4Box *steelBox = new G4Box("steelbox", PmtHalfWidth, PmtHalfLengthHole, PmtHalfLengthHole);
    G4Box *steelCutout = new G4Box("steelCutout", PmtHalfWidth,  PmtHalfLength, PmtHalfLength);
    G4SubtractionSolid *PmtCasing = new G4SubtractionSolid("PmtCasing", steelBox, steelCutout, 0, G4ThreeVector(0,0,0));
    G4LogicalVolume *logicCasing = new G4LogicalVolume(PmtCasing, steel304, "logicCasing");
    
    G4VPhysicalVolume *physCasing1 = new G4PVPlacement(0, Pos1, logicCasing, "physCasing1", logicWorld, false, 0, checkOverlaps);
    G4VPhysicalVolume *physCasing2 = new G4PVPlacement(rot90, Pos2, logicCasing, "physCasing2", logicWorld, false, 0, checkOverlaps);
    G4VPhysicalVolume *physCasing3 = new G4PVPlacement(0, Pos3, logicCasing, "physCasing3", logicWorld, false, 0, checkOverlaps);
    G4VPhysicalVolume *physCasing4 = new G4PVPlacement(rot90, Pos4, logicCasing, "physCasing4", logicWorld, false, 0, checkOverlaps);
    G4VPhysicalVolume *physCasing5 = new G4PVPlacement(0, Pos5, logicCasing, "physCasing5", logicWorld, false, 0, checkOverlaps);
    G4VPhysicalVolume *physCasing6 = new G4PVPlacement(rot90, Pos6, logicCasing, "physCasing6", logicWorld, false, 0, checkOverlaps);
    G4VPhysicalVolume *physCasing7 = new G4PVPlacement(0, Pos7, logicCasing, "physCasing7", logicWorld, false, 0, checkOverlaps);
    G4VPhysicalVolume *physCasing8 = new G4PVPlacement(rot90, Pos8, logicCasing, "physCasing8", logicWorld, false, 0, checkOverlaps);

    G4VisAttributes* visPmt = new G4VisAttributes(G4Colour(0.5, 0.5, 0.75, 1.0)); // bluish-gray, 90% opaque
    visPmt->SetForceSolid(true);
    logicDetector->SetVisAttributes(visPmt);
    logicCasing->SetVisAttributes(visTeflon);

    fScoringVolume = logicDetector;


// //Radal Electric Field
//     const G4double V0   = 4000.0 * volt;
//     const G4double rMin = 0.1    * mm;

//     auto* eField     = new RadialElectricField(V0, rMin);
//     auto* equation   = new G4EqMagElectricField(eField);
//     auto* stepper    = new G4ClassicalRK4(equation, 8);
//     //Using Integration Driver
//     auto* intDriver  = new G4MagInt_Driver(1.0e-2 * mm, stepper, stepper->GetNumberOfVariables());
//     auto* chordFinder = new G4ChordFinder(intDriver);

//     auto* fieldManager = new G4FieldManager();
//     fieldManager->SetDetectorField(eField);  
//     fieldManager->SetChordFinder(chordFinder);   

//     logicChamber->SetFieldManager(fieldManager, true);


//Uniform Electric Field

    electricField2 = new ElectricField2();

    equation = new G4EqMagElectricField(electricField2);

    G4int nvar = 8;

    auto pStepper = new G4DormandPrince745(equation, nvar);

    auto globalFieldManager = G4TransportationManager::GetTransportationManager()->GetFieldManager();

    auto localFieldManager = new G4FieldManager(electricField2);
    localFieldManager->SetDetectorField(electricField2);

    G4double minStep = 0.001*mm;
    auto pIntegrationDriver = new G4IntegrationDriver<G4DormandPrince745>(minStep, pStepper, nvar);

    chordFinder2 = new G4ChordFinder(pIntegrationDriver);
    globalFieldManager->SetChordFinder(chordFinder2);
    localFieldManager->SetChordFinder(chordFinder2);

    logicWorld->SetFieldManager(localFieldManager, true);


    return physWorld;
}


G4LogicalVolume* MyDetectorConstruction::GetScoringVolume() const 
{
    return fScoringVolume;
}

void MyDetectorConstruction::ConstructSDandField()
{
   MySensitiveDetector *sensDet = new MySensitiveDetector("SensitiveDetector");
       
    logicDetector->SetSensitiveDetector(sensDet);
    // auto electricField = new ElectricField();
    // auto fieldManager = G4TransportationManager::GetTransportationManager()->GetFieldManager();
    // fLogicWorld->SetFieldManager(fieldManager, true);  // Apply to world and all daughters

}
