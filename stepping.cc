#include "stepping.hh"

MySteppingAction::MySteppingAction(MyEventAction *eventAction)
{
	fEventAction = eventAction;
}

MySteppingAction::~MySteppingAction()
{}

namespace 
{
    G4ThreeVector RandomUnitVector() {
        double costheta = 2.*G4UniformRand() - 1.;
        double sintheta = std::sqrt(1. - costheta*costheta);
        double phi = 2.*CLHEP::pi*G4UniformRand();
        return G4ThreeVector(sintheta*cos(phi), sintheta*sin(phi), costheta);
    }
}

void MySteppingAction::ClearStagnationData(G4int trackID)
{
	previousEnergy.erase(trackID);
	stagnationCounter.erase(trackID);
}

void MySteppingAction::UserSteppingAction(const G4Step *step)
{		
	// Prevent runaway memory usage
	const size_t maxMapSize = 10000;
	if (previousEnergy.size() > maxMapSize) 
	{
		G4cout << "Clearing previousEnergy map!" << G4endl;
		previousEnergy.clear();
		stagnationCounter.clear();
	}

	
	G4LogicalVolume *volume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume();

	
	const MyDetectorConstruction *detectorConstruction = static_cast<const MyDetectorConstruction*> (G4RunManager::GetRunManager()->GetUserDetectorConstruction());
	
	G4LogicalVolume *fScoringVolume = detectorConstruction->GetScoringVolume();

	G4Track *track = step->GetTrack();
	G4int id = track->GetTrackID();
	G4double energy = track->GetKineticEnergy();
	const G4ParticleDefinition* pd = track->GetParticleDefinition();
	G4double edep = step->GetTotalEnergyDeposit() / keV;
	static std::map<G4int, G4double> previousEnergy;
	static std::map<G4int, int> stagnationCounter;
	const int maxSteps = 100;
	const double tol = 0.01 * eV;
	NEST::INTERACTION_TYPE recoilType = NEST::NoneType;

// ===================== Monty Carlo thermal ionization for low-energy electrons ========================

	if (pd == G4Electron::ElectronDefinition() && energy < 10.1*eV)
	{
    // Estimate probability of ionization 
    	double P_ionize = 0.5;

    	if (G4UniformRand() < P_ionize)
    	{

        	track->SetTrackStatus(fStopAndKill);
        	G4cout << "Thermal e- ionized by MC model!!!" << G4endl;
			ClearStagnationData(id);	
        	return;
    	}
		else
		{
    		if(previousEnergy.count(id))
    		{
				G4cout << "Starting process..." << G4endl;
    			G4double prevE = previousEnergy[id];        
        		if(std::abs(energy - prevE) < tol)
        		{
					G4cout << "energy loss < total" << G4endl;
            		stagnationCounter[id]++;
        		}
        		else
        		{
					G4cout << "energy loss > total" << G4endl;
            		stagnationCounter[id] = 0;
        		}
        
        		if(stagnationCounter[id] >= maxSteps)
        		{
            		track->SetTrackStatus(fStopAndKill);
					G4cout << "killing: " << pd << G4endl;
					ClearStagnationData(id);	
        			return;
       			}
    		}
    		else
    		{
				G4cout << "Did not start process" << G4endl;
        		stagnationCounter[id] = 0;
    		}
			previousEnergy[id] = energy;

		}

	}
	else
	{
    	if(previousEnergy.count(id))
    	{
			G4cout << "Starting process... (2.0)" << G4endl;
    		G4double prevE = previousEnergy[id];        
        	if(std::abs(energy - prevE) < tol)
        	{
				G4cout << "energy loss < total (2.0)" << G4endl;
           		stagnationCounter[id]++;
        	}
        	else
        	{
				G4cout << "energy loss > total (2.0)" << G4endl;
           		stagnationCounter[id] = 0;
        	}
        
        	if(stagnationCounter[id] >= maxSteps)
        	{
            	track->SetTrackStatus(fStopAndKill);
				G4cout << "killing: (2.0) " << pd << G4endl;
				ClearStagnationData(id);	
        		return;
       		}
    	}
    	else
    	{
			G4cout << "Did not start process (2.0)" << G4endl;
       		stagnationCounter[id] = 0;
    	}
		previousEnergy[id] = energy;		
	}


	if (pd == G4Neutron::NeutronDefinition()) 
	{
 		recoilType = NEST::INTERACTION_TYPE::NR; // Nuclear Recoil
	}
	if (pd == G4Electron::ElectronDefinition())
	{
		recoilType = NEST::INTERACTION_TYPE::beta;
	}
	if (pd == G4OpticalPhoton::OpticalPhotonDefinition())
	{
		recoilType = NEST::INTERACTION_TYPE::gammaRay;
	}
	if (pd == G4Gamma::GammaDefinition())
	{
		recoilType = NEST::INTERACTION_TYPE::gammaRay;
	}

	//Nest Part
  	if (volume && volume->GetName() == "logicChamber")
	{
		if (edep <= 10e-2) 
		{
			if (edep > 0)
			{
				track->SetTrackStatus(fStopAndKill);
				ClearStagnationData(id);
			}
			return;
		}

        // Get position of interaction
        G4ThreeVector pos = step->GetPreStepPoint()->GetPosition();

        // Get electric field at this position from nestDetector
        double Efield = nestDetector->get_ElectricField(pos.x() / cm, pos.y() / cm, pos.z() / cm);

        // Create NEST object
   		static nestPart detector;
		static NEST::NESTcalc nestCalc(&detector);

   		NEST::YieldResult yields = nestCalc.GetYields(recoilType, edep, 3.0558, Efield,          // drift field
    	131.293,                     // A
    	54                           // Z
		);
		G4double nPhotons = std::round(yields.PhotonYield);
		G4double nElectrons = std::round(yields.ElectronYield);

		
		G4cout << "position: " << pos << G4endl;
		G4cout << "Efield: " << Efield << G4endl;
		G4cout << "edep (keV): " << edep << G4endl;
		G4cout << "recoil type: " << recoilType << G4endl;
		G4cout << "Photon Yield: " << nPhotons << G4endl;
		G4cout << "Electron Yield: " << nElectrons << G4endl;


		G4ParticleDefinition* photonDef = G4OpticalPhoton::OpticalPhotonDefinition();
    	G4ParticleDefinition* eDef = G4Electron::ElectronDefinition(); 
			
		G4TrackVector* secondaries = new G4TrackVector();


    // Spawn optical photons
    	// for (int i = 0; i < nPhotons; ++i) 
		// {

        // 	// G4ThreeVector dir = RandomUnitVector();
		// 	// G4double photonEnergy = 7.0 *eV;  

        // 	// G4DynamicParticle* dynPart = new G4DynamicParticle(photonDef, dir, photonEnergy);
        // 	// G4Track* newTrack = new G4Track(dynPart, track->GetGlobalTime(), pos);
        // 	// newTrack->SetTouchableHandle(track->GetTouchableHandle());
        // 	// newTrack->SetParentID(track->GetTrackID());

        // 	// secondaries->push_back(newTrack);
    	// }

    //Spawn electrons
    	for (int i = 0; i < nElectrons; ++i) 
		{

        	G4ThreeVector dir = RandomUnitVector();
        	G4double electronEnergy = 10.0*eV;

        	G4DynamicParticle* dynPart = new G4DynamicParticle(eDef, dir, electronEnergy);
        	G4Track* newTrack = new G4Track(dynPart, track->GetGlobalTime(), pos);
        	newTrack->SetTouchableHandle(track->GetTouchableHandle());
        	newTrack->SetParentID(track->GetTrackID());

        	secondaries->push_back(newTrack);
			cout << "Creating electron!" << G4endl;

    	}

    	if (!secondaries->empty()) 
		{
			G4TrackVector* trackSecondaries = const_cast<G4Step*>(step)->GetfSecondary();
        	trackSecondaries->insert(trackSecondaries->end(), secondaries->begin(), secondaries->end());
			cout << "done creating secondaries" << G4endl;
    	}

		if (energy < 0.9e-6 *MeV)
		{
			track->SetTrackStatus(fStopAndKill);
			ClearStagnationData(id);
			G4cout << "Killing track ID " << track->GetTrackID() << G4endl;
		}

    	delete secondaries;

    }

	

	
	
	previousEnergy[id] = energy;
	
	// if(volume != fScoringVolume) {
	// 	//G4cout << volume->GetName() << G4endl;
	// 	//G4cout << fScoringVolume->GetName() << G4endl;
	// 	return;
	// }
	
	fEventAction->AddEdep(edep);
}


