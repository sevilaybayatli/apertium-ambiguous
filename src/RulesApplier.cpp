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

//#include "../pugixml/pugixml.hpp"
#include "pugixml.hpp"
#include "RuleParser.h"
#include "RuleExecution.h"
#include "TranElemLiterals.h"
#include "CLExec.h"

using namespace std;
using namespace pugi;
using namespace elem;

int
main (int argc, char **argv)
{
  string localeId, transferFilePath, lextorFilePath, interInFilePath;

  if (argc == 5)
    {
      localeId = argv[1];
      transferFilePath = argv[2];
      lextorFilePath = argv[3];
      interInFilePath = argv[4];
    }
  else
    {
//      localeId = "es_ES";
//      transferFilePath = "transferFile.t1x";
//      sentenceFilePath = "spa-test.txt";
//      lextorFilePath = "spa-test.lextor";
//      interInFilePath = "inter2.txt";

//      localeId = "kk_KZ";
//      transferFilePath = "apertium-kaz-tur.kaz-tur.t1x";
//      sentenceFilePath = "sample-sentences.txt";
//      lextorFilePath = "sample-lextor.txt";
//      interInFilePath = "sample-inter.txt";

      localeId = "es_ES";
      transferFilePath = "./issues/apertium-eng-spa.spa-eng.t1x";
      lextorFilePath = "./issues/lextor.txt";
      interInFilePath = "./issues/interIn.txt";

      cout << "Error in parameters !" << endl;
      cout
	  << "Parameters are : localeId transferFilePath sentenceFilePath lextorFilePath interInFilePath"
	  << endl;
      cout << "localeId : ICU locale ID for the source language. For Kazakh => kk-KZ"
	  << endl;
      cout << "transferFilePath : Apertium transfer file of the language pair used."
	  << endl;
      cout << "lextorFilePath : Apertium lextor file for the source language sentences."
	  << endl;
      cout
	  << "interInFilePath : Output file name of this program which is the input for apertium interchunk."
	  << endl;
      return -1;
    }

  ifstream lextorFile (lextorFilePath.c_str ());
  if (lextorFile.is_open ())
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

      vector<string> tokenizedSentences;

      string tokenizedSentence;
      while (getline (lextorFile, tokenizedSentence))
	{
	  tokenizedSentences.push_back (tokenizedSentence);
	}
      lextorFile.close ();

      map<string, vector<vector<string> > > attrs = RuleParser::getAttrs (transfer);
      map<string, string> vars = RuleParser::getVars (transfer);
      map<string, vector<string> > lists = RuleParser::getLists (transfer);

      ofstream interInFile (interInFilePath.c_str ());
      if (interInFile.is_open ())
	for (unsigned i = 0; i < tokenizedSentences.size (); i++)
	  {
//	    cout << i << endl;

	    string tokenizedSentence;
	    tokenizedSentence = tokenizedSentences[i];

	    // spaces after each token
	    vector<string> spaces;

	    // tokens in the sentence order
	    vector<string> slTokens, tlTokens;

	    // tags of tokens in order
	    vector<vector<string> > slTags, tlTags;

	    RuleParser::sentenceTokenizer (&slTokens, &tlTokens, &slTags, &tlTags,
					   &spaces, tokenizedSentence);

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

	    RuleExecution::ruleOuts (&ruleOutputs, &tokenRules, slTokens, slTags,
				     tlTokens, tlTags, rulesApplied, attrs, lists, &vars,
				     spaces, localeId);
	    // final outs
	    vector<string> outs;
	    // number of possible combinations
	    unsigned compNum;
	    // nodes for every token and rule
	    map<unsigned, vector<RuleExecution::Node> > nodesPool;
	    // ambiguous informations
	    vector<RuleExecution::AmbigInfo> ambigInfo;
	    // rules combinations
	    vector<vector<RuleExecution::Node> > combNodes;

	    nodesPool = RuleExecution::getNodesPool (tokenRules);

	    RuleExecution::getAmbigInfo (tokenRules, nodesPool, &ambigInfo, &compNum);
	    RuleExecution::getOuts (&outs, &combNodes, ambigInfo, nodesPool, ruleOutputs,
				    spaces);

	    // write the outs
	    for (unsigned j = 0; j < outs.size (); j++)
	      interInFile << outs[j] << endl;
	  }
      else
	cout << "ERROR in opening files!" << endl;
      interInFile.close ();

      cout << "RulesApplier finished!";
    }
  else
    {
      cout << "ERROR in opening files!" << endl;
    }

  return 0;
}
