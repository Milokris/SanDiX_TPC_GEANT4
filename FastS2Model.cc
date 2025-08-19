#include "FastS2Model.hh"

void LXeElectronDriftModel::DoIt(const G4FastTrack& fastTrack, G4FastStep& fastStep) {
    G4Track* track = const_cast<G4Track*>(fastTrack.GetPrimaryTrack());
    G4ThreeVector pos = track->GetPosition();
    G4ThreeVector dir = track->GetMomentumDirection();

    //G4cout << "Fast Sim model electron time: " << fastTrack.GetPrimaryTrack()->GetGlobalTime() << G4endl;

    //G4cout << "Old position: " << pos << G4endl;
    //G4cout << "Old Direction: " << dir << G4endl;
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
    // G4cout << "dirX: " << dirX << G4endl;
    // G4cout << "dirY: " << dirY << G4endl;
    // G4cout << "dirZ: " << dirZ << G4endl;    
    // double dirR = std::sqrt(dirX*dirX + dirY*dirY + dirZ*dirZ);
    // double unitDirX = dirX / dirR;
    // double unitDirY = dirY / dirR;
    // double unitDirZ = dirZ / dirR;
    // G4cout << "Unit DirX: " << unitDirX << G4endl;
    // G4cout << "Unit DirY: " << unitDirY << G4endl;
    // G4cout << "Unit DirZ: " << unitDirZ << G4endl;

    // Electric field
    double Efield = nestDetector->get_ElectricField(x, y, z);

    if (Efield <= 0.) {
        fastStep.KillPrimaryTrack();
        fastStep.ProposePrimaryTrackPathLength(0.0);        
        G4cout << "Killing drift electron outside electric field region" << G4endl;
        return;
    }
    //G4cout << "Electric Field: " << Efield << G4endl;
    // Check for S2
    if (CheckGainRegion(x, y, z, Efield)) 
    {
        //G4cout << "Starting S2 Process!!!" << G4endl;
        int nPhotons = photPerE;
        G4ParticleDefinition* photonDef = G4OpticalPhoton::OpticalPhotonDefinition();
        G4TrackVector* secondaries = new G4TrackVector();

        for (int i = 0; i < nPhotons; ++i) 
		{
            //G4cout << "Creating photons..." << G4endl;
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

            //G4cout << "Created photon time: " << fastTrack.GetPrimaryTrack()->GetGlobalTime() << G4endl;

            // // Create a fast-track secondary
            // G4Track* newTrack = new G4Track(dynPart, fastTrack.GetPrimaryTrack()->GetGlobalTime(),
            //                                 fastTrack.GetPrimaryTrack()->GetPosition());
            // newTrack->SetTouchableHandle(fastTrack.GetPrimaryTrack()->GetTouchableHandle());

            // // Push to fast sim secondaries
            // secondaries->push_back(newTrack);
            
            // G4Track* newTrack = fastTrack.CreateSecondary(dynPart, pos, track->GetGlobalTime());
            // newTrack->SetTouchableHandle(track->GetTouchableHandle());
            
			// G4Track* newTrack = new G4Track(dynPart, track->GetGlobalTime(), pos);
			// newTrack->SetTouchableHandle(track->GetTouchableHandle());
			// newTrack->SetParentID(track->GetTrackID());

			// secondaries->push_back(newTrack);

			// G4TrackVector* trackSecondaries = const_cast<G4Step*>(step)->GetfSecondary();
        	// trackSecondaries->insert(trackSecondaries->end(), secondaries->begin(), secondaries->end());
			// cout << "done creating secondaries" << G4endl;
    	
    	}
        nS2Events++;
        totalS2Photons += nPhotons;

        fastStep.KillPrimaryTrack();
        fastStep.ProposePrimaryTrackPathLength(0.0);
        //G4cout << "Killed primary after S2" << G4endl;
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

    double farStep = 5.*mm;
    double nearStep = 0.1*um;
    double stepLength = farStep;
    stepLength = 0.50*(r - gainArea);  //0.5*(dist - gainArea);
    if (stepLength <= nearStep)
    {
        stepLength = nearStep;
    }
    if (stepLength >= farStep)
    {
        stepLength = farStep;
    }
    // G4cout << "Step Length: " << stepLength << G4endl;
    // G4cout << "r = " << r << G4endl;
    // G4cout << "Electric Field: " << Efield << G4endl;
    // G4cout << "gainArea: " << gainArea << G4endl;

    double dt = stepLength / v_drift;

    //New WAY

    G4ThreeVector driftDir = dir.unit();
    G4ThreeVector v1 = driftDir.orthogonal().unit();
    G4ThreeVector v2 = driftDir.cross(v1).unit();

    double sigma_L = sqrt(D_L * dt);
    double sigma_T = sqrt(D_T * dt);

    //G4cout << "Sigma_L " << sigma_L << G4endl;
    //G4cout << "Sigma_T " << sigma_T << G4endl;

    G4ThreeVector diffusion = G4RandGauss::shoot(0., sigma_L) * driftDir + G4RandGauss::shoot(0., sigma_T) * v1 + G4RandGauss::shoot(0., sigma_T) * v2;
    G4ThreeVector newPos = pos + v_drift_dir*v_drift*dt + diffusion*dt;
    
    //G4cout << "Diffusion: " << diffusion*dt << G4endl;

    //G4cout << "dt = " << dt << " us" << G4endl;

    double xDiffusion = dirY*G4RandGauss::shoot(0., sqrt(D_T*dt)) + dirX*G4RandGauss::shoot(0., sqrt(D_L*dt));
    double xDrift = - unitX*v_drift*dt;
    double yDiffusion = dirX*G4RandGauss::shoot(0., sqrt(D_T*dt)) + dirY*G4RandGauss::shoot(0., sqrt(D_L*dt));
    double yDrift = - unitY*v_drift*dt;
    double zDiffusion = G4RandGauss::shoot(0., sqrt(D_T*dt)) + dirZ*G4RandGauss::shoot(0., sqrt(D_L*dt));
    //G4cout << "xDiffusion: " << xDiffusion << G4endl;
    //G4cout << "yDiffusion: " << yDiffusion << G4endl;
    // G4cout << "XDrift: " << xDrift << G4endl;
    // G4cout << "yDrift: " << yDrift << G4endl;
    //G4cout << "zDiffusion: " << zDiffusion << G4endl;
    // G4cout << "Drift direction: " << "( " << xDrift << ", " << yDrift << ")" << G4endl;
    // G4cout << "Drift speed: " << v_drift << G4endl;

    // Drift + diffusion
    // double newX = pos.x() + xDrift + xDiffusion;
    // double newY = pos.y() + yDrift + yDiffusion;         
    // double newZ = pos.z() + zDiffusion; //diffusion really big for some reason, divided by 10

    double newX = newPos.x();
    double newY = newPos.y();        
    double newZ = newPos.z();

    double newR = std::sqrt(newX*newX + newY*newY);
    if (newR >= 25. || newZ >= 59. || newZ <= -59.)
    {
        //G4cout << "New R: " << newR << G4endl;
        fastStep.KillPrimaryTrack();
        fastStep.ProposePrimaryTrackPathLength(0.0);        
    }

    //G4ThreeVector newPos(newX, newY, newZ);


    double displacementX = newX - pos.x();
    double displacementY = newY - pos.y();
    double displacementZ = newZ - pos.z();
    double newr = std::sqrt(displacementX*displacementX + displacementY*displacementY + displacementZ*displacementZ);
    double newDirX = displacementX / newr;
    double newDirY = displacementY / newr;
    double newDirZ = displacementZ / newr;
    G4ThreeVector newDir(newDirX, newDirY, newDirZ);
    //G4cout << "New Direction: " << newDir << G4endl;
    //G4cout << "New Position: " << newPos << G4endl;


    // Update track
    //G4cout << "Are we getting here?" << G4endl;
    fastStep.ProposePrimaryTrackFinalPosition(newPos);
    fastStep.ProposePrimaryTrackFinalMomentumDirection(newDir);
    fastStep.ProposePrimaryTrackFinalKineticEnergy(track->GetKineticEnergy()); 
    fastStep.ProposePrimaryTrackPathLength(stepLength);
    fastStep.ProposePrimaryTrackFinalTime(fastTrack.GetPrimaryTrack()->GetGlobalTime() + dt*microsecond);

    //fParticleChange.ProposeGlobalTime(fastTrack.GetPrimaryTrack()->GetGlobalTime() + dt*microsecond);
   

}
