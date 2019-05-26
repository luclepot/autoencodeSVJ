#include "TChain.h"
#include "TLeaf.h"
#include "TLorentzVector.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <string>
#include <cmath>
#include <cassert>
#include <string>
#include "TFileCollection.h"
#include "THashList.h"
#include "TBenchmark.h"
#include <iostream>
#include <sstream> 
#include <utility>
#include <map>
#include <cassert>
#include <chrono>

using std::fabs;
using std::chrono::microseconds;  
using std::chrono::duration_cast;
using std::string;
using std::endl;
using std::cout;
using std::vector;
using std::pair; 
using std::to_string;
using std::stringstream; 

namespace vectorTypes{
    enum vectorType {
        Lorentz,
        Mock,
        Map
    };
};

namespace Cuts {
    enum CutType {
        leptonCounts,
        jetCounts,
        jetEtas,
        jetDeltaEtas,
        metRatio,
        jetPt,
        jetDiJet,
        metValue,
        preselection,
        metRatioTight,
        selection,
        COUNT
    };
};

// import for backportability (;-<)
using namespace vectorTypes; 
using namespace Cuts; 

class SVJFinder {
public:
    /// CON/DESTRUCTORS
    ///

        // constructor, requires argv as input
        SVJFinder(char **argv, bool _debug=false, bool _timing=true) {
            start();
            cout << endl;
            timing = _timing;
            log("-----------------------------------");
            log(":          SVJAnalysis            :");
            log("-----------------------------------");
            this->init_vars(argv);
            log("SVJ object created");
            end();
            logt();
            log();
            debug = _debug; 
        }

        // destructor for dynamically allocated data
        ~SVJFinder() {
            start(); 

            Debug(true); 
            log();
            logp("Quitting; cleaning up class variables...  ");
            DelVector(varValues);
            DelVector(vectorVarValues);
            DelVector(LorentzVectors);
            DelVector(MockVectors);
            DelVector(MapVectors);
            logr("Success");
            end();
            logt();
            
            log(); 
            // DelVector(varLeaves);
            // DelVector(vectorVarLeaves);
            // for (vector<TLeaf*> vec : compVectors) 
            //     DelVector(vec);
        }

    /// FILE HANDLERS
    ///

        // sets up tfile collection and returns a pointer to it
        TFileCollection *MakeFileCollection() {
            start();
            log("Loading File Collection from " + path);
            if (fc)
                delete fc;
            fc = new TFileCollection(sample.c_str(), sample.c_str(), path.c_str());
            log("Success: Loaded " + std::to_string(fc->GetNFiles())  + " file(s).");
            end();
            logt();
            log();
            return fc;
        }

        // sets up tchain and returns a pointer to it
        TChain *MakeChain() {
            start();
            log("Creating file chain with tree type '" + treename + "'...");
            if (chain)  
                delete chain;
            chain = new TChain(TString(treename));
            chain->AddFileInfoList(fc->GetList());
            nEvents = (Int_t)chain->GetEntries();
            log("Success");
            end();
            logt();
            log();
            return chain;
        }

    /// VARIABLE TRACKER FUNCTIONS
    ///

        // creates, assigns, and returns tlorentz vector pointer to be updated on GetEntry
        vector<TLorentzVector>* AddLorentz(string vectorName, vector<string> components) {
            start();
            assert(components.size() == 4);
            AddCompsBase(vectorName, components);
            size_t i = LorentzVectors.size();
            subIndex.push_back(std::make_pair(i, vectorType::Lorentz));
            vector<TLorentzVector>* ret = new vector<TLorentzVector>; 
            LorentzVectors.push_back(ret);
            logr("Success");
            end();
            logt();
            return ret;
        }

        // creates, assigns, and returns mock tlorentz vector pointer to be updated on GetEntry
        vector<TLorentzMock>* AddLorentzMock(string vectorName, vector<string> components) {
            start();
            assert(components.size() > 1 && components.size() < 5);
            AddCompsBase(vectorName, components);
            size_t i = MockVectors.size();
            subIndex.push_back(std::make_pair(i, vectorType::Mock));
            vector<TLorentzMock>* ret = new vector<TLorentzMock>; 
            MockVectors.push_back(ret);
            logr("Success");
            end();
            logt();
            return ret;
        }

        // creates, assigns, and returns general double vector pointer to be updated on GetEntry
        vector<vector<double>>* AddComps(string vectorName, vector<string> components) {
            start(); 
            AddCompsBase(vectorName, components);
            size_t i = MapVectors.size();
            subIndex.push_back(std::make_pair(i, vectorType::Map));
            vector<vector<double>>* ret = new vector<vector<double>>;
            MapVectors.push_back(ret);
            logr("Success");
            end();
            logt();
            return ret;
        }

