#ifndef SRC_RULEEXECUTION_H_
#define SRC_RULEEXECUTION_H_

//#include "../pugixml/pugixml.hpp"
#include "pugixml.hpp"

using namespace std;
using namespace pugi;

class RuleExecution
{
public:

  static string
  noRuleOut (vector<string> analysis);

  static bool
  outputs (
      vector<string>* outs,
      vector<vector<pair<unsigned, unsigned> > >* rulesIds,
      vector<vector<vector<unsigned> > >* outsRules,
      vector<pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<unsigned> > > > > *ambigInfo,
      vector<string> tlTokens, vector<vector<string> > tags,
      map<unsigned, map<unsigned, string> > ruleOuts,
      map<unsigned, vector<pair<unsigned, unsigned> > > tokenRules,
      vector<string> spaces);

  static void
  weightIndices (
      vector<vector<unsigned> >* weigInds,
      vector<pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<unsigned> > > > > ambigInfo,
      vector<vector<vector<unsigned> > > outsRules);

  static void
  ruleOuts (map<unsigned, map<unsigned, string> >* ruleOuts,
	    map<unsigned, vector<pair<unsigned, unsigned> > >* tokenRules,
	    vector<string> slTokens, vector<vector<string> > slTags,
	    vector<string> tlTokens, vector<vector<string> > tlTags,
	    map<xml_node, vector<vector<unsigned> > > rulesApplied,
	    map<string, vector<vector<string> > > attrs,
	    map<string, vector<string> > lists, map<string, string>* vars,
	    vector<string> spaces, string localeId);

  static vector<string>
  ruleExe (xml_node rule, vector<vector<string> >* slAnalysisTokens,
	   vector<vector<string> >* tlAnalysisTokens,
	   map<string, vector<vector<string> > > attrs,
	   map<string, vector<string> > lists, map<string, string>* vars,
	   vector<string> spaces, unsigned firPat, string localeId);

  static vector<string>
  choose (xml_node choose, vector<vector<string> >* slAnalysisTokens,
	  vector<vector<string> >* tlAnalysisTokens,
	  map<string, vector<vector<string> > > attrs, map<string, vector<string> > lists,
	  map<string, string>* vars, vector<string> spaces, unsigned firPat,
	  string localeId, map<unsigned, unsigned> paramToPattern);

  static void
  let (xml_node let, vector<vector<string> >* slAnalysisTokens,
       vector<vector<string> >* tlAnalysisTokens,
       map<string, vector<vector<string> > > attrs, map<string, string>* vars,
       vector<string> spaces, unsigned firPat, string localeId,
       map<unsigned, unsigned> paramToPattern);

  static vector<string>
  callMacro (xml_node callMacro, vector<vector<string> >* slAnalysisTokens,
	     vector<vector<string> >* tlAnalysisTokens,
	     map<string, vector<vector<string> > > attrs,
	     map<string, vector<string> > lists, map<string, string>* vars,
	     vector<string> spaces, unsigned firPat, string localeId,
	     map<unsigned, unsigned> paramToPattern);

  static vector<string>
  out (xml_node out, vector<vector<string> >* slAnalysisTokens,
       vector<vector<string> >* tlAnalysisTokens,
       map<string, vector<vector<string> > > attrs, map<string, string>* vars,
       vector<string> spaces, unsigned firPat, string localeId,
       map<unsigned, unsigned> paramToPattern);

  static vector<string>
  chunk (xml_node chunkNode, vector<vector<string> >* slAnalysisTokens,
	 vector<vector<string> >* tlAnalysisTokens,
	 map<string, vector<vector<string> > > attrs, map<string, string>* vars,
	 vector<string> spaces, unsigned firPat, string localeId,
	 map<unsigned, unsigned> paramToPattern);

  static vector<string>
  formatTokenTags (string token, vector<string> tags);

  static vector<string>
  findAttrPart (vector<string> tokenTags, vector<vector<string> > attrTags);

  static vector<string>
  clip (xml_node clip, vector<vector<string> >* slAnalysisTokens,
	vector<vector<string> >* tlAnalysisTokens,
	map<string, vector<vector<string> > > attrs, map<string, string>* vars,
	vector<string> spaces, unsigned firPat, string localeId,
	map<unsigned, unsigned> paramToPattern);

