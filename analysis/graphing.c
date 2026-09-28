#include <TChain.h>
#include <TFile.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TLegend.h>
#include <TSystemDirectory.h>
#include <TList.h>
#include <TSystemFile.h>
#include <iostream>

void graphing(const char* pattern="output")
{
    const double NR_ACCEPTANCE = 0.50; //Set NR_acceptance value here

    // Find all ROOT files beginning with "output"
    TSystemDirectory dir(".", ".");
    TList *files = dir.GetListOfFiles();

    if(!files)
    {
        std::cerr<<"No files found\n";
        return;
    }

    TIter next(files);
    TSystemFile *file;

    TChain chain("Events");

    TH1F *hDriftAll = new TH1F("hDriftAll", "Electron Drift Times", 1000, -1, 10);
    hDriftAll->SetDirectory(0);

    int canvasIndex = 0;

    // Loop over files
    while((file=(TSystemFile*)next()))
    {
        TString fname = file->GetName();

        if(file->IsDirectory()) continue;
        if(!fname.EndsWith(".root")) continue;
        if(!fname.BeginsWith(pattern)) continue;

        std::cout<<"Reading "<<fname<<std::endl;

        TFile f(fname);
        if(f.IsZombie()) continue;

        if(f.Get("Events"))
        {
            chain.Add(fname);
        }
        else
        {
            std::cout<<"No Events tree in "<<fname<<std::endl;
        }

        // Waveform plotting (independent of chain logic)
        TH1 *hTime = (TH1*)f.Get("Time");
        if(!hTime) continue;

        if(hTime->GetEntries() == 0)
        {
            std::cout<<"Skipping waveform for "<<fname
                     <<" (no photon activity)"<<std::endl;
            continue;
        }
        // Accumulate drift time histogram across all files
        TH1 *hDrift = (TH1*)f.Get("drift times");
        if(hDrift)
        {
            hDrift->SetDirectory(0);
            hDriftAll->Add(hDrift);
        }

        hTime->SetDirectory(0);

        TString cname;
        cname.Form("cTime_%d",canvasIndex++);

        //TCanvas *cRun = new TCanvas(cname,fname,900,650); //comment out to not draw waveforms

        hTime->SetTitle(fname + " : photon hits vs time");
        hTime->GetXaxis()->SetTitle("time [#mus]");
        hTime->GetYaxis()->SetTitle("hits");
        hTime->SetLineWidth(2);
        hTime->SetLineColor(kBlue+1);

        //hTime->Draw("HIST"); //comment out to not draw waveforms
        //cRun->Update(); //comment out to not draw waveforms

    }

    // ------------------------------------------------------------
    // Golden parameter plot
    // ------------------------------------------------------------
    Long64_t totalEntries = chain.GetEntries();
    if(totalEntries == 0)
    {
        std::cerr<<"No Events entries found in chain\n";
        return;
    }
    std::cout<<"Total entries in chain: " << totalEntries << std::endl;
    std::cout<<"ER entries: " << chain.GetEntries("recoilType==0") << std::endl;
    std::cout<<"NR entries: " << chain.GetEntries("recoilType==1") << std::endl;

    TCanvas *cGold = new TCanvas("cGold","Golden Parameter",800,600);
    chain.SetMarkerStyle(20);
    chain.SetMarkerSize(0.7);

    // Draw all points first to establish axes
    chain.Draw("logS2:S1","","P");

    TH1 *htemp = (TH1*)gPad->GetListOfPrimitives()->FindObject("htemp");
    if(htemp) {
        htemp->GetYaxis()->SetTitle("log_{10} S2"); //S2 or S2/S1
        htemp->GetYaxis()->SetTitleOffset(1.2);
        gPad->Modified();
    }

    // ER band
    chain.SetMarkerColor(kBlue);
    chain.Draw("logS2:S1","recoilType==0","P SAME");

    // NR band
    chain.SetMarkerColor(kRed);
    chain.Draw("logS2:S1","recoilType==1","P SAME");

    // Manual legend with dummy marker objects
    TLegend *leg = new TLegend(0.7, 0.75, 0.88, 0.88);
    leg->SetBorderSize(1);

    TMarker *mER = new TMarker(0, 0, 20);
    mER->SetMarkerColor(kBlue);
    mER->SetMarkerSize(0.7);

    TMarker *mNR = new TMarker(0, 0, 20);
    mNR->SetMarkerColor(kRed);
    mNR->SetMarkerSize(0.7);

    leg->AddEntry(mER, "ER", "P");
    leg->AddEntry(mNR, "NR", "P");
    leg->Draw();

    cGold->Update();
    std::cout<<"Done\n";

    // ------------------------------------------------------------
    // Drift time plot
    // ------------------------------------------------------------
    if(hDriftAll->GetEntries() > 0)
    {
        TCanvas *cDrift = new TCanvas("cDrift","Drift Times",800,600);
        hDriftAll->SetLineColor(kBlue+1);
        hDriftAll->SetLineWidth(2);
        hDriftAll->GetXaxis()->SetTitle("drift time [#mus]");
        hDriftAll->GetYaxis()->SetTitle("counts");
        hDriftAll->Draw("HIST");
        cDrift->Update();
    }
    else
    {
        std::cout<<"No drift time histogram found in any file\n";
    }

    // ------------------------------------------------------------
    // ER Leakage Calculation
    // ------------------------------------------------------------

    // --- Configuration ---
    const int    S1_MIN   = 1;
    const int    S1_MAX   = 160;
    const int    BIN_WIDTH = 10;    // S1 hits per bin
    const int    N_BINS   = (S1_MAX - S1_MIN) / BIN_WIDTH;

    const double LOG_MIN  = 1.2;   // logS2 axis range (match your plot)
    const double LOG_MAX  =  2.8;   // was 1
    const int    LOG_BINS =  60;    // number of histogram bins for the projection

    // Arrays to store per-bin results
    double binCenters[N_BINS], leakage[N_BINS];
    double mu_NR[N_BINS], sigma_NR[N_BINS];
    double mu_ER[N_BINS], sigma_ER[N_BINS];
    int    nBins_used = 0;
    double nER_perbin[N_BINS];

    double leakage_err[N_BINS];      // analytical Gaussian uncertainty
    double leakage_direct[N_BINS];   // direct count leakage
    double leakage_direct_err[N_BINS]; // binomial uncertainty on direct count
    double s1_err[N_BINS];           // x error bars = half the bin width
    double cut_vals[N_BINS];  // actual cut position in logS2
    double leakage_direct_upperlimit[N_BINS];


    // Weights for total leakage average
    double totalLeakageNumer = 0.0;
    double totalLeakageDenom = 0.0;

    std::cout << "\n=== Leakage Calculation ===" << std::endl;
    std::cout << Form("%-10s %-10s %-10s %-10s %-10s %-12s %-12s %-12s %-12s",
                    "S1_lo","S1_hi","mu_NR","mu_ER","sigma_ER",
                    "Leak_Gaus","Gaus_err","Leak_Direct","Direct_err")
            << std::endl;

    for(int i = 0; i < N_BINS; i++)
    {
        int s1_lo = S1_MIN + i * BIN_WIDTH;
        int s1_hi = s1_lo + BIN_WIDTH;
        double s1_center = 0.5 * (s1_lo + s1_hi);

        // --- Build selection strings ---
        TString cutNR = Form("recoilType==1 && S1>=%d && S1<%d", s1_lo, s1_hi);
        TString cutER = Form("recoilType==0 && S1>=%d && S1<%d", s1_lo, s1_hi);

        // --- Create 1D projection histograms ---
        TString hNR_name = Form("hNR_bin%d", i);
        TString hER_name = Form("hER_bin%d", i);

        TH1F *hNR = new TH1F(hNR_name, hNR_name, LOG_BINS, LOG_MIN, LOG_MAX);
        TH1F *hER = new TH1F(hER_name, hER_name, LOG_BINS, LOG_MIN, LOG_MAX);

        // Fill from TChain using Draw with >> syntax
        chain.Project(hNR_name, "logS2", cutNR);
        chain.Project(hER_name, "logS2", cutER);


        // --- Guard 1: too few total entries ---
        if(hNR->GetEntries() < 20 || hER->GetEntries() < 20)
        {
            std::cout << Form("Bin S1=[%d,%d): too few events (NR=%lld, ER=%lld), skipping",
                            s1_lo, s1_hi,
                            (long long)hNR->GetEntries(),
                            (long long)hER->GetEntries())
                    << std::endl;
            delete hNR; delete hER;
            continue;
        }

        // --- Guard 2: too few entries within the logS2 histogram range ---
        // GetEntries() counts everything including over/underflow; Integral() does not.
        // If events fall outside [LOG_MIN, LOG_MAX] the fit will see an empty histogram
        // and return a null TFitResultPtr, causing a segfault on ->Status().
        if(hNR->Integral() < 20 || hER->Integral() < 20)
        {
            std::cout << Form("Bin S1=[%d,%d): too few entries inside logS2 range "
                              "[%.2f, %.2f] (NR=%.0f, ER=%.0f), skipping — "
                              "consider widening LOG_MIN/LOG_MAX",
                              s1_lo, s1_hi, LOG_MIN, LOG_MAX,
                              hNR->Integral(), hER->Integral())
                      << std::endl;
            delete hNR; delete hER;
            continue;
        }

        // --- Fit Gaussians ---
        // "Q" = quiet (suppress terminal output), "N" = don't draw, "S" = return result
        TFitResultPtr rNR = hNR->Fit("gaus", "QNS");
        TFitResultPtr rER = hER->Fit("gaus", "QNS");

        // --- Guard 3: fit returned a null/empty result pointer ---
        // This happens when Minuit finds no data in the fit range even after the
        // Integral() check (e.g. all entries in a single bin). Dereferencing a null
        // TFitResultPtr segfaults, so check .Get() before any -> access.
        if(!rNR.Get() || !rER.Get())
        {
            std::cout << Form("Bin S1=[%d,%d): fit returned null result pointer, skipping",
                              s1_lo, s1_hi)
                      << std::endl;
            delete hNR; delete hER;
            continue;
        }

        // --- Guard 4: fit did not converge ---
        if(rNR->Status() != 0 || rER->Status() != 0)
        {
            std::cout << Form("Bin S1=[%d,%d): fit did not converge "
                              "(NR status=%d, ER status=%d), skipping",
                              s1_lo, s1_hi, rNR->Status(), rER->Status())
                      << std::endl;
            delete hNR; delete hER;
            continue;
        }

        double muNR    = rNR->Parameter(1);
        double muER    = rER->Parameter(1);
        double sigER   = rER->Parameter(2);
        double sigNR   = rNR->Parameter(2);

        double cut     = muNR + sigNR * TMath::Sqrt2() * TMath::ErfInverse(2.0*NR_ACCEPTANCE - 1.0);
        cut_vals[nBins_used] = cut;
        double z       = (muER - cut) / (TMath::Sqrt2() * sigER);
        double leak    = 0.5 * TMath::Erfc(z);

        // --- Analytical uncertainty via error propagation ---
        double err_muER  = rER->ParError(1);   // fit uncertainty on mu_ER
        double err_sigER = rER->ParError(2);   // fit uncertainty on sigma_ER

        // partial derivatives of z w.r.t. muER and sigER
        double dz_dmuER  = 1.0 / (TMath::Sqrt2() * sigER);
        double dz_dsigER = -z / sigER;   // from quotient rule

        double sigma_z   = TMath::Sqrt(dz_dmuER*dz_dmuER * err_muER*err_muER
                                    + dz_dsigER*dz_dsigER * err_sigER*err_sigER);

        // derivative of 0.5*Erfc(z) w.r.t. z is -exp(-z^2)/sqrt(pi)
        double d_leak_dz = -TMath::Exp(-z*z) / TMath::Sqrt(TMath::Pi());
        double leak_err  = TMath::Abs(d_leak_dz) * sigma_z;


        // --- Direct count leakage cross-check ---
        TString cutER_below = Form("recoilType==0 && S1>=%d && S1<%d && logS2<%f",
                                    s1_lo, s1_hi, cut);
        TString cutER_total = Form("recoilType==0 && S1>=%d && S1<%d", s1_lo, s1_hi);

        double nER_below = chain.GetEntries(cutER_below);
        double nER_total = chain.GetEntries(cutER_total);

        double leak_direct = (nER_total > 0) ? nER_below / nER_total : 0.0;
        double leak_direct_err = (nER_total > 0) ?
            TMath::Sqrt(leak_direct * (1.0 - leak_direct) / nER_total) : 0.0;

        if(nER_below == 0)
        {
            leakage_direct    [nBins_used] = 0.0;  // will be ignored in plot
            leakage_direct_err[nBins_used] = 0.0;
            leakage_direct_upperlimit[nBins_used] = 2.996 / nER_total;
        }
        else
        {
            leakage_direct_upperlimit[nBins_used] = 0.0;  // not an upper limit bin
        }

        // --- Store everything ---
        binCenters        [nBins_used] = s1_center;
        leakage           [nBins_used] = leak;
        leakage_err       [nBins_used] = leak_err;
        leakage_direct    [nBins_used] = leak_direct;
        leakage_direct_err[nBins_used] = leak_direct_err;
        s1_err            [nBins_used] = 0.5 * BIN_WIDTH;
        nER_perbin        [nBins_used] = nER_total;
        mu_NR  [nBins_used] = muNR;
        mu_ER  [nBins_used] = muER;
        sigma_NR[nBins_used] = sigNR;
        sigma_ER[nBins_used] = sigER;
        nBins_used++;

        double nER = hER->GetEntries();
        totalLeakageNumer += leak * nER;
        totalLeakageDenom += nER;

        std::cout << Form("%-10d %-10d %-10.4f %-10.4f %-10.4f %-12.4e %-12.4e %-12.4e %-12.4e",
                        s1_lo, s1_hi, muNR, muER, sigER,
                        leak, leak_err, leak_direct, leak_direct_err)
                << std::endl;

        delete hNR;
        delete hER;
    }
    // ← loop ends here
    std::cout << "logS2 range: ["
          << chain.GetMinimum("logS2") << ", "
          << chain.GetMaximum("logS2") << "]" << std::endl;

    if(nBins_used == 0)
    {
        std::cerr << "No valid S1 bins found for leakage calculation. "
                     "Check LOG_MIN/LOG_MAX range and that logS2 is filled correctly.\n";
        return;
    }

    // --- Total weighted-average leakage (unsmoothed, for reference) ---
    double totalLeakage = (totalLeakageDenom > 0) ?
                           totalLeakageNumer / totalLeakageDenom : 0.0;
    std::cout << "\nTotal ER leakage (unsmoothed, weighted avg): " << totalLeakage << std::endl;

    // ----------------------------------------------------------------
    // Smooth the NR medians with a polynomial, then recompute leakage
    // ----------------------------------------------------------------
    TGraph *gMuNR_raw = new TGraph(nBins_used, binCenters, mu_NR);
    TF1 *fNRsmooth = new TF1("fNRsmooth", "pol3", S1_MIN, S1_MAX);
    gMuNR_raw->Fit("fNRsmooth", "Q");  // "Q" = quiet

    // Recompute leakage array using smoothed NR median
    double leakage_smooth[N_BINS];
    double totalLeakageNumer_smooth = 0.0;
    double totalLeakageDenom_smooth = 0.0;

    std::cout << "\n=== Smoothed Leakage ===" << std::endl;
    std::cout << Form("%-10s %-10s %-10s %-10s %-12s %-12s",
                      "S1_center","mu_NR_raw","mu_NR_fit","mu_ER",
                      "Leak_raw","Leak_smooth") << std::endl;

    for(int i = 0; i < nBins_used; i++)
    {
        double muNR_smooth = fNRsmooth->Eval(binCenters[i]);
        double cut_smooth  = muNR_smooth + sigma_NR[i] * TMath::Sqrt2() * TMath::ErfInverse(2.0*NR_ACCEPTANCE - 1.0);
        double z_smooth    = (mu_ER[i] - cut_smooth) / (TMath::Sqrt2() * sigma_ER[i]);
        leakage_smooth[i]  = 0.5 * TMath::Erfc(z_smooth);

        totalLeakageNumer_smooth += leakage_smooth[i] * nER_perbin[i];
        totalLeakageDenom_smooth += nER_perbin[i];

        std::cout << Form("%-10.1f %-10.4f %-10.4f %-10.4f %-12.4e %-12.4e",
                          binCenters[i], mu_NR[i], muNR_smooth,
                          mu_ER[i], leakage[i], leakage_smooth[i])
                  << std::endl;
    }

    double totalLeakage_smooth = (totalLeakageDenom_smooth > 0) ?
                                  totalLeakageNumer_smooth / totalLeakageDenom_smooth : 0.0;

    std::cout << "\nTotal ER leakage (smoothed, weighted avg): "
              << totalLeakage_smooth << std::endl;

    // ------------------------------------------------------------
    // Plot: leakage vs S1 bin center
    // ------------------------------------------------------------
    TCanvas *cLeak = new TCanvas("cLeak", "ER Leakage vs S1", 800, 600);
    cLeak->SetLogy();

    // Analytical leakage with error bars
    TGraphErrors *gLeak = new TGraphErrors(nBins_used,
                                            binCenters, leakage,
                                            nullptr, leakage_err);
    gLeak->SetTitle(Form("ER Leakage (%.0f%% NR acceptance);S1 [photon hits];Leakage fraction",
                        NR_ACCEPTANCE * 100.0));
    gLeak->SetMarkerStyle(21);
    gLeak->SetMarkerColor(kBlue+1);
    gLeak->SetLineColor(kBlue+1);
    gLeak->Draw("ALP");

    // Direct count leakage with binomial error bars (only nonzero bins)
    TGraphErrors *gLeakDirect = new TGraphErrors(nBins_used,
                                                  binCenters, leakage_direct,
                                                  nullptr, leakage_direct_err);
    gLeakDirect->SetMarkerStyle(20);
    gLeakDirect->SetMarkerColor(kRed+1);
    gLeakDirect->SetLineColor(kRed+1);
    gLeakDirect->Draw("P SAME");  // no L so zero points don't draw a line

    // Upper limits as downward arrows
    // Collect only the bins that are actually upper limits
    std::vector<double> ul_x, ul_y;
    for(int i = 0; i < nBins_used; i++)
    {
        if(leakage_direct_upperlimit[i] > 0.0)
        {
            ul_x.push_back(binCenters[i]);
            ul_y.push_back(leakage_direct_upperlimit[i]);
        }
    }

    TGraph *gUL = new TGraph(ul_x.size(), ul_x.data(), ul_y.data());
    gUL->SetMarkerStyle(23);  // filled downward triangle
    gUL->SetMarkerSize(1.2);
    gUL->SetMarkerColor(kRed+1);
    gUL->Draw("P SAME");

    TLegend *legLeak = new TLegend(0.55, 0.68, 0.88, 0.88);
    legLeak->AddEntry(gLeak,       "Gaussian fit", "LP");
    legLeak->AddEntry(gLeakDirect, "Direct count", "P");
    legLeak->AddEntry(gUL,         "95% CL upper limit", "P");
    legLeak->Draw();

    cLeak->Update();

    // ------------------------------------------------------------
    // Plot: NR and ER band means ± 1σ overlaid on scatter (LZ-style)
    // ------------------------------------------------------------
    double mu_ER_up[N_BINS], mu_ER_dn[N_BINS];
    double mu_NR_up[N_BINS], mu_NR_dn[N_BINS];
    for(int i = 0; i < nBins_used; i++) {
        mu_ER_up[i] = mu_ER[i] + sigma_ER[i];
        mu_ER_dn[i] = mu_ER[i] - sigma_ER[i];
        mu_NR_up[i] = mu_NR[i] + sigma_NR[i];
        mu_NR_dn[i] = mu_NR[i] - sigma_NR[i];
    }

    TCanvas *cBands = new TCanvas("cBands", "Band Means vs S1", 800, 600);

    TH2F *hFrame = new TH2F("hFrame", "NR/ER Discrimination Bands",
                            100, S1_MIN, S1_MAX,
                            100, 1.0, LOG_MAX); //was LOG_MIN instead of 1.0
    hFrame->SetStats(0);
    hFrame->GetXaxis()->SetTitle("S1 [photon hits]");
    hFrame->GetYaxis()->SetTitle("log_{10} S2");
    hFrame->GetYaxis()->SetTitleOffset(1.2);
    hFrame->Draw();

    chain.SetMarkerStyle(20);
    chain.SetMarkerSize(0.3);
    chain.SetMarkerColorAlpha(kCyan+kBlue, 1.0);
    chain.Draw("logS2:S1","recoilType==0","P SAME");
    chain.SetMarkerColorAlpha(kOrange + 7, 1.0);
    chain.Draw("logS2:S1","recoilType==1","P SAME");

    // ER mean (solid) and ±1σ (dashed)
    TGraph *gMuER    = new TGraph(nBins_used, binCenters, mu_ER);
    TGraph *gER_up   = new TGraph(nBins_used, binCenters, mu_ER_up);
    TGraph *gER_dn   = new TGraph(nBins_used, binCenters, mu_ER_dn);

    gMuER->SetLineColor(kBlack);  gMuER->SetLineWidth(5); gMuER->SetLineStyle(1);
    gER_up->SetLineColor(kBlack); gER_up->SetLineWidth(5); gER_up->SetLineStyle(2);
    gER_dn->SetLineColor(kBlack); gER_dn->SetLineWidth(5); gER_dn->SetLineStyle(2);

    // NR mean (solid) and ±1σ (dashed)
    TGraph *gMuNR    = new TGraph(nBins_used, binCenters, mu_NR);
    TGraph *gNR_up   = new TGraph(nBins_used, binCenters, mu_NR_up);
    TGraph *gNR_dn   = new TGraph(nBins_used, binCenters, mu_NR_dn);

    gMuNR->SetLineColor(kBlack);  gMuNR->SetLineWidth(5); gMuNR->SetLineStyle(1);
    gNR_up->SetLineColor(kBlack); gNR_up->SetLineWidth(5); gNR_up->SetLineStyle(2);
    gNR_dn->SetLineColor(kBlack); gNR_dn->SetLineWidth(5); gNR_dn->SetLineStyle(2);

    cBands->cd();
    gER_up->Draw("L SAME");
    gER_dn->Draw("L SAME");
    gMuER->Draw("L SAME");

    gNR_up->Draw("L SAME");
    gNR_dn->Draw("L SAME");
    gMuNR->Draw("L SAME");

    TLegend *legB = new TLegend(0.12, 0.72, 0.50, 0.88);
    legB->AddEntry(gMuER, "ER mean (#mu_{ER})", "L");
    legB->AddEntry(gER_up, "ER #pm1#sigma (16%#minus84%)", "L");
    legB->AddEntry(gMuNR, "NR mean (#mu_{NR})", "L");
    legB->AddEntry(gNR_up, "NR #pm1#sigma (16%#minus84%)", "L");
    legB->Draw();

    cBands->Update();
}




