        // creates, assigns, and returns a vectorized single variable pointer to be updates on GetEntry
        vector<double>* AddVectorVar(string vectorVarName, string component) {
            start();
            logp("Adding 1 component to vector var " + vectorVarName + "...  ");
            int i = int(vectorVarValues.size());
            vectorVarIndex[vectorVarName] = i;
            vectorVarLeaves.push_back(chain->FindLeaf(TString(component)));
            vector<double>* ret = new vector<double>;
            vectorVarValues.push_back(ret);
            logr("Success");
            end();
            logt();
            // log(vectorVarIndex.size());
            // log(vectorVarValues.back().size());
            // log(i);
            return ret;
        }

        // creates, assigns, and returns a singular double variable pointer to update on GetEntry 
        double* AddVar(string varName, string component) {
            start();
            logp("Adding 1 component to var " + varName + "...  ");
            size_t i = varLeaves.size();
            varIndex[varName] = i;
            double* ret = new double;
            varLeaves.push_back(chain->FindLeaf(TString(component)));
            varValues.push_back(ret);
            logr("Success");
            end();
            logt(); 
            return ret;
        }

    /// ENTRY LOADING
    ///

        // get the ith entry of the TChain
        void GetEntry(int entry = 0) {
            assert(entry < chain->GetEntries());
            logp("Getting entry " + to_string(entry) + "...  ");
            chain->GetEntry(entry);
            for (size_t i = 0; i < subIndex.size(); ++i) {
                switch(subIndex[i].second) {
                    case vectorType::Lorentz: {
                        SetLorentz(i, subIndex[i].first);
                        break;
                    }
                    case vectorType::Mock: {
                        SetMock(i, subIndex[i].first);
                        break;
                    }
                    case vectorType::Map: {
                        SetMap(i, subIndex[i].first);
                        break;
                    }
                }
            }

            for (size_t i = 0; i < varValues.size(); ++i) {
                SetVar(i);
            }

            for (size_t i = 0; i < vectorVarValues.size(); ++i) {
                SetVectorVar(i);
            }
            // cout << vectorVarValues.size() << endl;
            // for (size_t i = 0; i < vectorVarValues.size(); ++i) {
            //     cout << i << " | "; 
            //     for (size_t j = 0; j < vectorVarValues[i].size(); ++j) {
            //         cout << j << ": " << vectorVarValues[i][j] << ", ";
            //     }
            //     cout << endl; 
            // }
            logr("Success");
        }

        // get the number of entries in the TChain
        Int_t GetEntries() {
            return nEvents;  
        }

    /// CUTS
    ///

        void Cut(bool expression, Cuts::CutType cutName) {
            cutValues[cutName] = expression ? 1 : 0;
        }

        bool Cut(Cuts::CutType cutName) {
            return cutValues[cutName]; 
        }

        bool CutsRange(int start, int end) {
            return std::all_of(cutValues.begin() + start, cutValues.begin() + end, [](int i){return i > 0;});
        }

        void InitCuts() {
            std::fill(cutValues.begin(), cutValues.end(), -1);
        }

        void PrintCuts() {
            print(&cutValues);
        }


    /// SWITCHES, TIMING, AND LOGGING
    ///

        // Turn on or off debug logging with this switch
        void Debug(bool debugSwitch) {
            debug = debugSwitch;
        }

        // turn on or off timing logs with this switch (dependent of debug=true)
        void Timing(bool timingSwitch) {
            timing=timingSwitch;
        }

        // prints a summary of the current entry
        void Current() {
            log();
            if (varIndex.size() > 0) {
                log();
                print("SINGLE VARIABLES:");
            }
            for (auto it = varIndex.begin(); it != varIndex.end(); it++) {
                print(it->first, 1);
                print(varValues[it->second], 2);
            }
            if (vectorVarIndex.size() > 0) {
                log();
                print("VECTOR VARIABLES:");
            }
            for (auto it = vectorVarIndex.begin(); it != vectorVarIndex.end(); it++) {
                print(it->first, 1);
                print(varValues[it->second], 2);
            }
            if (MapVectors.size() > 0) {
                log();
                print("MAP VECTORS:");
            }
            for (auto it = compIndex.begin(); it != compIndex.end(); it++) {
                if (subIndex[it->second].second == vectorType::Map) {
                    print(it->first, 1);
                    print(MapVectors[subIndex[it->second].first], 2);
                }
            }
            if (MockVectors.size() > 0) {
                log();
                print("MOCK VECTORS:");
            }
            for (auto it = compIndex.begin(); it != compIndex.end(); it++) {
                if (subIndex[it->second].second == vectorType::Mock) {
                    print(it->first, 1);
                    print(MockVectors[subIndex[it->second].first], 2);
                }
            }
            if (LorentzVectors.size() > 0) {
                log();
                print("TLORENTZ VECTORS:");
            }
            for (auto it = compIndex.begin(); it != compIndex.end(); it++) {
                if (subIndex[it->second].second == vectorType::Lorentz) {
                    print(it->first, 1);
                    print(LorentzVectors[subIndex[it->second].first], 2);
                }
            }
            log(); 
            log();
        }

