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

#include "../pugixml/pugixml.hpp"
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
  string inFilePath = "wiki.txt", preProcessFilePath = "preProcess.txt",
      sentenceFilePath = "sentences.txt", biltransFilePath = "biltrans.txt",
      lextorFilePath = "lextor.txt", outputFilePath = "default.out", interInFilePath =
	  "interchunkIn.txt", interOutFilePath = "interchunkOut.txt", postOutFilePath =
	  "postchunkOut.txt", transferOutFilePath = "transferOut.txt", weightOutFilePath =
	  "weightOut.txt", beamFilePath = "beamResults.txt";

  if (argc > 2)
    {
      inFilePath = argv[1];
      outputFilePath = argv[2];
      if (argc == 3)
	cout << "Yasmet training mode" << endl;
      else if (argc == 4)
	cout << "Beam search mode" << endl;
    }
  else
    {
      cout << "Error!! Please pass at least two arguments , input and output." << endl;
      return -1;
    }

  cout << "Input file : " << inFilePath << endl;
  cout << "Output file : " << outputFilePath << endl;

// start timer
  struct timeval stop, start;
  gettimeofday (&start, NULL);

  cout << "Pre-process corpus started" << endl;

  ifstream inFile (inFilePath.c_str ());
  ofstream preProcessFile (preProcessFilePath.c_str ());

  // preprossing
  if (inFile.is_open () && preProcessFile.is_open ())
    {
      string line;
      string latin = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
      while (getline (inFile, line))
	{
	  // remove double quotes
	  for (unsigned i = 0; i < line.size (); i++)
	    {
	      if (line[i] == '"' || line[i] == '[' || line[i] == ']' || line[i] == '{'
		  || line[i] == '}' || line[i] == '<' || line[i] == '>' || line[i] == '('
		  || line[i] == ')' || line[i] == '/' || line[i] == '|' || line[i] == '\\'
		  || line[i] == '$')
		{
		  line.erase (i, 1);
		  i--;
		}
	    }
	  preProcessFile << line << endl;
	}

      inFile.close ();
      preProcessFile.close ();
    }
  else
    {
      cout << "No file found!" << endl;
    }
  cout << "Pre-process corpus finished" << endl;

  cout << "Breaking corpus into sentences started" << endl;
// then segment the line by sentences
  CLExec::segmenter (preProcessFilePath, sentenceFilePath);
  cout << "Breaking corpus into sentences finished" << endl;

// do the biltrans for all sentences
  cout << "biltrans of all sentences started" << endl;
  CLExec::biltrans (sentenceFilePath, biltransFilePath);
  cout << "biltrans of all sentences finished" << endl;