// #include <TChain.h>
// #include <TFile.h>
// #include <TCanvas.h>
// #include <TH1.h>
// #include <TLegend.h>
// #include <TSystemDirectory.h>
// #include <TList.h>
// #include <TSystemFile.h>
// #include <iostream>

// void graphing(const char* pattern="output")
// {
//     const double NR_ACCEPTANCE = 0.50; //Set NR_acceptance value here

//     // Find all ROOT files beginning with "output"
//     TSystemDirectory dir(".", ".");
//     TList *files = dir.GetListOfFiles();

//     if(!files)
//     {
//         std::cerr<<"No files found\n";
//         return;
//     }

//     TIter next(files);
//     TSystemFile *file;

//     TChain chain("Events");

//     TH1F *hDriftAll = new TH1F("hDriftAll", "Electron Drift Times", 1000, -1, 10);
//     hDriftAll->SetDirectory(0);

//     int canvasIndex = 0;

//     // Loop over files
//     while((file=(TSystemFile*)next()))
//     {
//         TString fname = file->GetName();

//         if(file->IsDirectory()) continue;
//         if(!fname.EndsWith(".root")) continue;
//         if(!fname.BeginsWith(pattern)) continue;

//         std::cout<<"Reading "<<fname<<std::endl;

//         TFile *f = TFile::Open(fname);
//         if(!f || f->IsZombie()) continue;

