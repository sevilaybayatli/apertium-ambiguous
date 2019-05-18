//#include <stdio.h>
//#include <iostream>
//#include <fstream>
//#include <stdexcept>
//#include <unistd.h>
//#include <stdlib.h>
//#include <string>
//#include <map>
//#include <vector>
//#include <string.h>
//#include <sys/stat.h>
//#include <time.h>
//#include <sys/time.h>
//#include <sstream>
//#include <algorithm>
//
//#include "../pugixml/pugixml.hpp"
//#include "RuleParser.h"
//#include "RuleExecution.h"
//#include "TranElemLiterals.h"
//#include "CLExec.h"
//
//#include <omp.h>
//
//using namespace std;
//using namespace pugi;
//using namespace elem;
//
//int
//main (int argc, char **argv)
//{
//  string sentenceFilePath, lextorFilePath, localeId, transferFilePath, modelsDest,
//      beamSize, transferOutFilePath, beamOutFilePath;
//
//  if (argc == 9)
//    {
//      localeId = argv[1];
//      transferFilePath = argv[2];
//      sentenceFilePath = argv[3];
//      lextorFilePath = argv[4];
//
//      transferOutFilePath = argv[5];
//      beamOutFilePath = argv[6];
//
//      modelsDest = argv[7];
//      beamSize = argv[8];
//    }
//  else
//    {
////      localeId = "es_ES";
////      transferFilePath = "transferFile.t1x";
////      sentenceFilePath = "spa-test.txt";
////      lextorFilePath = "spa-test.lextor";
////      interInFilePath = "beaminter.out";
////      modelsDest = "modelstry";
////      k = "8";
//
//      localeId = "kk_KZ";
//      transferFilePath = "apertium-kaz-tur.kaz-tur.t1x";
//      sentenceFilePath = "src.txt";
//      lextorFilePath = "lextor.txt";
//
//      transferOutFilePath = "beam-transfer.txt";
//      beamOutFilePath = "beamOutFile.txt";
//
//      modelsDest = "./UntitledFolder/models";
//      beamSize = "8";
//
//      cout << "Error in parameters !" << endl;
//      cout
//	  << "Parameters are : localeId transferFilePath sentenceFilePath lextorFilePath transferOutFilePath beamOutFilePath modelsDest beamSize"
//	  << endl;
//      cout << "localeId : ICU locale ID for the source language. For Kazakh => kk-KZ"
//	  << endl;
//      cout << "transferFilePath : Apertium transfer file of the language pair used."
//	  << endl;
//      cout << "sentenceFilePath : Source language sentences file." << endl;
//      cout << "lextorFilePath : Apertium lextor file for the source language sentences."
//	  << endl;
//      cout
//	  << "transferOutFilePath : Output file of apertium transfer for the source language sentences."
//	  << endl;
//      cout
//	  << "beamOutFilePath : Output file name of this program which is the best translations for the language sentences."
//	  << endl;
//      cout << "modelsDest : Yasmet models destination." << endl;
//      cout << "beamSize : The size of beam in beam search algorithm." << endl;
//      return -1;
//    }
//
//  // seed for randomness
//  srand (time (NULL));
//
//  ifstream lextorFile (lextorFilePath.c_str ());
//  ifstream inSentenceFile (sentenceFilePath.c_str ());
//  if (lextorFile.is_open () && inSentenceFile.is_open ())
//    {
//      // load transfer file in an xml document object
//      xml_document transferDoc;
//      xml_parse_result result = transferDoc.load_file (transferFilePath.c_str ());
//
//      if (string (result.description ()) != "No error")
//	{
//	  cout << "ERROR : " << result.description () << endl;
//	  return -1;
//	}
//
//      // xml node of the parent node (transfer) in the transfer file
//      xml_node transfer = transferDoc.child ("transfer");
//
//      vector<string> sourceSentences, tokenizedSentences;
//
//      string tokenizedSentence;
//      while (getline (lextorFile, tokenizedSentence))
//	{
//	  string sourceSentence;
//	  if (!getline (inSentenceFile, sourceSentence))
//	    sourceSentence = "No more sentences";
//
//	  sourceSentences.push_back (sourceSentence);
//	  tokenizedSentences.push_back (tokenizedSentence);
//	}
//      lextorFile.close ();
//      inSentenceFile.close ();
//
//      map<string, vector<vector<string> > > attrs = RuleParser::getAttrs (transfer);
//      map<string, string> vars = RuleParser::getVars (transfer);
//      map<string, vector<string> > lists = RuleParser::getLists (transfer);
//
//      map<string, map<string, vector<float> > > classesWeights =
//	  CLExec::loadYasmetModels (modelsDest);
//
////      vector<vector<string> > vouts;
//
//      int beam;
//      stringstream buffer (beamSize);
//      buffer >> beam;
//
//      // empty the output file
//      ofstream beamFile (beamOutFilePath.c_str ());
//      beamFile.close ();
//
//      ifstream transferOutFile (transferOutFilePath.c_str ());
//
//      if (transferOutFile.is_open ())
//	for (unsigned i = 0; i < sourceSentences.size (); i++)
//	  {
//	    cout << i << endl;
//
//	    string sourceSentence, tokenizedSentence;
//	    sourceSentence = sourceSentences[i];
//	    tokenizedSentence = tokenizedSentences[i];
//
//	    // spaces after each token
//	    vector<string> spaces;
//
//	    // tokens in the sentence order
//	    vector<string> slTokens, tlTokens;
//
//	    // tags of tokens in order
//	    vector<vector<string> > slTags, tlTags;
//
//	    RuleParser::sentenceTokenizer (&slTokens, &tlTokens, &slTags, &tlTags,
//					   &spaces, tokenizedSentence);
//
//	    // map of tokens ids and their matched categories
//	    map<unsigned, vector<string> > catsApplied;
//
//	    RuleParser::matchCats (&catsApplied, slTokens, slTags, transfer);
//
//	    // map of matched rules and a pair of first token id and patterns number
//	    map<xml_node, vector<pair<unsigned, unsigned> > > rulesApplied;
//
//	    RuleParser::matchRules (&rulesApplied, slTokens, catsApplied, transfer);
//
//	    // rule and (target) token map to specific output
//	    // if rule has many patterns we will choose the first token only
//	    map<unsigned, map<unsigned, string> > ruleOutputs;
//
//	    // map (target) token to all matched rules ids and the number of pattern items of each rule
//	    map<unsigned, vector<pair<unsigned, unsigned> > > tokenRules;
//
//	    RuleExecution::ruleOuts (&ruleOutputs, &tokenRules, slTokens, slTags,
//				     tlTokens, tlTags, rulesApplied, attrs, lists, &vars,
//				     spaces, localeId);
//
//	    // final outputs
//	    vector<string> outs;
//	    // number of generated combinations
//	    unsigned compNum;
//	    // nodes for every token and rule
//	    map<unsigned, vector<RuleExecution::Node*> > nodesPool;
//	    // ambiguous informations
//	    vector<RuleExecution::AmbigInfo*> ambigInfo;
//	    // beam tree
//	    vector<pair<vector<RuleExecution::Node*>, float> > beamTree;
//	    // rules combinations
//	    vector<vector<RuleExecution::Node*> > combNodes;
//
//	    nodesPool = RuleExecution::getNodesPool (tokenRules);
//
//	    RuleExecution::getAmbigInfo (tokenRules, nodesPool, &ambigInfo, &compNum);
//
//	    vector<RuleExecution::AmbigInfo*> newAmbigInfo;
//	    for (unsigned j = 0; j < ambigInfo.size (); j++)
//	      if (ambigInfo[j]->combinations.size () > 1)
//		newAmbigInfo.push_back (ambigInfo[j]);
//
//	    CLExec::beamSearch (&beamTree, beam, slTokens, newAmbigInfo, classesWeights,
//				localeId);
//
//	    RuleExecution::getOuts (&outs, &combNodes, beamTree, nodesPool, ruleOutputs,
//				    spaces);
//
//	    // read transfer
//	    string line;
//	    vector<string> beamTransfers;
//	    for (unsigned j = 0; j < outs.size (); j++)
//	      {
//		getline (transferOutFile, line);
//		beamTransfers.push_back (line);
//	      }
//
//	    // write beam results
//	    ofstream beamFile (beamOutFilePath.c_str (), ofstream::app);
//	    if (beamFile.is_open ())
//	      {
//		beamFile << "source sentence (" << (i + 1) << ") : " << endl;
//		beamFile << sourceSentence << endl << endl;
//		// just take first best
//		for (unsigned j = 0; j < /*outs.size ()*/1; j++)
//		  {
//		    beamFile << "target sentence " /*<< (j + 1)*/<< " : " << endl;
//		    beamFile << beamTransfers[j] << endl;
//		    beamFile << "weight = " << beamTree[j].second << endl;
//		    beamFile << "rules : ";
//		    for (unsigned k = 0; k < combNodes[j].size (); k++)
//		      if (combNodes[j][k]->ruleId)
//			beamFile << combNodes[j][k]->ruleId << " ";
//		    beamFile << endl << endl;
//		    beamFile
//			<< "------------------------------------------------------------------"
//			<< endl << endl;
//		  }
//	      }
//	    beamFile.close ();
//	  }
//      else
//	{
//	  cout << "ERROR in opening files!" << endl;
//	}
//      transferOutFile.close ();
//    }
//  else
//    {
//      cout << "ERROR in opening files!" << endl;
//    }
//  return 0;
//}
