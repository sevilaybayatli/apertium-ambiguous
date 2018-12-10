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

      vector<vector<string> > vslTokens;
      vector<vector<string> > vouts;
      vector<vector<vector<unsigned> > > vrulesIds;
      vector<vector<vector<vector<xml_node> > > > voutsRules;
      vector<
	  vector<
	      pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<xml_node> > > > > > vambigInfo;
      vector<vector<vector<unsigned> > > vweigInds;

      map<string, vector<vector<string> > > attrs = RuleParser::getAttrs (transfer);
      map<string, string> vars = RuleParser::getVars (transfer);
      map<string, vector<string> > lists = RuleParser::getLists (transfer);

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
	  map<int, vector<string> > catsApplied;

	  RuleParser::matchCats (&catsApplied, slTokens, slTags, transfer);

	  // map of matched rules and tokens id
	  map<xml_node, vector<vector<int> > > rulesApplied;

	  RuleParser::matchRules (&rulesApplied, slTokens, catsApplied, transfer);

	  // rule and (target) token map to specific output
	  // if rule has many patterns we will choose the first token only
	  map<xml_node, map<int, vector<string> > > ruleOutputs;

	  // map (target) token to all matched rules ids and the number of pattern items of each rule
	  map<int, vector<pair<xml_node, unsigned> > > tokenRules;

	  RuleExecution::ruleOuts (&ruleOutputs, &tokenRules, slTokens, slTags, tlTokens,
				   tlTags, rulesApplied, attrs, lists, &vars, spaces,
				   localeId);

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

	  //	  cout << "outs size = " << outs.size() << endl;
	  //	  cout << endl << endl;
	  //	  for (unsigned j = 0; j < outs.size (); j++)
	  //	    cout << outs[j] << endl;
	  //	  cout << endl;
	  //
	  //	  for (unsigned j = 0; j < rulesIds.size (); j++)
	  //	    {
	  //	      for (unsigned k = 0; k < rulesIds[j].size (); k++)
	  //		{
	  //		  cout << rulesIds[j][k] << " ";
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
	  //		      cout << outsRules[j][k][h].attribute (ID).value () << "-";
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
	  //	  for (map<xml_node, vector<vector<int> > >::iterator it = rulesApplied.begin ();
	  //	      it != rulesApplied.end (); it++)
	  //	    {
	  //	      xml_node rule = it->first;
	  //	      vector<vector<int> > tokenIds = it->second;
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

      // Read transfer sentences from tansfer file
      vector<string> vtransfers[sourceSentences.size ()];

      ifstream transferOutFile (transferOutFilePath.c_str ());
      if (transferOutFile.is_open ())
	for (unsigned i = 0; i < sourceSentences.size (); i++)
	  {
	    string line;
	    vector<string> outs = vouts[i];
	    vector<string> transfers;
	    for (unsigned j = 0; j < outs.size (); j++)
	      {
		getline (transferOutFile, line);
		transfers.push_back (line);
	      }

	    vtransfers[i] = transfers;
	  }
      else
	cout << "error in opening weightOutFile" << endl;
      transferOutFile.close ();

      // Read weights from weights file
      vector<float> vweights[sourceSentences.size ()];

      ifstream weightOutFile (weightOutFilePath.c_str ());
      if (weightOutFile.is_open ())
	for (unsigned i = 0; i < sourceSentences.size (); i++)
	  {
	    string line;
	    vector<string> outs = vouts[i];
	    vector<float> weights;
	    float sum = 0;
	    for (unsigned j = 0; j < outs.size (); j++)
	      {
		getline (weightOutFile, line);
		float weight = strtof (line.c_str (), NULL);
		weights.push_back (weight);
		sum += weight;
	      }

	    // to avoid nans
	    if (sum == 0)
	      for (unsigned j = 0; j < outs.size (); j++)
		{
		  weights[j] = 1 / outs.size ();
		}

	    else
	      for (unsigned j = 0; j < outs.size (); j++)
		{

		  weights[j] /= sum;
		}

	    vweights[i] = weights;
	  }
      else
	cout << "error in opening weightOutFile" << endl;
      weightOutFile.close ();

      // Yasmet format preparing
      mkdir (datasetsPath.c_str (), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

      for (unsigned g = 0; g < sourceSentences.size (); g++)
	{
	  vector<
	      pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<xml_node> > > > > ambigInfo =
	      vambigInfo[g];

	  vector<vector<unsigned> > weigInds = vweigInds[g];

	  vector<float> weights = vweights[g];

	  vector<string> slTokens = vslTokens[g];

	  vector<vector<unsigned> >::iterator weigIndIt = weigInds.begin ();
	  for (unsigned i = 0; i < ambigInfo.size (); i++)
	    {

	      pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<xml_node> > > > p1 =
		  ambigInfo[i];
	      pair<unsigned, unsigned> p2 = p1.first;
	      vector<vector<xml_node> > ambigRules = p1.second.second;

	      // name of the file is the concatenation of rules ids
	      string rulesNums;
	      for (unsigned x = 0; x < ambigRules.size (); x++)
		{
		  for (unsigned y = 0; y < ambigRules[x].size (); y++)
		    {
		      rulesNums += ambigRules[x][y].attribute (ID).value ();
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
		dataset << ambigRules.size () << endl;

	      for (unsigned x = 0; x < ambigRules.size (); x++)
		{

		  dataset << x << " $ ";

		  vector<unsigned> weigInd = *weigIndIt++;
		  float count = 0;

		  for (unsigned z = 0; z < weigInd.size (); z++)
		    {
		      count += weights[weigInd[z]];
		    }

		  dataset << count << " #";

		  string features;
		  for (unsigned v = 0; v < ambigRules.size (); v++)
		    {
		      char label[21];
		      sprintf (label, "%d", v);

		      for (unsigned z = p2.first; z < p2.first + p2.second; z++)
			{
			  char num[21];
			  sprintf (num, "%d", z - p2.first);
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
		  for (unsigned k = 0; k < vrulesIds[i][j].size (); k++)
		    outputFile << vrulesIds[i][j][k] << " ";
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
	      for (unsigned k = 0; k < vrulesIds[i][maxInd].size (); k++)
		modelWeightFile << vrulesIds[i][maxInd][k] << " ";

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
	      for (unsigned k = 0; k < vrulesIds[i][random].size (); k++)
		randomFile << vrulesIds[i][random][k] << " ";

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