//         if(f->Get("Events"))
//         {
//             chain.Add(fname);
//         }
//         else
//         {
//             std::cout<<"No Events tree in "<<fname<<std::endl;
//         }

//         // Waveform plotting (independent of chain logic)
//         TH1 *hTime = (TH1*)f->Get("Time");
//         if(!hTime) { f->Close(); continue; }

//         if(hTime->GetEntries() == 0)
//         {
//             std::cout<<"Skipping waveform for "<<fname
//                      <<" (no photon activity)"<<std::endl;
//             continue;
//         }
//         // Accumulate drift time histogram across all files
//         TH1 *hDrift = (TH1*)f->Get("drift times");
//         if(hDrift)
//         {
//             hDrift->SetDirectory(0);
//             hDriftAll->Add(hDrift);
//         }

//         hTime->SetDirectory(0);
//         f->Close();

//         TString cname;
//         cname.Form("cTime_%d",canvasIndex++);

//         //TCanvas *cRun = new TCanvas(cname,fname,900,650); //comment out to not draw waveforms

//         hTime->SetTitle(fname + " : photon hits vs time");
//         hTime->GetXaxis()->SetTitle("time [#mus]");
//         hTime->GetYaxis()->SetTitle("hits");
//         hTime->SetLineWidth(2);
//         hTime->SetLineColor(kBlue+1);

