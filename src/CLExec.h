#ifndef SRC_CLEXEC_H_
#define SRC_CLEXEC_H_

#include <string>
#include <vector>

#include "pugixml.hpp"
#include "RuleExecution.h"

using namespace std;
using namespace pugi;

class CLExec {
public:

	static map<string, map<string, vector<float> > >
	loadYasmetModels(string modelsDest/*, string *localeid*/);

	static string
	toLowerCase(string s, string localeId);

	static string
	toUpperCase(string s, string localeId);

	static string
	FirLetUpperCase(string s, string localeId);

	static int
	compare(string s1, string s2);

	static int
	compareCaseless(string s1, string s2, string localeId);

	static void
	beamSearch(vector<pair<vector<RuleExecution::Node*>, float> > *beamTree,
			unsigned beam, vector<string> slTokens,
			vector<vector<string> > slTags,
			vector<RuleExecution::AmbigInfo*> ambigInfo,
			map<string, map<string, vector<float> > > classesWeights,
			string localeId);

	static void
	getTransInds(vector<pair<unsigned, float> > *transInds,
			vector<pair<vector<unsigned>, float> > beamTree,
			vector<vector<pair<unsigned, unsigned> > > rulesIds);
};

#endif /* SRC_CLEXEC_H_ */