        // time of last call, in seconds
        double ts() {
            return duration/1000000.; 
        }

        // '', in milliseconds
        double tms() {
            return duration/1000.;
        }

        // '', in microseconds
        double tus() {
            return duration; 
        }

        // log the time! of the last call
        void logt() {
            if (timing)
                log("(execution time: " + to_string(ts()) + "s)");             
        }

        // internal timer start
        void start() {
            timestart = std::chrono::high_resolution_clock::now(); 
        }

        // internal timer end
        void end() {
            duration = duration_cast<microseconds>(std::chrono::high_resolution_clock::now() - timestart).count();
        }


    /// PUBLIC DATA
    ///
        // general init vars, parsed from argv
        string sample, path, outdir, treename;

        // number of events
        Int_t nEvents;
        // internal debug switch
        bool debug=true, timing=true;
                    
private:
    /// CON/DESTRUCTOR HELPERS
    ///
        template<typename t>
        void DelVector(vector<t*> &v) {
            for (size_t i = 0; i < v.size(); ++i) {
                delete v[i];
                v[i] = nullptr;
            }
        }

        void init_vars(char **argv) {
            log("Starting");

            sample = argv[1];
            log(string("sample: " + sample)); 

            path = argv[2];
            log(string("File list to open: " + path));

            outdir = argv[6];
            log(string("Output directory: " + outdir)); 

            treename = argv[8];
            log(string("Tree name: " + treename));
        }

    /// VARIABLE TRACKER HELPERS
    /// 
    
        void AddCompsBase(string& vectorName, vector<string>& components) {
            if(compIndex.find(vectorName) != compIndex.end())
                throw "Vector variable '" + vectorName + "' already exists!"; 
            size_t index = compIndex.size();
            logp("Adding " + to_string(components.size()) + " components to vector " + vectorName + "...  "); 
            compVectors.push_back(vector<TLeaf*>());
            compNames.push_back(vector<string>());

            for (size_t i = 0; i < components.size(); ++i) {
                compVectors[index].push_back(chain->FindLeaf(components[i].c_str()));
                compNames[index].push_back(lastWord(components[i]));
            }
            compIndex[vectorName] = index;
        }

    /// ENTRY LOADER HELPERS
    /// 

        void SetLorentz(size_t leafIndex, size_t lvIndex) {
            vector<TLeaf*> & v = compVectors[leafIndex];
            vector<TLorentzVector> * ret = LorentzVectors[lvIndex];
            ret->clear();

            size_t n = v[0]->GetLen(); 
            for (size_t i = 0; i < n; ++i) {
                ret->push_back(TLorentzVector());
                ret->at(i).SetPtEtaPhiM(
                    v[0]->GetValue(i),
                    v[1]->GetValue(i),
                    v[2]->GetValue(i),
                    v[3]->GetValue(i)
                );
            }
        }

        void SetMock(size_t leafIndex, size_t mvIndex) {
            vector<TLeaf*> & v = compVectors[leafIndex];
            vector<TLorentzMock>* ret = MockVectors[mvIndex];            
            ret->clear();

            size_t n = v[0]->GetLen(), size = v.size();
            for(size_t i = 0; i < n; ++i) {
                switch(size) {
                    case 2: {
                        ret->push_back(TLorentzMock(v[0]->GetValue(i), v[1]->GetValue(i)));
                        break;
                    }
                    case 3: {
                        ret->push_back(TLorentzMock(v[0]->GetValue(i), v[1]->GetValue(i), v[2]->GetValue(i)));
                        break;
                    }
                    case 4: {
                        ret->push_back(TLorentzMock(v[0]->GetValue(i), v[1]->GetValue(i), v[2]->GetValue(i), v[3]->GetValue(i)));
                        break;
                    }
                    default: {
                        throw "Invalid number arguments for MockTLorentz vector (" + to_string(size) + ")";
                    }                   
                }
            }
        }

        void SetMap(size_t leafIndex, size_t mIndex) {
            vector<TLeaf*> & v = compVectors[leafIndex];

            // cout << mIndex << endl;
            // cout << leafIndex << endl;
            // cout << MapVectors.size() << endl; 
            vector<vector<double>>* ret = MapVectors[mIndex];
            size_t n = v[0]->GetLen();

            ret->clear(); 
            ret->resize(n);
            
            for (size_t i = 0; i < n; ++i) {
                ret->at(i).clear();
                ret->at(i).reserve(v.size());
                for (size_t j = 0; j < v.size(); ++j) {
                    ret->at(i).push_back(v[j]->GetValue(i));
                }
            }
        }