//         //hTime->Draw("HIST"); //comment out to not draw waveforms
//         //cRun->Update(); //comment out to not draw waveforms

//     }

//     // ------------------------------------------------------------
//     // Golden parameter plot
//     // ------------------------------------------------------------
//     Long64_t totalEntries = chain.GetEntries();
//     if(totalEntries == 0)
//     {
//         std::cerr<<"No Events entries found in chain\n";
//         return;
//     }
//     std::cout<<"Total entries in chain: " << totalEntries << std::endl;
//     std::cout<<"ER entries: " << chain.GetEntries("recoilType==0") << std::endl;
//     std::cout<<"NR entries: " << chain.GetEntries("recoilType==1") << std::endl;

//     TCanvas *cGold = new TCanvas("cGold","Golden Parameter",800,600);
//     chain.SetMarkerStyle(20);
//     chain.SetMarkerSize(0.7);

//     // Draw all points first to establish axes
//     chain.Draw("logS2:S1","","P");

//     TH1 *htemp = (TH1*)gPad->GetListOfPrimitives()->FindObject("htemp");
//     if(htemp) {
//         htemp->GetYaxis()->SetTitle("log_{10} S2"); //S2 or S2/S1
//         htemp->GetYaxis()->SetTitleOffset(1.2);
//         gPad->Modified();
//     }

