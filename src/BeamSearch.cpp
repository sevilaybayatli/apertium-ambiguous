#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <sstream>

//#include "../pugixml/pugixml.hpp"
#include "pugixml.hpp"
#include "RuleParser.h"
#include "RuleExecution.h"
#include "TranElemLiterals.h"
#include "CLExec.h"

#include <omp.h>

using namespace std;
using namespace pugi;
using namespace elem;

int
main (int argc, char **argv)
{
  string lextorFilePath, interInFilePath, localeId, transferFilePath, modelsDest, k;

  if (argc == 7)
    {
      localeId = argv[1];
      transferFilePath = argv[2];
      lextorFilePath = argv[3];
      interInFilePath = argv[4];
      modelsDest = argv[5];
      k = argv[6];
    }
  else
    {
      localeId = "es_ES";
      transferFilePath = "apertium-eng-spa.spa-eng.t1x";
      lextorFilePath = "lextor.txt";
      interInFilePath = "beaminter.txt";
      modelsDest = "/home/aboelhamd/Downloads/models";
      k = "8";

//      localeId = "kk_KZ";
//      transferFilePath = "apertium-kaz-tur.kaz-tur.t1x";
//      sentenceFilePath = "src.txt";
//      lextorFilePath = "lextor.txt";
//      interInFilePath = "beam-inter.txt";
//      modelsDest = "./UntitledFolder/models";
//      k = "8";

      cout << "Error in parameters !" << endl;
      cout
	  << "Parameters are : localeId transferFilePath lextorFilePath interInFilePath modelsDest beamSize"
	  << endl;
      cout << "localeId : ICU locale ID for the source language. For Kazakh => kk-KZ"
	  << endl;
      cout << "transferFilePath : Apertium transfer file of the language pair used."
	  << endl;
      cout << "lextorFilePath : Apertium lextor file for the source language sentences."
	  << endl;
      cout
	  << "interInFilePath : Output file of this program which is the input for apertium interchunk."
	  << endl;
      cout << "modelsDest : Yasmet models destination." << endl;
      cout << "beamSize : The size of beam in beam search algorithm." << endl;
      return -1;
    }

  ifstream lextorFile (lextorFilePath.c_str ());
  ofstream interInFile (interInFilePath.c_str ());
  if (lextorFile.is_open () && interInFile.is_open ())
    {
      // load transfer file in an xml document object
      xml_document transferDoc;
      xml_parse_result result = transferDoc.load_file (transferFilePath.c_str ());
      if (string (result.description ()) != "No error")
	{
	  cout << "ERROR : " << result.description () << endl;
	  return -1;
	}

      // xml node of the parent node (transfer) in the transfer file
      xml_node transfer = transferDoc.child ("transfer");

      map<string, vector<vector<string> > > attrs = RuleParser::getAttrs (transfer);
      map<string, string> vars = RuleParser::getVars (transfer);
      map<string, vector<string> > lists = RuleParser::getLists (transfer);
      map<string, map<string, vector<float> > > classesWeights =
	  CLExec::loadYasmetModels (modelsDest);

      int beam;
      stringstream buffer (k);
      buffer >> beam;

      string tokenizedSentence;
      while (getline (lextorFile, tokenizedSentence))
	{
	  //	  cout << i << endl;

	  // spaces after each token
	  vector<string> spaces;

	  // tokens in the sentence order
	  vector<string> slTokens, tlTokens;

	  // tags of tokens in order
	  vector<vector<string> > slTags, tlTags;

	  RuleParser::sentenceTokenizer (&slTokens, &tlTokens, &slTags, &tlTags, &spaces,
					 tokenizedSentence);

	  // map of tokens ids and their matched categories
	  map<unsigned, vector<string> > catsApplied;

	  RuleParser::matchCats (&catsApplied, slTokens, slTags, transfer);

	  // map of matched rules and a pair of first token id and patterns number
	  map<xml_node, vector<pair<unsigned, unsigned> > > rulesApplied;

	  RuleParser::matchRules (&rulesApplied, slTokens, catsApplied, transfer);

	  // rule and (target) token map to specific output
	  // if rule has many patterns we will choose the first token only
	  map<unsigned, map<unsigned, string> > ruleOutputs;

	  // map (target) token to all matched rules ids and the number of pattern items of each rule
	  map<unsigned, vector<pair<unsigned, unsigned> > > tokenRules;

	  RuleExecution::ruleOuts (&ruleOutputs, &tokenRules, slTokens, slTags, tlTokens,
				   tlTags, rulesApplied, attrs, lists, &vars, spaces,
				   localeId);

	  // final outputs
	  vector<string> outs;
	  // number of generated combinations
	  unsigned compNum;
	  // nodes for every token and rule
	  map<unsigned, vector<RuleExecution::Node*> > nodesPool;
	  // ambiguous informations
	  vector<RuleExecution::AmbigInfo*> ambigInfo;
	  // beam tree
	  vector<pair<vector<RuleExecution::Node*>, float> > beamTree;
	  // rules combinations
	  vector<vector<RuleExecution::Node*> > combNodes;

	  nodesPool = RuleExecution::getNodesPool (tokenRules);

	  RuleExecution::getAmbigInfo (tokenRules, nodesPool, &ambigInfo, &compNum);

	  vector<RuleExecution::AmbigInfo*> newAmbigInfo;
	  for (unsigned j = 0; j < ambigInfo.size (); j++)
	    if (ambigInfo[j]->combinations.size () > 1)
	      newAmbigInfo.push_back (ambigInfo[j]);

	  CLExec::beamSearch (&beamTree, beam, slTokens, newAmbigInfo, classesWeights,
			      localeId);

	  // take the first sentence only
	  beamTree.erase (beamTree.begin () + 1, beamTree.end ());

	  RuleExecution::getOuts (&outs, &combNodes, beamTree, nodesPool, ruleOutputs,
				  spaces);

	  // write the outs
	  for (unsigned j = 0; j < outs.size (); j++)
	    interInFile << outs[j] << endl;

	}
      interInFile.close ();
      lextorFile.close ();
    }
  else
    {
      cout << "ERROR in opening files!" << endl;
    }
  return 0;
}
