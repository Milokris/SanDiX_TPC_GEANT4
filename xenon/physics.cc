#include "physics.hh"


MyPhysicsList::MyPhysicsList()
{
    RegisterPhysics (new G4EmStandardPhysics());
    RegisterPhysics(new G4HadronPhysicsQGSP_BERT());
    RegisterPhysics(new G4DecayPhysics());
    RegisterPhysics(new G4IonElasticPhysics());
    RegisterPhysics(new G4IonPhysics());
    RegisterPhysics(new G4StoppingPhysics());
    RegisterPhysics(new G4EmLivermorePhysics()); 
    RegisterPhysics(new G4EmExtraPhysics());
    RegisterPhysics(new G4StepLimiterPhysics());
    G4ProductionCutsTable::GetProductionCutsTable()->SetEnergyRange(.01*eV, 1*GeV);

    G4OpticalPhysics* opticalPhysics = new G4OpticalPhysics();
    RegisterPhysics(opticalPhysics);
}

MyPhysicsList::~MyPhysicsList()
{}