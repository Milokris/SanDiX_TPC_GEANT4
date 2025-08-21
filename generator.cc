#include "generator.hh"

int incidentRecoilType = 0; //0==ER , 1==NR

MyPrimaryGenerator::MyPrimaryGenerator()
{
    fParticleGun = new G4ParticleGun(1);
        
    G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
    G4String particleName = "gamma";
    G4ParticleDefinition *particle = particleTable->FindParticle(particleName);
    fParticleGun->SetParticleDefinition(particle);

    fParticleGun->SetParticleEnergy(0.);  // clear old energy def
    G4ThreeVector pos(50*mm, 10.*mm, 0.);
    G4ThreeVector mom(-1., 0., 0.);
    fParticleGun->SetParticlePosition(pos);
    fParticleGun->SetParticleMomentumDirection(mom);

    fParticleGun->SetParticleEnergy(.10*MeV);

}

MyPrimaryGenerator::~MyPrimaryGenerator()
{
    delete fParticleGun;
}

void MyPrimaryGenerator::GeneratePrimaries(G4Event *anEvent)
{
    fParticleGun->GeneratePrimaryVertex(anEvent);
}