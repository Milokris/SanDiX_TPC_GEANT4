#include "physics.hh"


MyPhysicsList::MyPhysicsList()
{
    //G4VModularPhysicsList::ConstructProcess();  // construct all physics

    //RegisterPhysics (new G4EmStandardPhysics());
    //RegisterPhysics(new G4EmExtraPhysics());
    //RegisterPhysics(new G4EmLivermorePhysics()); 
    RegisterPhysics(new G4EmLowEPPhysics());

    RegisterPhysics(new G4HadronPhysicsQGSP_BERT());
    RegisterPhysics(new G4DecayPhysics());
    //RegisterPhysics(new G4IonElasticPhysics());
    //RegisterPhysics(new G4IonPhysics());
    RegisterPhysics(new G4StoppingPhysics());
    RegisterPhysics(new G4StepLimiterPhysics());
    RegisterPhysics(new G4OpticalPhysics());


    G4ProductionCutsTable::GetProductionCutsTable()->SetEnergyRange(.1*eV, 1.*GeV); //.1*eV, 1*GeV);
}

MyPhysicsList::~MyPhysicsList()
{}

void MyPhysicsList::ConstructProcess()
{
    G4VModularPhysicsList::ConstructProcess();  // call base class to build processes

    // Remove electron ionization here
    auto particleIterator = GetParticleIterator();
    particleIterator->reset();
    while ((*particleIterator)()) 
    {
        G4ParticleDefinition* particle = particleIterator->value();
        if (particle->GetParticleName() == "e-") 
        {
            G4ProcessManager* pManager = particle->GetProcessManager();
            G4ProcessVector* processVector = pManager->GetProcessList();
            for (int i = 0; i < processVector->size(); ++i) 
            {
                G4VProcess* proc = (*processVector)[i];
                if (proc->GetProcessName() == "eIoni") 
                {
                    pManager->RemoveProcess(proc);
                    G4cout << "Removed ionization!!!!!!" << G4endl;
                    break;
                }
            }
        }
    }
}