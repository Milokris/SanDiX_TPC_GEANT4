void graphing(const char* filename="output0.root") {

    TFile *f = TFile::Open(filename);
    if (!f || f->IsZombie()) {
        std::cerr << "Could not open file!" << std::endl;
        return;
    }

    // Get histograms
    TH1 *hTime = (TH1*)f->Get("Time");
    TH1 *hX    = (TH1*)f->Get("posX");
    TH1 *hY    = (TH1*)f->Get("posY");
    TH1 *hZ    = (TH1*)f->Get("posZ");

    // Set titles and axis labels
    hTime->SetTitle("Photon counts vs time");
    hTime->GetXaxis()->SetTitle("time [#mus]"); //#mu
    hTime->GetYaxis()->SetTitle("hits");

    hX->SetTitle("Photon counts vs X position");
    hX->GetXaxis()->SetTitle("X position [mm]");
    hX->GetYaxis()->SetTitle("hits");

    hY->SetTitle("Photon counts vs Y position");
    hY->GetXaxis()->SetTitle("Y position [mm]");
    hY->GetYaxis()->SetTitle("hits");

    hZ->SetTitle("Photon counts vs Z position");
    hZ->GetXaxis()->SetTitle("Z position [mm]");
    hZ->GetYaxis()->SetTitle("hits");

    // Improve line visibility
    for (auto h : {hTime, hX, hY, hZ}) {
        h->SetLineWidth(2);
        h->SetLineColor(kBlue+1);
    }

    TCanvas *c = new TCanvas("c","Photon histograms",1200,800);
    //c->Divide(2,2);

    c->cd(1); hTime->Draw("HIST");
    // c->cd(2); hX->Draw("HIST");
    // c->cd(3); hY->Draw("HIST");
    // c->cd(4); hZ->Draw("HIST");

    c->Update();


    //--------Golden Parameter Plot------

    TTree *tEvents = (TTree*)f->Get("Events");
    if (!tEvents) {
        std::cerr << "Could not find ntuple 'Events'!" << std::endl;
        return;
    }

    // Create a scatter plot of logS2 vs S1
    TCanvas *c2 = new TCanvas("c2","Golden parameter",800,600);

    // Draw ER and NR separately for clarity
    tEvents->SetMarkerStyle(20);
    tEvents->SetMarkerSize(0.7);

    // ER events (recoilType==0) in blue
    tEvents->SetMarkerColor(kBlue);
    //tEvents->Draw("logS2:S1","recoilType==0","P");
    tEvents->Draw("logS2:S1>>hER(100,0,100000,100,2,6)","recoilType==0","P");


    TH1 *h = (TH1*)gPad->GetPrimitive("htemp");
    if (h) 
    {
        h->GetYaxis()->SetRangeUser(-10, 8);
    }
    // NR events (recoilType==1) in red
    tEvents->SetMarkerColor(kRed);
    //tEvents->Draw("logS2:S1","recoilType==1","P"); // "P SAME"
    tEvents->Draw("logS2:S1>>hNR(100,0,15000,100,2,6)","recoilType==1","P"); // "P SAME"



    c2->BuildLegend();
    c2->Update();

    // Save plot to file
    //c1->SaveAs("time_hist.png");

}
