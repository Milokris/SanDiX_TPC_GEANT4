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

    std::vector<G4double> E = 
    {6.50*eV, 6.70*eV, 6.90*eV, 7.00*eV, 7.05*eV, 7.10*eV, 7.15*eV, 7.20*eV,
    7.25*eV, 7.30*eV, 7.35*eV, 7.40*eV, 7.50*eV, 7.70*eV, 7.90*eV, 8.10*eV, 8.20*eV};
    const size_t N_E = E.size();

//World Volume
    G4Material *worldMat = nist->FindOrBuildMaterial("G4_AIR"); // G4_Galactic or G4_AIR
    G4MaterialPropertiesTable* mptAir = new G4MaterialPropertiesTable();

    //const G4int nEntries = 2;
    //G4double photonEnergy[nEntries] = {1.55*eV, 10.0*eV}; 
    //G4double rIndexAir[nEntries] = {1.0003, 1.0003};
    //mptAir->AddProperty("RINDEX", photonEnergy, rIndexAir, nEntries);

    std::vector<G4double> Air_RINDEX(N_E, 1.0003);
    std::vector<G4double> airAbsLength(N_E, 10.*m); 
    mptAir->AddProperty("RINDEX", E.data(), Air_RINDEX.data(), N_E);
    mptAir->AddProperty("ABSLENGTH", E.data(), airAbsLength.data(), N_E);

    worldMat->SetMaterialPropertiesTable(mptAir);

    G4Material* xenon = new G4Material("xenon", 3.0558*g/cm3, 1); //Based on 50 mm diameter, 100 mm height, 0.6 kg. 0.01*g/cm3, 1); //
    G4Element* elXe = new G4Element("Xenon", "Xe", 54., 131.3 * g/mole);
    xenon->AddElement(elXe, 1);    

    G4Box *solidWorld = new G4Box("solidWorld", 200*mm, 200*mm, 200*mm);

    G4LogicalVolume *logicWorld = new G4LogicalVolume(solidWorld, worldMat, "logicWorld"); //using xenon instead of worldMat
    logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());
    fLogicWorld = logicWorld;
    G4VPhysicalVolume *physWorld = new G4PVPlacement(0, G4ThreeVector(0.,
    0.,0.), logicWorld, "physWorld", 0, false, 0, true);

//Xenon Chamber
    G4MaterialPropertiesTable* xenonMPT = new G4MaterialPropertiesTable();
    std::vector<G4double> LXe_RINDEX(N_E, 1.69);
    std::vector<G4double> LXe_RAYLEIGH(N_E, 35.*cm);
    std::vector<G4double> LXe_ABS(N_E, 10.*m);
    xenonMPT->AddProperty("RINDEX",    &E[0], &LXe_RINDEX[0], N_E);
    xenonMPT->AddProperty("RAYLEIGH",  &E[0], &LXe_RAYLEIGH[0], N_E);
    xenonMPT->AddProperty("ABSLENGTH", &E[0], &LXe_ABS[0],      N_E);   
    //G4double rIndexXenon[nEntries] = {1.7, 1.7}; 
    //xenonMPT->AddProperty("RINDEX", photonEnergy, rIndexXenon, nEntries);
    xenonMPT->AddConstProperty("SCINTILLATIONYIELD", 0.0);

    xenon->SetMaterialPropertiesTable(xenonMPT);

    for (auto comp : { kSCINTILLATIONCOMPONENT1, kSCINTILLATIONCOMPONENT2, kSCINTILLATIONCOMPONENT3 }) {
        if (xenonMPT->GetProperty(comp)) {
            G4cout << comp << " exists!" << G4endl;
        } else {
            G4cout << comp << " NOT found" << G4endl;
        }
    }