  static vector<string>
  concat (xml_node concat, vector<vector<string> >* slAnalysisTokens,
	  vector<vector<string> >* tlAnalysisTokens,
	  map<string, vector<vector<string> > > attrs, map<string, string>* vars,
	  vector<string> spaces, unsigned firPat, string localeId,
	  map<unsigned, unsigned> paramToPattern);

  static bool
  equal (xml_node equal, vector<vector<string> >* slAnalysisTokens,
	 vector<vector<string> >* tlAnalysisTokens,
	 map<string, vector<vector<string> > > attrs, map<string, string>* vars,
	 vector<string> spaces, unsigned firPat, string localeId,
	 map<unsigned, unsigned> paramToPattern);

  static bool
  test (xml_node test, vector<vector<string> >* slAnalysisTokens,
	vector<vector<string> >* tlAnalysisTokens,
	map<string, vector<vector<string> > > attrs, map<string, vector<string> > lists,
	map<string, string>* vars, vector<string> spaces, unsigned firPat,
	string localeId, map<unsigned, unsigned> paramToPattern);

  static bool
  And (xml_node And, vector<vector<string> >* slAnalysisTokens,
       vector<vector<string> >* tlAnalysisTokens,
       map<string, vector<vector<string> > > attrs, map<string, vector<string> > lists,
       map<string, string>* vars, vector<string> spaces, unsigned firPat, string localeId,
       map<unsigned, unsigned> paramToPattern);

  static bool
  Or (xml_node Or, vector<vector<string> >* slAnalysisTokens,
      vector<vector<string> >* tlAnalysisTokens,
      map<string, vector<vector<string> > > attrs, map<string, vector<string> > lists,
      map<string, string>* vars, vector<string> spaces, unsigned firPat, string localeId,
      map<unsigned, unsigned> paramToPattern);

  static bool
  in (xml_node in, vector<vector<string> >* slAnalysisTokens,
      vector<vector<string> >* tlAnalysisTokens,
      map<string, vector<vector<string> > > attrs, map<string, vector<string> > lists,
      map<string, string>* vars, vector<string> spaces, unsigned firPat, string localeId,
      map<unsigned, unsigned> paramToPattern);

  static bool
  Not (xml_node Not, vector<vector<string> >* slAnalysisTokens,
       vector<vector<string> >* tlAnalysisTokens,
       map<string, vector<vector<string> > > attrs, map<string, vector<string> > lists,
       map<string, string>* vars, vector<string> spaces, unsigned firPat, string localeId,
       map<unsigned, unsigned> paramToPattern);

  static vector<string>
  litTag (xml_node litTag);

  static string
  lit (xml_node lit);

  static string
  var (xml_node var, map<string, string>* vars);

  static void
  append (xml_node append, vector<vector<string> >* slAnalysisTokens,
	  vector<vector<string> >* tlAnalysisTokens,
	  map<string, vector<vector<string> > > attrs, map<string, string>* vars,
	  vector<string> spaces, unsigned firPat, string localeId,
	  map<unsigned, unsigned> paramToPattern);

  static string
  b (xml_node b, vector<string> spaces, unsigned firPat, string localeId,
     map<unsigned, unsigned> paramToPattern);

  static string
  caseOf (xml_node caseOf, vector<vector<string> >* slAnalysisTokens,
	  vector<vector<string> >* tlAnalysisTokens, string localeId,
	  map<unsigned, unsigned> paramToPattern);

  static string
  getCaseFrom (xml_node getCaseFrom, vector<vector<string> >* slAnalysisTokens,
	       vector<vector<string> >* tlAnalysisTokens,
	       map<string, vector<vector<string> > > attrs, map<string, string>* vars,
	       vector<string> spaces, unsigned firPat, string localeId,
	       map<unsigned, unsigned> paramToPattern);

  static void
  modifyCase (xml_node modifyCase, vector<vector<string> >* slAnalysisTokens,
	      vector<vector<string> >* tlAnalysisTokens,
	      map<string, vector<vector<string> > > attrs, map<string, string>* vars,
	      vector<string> spaces, unsigned firPat, string localeId,
	      map<unsigned, unsigned> paramToPattern);
};

#endif /* SRC_RULEEXECUTION_H_ */
