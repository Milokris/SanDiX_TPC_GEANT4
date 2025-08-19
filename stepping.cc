#include "stepping.hh"


MySteppingAction::MySteppingAction() //MyEventAction *eventAction)
{
	//fEventAction = eventAction;
}

MySteppingAction::~MySteppingAction()
{}

// namespace 
// {
//     G4ThreeVector RandomUnitVector() 
// 	{
//         double costheta = 2.*G4UniformRand() - 1.;
//         double sintheta = std::sqrt(1. - costheta*costheta);
//         double phi = 2.*CLHEP::pi*G4UniformRand();
//         return G4ThreeVector(sintheta*cos(phi), sintheta*sin(phi), costheta);
//     }
// }

// namespace
// {
// 	G4double SampleLXePhotonEnergy_GaussEnergy() 
// 	{
//   		const double mean_eV  = 1239.8419843320026/178.0; // ~6.966 eV
//   		const double sigma_eV = 0.20;                     // from conversion above
//   		double E_eV = CLHEP::RandGauss::shoot(mean_eV, sigma_eV);
//   		if (E_eV < 6.5) E_eV = 6.5;                       // trim tails to your grid
//   		if (E_eV > 8.2) E_eV = 8.2;
//   		return E_eV*eV;
// 	}

// }

void MySteppingAction::ClearStagnationData(G4int trackID)
{
	previousEnergy.erase(trackID);
	stagnationCounter.erase(trackID);
}

int createdElectrons = 0;
int nS1Events = 0;
int nS2Events = 0;
int totalS1Photons = 0;
int totalS2Photons = 0;
double ElThreshold = 412000.0;    //412000.0; //Literature says proportional Electroluminescense starts at 412 kV/cm
double gainArea = voltage / ((ElThreshold) * std::log(b/a)); //   0.015; //in mm so ~15 microns
G4double lastStep = 10000.*mm;
int photPerE = 30;
G4ThreadLocal std::unordered_set<G4int> DriftTrackIDs;


