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
#include <algorithm>

#include "pugixml.hpp"
#include "RuleParser.h"
#include "RuleExecution.h"
#include "TranElemLiterals.h"
#include "CLExec.h"

#include <omp.h>

using namespace std;
using namespace pugi;
using namespace elem;

int main(int argc, char **argv) {
	string sentenceFilePath, lextorFilePath, localeId, transferFilePath,
			targetFilePath, weightFilePath, analysisFilePath, bestModFilePath,
			randModFilePath;

	int opt;
	while ((opt = getopt(argc, argv, ":a:b:r:")) != -1) {
		switch (opt) {
		case 'a':
			analysisFilePath = optarg;
			break;
		case 'b':
			bestModFilePath = optarg;
			break;
		case 'r':
			randModFilePath = optarg;
			break;
		case ':':
			printf("option %c needs a value\n", optopt);
			return -1;
		case '?':
			printf("unknown option: %c\n", optopt);
			return -1;
		}
	}

	if (argc - optind == 6) {
		localeId = argv[argc - 6];
		transferFilePath = argv[argc - 5];
		sentenceFilePath = argv[argc - 4];
		lextorFilePath = argv[argc - 3];
		targetFilePath = argv[argc - 2];
		weightFilePath = argv[argc - 1];
		cout << localeId << "  " << transferFilePath << "  " << sentenceFilePath
				<< "  " << lextorFilePath << "  " << targetFilePath << "  "
				<< weightFilePath << endl;
	} else {
//      localeId = "es_ES";
//      transferFilePath = "transferFile.t1x";
//      sentenceFilePath = "spa-test.txt";
//      lextorFilePath = "spa-test.lextor";
//      interInFilePath = "beaminter.out";
//      modelsDest = "modelstry";
//      k = "8";

//      localeId = "kk_KZ";
//      transferFilePath = "apertium-kaz-tur.kaz-tur.t1x";
//      sentenceFilePath = "sample-sentences.txt";
//      lextorFilePath = "sample-lextor.txt";
//
//      transferOutFilePath = "sample-transfer.txt";
//      weightFilePath = "sample-weights.txt";
//
//      outputFilePath = "outAnalysis.txt";
//      bestModFilePath = "bestModFile.txt";
//      randModFilePath = "randModFile.txt";

		localeId = "es_ES";
		transferFilePath = "test/apertium-kaz-tur.kaz-tur.t1x";
		sentenceFilePath = "test/source.txt";
		lextorFilePath = "test/lextor.txt";

		targetFilePath = "test/transfer.txt";
		weightFilePath = "test/night-weights.txt";

		analysisFilePath = "outAnalysis.txt";
		bestModFilePath = "bestModFile.txt";
		randModFilePath = "randModFile.txt";

		cout << "Error in parameters !" << endl;
		cout << "Parameters are : localeId transferFilePath sentenceFilePath"
				<< " lextorFilePath targetFilePath weightOutFilePath"
				<< " [-a analysisFilePath] [-b bestModFilePath] [-r randModFilePath]"
				<< endl;
		cout
				<< "localeId : ICU locale ID for the source language. For Kazakh => kk_KZ"
				<< endl;
		cout
				<< "transferFilePath : Apertium transfer file of the language pair used."
				<< endl;
		cout << "sentenceFilePath : Source language sentences file." << endl;
		cout
				<< "lextorFilePath : Apertium lextor file for the source language sentences."
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
//		return -1;
	}

	// seed for randomness
	srand(time(NULL));

	ifstream lextorFile(lextorFilePath.c_str());
	ifstream inSentenceFile(sentenceFilePath.c_str());
	if (lextorFile.is_open() && inSentenceFile.is_open()) {
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

		vector<string> sourceSentences, tokenizedSentences;

		string tokenizedSentence;
		while (getline(lextorFile, tokenizedSentence)) {
			string sourceSentence;
			if (!getline(inSentenceFile, sourceSentence))
				sourceSentence = "No more sentences";

			sourceSentences.push_back(sourceSentence);
			tokenizedSentences.push_back(tokenizedSentence);
		}
		lextorFile.close();
		inSentenceFile.close();

		map<string, vector<vector<string> > > attrs = RuleParser::getAttrs(
				transfer);
		map<string, string> vars = RuleParser::getVars(transfer);
		map<string, vector<string> > lists = RuleParser::getLists(transfer);

		if (!analysisFilePath.empty()) {
			ofstream analysisFile(analysisFilePath.c_str());
			analysisFile.close();
		}
		if (!bestModFilePath.empty()) {
			ofstream bestModFile(bestModFilePath.c_str());
			bestModFile.close();
		}
		if (!randModFilePath.empty()) {
			ofstream randModFile(randModFilePath.c_str());
			randModFile.close();
		}

		ifstream weightFile(weightFilePath.c_str());
		ifstream targetFile(targetFilePath.c_str());

		if (weightFile.is_open() && targetFile.is_open())
			for (unsigned i = 0; i < sourceSentences.size(); i++) {
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

				RuleExecution::ruleOuts(&ruleOutputs, &tokenRules, slTokens,
						slTags, tlTokens, tlTags, rulesApplied, attrs, lists,
						&vars, spaces, localeId);

				// final outputs
				vector<string> normOuts;
				// number of generated combinations
				unsigned compNum;
				// nodes for every token and rule
				map<unsigned, vector<RuleExecution::Node*> > nodesPool;
				// ambiguous informations
				vector<RuleExecution::AmbigInfo*> ambigInfo;
				// rules combinations
				vector<vector<RuleExecution::Node*> > normCombNodes;

				nodesPool = RuleExecution::getNodesPool(tokenRules);

				RuleExecution::getAmbigInfo(tokenRules, nodesPool, &ambigInfo,
						&compNum);

				RuleExecution::getOuts(&normOuts, &normCombNodes, ambigInfo,
						nodesPool, ruleOutputs, spaces);

				// read weights
				string line;
				vector<float> normWeights;
				for (unsigned j = 0; j < normOuts.size(); j++) {
					getline(weightFile, line);
					float weight = strtof(line.c_str(), NULL);
					normWeights.push_back(weight);
				}

				// read transfer
				vector<string> normTransfers;
				for (unsigned j = 0; j < normOuts.size(); j++) {
					getline(targetFile, line);
					normTransfers.push_back(line);
				}

				// remove redundant outputs
				vector<string> outs;
				vector<vector<RuleExecution::Node*> > combNodes;
				vector<float> weights;
				vector<string> transfers;
				for (unsigned j = 0; j < normOuts.size(); j++)
					if (find(outs.begin(), outs.end(), normOuts[j])
							== outs.end()) {
						outs.push_back(normOuts[j]);
						combNodes.push_back(normCombNodes[j]);
						weights.push_back(normWeights[j]);
						transfers.push_back(normTransfers[j]);
					}
				normOuts = outs;
				normCombNodes = combNodes;
				normWeights = weights;
				normTransfers = transfers;

				// normalize weights
				RuleExecution::normaliseWeights(&normWeights);

				// write normal outputs
				if (!analysisFilePath.empty()) {
					ofstream analysisFile(analysisFilePath.c_str(),
							ofstream::app);
					if (analysisFile.is_open()) {
						analysisFile << "Analysis of sentence : " << endl;
						analysisFile << sourceSentence << endl << endl << endl;

						analysisFile << endl;
						analysisFile
								<< "sentence id ||| coverage id ||| original sentence |||"
								<< " lextor ||| rules ||| chunker ||| final sentence ||| score"
								<< endl << endl;

						for (unsigned j = 0; j < normWeights.size(); j++) {
							// sentence id
							analysisFile << (i + 1) << " ||| ";
							// coverage id
							analysisFile << (j + 1) << " ||| ";
							// original sentence
							analysisFile << sourceSentence << " ||| ";
							// lextor
							analysisFile << tokenizedSentence << " ||| ";
							// rules
							for (unsigned k = 0; k < normCombNodes[j].size();
									k++)
								if (normCombNodes[j][k]->ruleId)
									analysisFile << normCombNodes[j][k]->ruleId
											<< " ";
							analysisFile << "||| ";
							// chuncker
							analysisFile << normOuts[j] << " ||| ";
							// final sentence
							analysisFile << normTransfers[j] << " ||| ";
							// score
							analysisFile << normWeights[j] << endl << endl;
						}

						analysisFile
								<< "---------------------------------------------------------------------------------------------------------"
								<< endl << endl;

						analysisFile.close();
					}
				}

				// Model weighting
				// best weight
				if (!bestModFilePath.empty()) {
					ofstream bestModFile(bestModFilePath.c_str(),
							ofstream::app);
					if (bestModFile.is_open()) {
						unsigned maxInd = 0;
						for (unsigned j = 1; j < normWeights.size(); j++) {
							if (normWeights[j] > normWeights[maxInd])
								maxInd = j;
						}

						// final sentence
						bestModFile << normTransfers[maxInd] << endl;
					}
					bestModFile.close();
				}
				if (!randModFilePath.empty()) {
					// Random weight
					ofstream randModFile(randModFilePath.c_str(),
							ofstream::app);
					if (randModFile.is_open()) {
						int random = rand() % normWeights.size();

						// final sentence
						randModFile << normTransfers[random] << endl;
					}
					randModFile.close();
				}

				// delete AmbigInfo pointers
				for (unsigned j = 0; j < ambigInfo.size(); j++) {
					// delete the dummy node pointers
					set<RuleExecution::Node*> dummies;
					for (unsigned k = 0; k < ambigInfo[j]->combinations.size();
							k++)
						dummies.insert(ambigInfo[j]->combinations[k][0]);
					for (set<RuleExecution::Node*>::iterator it =
							dummies.begin(); it != dummies.end(); it++)
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
		else {
			cout << "ERROR in opening files!" << endl;
		}
		weightFile.close();
		targetFile.close();
	} else {
		cout << "ERROR in opening files!" << endl;
	}
	return 0;
}