//     // ER band
//     chain.SetMarkerColor(kBlue);
//     chain.Draw("logS2:S1","recoilType==0","P SAME");

//     // NR band
//     chain.SetMarkerColor(kRed);
//     chain.Draw("logS2:S1","recoilType==1","P SAME");

//     // Manual legend with dummy marker objects
//     TLegend *leg = new TLegend(0.7, 0.75, 0.88, 0.88);
//     leg->SetBorderSize(1);

//     TMarker *mER = new TMarker(0, 0, 20);
//     mER->SetMarkerColor(kBlue);
//     mER->SetMarkerSize(0.7);

//     TMarker *mNR = new TMarker(0, 0, 20);
//     mNR->SetMarkerColor(kRed);
//     mNR->SetMarkerSize(0.7);

//     leg->AddEntry(mER, "ER", "P");
//     leg->AddEntry(mNR, "NR", "P");
//     leg->Draw();

//     cGold->Update();
//     std::cout<<"Done\n";

//     // ------------------------------------------------------------
//     // Drift time plot
//     // ------------------------------------------------------------
//     if(hDriftAll->GetEntries() > 0)
//     {
//         TCanvas *cDrift = new TCanvas("cDrift","Drift Times",800,600);
//         hDriftAll->SetLineColor(kBlue+1);
//         hDriftAll->SetLineWidth(2);
//         hDriftAll->GetXaxis()->SetTitle("drift time [#mus]");
//         hDriftAll->GetYaxis()->SetTitle("counts");
//         hDriftAll->Draw("HIST");
//         cDrift->Update();
//     }
//     else
//     {
//         std::cout<<"No drift time histogram found in any file\n";
//     }

//     // ------------------------------------------------------------
//     // ER Leakage Calculation
//     // ------------------------------------------------------------

//     // --- Configuration ---
//     const int    S1_MIN   = 1;
//     const int    S1_MAX   = 160;
//     const int    BIN_WIDTH = 10;    // S1 hits per bin
//     const int    N_BINS   = (S1_MAX - S1_MIN) / BIN_WIDTH;

