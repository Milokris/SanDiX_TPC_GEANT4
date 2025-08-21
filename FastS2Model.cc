#include "FastS2Model.hh"

void LXeElectronDriftModel::DoIt(const G4FastTrack& fastTrack, G4FastStep& fastStep) {
    G4Track* track = const_cast<G4Track*>(fastTrack.GetPrimaryTrack());
    G4ThreeVector pos = track->GetPosition();
    G4ThreeVector dir = track->GetMomentumDirection();

    static nestPart detector;
	static NEST::NESTcalc nestCalc(&detector);

    double x = pos.x()/cm;
    double y = pos.y()/cm;
    double z = pos.z()/cm;
    double r = std::sqrt(x*x + y*y);
    double unitX = x / r;
    double unitY = y / r;

    double dirX = dir.x();
    double dirY = dir.y();
    double dirZ = dir.z();

    // Electric field
    double Efield = nestDetector->get_ElectricField(x, y, z);

    if (Efield <= 0.) {
        fastStep.KillPrimaryTrack();
        fastStep.ProposePrimaryTrackPathLength(0.0);        
        G4cout << "Killing drift electron outside electric field region" << G4endl;
        return;
    }
    // Check for S2
    if (CheckGainRegion(x, y, z, Efield)) 
    {
        int nPhotons = photPerE; //I don't have the real number for this, right now I'm using 30.
        G4ParticleDefinition* photonDef = G4OpticalPhoton::OpticalPhotonDefinition();
        G4TrackVector* secondaries = new G4TrackVector();

        for (int i = 0; i < nPhotons; ++i) 
		{
			G4ThreeVector randDir = RandomUnitVector();
			G4double photonEnergy = SampleLXePhotonEnergy_GaussEnergy();  

			G4DynamicParticle* dynPart = new G4DynamicParticle(photonDef, randDir, photonEnergy);
			G4ThreeVector perp = randDir.orthogonal();
			G4double phi = CLHEP::twopi * G4UniformRand();
			G4ThreeVector pol = perp.rotate(dir, phi).unit();
			dynPart->SetPolarization(pol);

            G4DynamicParticle dynPhoton(photonDef, randDir, photonEnergy);
            fastStep.CreateSecondaryTrack(dynPhoton, pol, fastTrack.GetPrimaryTrack()->GetPosition(), 
                fastTrack.GetPrimaryTrack()->GetGlobalTime(), false);
    	
    	}
        nS2Events++;
        totalS2Photons += nPhotons;

        fastStep.KillPrimaryTrack();
        fastStep.ProposePrimaryTrackPathLength(0.0);
        return;
    }

    double temp = 162.;
    double pressure = 1.;
    double density = 3.0558;
    bool highField = (Efield >= 5000.);

    double v_drift = nestCalc.SetDriftVelocity(temp, density, Efield, pressure); // mm/us
    G4ThreeVector v_drift_dir (-pos.x(), -pos.y(), 0);
    v_drift_dir = v_drift_dir.unit();
    double D_T = 0.;
    double D_L = 0.;

    if (Efield < 20000)
    {
	    D_T = nestCalc.GetDiffTran_Liquid(Efield, highField, temp, pressure, density, 54) * pow(10,-4); // converts from cm2/s to mm2/us
	    D_L = nestCalc.GetDiffLong_Liquid(Efield, highField, temp, pressure, density, 54, 0) * pow(10, -4); // converts from cm2/s mm2/us
    }



    if (v_drift == 0.) {
        fastStep.KillPrimaryTrack();
        fastStep.ProposePrimaryTrackPathLength(0.0);
        G4cout << "Killing drift electron because drift velocity is 0" << G4endl;
        return;
    }

    double farStep = 5.*mm; //was 5.*mm
    double nearStep = 0.1*um; //was .1*um
    double stepLength = farStep;
    stepLength = 0.5*(r - gainArea); 
    if (stepLength <= nearStep)
    {
        stepLength = nearStep;
    }
    if (stepLength >= farStep)
    {
        stepLength = farStep;
    }


    double dt = stepLength / v_drift;

    G4ThreeVector driftDir = dir.unit();
    G4ThreeVector v1 = driftDir.orthogonal().unit();
    G4ThreeVector v2 = driftDir.cross(v1).unit();

    double sigma_L = sqrt(D_L * dt);
    double sigma_T = sqrt(D_T * dt);


    G4ThreeVector diffusion = G4RandGauss::shoot(0., sigma_L) * driftDir + G4RandGauss::shoot(0., sigma_T) * v1 + G4RandGauss::shoot(0., sigma_T) * v2;
    G4ThreeVector newPos = pos + v_drift_dir*v_drift*dt + diffusion*dt;

    double newX = newPos.x();
    double newY = newPos.y();        
    double newZ = newPos.z();

    double newR = std::sqrt(newX*newX + newY*newY);
    if (newR >= 25. || newZ >= 59. || newZ <= -59.)
    {
        fastStep.KillPrimaryTrack();
        fastStep.ProposePrimaryTrackPathLength(0.0);        
    }


    double displacementX = newX - pos.x();
    double displacementY = newY - pos.y();
    double displacementZ = newZ - pos.z();
    double newr = std::sqrt(displacementX*displacementX + displacementY*displacementY + displacementZ*displacementZ);
    double newDirX = displacementX / newr;
    double newDirY = displacementY / newr;
    double newDirZ = displacementZ / newr;

    G4ThreeVector newDir(newDirX, newDirY, newDirZ);



    // Update track

    fastStep.ProposePrimaryTrackFinalPosition(newPos);
    fastStep.ProposePrimaryTrackFinalMomentumDirection(newDir);
    fastStep.ProposePrimaryTrackFinalKineticEnergy(track->GetKineticEnergy()); 
    fastStep.ProposePrimaryTrackPathLength(stepLength);
    fastStep.ProposePrimaryTrackFinalTime(fastTrack.GetPrimaryTrack()->GetGlobalTime() + dt*microsecond);   
}
