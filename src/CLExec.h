#ifndef SRC_CLEXEC_H_
#define SRC_CLEXEC_H_

#include <string>
#include <vector>

//#include "../pugixml/pugixml.hpp"

using namespace std;
using namespace pugi;

class CLExec
{
public:

  static void
  segmenter (string inFilePath, string outFilePath);

  static void
  lextor (string inFilePath, string outFilePath);

  static void
  biltrans (string inFilePath, string outFilePath);

  static void
  interchunk (string inFilePath, string outFilePath);

  static void
  postchunk (string inFilePath, string outFilePath);

  static void
  transfer (string inFilePath, string outFilePath);

  static void
  assignWeights (string inFilePath, string outFilePath);

  static vector<string>
  getFilesInDir (string dir);

//  static void
//  runYasmet ();

  static map<string, map<string, vector<float> > >
  loadYasmetModels (string modelsDest);

  static void
  handleDatasets ();

  static string
  toLowerCase (string word, string localeId);

  static string
  toUpperCase (string word, string localeId);

  static string
  FirLetUpperCase (string word, string localeId);

  static int
  compare (string word1, string word2);

  static int
  compareCaseless (string word1, string word2, string localeId);

  static void
  beamSearch (
      vector<pair<vector<unsigned>, float> > *beamTree,
      unsigned beam,
      vector<string> slTokens,
      vector<pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<xml_node> > > > > ambigInfo,
      map<string, map<string, vector<float> > > classesWeights, string localeId);

  static void
  getTransInds (vector<pair<unsigned, float> > *Translations,
		vector<pair<vector<unsigned>, float> > beamTree,
		vector<vector<unsigned> > weigInds);
};

#endif /* SRC_CLEXEC_H_ */