//     const double LOG_MIN  = -0.2;   // logS2 axis range (match your plot)
//     const double LOG_MAX  =  1.0; // was 1
//     const int    LOG_BINS =  60;    // number of histogram bins for the projection

//     // Arrays to store per-bin results
//     double binCenters[N_BINS], leakage[N_BINS];
//     double mu_NR[N_BINS], sigma_NR[N_BINS];
//     double mu_ER[N_BINS], sigma_ER[N_BINS];
//     int    nBins_used = 0;
//     double nER_perbin[N_BINS];

//     double leakage_err[N_BINS];      // analytical Gaussian uncertainty
//     double leakage_direct[N_BINS];   // direct count leakage
//     double leakage_direct_err[N_BINS]; // binomial uncertainty on direct count
//     double s1_err[N_BINS];           // x error bars = half the bin width
//     double cut_vals[N_BINS];  // actual cut position in logS2
//     double leakage_direct_upperlimit[N_BINS];


//     // Weights for total leakage average
//     double totalLeakageNumer = 0.0;
//     double totalLeakageDenom = 0.0;

//     std::cout << "\n=== Leakage Calculation ===" << std::endl;
//     std::cout << Form("%-10s %-10s %-10s %-10s %-10s %-12s %-12s %-12s %-12s",
//                     "S1_lo","S1_hi","mu_NR","mu_ER","sigma_ER",
//                     "Leak_Gaus","Gaus_err","Leak_Direct","Direct_err")
//             << std::endl;

//     //chain.GetEntry(0);
//     //chain.Print(); // prints all branch names
//     for(int i = 0; i < N_BINS; i++)
//     {
//         int s1_lo = S1_MIN + i * BIN_WIDTH;
//         int s1_hi = s1_lo + BIN_WIDTH;
//         double s1_center = 0.5 * (s1_lo + s1_hi);

//         // --- Build selection strings ---
//         TString cutNR = Form("recoilType==1 && S1>=%d && S1<%d", s1_lo, s1_hi);
//         TString cutER = Form("recoilType==0 && S1>=%d && S1<%d", s1_lo, s1_hi);

//         // --- Create 1D projection histograms ---
//         TString hNR_name = Form("hNR_bin%d", i);
//         TString hER_name = Form("hER_bin%d", i);

//         TH1F *hNR = new TH1F(hNR_name, hNR_name, LOG_BINS, LOG_MIN, LOG_MAX);
//         TH1F *hER = new TH1F(hER_name, hER_name, LOG_BINS, LOG_MIN, LOG_MAX);

//         // Fill from TChain using Draw with >> syntax
//         chain.Project(hNR_name, "logS2", cutNR);
//         chain.Project(hER_name, "logS2", cutER);

//         // Skip bins with too few events to fit reliably
//         if(hNR->GetEntries() < 20 || hER->GetEntries() < 20)
//         {
//             std::cout << Form("Bin S1=[%d,%d): too few events (NR=%lld, ER=%lld), skipping",
//                             s1_lo, s1_hi,
//                             (long long)hNR->GetEntries(),
//                             (long long)hER->GetEntries())
//                     << std::endl;
//             delete hNR; delete hER;
//             continue;
//         }

// // --- Fit Gaussians ---
//         // "Q" = quiet (suppress terminal output), "N" = don't draw, "S" = return result
//         TFitResultPtr rNR = hNR->Fit("gaus", "QNS");
//         TFitResultPtr rER = hER->Fit("gaus", "QNS");

//         // Check fits converged
//         if(rNR->Status() != 0 || rER->Status() != 0)
//         {
//             std::cout << Form("Bin S1=[%d,%d): fit did not converge, skipping",
//                                 s1_lo, s1_hi)
//                       << std::endl;
//             delete hNR; delete hER;
//             continue;
//         }

//         double muNR    = rNR->Parameter(1);
//         double muER    = rER->Parameter(1);
//         double sigER   = rER->Parameter(2);
//         double sigNR   = rNR->Parameter(2);

//         double cut     = muNR + sigNR * TMath::Sqrt2() * TMath::ErfInverse(2.0*NR_ACCEPTANCE - 1.0);
//         cut_vals[nBins_used] = cut;
//         double z       = (muER - cut) / (TMath::Sqrt2() * sigER);
//         double leak    = 0.5 * TMath::Erfc(z);

//         // --- Analytical uncertainty via error propagation ---
//         double err_muER  = rER->ParError(1);   // fit uncertainty on mu_ER
//         double err_sigER = rER->ParError(2);   // fit uncertainty on sigma_ER

//         // partial derivatives of z w.r.t. muER and sigER
//         double dz_dmuER  = 1.0 / (TMath::Sqrt2() * sigER);
//         double dz_dsigER = -z / sigER;   // from quotient rule

//         double sigma_z   = TMath::Sqrt(dz_dmuER*dz_dmuER * err_muER*err_muER
//                                     + dz_dsigER*dz_dsigER * err_sigER*err_sigER);

//         // derivative of 0.5*Erfc(z) w.r.t. z is -exp(-z^2)/sqrt(pi)
//         double d_leak_dz = -TMath::Exp(-z*z) / TMath::Sqrt(TMath::Pi());
//         double leak_err  = TMath::Abs(d_leak_dz) * sigma_z;


//         // --- Direct count leakage cross-check ---
//         TString cutER_below = Form("recoilType==0 && S1>=%d && S1<%d && logS2<%f",
//                                     s1_lo, s1_hi, cut);
//         TString cutER_total = Form("recoilType==0 && S1>=%d && S1<%d", s1_lo, s1_hi);

//         double nER_below = chain.GetEntries(cutER_below);
//         double nER_total = chain.GetEntries(cutER_total);

