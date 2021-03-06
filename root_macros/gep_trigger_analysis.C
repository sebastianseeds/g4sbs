#include "TFile.h"
#include "TTree.h"
#include "TChain.h"
#include "gep_tree.C"
#include "G4SBSRunData.hh"
#include "TRandom3.h"
#include "TChainElement.h"
#include "TObjArray.h"
#include "TString.h"
#include "TObjString.h"
//#include "TIter.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TMath.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>

using namespace std;

void gep_trigger_analysis( const char *rootfilename, const char *logicfilename_ecal, const char *logicfilename_hcal, const char *outputfilename, int pythia6flag=1 ){
  TFile *fout = new TFile(outputfilename,"RECREATE");
  TChain *C = new TChain("T");
  C->Add(rootfilename);

  gep_tree *T = new gep_tree( C );

  G4SBSRunData *rd;

  long ngen = 0;
  int nfiles = 0;
  
  TObjArray *FileList = C->GetListOfFiles();
  TIter next(FileList);

  TChainElement *chEl = 0;

  set<TString> bad_file_list;
  
  while( (chEl=(TChainElement*)next() )){
    TFile newfile(chEl->GetTitle());
    newfile.GetObject("run_data",rd);
    if( rd ){
      ngen += rd->fNtries;
      nfiles++;
    } else {
      bad_file_list.insert( chEl->GetTitle());
    }
  }

  cout << "number of generated events = " << ngen << endl;
  
  set<int> list_of_nodes_ecal;
  map<int, set<int> > cells_logic_sums_ecal; //mapping between node numbers and cell numbers
  map<int, double> logic_mean_ecal; //mean peak positions by node number
  map<int, double> logic_sigma_ecal; //peak width by node number
  map<int, double> threshold_ecal; //threshold by node number
  map<std::pair<int,int>, int > cell_rowcol_ecal; //cell numbers mapped by unique row and column pairs
  map<int,set<int> > nodes_cells_ecal; //mapping of nodes by cell number:
  
  ifstream logicfile_ecal(logicfilename_ecal);
  //ifstream thresholdfile(thresholdfilename);

  TString currentline;
  
  int current_node = 1;

  bool first_cell = true;
  
  while( currentline.ReadLine( logicfile_ecal ) ){
    if( !currentline.BeginsWith( "#" ) ){
      
      
      TObjArray *tokens = currentline.Tokenize(" ");
      int ntokens = tokens->GetEntries();
      if( ntokens >= 11 ){
	cout << currentline.Data() << ", ntokens = " << ntokens << endl;
	
	TString snode = ( (TObjString*) (*tokens)[0] )->GetString();
	int nodenumber = snode.Atoi();
	
	TString scell = ( (TObjString*) (*tokens)[1] )->GetString();
	int cellnumber = scell.Atoi();
	
	TString speakpos = ( (TObjString*) (*tokens)[8] )->GetString();
	double mean = speakpos.Atof();
	
	TString ssigma = ( (TObjString*) (*tokens)[9] )->GetString();
	double sigma = ssigma.Atof();

	TString sthreshold = ( (TObjString*) (*tokens)[10] )->GetString();
	double threshold = sthreshold.Atof();

	TString srow = ( (TObjString*) (*tokens)[2] )->GetString();
	TString scol = ( (TObjString*) (*tokens)[3] )->GetString();

	std::pair<int,int> rowcoltemp( srow.Atoi(), scol.Atoi() );

	cell_rowcol_ecal[rowcoltemp] = cellnumber;
	
	list_of_nodes_ecal.insert( nodenumber );

	cells_logic_sums_ecal[nodenumber].insert( cellnumber );

	logic_mean_ecal[nodenumber] = mean;
	logic_sigma_ecal[nodenumber] = sigma;
	threshold_ecal[nodenumber] = threshold;

	nodes_cells_ecal[ cellnumber ].insert(nodenumber);
	
	
      }
    }
  }

  set<int> list_of_nodes_hcal;
  map<int, set<int> > cells_logic_sums_hcal; //mapping between node numbers and cell numbers
  map<int, double> logic_mean_hcal; //mean peak positions by node number
  map<int, double> logic_sigma_hcal; //peak width by node number
  map<int, double> threshold_hcal; //threshold by node number
  map<std::pair<int,int>, int > cell_rowcol_hcal; //cell numbers mapped by unique row and column pairs
  map<int,set<int> > nodes_cells_hcal; //mapping of nodes by cell number:

  ifstream logicfile_hcal(logicfilename_hcal);

  current_node = 1;
  //  bool first_cell = true;

  while( currentline.ReadLine(logicfile_hcal) ){
    if( !currentline.BeginsWith("#") ){
      TObjArray *tokens = currentline.Tokenize(" ");
      int ntokens = tokens->GetEntries();
      if( ntokens >= 11 ){
	cout << currentline.Data() << ", ntokens = " << ntokens << endl;

	TString snode = ( (TObjString*) (*tokens)[0] )->GetString();
	int nodenumber = snode.Atoi();
	
	TString scell = ( (TObjString*) (*tokens)[1] )->GetString();
	int cellnumber = scell.Atoi();
	
	TString speakpos = ( (TObjString*) (*tokens)[8] )->GetString();
	double mean = speakpos.Atof();
	
	TString ssigma = ( (TObjString*) (*tokens)[9] )->GetString();
	double sigma = ssigma.Atof();

	TString sthreshold = ( (TObjString*) (*tokens)[10] )->GetString();
	double threshold = sthreshold.Atof();

	TString srow = ( (TObjString*) (*tokens)[2] )->GetString();
	TString scol = ( (TObjString*) (*tokens)[3] )->GetString();

	std::pair<int,int> rowcoltemp( srow.Atoi(), scol.Atoi() );

	cell_rowcol_hcal[rowcoltemp] = cellnumber;
	
	list_of_nodes_hcal.insert( nodenumber );

	cells_logic_sums_hcal[nodenumber].insert( cellnumber );

	logic_mean_hcal[nodenumber] = mean;
	logic_sigma_hcal[nodenumber] = sigma;
	threshold_hcal[nodenumber] = threshold;

	nodes_cells_hcal[ cellnumber ].insert(nodenumber);
	
      }
    }
  }
  
  TH1D::SetDefaultSumw2();

  double PI = TMath::Pi();

  //Photoelectron statistics:
  double phe_per_GeV_ECAL = 1000.0/1.33; //~ 750 pe/GeV
  double phe_per_GeV_HCAL = 1000.0/0.30; //~ 3,333 pe/GeV (but sampling fraction is small)

  fout->cd();

  TH1D *hrate_vs_threshold_ECAL = new TH1D("hrate_vs_threshold_ECAL","",30,0.0,1.5);
  //TH1D *hnum_logic_sums_fired_vs_threshold = new TH1D("hnum_logic_sums_fired_vs_threshold

  TH1D *hrate_vs_threshold_HCAL = new TH1D("hrate_vs_threshold_HCAL","",40,0.0,2.0);

  //TH2D *htrue_coincidence_rate_vs_threshold_ECAL_HCAL = new TH2D("htrue_coincidence_rate_vs_threshold_ECAL_HCAL","",40,0,2.0,30,0,1.5);
  
  TH2D *hnphesum_vs_node_ECAL = new TH2D("hnphesum_vs_node_ECAL","",list_of_nodes_ecal.size(),0.5,list_of_nodes_ecal.size()+0.5,100,0.0,5000.0);
  TH2D *hnphesum_vs_node_HCAL = new TH2D("hnphesum_vs_node_HCAL","",list_of_nodes_hcal.size(),0.5,list_of_nodes_hcal.size()+0.5,100,0.0,3500.0);

  TH2D *hmaxnode_ECAL_vs_HCAL = new TH2D("hmaxnode_ECAL_vs_HCAL","",list_of_nodes_hcal.size(),0.5,list_of_nodes_hcal.size()+0.5,list_of_nodes_ecal.size(),0.5,list_of_nodes_ecal.size()+0.5);
  TH2D *hallnodes_ECAL_vs_HCAL = new TH2D("hallnodes_ECAL_vs_HCAL","",list_of_nodes_hcal.size(),0.5,list_of_nodes_hcal.size()+0.5,list_of_nodes_ecal.size(),0.5,list_of_nodes_ecal.size()+0.5);
  //TH2D *hallnodes_ECAL_vs_HCAL = new TH2D("hallnodes_ECAL_vs_HCAL","",list_of_nodes_hcal.size(),0.5,list_of_nodes_hcal.size()+0.5,list_of_nodes_ecal.size(),0.5,list_of_nodes_ecal.size()+0.5);

  
  
  double Ibeam = 75.0e-6; //Amps
  double Ltarget = 40.0; //cm
  double e = 1.602e-19; //electron charge;
  double rho_target = 0.072; //g/cm^3
  double N_A = 6.022e23; //atoms/mol:
  double Mmol_H = 1.008; //g/mol
  double Lumi = rho_target * Ltarget * N_A / Mmol_H * Ibeam/e; //~ 8e38;
  
  TRandom3 num(0);

  cout << "Entering event loop " << endl;
  
  long nevent=0;
  for( nevent=0; nevent<C->GetEntries(); ++nevent ){
    T->GetEntry(nevent);

    double nu = T->ev_Q2 / 2.0 / 0.938272;
    double pp_elastic = sqrt(pow(nu,2)+2.0*.938272*nu);
    
    double weight;
    //cross section is given in mb: 1 mb = 1e-3 * 1e-24 = 1e-27 cm^2
    if (pythia6flag != 0 ){
      weight = Lumi * T->primaries_Sigma * 1.0e-27/ double(ngen); //luminosity times cross section / number of events generated.
    } else {
      weight = T->ev_rate / double(nfiles);
    }
      
    if( (nevent+1) % 1000 == 0 ){ cout << "Event number " << nevent+1 << ", event weight = " << weight << endl; }
    
    map<int,double> node_sums; //initialize all node sums to zero:
    for( set<int>::iterator inode = list_of_nodes_ecal.begin(); inode != list_of_nodes_ecal.end(); ++inode ){
      node_sums[ *inode ] = 0.0;
    }

    for( int ihit = 0; ihit<T->Earm_ECalTF1_hit_nhits; ihit++ ){
      int rowhit = ( *(T->Earm_ECalTF1_hit_row))[ihit]+1;
      int colhit = ( *(T->Earm_ECalTF1_hit_col))[ihit]+1;
      std::pair<int,int> rowcolhit( rowhit,colhit );

      int cellhit = cell_rowcol_ecal[rowcolhit];

      //int trigger_group = nodes_cells_ecal[cellhit];
      
      double edep = (*(T->Earm_ECalTF1_hit_sumedep))[ihit];

      int nphe = num.Poisson( phe_per_GeV_ECAL * edep );

      for( set<int>::iterator inode = nodes_cells_ecal[cellhit].begin(); inode != nodes_cells_ecal[cellhit].end(); ++inode ){
	node_sums[ *inode ] += double(nphe);
      }
      
      //node_sums[ trigger_group ] += double(nphe);
    }

    vector<int> trigger_nodes_fired(hrate_vs_threshold_ECAL->GetNbinsX());
    for( int ithr=0; ithr<hrate_vs_threshold_ECAL->GetNbinsX(); ithr++ ){
      trigger_nodes_fired[ithr] = 0;
    }

    int maxnode_ECAL=-1;
    int maxnode_HCAL=-1;
    double maxsum_ECAL = 0.0;
    double maxsum_HCAL = 0.0;
    
    for( set<int>::iterator inode = list_of_nodes_ecal.begin(); inode != list_of_nodes_ecal.end(); ++inode ){
      for( int bin=1; bin<=hrate_vs_threshold_ECAL->GetNbinsX(); bin++ ){
	if( node_sums[*inode]/logic_mean_ecal[*inode] > hrate_vs_threshold_ECAL->GetBinLowEdge(bin) ){
	  //cout << "node above threshold, nphe, peak position = " << node_sums[*inode] << ", " << logic_mean_ecal[*inode] << endl;
	  trigger_nodes_fired[bin-1]++;
	}
	
      }
      if( node_sums[*inode] > maxsum_ECAL ) {
	maxsum_ECAL = node_sums[*inode];
	maxnode_ECAL = *inode;
      }
      
      if( node_sums[*inode] > 0.0 ) hnphesum_vs_node_ECAL->Fill( *inode, node_sums[*inode], weight );
    }
    for( int ithr=0; ithr<hrate_vs_threshold_ECAL->GetNbinsX(); ithr++ ){
      if( trigger_nodes_fired[ithr] > 0 ) hrate_vs_threshold_ECAL->Fill( hrate_vs_threshold_ECAL->GetBinCenter(ithr+1), weight );
    }

    map<int,double> node_sums_hcal;
    for( set<int>::iterator inode = list_of_nodes_hcal.begin(); inode != list_of_nodes_hcal.end(); ++inode ){
      node_sums_hcal[*inode] = 0.0;
    }
    
    for( int ihit=0; ihit<T->Harm_HCalScint_hit_nhits; ihit++ ){
      int rowhit = (*(T->Harm_HCalScint_hit_row))[ihit]+1;
      int colhit = (*(T->Harm_HCalScint_hit_col))[ihit]+1;
      std::pair<int,int> rowcolhit(rowhit,colhit);
      int cellhit = cell_rowcol_hcal[rowcolhit];
      //int trigger_group = nodes_cells_hcal[cellhit];
      double edep = (*(T->Harm_HCalScint_hit_sumedep))[ihit];
      int nphe = num.Poisson( phe_per_GeV_HCAL * edep );
      //cout << "HCAL hit " << ihit+1 << " node, edep, nphe = " << trigger_group << ", " << edep << ", " << nphe << endl;
      //node_sums_hcal[trigger_group] += double(nphe);
      for( set<int>::iterator inode = nodes_cells_hcal[cellhit].begin(); inode != nodes_cells_hcal[cellhit].end(); ++inode ){
	
	node_sums_hcal[*inode] += double(nphe);
	
      }
    }
    
    vector<int> trigger_nodes_fired_hcal(hrate_vs_threshold_HCAL->GetNbinsX());
    for( int ithr=0; ithr<hrate_vs_threshold_HCAL->GetNbinsX(); ithr++ ){
      trigger_nodes_fired_hcal[ithr] = 0;
    }
    
    for( set<int>::iterator inode = list_of_nodes_hcal.begin(); inode != list_of_nodes_hcal.end(); ++inode ){
      for( int bin=1; bin<=hrate_vs_threshold_HCAL->GetNbinsX(); bin++ ){
	if( node_sums_hcal[*inode]/logic_mean_hcal[*inode] > hrate_vs_threshold_HCAL->GetBinLowEdge(bin) ){
	  trigger_nodes_fired_hcal[bin-1]++;
	}
      }
      if( node_sums_hcal[*inode] > maxsum_HCAL ) {
	maxsum_HCAL = node_sums_hcal[*inode];
	maxnode_HCAL = *inode;
      }
	
      if( node_sums_hcal[ *inode ] > 0.0 ) hnphesum_vs_node_HCAL->Fill( *inode, node_sums_hcal[*inode], weight );
    }
    for( int ithr=0; ithr<hrate_vs_threshold_HCAL->GetNbinsX(); ithr++ ){
      if( trigger_nodes_fired_hcal[ithr] > 0 ) hrate_vs_threshold_HCAL->Fill( hrate_vs_threshold_HCAL->GetBinCenter(ithr+1),weight );
      for( int jthr=0; jthr<hrate_vs_threshold_ECAL->GetNbinsX(); jthr++ ){
	if( trigger_nodes_fired[jthr] > 0 && trigger_nodes_fired_hcal[ithr] > 0 ){
	  htrue_coincidence_rate_vs_threshold_ECAL_HCAL->Fill( hrate_vs_threshold_HCAL->GetBinCenter(ithr+1),hrate_vs_threshold_ECAL->GetBinCenter(jthr+1),weight );
	}
      }
    }

    for( set<int>::iterator inode = list_of_nodes_ecal.begin(); inode != list_of_nodes_ecal.end(); ++inode ){
      for( set<int>::iterator jnode = list_of_nodes_hcal.begin(); jnode != list_of_nodes_hcal.end(); ++jnode ){
	//Fill the correlation histogram for all true coincidence events for which ECAL and HCAL node are both above threshold:
	if( node_sums[*inode] >= nominal_threshold_ECAL*logic_mean_ecal[*inode] && node_sums_hcal[*jnode] >= nominal_threshold_HCAL*logic_mean_hcal[*jnode] ){
	  hallnodes_ECAL_vs_HCAL->Fill( *jnode, *inode, weight );
	}
      }
    }
    if( maxsum_ECAL >= nominal_threshold_ECAL*logic_mean_ecal[maxnode_ECAL] && maxsum_HCAL >= nominal_threshold_HCAL*logic_mean_hcal[maxnode_HCAL] ){
      hmaxnode_ECAL_vs_HCAL->Fill( maxnode_HCAL, maxnode_ECAL, weight );
    }
    //}
  }
 
  // TCanvas *c1 = new TCanvas("c1","c1",1200,900);
  
  // c1->Divide(2,1);

  // c1->cd(1)->SetLogy();
  // hrate_vs_threshold_ECAL->SetMarkerStyle(20);
  // hrate_vs_threshold_ECAL->Draw();
  
  // c1->cd(2)->SetLogy();
  // hrate_vs_threshold_HCAL->SetMarkerStyle(20);
  // hrate_vs_threshold_HCAL->Draw();

  hrate_vs_threshold_HCAL->SetMarkerStyle(20);
  hrate_vs_threshold_ECAL->SetMarkerStyle(20);
  
  fout->Write();
  fout->Close();
}
