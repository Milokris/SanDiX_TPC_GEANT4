#include "generator.hh"

MyPrimaryGenerator::MyPrimaryGenerator()
{
    fParticleSource = new G4GeneralParticleSource();


    // fParticleGun = new G4ParticleGun(1);

    // G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
    // G4String particleName = "neutron"; //gamma or neutron or e-
    // G4ParticleDefinition *particle = particleTable->FindParticle(particleName);
    // fParticleGun->SetParticleDefinition(particle);

    // fParticleGun->SetParticleEnergy(0.);  // clear old energy def
    // G4ThreeVector pos(50*mm, 1.*mm, 0.); //50*mm, 10.*mm, 0.
    // G4ThreeVector mom(-1., 0., 0.);
    // fParticleGun->SetParticlePosition(pos);
    // fParticleGun->SetParticleMomentumDirection(mom);

    // fParticleGun->SetParticleEnergy(2.45*MeV); //137Cs source emits 662 keV gamma. D-D neutron source emits 2.45 MeV
    // //to produce golden parameter graph, use gammas ranging between 0.001 - 0.01 MeV


}

MyPrimaryGenerator::~MyPrimaryGenerator()
{
    delete fParticleSource;

    // delete fParticleGun;
}

void MyPrimaryGenerator::GeneratePrimaries(G4Event *anEvent)
{
    fParticleSource->GeneratePrimaryVertex(anEvent);


    //fParticleGun->GeneratePrimaryVertex(anEvent);
}