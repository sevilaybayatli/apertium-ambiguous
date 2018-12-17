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
  string sentenceFilePath, lextorFilePath, interInFilePath, localeId, transferFilePath;

  if (argc == 6)
    {
      localeId = argv[1];
      transferFilePath = argv[2];
      sentenceFilePath = argv[3];
      lextorFilePath = argv[4];
      interInFilePath = argv[5];
    }
  else
    {
//      localeId = "es_ES";
//      transferFilePath = "transferFile.t1x";
//      sentenceFilePath = "spa-test.txt";
//      lextorFilePath = "spa-test.lextor";
//      interInFilePath = "inter.txt";
      localeId = "kk_KZ";
      transferFilePath = "transferFile1.t1x";
      sentenceFilePath = "sample-sentneces.txt";
      lextorFilePath = "lextor.txt";
      interInFilePath = "inter.txt";
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

      vector<vector<string> > vouts;

      ofstream interInFile (interInFilePath.c_str ());
      ofstream longSentFile ("longsentences.txt", ios::app);

      if (interInFile.is_open () && longSentFile.is_open ())
	for (unsigned i = 0; i < sourceSentences.size (); i++)
	  {
	    // cout << i << endl;

	    string sourceSentence, tokenizedSentence;
	    sourceSentence = sourceSentences[i];
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

	    // map of matched rules and tokens id
	    map<xml_node, vector<vector<unsigned> > > rulesApplied;

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
	    // applied rules with the first token id applied to
	    vector<vector<pair<unsigned, unsigned> > > rulesIds;
	    // group ambiguous rules applied to the same tokens
	    vector<vector<vector<unsigned> > > outsRules;

	    // ambiguity info contains the id of the first token and
	    // the number of the token as a pair and then another pair
	    // contains position of ambiguous rules among other rules and
	    // vector of the rules applied to that tokens
	    vector<
		pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<unsigned> > > > > ambigInfo;

	    if (RuleExecution::outputs (&outs, &rulesIds, &outsRules, &ambigInfo,
					tlTokens, tlTags, ruleOutputs, tokenRules,
					spaces))
	      {
		for (unsigned j = 0; j < outs.size (); j++)
		  {
		    interInFile << outs[j] << endl;
		  }
	      }
	    else
	      {
		longSentFile << sourceSentence << endl;
	      }

//	  for (map<unsigned, map<unsigned, string> >::iterator it = ruleOutputs.begin ();
//	      it != ruleOutputs.end (); it++)
//	    {
//	      cout << "rule id = " << it->first << endl;
//	      for (map<unsigned, string>::iterator it1 = it->second.begin ();
//		  it1 != it->second.end (); it1++)
//		{
//		  cout << "first token id = " << it1->first << endl;
//		  cout << "out : " << it1->second << endl;
//		}
//	    }
//
//	  cout << "outs size = " << outs.size () << endl;
//	  cout << endl << endl;
//	  for (unsigned j = 0; j < outs.size (); j++)
//	    cout << outs[j] << endl;
//	  cout << endl;
//
//	  for (unsigned j = 0; j < rulesIds.size (); j++)
//	    {
//	      for (unsigned k = 0; k < rulesIds[j].size (); k++)
//		{
//		  cout << rulesIds[j][k].first << " ";
//		}
//	      cout << endl;
//	    }
//	  cout << endl << endl;
//
//	  for (unsigned j = 0; j < outsRules.size (); j++)
//	    {
//	      for (unsigned k = 0; k < outsRules[j].size (); k++)
//		{
//		  for (unsigned h = 0; h < outsRules[j][k].size (); h++)
//		    {
//		      cout << outsRules[j][k][h] << "-";
//		    }
//		  cout << " ";
//		}
//	      cout << endl;
//	    }
//	  cout << endl << endl;
//
//	  for (unsigned j = 0; j < slTokens.size (); j++)
//	    {
//	      cout << j << " : " << slTokens[j] << "  " << tlTokens[j] << endl;
//	    }
//	  cout << endl;
//
//	  for (map<xml_node, vector<vector<unsigned> > >::iterator it =
//	      rulesApplied.begin (); it != rulesApplied.end (); it++)
//	    {
//	      xml_node rule = it->first;
//	      vector<vector<unsigned> > tokenIds = it->second;
//
//	      cout << "rule id = " << rule.attribute (ID).value () << endl;
//	      for (unsigned j = 0; j < tokenIds.size (); j++)
//		{
//		  for (unsigned k = 0; k < tokenIds[j].size (); k++)
//		    {
//		      cout << tokenIds[j][k] << " ";
//		    }
//		  cout << endl;
//		}
//	      cout << endl;
//	    }

	  }
      else
	cout << "ERROR in opening files!" << endl;
      interInFile.close ();
      longSentFile.close ();
    }
  else
    {
      cout << "ERROR in opening files!" << endl;
    }

  return 0;
}
