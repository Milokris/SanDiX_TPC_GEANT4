#include "generator.hh"

MyPrimaryGenerator::MyPrimaryGenerator()
{
    fParticleGun = new G4ParticleGun(1);
        
    G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
    G4String particleName = "neutron";
    G4ParticleDefinition *particle = particleTable->FindParticle(particleName);
    fParticleGun->SetParticleDefinition(particle);

    fParticleGun->SetParticleEnergy(0.);  // clear old energy def
    G4ThreeVector pos(50*mm, 10.*mm, 0.);
    G4ThreeVector mom(-1., 0., 0.);
    fParticleGun->SetParticlePosition(pos);
    fParticleGun->SetParticleMomentumDirection(mom);

//Regular Particle Gun
    fParticleGun->SetParticleEnergy(10.*MeV);

//137 Cs Gamma Ray Source *make sure particle set to gamma*
    // fParticleGun->SetParticleEnergy(662*keV);



}

MyPrimaryGenerator::~MyPrimaryGenerator()
{
    delete fParticleGun;
}

void MyPrimaryGenerator::GeneratePrimaries(G4Event *anEvent)
{
    fParticleGun->GeneratePrimaryVertex(anEvent);
}