//     std::vector<G4double> ScintphotonEnergy = {0.0 *eV, 0.0 *eV}; //{7.0 * eV, 7.5 * eV};
//     std::vector<G4double> scintYield = {0.0, 0.0}; //{1.0, 1.0};
//     xenonMPT->AddProperty("SCINTILLATIONCOMPONENT1", ScintphotonEnergy, scintYield);
// // Scintillation yield
//     xenonMPT->AddConstProperty("SCINTILLATIONYIELD", 0);//1000./MeV);  // or measured value
// //include electric field into scintillation yield?
//     xenonMPT->AddConstProperty("RESOLUTIONSCALE", 1.0);
//     xenonMPT->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 27.*ns, true);
//     xenonMPT->AddConstProperty("SCINTILLATIONYIELD1", 0);//1.0);  // all light in 1st component
//     xenonMPT->AddConstProperty("ELECTRONMOBILITY", 1.5e6 * mm2/volt/s, true);  
//     xenonMPT->AddConstProperty("ELECTRONLIFETIME", 3.0 * ms, true);


    G4MaterialPropertiesTable* mpt = xenon->GetMaterialPropertiesTable();
    if (mpt) {
        G4cout << "=== Xenon Material Properties Table ===" << G4endl;
        mpt->DumpTable();
    } else {
        G4cout << "No material properties table set for xenon!" << G4endl;
    }


    G4double xenonRadius = 25*mm; 
    G4double xenonHalfHeight = 50*mm;


    G4Tubs *xenonChamber = new G4Tubs("xenonChamber", 0, xenonRadius, xenonHalfHeight + 9*mm, 0.*deg, 360.*deg);
    G4LogicalVolume *logicChamber = new G4LogicalVolume(xenonChamber, xenon, "logicChamber");
    G4VPhysicalVolume *physChamber = new G4PVPlacement(0, G4ThreeVector(0, 0, 0), logicChamber, "physicalChamber", 
logicWorld, false, 0, checkOverlaps);
    auto xeVis = new G4VisAttributes(G4Colour(0.0, 0.0, 1.00, 0.75));  // blue, 75% opacity
    xeVis->SetForceSolid(true);
    logicChamber->SetVisAttributes(xeVis);

    static ElectricField2* electricField = new ElectricField2();

    // G4Region* chamberRegion = new G4Region("ChamberRegion");
    // logicChamber->SetRegion(chamberRegion);
    // chamberRegion->AddRootLogicalVolume(logicChamber);
    // new FastS2Model("S2FastSim", chamberRegion, electricField);

    G4ProductionCuts* cuts = new G4ProductionCuts();
    cuts->SetProductionCut(0.001*mm, "e-"); //should be around .001 *mm
    //chamberRegion->SetProductionCuts(cuts);

    // G4UserLimits* limits = new G4UserLimits(.01*mm);
    logicChamber->SetUserLimits(new DynamicUserLimits(10*mm, 0.1*um)); //min step size 0.1 micron


//Stainless Steel frame
    G4double steelDensity = 9.0*g/cm3;
    G4double steelHalfHeight = 54.5*mm;
    G4double steelHalfThickness = 4.5*mm;
    G4double steelRingThickness = 5.*mm;
    G4Material* steel304 = new G4Material("StainlessSteel", steelDensity, 3);
    steel304->AddElement(nist->FindOrBuildElement("Fe"), 70.*perCent);
    steel304->AddElement(nist->FindOrBuildElement("Cr"), 18.*perCent);
    steel304->AddElement(nist->FindOrBuildElement("Ni"), 12.*perCent);
    //G4double rIndexSteel[nEntries] = {2.5, 2.5};  // Approximate placeholder
    G4MaterialPropertiesTable* mptSteel = new G4MaterialPropertiesTable();
    //mptSteel->AddProperty("RINDEX", photonEnergy, rIndexSteel, nEntries);

    std::vector<G4double> STEEL_REFLECT(N_E, 0.30); //0.3 is default but i do not have data on this for SanDiX
    mptSteel->AddProperty("REFLECTIVITY", &E[0], &STEEL_REFLECT[0], N_E);
    steel304->SetMaterialPropertiesTable(mptSteel); 

    //steel surface
    G4OpticalSurface* steelSurf = new G4OpticalSurface("Steel_Surface");
    steelSurf->SetType(dielectric_metal);
    steelSurf->SetModel(unified);
    steelSurf->SetFinish(polished);
    G4MaterialPropertiesTable* steelSurfMPT = new G4MaterialPropertiesTable();
    steelSurfMPT->AddProperty("REFLECTIVITY", &E[0], &STEEL_REFLECT[0], N_E);
    steelSurf->SetMaterialPropertiesTable(steelSurfMPT);


    G4Tubs *steelFrame = new G4Tubs("steelFrame", xenonRadius, xenonRadius + steelRingThickness,
steelHalfThickness, 0, 360*deg);
    G4LogicalVolume *logicFrame = new G4LogicalVolume(steelFrame, steel304, "logicFrame");

    G4VPhysicalVolume *physFrame1 = new G4PVPlacement(0, G4ThreeVector(0, 0, steelHalfHeight), 
logicFrame, "physicalFrame1", logicWorld, false, 0, true);
    G4VPhysicalVolume *physFrame2 = new G4PVPlacement(0, G4ThreeVector(0, 0, -1*steelHalfHeight), 
logicFrame, "physicalFrame2", logicWorld, false, 0, true);
    auto steelVis = new G4VisAttributes(G4Colour(0, .8, 0, 0.5));  // green, 50% opacity
    steelVis->SetForceSolid(true);
    logicFrame->SetVisAttributes(steelVis);

    G4MaterialPropertiesTable* mpt2 = steel304->GetMaterialPropertiesTable();
    if (mpt2) {
        G4cout << "=== Steel Material Properties Table ===" << G4endl;
        mpt2->DumpTable();
    } else {
        G4cout << "No material properties table set for Steel!" << G4endl;
    }

    new G4LogicalSkinSurface("SteelSkin", logicFrame, steelSurf);


