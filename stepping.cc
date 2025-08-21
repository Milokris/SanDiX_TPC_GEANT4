#include "stepping.hh"


MySteppingAction::MySteppingAction()
{}

MySteppingAction::~MySteppingAction()
{}


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
double ElThreshold = 412000.0;  //Literature says proportional Electroluminescense starts at 412 kV/cm
double gainArea = voltage / ((ElThreshold) * std::log(b/a)); //   0.015; //in mm so ~15 microns
G4double lastStep = 10000.*mm;
int photPerE = 30;
G4ThreadLocal std::unordered_set<G4int> DriftTrackIDs;


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

    // Get position of interaction
    G4ThreeVector pos = step->GetPreStepPoint()->GetPosition();
	G4ThreeVector dir = step->GetPreStepPoint()->GetMomentumDirection();

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
	

    // If parent is in the drift list, this is a descendant
    if (DriftTrackIDs.count(parentID) && parentID > 0)
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
    }

	NEST::INTERACTION_TYPE recoilType = NEST::NoneType;

	if (pd == G4Electron::ElectronDefinition()) 
	{
    // Estimate probability of ionization 
		double P_ionize = 0;

		// I was going to create my own monty-carlo simulation to ionize the 
		// non-drift electrons realistically (because eIoni in physics list is not 
		// realistic for low energy electrons),
		// but I ran out of time.

    	if (G4UniformRand() < P_ionize)
    	{

			edep = energy / keV;

        	track->SetTrackStatus(fStopAndKill);
        	track->SetKineticEnergy(0.0);

			ClearStagnationData(id);	
    	}
	}

	//Stagnation Control
	if(previousEnergy.count(id))
    {
		G4double prevE = previousEnergy[id];        
    	if(std::abs(energy - prevE) < tol)
        {
           	stagnationCounter[id]++;
    	}
    	else
        {
           	stagnationCounter[id] = 0;
    	}
        
    	if(stagnationCounter[id] >= maxSteps)
        {
            track->SetTrackStatus(fStopAndKill);
			ClearStagnationData(id);	
    		return;
       	}
	}
	else
	{
    	stagnationCounter[id] = 0;
	}
	previousEnergy[id] = energy;


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
		recoilType = NEST::INTERACTION_TYPE::beta;
		//return;
	}
	else if (pd == G4Gamma::GammaDefinition())
	{
		recoilType = NEST::INTERACTION_TYPE::gammaRay;
		//return; //Just to see if I can get better S1 and S2 waveforms with no gamma scintillations
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
				}
				else 
				{
					return;
				}
			}
			else return;
		}

		if (edep <= 14.*pow(10,-3) && !driftElectron) //14 eV needed for scintillation in liquid xenon.
		{
			return;
		}


		if(!S2Event)
		{
			// Create NEST object
			G4cout << "~~~~~~~~~~~~~~~initializing S1 event~~~~~~~~~~~~~~~" << G4endl;

			//I think this is unrealistic to simply plug in energy deposited in a step into NEST,
			//as some energies are deposited over multiple steps leading to double counting
			//however I don't have the time to fix this

			NEST::YieldResult yields = nestCalc.GetYields(recoilType, edep, density, Efield,          // drift field
			131.293,                     // A
			54                           // Z
			);
			nPhotons = std::round(yields.PhotonYield);
			nElectrons = std::round(yields.ElectronYield);

			G4cout << "position: " << pos << G4endl;
			G4cout << "Efield: " << Efield << G4endl;
			G4cout << "edep (keV): " << edep << G4endl;
			G4cout << "recoil type: " << recoilType << G4endl;
			G4cout << "Photon Yield: " << nPhotons << G4endl;
			G4cout << "Electron Yield: " << nElectrons << G4endl;
			G4cout << "Incident Particle: " << incidentParticle << G4endl;
			G4cout << "Particle Energy: " << energy/keV << " keV" << G4endl;
			G4cout << "Track ID: " << trackID << G4endl;
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
			DriftTrackIDs.insert(newTrack->GetTrackID());

        	secondaries->push_back(newTrack);

			createdElectrons = createdElectrons + 1;
    	}

    	if (!secondaries->empty()) 
		{
			G4TrackVector* trackSecondaries = const_cast<G4Step*>(step)->GetfSecondary();
        	trackSecondaries->insert(trackSecondaries->end(), secondaries->begin(), secondaries->end());
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

		if(S2Event)
		{
			track->SetTrackStatus(fStopAndKill);
			ClearStagnationData(id);
		}
    	delete secondaries;

    }

	previousEnergy[id] = energy;
	
	//fEventAction->AddEdep(edep);
}


