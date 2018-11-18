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
  string sentenceFilePath, lextorFilePath, interInFilePath;

  if (argc == 4)
    {
      sentenceFilePath = argv[1];
      lextorFilePath = argv[2];
      interInFilePath = argv[3];
    }
  else
    {
      cout << "Error in parameters !" << endl;
      return -1;
    }

//  cout << "Sentences Analysis started" << endl;
  ifstream lextorFile (lextorFilePath.c_str ());
  ifstream inSentenceFile (sentenceFilePath.c_str ());
  if (lextorFile.is_open () && inSentenceFile.is_open ())
    {
      // load transfer file in an xml document object
      xml_document transferDoc;
      xml_parse_result result = transferDoc.load_file ("transferFile.t1x");

      if (string (result.description ()) != "No error")
	{
	  cout << "ERROR : " << result.description () << endl;
	  return -1;
	}

      // xml node of the parent node (transfer) in the transfer file
      xml_node transfer = transferDoc.child ("transfer");

      vector<string> sourceSentences, tokenizedSentences;

      string tokenizedSentence;
      while (getline (lextorFile, tokenizedSentence))
	{
	  string sourceSentence;
	  if (!getline (inSentenceFile, sourceSentence))
	    sourceSentence = "No more sentences";

	  sourceSentences.push_back (sourceSentence);
	  tokenizedSentences.push_back (tokenizedSentence);
	}
      lextorFile.close ();
      inSentenceFile.close ();

      vector<vector<string> > vslTokens;
      vector<vector<string> > vouts;
      vector<vector<vector<unsigned> > > vrulesIds;
      vector<vector<vector<vector<xml_node> > > > voutsRules;
      vector<
	  vector<
	      pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<xml_node> > > > > > vambigInfo;
      vector<vector<vector<unsigned> > > vweigInds;

      for (unsigned i = 0; i < sourceSentences.size (); i++)
	{
	  string sourceSentence, tokenizedSentence;
	  sourceSentence = sourceSentences[i];
	  tokenizedSentence = tokenizedSentences[i];

	  // spaces after each token
	  vector<string> spaces;

	  // tokens in the sentence order
	  vector<string> slTokens, tlTokens;

	  // tags of tokens in order
	  vector<vector<string> > slTags, tlTags;

	  RuleParser::sentenceTokenizer (&slTokens, &tlTokens, &slTags, &tlTags, &spaces,
					 tokenizedSentence);

	  // map of tokens ids and their matched categories
	  map<int, vector<string> > catsApplied;

	  RuleParser::matchCats (&catsApplied, slTokens, slTags, transfer);

	  // map of matched rules and tokens id
	  map<xml_node, vector<vector<int> > > rulesApplied;

	  RuleParser::matchRules (&rulesApplied, slTokens, catsApplied, transfer);

	  map<string, vector<vector<string> > > attrs = RuleParser::getAttrs (transfer);

	  // rule and (target) token map to specific output
	  // if rule has many patterns we will choose the first token only
	  map<xml_node, map<int, vector<string> > > ruleOutputs;

	  // map (target) token to all matched rules and the number of pattern items of each rule
	  map<int, vector<pair<xml_node, unsigned> > > tokenRules;

	  RuleExecution::ruleOuts (&ruleOutputs, &tokenRules, tlTokens, tlTags,
				   rulesApplied, attrs);

	  // final outs and their applied rules
	  vector<string> outs;
	  vector<vector<unsigned> > rulesIds;
	  vector<vector<vector<xml_node> > > outsRules;

	  // ambiguity info contains the id of the first token and
	  // the number of the token as a pair and then another pair
	  // contains position of ambiguous rules among other rules and
	  // vector of the rules applied to that tokens
	  vector<
	      pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<xml_node> > > > > ambigInfo;

	  RuleExecution::outputs (&outs, &rulesIds, &outsRules, &ambigInfo, tlTokens,
				  tlTags, ruleOutputs, tokenRules, spaces);

	  // indexes of accumulated weights
	  vector<vector<unsigned> > weigInds;

	  RuleExecution::weightIndices (&weigInds, ambigInfo, outsRules);

	  vslTokens.push_back (slTokens);
	  vouts.push_back (outs);
	  vrulesIds.push_back (rulesIds);
	  voutsRules.push_back (outsRules);
	  vambigInfo.push_back (ambigInfo);
	  vweigInds.push_back (weigInds);
	}

      ofstream interInFile (interInFilePath.c_str ());
      if (interInFile.is_open ())
	for (unsigned i = 0; i < sourceSentences.size (); i++)
	  {
	    vector<string> outs = vouts[i];
	    for (unsigned j = 0; j < outs.size (); j++)
	      {
		interInFile << outs[j] << endl;
	      }
	  }
      else
	cout << "error in opening interInFile" << endl;
      interInFile.close ();
//      cout << "Sentences Analysis finished" << endl;
    }
  else
    {
      cout << "ERROR in opening files!" << endl;
    }

  return 0;
}
