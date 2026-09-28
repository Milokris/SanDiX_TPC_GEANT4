#include "stepping.hh"
#include "event.hh"
MySteppingAction::MySteppingAction(MyEventAction* eventAction) : fEventAction(eventAction)
{}

MySteppingAction::~MySteppingAction()
{}

void MySteppingAction::ClearStagnationData(G4int eventID, G4int trackID)
{
	TrackKey key{eventID, trackID};
	previousEnergy.erase(key);
	stagnationCounter.erase(key);
}
void MySteppingAction::ClearAllStagnationData()
{
	previousEnergy.clear();
	stagnationCounter.clear();
}

double ElThreshold = 412000.0; //Literature says proportional Electroluminescense starts at 412 kV/cm
G4double gainArea = (voltage / (ElThreshold * std::log(b/a))) * cm;
G4double lastStep = 10000.*mm;

void MySteppingAction::UserSteppingAction(const G4Step *step)
{
	// Prevent runaway memory usage
	const size_t maxMapSize = 10000; //10000
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
	double beforeEnergy;
	int stepNumber = track->GetCurrentStepNumber();
	const G4ParticleDefinition* pd = track->GetParticleDefinition();
	auto incidentParticle = pd->GetParticleName();
	G4double edep = step->GetTotalEnergyDeposit() / keV;
	const int maxSteps = 1000; //1000
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
	G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
	TrackKey key{eventID, id}; // If parent is in the drift list, this is a descendant

	// Record trajectory for Three.js event display (skip optical photons and drift electrons)
	if (pd != G4OpticalPhoton::OpticalPhotonDefinition() && !(info && info->IsDrift())) {
		fEventAction->RecordStep(trackID, parentID, std::string(incidentParticle),
			(float)pos.x(), (float)pos.y(), (float)pos.z());
	}

	if (fEventAction->IsDriftTrack(parentID) && parentID > 0)
	{
		// add this track to the list for future checks
		fEventAction->AddDriftTrackID(trackID); // kill it
		track->SetTrackStatus(fStopAndKill);
		G4cout << "Killed track " << trackID << " from drift parent " << parentID << G4endl;
		return;
	}
	if (info && info->IsDrift())
	{
		if (volume && volume->GetName() != "logicChamber")
		{
			track->SetTrackStatus(fStopAndKill);
			//G4cout << "Drift electron left logic chamber" << G4endl;
		}
		return;
	}
	NEST::INTERACTION_TYPE recoilType = NEST::NoneType; //Stagnation Control
	if(previousEnergy.count(key))
	{
		G4double prevE = previousEnergy[key];
		if(std::abs(energy - prevE) < tol)
		{
			stagnationCounter[key]++;
		}
		else
		{
			stagnationCounter[key] = 0;
		}
		if(stagnationCounter[key] >= maxSteps - 5)
		{
			G4String process = "";
			if(step->GetPostStepPoint()->GetProcessDefinedStep())
			{
				process = step->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();
			}
			// G4cout << "VERBOSE | Event=" << eventID
			// << " TrackID=" << id
			// << " Step=" << stepNumber
			// << " Particle=" << incidentParticle
			// << " KE (keV)=" << energy/keV
			// << " StepLen=" << stepLength
			// << " Process=" << process
			// << " Pos=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")"
			// << G4endl;
		}
		if(stagnationCounter[key] >= maxSteps)
		{
			G4cout << "--------Particle Stagnated: " << incidentParticle << G4endl;
			// G4cout << "--------Point: " << pos << G4endl;
			// G4cout << "--------Energy: " << energy/keV << " keV" << G4endl;
			// G4cout << "--------Event Number: " << eventID << G4endl; G4cout << " " << G4endl;
		track->SetTrackStatus(fStopAndKill);
		ClearStagnationData(eventID, id);
		return;
		}
	}
	else
	{
		stagnationCounter[key] = 0;
	}

	if (edep > 0.01)
	{
		beforeEnergy = energy/keV + edep;
		// G4cout << "beforeEnergy (keV): " << beforeEnergy << G4endl;
		// G4cout << "energy (keV): " << energy/keV << G4endl;
		// G4cout << "step number: " << stepNumber << G4endl;
	}
	previousEnergy[key] = energy;
	if (pd == G4Neutron::NeutronDefinition() || pd == G4Proton::ProtonDefinition() || pd->GetParticleType() == "nucleus")
	{
		recoilType = NEST::INTERACTION_TYPE::NR; // Nuclear Recoil
	}
	else if (pd == G4Alpha::AlphaDefinition() || pd->GetParticleName() == "ion")
	{
		recoilType = NEST::INTERACTION_TYPE::ion;
	}
	else if (pd == G4Electron::ElectronDefinition() || pd == G4Positron::PositronDefinition() || pd->GetParticleType() == "muon" || pd->GetParticleType() == "tau")
	{
		recoilType = NEST::INTERACTION_TYPE::beta;
		//return; //this stops a signal from betas for cleaner nuclear recoil signal from neutrons
	}
	else if (pd == G4Gamma::GammaDefinition())
	{
		recoilType = NEST::INTERACTION_TYPE::gammaRay;
		//return; //this stops a signal from gammas for cleaner nuclear recoil signal from neutrons
	}
	else if (pd == G4OpticalPhoton::OpticalPhotonDefinition())
	{
		return;
		recoilType = NEST::INTERACTION_TYPE::gammaRay;
	}

	//This stops events from happening too close to the wall
	if (edep > 0)
	{
		if (r_unit > 2.0)
		{
			//G4cout << "TOO CLOSE TO WALL R = " << r_unit << G4endl;
			//G4cout << "Position: " << pos << G4endl;
			return;
		}

		MyEventAction* eventAction = (MyEventAction*)G4RunManager::GetRunManager()->GetUserEventAction();
		if (eventAction->incidentRecoilType == 1) //neutron
		{
			if (recoilType != NEST::INTERACTION_TYPE::NR) return;

			// if (incidentID > -1)
			// {
			//if (id != incidentID) return;
			//This stops S1 events from secondary particles
			// if (incidentX > x+0.01 || incidentX < x-0.01)
			// {
			// G4cout << "not in initial cluster!!!" << G4endl;
			// return;
			// }
			// if (incidentY > y+0.01 || incidentY < y-0.01)
			// {
			// G4cout << "not in initial cluster!!!" << G4endl;
			// return;
			// }
			// if (incidentZ > z+0.01 || incidentZ < z-0.01)
			// {
			// G4cout << "not in initial cluster!!!" << G4endl;
			// return;
			// }
			// }
			// else
			// {
			// incidentID = id;
			// incidentX = x;
			// incidentY = y;
			// incidentZ = z;
			// }
		}
	}

	//This segment stops multiple S1 signals for incident gammas
	// if (edep > 0)
	// {
	// if (pd == G4Gamma::GammaDefinition() || pd == G4Electron::ElectronDefinition() || pd == G4Positron::PositronDefinition() ||
	// pd->GetParticleType() == "muon" || pd->GetParticleType() == "tau")
	// {
	// if (incidentID > -1)
	// {
	// if (id != incidentID) return;
	//This stops S1 events from secondary particles
	// if (incidentX > x+0.5 || incidentX < x-0.5) return;
	// if (incidentY > y+0.5 || incidentY < y-0.5) return;
	// if (incidentZ > z+0.5 || incidentZ < z-0.5) return;
	// }
	// else
	// {
	// incidentID = id;
	// incidentX = x;
	// incidentY = y;
	// incidentZ = z;
	// }
	// }
	// }


	//Nest Part
	if (volume && volume->GetName() == "logicChamber")
	{
		if (driftElectron)
		{
			if (edep > 0)
			{
				if (Efield > ElThreshold)
				{ nPhotons = fEventAction->photPerE;
					nElectrons = 0; S2Event = true;
				}
				else
				{
					G4cout << "Drift electron edep outside S2 zone" << G4endl;
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
			G4cout << G4endl;
			G4cout << "~~~~~~~~~~~~~~~initializing S1 event~~~~~~~~~~~~~~~" << G4endl;
			NEST::YieldResult yields = nestCalc.GetYields(recoilType, edep, density, Efield, // drift field
				131.293, // A
				54 // Z
				);
			NEST::QuantaResult quanta = nestCalc.GetQuanta(yields, density);
			nPhotons = quanta.photons; //divide by 10 for VR file size
			nElectrons = quanta.electrons; //divide by 100 for VR file size
			int nExcitons = quanta.excitons;
			int nIons = quanta.ions;
			int nRecombinedElectrons = nIons - nElectrons;
			//For mean value, not spread
			//nPhotons = std::round(yields.PhotonYield)/10; //divide by 10 for VR file size
			//nElectrons = std::round(yields.ElectronYield); //Divide by 100 is just to decrease file size for VR

			G4cout << "Incident Particle: " << incidentParticle << G4endl;
			G4cout << "position: " << pos << G4endl;
			G4cout << "Particle Energy: " << beforeEnergy << " keV" << G4endl;
			G4cout << G4endl;
			G4cout << "edep (keV): " << edep << G4endl;
			G4cout << "Efield (V/cm): " << Efield << G4endl;
			// G4cout << "recoil type: " << recoilType << G4endl;
			G4cout << "Number of excimers: " << nExcitons << G4endl;
			G4cout << "Number of ions: " << nIons << G4endl;
			G4cout << G4endl;
			G4cout << "Number of escape Electrons: " << nElectrons << G4endl;
			G4cout << "Number of Recombined Electrons: " << nRecombinedElectrons << G4endl;
			G4cout << G4endl;
			NEST::photonstream timing = nestCalc.GetPhotonTimes(recoilType, nPhotons, nExcitons, Efield, edep);
			int nFastPhotons = 0, nSlowPhotons = 0;
			for (double t : timing)
			{
				if (t < 10.0) nFastPhotons++;  // ~10 ns threshold
				else          nSlowPhotons++;
			}
			G4cout << "Photon Yield: " << nPhotons << G4endl;
			G4cout << "Fast photons: " << nFastPhotons << G4endl;
			G4cout << "Slow photons: " << nSlowPhotons << G4endl;
			G4cout << G4endl;



			G4cout << "Track ID: " << trackID << G4endl;
			G4cout << " " << G4endl;
			G4double time = track->GetGlobalTime(); // in ns
			// G4cout << "Time: " << time << " ns" << G4endl;
			// G4cout << " " << G4endl;
		}

		G4ParticleDefinition* photonDef = G4OpticalPhoton::OpticalPhotonDefinition();
		G4ParticleDefinition* eDef = G4Electron::ElectronDefinition();
		G4TrackVector* secondaries = new G4TrackVector();

		//Spawn optical photons
		for (int i = 0; i < nPhotons; ++i) //divided by 100 to reduce file size for VR
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
			fEventAction->AddDriftTrackID(newTrack->GetTrackID());
			secondaries->push_back(newTrack);
			fEventAction->AddElectrons(1);
		}
		// Record origin of this electron cluster for Three.js drift-line display (S1 only)
		if (nElectrons > 0 && !S2Event) {
			fEventAction->AddDriftCluster((float)pos.x(), (float)pos.y(), (float)pos.z(), nElectrons);
		}
		if (!secondaries->empty())
		{
			G4TrackVector* trackSecondaries = const_cast<G4Step*>(step)->GetfSecondary();
			trackSecondaries->insert(trackSecondaries->end(), secondaries->begin(), secondaries->end());
		}
		if (S2Event)
		{
			fEventAction->AddS2Photons((G4int)nPhotons);
		}
		else
		{
			fEventAction->AddS1Photons((G4int)nPhotons);
		}
		if(S2Event)
		{
			track->SetTrackStatus(fStopAndKill);
			ClearStagnationData(eventID, id);
		}
		delete secondaries;
	}
	previousEnergy[key] = energy;
	//fEventAction->AddEdep(edep);
}





















//-----------This code is for clustering S1 events together, which should be more physically accurate, but it doesn't seem to effect anything. keeping it just in case I need it.
//Make sure to change both stepping.hh and stepping.cc




// #include "stepping.hh"
// #include "event.hh"


// MySteppingAction::MySteppingAction(MyEventAction* eventAction)
//     : fEventAction(eventAction)
// {}

// MySteppingAction::~MySteppingAction()
// {}


// const double ElThreshold = 412000.0;  //Literature says proportional Electroluminescense starts at 412 kV/cm
// double gainArea = voltage / ((ElThreshold) * std::log(b/a)); //   0.015; //in mm so ~15 microns
// G4double lastStep = 10000.*mm;
// double density = 3.0558; //3.0558*g/cm3
// double temp = 162; //162 Kelvin
// double pressure = 1; //1 Bar (atmospheric pressure)
// //static nestPart detector;
// static NEST::NESTcalc nestCalc(&detector);
// double Efield;
// bool S2Event;
// int nPhotons;
// int nElectrons;
// const double CLUSTER_RADIUS = 0.5*mm; //1.0*mm;  // tune this (0.1–1 mm typical) was 0.5*mm

// void MySteppingAction::ClearStagnationData(G4int eventID, G4int trackID)
// {
//     TrackKey key{eventID, trackID};
//     previousEnergy.erase(key);
//     stagnationCounter.erase(key);
// }

// void MySteppingAction::ClearAllStagnationData()
// {
// 	previousEnergy.clear();
// 	stagnationCounter.clear();
// 	clusters.clear();
// }

// void MySteppingAction::ProcessCluster(const Cluster& c, NEST::INTERACTION_TYPE recoilType, G4Track* track, double beforeEnergy, const G4Step* step)
// {
//     double edep = c.edep;
//     G4ThreeVector pos = c.pos;

// 	auto incidentParticle = track->GetParticleDefinition()->GetParticleName();
// 	G4int trackID = track->GetTrackID();

// 	if(!S2Event)
// 	{
// 		// Create NEST object
// 		G4cout << G4endl;
// 		G4cout << "~~~~~~~~~~~~~~~initializing S1 event~~~~~~~~~~~~~~~" << G4endl;

// 		NEST::YieldResult yields = nestCalc.GetYields(recoilType, edep, density, Efield,          // drift field
// 		131.293,                     // A
// 		54                           // Z
// 		);
// 		NEST::QuantaResult quanta = nestCalc.GetQuanta(yields, density);

// 		nPhotons = quanta.photons;   //divide by 10 for VR file size
// 		nElectrons = quanta.electrons; //divide by 100 for VR file size

// 		//For mean value, not spread
// 		//nPhotons = std::round(yields.PhotonYield)/10; //divide by 10 for VR file size
// 		//nElectrons = std::round(yields.ElectronYield); //Divide by 100 is just to decrease file size for VR

// 		G4cout << "position: " << pos << G4endl;
// 		// G4cout << "Efield: " << Efield << G4endl;
// 		G4cout << "edep (keV): " << edep << G4endl;
// 		// G4cout << "recoil type: " << recoilType << G4endl;
// 		// G4cout << "Photon Yield: " << nPhotons << G4endl;
// 		// G4cout << "Electron Yield: " << nElectrons << G4endl;
// 		G4cout << "Particle: " << incidentParticle << G4endl;
// 		G4cout << "Particle Energy: " << beforeEnergy << " keV" << G4endl;
// 		G4cout << "Track ID: " << trackID << G4endl;
// 		G4cout << "   " << G4endl;
// 		G4double time = track->GetGlobalTime(); // in ns
// 		// G4cout << "Time: " << time << " ns" << G4endl;
// 		// G4cout << "   " << G4endl;

// 	}


// 	G4ParticleDefinition* photonDef = G4OpticalPhoton::OpticalPhotonDefinition();
// 	G4ParticleDefinition* eDef = G4Electron::ElectronDefinition();

// 	G4TrackVector* secondaries = new G4TrackVector();

// // Spawn optical photons
// 	// for (int i = 0; i < nPhotons; ++i) //divided by 100 to reduce file size for VR
// 	// {

// 	// 		G4ThreeVector dir = RandomUnitVector();
// 	// 		G4double photonEnergy = SampleLXePhotonEnergy_GaussEnergy();

// 	// 		G4DynamicParticle* dynPart = new G4DynamicParticle(photonDef, dir, photonEnergy);
// 	// 		G4ThreeVector perp = dir.orthogonal();
// 	// 		G4double phi = CLHEP::twopi * G4UniformRand();
// 	// 		G4ThreeVector pol = perp.rotate(dir, phi).unit();
// 	// 		dynPart->SetPolarization(pol);

// 	// 		G4Track* newTrack = new G4Track(dynPart, track->GetGlobalTime(), pos);
// 	// 		newTrack->SetTouchableHandle(track->GetTouchableHandle());
// 	// 		newTrack->SetParentID(track->GetTrackID());

// 	// 		secondaries->push_back(newTrack);
// 	// }

// //Spawn electrons
// 	for (int i = 0; i < nElectrons; ++i)
// 	{

// 		G4ThreeVector dir = RandomUnitVector();
// 		G4double electronEnergy = 0.1*eV;

// 		G4DynamicParticle* dynPart = new G4DynamicParticle(eDef, dir, electronEnergy);
// 		G4Track* newTrack = new G4Track(dynPart, track->GetGlobalTime(), pos);
// 		newTrack->SetUserInformation(new DriftElectronInfo(true, true));
// 		newTrack->SetTouchableHandle(track->GetTouchableHandle());
// 		newTrack->SetParentID(track->GetTrackID());
// 		fEventAction->AddDriftTrackID(newTrack->GetTrackID());

// 		secondaries->push_back(newTrack);

// 		fEventAction->AddElectrons(1);
// 	}

// 	if (!secondaries->empty())
// 	{
// 		G4TrackVector* trackSecondaries = const_cast<G4Step*>(step)->GetfSecondary();
// 		trackSecondaries->insert(trackSecondaries->end(), secondaries->begin(), secondaries->end());
// 	}

// 	if (S2Event)
//     	fEventAction->AddS2Photons(nPhotons);
// 	else
//     	fEventAction->AddS1Photons(nPhotons);

// 	delete secondaries;

// }




// void MySteppingAction::UserSteppingAction(const G4Step *step)
// {
// 	// Prevent runaway memory usage
// 	const size_t maxMapSize = 10000; //10000
// 	if (previousEnergy.size() > maxMapSize)
// 	{
// 		G4cout << "Clearing previousEnergy map!" << G4endl;
// 		previousEnergy.clear();
// 		stagnationCounter.clear();
// 	}


// 	G4LogicalVolume *volume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume();

//     // Get position of interaction
//     G4ThreeVector pos = step->GetPreStepPoint()->GetPosition();
// 	G4ThreeVector dir = step->GetPreStepPoint()->GetMomentumDirection();

// 	const MyDetectorConstruction *detectorConstruction = static_cast<const MyDetectorConstruction*> (G4RunManager::GetRunManager()->GetUserDetectorConstruction());

// 	G4LogicalVolume *fScoringVolume = detectorConstruction->GetScoringVolume();

// 	G4Track *track = step->GetTrack();
// 	G4int id = track->GetTrackID();
// 	G4double energy = track->GetKineticEnergy();
// 	double beforeEnergy;
// 	int stepNumber = track->GetCurrentStepNumber();
// 	const G4ParticleDefinition* pd = track->GetParticleDefinition();
// 	auto incidentParticle = pd->GetParticleName();
// 	G4double edep = step->GetTotalEnergyDeposit() / keV;
// 	const int maxSteps = 1000; //1000
// 	const double tol = 0.1 * eV;
// 	S2Event = false;
// 	bool driftElectron = false;
// 	auto info = dynamic_cast<DriftElectronInfo*>(track->GetUserInformation());
//     Efield = nestDetector->get_ElectricField(pos.x() / cm, pos.y() / cm, pos.z() / cm);
// 	double x = pos.x() / cm;
// 	double y = pos.y() / cm;
// 	double z = pos.z() / cm;
// 	double r_unit = std::sqrt(x*x + y*y + z*z);
// 	double x_unit = (r_unit > 0 ? x / r_unit : 0.);
// 	double y_unit = (r_unit > 0 ? y / r_unit : 0.);
// 	double z_unit = (r_unit > 0 ? z / r_unit : 0.);
// 	double r_E = std::sqrt(x*x + y*y);
// 	double x_E = (r_E > 0 ? x / r_E : 0.);
//     double y_E = (r_E > 0 ? y / r_E : 0.);
// 	G4double stepLength = step->GetStepLength();
// 	nPhotons = 0;
// 	nElectrons = 0;
// 	G4int parentID = track->GetParentID();
//     G4int trackID = track->GetTrackID();
// 	G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
// 	TrackKey key{eventID, id};

// 	auto FinalizeIfActive = [&]() {
//     if (clusters.count(key) && clusters[key].active)
//     {
//         Cluster& c = clusters[key];
//         ProcessCluster(c, c.recoilType, track, c.beforeEnergy, step);
//         c.active = false;
//     }
// 	};

// 	if (clusters.count(key) && clusters[key].active)
// 	{
// 		double dist = (pos - clusters[key].pos).mag();
// 		if (dist > CLUSTER_RADIUS || energy/keV < 1.0)
// 		{
// 			ProcessCluster(clusters[key], clusters[key].recoilType, track, clusters[key].beforeEnergy, step);
// 			clusters[key].active = false;
// 		}
// 	}


//     // If parent is in the drift list, this is a descendant
//     if (fEventAction->IsDriftTrack(parentID) && parentID > 0)
//     {
//         // add this track to the list for future checks
//         fEventAction->AddDriftTrackID(trackID);

//         // kill it
//         track->SetTrackStatus(fStopAndKill);
//         G4cout << "Killed track " << trackID << " from drift parent " << parentID << G4endl;
//         return;
//     }


//     if (info && info->IsDrift())
// 	{
//         if (volume && volume->GetName() != "logicChamber")
// 		{
// 			track->SetTrackStatus(fStopAndKill);
// 			//G4cout << "Drift electron left logic chamber" << G4endl;
// 		}
// 		return;
//     }

// 	NEST::INTERACTION_TYPE recoilType = NEST::NoneType;


// 	//Stagnation Control
// 	if(previousEnergy.count(key))
//     {
// 		G4double prevE = previousEnergy[key];
//     	if(std::abs(energy - prevE) < tol)
//         {
//            	stagnationCounter[key]++;
//     	}
//     	else
//         {
//            	stagnationCounter[key] = 0;
//     	}

// 		if(stagnationCounter[key] >= maxSteps - 5)
// 		{
// 			G4String process = "";
//         	if(step->GetPostStepPoint()->GetProcessDefinedStep())
// 			{
//             	process = step->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();
// 			}

// 			// G4cout << "VERBOSE | Event=" << eventID
//             //    << " TrackID=" << id
//             //    << " Step=" << stepNumber
//             //    << " Particle=" << incidentParticle
//             //    << " KE (keV)=" << energy/keV
//             //    << " StepLen=" << stepLength
//             //    << " Process=" << process
//             //    << " Pos=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")"
//             //    << G4endl;
// 		}

//     	if(stagnationCounter[key] >= maxSteps)
//         {
// 			G4cout << "--------Particle Stagnated: " << incidentParticle << G4endl;
// 			// G4cout << "--------Point: " << pos << G4endl;
// 			G4cout << "--------Energy: " << energy/keV << " keV" << G4endl;
// 			// G4cout << "--------Event Number: " << eventID << G4endl;
// 			G4cout << " " << G4endl;
// 			FinalizeIfActive();
//             track->SetTrackStatus(fStopAndKill);
// 			ClearStagnationData(eventID, id);
//     		return;
//        	}
// 	}
// 	else
// 	{
//     	stagnationCounter[key] = 0;
// 	}
// 	if (edep > 0.01)
// 	{
// 		beforeEnergy = energy/keV + edep;
// 		// G4cout << "beforeEnergy (keV): " << beforeEnergy << G4endl;
// 		// G4cout << "energy (keV): " << energy/keV << G4endl;
// 		// G4cout << "step number: " << stepNumber << G4endl;
// 	}
// 	previousEnergy[key] = energy;


// 	if (pd == G4Neutron::NeutronDefinition() || pd == G4Proton::ProtonDefinition() || pd->GetParticleType() == "nucleus")
// 	{
//  		recoilType = NEST::INTERACTION_TYPE::NR; // Nuclear Recoil
// 	}
// 	else if (pd == G4Alpha::AlphaDefinition() || pd->GetParticleName() == "ion")
// 	{
// 		recoilType = NEST::INTERACTION_TYPE::ion;
// 	}
// 	else if (pd == G4Electron::ElectronDefinition() || pd == G4Positron::PositronDefinition() ||
// 			pd->GetParticleType() == "muon" || pd->GetParticleType() == "tau")
// 	{
// 		recoilType = NEST::INTERACTION_TYPE::beta;
// 		//return; //this stops a signal from betas for cleaner nuclear recoil signal from neutrons
// 	}
// 	else if (pd == G4Gamma::GammaDefinition())
// 	{
// 		recoilType = NEST::INTERACTION_TYPE::gammaRay;
// 		//return; //this stops a signal from gammas for cleaner nuclear recoil signal from neutrons
// 	}
// 	else if (pd == G4OpticalPhoton::OpticalPhotonDefinition())
// 	{
// 		return;
// 		recoilType = NEST::INTERACTION_TYPE::gammaRay;
// 	}

// 	//This stops events from happening too close to the wall
// 	if (edep > 0)
// 	{

// 		if (r_unit > 2.0)
// 		{
// 			//G4cout << "TOO CLOSE TO WALL R = " << r_unit << G4endl;
// 			//G4cout << "Position: " << pos << G4endl;
// 			FinalizeIfActive();
// 			return;
// 		}
// 		MyEventAction* eventAction = (MyEventAction*)G4RunManager::GetRunManager()->GetUserEventAction();
// 		if (eventAction->incidentRecoilType == 1) //neutron
// 		{
// 			if (recoilType != NEST::INTERACTION_TYPE::NR)
// 			{
// 				return;
// 			}

// 			// if (incidentID > -1)
// 			// {
// 				//if (id != incidentID) return; //This stops S1 events from secondary particles
// 			// 	if (incidentX > x+0.01 || incidentX < x-0.01)
// 			// 	{
// 			// 		G4cout << "not in initial cluster!!!" << G4endl;
// 			// 		return;
// 			// 	}
// 			// 	if (incidentY > y+0.01 || incidentY < y-0.01)
// 			// 	{
// 			// 		G4cout << "not in initial cluster!!!" << G4endl;
// 			// 		return;
// 			// 	}
// 			// 	if (incidentZ > z+0.01 || incidentZ < z-0.01)
// 			// 	{
// 			// 		G4cout << "not in initial cluster!!!" << G4endl;
// 			// 		return;
// 			// 	}
// 			// }
// 			// else
// 			// {
// 			// 	incidentID = id;
// 			// 	incidentX = x;
// 			// 	incidentY = y;
// 			// 	incidentZ = z;
// 			// }
// 		}

// 	}

// 	//This segment stops multiple S1 signals for incident gammas
// 	// if (edep > 0)
// 	// {
// 	// 	if (pd == G4Gamma::GammaDefinition() || pd == G4Electron::ElectronDefinition() || pd == G4Positron::PositronDefinition() ||
// 	// 	pd->GetParticleType() == "muon" || pd->GetParticleType() == "tau")
// 	// 	{
// 	// 		if (incidentID > -1)
// 	// 		{
// 	// 			if (id != incidentID) return; //This stops S1 events from secondary particles
// 	// 			if (incidentX > x+0.5 || incidentX < x-0.5) return;
// 	// 			if (incidentY > y+0.5 || incidentY < y-0.5) return;
// 	// 			if (incidentZ > z+0.5 || incidentZ < z-0.5) return;
// 	// 		}
// 	// 		else
// 	// 		{
// 	// 			incidentID = id;
// 	// 			incidentX = x;
// 	// 			incidentY = y;
// 	// 			incidentZ = z;
// 	// 		}
// 	// 	}
// 	// }


// 	//Nest Part

//   	if (volume && volume->GetName() == "logicChamber")
// 	{
// 		if (driftElectron)
// 		{
// 			if (edep > 0)
// 			{
// 				if (Efield > ElThreshold)
// 				{
// 					nPhotons = fEventAction->photPerE;
// 					nElectrons = 0;
// 					S2Event = true;
// 				}
// 				else
// 				{
// 					G4cout << "Drift electron edep outside S2 zone" << G4endl;
// 					return;
// 				}
// 			}
// 			else return;
// 		}

// 		if (edep <= 14.*pow(10,-3) && !driftElectron) //14 eV needed for scintillation in liquid xenon.
// 		{
// 			return;
// 		}

// 		Cluster& c = clusters[key];

// 		if (!c.active)
// 		{
// 			c.pos          = pos;
// 			c.edep         = edep;
// 			c.trackID      = trackID;
// 			c.recoilType   = recoilType;
// 			c.beforeEnergy = beforeEnergy;
// 			c.active       = true;
// 		}
// 		else
// 		{
// 			double dist = (pos - c.pos).mag();

// 			if (dist < CLUSTER_RADIUS)
// 			{
// 				// Accumulate — fix centroid update order
// 				double oldEdep = c.edep;
// 				c.edep += edep;
// 				c.pos = (c.pos * oldEdep + pos * edep) / c.edep;
// 				c.beforeEnergy = beforeEnergy;

// 				G4cout << "Are we getting here? " << G4endl;
// 				G4cout << "Old edep: " << oldEdep << G4endl;
// 				G4cout << "New edep: " << c.edep << G4endl;
// 				G4cout << "Centroid Position: " << c.pos << G4endl;


// 				// if (energy/eV <= 1.0)
// 				// {
// 				// 	G4cout << "Closing Cluster, Particle Energy: " << energy << G4endl;
// 				// 	ProcessCluster(c, recoilType, track, beforeEnergy, step);
// 				// }
// 			}
// 		}

// 		G4TrackStatus status = track->GetTrackStatus();
// 		bool trackEnding = (status == fStopAndKill || status == fStopButAlive);

// 		if (trackEnding)
// 		{
// 			FinalizeIfActive();
// 		}


// 		if(S2Event)
// 		{
// 			track->SetTrackStatus(fStopAndKill);
// 			ClearStagnationData(eventID, id);
// 		}

//     }

// 	previousEnergy[key] = energy;

// 	//fEventAction->AddEdep(edep);
// }
