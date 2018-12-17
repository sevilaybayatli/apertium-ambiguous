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

#include <omp.h>

using namespace std;
using namespace pugi;
using namespace elem;

int
main (int argc, char **argv)
{
  string sentenceFilePath = "sentences.txt", lextorFilePath = "lextor.txt",
      transferOutFilePath = "transferOut.txt", beamFilePath = "BeamSearch-", modelsDest =
	  "models", localeId = "kk_KZ", transferFilePath = "tranferFile.tx1";

  if (argc >= 7)
    {
      localeId = argv[1];
      transferFilePath = argv[2];
      sentenceFilePath = argv[3];
      lextorFilePath = argv[4];
      transferOutFilePath = argv[5];
      modelsDest = argv[6];
    }
  else
    {
      cout << "Error in parameters !" << endl;
      return -1;
    }

  ifstream lextorFile (lextorFilePath.c_str ());
  ifstream inSentenceFile (sentenceFilePath.c_str ());
  if (lextorFile.is_open () && inSentenceFile.is_open ())
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

      map<string, vector<vector<string> > > attrs = RuleParser::getAttrs (transfer);
      map<string, string> vars = RuleParser::getVars (transfer);
      map<string, vector<string> > lists = RuleParser::getLists (transfer);

      vector<vector<string> > vslTokens;
      vector<vector<vector<pair<unsigned, unsigned> > > > vrulesIds;
      vector<
	  vector<
	      pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<unsigned> > > > > > vambigInfo;

      for (unsigned i = 0; i < sourceSentences.size (); i++)
	{
	  //	  cout << i << endl;
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
	  map<unsigned, vector<string> > catsApplied;

	  RuleParser::matchCats (&catsApplied, slTokens, slTags, transfer);

	  // map of matched rules and tokens id
	  map<xml_node, vector<vector<unsigned> > > rulesApplied;

	  RuleParser::matchRules (&rulesApplied, slTokens, catsApplied, transfer);

	  // rule and (target) token map to specific output
	  // if rule has many patterns we will choose the first token only
	  map<unsigned, map<unsigned, string> > ruleOutputs;

	  // map (target) token to all matched rules ids and the number of pattern items of each rule
	  map<unsigned, vector<pair<unsigned, unsigned> > > tokenRules;

	  RuleExecution::ruleOuts (&ruleOutputs, &tokenRules, slTokens, slTags, tlTokens,
				   tlTags, rulesApplied, attrs, lists, &vars, spaces,
				   localeId);

	  // final outs and their applied rules
	  vector<string> outs;
	  vector<vector<pair<unsigned, unsigned> > > rulesIds;
	  vector<vector<vector<unsigned> > > outsRules;

	  // ambiguity info contains the id of the first token and
	  // the number of the token as a pair and then another pair
	  // contains position of ambiguous rules among other rules and
	  // vector of the rules applied to that tokens
	  vector<
	      pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<unsigned> > > > > ambigInfo;

	  RuleExecution::outputs (&outs, &rulesIds, &outsRules, &ambigInfo, tlTokens,
				  tlTags, ruleOutputs, tokenRules, spaces);

	  // indexes of accumulated weights
	  vector<vector<unsigned> > weigInds;

	  RuleExecution::weightIndices (&weigInds, ambigInfo, outsRules);

	  vslTokens.push_back (slTokens);
	  vrulesIds.push_back (rulesIds);
	  vambigInfo.push_back (ambigInfo);
	}

      // Read transfer sentences from tansfer file
      vector<string> vtransfers[sourceSentences.size ()];

      ifstream transferOutFile (transferOutFilePath.c_str ());
      if (transferOutFile.is_open ())
	for (unsigned i = 0; i < sourceSentences.size (); i++)
	  {
	    string line;
	    vector<string> transfers;
	    for (unsigned j = 0; j < vrulesIds[i].size (); j++)
	      {
		getline (transferOutFile, line);
		transfers.push_back (line);
	      }

	    vtransfers[i] = transfers;
	  }
      else
	cout << "error in opening weightOutFile" << endl;
      transferOutFile.close ();

      // beamSearch
      map<string, map<string, vector<float> > > classesWeights =
	  CLExec::loadYasmetModels (modelsDest);

      for (int ind = 6; ind < argc; ind++)
	{
	  int beam;
	  istringstream buffer (argv[ind]);
	  buffer >> beam;

	  vector<vector<pair<unsigned, float> > > bestTransInds;

	  for (unsigned i = 0; i < sourceSentences.size (); i++)
	    {
	      vector<pair<vector<unsigned>, float> > beamTree;
	      CLExec::beamSearch (&beamTree, beam, vslTokens[i], vambigInfo[i],
				  classesWeights, localeId);

	      vector<pair<unsigned, float> > transInds;
	      CLExec::getTransInds (&transInds, beamTree, vrulesIds[i]);

	      // Take the best translation index and weight only
	      bestTransInds.push_back (transInds);
	    }

	  // write best translations to file
	  ofstream beamFile (
	      (beamFilePath + string (argv[ind]) + string (".txt")).c_str ());
	  if (beamFile.is_open ())
	    {
	      beamFile << "Beam search algorithm results with beam = " << beam << endl;
	      beamFile << "-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-"
		  << endl << endl << endl;
	      for (unsigned i = 0; i < sourceSentences.size (); i++)
		{
		  beamFile << (i + 1) << endl;
		  beamFile << "Source : " << sourceSentences[i] << endl << endl;

		  for (unsigned j = 0; j < bestTransInds[i].size (); j++)
		    {
		      beamFile << "Target : " << vtransfers[i][bestTransInds[i][j].first]
			  << endl;
		      beamFile << "Weight : " << bestTransInds[i][j].second << endl
			  << endl;
		    }
		  beamFile << "--------------------------------------------------" << endl
		      << endl;
		}
	      beamFile.close ();
	    }
	}
    }
  else
    {
      cout << "ERROR in opening files!" << endl;
    }
  return 0;
}
