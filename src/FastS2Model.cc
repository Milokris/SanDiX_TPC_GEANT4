#include "FastS2Model.hh"

void LXeElectronDriftModel::DoIt(const G4FastTrack& fastTrack, G4FastStep& fastStep) {
    G4Track* track = const_cast<G4Track*>(fastTrack.GetPrimaryTrack());
    G4ThreeVector pos = track->GetPosition();
    static nestPart detector;
	static NEST::NESTcalc nestCalc(&detector);

    const double x = pos.x()/cm;
    const double y = pos.y()/cm;
    const double z = pos.z()/cm;
    const G4double radius = std::hypot(pos.x(), pos.y());

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
        MyEventAction* eventAction = (MyEventAction*)
            G4RunManager::GetRunManager()->GetUserEventAction();
        int nPhotons = eventAction->photPerE;  //According to SanDiX paper this is ~17.

        G4ParticleDefinition* photonDef = G4OpticalPhoton::OpticalPhotonDefinition();

        for (int i = 0; i < nPhotons; ++i) //divide by 10 to reduce file size for VR
		{
			G4ThreeVector randDir = RandomUnitVector();
			G4double photonEnergy = SampleLXePhotonEnergy_GaussEnergy();

				G4ThreeVector pol = randDir.orthogonal().unit();
				pol.rotate(randDir, CLHEP::twopi * G4UniformRand());

            G4DynamicParticle dynPhoton(photonDef, randDir, photonEnergy);
            fastStep.CreateSecondaryTrack(dynPhoton, pol, fastTrack.GetPrimaryTrack()->GetPosition(),
                fastTrack.GetPrimaryTrack()->GetGlobalTime(), false);

    	}
        eventAction->AddS2Photons(nPhotons);


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



    if (!(v_drift > 0.) || !std::isfinite(v_drift)) {
        fastStep.KillPrimaryTrack();
        fastStep.ProposePrimaryTrackPathLength(0.0);
        G4cout << "Killing drift electron because drift velocity is non-positive or non-finite" << G4endl;
        return;
    }

        G4double farStep = 5.*mm; //was 5.*mm
        G4double nearStep = 0.1*um; //was .1*um
        G4double stepLength = 0.5*(radius - gainArea);
        if (stepLength <= nearStep)
        {
            stepLength = nearStep;
        }
        if (stepLength >= farStep)
        {
            stepLength = farStep;
        }


    const double dt_us = (stepLength/mm) / v_drift;

    G4ThreeVector driftDir = v_drift_dir;
    G4ThreeVector v1 = driftDir.orthogonal().unit();
    G4ThreeVector v2 = driftDir.cross(v1).unit();

    const G4double diffusionL = G4RandGauss::shoot(0., std::sqrt(D_L * dt_us))*mm;
    const G4double diffusionT1 = G4RandGauss::shoot(0., std::sqrt(D_T * dt_us))*mm;
    const G4double diffusionT2 = G4RandGauss::shoot(0., std::sqrt(D_T * dt_us))*mm;
    const G4ThreeVector diffusion = diffusionL*driftDir + diffusionT1*v1 + diffusionT2*v2;
    const G4ThreeVector newPos = pos + v_drift_dir*(v_drift*dt_us)*mm + diffusion;

    double newX = newPos.x();
    double newY = newPos.y();
    double newZ = newPos.z();

    double newR = std::sqrt(newX*newX + newY*newY);
    if (newR >= 25.*mm || newZ >= 59.*mm || newZ <= -59.*mm)
    {
        fastStep.KillPrimaryTrack();
        fastStep.ProposePrimaryTrackPathLength(0.0);
        return;
    }

    const G4ThreeVector displacement = newPos - pos;
    const G4ThreeVector newDir = displacement.mag2() > 0. ? displacement.unit() : v_drift_dir;



    // Update track

    fastStep.ProposePrimaryTrackFinalPosition(newPos);
    fastStep.ProposePrimaryTrackFinalMomentumDirection(newDir);
    fastStep.ProposePrimaryTrackFinalKineticEnergy(track->GetKineticEnergy());
    fastStep.ProposePrimaryTrackPathLength(stepLength);
    fastStep.ProposePrimaryTrackFinalTime(fastTrack.GetPrimaryTrack()->GetGlobalTime() + dt_us*microsecond);
}