// do the lextor for all sentences
  cout << "Lextor of all sentences started" << endl;
  CLExec::lextor (biltransFilePath, lextorFilePath);
  cout << "Lextor of all sentences finished" << endl;

  cout << "Sentences Analysis started" << endl;
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
      cout << "Sentences Analysis finished" << endl;

      cout << "Interchunk started" << endl;
      CLExec::interchunk (interInFilePath, interOutFilePath);
      cout << "Interchunk finished" << endl;

      cout << "Postchunk started" << endl;
      CLExec::postchunk (interOutFilePath, postOutFilePath);
      cout << "Postchunk finished" << endl;

      cout << "Transfer started" << endl;
      CLExec::transfer (postOutFilePath, transferOutFilePath);
      cout << "Transfer finished" << endl;

      cout << "Model scoring started" << endl;
      CLExec::assignWeights (transferOutFilePath, weightOutFilePath);
      cout << "Model scoring finished" << endl;

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
	    for (unsigned j = 0; j < outs.size (); j++)
	      {
		weights[j] /= sum;
	      }

	    vweights[i] = weights;
	  }
      else
	cout << "error in opening weightOutFile" << endl;
      weightOutFile.close ();

      // prepare yasmet datasets
      if (argc == 3)
	{
	  cout << "Yasmet training started" << endl;
	  for (unsigned g = 0; g < sourceSentences.size (); g++)
	    {
	      vector<
		  pair<pair<unsigned, unsigned>,
		      pair<unsigned, vector<vector<xml_node> > > > > ambigInfo =
		  vambigInfo[g];

	      vector<vector<unsigned> > weigInds = vweigInds[g];

	      vector<float> weights = vweights[g];

	      vector<string> slTokens = vslTokens[g];

	      vector<vector<unsigned> >::iterator weigIndIt = weigInds.begin ();
	      for (unsigned i = 0; i < ambigInfo.size (); i++)
		{

		  pair<pair<unsigned, unsigned>,
		      pair<unsigned, vector<vector<xml_node> > > > p1 = ambigInfo[i];
		  pair<unsigned, unsigned> p2 = p1.first;
		  vector<vector<xml_node> > ambigRules = p1.second.second;

		  // name of the file is the concatenation of rules ids
		  string rulesNums;
		  for (unsigned x = 0; x < ambigRules.size (); x++)
		    {
		      for (unsigned y = 0; y < ambigRules[x].size (); y++)
			{
			  xml_node rule = ambigRules[x][y];
			  string ruleCmnt = rule.first_attribute ().value ();

			  rulesNums += ruleCmnt.substr (ruleCmnt.find_last_of ("-") + 1);
			  rulesNums += "_";

			}
		      rulesNums += "+";
		    }

		  // if it's the first time to open , put the number of classes
		  bool firstTime = true;
		  if (FILE *file = fopen ((DATASETS + string ("/") + rulesNums).c_str (),
					  "r"))
		    {
		      firstTime = false;
		      fclose (file);
		    }

		  ofstream dataset ((DATASETS + string ("/") + rulesNums).c_str (),
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
			      string word = CLExec::toLowerCase (slTokens[z]);
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
	  cout << "Yasmet training finished" << endl;
	}

      // beamSearch
      if (argc == 4)
	{
	  cout << "Beam search started" << endl;

//	  CLExec::runYasmet ();

	  // load yasmet models data
	  map<string, map<string, vector<float> > > classesWeights =
	      CLExec::loadYasmetModels ();

	  int beam;
	  istringstream buffer (argv[3]);
	  buffer >> beam;

	  vector<pair<unsigned, float> > bestTransInds;

	  for (unsigned i = 0; i < sourceSentences.size (); i++)
	    {
	      vector<pair<vector<unsigned>, float> > beamTree;
	      CLExec::beamSearch (&beamTree, beam, vslTokens[i], vambigInfo[i],
				  classesWeights);

	      vector<pair<unsigned, float> > transInds;
	      CLExec::getTransInds (&transInds, beamTree, vrulesIds[i]);

	      // Take the best translation index and weight only
	      bestTransInds.push_back (transInds[0]);
	    }

	  // write best translations to file
	  ofstream beamFile (beamFilePath.c_str ());
	  if (beamFile.is_open ())
	    {
	      beamFile << "Beam search algorithm results with beam = " << beam << endl;
	      beamFile << "-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-"
		  << endl << endl << endl;
	      for (unsigned i = 0; i < sourceSentences.size (); i++)
		{
		  beamFile << (i + 1) << endl;
		  beamFile << "Source : " << sourceSentences[i] << endl;
		  beamFile << "Target : " << vtransfers[i][bestTransInds[i].first]
		      << endl;
		  if (bestTransInds[i].second == 0)
		    beamFile << "Weight : " << 1 << endl;
		  else
		    beamFile << "Weight : " << bestTransInds[i].second << endl;
		  beamFile << "--------------------------------------------------" << endl
		      << endl;
		}
	      beamFile.close ();
	    }

	  cout << "Beam search finished" << endl;
	}

      // Write sentence analysis
      /*ofstream outputFile (outputFilePath.c_str ());
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
       }*/
    }
  else
    {
      cout << "ERROR in opening files!" << endl;
    }

  cout << endl << "-----------------------------------------------------" << endl << endl;
  cout << "Finished" << endl;

  gettimeofday (&stop, NULL);
  cout << "\nTime taken :  "

  << (stop.tv_sec - start.tv_sec + (stop.tv_usec - start.tv_usec) / 1000000.0)
      << "  seconds" << endl;

  return 0;
}
