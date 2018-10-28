#ifndef SRC_RULEEXECUTION_H_
#define SRC_RULEEXECUTION_H_

#include "../pugixml/pugixml.hpp"

using namespace std;
using namespace pugi;

class RuleExecution
{
public:

  static void
  outputs (
      vector<string>* outs,
      vector<vector<unsigned> >* rulesPrint,
      vector<vector<vector<xml_node> > >* outsRules,
      vector<pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<xml_node> > > > > *ambigInfo,
      vector<string> tlTokens, vector<vector<string> > tags,
      map<xml_node, map<int, vector<string> > > ruleOuts,
      map<int, vector<pair<xml_node, unsigned> > > tokenRules, vector<string> spaces);

  static void
  weightIndices (
      vector<vector<unsigned> >* weigInds,
      vector<pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<xml_node> > > > > ambigInfo,
      vector<vector<vector<xml_node> > > outsRules);

  static void
  ruleOuts (map<xml_node, map<int, vector<string> > >* ruleOuts,
	    map<int, vector<pair<xml_node, unsigned> > >* tokenRules,
	    vector<string> tlTokens, vector<vector<string> > tags,
	    map<xml_node, vector<vector<int> > > rulesApplied,
	    map<string, vector<vector<string> > > attrs);

  static vector<string>
  ruleExe (xml_node rule, vector<vector<string> >* tlAnalysisTokens,
	   map<string, vector<vector<string> > > attrs);

  static void
  chooseAction (xml_node choose, vector<vector<string> >* tlAnalysisTokens,
		map<string, vector<vector<string> > > attrs,
		map<int, int> paramToPattern = map<int, int> ());

  static void
  letAction (xml_node let, vector<vector<string> >* tlAnalysisTokens,
	     map<string, vector<vector<string> > > attrs, map<int, int> paramToPattern =
		 map<int, int> ());

  static void
  macroAction (xml_node callMacro, vector<vector<string> >* tlAnalysisTokens,
	       map<string, vector<vector<string> > > attrs);
  static vector<string>
  outAction (xml_node out, vector<vector<string> >* tlAnalysisTokens,
	     map<string, vector<vector<string> > > attrs);

  static vector<string>
  formatTokenTags (string token, vector<string> tags);

  static vector<string>
  findAttrPart (vector<string> tokenTags, vector<vector<string> > attrTags);

  static vector<string>
  clipAction (xml_node clip, vector<vector<string> >* tlAnalysisTokens,
	      map<string, vector<vector<string> > > attrs, map<int, int> paramToPattern =
		  map<int, int> ());

  static vector<string>
  concat (xml_node concat, vector<vector<string> >* tlAnalysisTokens,
	  map<string, vector<vector<string> > > attrs,
	  map<int, int> paramToPattern = map<int, int> ());

  static bool
  equal (xml_node equal, vector<vector<string> >* tlAnalysisTokens,
	 map<string, vector<vector<string> > > attrs,
	 map<int, int> paramToPattern = map<int, int> ());

  static vector<string>
  litTagAction (xml_node litTag);
};

#endif /* SRC_RULEEXECUTION_H_ */