//Teflon Container
    G4Material* teflon = nist->FindOrBuildMaterial("G4_TEFLON");
    //G4double rIndexTeflon[nEntries] = {1.35, 1.35};
    G4MaterialPropertiesTable* mptTeflon = new G4MaterialPropertiesTable();
    // mptTeflon->AddProperty("RINDEX", photonEnergy, rIndexTeflon, nEntries);
    std::vector<G4double> PTFE_RINDEX(N_E, 1.575);
    std::vector<G4double> PTFE_ABS(N_E, 1.*mm);
    mptTeflon->AddProperty("RINDEX",    &E[0], &PTFE_RINDEX[0], N_E);
    mptTeflon->AddProperty("ABSLENGTH", &E[0], &PTFE_ABS[0],    N_E);
    teflon->SetMaterialPropertiesTable(mptTeflon);

    //Teflon skin
    G4OpticalSurface* ptfeSurf = new G4OpticalSurface("PTFE_Surface");
    ptfeSurf->SetType(dielectric_dielectric);
    ptfeSurf->SetModel(unified);
    ptfeSurf->SetFinish(groundfrontpainted);
    ptfeSurf->SetSigmaAlpha(0.1);

    G4MaterialPropertiesTable* ptfeSurfMPT = new G4MaterialPropertiesTable();
    std::vector<G4double> PTFE_REFLECT(N_E, 0.98); // sintered PTFE reflectivity in LXe
    ptfeSurfMPT->AddProperty("REFLECTIVITY", &E[0], &PTFE_REFLECT[0], N_E);
    ptfeSurfMPT->AddConstProperty("SPECULARLOBECONSTANT", 0.05, true);
    ptfeSurfMPT->AddConstProperty("SPECULARSPIKECONSTANT", 0.00, true);
    ptfeSurfMPT->AddConstProperty("BACKSCATTERCONSTANT",   0.10, true);
    ptfeSurf->SetMaterialPropertiesTable(ptfeSurfMPT);

    G4Box *outerBox = new G4Box("outerBox", xenonRadius + steelRingThickness, xenonRadius + steelRingThickness, xenonHalfHeight);
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

    new G4LogicalSkinSurface("PTFE_Skin", logicContainer, ptfeSurf);



//Teflon Caps
    G4double capHalfThickness = 5*mm;
    G4Box *teflonCap1 = new G4Box("teflonCap1", xenonRadius+steelRingThickness, xenonRadius+steelRingThickness, capHalfThickness);
    G4Box *teflonCap2 = new G4Box("teflonCap2", xenonRadius+steelRingThickness, xenonRadius+steelRingThickness, capHalfThickness);
    G4LogicalVolume *logicCap1 = new G4LogicalVolume(teflonCap1, teflon, "logicCap1");
    G4VPhysicalVolume *physCap1 = new G4PVPlacement(0, G4ThreeVector(0, 0, steelHalfHeight + steelHalfThickness + capHalfThickness), logicCap1, 
"physicalCap1", logicWorld, false, 0, true);
    G4LogicalVolume *logicCap2 = new G4LogicalVolume(teflonCap2, teflon, "logicCap2");
    G4VPhysicalVolume *physCap2 = new G4PVPlacement(0, G4ThreeVector(0, 0, -1*steelHalfHeight - steelHalfThickness - capHalfThickness), logicCap2, 
"physicalCap2", logicWorld, false, 0, true);
    logicCap1->SetVisAttributes(visTeflon);
    logicCap2->SetVisAttributes(visTeflon);

    G4MaterialPropertiesTable* mpt3 = teflon->GetMaterialPropertiesTable();
    if (mpt3) {
        G4cout << "=== Teflon Material Properties Table ===" << G4endl;
        mpt3->DumpTable();
    } else {
        G4cout << "No material properties table set for Teflon!" << G4endl;
    }

    new G4LogicalSkinSurface("PTFE_Skin", logicCap1, ptfeSurf);
    new G4LogicalSkinSurface("PTFE_Skin", logicCap2, ptfeSurf);



