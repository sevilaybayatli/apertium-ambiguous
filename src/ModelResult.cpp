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
  string sentenceFilePath, lextorFilePath, localeId, transferFilePath,
      transferOutFilePath, weightFilePath, outputFilePath, bestModFilePath,
      randModFilePath;

  if (argc == 10)
    {
      localeId = argv[1];
      transferFilePath = argv[2];
      sentenceFilePath = argv[3];
      lextorFilePath = argv[4];

      transferOutFilePath = argv[5];
      weightFilePath = argv[6];

      outputFilePath = argv[7];
      bestModFilePath = argv[8];
      randModFilePath = argv[9];
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

      localeId = "kk_KZ";
      transferFilePath = "apertium-kaz-tur.kaz-tur.t1x";
      sentenceFilePath = "sample-sentences.txt";
      lextorFilePath = "sample-lextor.txt";

      transferOutFilePath = "sample-transfer.txt";
      weightFilePath = "sample-weights.txt";

      outputFilePath = "outAnalysis.txt";
      bestModFilePath = "bestModFile.txt";
      randModFilePath = "randModFile.txt";

      cout << "Error in parameters !" << endl;
      cout
	  << "Parameters are : localeId transferFilePath sentenceFilePath lextorFilePath transferOutFilePath weightOutFilePath outputFilePath bestModFilePath randModFilePath"
	  << endl;
      cout << "localeId : ICU locale ID for the source language. For Kazakh => kk-KZ"
	  << endl;
      cout << "transferFilePath : Apertium transfer file of the language pair used."
	  << endl;
      cout << "sentenceFilePath : Source language sentences file." << endl;
      cout << "lextorFilePath : Apertium lextor file for the source language sentences."
	  << endl;
      cout
	  << "transferOutFilePath : Output file of apertium transfer for the source language sentences."
	  << endl;
      cout
	  << "weightOutFilePath : Language model weights file for the source language sentences."
	  << endl;
      cout
	  << "outputFilePath : First output file name of this program which is the complete analysis for the source language sentences."
	  << endl;
      cout
	  << "bestModFilePath : Second output file name which is the best (language model) translations for the source language sentences."
	  << endl;
      cout
	  << "randModFilePath : Third output file name which is random translations from (language model) for the source language sentences."
	  << endl;
      return -1;
    }

  // seed for randomness
  srand (time (NULL));

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

      // empty output files
      ofstream outputFile (outputFilePath.c_str ());
      outputFile.close ();
      ofstream bestModFile (bestModFilePath.c_str ());
      bestModFile.close ();
      ofstream randModFile (randModFilePath.c_str ());
      randModFile.close ();

      ifstream weightFile (weightFilePath.c_str ());
      ifstream transferOutFile (transferOutFilePath.c_str ());

      if (weightFile.is_open () && transferOutFile.is_open ())
	for (unsigned i = 0; i < sourceSentences.size (); i++)
	  {
	    cout << i << endl;

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

	    // final outputs
	    vector<string> normOuts;
	    // number of generated combinations
	    unsigned compNum;
	    // nodes for every token and rule
	    map<unsigned, vector<RuleExecution::Node> > nodesPool;
	    // ambiguous informations
	    vector<RuleExecution::AmbigInfo> ambigInfo;
	    // beam tree
	    vector<pair<vector<RuleExecution::Node>, float> > beamTree;
	    // rules combinations
	    vector<vector<RuleExecution::Node> > normCombNodes;

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

	    // read transfer
	    vector<string> normTransfers;
	    for (unsigned j = 0; j < normOuts.size (); j++)
	      {
		getline (transferOutFile, line);
		normTransfers.push_back (line);
	      }

	    // remove redundant outputs
	    vector<string> outs;
	    vector<vector<RuleExecution::Node> > combNodes;
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

	    // write normal outputs
	    ofstream outputFile (outputFilePath.c_str (), ofstream::app);
	    if (outputFile.is_open ())
	      {
		outputFile << "Analysis of sentence : " << endl;
		outputFile << sourceSentence << endl << endl << endl;

		outputFile << endl;
		outputFile << "sentence id ||| coverage id ||| original sentence |||"
		    << " lextor ||| rules ||| chunker ||| final sentence ||| score"
		    << endl << endl;

		for (unsigned j = 0; j < normWeights.size (); j++)
		  {
		    // sentence id
		    outputFile << (i + 1) << " ||| ";
		    // coverage id
		    outputFile << (j + 1) << " ||| ";
		    // original sentence
		    outputFile << sourceSentence << " ||| ";
		    // lextor
		    outputFile << tokenizedSentence << " ||| ";
		    // rules
		    for (unsigned k = 0; k < normCombNodes[j].size (); k++)
		      if (normCombNodes[j][k].ruleId)
			outputFile << normCombNodes[j][k].ruleId << " ";
		    outputFile << "||| ";
		    // chuncker
		    outputFile << normOuts[j] << " ||| ";
		    // final sentence
		    outputFile << normTransfers[j] << " ||| ";
		    // score
		    outputFile << normWeights[j] << endl << endl;
		  }

		outputFile
		    << "---------------------------------------------------------------------------------------------------------"
		    << endl << endl;

		outputFile.close ();
	      }

	    // Model weighting
	    // best weight
	    ofstream bestModFile (bestModFilePath.c_str (), ofstream::app);
	    if (bestModFile.is_open ())
	      {
		bestModFile
		    << "---------------------------------------------------------------------------------------------------------"
		    << endl << endl;

		bestModFile << (i + 1) << endl;
		bestModFile << "Source :  " << sourceSentence << endl << endl;

		unsigned maxInd = 0;
		for (unsigned j = 1; j < normWeights.size (); j++)
		  {
		    if (normWeights[j] > normWeights[maxInd])
		      maxInd = j;
		  }

		// final sentence
		bestModFile << "Target :  " << normTransfers[maxInd] << endl;
		// score
		bestModFile << "Weight :  " << normWeights[maxInd] << endl;
		// rules
		bestModFile << "Rules :  ";
		for (unsigned k = 0; k < normCombNodes[maxInd].size (); k++)
		  if (normCombNodes[maxInd][k].ruleId)
		    bestModFile << normCombNodes[maxInd][k].ruleId << " ";

		bestModFile << endl
		    << "---------------------------------------------------------------------------------------------------------"
		    << endl << endl << endl;
	      }
	    bestModFile.close ();

	    // Random weight
	    ofstream randModFile (randModFilePath.c_str (), ofstream::app);
	    if (randModFile.is_open ())
	      {
		randModFile << (i + 1) << endl;
		randModFile << "Source :  " << sourceSentence << endl << endl;

		int random = rand () % normWeights.size ();

		// final sentence
		randModFile << "Target :  " << normTransfers[random] << endl;
		// score
		randModFile << "Weight :  " << normWeights[random] << endl;
		// rules
		randModFile << "Rules :  ";
		for (unsigned k = 0; k < normCombNodes[random].size (); k++)
		  if (normCombNodes[random][k].ruleId)
		    randModFile << normCombNodes[random][k].ruleId << " ";

		randModFile << endl
		    << "---------------------------------------------------------------------------------------------------------"
		    << endl << endl << endl;
	      }
	    randModFile.close ();
	  }
      else
	{
	  cout << "ERROR in opening files!" << endl;
	}
      weightFile.close ();
      transferOutFile.close ();
    }
  else
    {
      cout << "ERROR in opening files!" << endl;
    }
  return 0;
}
