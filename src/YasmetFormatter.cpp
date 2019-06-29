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
	string localeId, transferFilePath, lextorFilePath, targetFilePath,
			weightsFilePath, datasetsPath;

	bool tagsFeats = false;
	int opt;
	while ((opt = getopt(argc, argv, ":r:t")) != -1) {
		switch (opt) {
		case 't':
			tagsFeats = true;
			break;
		case 'r':
			targetFilePath = optarg;
			break;
		case ':':
			printf("option %c needs a value\n", optopt);
			return -1;
		case '?':
			printf("unknown option: %c\n", optopt);
			return -1;
		}
	}

	if (argc - optind == 5) {
		localeId = argv[argc - 5];
		transferFilePath = argv[argc - 4];
		lextorFilePath = argv[argc - 3];
		weightsFilePath = argv[argc - 2];
		datasetsPath = argv[argc - 1];
	} else {
//      localeId = "es_ES";
//      transferFilePath = "transferFile.t1x";
//      sentenceFilePath = "spa-test.txt";
//      lextorFilePath = "spa-test.lextor";
//      transferOutFilePath = "transfer.out";
//      weightOutFilePath = "weights.txt";
//      outputFilePath = "output.out";
//      datasetsPath = "datasetstry2";

		localeId = "kk_KZ";
		transferFilePath = "apertium-kaz-tur.kaz-tur.t1x";
		lextorFilePath = "sample-lextor.txt";
		weightsFilePath = "norm-weights.txt";
		datasetsPath = "datasetstry1234";
		targetFilePath = "target.txt";

		cout << "Error in parameters !" << endl;
		cout << "Parameters are : localeId transferFilePath lextorFilePath"
				<< " weightOutFilePath datasetsPath [-r targetFilePath] [-t]"
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
				<< "weightOutFilePath : Language model weights file for the source language sentences."
				<< endl;
		cout
				<< "datasetsPath : Datasets destination to put in the generated yasmet files."
				<< endl;
		cout << "-r : Remove \"bad\" sentences (with # or @)." << endl;
		cout << "targetFilePath : Target file path for these sentences."
				<< endl;
		cout << "-t : Tags as features in yasmet." << endl;
		return -1;
	}

	ifstream lextorFile(lextorFilePath.c_str());
	ifstream weightOutFile(weightsFilePath.c_str());
	ifstream targetFile(targetFilePath.c_str());
	if (lextorFile.is_open() && weightOutFile.is_open()
			&& (targetFilePath.empty() || targetFile.is_open())) {
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
			// number of generated combinations
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

			vector<RuleExecution::AmbigInfo*> newAmbigInfo;
			for (unsigned j = 0; j < ambigInfo.size(); j++)
				if (ambigInfo[j]->combinations.size() > 1)
					newAmbigInfo.push_back(ambigInfo[j]);
			ambigInfo = newAmbigInfo;

			// remove bad sentences with (*,#,@)
			string line;

			bool isBad = false;
			for (unsigned j = 0; j < outs.size(); j++) {
				getline(targetFile, line);
//				cout << line << "  " << line.find('#') << "  " << line.find('@')
//						<< endl;
				if (line.find('#') != string::npos
						|| line.find('@') != string::npos) {
					isBad = true;
					break;
				}
			}

			// read weights
			vector<float> weights;
			for (unsigned j = 0; j < outs.size(); j++) {
				getline(weightOutFile, line);
				float weight = strtof(line.c_str(), NULL);
				weights.push_back(weight);
			}

			if (!targetFilePath.empty() && isBad)
				continue;

			goodSents++;

			RuleExecution::normaliseWeights(&weights, ambigInfo);

			// Yasmet format preparing
			// make a directory if not found
			mkdir(datasetsPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

			unsigned weigInd = 0;
			for (unsigned i = 0; i < ambigInfo.size(); i++) {
				RuleExecution::AmbigInfo* ambig = ambigInfo[i];

				// name of the file is the concatenation of rules ids
				string rulesNums;
				for (unsigned x = 0; x < ambig->combinations.size(); x++) {
					// avoid dummy node
					for (unsigned y = 1; y < ambig->combinations[x].size();
							y++) {
						stringstream ss;
						ss << ambig->combinations[x][y]->ruleId;
						rulesNums += ss.str();

						if (y + 1 < ambig->combinations[x].size())
							rulesNums += "_";
					}
					rulesNums += "+";
				}

				// if it's the first time to open , put the number of classes
				bool firstTime = true;
				if (FILE *file = fopen(
						(datasetsPath + string("/") + rulesNums).c_str(),
						"r")) {
					firstTime = false;
					fclose(file);
				}

				ofstream dataset(
						(datasetsPath + string("/") + rulesNums).c_str(),
						ofstream::app);

				if (firstTime)
					dataset << ambig->combinations.size() << endl;

				for (unsigned x = 0; x < ambig->combinations.size(); x++) {

					dataset << x << " $ ";

					float weight = weights[x + weigInd];

					dataset << weight << " #";

					string features;
					for (unsigned v = 0; v < ambig->combinations.size(); v++) {
						stringstream ss;
						ss << v;
						string label = ss.str();

						for (unsigned z = ambig->firTokId;
								z < ambig->firTokId + ambig->maxPat; z++) {
							stringstream ss;
							ss << z - ambig->firTokId;
							string num = ss.str();
							string word = CLExec::toLowerCase(slTokens[z],
									localeId);

							for (unsigned c = 0; c < word.length(); c++)
								if (word[c] == ' ')
									word.replace(c, 1, "_");

							features += " " + word + "_" + num + ":" + label;

							if (tagsFeats)
								for (unsigned d = 0; d < slTags[z].size(); d++)
									features += " " + slTags[z][d] + "_" + num
											+ ":" + label;
						}
						features += " #";
					}
					dataset << features << endl;
				}
				weigInd += ambig->combinations.size();
				dataset.close();
			}

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
		weightOutFile.close();

		cout << "There are " << goodSents << " good sentences from " << allSents
				<< endl;
	} else {
		cout << "ERROR in opening files!" << endl;
	}

	return 0;
}
