#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <vector>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <sstream>
#include <algorithm>

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
  string lextorFilePath, localeId, transferFilePath, transferOutFilePath, weightFilePath,
      bestModFilePath;

  if (argc == 7)
    {
      localeId = argv[1];
      transferFilePath = argv[2];
      lextorFilePath = argv[3];

      transferOutFilePath = argv[4];
      weightFilePath = argv[5];

      bestModFilePath = argv[6];
    }
  else
    {
//      localeId = "es_ES";
//      transferFilePath = "transferFile.t1x";
//      sentenceFilePath = "spa-test.txt";
//      lextorFilePath = "spa-test.lextor";
//      interInFilePath = "beaminter.out";
//      modelsDest = "modelstry";
//      k = "8";

//      localeId = "kk_KZ";
//      transferFilePath = "apertium-kaz-tur.kaz-tur.t1x";
//      sentenceFilePath = "sample-sentences.txt";
//      lextorFilePath = "sample-lextor.txt";
//
//      transferOutFilePath = "sample-transfer.txt";
//      weightFilePath = "sample-weights.txt";
//
//      outputFilePath = "outAnalysis.txt";
//      bestModFilePath = "bestModFile.txt";
//      randModFilePath = "randModFile.txt";

      localeId = "es_ES";
      transferFilePath = "transferFile3.t1x";
      lextorFilePath = "spa-lextor.txt";

      transferOutFilePath = "spa-transfer.txt";
      weightFilePath = "spa-weight.txt";

      bestModFilePath = "bestModFile.txt";

      cout << "Error in parameters !" << endl;
      cout << "Parameters are : localeId transferFilePath lextorFilePath"
	  << " transferOutFilePath weightOutFilePath bestModFilePath" << endl;
      cout << "localeId : ICU locale ID for the source language. For Kazakh => kk-KZ"
	  << endl;
      cout << "transferFilePath : Apertium transfer file of the language pair used."
	  << endl;
      cout << "lextorFilePath : Apertium lextor file for the source language sentences."
	  << endl;
      cout
	  << "transferOutFilePath : Output file of apertium transfer for the source language sentences."
	  << endl;
      cout
	  << "weightOutFilePath : Language model weights file for the source language sentences."
	  << endl;
      cout
	  << "bestModFilePath : Second output file name which is the best (language model) translations for the source language sentences."
	  << endl;
      return -1;
    }

  ifstream lextorFile (lextorFilePath.c_str ());
  ifstream weightFile (weightFilePath.c_str ());
  ifstream transferOutFile (transferOutFilePath.c_str ());
  ofstream bestModFile (bestModFilePath.c_str ());

  if (lextorFile.is_open () && weightFile.is_open () && transferOutFile.is_open ()
      && bestModFile.is_open ())
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

      // unsigned i = 0;
      string tokenizedSentence;
      while (getline (lextorFile, tokenizedSentence))
	{
	  // cout << i << endl;

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
	  vector<string> normOuts;
	  // number of generated combinations
	  unsigned compNum;
	  // nodes for every token and rule
	  map<unsigned, vector<RuleExecution::Node*> > nodesPool;
	  // ambiguous informations
	  vector<RuleExecution::AmbigInfo*> ambigInfo;
	  // rules combinations
	  vector<vector<RuleExecution::Node*> > normCombNodes;

	  nodesPool = RuleExecution::getNodesPool (tokenRules);

	  RuleExecution::getAmbigInfo (tokenRules, nodesPool, &ambigInfo, &compNum);

	  RuleExecution::getOuts (&normOuts, &normCombNodes, ambigInfo, nodesPool,
				  ruleOutputs, spaces);

	  // read weights
	  string line;
	  vector<float> normWeights;
	  for (unsigned j = 0; j < normOuts.size (); j++)
	    {
	      getline (weightFile, line);
	      float weight = strtof (line.c_str (), NULL);
	      normWeights.push_back (weight);
	    }
	  // beware of the newline
	  getline (weightFile, line);

	  // read transfer
	  vector<string> normTransfers;
	  for (unsigned j = 0; j < normOuts.size (); j++)
	    {
	      getline (transferOutFile, line);
	      normTransfers.push_back (line);
	    }
	  // beware of the newline
	  getline (transferOutFile, line);

	  // remove redundant outputs
	  vector<string> outs;
	  vector<vector<RuleExecution::Node*> > combNodes;
	  vector<float> weights;
	  vector<string> transfers;
	  for (unsigned j = 0; j < normOuts.size (); j++)
	    if (find (outs.begin (), outs.end (), normOuts[j]) == outs.end ())
	      {
		outs.push_back (normOuts[j]);
		combNodes.push_back (normCombNodes[j]);
		weights.push_back (normWeights[j]);
		transfers.push_back (normTransfers[j]);
	      }
	  normOuts = outs;
	  normCombNodes = combNodes;
	  normWeights = weights;
	  normTransfers = transfers;

	  // normalize weights
	  RuleExecution::normaliseWeights (&normWeights);

	  // best model weight
	  unsigned maxInd = 0;
	  for (unsigned j = 1; j < normWeights.size (); j++)
	    {
	      if (normWeights[j] > normWeights[maxInd])
		maxInd = j;
	    }
	  bestModFile << normTransfers[maxInd] << endl;

	}

      weightFile.close ();
      transferOutFile.close ();
      lextorFile.close ();
      bestModFile.close ();
    }
  else
    {
      cout << "ERROR in opening files!" << endl;
    }
  return 0;
}
