/*
 * CLExec.cpp
 *
 *  Created on: Jun 21, 2018
 *      Author: aboelhamd
 */

#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <algorithm>
#include <cfloat>
#include <sstream>
#include <apertium-3.5/apertium/string_utils.h>
#include <apertium-3.5/apertium/utf_converter.h>

#include "CLExec.h"
#include "TranElemLiterals.h"
#include "pugixml.hpp"

using namespace std;
using namespace pugi;
using namespace elem;

map<string, map<string, vector<float> > > CLExec::loadYasmetModels(
		string modelsFilePath/*, string *localeid*/) {
	// map with key yasmet model name and the value is
	// another map with key word name and the value is
	// vector of weights in order
	map<string, map<string, vector<float> > > classWeights;

	ifstream modelsFile((modelsFilePath).c_str());

	if (modelsFile.is_open()) {
		string line, model, token, weight;

		// localeid
//      getline (modelsFile, line);
//      *localeid = line;

		while (getline(modelsFile, line)) {
			// 0=>word , 1=>rule_num & 2=>wieght
			// we don't need rule number , because
			// the weights are already sorted

			char lineChar[line.size()];
			strcpy(lineChar, line.c_str());

			token = strtok(lineChar, ": ");
//			cout << endl << token << endl;
			if (token == "file") {
				model = strtok(NULL, ": ");
				continue;
			}
			// skip rule_num
			/*cout << model << "  " << */strtok(NULL, ": ") /*<< endl*/;
//			cout << "rulenum= " << strtok(NULL, ": ") << endl;

			weight = strtok(NULL, ": ");
//			cout << "weight= " << weight << endl;

			float w = strtof(weight.c_str(), NULL);
//			cout << weight << " = " << w << endl;

//			cout << w << endl;
//			if (w < 0)
//				cout << w << endl;
//			classWeights[model][token].push_back(w);
			map<string, map<string, vector<float> > >::iterator it1 =
					classWeights.find(model);
			if (it1 == classWeights.end()) {
				classWeights.insert(
						pair<string, map<string, vector<float> > >(model,
								map<string, vector<float> >()));
				it1 = classWeights.find(model);
			}

			map<string, vector<float> >::iterator it2 = it1->second.find(token);
			if (it2 != it1->second.end()) {
				it2->second.push_back(w);
			} else {
				it1->second.insert(
						pair<string, vector<float> >(token, vector<float>()));
				it2 = it1->second.find(token);
				it2->second.push_back(w);
			}
//			if (classWeights[model][token][classWeights[model][token].size() - 1]
//					< 0)
//				cout << w << endl;
//			cout
//					<< classWeights[model][token][classWeights[model][token].size()
//							- 1] << endl;
		}
	} else {
		cout << "error in opening models file" << endl;
	}
//	for (map<string, map<string, vector<float> > >::iterator it =
//			classWeights.begin(); it != classWeights.end(); it++) {
//		cout << "model=" << it->first << endl;
//		for (map<string, vector<float> >::iterator it2 = it->second.begin();
//				it2 != it->second.end(); it2++) {
//			cout << "word= " << it2->first << endl;
////			vector<float> weights = it2->second;
//			for (unsigned i = 0; i < it2->second.size(); i++) {
//				cout << it2->second[i] << endl;
//			}
//			cout << endl;
//		}
//	}
	return classWeights;
}

string CLExec::toLowerCase(string s, string localeId) {
	wstring ws = UtfConverter::fromUtf8(s);
	ws = StringUtils::tolower(ws);
	s = UtfConverter::toUtf8(ws);

	return s;
}

string CLExec::toUpperCase(string s, string localeId) {
	wstring ws = UtfConverter::fromUtf8(s);
	ws = StringUtils::toupper(ws);
	s = UtfConverter::toUtf8(ws);

	return s;
}

string CLExec::FirLetUpperCase(string s, string localeId) {
	wstring ws = UtfConverter::fromUtf8(s);
	ws = StringUtils::tolower(ws);
	ws[0] = (wchar_t) towupper(ws[0]);
	s = UtfConverter::toUtf8(ws);

	return s;
}

// The result of bitwise character comparison: 0 if this contains
// the same characters as text, -1 if the characters in this are
// bitwise less than the characters in text, +1 if the characters
// in this are bitwise greater than the characters in text.
int CLExec::compare(string s1, string s2) {
	wstring ws1 = UtfConverter::fromUtf8(s1);
	wstring ws2 = UtfConverter::fromUtf8(s2);

	return ws1.compare(ws2);
}

int CLExec::compareCaseless(string s1, string s2, string localeId) {
	wstring ws1 = UtfConverter::fromUtf8(s1);
	ws1 = StringUtils::tolower(ws1);
	wstring ws2 = UtfConverter::fromUtf8(s2);
	ws2 = StringUtils::tolower(ws2);

	return ws1.compare(ws2);
}

// to sort translations from best to worth by their weight
bool sortParameter(pair<vector<RuleExecution::Node*>, float> a,
		pair<vector<RuleExecution::Node*>, float> b) {
	return (a.second > b.second);
}