//         double leak_direct = (nER_total > 0) ? nER_below / nER_total : 0.0;
//         double leak_direct_err = (nER_total > 0) ?
//             TMath::Sqrt(leak_direct * (1.0 - leak_direct) / nER_total) : 0.0;

//         if(nER_below == 0)
//         {
//             leakage_direct    [nBins_used] = 0.0;  // will be ignored in plot
//             leakage_direct_err[nBins_used] = 0.0;
//             leakage_direct_upperlimit[nBins_used] = 2.996 / nER_total;
//         }
//         else
//         {
//             leakage_direct_upperlimit[nBins_used] = 0.0;  // not an upper limit bin
//         }



//         // --- Store everything ---
//         binCenters        [nBins_used] = s1_center;
//         leakage           [nBins_used] = leak;
//         leakage_err       [nBins_used] = leak_err;
//         leakage_direct    [nBins_used] = leak_direct;
//         leakage_direct_err[nBins_used] = leak_direct_err;
//         s1_err            [nBins_used] = 0.5 * BIN_WIDTH;
//         nER_perbin        [nBins_used] = nER_total;
//         mu_NR  [nBins_used] = muNR;
//         mu_ER  [nBins_used] = muER;
//         sigma_NR[nBins_used] = sigNR;
//         sigma_ER[nBins_used] = sigER;
//         nBins_used++;

//         double nER = hER->GetEntries();
//         totalLeakageNumer += leak * nER;
//         totalLeakageDenom += nER;

//         std::cout << Form("%-10d %-10d %-10.4f %-10.4f %-10.4f %-12.4e %-12.4e %-12.4e %-12.4e",
//                         s1_lo, s1_hi, muNR, muER, sigER,
//                         leak, leak_err, leak_direct, leak_direct_err)
//                 << std::endl;

//         delete hNR;
//         delete hER;
//     }
//     // ← loop ends here

//     // --- Total weighted-average leakage (unsmoothed, for reference) ---
//     double totalLeakage = (totalLeakageDenom > 0) ?
//                            totalLeakageNumer / totalLeakageDenom : 0.0;
//     std::cout << "\nTotal ER leakage (unsmoothed, weighted avg): " << totalLeakage << std::endl;

//     // ----------------------------------------------------------------
//     // Smooth the NR medians with a polynomial, then recompute leakage
//     // ----------------------------------------------------------------
//     if(nBins_used == 0) {
//         std::cerr << "No valid bins found, cannot plot leakage.\n";
//         return;
//     }
//     TGraph *gMuNR_raw = new TGraph(nBins_used, binCenters, mu_NR);
//     TF1 *fNRsmooth = new TF1("fNRsmooth", "pol3", S1_MIN, S1_MAX);
//     gMuNR_raw->Fit("fNRsmooth", "Q");  // "Q" = quiet

//     // Recompute leakage array using smoothed NR median
//     double leakage_smooth[N_BINS];
//     double totalLeakageNumer_smooth = 0.0;
//     double totalLeakageDenom_smooth = 0.0;

//     std::cout << "\n=== Smoothed Leakage ===" << std::endl;
//     std::cout << Form("%-10s %-10s %-10s %-10s %-12s %-12s",
//                       "S1_center","mu_NR_raw","mu_NR_fit","mu_ER",
//                       "Leak_raw","Leak_smooth") << std::endl;

//     for(int i = 0; i < nBins_used; i++)
//     {
//         double muNR_smooth = fNRsmooth->Eval(binCenters[i]);
//         double cut_smooth  = muNR_smooth + sigma_NR[i] * TMath::Sqrt2() * TMath::ErfInverse(2.0*NR_ACCEPTANCE - 1.0);
//         double z_smooth    = (mu_ER[i] - cut_smooth) / (TMath::Sqrt2() * sigma_ER[i]);
//         leakage_smooth[i]  = 0.5 * TMath::Erfc(z_smooth);

//         // Weight by number of ER events — approximate using sigma_ER as proxy
//         // (we don't store nER per bin, so reweight by 1 for simplicity,
//         //  or add a nER_perbin array above if you want exact weighting)
//         totalLeakageNumer_smooth += leakage_smooth[i] * nER_perbin[i];
//         totalLeakageDenom_smooth += nER_perbin[i];

//         std::cout << Form("%-10.1f %-10.4f %-10.4f %-10.4f %-12.4e %-12.4e",
//                           binCenters[i], mu_NR[i], muNR_smooth,
//                           mu_ER[i], leakage[i], leakage_smooth[i])
//                   << std::endl;
//     }

//     double totalLeakage_smooth = (totalLeakageDenom_smooth > 0) ?
//                                   totalLeakageNumer_smooth / totalLeakageDenom_smooth : 0.0;

//     std::cout << "\nTotal ER leakage (smoothed, weighted avg): "
//               << totalLeakage_smooth << std::endl;

//     // ------------------------------------------------------------
//     // Plot: leakage vs S1 bin center
//     // ------------------------------------------------------------
//     TCanvas *cLeak = new TCanvas("cLeak", "ER Leakage vs S1", 800, 600);
//     cLeak->SetLogy();

//     // Analytical leakage with error bars
//     TGraphErrors *gLeak = new TGraphErrors(nBins_used,
//                                             binCenters, leakage,
//                                             nullptr, leakage_err);
//     gLeak->SetTitle(Form("ER Leakage (%.0f%% NR acceptance);S1 [photon hits];Leakage fraction",
//                         NR_ACCEPTANCE * 100.0));
//     gLeak->SetMarkerStyle(21);
//     gLeak->SetMarkerColor(kBlue+1);
//     gLeak->SetLineColor(kBlue+1);
//     if(gLeak->GetN() > 0) gLeak->Draw("ALP");

