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
  string sentenceFilePath = "sentences.txt", lextorFilePath = "lextor.txt",
      transferOutFilePath = "transfer.txt", weightOutFilePath = "weights.txt",
      outputFilePath = "output.txt", localeId = "kk_KZ", transferFilePath =
	  "transferFile.tx1", datasetsPath = "datasets";

  if (argc == 9)
    {
      localeId = argv[1];
      transferFilePath = argv[2];
      sentenceFilePath = argv[3];
      lextorFilePath = argv[4];
      transferOutFilePath = argv[5];
      weightOutFilePath = argv[6];
      outputFilePath = argv[7];
      datasetsPath = argv[8];
    }
  else
    {
      localeId = "es_ES";
      transferFilePath = "transferFile.t1x";
      sentenceFilePath = "spa-test.txt";
      lextorFilePath = "spa-test.lextor";
      transferOutFilePath = "transfer.out";
      weightOutFilePath = "weights.txt";
      outputFilePath = "output.out";
      datasetsPath = "datasetstry";
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
      vector<vector<string> > vouts;
      vector<vector<RuleExecution::AmbigInfo*> > vambigInfo;
      vector<vector<vector<RuleExecution::Node*> > > vcompNodes;

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
	  // number of generated combinations
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

	  vslTokens.push_back (slTokens);
	  vouts.push_back (outs);
	  vambigInfo.push_back (ambigInfo);
	  vcompNodes.push_back (combNodes);
	}

      // Read transfer sentences from transfer file
      vector<vector<string> > vtransfers;

      ifstream transferOutFile (transferOutFilePath.c_str ());
      if (transferOutFile.is_open ())
	for (unsigned i = 0; i < sourceSentences.size (); i++)
	  {
	    string line;
	    vector<string> transfers;
	    for (unsigned j = 0; j < vouts[i].size (); j++)
	      {
		getline (transferOutFile, line);
		transfers.push_back (line);
	      }

	    vtransfers.push_back (transfers);
	  }
      else
	cout << "error in opening weightOutFile" << endl;
      transferOutFile.close ();

      // Read weights from weights file
      vector<vector<float> > vweights;

      ifstream weightOutFile (weightOutFilePath.c_str ());
      if (weightOutFile.is_open ())
	for (unsigned i = 0; i < sourceSentences.size (); i++)
	  {
	    string line;
	    vector<float> weights;
	    for (unsigned j = 0; j < vouts[i].size (); j++)
	      {
		getline (weightOutFile, line);
		float weight = strtof (line.c_str (), NULL);
		weights.push_back (weight);
	      }
	    vweights.push_back (weights);
	  }
      else
	cout << "error in opening weightOutFile" << endl;
      weightOutFile.close ();

      // normalise the weights
      RuleExecution::normaliseWeights (&vweights, vambigInfo);

      // Yasmet format preparing
      // make a directory if not found
      mkdir (datasetsPath.c_str (), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

      for (unsigned g = 0; g < sourceSentences.size (); g++)
	{
	  vector<RuleExecution::AmbigInfo*> ambigInfo = vambigInfo[g];

	  vector<float> weights = vweights[g];

	  vector<string> slTokens = vslTokens[g];

	  unsigned weigInd = 0;
	  for (unsigned i = 0; i < ambigInfo.size (); i++)
	    {
	      RuleExecution::AmbigInfo* ambig = ambigInfo[i];

	      // name of the file is the concatenation of rules ids
	      string rulesNums;
	      for (unsigned x = 0; x < ambig->combinations.size (); x++)
		{
		  // avoid dummy node
		  for (unsigned y = 1; y < ambig->combinations[x].size (); y++)
		    {
		      stringstream ss;
		      ss << ambig->combinations[x][y]->ruleId;
		      rulesNums += ss.str ();

		      if (y + 1 < ambig->combinations[x].size ())
			rulesNums += "_";
		    }
		  rulesNums += "+";
		}

	      // if it's the first time to open , put the number of classes
	      bool firstTime = true;
	      if (FILE *file = fopen ((datasetsPath + string ("/") + rulesNums).c_str (),
				      "r"))
		{
		  firstTime = false;
		  fclose (file);
		}

	      ofstream dataset ((datasetsPath + string ("/") + rulesNums).c_str (),
				ofstream::out | ofstream::app);

	      if (firstTime)
		dataset << ambig->combinations.size () << endl;

	      for (unsigned x = 0; x < ambig->combinations.size (); x++)
		{

		  dataset << x << " $ ";

		  float weight = weights[x + weigInd];

		  dataset << weight << " #";

		  string features;
		  for (unsigned v = 0; v < ambig->combinations.size (); v++)
		    {
		      stringstream ss;
		      ss << v;
		      string label = ss.str ();

		      for (unsigned z = ambig->firTokId;
			  z < ambig->firTokId + ambig->maxPat; z++)
			{
			  stringstream ss;
			  ss << z - ambig->firTokId;
			  string num = ss.str ();
			  string word = CLExec::toLowerCase (slTokens[z], localeId);
			  for (unsigned c = 0; c < word.length (); c++)
			    if (word[c] == ' ')
			      word.replace (c, 1, "_");

			  features += " " + word + "_" + num + ":" + label;
			}
		      features += " #";
		    }
		  dataset << features << endl;
		}
	      weigInd += ambig->combinations.size ();
	      dataset.close ();
	    }
	}

      // Write output to output file
      ofstream outputFile (outputFilePath.c_str ());
      if (outputFile.is_open ())
	{
	  for (unsigned i = 0; i < sourceSentences.size (); i++)
	    {
	      outputFile
		  << "---------------------------------------------------------------------------------------------------------"
		  << endl << endl;
	      outputFile << "Analysis of sentence : " << endl;
	      outputFile << sourceSentences[i] << endl << endl << endl;

	      outputFile << endl;
	      outputFile << "sentence id ||| coverage id ||| original sentence |||"
		  << " lextor ||| rules ||| chunker ||| final sentence ||| score" << endl
		  << endl;

	      for (unsigned j = 0; j < vweights[i].size (); j++)
		{
		  // sentence id
		  outputFile << (i + 1) << " ||| ";
		  // coverage id
		  outputFile << (j + 1) << " ||| ";
		  // original sentence
		  outputFile << sourceSentences[i] << " ||| ";
		  // lextor
		  outputFile << tokenizedSentences[i] << " ||| ";
		  // rules
		  for (unsigned k = 0; k < vcompNodes[i][j].size (); k++)
		    outputFile << vcompNodes[i][j][k]->ruleId << " ";
		  outputFile << "||| ";
		  // chuncker
		  outputFile << vouts[i][j] << " ||| ";
		  // final sentence
		  outputFile << vtransfers[i][j] << " ||| ";
		  // score
		  outputFile << vweights[i][j] << endl << endl;
		}
	    }
	  outputFile.close ();
	}

      // Model weighting
      string modelWeightFilePath = "modelWeight.txt";
      ofstream modelWeightFile (modelWeightFilePath.c_str ());
      if (modelWeightFile.is_open ())
	{
	  modelWeightFile
	      << "---------------------------------------------------------------------------------------------------------"
	      << endl;
	  modelWeightFile << "Best sentences with model weighting" << endl;
	  modelWeightFile
	      << "---------------------------------------------------------------------------------------------------------"
	      << endl << endl << endl;
	  for (unsigned i = 0; i < sourceSentences.size (); i++)
	    {
	      modelWeightFile << (i + 1) << endl;
	      modelWeightFile << "Source :  " << sourceSentences[i] << endl << endl;

	      unsigned maxInd = 0;
	      for (unsigned j = 1; j < vweights[i].size (); j++)
		{
		  if (vweights[i][j] > vweights[i][maxInd])
		    maxInd = j;
		}

	      // final sentence
	      modelWeightFile << "Target :  " << vtransfers[i][maxInd] << endl;
	      // score
	      modelWeightFile << "Weight :  " << vweights[i][maxInd] << endl;
	      // rules
	      modelWeightFile << "Rules :  ";
	      for (unsigned k = 0; k < vcompNodes[i][maxInd].size (); k++)
		outputFile << vcompNodes[i][maxInd][k]->ruleId << " ";

	      modelWeightFile << endl
		  << "---------------------------------------------------------------------------------------------------------"
		  << endl << endl << endl;
	    }
	  modelWeightFile.close ();
	}

      // Random choosing
      string randomFilePath = "randomWeight.txt";
      ofstream randomFile (randomFilePath.c_str ());
      if (randomFile.is_open ())
	{
	  // seed for randomness
	  srand (time (NULL));
	  randomFile
	      << "---------------------------------------------------------------------------------------------------------"
	      << endl;
	  randomFile << "Random sentences" << endl;
	  randomFile
	      << "---------------------------------------------------------------------------------------------------------"
	      << endl << endl << endl;
	  for (unsigned i = 0; i < sourceSentences.size (); i++)
	    {
	      randomFile << (i + 1) << endl;
	      randomFile << "Source :  " << sourceSentences[i] << endl << endl;

	      int random = rand () % vweights[i].size ();

	      // final sentence
	      randomFile << "Target :  " << vtransfers[i][random] << endl;
	      // score
	      randomFile << "Weight :  " << vweights[i][random] << endl;
	      // rules
	      randomFile << "Rules :  ";
	      for (unsigned k = 0; k < vcompNodes[i][random].size (); k++)
		outputFile << vcompNodes[i][random][k]->ruleId << " ";

	      randomFile << endl
		  << "---------------------------------------------------------------------------------------------------------"
		  << endl << endl << endl;
	    }
	  randomFile.close ();
	}

    }
  else
    {
      cout << "ERROR in opening files!" << endl;
    }

  return 0;
}