void CLExec::beamSearch(
		vector<pair<vector<RuleExecution::Node*>, float> > *beamTree,
		unsigned beam, vector<string> slTokens, vector<vector<string> > slTags,
		vector<RuleExecution::AmbigInfo*> ambigInfo,
		map<string, map<string, vector<float> > > classesWeights,
		string localeId) {
	// Initialization
	(*beamTree).push_back(pair<vector<RuleExecution::Node*>, float>());

	for (unsigned i = 0; i < ambigInfo.size(); i++) {
//      for (unsigned x = 0; x < beamTree->size (); x++)
//	{
//	  cout << "weight = " << (*beamTree)[x].second << endl;
//	  for (unsigned j = 0; j < (*beamTree)[x].first.size (); j++)
//	    {
//	      cout << (*beamTree)[x].first[j].tokenId << "  "
//		  << (*beamTree)[x].first[j].ruleId << "  "
//		  << (*beamTree)[x].first[j].patNum << endl;
//	    }
//	}

		RuleExecution::AmbigInfo* ambig = ambigInfo[i];
//      pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<unsigned> > > > p =
//	  ambigInfo[i];
//      pair<unsigned, unsigned> wordInd = p.first;
//      vector<vector<unsigned> > ambigRules = p.second.second;
		unsigned ambigRulesSize = ambig->combinations.size();

		// name of the file is the concatenation of rules ids
		string rulesNums;
		for (unsigned x = 0; x < ambigRulesSize; x++) {
			// avoid dummy node
			for (unsigned y = 1; y < ambig->combinations[x].size(); y++) {
				stringstream ss;
				ss << ambig->combinations[x][y]->ruleId;
				rulesNums += ss.str();

				if (y + 1 < ambig->combinations[x].size())
					rulesNums += "_";
			}
			rulesNums += "+";
		}

//      cout << rulesNums << endl;

		map<string, vector<float> > classWeights = classesWeights[(rulesNums
				+ ".model")];

		// build new tree for the new words
		vector<pair<vector<RuleExecution::Node*>, float> > newTree;

		// initialize the new tree
		for (unsigned x = 0; x < ambigRulesSize; x++) {
			newTree.push_back(
					pair<vector<RuleExecution::Node*>, float>(
							vector<RuleExecution::Node*>(), 0));
		}
		// put rules
		for (unsigned z = 0; z < ambigRulesSize; z++) {
			for (unsigned y = 0; y < ambig->combinations[z].size(); y++) {
				newTree[z].first.push_back(ambig->combinations[z][y]);
			}
		}

		for (unsigned x = ambig->firTokId; x < ambig->firTokId + ambig->maxPat;
				x++) {
			// word key is the word and it's order in the rule
			stringstream ss;
			ss << x - ambig->firTokId;
			string num = "_" + ss.str();

			// handle the case of two lemmas separated by a space
			for (unsigned t = 0; t < slTokens[x].size(); t++)
				if (slTokens[x][t] == ' ')
					slTokens[x].replace(t, 1, "_");

			string word = toLowerCase(slTokens[x], localeId) + num;
			vector<float> wordWeights = classWeights[word];

			// put weights
			if (wordWeights.empty()) {
				wordWeights = vector<float>(ambigRulesSize, 1.0);
				cout << "word : " << word << "  is not found in dataset : "
						<< rulesNums << endl;
			}

			vector<float> tagWeights;
			for (unsigned t = 0; t < slTags[x].size(); t++) {
				string tag = slTags[x][t] + num;
				tagWeights = classWeights[tag];
				for (unsigned w = 0; w < tagWeights.size(); w++)
					wordWeights[w] += tagWeights[w];
			}
			for (unsigned z = 0; z < ambigRulesSize; z++)
				newTree[z].second += wordWeights[z];

		}

// expand beamTree
		unsigned initSize = beamTree->size();
		for (unsigned z = 0; z < ambigRulesSize - 1; z++) {
			for (unsigned x = 0; x < initSize; x++) {
				beamTree->push_back(
						pair<vector<RuleExecution::Node*>, float>(
								(*beamTree)[x]));
			}
		}

// merge the two trees
		for (unsigned z = 0; z < ambigRulesSize; z++) {
			for (unsigned x = initSize * z; x < initSize * (z + 1); x++) {
				// put the new rules with the old
				(*beamTree)[x].first.insert((*beamTree)[x].first.end(),
						newTree[z].first.begin(), newTree[z].first.end());

				// add their wiehgts
				(*beamTree)[x].second += newTree[z].second;
			}
		}

// sort beam tree
		sort(beamTree->begin(), beamTree->end(), sortParameter);

// remove elements more than (beam)
		if (beamTree->size() > beam)
			beamTree->erase(beamTree->begin() + beam, beamTree->end());
	}
}

void CLExec::getTransInds(vector<pair<unsigned, float> > *transInds,
		vector<pair<vector<unsigned>, float> > beamTree,
		vector<vector<pair<unsigned, unsigned> > > rulesIds) {
	for (unsigned i = 0; i < beamTree.size(); i++) {
		vector<unsigned> transInd = beamTree[i].first;
		for (unsigned j = 0; j < rulesIds.size(); j++) {
			vector<pair<unsigned, unsigned> > weigInd = rulesIds[j];

			unsigned count = 0;
			for (unsigned x = 0; x < weigInd.size() && count < transInd.size();
					x++) {
				if (transInd[count] == weigInd[x].first)
					count++;
			}

			if (count == transInd.size()) {
				transInds->push_back(
						pair<unsigned, float>(j, beamTree[i].second));
				break;
			}
		}
	}
}