//PMTs

    G4Material* matFusedSilica = nist->FindOrBuildMaterial("G4_SILICON_DIOXIDE");
    G4MaterialPropertiesTable* mptSilica = new G4MaterialPropertiesTable();

    std::vector<G4double> SiO2_RINDEX;
    SiO2_RINDEX.reserve(N_E);
    for (auto e : E) 
    {
        G4double frac = ( (e/eV) - 6.5 ) / 0.7;
        if (frac < 0.) frac = 0.; if (frac > 1.) frac = 1.;
        G4double val = 1.56 + 0.04 * frac;  // 1.56 -> 1.60
        SiO2_RINDEX.push_back(val);
    }
    std::vector<G4double> SiO2_ABS(N_E, 1.*m);
    mptSilica->AddProperty("RINDEX",    &E[0], &SiO2_RINDEX[0], N_E);
    mptSilica->AddProperty("ABSLENGTH", &E[0], &SiO2_ABS[0],    N_E);
    matFusedSilica->SetMaterialPropertiesTable(mptSilica);

    //PMT surfaces
    auto pmtSurface = new G4OpticalSurface("LXe_to_PMT_Surface");
    pmtSurface->SetType(dielectric_dielectric);
    pmtSurface->SetModel(unified);
    pmtSurface->SetFinish(polished);
    pmtSurface->SetMaterialPropertiesTable(mptSilica);


    G4double PmtHalfLength = 10.25*mm;
    G4double PmtHalfWidth = 5.0*mm; //14.125*mm;

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
    logicDetector = new G4LogicalVolume(solidDetector, worldMat, "logicDetector"); //using xenon instead of worldMat
    G4VPhysicalVolume *physDetector = new G4PVPlacement(0, Pos1, logicDetector, "physDetector", logicWorld, false, 1, checkOverlaps);
    new G4LogicalBorderSurface("LXe_to_PMT1", physChamber, physDetector, pmtSurface);
    new G4LogicalBorderSurface("PMT_to_LXe1", physDetector, physChamber, pmtSurface);
    physDetector = new G4PVPlacement(rot90, Pos2, logicDetector, "physDetector", logicWorld, false, 2, checkOverlaps);
    new G4LogicalBorderSurface("LXe_to_PMT2", physChamber, physDetector, pmtSurface);
    new G4LogicalBorderSurface("PMT_to_LXe2", physDetector, physChamber, pmtSurface);
    physDetector = new G4PVPlacement(0, Pos3, logicDetector, "physDetector", logicWorld, false, 3, checkOverlaps);
    new G4LogicalBorderSurface("LXe_to_PMT3", physChamber, physDetector, pmtSurface);
    new G4LogicalBorderSurface("PMT_to_LXe3", physDetector, physChamber, pmtSurface);
    physDetector = new G4PVPlacement(rot90, Pos4, logicDetector, "physDetector", logicWorld, false, 4, checkOverlaps);
    new G4LogicalBorderSurface("LXe_to_PMT4", physChamber, physDetector, pmtSurface);
    new G4LogicalBorderSurface("PMT_to_LXe4", physDetector, physChamber, pmtSurface);
    physDetector = new G4PVPlacement(0, Pos5, logicDetector, "physDetector", logicWorld, false, 5, checkOverlaps);
    new G4LogicalBorderSurface("LXe_to_PMT5", physChamber, physDetector, pmtSurface);
    new G4LogicalBorderSurface("PMT_to_LXe5", physDetector, physChamber, pmtSurface);
    physDetector = new G4PVPlacement(rot90, Pos6, logicDetector, "physDetector", logicWorld, false, 6, checkOverlaps);
    new G4LogicalBorderSurface("LXe_to_PMT6", physChamber, physDetector, pmtSurface);
    new G4LogicalBorderSurface("PMT_to_LXe6", physDetector, physChamber, pmtSurface);
    physDetector = new G4PVPlacement(0, Pos7, logicDetector, "physDetector", logicWorld, false, 7, checkOverlaps);
    new G4LogicalBorderSurface("LXe_to_PMT7", physChamber, physDetector, pmtSurface);
    new G4LogicalBorderSurface("PMT_to_LXe7", physDetector, physChamber, pmtSurface);
    physDetector = new G4PVPlacement(rot90, Pos8, logicDetector, "physDetector", logicWorld, false, 8, checkOverlaps);
    new G4LogicalBorderSurface("LXe_to_PMT8", physChamber, physDetector, pmtSurface);
    new G4LogicalBorderSurface("PMT_to_LXe8", physDetector, physChamber, pmtSurface);

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

    new G4LogicalSkinSurface("SteelSkin", logicCasing, steelSurf);

    fScoringVolume = logicDetector;

