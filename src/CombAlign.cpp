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

using namespace std;
using namespace pugi;
using namespace elem;

int
main (int argc, char **argv)
{
  string localeId, transferFilePath, lextorFilePath, chunkerFilePath, referenceFilePath,
      newRefFilePath;

  if (argc == 7)
    {
      localeId = argv[1];
      transferFilePath = argv[2];
      lextorFilePath = argv[3];
      chunkerFilePath = argv[4];
      referenceFilePath = argv[5];
      newRefFilePath = argv[6];
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
      transferFilePath =
	  "/home/aboelhamd/apertium-eng-spa-ambiguous-rules/apertium-eng-spa.spa-eng.t1x";
      lextorFilePath =
	  "/home/aboelhamd/eclipse-workspace/machinetranslation/test-lextor.txt";
      chunkerFilePath =
	  "/home/aboelhamd/eclipse-workspace/machinetranslation/test-chunker.txt";
      referenceFilePath =
	  "/home/aboelhamd/eclipse-workspace/machinetranslation/tgt-test.txt";
      newRefFilePath =
	  "/home/aboelhamd/eclipse-workspace/machinetranslation/tgt-test-mul.txt";

      cout << "Error in parameters !" << endl;
      cout << "Parameters are : localeId transferFilePath lextorFilePath"
	  << " chunkerFilePath referenceFilePath newRefFilePath" << endl;
      cout << "localeId : ICU locale ID for the source language. For Kazakh => kk_KZ"
	  << endl;
      cout << "transferFilePath : Apertium transfer file of the language pair used."
	  << endl;
      cout << "lextorFilePath : Apertium lextor file for the source language sentences."
	  << endl;
      cout << "chunkerFilePath : chunker file path (output of this program and"
	  << " input for apertium interchunk)." << endl;
      cout << "referenceFilePath : Reference parallel target translation file path."
	  << endl;
      cout << "newRefFilePath : New aligned reference file path." << endl;
      return -1;
    }

  ifstream lextorFile (lextorFilePath.c_str ());
  ofstream chunkerFile (chunkerFilePath.c_str ());
  ifstream referenceFile (referenceFilePath.c_str ());
  ofstream newRefFile (newRefFilePath.c_str ());
  if (lextorFile.is_open () && chunkerFile.is_open () && referenceFile.is_open ()
      && newRefFile.is_open ())
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

//      unsigned i = 0;
      string tokenizedSentence, refSent;
      while (getline (lextorFile, tokenizedSentence) && getline (referenceFile, refSent))
	{
//	  cout << i++ << endl;

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
	  // final outs
	  vector<string> outs;
	  // number of possible combinations
	  unsigned compNum;
	  // nodes for every token and rule
	  map<unsigned, vector<RuleExecution::Node*> > nodesPool;
	  // ambiguous informations
	  vector<RuleExecution::AmbigInfo*> ambigInfo;

	  // rules combinations
	  vector<vector<RuleExecution::Node*> > combNodes;

	  nodesPool = RuleExecution::getNodesPool (tokenRules);

	  RuleExecution::getAmbigInfo (tokenRules, nodesPool, &ambigInfo, &compNum);
	  RuleExecution::getOuts (&outs, &combNodes, ambigInfo, nodesPool, ruleOutputs,
				  spaces);

	  // write the outs
	  for (unsigned j = 0; j < outs.size (); j++)
	    {
	      chunkerFile << outs[j] << endl;
	      newRefFile << refSent << endl;
	    }

	  chunkerFile << endl;
	  newRefFile << endl;

	  // delete AmbigInfo pointers
	  for (unsigned j = 0; j < ambigInfo.size (); j++)
	    {
	      // delete the dummy node pointers
	      set<RuleExecution::Node*> dummies;
	      for (unsigned k = 0; k < ambigInfo[j]->combinations.size (); k++)
		dummies.insert (ambigInfo[j]->combinations[k][0]);
	      for (set<RuleExecution::Node*>::iterator it = dummies.begin ();
		  it != dummies.end (); it++)
		delete (*it);

	      delete ambigInfo[j];
	    }
	  // delete Node pointers
	  for (map<unsigned, vector<RuleExecution::Node*> >::iterator it =
	      nodesPool.begin (); it != nodesPool.end (); it++)
	    {
	      for (unsigned j = 0; j < it->second.size (); j++)
		{
		  delete it->second[j];
		}
	    }
	}

      lextorFile.close ();
      chunkerFile.close ();
      referenceFile.close ();
      newRefFile.close ();
//      cout << "CombAlign finished!";
    }
  else
    {
      cout << "ERROR in opening files!" << endl;
    }

  return 0;
}
