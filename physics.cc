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

    auto* opticalParams = G4OpticalParameters::Instance();
    opticalParams->SetProcessActivation("Cerenkov", false);

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
        // G4ProcessManager* pManager = particle->GetProcessManager();
        // G4ProcessVector* processVector = pManager->GetProcessList();

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
                if (proc->GetProcessName() == "msc")
                {
                    pManager->RemoveProcess(proc);
                    G4cout << "Removed msc!!!!!!" << G4endl;
                    // // Construct and configure a new MSC process
                    // auto* msc = new G4eMultipleScattering();
                    // auto* model = new G4UrbanMscModel();  // Or try G4GoudsmitSaundersonMscModel()

                    // model->SetStepLimitType(fMinimal);
                    // //model->SetRangeFactor(0.01);
                    // //model->SetLateralDisplacement(true);
                    // model->SetGeomFactor(3.0);

                    // msc->AddEmModel(0, model);
                    // msc->SetStepLimitType(fMinimal);
                    // //msc->SetRangeFactor(0.01);

                    // // Add the new MSC back
                    // pManager->AddProcess(msc, -1, 1, 1);  // ordering: -1 init, 1 along step, 1 post step
                    // G4cout << "Added custom MSC!" << G4endl;
                    
                }
            }
        }


        // if (!pManager) continue;
        // for (int i = 0; i < processVector->size(); ++i) 
        // {
        //     G4VProcess* proc = (*processVector)[i];
        //     G4cout << "Particle: " << particle->GetParticleName() << "Process Name: " << proc->GetProcessName() << G4endl;

        //     if (proc->GetProcessName() == "Scintillation") 
        //     {
        //         pManager->RemoveProcess(proc);
        //         G4cout << "Removed scintillation from: " << particle->GetParticleName() << G4endl;
        //         break;
        //     }
        // }
    }
}