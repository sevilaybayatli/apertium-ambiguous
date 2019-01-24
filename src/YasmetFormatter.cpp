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
      transferOutFilePath = "transfer.txt", weightOutFilePath = "weights.txt", localeId =
	  "kk_KZ", transferFilePath = "transferFile.tx1", datasetsPath = "datasets";

  if (argc == 8)
    {
      localeId = argv[1];
      transferFilePath = argv[2];
      sentenceFilePath = argv[3];
      lextorFilePath = argv[4];
      transferOutFilePath = argv[5];
      weightOutFilePath = argv[6];
      datasetsPath = argv[7];
    }
  else
    {
//      localeId = "es_ES";
//      transferFilePath = "transferFile.t1x";
//      sentenceFilePath = "spa-test.txt";
//      lextorFilePath = "spa-test.lextor";
//      transferOutFilePath = "transfer.out";
//      weightOutFilePath = "weights.txt";
//      outputFilePath = "output.out";
//      datasetsPath = "datasetstry2";

//./yasmet-formatter $localeId sentences.txt lextor.txt transfer.txt weights.txt $outputFile $datasets;
      localeId = "kk_KZ";
      transferFilePath = "apertium-kaz-tur.kaz-tur.t1x";
      sentenceFilePath = "sample-sentences.txt";
      lextorFilePath = "sample-lextor.txt";
      transferOutFilePath = "norm-transfer.txt";
      weightOutFilePath = "norm-weights.txt";
      datasetsPath = "datasetstry1234";

      cout << "Error in parameters !" << endl;
      cout
	  << "Parameters are : localeId transferFilePath sentenceFilePath lextorFilePath transferOutFilePath weightOutFilePath datasetsPath"
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
      cout << "datasetsPath : Datasets destination to put in the generated yasmet files."
	  << endl;
      return -1;
    }

  ifstream lextorFile (lextorFilePath.c_str ());
  ifstream inSentenceFile (sentenceFilePath.c_str ());
  if (lextorFile.is_open () && inSentenceFile.is_open ())
    {
      // load transfer file in an xml document object
      xml_document* transferDoc = new xml_document ();
      xml_parse_result result = transferDoc->load_file (transferFilePath.c_str ());

      if (string (result.description ()) != "No error")
	{
	  cout << "ERROR : " << result.description () << endl;
	  return -1;
	}

      // xml node of the parent node (transfer) in the transfer file
      xml_node transfer = transferDoc->child ("transfer");

      vector<string> *sourceSentences = new vector<string> (), *tokenizedSentences =
	  new vector<string> ();

      string tokenizedSentence;
      //unsigned i = 0;
      while (getline (lextorFile, tokenizedSentence))
	{
	  string sourceSentence;
	  if (!getline (inSentenceFile, sourceSentence))
	    sourceSentence = "No more sentences";

	  sourceSentences->push_back (sourceSentence);
	  tokenizedSentences->push_back (tokenizedSentence);
	  //if (i == 100)
	  //  break;
	  //i++;
	}
      lextorFile.close ();
      inSentenceFile.close ();

      map<string, vector<vector<string> > > attrs = RuleParser::getAttrs (transfer);
      map<string, string> vars = RuleParser::getVars (transfer);
      map<string, vector<string> > lists = RuleParser::getLists (transfer);

//      vector<vector<string> >* vslTokens = new vector<vector<string> > ();
//      vector<unsigned> vouts;
//      vector<vector<RuleExecution::AmbigInfo> >* vambigInfo = new vector<
//	  vector<RuleExecution::AmbigInfo> > ();
//      vector<vector<vector<RuleExecution::Node> > > vcompNodes;

      ifstream weightOutFile (weightOutFilePath.c_str ());
      if (weightOutFile.is_open ())
	for (unsigned i = 0; i < sourceSentences->size (); i++)
	  {
	    cout << i << endl;

	    string sourceSentence, tokenizedSentence;
	    sourceSentence = (*sourceSentences)[i];
	    tokenizedSentence = (*tokenizedSentences)[i];

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
	    // number of generated combinations
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

	    vector<RuleExecution::AmbigInfo> newAmbigInfo;
	    for (unsigned j = 0; j < ambigInfo.size (); j++)
	      if (ambigInfo[j].combinations.size () > 1)
		newAmbigInfo.push_back (ambigInfo[j]);
	    ambigInfo = newAmbigInfo;

	    // read weights
	    string line;
	    vector<float> weights;
	    weights.reserve (1000);
	    for (unsigned j = 0; j < outs.size (); j++)
	      {
		getline (weightOutFile, line);
		float weight = strtof (line.c_str (), NULL);
		weights.push_back (weight);
	      }

	    RuleExecution::normaliseWeights (&weights, ambigInfo);

	    // Yasmet format preparing
	    // make a directory if not found
	    mkdir (datasetsPath.c_str (), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	    unsigned weigInd = 0;
	    for (unsigned i = 0; i < ambigInfo.size (); i++)
	      {
		RuleExecution::AmbigInfo ambig = ambigInfo[i];

		// name of the file is the concatenation of rules ids
		string rulesNums;
		for (unsigned x = 0; x < ambig.combinations.size (); x++)
		  {
		    // avoid dummy node
		    for (unsigned y = 1; y < ambig.combinations[x].size (); y++)
		      {
			stringstream ss;
//			    ss->clear ();
			ss << ambig.combinations[x][y].ruleId;
			rulesNums += ss.str ();

			if (y + 1 < ambig.combinations[x].size ())
			  rulesNums += "_";
		      }
		    rulesNums += "+";
		  }

		// if it's the first time to open , put the number of classes
		bool firstTime = true;
		if (FILE *file = fopen (
		    (datasetsPath + string ("/") + rulesNums).c_str (), "r"))
		  {
		    firstTime = false;
		    fclose (file);
		  }

//		    stringstream* dataset = new stringstream ();
		ofstream dataset ((datasetsPath + string ("/") + rulesNums).c_str (),
				  ofstream::app);

		if (firstTime)
		  dataset << ambig.combinations.size () << endl;

		for (unsigned x = 0; x < ambig.combinations.size (); x++)
		  {

		    dataset << x << " $ ";

		    float weight = weights[x + weigInd];

		    dataset << weight << " #";

		    string features;
		    for (unsigned v = 0; v < ambig.combinations.size (); v++)
		      {
			stringstream ss;
//			    ss.clear ();
			ss << v;
			string label = ss.str ();

			for (unsigned z = ambig.firTokId;
			    z < ambig.firTokId + ambig.maxPat; z++)
			  {
			    stringstream ss;
//				ss->clear ();
			    ss << z - ambig.firTokId;
			    string num = ss.str ();
//			  *num = ss->str ();
			    string word = CLExec::toLowerCase (slTokens[z], localeId);

			    for (unsigned c = 0; c < word.length (); c++)
			      if (word[c] == ' ')
				word.replace (c, 1, "_");

			    features += " " + word + "_" + num + ":" + label;
			  }
			features += " #";
		      }
		    dataset << features << endl;
//		  delete (features);
		  }
		weigInd += ambig.combinations.size ();
//	      dataset.close ();
	      }
//	      }
	  }
      weightOutFile.close ();

    }
  else
    {
      cout << "ERROR in opening files!" << endl;
    }

  return 0;
}