        void SetVar(size_t leafIndex) {
            *varValues[leafIndex] = varLeaves[leafIndex]->GetValue(0);
        }

        void SetVectorVar(size_t leafIndex) {
            vectorVarValues[leafIndex]->clear();
            for (int i = 0; i < vectorVarLeaves[leafIndex]->GetLen(); ++i) {
                vectorVarValues[leafIndex]->push_back(vectorVarLeaves[leafIndex]->GetValue(i));
            }
            // log(leafIndex);
            // log(vectorVarLeaves[leafIndex]->GetLen());
            // log(vectorVarValues[leafIndex].size());
            // log();
        }

    /// SWITCH, TIMING, AND LOGGING HELPERS
    /// 
        void log() {
            if (debug)
                cout << LOG_PREFIX << endl; 
        }
        
        template<typename t>
        void log(t s) {
            if (debug) {
                cout << LOG_PREFIX;
                lograw(s);
                cout << endl;
            }
        }

        template<typename t>
        void logp(t s) {
            if (debug) {
                cout << LOG_PREFIX;
                lograw(s);
            }
        }

        template<typename t>
        void logr(t s) {
            if (debug) {
                lograw(s);
                cout << endl; 
            }
        }

        template<typename t>
        void warning(t s) {
            debug = true;
            log("WARNING :: " + to_string(s));
            debug = false;
        }

        template<typename t>
        void lograw(t s) {
            cout << s; 
        }

        void indent(int level){
            cout << LOG_PREFIX << string(level*3, ' ');
        }

        void print(string s, int level=0) {
            indent(level);
            cout << s << endl;
        }

        template<typename t>
        void print(t* var, int level=0) {
            indent(level); cout << *var << endl;
        }

        template<typename t>
        void print(vector<t>* var, int level=0) {
            indent(level);
            cout << "{ ";
            for (size_t i = 0; i < var->size() - 1; ++i) {
                cout << var->at(i) << ", ";
            }
            cout << var->back() << " }";
            cout << endl;
        }

        template<typename t>
        void print(vector<vector<t>>* var, int level=0) {
            for (size_t i = 0; i < var->size(); ++i) {
                print(&var[i], level);
            }
        }

        void print(vector<TLorentzMock>* var, int level=0) {
            for (size_t i = 0; i < var->size(); ++i) {
                auto elt = var->at(i);
                indent(level); cout << "(Pt,Eta)=(" << elt.Pt() << "," << elt.Eta() << "}" << endl;
            }
        }

        void print(vector<TLorentzVector>* var, int level=0) {
            for (size_t i = 0; i < var->size(); ++i) {
                auto elt = var->at(i);
                indent(level);
                elt.Print();
            }
        }

        void print() {
            indent(0);
            cout << endl; 
        }

        vector<string> split(string s, char delimiter = '.') {
            std::replace(s.begin(), s.end(), delimiter, ' ');
            vector<string> ret;
            stringstream ss(s);
            string temp;
            while(ss >> temp)
                ret.push_back(temp);
            return ret;
        }

        string lastWord(string s, char delimiter = '.') {
            return split(s, delimiter).back(); 
        }

    /// PRIVATE DATA
    /// 

        double duration = 0;
        std::chrono::high_resolution_clock::time_point timestart;

        TFileCollection *fc=nullptr;
        TChain *chain=nullptr;

        const string LOG_PREFIX = "SVJAnalysis :: ";
        std::map<vectorType, std::string> componentTypeStrings = {
            {vectorType::Lorentz, "TLorentzVector"},
            {vectorType::Mock, "MockTLorentzVector"},
            {vectorType::Map, "Map"}
        };

        // single variable data
        std::map<string, size_t> varIndex;
        vector<TLeaf *> varLeaves;
        vector<double*> varValues;

        // vector variable data
        std::map<string, size_t> vectorVarIndex;
        vector<TLeaf *> vectorVarLeaves;
        vector<vector<double>*> vectorVarValues;

        // vector component data
        //   indicies
        std::map<string, size_t> compIndex;
        vector<pair<size_t, vectorType>> subIndex;
        //   names
        vector<vector<TLeaf*>> compVectors;
        vector<vector<string>> compNames;
        //   values
        vector< vector< TLorentzVector >*> LorentzVectors;
        vector< vector< TLorentzMock >*> MockVectors;
        vector<vector<vector<double>>*> MapVectors;

        // cut variables
        vector<int> cutValues = vector<int>(Cuts::COUNT); 
};