//Anode Wire
    G4Material* tungsten = nist->FindOrBuildMaterial("G4_W");
    G4Tubs *anodeWire = new G4Tubs("anodeWire", 0, 12.5*um, xenonHalfHeight, 0.*deg, 360.*deg);
    G4LogicalVolume *logicAnode = new G4LogicalVolume(anodeWire, tungsten, "logicAnode");
    G4VPhysicalVolume *physAnode = new G4PVPlacement(0, G4ThreeVector(0, 0, 0), logicAnode, "physAnode", logicChamber, false, 0, true);
    
    auto wireVis = new G4VisAttributes(G4Colour(1.0, 1.0, 0.0, 1.0)); //yellow
    wireVis->SetForceSolid(true);
    logicAnode->SetVisAttributes(wireVis);

//Cathode Wires
    G4Tubs *cathodeWire = new G4Tubs("cathodeWire", 0, 109*um, xenonHalfHeight, 0.*deg, 360.*deg);
    G4LogicalVolume *logicCathode = new G4LogicalVolume(cathodeWire, steel304, "logicCathode");

    double angleIncrement = 360./20;
    G4double cathodeRadius = xenonRadius - 120*um;
    for (int i = 0; i < 20; ++i)
    {
        G4double cathodeAngle = i * angleIncrement *deg;
        G4double cathodeX = cathodeRadius * std::cos(cathodeAngle);
        G4double cathodeY = cathodeRadius * std::sin(cathodeAngle);
        G4ThreeVector cathodePosition(cathodeX, cathodeY, 0);
        std::string physCathodeName = "physCathode_" + std::to_string(i);

        G4VPhysicalVolume *physCathode = new G4PVPlacement(0, cathodePosition, logicCathode, physCathodeName, logicChamber, false, i, true);
    }
    logicCathode->SetVisAttributes(wireVis);



//Radial Electric Field

    electricField2 = new ElectricField2();

    equation = new G4EqMagElectricField(electricField2);

    G4int nvar = 8;

    auto pStepper = new G4DormandPrince745(equation, nvar);

    auto globalFieldManager = G4TransportationManager::GetTransportationManager()->GetFieldManager();
    globalFieldManager->SetDetectorField(electricField2);

    auto localFieldManager = new G4FieldManager(electricField2);
    localFieldManager->SetDetectorField(electricField2);


                              // Relative accuracy values:
    //G4double minEps= 1.0e-5;  //   Minimum & value for largest steps
    //G4double maxEps= 1.0e-4;  //   Maximum & value for smallest steps

    //globalFieldManager->SetMinimumEpsilonStep( minEps );
    //globalFieldManager->SetMaximumEpsilonStep( maxEps );
    //globalFieldManager->SetDeltaOneStep( 0.5e-3 * mm );  // 0.5 micrometer



    G4double minStep = 0.001*mm;
    auto pIntegrationDriver = new G4IntegrationDriver<G4DormandPrince745>(minStep, pStepper, nvar);

    chordFinder2 = new G4ChordFinder(pIntegrationDriver);
    globalFieldManager->SetChordFinder(chordFinder2);
    localFieldManager->SetChordFinder(chordFinder2);

    //G4TransportationManager::GetTransportationManager()->SetFieldManager(globalFieldManager);


    //logicChamber->SetFieldManager(localFieldManager, true);
    logicWorld->SetFieldManager(globalFieldManager, true);

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