void MySteppingAction::UserSteppingAction(const G4Step *step)
{		

	//G4cout << "Stepping gainArea: " << gainArea << G4endl;
	
	// Prevent runaway memory usage
	const size_t maxMapSize = 10000;
	if (previousEnergy.size() > maxMapSize) 
	{
		G4cout << "Clearing previousEnergy map!" << G4endl;
		previousEnergy.clear();
		stagnationCounter.clear();
	}

	//G4cout << "Gain area is: " << gainArea << G4endl;


	G4LogicalVolume *volume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume();

    // Get position of interaction
    G4ThreeVector pos = step->GetPreStepPoint()->GetPosition();
	G4ThreeVector dir = step->GetPreStepPoint()->GetMomentumDirection();

	//double r = std::sqrt(pos.x()/mm * pos.x()/mm + pos.y()/mm * pos.y()/mm);
	const MyDetectorConstruction *detectorConstruction = static_cast<const MyDetectorConstruction*> (G4RunManager::GetRunManager()->GetUserDetectorConstruction());
	
	G4LogicalVolume *fScoringVolume = detectorConstruction->GetScoringVolume();

	G4Track *track = step->GetTrack();
	G4int id = track->GetTrackID();
	G4double energy = track->GetKineticEnergy();
	const G4ParticleDefinition* pd = track->GetParticleDefinition();
	auto incidentParticle = pd->GetParticleName();
	G4double edep = step->GetTotalEnergyDeposit() / keV;
	static std::map<G4int, G4double> previousEnergy;
	static std::map<G4int, int> stagnationCounter;
	const int maxSteps = 1000;
	const double tol = 0.1 * eV;
	bool S2Event = false;
	bool driftElectron = false;
	auto info = dynamic_cast<DriftElectronInfo*>(track->GetUserInformation());
    double Efield = nestDetector->get_ElectricField(pos.x() / cm, pos.y() / cm, pos.z() / cm);
	double x = pos.x() / cm;
	double y = pos.y() / cm;
	double z = pos.z() / cm;
	double r_unit = std::sqrt(x*x + y*y + z*z);
	double x_unit = (r_unit > 0 ? x / r_unit : 0.);
	double y_unit = (r_unit > 0 ? y / r_unit : 0.);
	double z_unit = (r_unit > 0 ? z / r_unit : 0.);
	double r_E = std::sqrt(x*x + y*y);
	double x_E = (r_E > 0 ? x / r_E : 0.);
    double y_E = (r_E > 0 ? y / r_E : 0.);
	G4double stepLength = step->GetStepLength();
	G4double nPhotons = 0;
	G4double nElectrons = 0;
	double density = 3.0558; //3.0558*g/cm3
	double temp = 162; //162 Kelvin
	double pressure = 1; //1 Bar (atmospheric pressure)
	static nestPart detector;
	static NEST::NESTcalc nestCalc(&detector);
	G4int parentID = track->GetParentID();
    G4int trackID = track->GetTrackID();
	
	// if (info && info->IsDrift())
	// {
	// 	G4cout << "track id: " << trackID << " Is drift electron!!!" << G4endl;
	// }
	// else
	// {
	// 	G4cout << "track id: " << trackID << G4endl;
	// }
	
	// G4cout << "parent id: " << parentID << G4endl;

    // If parent is in the drift list, this is a descendant
    if (DriftTrackIDs.count(parentID))
    {
        // add this track to the list for future checks
        DriftTrackIDs.insert(trackID);

        // kill it
        track->SetTrackStatus(fStopAndKill);
        G4cout << "Killed track " << trackID << " from drift parent " << parentID << G4endl;
        return;
    }


    if (info && info->IsDrift()) 
	{
        if (volume && volume->GetName() != "logicChamber")
		{
			track->SetTrackStatus(fStopAndKill);
		}
		return;
	
		driftElectron = true;
		//track->SetKineticEnergy(energy / 1.25);
		track->SetKineticEnergy(0.1*eV);


		//NEW DRIFT MECHANICS!!!!

		// bool highField = (Efield >= 5000.); 
		// double v_drift = nestCalc.SetDriftVelocity(temp, density, Efield, pressure); // mm/us
		// double D_T = nestCalc.GetDiffTran_Liquid(Efield, highField, temp, pressure, density, 54); //mm2/us
		// double D_L = nestCalc.GetDiffLong_Liquid(Efield, highField, temp, pressure, density, 54, 0); //mm2/us
		// if (v_drift ==0)
		// {
		// 	G4cout << "v_drift = 0" << G4endl;
		// 	return;
		// }
		// double dt = stepLength / v_drift;

		// // G4cout << "Electric Field strength is: " << Efield << " V/cm" << G4endl;
		// // G4cout << "Drift velocity is: " << v_drift << " mm / us" << G4endl;
		// // G4cout << "Diffusion Transverse: " << D_T << " mm2 / us" << G4endl;
		// // G4cout << "Diffusion Longitudinal: " << D_L << " mm2 / us" << G4endl;

		// // propagate electron for timestep dt
		// double newX = x*10 + dir.y()*RandomGaussian(0, sqrt(D_T * dt)) + dir.x()*RandomGaussian(0, sqrt(D_L * dt)) + x_unit*v_drift*dt;
		// double newY = y*10 + dir.x()*RandomGaussian(0,sqrt(D_T * dt)) + dir.y()*RandomGaussian(0, sqrt(D_L * dt)) + y_unit*v_drift*dt;
		// double newZ = z*10 + RandomGaussian(0,sqrt(D_T * dt)) + dir.z()*RandomGaussian(0, sqrt(D_L * dt));



		//posNew = ()
		

    }
	NEST::INTERACTION_TYPE recoilType = NEST::NoneType;

	if (driftElectron && stepLength > lastStep)
	{
		//G4cout << "Passed it!!!!!!" << G4endl;
	}
	lastStep = stepLength;
// ===================== Monty Carlo thermal ionization for low-energy electrons ========================

	if (pd == G4Electron::ElectronDefinition()) // && energy < 10.1*eV)
	{
    // Estimate probability of ionization 
		double P_ionize = 0;
		if (Efield > ElThreshold)
		{
			P_ionize = 1.;
		}

    	//double P_ionize = 0.0001;
		//G4cout << "step length: " << stepLength / mm << " mm " << "at R = " << r <<  " With Electric field: " << Efield << G4endl;
		
		// if (r < 1)
		// {
		// 	track->SetTrackStatus(fStopAndKill);
		// 	G4cout << "Electron reached S2 zone!" << G4endl;
		// 	ClearStagnationData(id);
		// 	return;

		// }
    	if (G4UniformRand() < P_ionize)
    	{

        	// track->SetTrackStatus(fStopAndKill);
        	// G4cout << "Thermal e- ionized by MC model!!!" << G4endl;
			// ClearStagnationData(id);	
        	// return;

        	// auto touchable = step->GetPreStepPoint()->GetTouchable();

        	// MyHit* hit = new MyHit();
        	// hit->SetEdep(energy);
        	// hit->SetPos(pos);
        	// hit->SetVolume(touchable->GetVolume());
        	// fHitsCollection->insert(hit);
			edep = energy / keV;

        	track->SetTrackStatus(fStopAndKill);
        	track->SetKineticEnergy(0.0);

        	//G4cout << "Thermal e- ionized by MC model! Deposited " << edep *1000 << " eV" << G4endl;
			ClearStagnationData(id);	
        	// return;
    	}
		else
		{
    		if(previousEnergy.count(id))
    		{
				//G4cout << "Starting process..." << G4endl;
    			G4double prevE = previousEnergy[id];        
        		if(std::abs(energy - prevE) < tol)
        		{
					//G4cout << "energy loss < total" << G4endl;
            		stagnationCounter[id]++;
        		}
        		else
        		{
					//G4cout << "energy loss > total" << G4endl;
            		stagnationCounter[id] = 0;
        		}
        
        		if(stagnationCounter[id] >= maxSteps)
        		{
            		track->SetTrackStatus(fStopAndKill);
					//G4cout << "killing: " << id << G4endl;
					ClearStagnationData(id);	
        			return;
       			}
    		}
    		else
    		{
				//G4cout << "Did not start process" << G4endl;
        		stagnationCounter[id] = 0;
    		}
			previousEnergy[id] = energy;

		}

	}
	else
	{
    	if(previousEnergy.count(id))
    	{
			//G4cout << "Starting process... (2.0)" << G4endl;
    		G4double prevE = previousEnergy[id];        
        	if(std::abs(energy - prevE) < tol)
        	{
				//G4cout << "energy loss < total (2.0)" << G4endl;
           		stagnationCounter[id]++;
        	}
        	else
        	{
				//G4cout << "energy loss > total (2.0)" << G4endl;
           		stagnationCounter[id] = 0;
        	}
        
        	if(stagnationCounter[id] >= maxSteps)
        	{
            	track->SetTrackStatus(fStopAndKill);
				//G4cout << "killing: (2.0) " << pd << G4endl;
				ClearStagnationData(id);	
        		return;
       		}
    	}
    	else
    	{
			//G4cout << "Did not start process (2.0)" << G4endl;
       		stagnationCounter[id] = 0;
    	}
		previousEnergy[id] = energy;		
	}

	if (pd == G4Neutron::NeutronDefinition() || pd == G4Proton::ProtonDefinition() || pd->GetParticleType() == "nucleus") 
	{
 		recoilType = NEST::INTERACTION_TYPE::NR; // Nuclear Recoil
	}
	else if (pd == G4Alpha::AlphaDefinition() || pd->GetParticleName() == "ion")
	{
		recoilType = NEST::INTERACTION_TYPE::ion;
	}
	else if (pd == G4Electron::ElectronDefinition() || pd == G4Positron::PositronDefinition() ||
			pd->GetParticleType() == "muon" || pd->GetParticleType() == "tau")
	{
		//something like get parent ID and then get parent ID of parent ID to make sure no drift electron? but put at the top and kill--
		//also maybe set energy back to 0.1 eV to get rid of edep stuff messing with it/ or get energy based on velocity?
		recoilType = NEST::INTERACTION_TYPE::beta;
	}
	else if (pd == G4Gamma::GammaDefinition())
	{
		recoilType = NEST::INTERACTION_TYPE::gammaRay;
		return; //Just to see if i can get better S1 and S2 waveforms with no gamma scintillations
	}
	else if (pd == G4OpticalPhoton::OpticalPhotonDefinition())
	{
		return;
		recoilType = NEST::INTERACTION_TYPE::gammaRay;
	}


	//Nest Part

  	if (volume && volume->GetName() == "logicChamber")
	{
		if (driftElectron)
		{
			if (edep > 0)
			{
				if (Efield > ElThreshold)
				{
					nPhotons = photPerE;
					nElectrons = 0;
					S2Event = true;
					//G4cout << "--------------initializing S2 event--------------" << G4endl;
					//G4cout << "Electric Field is: " << Efield << G4endl;
				}
				else 
				{
					//G4cout << "Drift electron is depositing energy outside gain area with edep: " << edep << G4endl;
					return;
				}
			}
			else return;
		}

		if (edep <= 10.*pow(10,-2) && !driftElectron) //14*10e-3.  14 eV needed for scintillation in liquid xenon   been using 12.*10e-2
		{
			return; //

			if (edep > 0)
			{
				
				// track->SetTrackStatus(fStopAndKill);
				// ClearStagnationData(id);
				// G4cout << "Killing because of weird edep code????" << G4endl;

				const G4VProcess* proc = step->GetPostStepPoint()->GetProcessDefinedStep();
    			if (proc) 
				{
        			G4String procName = proc->GetProcessName();
        			//G4cout << "Previous process: " << procName << G4endl;
    			} 
				else 
				{
					//G4cout << "No process defined this step" << G4endl;
				}
			}


			return;
		}


		if(!S2Event)
		{
			// Create NEST object
			G4cout << "~~~~~~~~~~~~~~~initializing S1 event~~~~~~~~~~~~~~~" << G4endl;

			NEST::YieldResult yields = nestCalc.GetYields(recoilType, edep, density, Efield,          // drift field
			131.293,                     // A
			54                           // Z
			);
			nPhotons = std::round(yields.PhotonYield);
			nElectrons = std::round(yields.ElectronYield);
		}



		//G4cout << "r value: " << r << " Is drift electron? " << driftElectron << G4endl;


		if (!S2Event)
		{
			G4cout << "position: " << pos << G4endl;
			G4cout << "Efield: " << Efield << G4endl;
			G4cout << "edep (keV): " << edep << G4endl;
			G4cout << "recoil type: " << recoilType << G4endl;
			G4cout << "Photon Yield: " << nPhotons << G4endl;
			G4cout << "Electron Yield: " << nElectrons << G4endl;
			G4cout << "Incident Particle: " << incidentParticle << G4endl;
			G4cout << "   " << G4endl;
		}



		G4ParticleDefinition* photonDef = G4OpticalPhoton::OpticalPhotonDefinition();
    	G4ParticleDefinition* eDef = G4Electron::ElectronDefinition(); 
			
		G4TrackVector* secondaries = new G4TrackVector();


    // Spawn optical photons
    	for (int i = 0; i < nPhotons; ++i) 
		{

				G4ThreeVector dir = RandomUnitVector();
				G4double photonEnergy = SampleLXePhotonEnergy_GaussEnergy();  

				G4DynamicParticle* dynPart = new G4DynamicParticle(photonDef, dir, photonEnergy);
				G4ThreeVector perp = dir.orthogonal();
				G4double phi = CLHEP::twopi * G4UniformRand();
				G4ThreeVector pol = perp.rotate(dir, phi).unit();
				dynPart->SetPolarization(pol);

				G4Track* newTrack = new G4Track(dynPart, track->GetGlobalTime(), pos);
				newTrack->SetTouchableHandle(track->GetTouchableHandle());
				newTrack->SetParentID(track->GetTrackID());

				secondaries->push_back(newTrack);

    	}

    //Spawn electrons
    	for (int i = 0; i < nElectrons; ++i) 
		{

        	G4ThreeVector dir = RandomUnitVector();
        	G4double electronEnergy = 0.1*eV;

        	G4DynamicParticle* dynPart = new G4DynamicParticle(eDef, dir, electronEnergy);
        	G4Track* newTrack = new G4Track(dynPart, track->GetGlobalTime(), pos);
			newTrack->SetUserInformation(new DriftElectronInfo(true, true));
        	newTrack->SetTouchableHandle(track->GetTouchableHandle());
        	newTrack->SetParentID(track->GetTrackID());
			//G4cout << "Created electron time: " << track->GetGlobalTime() << G4endl;
			DriftTrackIDs.insert(newTrack->GetTrackID());

        	secondaries->push_back(newTrack);

			createdElectrons = createdElectrons + 1;
			//cout << "------------------------Number of Created Electrons: " << createdElectrons << G4endl;

    	}

    	if (!secondaries->empty()) 
		{
			G4TrackVector* trackSecondaries = const_cast<G4Step*>(step)->GetfSecondary();
        	trackSecondaries->insert(trackSecondaries->end(), secondaries->begin(), secondaries->end());
			//cout << "done creating secondaries" << G4endl;
    	}

		if(S2Event)
		{
			nS2Events = nS2Events +1;
			totalS2Photons = totalS2Photons + nPhotons;
		}
		else
		{
			nS1Events = nS1Events +1;
			totalS1Photons = totalS1Photons + nPhotons;
		}
		// G4cout << "Number of S1 events: " << nS1Events << G4endl;
		// G4cout << "Number of S2 events: " << nS2Events << G4endl;
		//G4cout << "   " << G4endl;
		//G4cout << "   " << G4endl;

		// if (energy < 0.9e-6 *MeV)
		// {
		// 	track->SetTrackStatus(fStopAndKill);
		// 	ClearStagnationData(id);
		// 	G4cout << "Killing track ID " << track->GetTrackID() << G4endl;
		// }

		if(S2Event)
		{
			track->SetTrackStatus(fStopAndKill);
			ClearStagnationData(id);
		}
    	delete secondaries;

    }

	

	
	
	previousEnergy[id] = energy;
	
	// if(volume != fScoringVolume) {
	// 	//G4cout << volume->GetName() << G4endl;
	// 	//G4cout << fScoringVolume->GetName() << G4endl;
	// 	return;
	// }
	
	//fEventAction->AddEdep(edep);
}


