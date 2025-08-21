#include "physics.hh"


MyPhysicsList::MyPhysicsList()
{
    RegisterPhysics(new G4EmLowEPPhysics());

    RegisterPhysics(new G4HadronPhysicsQGSP_BERT());
    RegisterPhysics(new G4DecayPhysics());
    RegisterPhysics(new G4StoppingPhysics());
    RegisterPhysics(new G4StepLimiterPhysics());
    RegisterPhysics(new G4OpticalPhysics());

    auto* opticalParams = G4OpticalParameters::Instance();
    opticalParams->SetProcessActivation("Cerenkov", false);

    G4ProductionCutsTable::GetProductionCutsTable()->SetEnergyRange(1.*eV, 1.*GeV); //.1*eV - 1.*GeV
}

MyPhysicsList::~MyPhysicsList()
{}

void MyPhysicsList::ConstructProcess()
{
    G4VModularPhysicsList::ConstructProcess(); 

    // Remove electron ionization and msc here, having them on slows it down dramatically
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
                    //pManager->RemoveProcess(proc);
                    //G4cout << "Removed ionization!!!!!!" << G4endl;
                    break;
                }
                if (proc->GetProcessName() == "msc")
                {
                    //pManager->RemoveProcess(proc);
                    //G4cout << "Removed msc!!!!!!" << G4endl;
                }
            }
            auto* fastSimProc = new G4FastSimulationManagerProcess("fastSimProc");
            pManager->AddProcess(fastSimProc, -1, -1, 1);
            G4cout << "Added FastSim process to electron" << G4endl;
        }
    }
}