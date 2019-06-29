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

#include "pugixml.hpp"
#include "RuleParser.h"
#include "RuleExecution.h"
#include "TranElemLiterals.h"
#include "CLExec.h"

using namespace std;
using namespace pugi;
using namespace elem;

int main(int argc, char **argv) {
	string localeId, transferFilePath, lextorFilePath, chunkerFilePath,
			newLextorFilePath;

	bool newline = false;
	int opt;
	while ((opt = getopt(argc, argv, ":u:n")) != -1) {
		switch (opt) {
		case 'u':
			newLextorFilePath = optarg;
			break;
		case 'n':
			newline = true;
			break;
		case ':':
			printf("option %c needs a value\n", optopt);
			return -1;
		case '?':
			printf("unknown option: %c\n", optopt);
			return -1;
		}
	}

	if (argc - optind == 4) {
		localeId = argv[argc - 4];
		transferFilePath = argv[argc - 3];
		lextorFilePath = argv[argc - 2];
		chunkerFilePath = argv[argc - 1];
	} else {
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
		lextorFilePath =
				"/home/aboelhamd/eclipse-workspace/machinetranslation/test-lextor.txt";
		chunkerFilePath =
				"/home/aboelhamd/eclipse-workspace/machinetranslation/test-chunker.txt";

		cout << "Error in parameters !" << endl;
		cout << "Parameters are : localeId transferFilePath"
				<< " lextorFilePath chunkerFilePath [-u newlextorFilePath] [-n]"
				<< endl;
		cout
				<< "localeId : ICU locale ID for the source language. For Kazakh => kk_KZ"
				<< endl;
		cout
				<< "transferFilePath : Apertium transfer file of the language pair used."
				<< endl;
		cout
				<< "lextorFilePath : Apertium lextor file for the source language sentences."
				<< endl;
		cout
				<< "chunkerFilePath : chunker file name of this program which is the input for apertium interchunk."
				<< endl;
		cout << "-u : remove sentences with unknown words." << endl;
		cout
				<< "newlextorFilePath : write the new sentences lextor in this lextor file."
				<< endl;
		cout
				<< "-n : put newline after each sentence ambiguous chunker (to use it removing bad sentences)."
				<< endl;
		return -1;
	}

	ifstream lextorFile(lextorFilePath.c_str());
	ofstream chunkerFile(chunkerFilePath.c_str());
	ofstream newLextorFile(newLextorFilePath.c_str());
	if (lextorFile.is_open() && chunkerFile.is_open()
			&& (newLextorFilePath.empty() || newLextorFile.is_open())) {
		// load transfer file in an xml document object
		xml_document transferDoc;
		xml_parse_result result = transferDoc.load_file(
				transferFilePath.c_str());

		if (string(result.description()) != "No error") {
			cout << "ERROR : " << result.description() << endl;
			return -1;
		}

		// xml node of the parent node (transfer) in the transfer file
		xml_node transfer = transferDoc.child("transfer");

		map<string, vector<vector<string> > > attrs = RuleParser::getAttrs(
				transfer);
		map<string, string> vars = RuleParser::getVars(transfer);
		map<string, vector<string> > lists = RuleParser::getLists(transfer);

		unsigned allSents = 0, goodSents = 0;
		string tokenizedSentence;
		while (getline(lextorFile, tokenizedSentence)) {
			allSents++;
			if (!newLextorFilePath.empty()
					&& tokenizedSentence.find("^*") != string::npos)
				continue;
			goodSents++;
			// write to new lextor file
			newLextorFile << tokenizedSentence << endl;

			// cout << i++ << endl;

			// spaces after each token
			vector<string> spaces;

			// tokens in the sentence order
			vector<string> slTokens, tlTokens;

			// tags of tokens in order
			vector<vector<string> > slTags, tlTags;

			RuleParser::sentenceTokenizer(&slTokens, &tlTokens, &slTags,
					&tlTags, &spaces, tokenizedSentence);

			// map of tokens ids and their matched categories
			map<unsigned, vector<string> > catsApplied;

			RuleParser::matchCats(&catsApplied, slTokens, slTags, transfer);

			// map of matched rules and a pair of first token id and patterns number
			map<xml_node, vector<pair<unsigned, unsigned> > > rulesApplied;

			RuleParser::matchRules(&rulesApplied, slTokens, catsApplied,
					transfer);

			// rule and (target) token map to specific output
			// if rule has many patterns we will choose the first token only
			map<unsigned, map<unsigned, string> > ruleOutputs;

			// map (target) token to all matched rules ids and the number of pattern items of each rule
			map<unsigned, vector<pair<unsigned, unsigned> > > tokenRules;

			RuleExecution::ruleOuts(&ruleOutputs, &tokenRules, slTokens, slTags,
					tlTokens, tlTags, rulesApplied, attrs, lists, &vars, spaces,
					localeId);
			// final outs
			vector<string> outs;
			// number of possible combinations
			unsigned compNum;
			// nodes for every token and rule
			map<unsigned, vector<RuleExecution::Node*> > nodesPool;
			// ambiguous informations
			vector<RuleExecution::AmbigInfo*> ambigInfo;

			// rules combinations
			vector<vector<RuleExecution::Node*> > combNodes;

			nodesPool = RuleExecution::getNodesPool(tokenRules);

			RuleExecution::getAmbigInfo(tokenRules, nodesPool, &ambigInfo,
					&compNum);
			RuleExecution::getOuts(&outs, &combNodes, ambigInfo, nodesPool,
					ruleOutputs, spaces);

			// write the outs
			for (unsigned j = 0; j < outs.size(); j++) {
				chunkerFile << outs[j] << endl;
			}
			if (newline)
				chunkerFile << endl;

			// delete AmbigInfo pointers
			for (unsigned j = 0; j < ambigInfo.size(); j++) {
				// delete the dummy node pointers
				set<RuleExecution::Node*> dummies;
				for (unsigned k = 0; k < ambigInfo[j]->combinations.size(); k++)
					dummies.insert(ambigInfo[j]->combinations[k][0]);
				for (set<RuleExecution::Node*>::iterator it = dummies.begin();
						it != dummies.end(); it++)
					delete (*it);

				delete ambigInfo[j];
			}
			// delete Node pointers
			for (map<unsigned, vector<RuleExecution::Node*> >::iterator it =
					nodesPool.begin(); it != nodesPool.end(); it++) {
				for (unsigned j = 0; j < it->second.size(); j++) {
					delete it->second[j];
				}
			}
		}

		lextorFile.close();
		chunkerFile.close();
		newLextorFile.close();

		cout << "There are " << goodSents << " good sentences from " << allSents
				<< endl;
	} else {
		cout << "ERROR in opening files!" << endl;
	}

	return 0;
}