//     // Direct count leakage with binomial error bars (only nonzero bins)
//     TGraphErrors *gLeakDirect = new TGraphErrors(nBins_used,
//                                                   binCenters, leakage_direct,
//                                                   nullptr, leakage_direct_err);
//     gLeakDirect->SetMarkerStyle(20);
//     gLeakDirect->SetMarkerColor(kRed+1);
//     gLeakDirect->SetLineColor(kRed+1);
//     if(gLeakDirect->GetN() > 0) gLeakDirect->Draw("P SAME");  // no L so zero points don't draw a line

//     // Upper limits as downward arrows
//     // Collect only the bins that are actually upper limits
//     std::vector<double> ul_x, ul_y;
//     for(int i = 0; i < nBins_used; i++)
//     {
//         if(leakage_direct_upperlimit[i] > 0.0)
//         {
//             ul_x.push_back(binCenters[i]);
//             ul_y.push_back(leakage_direct_upperlimit[i]);
//         }
//     }

//     TGraph *gUL = new TGraph();
//     gUL->SetMarkerStyle(23);
//     gUL->SetMarkerSize(1.2);
//     gUL->SetMarkerColor(kRed+1);
//     if(ul_x.size() > 0) {
//         gUL = new TGraph(ul_x.size(), ul_x.data(), ul_y.data());
//         gUL->SetMarkerStyle(23);
//         gUL->SetMarkerSize(1.2);
//         gUL->SetMarkerColor(kRed+1);
//         gUL->Draw("P SAME");
//     }

//     TLegend *legLeak = new TLegend(0.55, 0.68, 0.88, 0.88);
//     legLeak->AddEntry(gLeak,       "Gaussian fit", "LP");
//     legLeak->AddEntry(gLeakDirect, "Direct count", "P");
//     legLeak->AddEntry(gUL,         "95% CL upper limit", "P");
//     legLeak->Draw();

//     cLeak->Update();




//     // TCanvas *cLeak = new TCanvas("cLeak", "ER Leakage vs S1", 800, 600);
//     // cLeak->SetLogy();

//     // // Analytical leakage with error bars
//     // TGraphErrors *gLeak = new TGraphErrors(nBins_used,
//     //                                         binCenters, leakage,
//     //                                         s1_err, leakage_err);
//     // gLeak->SetTitle(Form("ER Leakage (%.0f%% NR acceptance);S1 [photon hits];Leakage fraction",
//     //                     NR_ACCEPTANCE * 100.0));
//     // gLeak->SetMarkerStyle(21);
//     // gLeak->SetMarkerColor(kBlue+1);
//     // gLeak->SetLineColor(kBlue+1);
//     // gLeak->Draw("ALP");

//     // // Direct count leakage with binomial error bars
//     // TGraphErrors *gLeakDirect = new TGraphErrors(nBins_used,
//     //                                             binCenters, leakage_direct,
//     //                                             s1_err, leakage_direct_err);
//     // gLeakDirect->SetMarkerStyle(20);
//     // gLeakDirect->SetMarkerColor(kRed+1);
//     // gLeakDirect->SetLineColor(kRed+1);
//     // gLeakDirect->Draw("LP SAME");

//     // TLegend *legLeak = new TLegend(0.55, 0.72, 0.88, 0.88);
//     // legLeak->AddEntry(gLeak,       "Gaussian fit", "LP");
//     // legLeak->AddEntry(gLeakDirect, "Direct count", "LP");
//     // legLeak->Draw();

//     // cLeak->Update();




//     // ------------------------------------------------------------
//     // Plot: band means (mu_NR and mu_ER) overlaid on scatter
//     // (visual sanity check that your fits landed correctly)
//     // ------------------------------------------------------------
//     // ------------------------------------------------------------
//     // Plot: band means (mu_NR and mu_ER) overlaid on scatter
//     // ------------------------------------------------------------
//     TCanvas *cBands = new TCanvas("cBands", "Band Means vs S1", 800, 600);

//     // Draw a dummy TH2F to set the axes exactly how you want
//     TH2F *hFrame = new TH2F("hFrame", "logS2:S1",
//                             100, 0, 160,   // x: S1 range
//                             100, -0.2, 1.5); // y: logS2 range
//     hFrame->SetStats(0);
//     hFrame->GetXaxis()->SetTitle("S1");
//     hFrame->GetYaxis()->SetTitle("log_{10} S2"); //S2 or S2/S1
//     hFrame->GetYaxis()->SetTitleOffset(1.2);
//     hFrame->SetTitle("Band Means vs S1");
//     hFrame->Draw(); // draws axes only, no data

//     // Now draw scatter on top
//     chain.SetMarkerStyle(20);
//     chain.SetMarkerSize(0.4);
//     chain.SetMarkerColor(kBlue);
//     chain.Draw("logS2:S1","recoilType==0","P SAME");
//     chain.SetMarkerColor(kRed);
//     chain.Draw("logS2:S1","recoilType==1","P SAME");

//     // Draw band means
//     TGraph *gMuER = new TGraph(nBins_used, binCenters, mu_ER);
//     TGraph *gMuNR = new TGraph(nBins_used, binCenters, cut_vals);

//     gMuER->SetMarkerStyle(22); gMuER->SetMarkerSize(1.2);
//     gMuER->SetMarkerColor(kCyan+1); gMuER->SetLineColor(kCyan+1); gMuER->SetLineWidth(2);

//     gMuNR->SetMarkerStyle(22); gMuNR->SetMarkerSize(1.2);
//     gMuNR->SetMarkerColor(kBlack); gMuNR->SetLineColor(kBlack); gMuNR->SetLineWidth(2);

//     gMuER->Draw("LP SAME");
//     gMuNR->Draw("LP SAME");

//     TLegend *legB = new TLegend(0.55, 0.72, 0.88, 0.88);
//     legB->AddEntry(gMuER, "ER band mean (#mu_{ER})", "LP");
//     legB->AddEntry(gMuNR, Form("NR cut line (%.0f%% NR acceptance)", NR_ACCEPTANCE * 100.0), "LP");    legB->Draw();
//     cBands->Update();
//     gApplication->Run(kTRUE);
// }
