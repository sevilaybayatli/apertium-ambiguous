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
  string localeId, transferFilePath, sourceFilePath, srcLexFilePath, targetFilePath,
      orderedSrcFilePath, orderedTrgFilePath;

  if (argc == 8)
    {
      localeId = argv[1];
      transferFilePath = argv[2];
      sourceFilePath = argv[3];
      srcLexFilePath = argv[4];
      targetFilePath = argv[5];
      orderedSrcFilePath = argv[6];
      orderedTrgFilePath = argv[7];
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
      sourceFilePath = "/home/aboelhamd/eclipse-workspace/machinetranslation/source.txt";
      srcLexFilePath =
	  "/home/aboelhamd/eclipse-workspace/machinetranslation/source-lextor.txt";
      targetFilePath = "/home/aboelhamd/eclipse-workspace/machinetranslation/target.txt";
      orderedSrcFilePath =
	  "/home/aboelhamd/eclipse-workspace/machinetranslation/ordered-source.txt";
      orderedTrgFilePath =
	  "/home/aboelhamd/eclipse-workspace/machinetranslation/ordered-target.txt";

      cout << "Error in parameters !" << endl;
      cout << "Parameters are : localeId transferFilePath sourceFilePath"
	  << " lextorFilePath targetFilePath orderedSrcFilePath orderedTrgFilePath"
	  << endl;
      cout << "localeId : ICU locale ID for the source language. For Kazakh => kk_KZ"
	  << endl;
      cout << "transferFilePath : Apertium transfer file of the language pair used."
	  << endl;
      cout << "sourceFilePath : File for the source language sentences." << endl;
      cout << "lextorFilePath : Apertium lextor file for the source language sentences."
	  << endl;
      cout << "targetFilePath : File for the target language sentences." << endl;
      cout
	  << "orderedSrcFilePath : New file for the ordered source language sentences by most ambiguous."
	  << endl;
      cout
	  << "orderedTrgFilePath : New file for the ordered target language sentences by most ambiguous."
	  << endl;
      return -1;
    }

  ifstream lextorFile (srcLexFilePath.c_str ());
  ifstream sourceFile (sourceFilePath.c_str ());
  ifstream targetFile (targetFilePath.c_str ());
  ofstream orderedSrcFile (orderedSrcFilePath.c_str ());
  ofstream orderedTrgFile (orderedTrgFilePath.c_str ());
  if (lextorFile.is_open () && sourceFile.is_open () && orderedSrcFile.is_open ()
      && targetFile.is_open () && orderedTrgFile.is_open ())
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

      vector<string> orderedSources, orderedTargets;
      vector<unsigned> ambigCounts;

      unsigned i = 0;
      string tokenizedSentence, sourceSentence, targetSentence;
      while (getline (lextorFile, tokenizedSentence)
	  && getline (sourceFile, sourceSentence) && getline (targetFile, targetSentence))
	{
	  cout << i++ << endl;

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

	  // number of possible combinations
	  unsigned compNum;
	  // nodes for every token and rule
	  map<unsigned, vector<RuleExecution::Node*> > nodesPool;

	  nodesPool = RuleExecution::getNodesPool (tokenRules);

	  compNum = RuleExecution::getAmbigCount (tokenRules, nodesPool);

	  bool put = false;
	  for (unsigned j = 0; j < ambigCounts.size (); j++)
	    {
	      if (compNum > ambigCounts[j])
		{
		  ambigCounts.insert (ambigCounts.begin () + j, compNum);
		  orderedSources.insert (orderedSources.begin () + j, sourceSentence);
		  orderedTargets.insert (orderedTargets.begin () + j, targetSentence);
		  put = true;
		  break;
		}
	    }
	  if (!put)
	    {
	      ambigCounts.push_back (compNum);
	      orderedSources.push_back (sourceSentence);
	      orderedTargets.push_back (targetSentence);
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

      // write the ordered sentences
      for (unsigned j = 0; j < 10000; j++)
	{
	  cout << j << endl;
	  orderedSrcFile << orderedSources[j] << endl;
	  orderedTrgFile << orderedTargets[j] << endl;
	}

      lextorFile.close ();
      sourceFile.close ();
      targetFile.close ();
      orderedSrcFile.close ();
      orderedTrgFile.close ();
    }
  else
    {
      cout << "ERROR in opening files!" << endl;
    }

  return 0;
}
