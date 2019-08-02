#include <string>
#include <iostream>
#include <vector>
#include <algorithm> 

using namespace std;

const string newname = "/eos/project/d/dshep/TOPCLASS/DijetAnomaly/ZprimeDark_2000GeV_13TeV_PU40/ZprimeDark_2000GeV_13TeV_PU40_1_ap.root";
const string oldname = "/eos/project/d/dshep/TOPCLASS/DijetAnomaly/ZprimeDark_2000GeV_13TeV_PU40/ZprimeDark_2000GeV_13TeV_PU40_0.root";

TCanvas *PlotTwo(TTree* tree1, TTree* tree2, string name, int bins) {

    float min1 = tree1->GetMinimum(name.c_str());
    float min2 = tree2->GetMinimum(name.c_str()); 

    float max1 = tree1->GetMaximum(name.c_str());
    float max2 = tree1->GetMaximum(name.c_str());

    float min = std::min(min1, min2);
    float max = std::max(max1, max2); 

    TH1F * h1 = new TH1F("h1", "h1", bins, min, max);
    TH1F * h2 = new TH1F("h2", "h2", bins, min, max);

    tree1->Draw((name + ">>h1").c_str(), "Jet.PT>0", "goff");
    tree2->Draw((name + ">>h2").c_str(), "Jet.PT>0", "goff");

    TCanvas *cst = new TCanvas("cst");
    auto legend = new TLegend(0.6, 0.7, .95, .92);

    THStack *hs = new THStack("hs", name.c_str()); 

    h1->SetMarkerStyle(21);
    h1->SetLineColor(kBlue);
    h1->SetLineStyle(1); 

    h2->SetMarkerStyle(21);
    h2->SetLineColor(kRed);
    h2->SetLineStyle(1); 


    hs->Add(h1);
    hs->Add(h2);

    cst->cd();
    hs->Draw("nostack"); 

    legend->SetHeader("Legend Title","C"); 
    legend->AddEntry(h1, "old signal", "f"); 
    legend->AddEntry(h2, "new signal", "f"); 
    legend->Draw(); 

    return cst; 
} 

void compare(string f1name, string f2name, vector<string> names, string treename="Delphes", int bins=100) {

    TFile * f1 = new TFile(f1name.c_str());
    TFile * f2 = new TFile(f2name.c_str());

    TTree * tree1 = (TTree*)f1->Get(treename.c_str());
    TTree * tree2 = (TTree*)f2->Get(treename.c_str());

    for (size_t i = 0; i < names.size(); ++i) {
        string name = names[i]; 
        TImage *img = TImage::Create();
        TCanvas* canvas = PlotTwo(tree1, tree2, name, bins); 
        img->FromPad(canvas);
        string outpath = name + ".png"; 
        cout << "saving image to file '" << outpath << "'" << endl;
        img->WriteImage(outpath.c_str()); 
        delete img;
    }
}