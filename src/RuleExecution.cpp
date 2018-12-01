#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <pugixml.hpp>
//#include "../pugixml/pugixml.hpp"
#include "TranElemLiterals.h"
#include "CLExec.h"

using namespace std;
using namespace pugi;
using namespace elem;

#include "RuleExecution.h"

// to sort rules in tokenRules descendingly by their number of pattern items
bool
sortParameter (pair<xml_node, unsigned> a, pair<xml_node, unsigned> b)
{
  return (a.second > b.second);
}

vector<string>
defaultOut (vector<string> analysis)
{
  vector<string> out;
  out.push_back ("^default<default>{^");
  out.insert (out.end (), analysis.begin (), analysis.end ());
  out.push_back ("$}$");

  return out;
}

void
putOut (vector<vector<string> >* outputs, vector<string> output, unsigned tokenIndex,
	vector<string> spaces)
{
  for (unsigned i = 0; i < outputs->size (); i++)
    {
      (*outputs)[i].insert ((*outputs)[i].end (), output.begin (), output.end ());
      (*outputs)[i].push_back (spaces[tokenIndex]);
    }
}

vector<vector<string> >
putOuts (vector<vector<string> > outputs, vector<vector<string> > nestedOutputs,
	 unsigned tokenIndex, vector<string> spaces)
{
  vector<vector<string> > newOutputs;

  for (unsigned i = 0; i < outputs.size (); i++)
    {
      for (unsigned j = 0; j < nestedOutputs.size (); j++)
	{
	  vector<string> newOutput;

	  newOutput.insert (newOutput.end (), outputs[i].begin (), outputs[i].end ());
	  newOutput.insert (newOutput.end (), nestedOutputs[j].begin (),
			    nestedOutputs[j].end ());
	  newOutput.push_back (spaces[tokenIndex]);

	  newOutputs.push_back (newOutput);
	}
    }

  return newOutputs;
}

void
nestedRules (vector<string> tlTokens, vector<string> output,
	     vector<xml_node> curNestedRules, vector<vector<string> >* outputs,
	     vector<vector<xml_node> >* nestedOutsRules,
	     map<xml_node, map<int, vector<string> > > ruleOuts,
	     map<int, vector<pair<xml_node, unsigned> > > tokenRules, unsigned curPatNum,
	     unsigned curTokIndex)
{
  vector<pair<xml_node, unsigned> > rulesApplied = tokenRules[curTokIndex];

  for (unsigned i = 0; i < rulesApplied.size (); i++)
    {
      xml_node rule = rulesApplied[i].first;
      unsigned patNum = rulesApplied[i].second;

      if (patNum <= curPatNum)
	{
	  vector<xml_node> newCurNestedRules = vector<xml_node> (curNestedRules);
	  newCurNestedRules.push_back (rule);

	  vector<string> newOutput = vector<string> (output);

	  vector<string> ruleOut = ruleOuts[rule][curTokIndex];

	  newOutput.insert (newOutput.end (), ruleOut.begin (), ruleOut.end ());

	  if (curPatNum - patNum == 0)
	    {
	      (*outputs).push_back (newOutput);
	      (*nestedOutsRules).push_back (newCurNestedRules);
	    }
	  else
	    {
	      nestedRules (tlTokens, newOutput, newCurNestedRules, outputs,
			   nestedOutsRules, ruleOuts, tokenRules, curPatNum - patNum,
			   curTokIndex + 1);
	    }
	}
    }

}

vector<string>
vectorToString (vector<vector<string> > vOutputs)
{
  vector<string> sOutputs;

  for (unsigned i = 0; i < vOutputs.size (); i++)
    {
      string output = "";
      for (unsigned j = 0; j < vOutputs[i].size (); j++)
	{
	  output += vOutputs[i][j];
	}

      sOutputs.push_back (output);
    }

  return sOutputs;
}

vector<vector<unsigned> >
ruleVectorToIds (vector<vector<vector<xml_node> > > vOutRules)
{
  vector<vector<unsigned> > sOutRules;

  for (unsigned i = 0; i < vOutRules.size (); i++)
    {
      vector<unsigned> output;
      for (unsigned j = 0; j < vOutRules[i].size (); j++)
	{
	  for (unsigned k = 0; k < vOutRules[i][j].size (); k++)
	    {
	      // we want the id only (after the last '-')
	      output.push_back (vOutRules[i][j][k].attribute (ID).as_int ());
	    }
	}

      sOutRules.push_back (output);
    }

  return sOutRules;
}

vector<vector<vector<xml_node> > >
putRules (vector<vector<vector<xml_node> > > outsRules,
	  vector<vector<xml_node> > nestedOutsRules)
{
  vector<vector<vector<xml_node> > > newRules;

  for (unsigned i = 0; i < outsRules.size (); i++)
    {
      for (unsigned j = 0; j < nestedOutsRules.size (); j++)
	{
	  vector<vector<xml_node> > newRule = vector<vector<xml_node> > (outsRules[i]);

	  newRule.push_back (nestedOutsRules[j]);

	  newRules.push_back (newRule);
	}
    }

  return newRules;
}

void
RuleExecution::outputs (
    vector<string>* outs,
    vector<vector<unsigned> >* rulesPrint,
    vector<vector<vector<xml_node> > >* outsRules,
    vector<pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<xml_node> > > > > *ambigInfo,
    vector<string> tlTokens, vector<vector<string> > tags,
    map<xml_node, map<int, vector<string> > > ruleOuts,
    map<int, vector<pair<xml_node, unsigned> > > tokenRules, vector<string> spaces)
{
  // position of ambiguous rule among other ones
  int ambigPos = -1;

  vector<vector<string> > outputs;
  outputs.push_back (vector<string> ());

  outsRules->push_back (vector<vector<xml_node> > ());

  for (unsigned i = 0; i < tlTokens.size ();)
    {
      if (outputs.size () > 1024)
	{
	  (*outs) = vectorToString (outputs);
	  (*rulesPrint) = ruleVectorToIds (*outsRules);
	  return;
	}
      vector<pair<xml_node, unsigned> > rulesApplied = tokenRules[i];

      if (rulesApplied.empty ())
	{
	  vector<string> defOut = defaultOut (formatTokenTags (tlTokens[i], tags[i]));

	  putOut (&outputs, defOut, i, spaces);

	  i++;
	}
      else
	{
	  ambigPos++;

	  // longest rule
	  unsigned maxPatterns = rulesApplied[0].second;

	  vector<vector<string> > nestedOutputs;
	  vector<vector<xml_node> > nestedOutsRules;

	  nestedRules (tlTokens, vector<string> (), vector<xml_node> (), &nestedOutputs,
		       &nestedOutsRules, ruleOuts, tokenRules, maxPatterns, i);

	  // if there is ambiguity save the info
	  if (nestedOutsRules.size () > 1)
	    {
	      ambigInfo->push_back (
		  pair<pair<unsigned, unsigned>,
		      pair<unsigned, vector<vector<xml_node> > > > (
		      pair<unsigned, unsigned> (i, maxPatterns),
		      pair<unsigned, vector<vector<xml_node> > > (ambigPos,
								  nestedOutsRules)));
	    }

	  outputs = putOuts (outputs, nestedOutputs, i + maxPatterns - 1, spaces);
	  (*outsRules) = putRules (*outsRules, nestedOutsRules);

	  i += maxPatterns;
	}
    }

  (*outs) = vectorToString (outputs);
  (*rulesPrint) = ruleVectorToIds (*outsRules);
}

void
RuleExecution::weightIndices (
    vector<vector<unsigned> >* weigInds,
    vector<pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<xml_node> > > > > ambigInfo,
    vector<vector<vector<xml_node> > > outsRules)
{
  for (unsigned i = 0; i < ambigInfo.size (); i++)
    {
      pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<xml_node> > > > p1 =
	  ambigInfo[i];
      vector<vector<xml_node> > ambigRules = p1.second.second;
      unsigned rulePos = p1.second.first;

      for (unsigned x = 0; x < ambigRules.size (); x++)
	{
	  vector<unsigned> weigInd;
	  for (unsigned z = 0; z < outsRules.size (); z++)
	    {
	      vector<xml_node> ambigRule = outsRules[z][rulePos];

	      bool same = true;

	      if (ambigRule.size () == ambigRules[x].size ())
		{
		  for (unsigned ind = 0; ind < ambigRule.size (); ind++)
		    {
		      if (ambigRule[ind] != ambigRules[x][ind])
			{
			  same = false;
			  break;
			}
		    }
		}
	      else
		{
		  same = false;
		}

	      if (same)
		weigInd.push_back (z);
	    }
	  weigInds->push_back (weigInd);
	}
    }

}

void
pushDistinct (map<int, vector<pair<xml_node, unsigned> > >* tokenRules, int tlTokInd,
	      xml_node rule, unsigned patNum)
{
  vector<pair<xml_node, unsigned> > pairs = (*tokenRules)[tlTokInd];
  for (unsigned i = 0; i < pairs.size (); i++)
    {
      if (pairs[i].first == rule)
	return;
    }

  (*tokenRules)[tlTokInd].push_back (pair<xml_node, unsigned> (rule, patNum));

  sort ((*tokenRules)[tlTokInd].begin (), (*tokenRules)[tlTokInd].end (), sortParameter);
}

void
RuleExecution::ruleOuts (map<xml_node, map<int, vector<string> > >* ruleOuts,
			 map<int, vector<pair<xml_node, unsigned> > >* tokenRules,
			 vector<string> slTokens, vector<vector<string> > slTags,
			 vector<string> tlTokens, vector<vector<string> > tlTags,
			 map<xml_node, vector<vector<int> > > rulesApplied,
			 map<string, vector<vector<string> > > attrs,
			 map<string, vector<string> > lists, map<string, string>* vars,
			 vector<string> spaces, string localeId)
{

  for (map<xml_node, vector<vector<int> > >::iterator it = rulesApplied.begin ();
      it != rulesApplied.end (); ++it)
    {
      xml_node rule = it->first;
      for (unsigned i = 0; i < rulesApplied[rule].size (); i++)
	{
	  vector<vector<string> > slAnalysisTokens, tlAnalysisTokens;

	  // format tokens and their tags into analysisTokens

	  vector<int> matchedTokens = rulesApplied[rule][i];

	  for (unsigned j = 0; j < matchedTokens.size (); j++)
	    {
	      int tokInd = matchedTokens[j];

	      vector<string> slAnalysisToken = RuleExecution::formatTokenTags (
		  slTokens[tokInd], slTags[tokInd]);

	      slAnalysisTokens.push_back (slAnalysisToken);

	      vector<string> tlAnalysisToken = RuleExecution::formatTokenTags (
		  tlTokens[tokInd], tlTags[tokInd]);

	      tlAnalysisTokens.push_back (tlAnalysisToken);

	      // insert the rule (if not found) then sort the vector
	      pushDistinct (tokenRules, tokInd, rule, matchedTokens.size ());
	    }

	  vector<string> output = RuleExecution::ruleExe (rule, &slAnalysisTokens,
							  &tlAnalysisTokens, attrs, lists,
							  vars, spaces, matchedTokens[0],
							  localeId); // first pattern index

	  (*ruleOuts)[rule][matchedTokens[0]] = output;
	}
    }

}

vector<string>
RuleExecution::ruleExe (xml_node rule, vector<vector<string> >* slAnalysisTokens,
			vector<vector<string> >* tlAnalysisTokens,
			map<string, vector<vector<string> > > attrs,
			map<string, vector<string> > lists, map<string, string>* vars,
			vector<string> spaces, unsigned firPat, string localeId)
{
  vector<string> output;

  map<unsigned, unsigned> paramToPattern = map<unsigned, unsigned> ();

  for (xml_node child = rule.child (ACTION).first_child (); child;
      child = child.next_sibling ())
    {
      vector<string> result;

      string childName = child.name ();
      if (childName == LET)
	{
	  let (child, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces, firPat,
	       localeId, paramToPattern);
	}
      else if (childName == CHOOSE)
	{
	  result = choose (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
			   spaces, firPat, localeId, paramToPattern);
	}
      else if (childName == CALL_MACRO)
	{
	  result = callMacro (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists,
			      vars, spaces, firPat, localeId, paramToPattern);
	}
      else if (childName == OUT)
	{
	  result = out (child, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces,
			firPat, localeId, paramToPattern);
	}

      else if (childName == MODIFY_CASE)
	modifyCase (child, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces,
		    firPat, localeId, paramToPattern);

      else if (childName == APPEND)
	append (child, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces, firPat,
		localeId, paramToPattern);

      output.insert (output.end (), result.begin (), result.end ());
    }

  return output;
}

vector<string>
RuleExecution::out (xml_node out, vector<vector<string> >* slAnalysisTokens,
		    vector<vector<string> >* tlAnalysisTokens,
		    map<string, vector<vector<string> > > attrs,
		    map<string, string>* vars, vector<string> spaces, unsigned firPat,
		    string localeId, map<unsigned, unsigned> paramToPattern)
{
  vector<string> output;

  for (xml_node child = out.first_child (); child; child = child.next_sibling ())
    {
      vector<string> result;

      string childName = child.name ();
      if (childName == B)
	{
	  result.push_back (b (child, spaces, firPat, localeId, paramToPattern));
	}
      else if (childName == CHUNK)
	{
	  result = chunk (child, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces,
			  firPat, localeId, paramToPattern);
	}

      output.insert (output.end (), result.begin (), result.end ());
    }

  return output;
}

vector<string>
RuleExecution::chunk (xml_node chunkNode, vector<vector<string> >* slAnalysisTokens,
		      vector<vector<string> >* tlAnalysisTokens,
		      map<string, vector<vector<string> > > attrs,
		      map<string, string>* vars, vector<string> spaces, unsigned firPat,
		      string localeId, map<unsigned, unsigned> paramToPattern)
{
  vector<string> output;

  output.push_back ("^");

  string name = chunkNode.attribute (NAME).value ();
  if (name.empty ())
    {
      string varName = chunkNode.attribute (NAME_FROM).value ();
      name = (*vars)[varName];
    }

  output.push_back (name);

  // tags element must be first item
  xml_node tags = chunkNode.child (TAGS);
  for (xml_node tag = tags.child (TAG); tag; tag = tag.next_sibling ())
    {
      vector<string> result;

      xml_node child = tag.first_child ();
      string childName = child.name ();
      if (childName == CLIP)
	result = clip (child, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces,
		       firPat, localeId, paramToPattern);
      else if (childName == LIT_TAG)
	result = litTag (child);

      output.insert (output.end (), result.begin (), result.end ());
    }

  output.push_back ("{");

  for (xml_node child = tags.next_sibling (); child; child = child.next_sibling ())
    {
      string childName = child.name ();
      if (childName == LU)
	{
	  // lu is the same as concat
	  vector<string> result = concat (child, slAnalysisTokens, tlAnalysisTokens,
					  attrs, vars, spaces, firPat, localeId,
					  paramToPattern);
	  output.push_back ("^");
	  output.insert (output.end (), result.begin (), result.end ());
	  output.push_back ("$");
	}
      else if (childName == MLU)
	{
	  // has only lu children
	  for (xml_node lu = child.first_child (); lu; lu = lu.next_sibling ())
	    {
	      vector<string> result = concat (lu, slAnalysisTokens, tlAnalysisTokens,
					      attrs, vars, spaces, firPat, localeId,
					      paramToPattern);
	      output.push_back ("^");
	      output.insert (output.end (), result.begin (), result.end ());
	      output.push_back ("$");

	      if (lu.next_sibling ())
		output.push_back ("+");
	    }
	}
      else if (childName == B)
	{
	  output.push_back (b (child, spaces, firPat, localeId, paramToPattern));
	}
    }

  output.push_back ("}$");

  return output;
}

vector<string>
RuleExecution::callMacro (xml_node callMacroNode,
			  vector<vector<string> >* slAnalysisTokens,
			  vector<vector<string> >* tlAnalysisTokens,
			  map<string, vector<vector<string> > > attrs,
			  map<string, vector<string> > lists, map<string, string>* vars,
			  vector<string> spaces, unsigned firPat, string localeId,
			  map<unsigned, unsigned> paramToPattern)
{
  vector<string> output;

  string macroName = callMacroNode.attribute (N).value ();

  map<unsigned, unsigned> newParamToPattern;
  unsigned i = 1;
  for (xml_node with_param = callMacroNode.child (WITH_PARAM); with_param; with_param =
      with_param.next_sibling ())
    {
      unsigned pos = with_param.attribute (POS).as_uint ();
      if (paramToPattern.size ())
	pos = paramToPattern[pos];

      newParamToPattern[i++] = pos;
    }

  xml_node transfer = callMacroNode.parent ();
  while (transfer.parent ())
    transfer = transfer.parent ();

  xml_node macros = transfer.child (SECTION_DEF_MACROS);

  xml_node macro;
  for (macro = macros.child (DEF_MACRO); macro; macro = macro.next_sibling ())
    {
      if (string (macro.attribute (N).value ()) == macroName)
	break;
    }

  for (xml_node child = macro.first_child (); child; child = child.next_sibling ())
    {
      vector<string> result;

      string childName = child.name ();
      if (childName == CHOOSE)
	result = choose (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
			 spaces, firPat, localeId, paramToPattern);

      else if (childName == OUT)
	result = out (child, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces,
		      firPat, localeId, paramToPattern);

      else if (childName == CALL_MACRO)
	result = callMacro (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
			    spaces, firPat, localeId, paramToPattern);

      else if (childName == LET)
	let (child, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces, firPat,
	     localeId, paramToPattern);

      else if (childName == MODIFY_CASE)
	modifyCase (child, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces,
		    firPat, localeId, paramToPattern);

      else if (childName == APPEND)
	append (child, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces, firPat,
		localeId, paramToPattern);

      output.insert (output.end (), result.begin (), result.end ());
    }

  return output;
}

vector<string>
RuleExecution::findAttrPart (vector<string> tokenTags, vector<vector<string> > attrTags)
{

  vector<string> matchedTags;
  for (unsigned i = 0; i < tokenTags.size (); i++)
    {
      for (unsigned j = 0; j < attrTags.size (); j++)
	{
	  if (tokenTags[i] == ("<" + attrTags[j][0] + ">"))
	    {
	      matchedTags.push_back (tokenTags[i]);
	      for (unsigned k = 1; k < attrTags[j].size () && (k + i) < tokenTags.size ();
		  k++)
		{

		  if (tokenTags[i + k] == ("<" + attrTags[j][k] + ">"))
		    matchedTags.push_back (tokenTags[i + k]);
		  else
		    break;
		}
	      if (matchedTags.size () == attrTags[j].size ())
		return matchedTags;
	      else
		matchedTags.clear ();
	    }
	}
    }
  return matchedTags;
}

bool
RuleExecution::equal (xml_node equal, vector<vector<string> >* slAnalysisTokens,
		      vector<vector<string> >* tlAnalysisTokens,
		      map<string, vector<vector<string> > > attrs,
		      map<string, string>* vars, vector<string> spaces, unsigned firPat,
		      string localeId, map<unsigned, unsigned> paramToPattern)
{

  xml_node firstChild = equal.first_child ();
  vector<string> firstResult;

  string firstName = firstChild.name ();
  if (firstName == CLIP)
    {
      firstResult = clip (firstChild, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
			  spaces, firPat, localeId, paramToPattern);
    }
  else if (firstName == CONCAT)
    {
      firstResult = concat (firstChild, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
			    spaces, firPat, localeId, paramToPattern);
    }
  else if (firstName == LIT_TAG)
    {
      firstResult = litTag (firstChild);
    }
  else if (firstName == LIT)
    {
      firstResult.push_back (lit (firstChild));
    }
  else if (firstName == B)
    {
      firstResult.push_back (b (firstChild, spaces, firPat, localeId, paramToPattern));
    }
  else if (firstName == CASE_OF)
    {
      firstResult.push_back (
	  caseOf (firstChild, slAnalysisTokens, tlAnalysisTokens, localeId,
		  paramToPattern));
    }
  else if (firstName == GET_CASE_FROM)
    {
      firstResult.push_back (
	  getCaseFrom (firstChild, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
		       spaces, firPat, localeId, paramToPattern));
    }
  else if (firstName == VAR)
    {
      firstResult.push_back (var (firstChild, vars));
    }

  xml_node secondChild = firstChild.next_sibling ();
  vector<string> secondResult;

  string secondName = secondChild.name ();
  if (secondName == CLIP)
    {
      secondResult = clip (secondChild, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
			   spaces, firPat, localeId, paramToPattern);
    }
  else if (secondName == CONCAT)
    {
      secondResult = concat (secondChild, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
			     spaces, firPat, localeId, paramToPattern);
    }
  else if (secondName == LIT_TAG)
    {
      secondResult = litTag (secondChild);
    }
  else if (secondName == LIT)
    {
      secondResult.push_back (secondChild.attribute (V).value ());
    }
  else if (secondName == B)
    {
      secondResult.push_back (b (secondChild, spaces, firPat, localeId, paramToPattern));
    }
  else if (secondName == CASE_OF)
    {
      secondResult.push_back (
	  caseOf (secondChild, slAnalysisTokens, tlAnalysisTokens, localeId,
		  paramToPattern));
    }
  else if (secondName == GET_CASE_FROM)
    {
      secondResult.push_back (
	  getCaseFrom (secondChild, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
		       spaces, firPat, localeId, paramToPattern));
    }
  else if (secondName == VAR)
    {
      secondResult.push_back (var (secondChild, vars));
    }

  string firstStr, secondStr;
  for (unsigned i = 0; i < firstResult.size (); i++)
    {
      firstStr += firstResult[i];
    }
  for (unsigned i = 0; i < secondResult.size (); i++)
    {
      secondStr += secondResult[i];
    }

  xml_attribute caseless = equal.attribute (CASE_LESS);
  if (string (caseless.value ()) == "yes")
    {
      return !(CLExec::compareCaseless (firstStr, secondStr, localeId));
    }
  else
    {
      return !(CLExec::compare (firstStr, secondStr));
    }
}

vector<string>
RuleExecution::choose (xml_node chooseNode, vector<vector<string> >* slAnalysisTokens,
		       vector<vector<string> >* tlAnalysisTokens,
		       map<string, vector<vector<string> > > attrs,
		       map<string, vector<string> > lists, map<string, string>* vars,
		       vector<string> spaces, unsigned firPat, string localeId,
		       map<unsigned, unsigned> paramToPattern)
{
  vector<string> output;

  for (xml_node child = chooseNode.first_child (); child; child = child.next_sibling ())
    {
      bool condition = false;

      string childName = child.name ();
      if (childName == WHEN)
	{
	  xml_node testNode = child.child (TEST);

	  condition = test (testNode, slAnalysisTokens, tlAnalysisTokens, attrs, lists,
			    vars, spaces, firPat, localeId, paramToPattern);
	}
      else
	{
	  // otherwise
	  condition = true;
	}

      if (condition)
	{
	  for (xml_node inst = child.first_child (); inst; inst = inst.next_sibling ())
	    {
	      vector<string> result;

	      string instName = inst.name ();
	      if (instName == CHOOSE)
		result = choose (inst, slAnalysisTokens, tlAnalysisTokens, attrs, lists,
				 vars, spaces, firPat, localeId, paramToPattern);

	      else if (instName == OUT)
		result = out (inst, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
			      spaces, firPat, localeId, paramToPattern);

	      else if (instName == CALL_MACRO)
		result = callMacro (inst, slAnalysisTokens, tlAnalysisTokens, attrs,
				    lists, vars, spaces, firPat, localeId,
				    paramToPattern);

	      else if (instName == LET)
		let (inst, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces,
		     firPat, localeId, paramToPattern);

	      else if (instName == MODIFY_CASE)
		modifyCase (inst, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces,
			    firPat, localeId, paramToPattern);

	      else if (instName == APPEND)
		append (inst, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces,
			firPat, localeId, paramToPattern);

	      output.insert (output.end (), result.begin (), result.end ());
	    }
	  break;
	}
    }

  return output;
}

bool
RuleExecution::test (xml_node test, vector<vector<string> >* slAnalysisTokens,
		     vector<vector<string> >* tlAnalysisTokens,
		     map<string, vector<vector<string> > > attrs,
		     map<string, vector<string> > lists, map<string, string>* vars,
		     vector<string> spaces, unsigned firPat, string localeId,
		     map<unsigned, unsigned> paramToPattern)
{
  xml_node child = test.first_child ();
  string childName = child.name ();

  bool condition = false;

  if (childName == EQUAL)
    {
      condition = equal (child, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces,
			 firPat, localeId, paramToPattern);
    }
  else if (childName == AND)
    {
      condition = And (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
		       spaces, firPat, localeId, paramToPattern);
    }
  else if (childName == OR)
    {
      condition = Or (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
		      spaces, firPat, localeId, paramToPattern);
    }
  else if (childName == NOT)
    {
      condition = Not (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
		       spaces, firPat, localeId, paramToPattern);
    }
  else if (childName == IN)
    {
      condition = in (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
		      spaces, firPat, localeId, paramToPattern);
    }

  return condition;
}

bool
RuleExecution::And (xml_node andNode, vector<vector<string> >* slAnalysisTokens,
		    vector<vector<string> >* tlAnalysisTokens,
		    map<string, vector<vector<string> > > attrs,
		    map<string, vector<string> > lists, map<string, string>* vars,
		    vector<string> spaces, unsigned firPat, string localeId,
		    map<unsigned, unsigned> paramToPattern)
{
  bool condition = false;

  for (xml_node child = andNode.first_child (); child; child = child.next_sibling ())
    {
      string childName = child.name ();
      if (childName == EQUAL)
	{
	  condition = equal (child, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
			     spaces, firPat, localeId, paramToPattern);
	}
      else if (childName == AND)
	{
	  condition = And (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
			   spaces, firPat, localeId, paramToPattern);
	}
      else if (childName == OR)
	{
	  condition = Or (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
			  spaces, firPat, localeId, paramToPattern);
	}
      else if (childName == NOT)
	{
	  condition = Not (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
			   spaces, firPat, localeId, paramToPattern);
	}
      else if (childName == IN)
	{
	  condition = in (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
			  spaces, firPat, localeId, paramToPattern);
	}

      if (!condition)
	break;
    }

  return condition;
}

bool
RuleExecution::Or (xml_node orNode, vector<vector<string> >* slAnalysisTokens,
		   vector<vector<string> >* tlAnalysisTokens,
		   map<string, vector<vector<string> > > attrs,
		   map<string, vector<string> > lists, map<string, string>* vars,
		   vector<string> spaces, unsigned firPat, string localeId,
		   map<unsigned, unsigned> paramToPattern)
{
  bool condition = false;

  for (xml_node child = orNode.first_child (); child; child = child.next_sibling ())
    {
      string childName = child.name ();
      if (childName == EQUAL)
	{
	  condition = equal (child, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
			     spaces, firPat, localeId, paramToPattern);
	}
      else if (childName == AND)
	{
	  condition = And (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
			   spaces, firPat, localeId, paramToPattern);
	}
      else if (childName == OR)
	{
	  condition = Or (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
			  spaces, firPat, localeId, paramToPattern);
	}
      else if (childName == NOT)
	{
	  condition = Not (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
			   spaces, firPat, localeId, paramToPattern);
	}
      else if (childName == IN)
	{
	  condition = in (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
			  spaces, firPat, localeId, paramToPattern);
	}

      if (condition)
	break;
    }

  return condition;
}

bool
RuleExecution::in (xml_node inNode, vector<vector<string> >* slAnalysisTokens,
		   vector<vector<string> >* tlAnalysisTokens,
		   map<string, vector<vector<string> > > attrs,
		   map<string, vector<string> > lists, map<string, string>* vars,
		   vector<string> spaces, unsigned firPat, string localeId,
		   map<unsigned, unsigned> paramToPattern)
{
  xml_node firstChild = inNode.first_child ();
  vector<string> firstResult;

  string firstName = firstChild.name ();
  if (firstName == CLIP)
    {
      firstResult = clip (firstChild, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
			  spaces, firPat, localeId, paramToPattern);
    }
  else if (firstName == CONCAT)
    {
      firstResult = concat (firstChild, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
			    spaces, firPat, localeId, paramToPattern);
    }
  else if (firstName == LIT_TAG)
    {
      firstResult = litTag (firstChild);
    }
  else if (firstName == LIT)
    {
      firstResult.push_back (lit (firstChild));
    }
  else if (firstName == B)
    {
      firstResult.push_back (b (firstChild, spaces, firPat, localeId, paramToPattern));
    }
  else if (firstName == CASE_OF)
    {
      firstResult.push_back (
	  caseOf (firstChild, slAnalysisTokens, tlAnalysisTokens, localeId,
		  paramToPattern));
    }
  else if (firstName == GET_CASE_FROM)
    {
      firstResult.push_back (
	  getCaseFrom (firstChild, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
		       spaces, firPat, localeId, paramToPattern));
    }
  else if (firstName == VAR)
    {
      firstResult.push_back (var (firstChild, vars));
    }

  string firstStr;
  for (unsigned i = 0; i < firstResult.size (); i++)
    {
      firstStr += firstResult[i];
    }

  xml_node listNode = firstChild.next_sibling ();

  string listName = listNode.attribute (N).value ();
  vector<string> list = lists[listName];

  xml_attribute caseless = inNode.attribute (CASE_LESS);
  if (string (caseless.value ()) == "yes")
    {
      for (unsigned i = 0; i < list.size (); i++)
	{
	  if (!CLExec::compareCaseless (firstStr, list[i], localeId))
	    return true;
	}
    }
  else
    {
      for (unsigned i = 0; i < list.size (); i++)
	{
	  if (!CLExec::compare (firstStr, list[i]))
	    return true;
	}
    }

  return false;
}

bool
RuleExecution::Not (xml_node NotNode, vector<vector<string> >* slAnalysisTokens,
		    vector<vector<string> >* tlAnalysisTokens,
		    map<string, vector<vector<string> > > attrs,
		    map<string, vector<string> > lists, map<string, string>* vars,
		    vector<string> spaces, unsigned firPat, string localeId,
		    map<unsigned, unsigned> paramToPattern)
{
  xml_node child = NotNode.first_child ();
  string childName = child.name ();

  bool condition = false;

  if (childName == EQUAL)
    {
      condition = equal (child, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces,
			 firPat, localeId, paramToPattern);
    }
  else if (childName == AND)
    {
      condition = And (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
		       spaces, firPat, localeId, paramToPattern);
    }
  else if (childName == OR)
    {
      condition = Or (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
		      spaces, firPat, localeId, paramToPattern);
    }
  else if (childName == NOT)
    {
      condition = Not (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
		       spaces, firPat, localeId, paramToPattern);
    }
  else if (childName == IN)
    {
      condition = in (child, slAnalysisTokens, tlAnalysisTokens, attrs, lists, vars,
		      spaces, firPat, localeId, paramToPattern);
    }

  return !condition;
}

vector<string>
RuleExecution::litTag (xml_node litTag)
{
  // splitting tags by '.'
  string tagsString = litTag.attribute (V).value ();
  char tagsChars[tagsString.size ()];
  strcpy (tagsChars, tagsString.c_str ());

  vector<string> tags;

  char * tag;
  tag = strtok (tagsChars, ".");
  while (tag != NULL)
    {
      tags.push_back ("<" + string (tag) + ">");
      tag = strtok (NULL, ".");
    }

  return tags;
}

string
RuleExecution::lit (xml_node lit)
{
  string litValue = lit.attribute (V).value ();
  return litValue;
}

string
RuleExecution::var (xml_node var, map<string, string>* vars)
{
  string varName = var.attribute (N).value ();
  string varValue = (*vars)[varName];
  return varValue;
}

void
RuleExecution::let (xml_node let, vector<vector<string> >* slAnalysisTokens,
		    vector<vector<string> >* tlAnalysisTokens,
		    map<string, vector<vector<string> > > attrs,
		    map<string, string>* vars, vector<string> spaces, unsigned firPat,
		    string localeId, map<unsigned, unsigned> paramToPattern)
{

  xml_node firstChild = let.first_child ();
  xml_node secondChild = firstChild.next_sibling ();

  string secondName = secondChild.name ();

  vector<string> secondResult;
  if (secondName == CLIP)
    {
      secondResult = clip (secondChild, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
			   spaces, firPat, localeId, paramToPattern);
    }
  else if (secondName == CONCAT)
    {
      secondResult = concat (secondChild, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
			     spaces, firPat, localeId, paramToPattern);
    }
  else if (secondName == LIT_TAG)
    {
      secondResult = litTag (secondChild);
    }
  else if (secondName == LIT)
    {
      secondResult.push_back (secondChild.attribute (V).value ());
    }
  else if (secondName == B)
    {
      secondResult.push_back (b (secondChild, spaces, firPat, localeId, paramToPattern));
    }
  else if (secondName == CASE_OF)
    {
      secondResult.push_back (
	  caseOf (secondChild, slAnalysisTokens, tlAnalysisTokens, localeId,
		  paramToPattern));
    }
  else if (secondName == GET_CASE_FROM)
    {
      secondResult.push_back (
	  getCaseFrom (secondChild, slAnalysisTokens, tlAnalysisTokens, attrs, vars,
		       spaces, firPat, localeId, paramToPattern));
    }
  else if (secondName == VAR)
    {
      secondResult.push_back (var (secondChild, vars));
    }

  string firstName = firstChild.name ();
  if (firstName == VAR)
    {
      string resultStr;
      for (unsigned i = 0; i < secondResult.size (); i++)
	resultStr += secondResult[i];

      string varName = firstChild.attribute (N).value ();
      (*vars)[varName] = resultStr;
    }
  else if (firstName == CLIP)
    {
      vector<string> firstResult = clip (firstChild, slAnalysisTokens, tlAnalysisTokens,
					 attrs, vars, spaces, firPat, localeId,
					 paramToPattern);
      if (firstResult.empty ())
	return;

      unsigned pos = firstChild.attribute (POS).as_uint ();
      if (paramToPattern.size ())
	pos = paramToPattern[pos];
      pos--;

      vector<vector<string> >* analysisTokens = slAnalysisTokens;
      string side = firstChild.attribute (SIDE).value ();
      if (side == TL)
	analysisTokens = tlAnalysisTokens;

      vector<string>* analysisToken = &((*analysisTokens)[pos]);
      // exchange the first part with the second part
      for (unsigned i = 0; i < analysisToken->size (); i++)
	{
	  if ((*analysisToken)[i] == firstResult[0])
	    {
	      analysisToken->erase (analysisToken->begin () + i,
				    analysisToken->begin () + i + firstResult.size ());
	      analysisToken->insert (analysisToken->begin () + i, secondResult.begin (),
				     secondResult.end ());
	      break;
	    }
	}
    }
}

// put the token and its tags in one vector and put tags between "<" , ">"
// the analysis will be done on this vector , "<>" to differ between tags and non-tags
// and the token for the lemma
vector<string>
RuleExecution::formatTokenTags (string token, vector<string> tags)
{

  vector<string> analysisToken;
  analysisToken.push_back (token);

  for (unsigned i = 0; i < tags.size (); i++)
    {
      analysisToken.push_back ("<" + tags[i] + ">");
    }

  return analysisToken;
}

vector<string>
RuleExecution::clip (xml_node clip, vector<vector<string> >* slAnalysisTokens,
		     vector<vector<string> >* tlAnalysisTokens,
		     map<string, vector<vector<string> > > attrs,
		     map<string, string>* vars, vector<string> spaces, unsigned firPat,
		     string localeId, map<unsigned, unsigned> paramToPattern)
{
  vector<string> result;

  unsigned pos = clip.attribute (POS).as_uint ();
  if (paramToPattern.size ())
    pos = paramToPattern[pos];
  pos--;

  string part = clip.attribute (PART).value ();

  xml_attribute linkTo = clip.attribute (LINK_TO);
  if (string (linkTo.name ()) == LINK_TO)
    {
      result = attrs[part][pos];
      return result;
    }

  string side = clip.attribute (SIDE).value ();

//  cout << pos << "  " << slAnalysisTokens->size () << "  " << tlAnalysisTokens->size ()
//      << endl;
  vector<string> analysisToken = (*slAnalysisTokens)[pos];

  if (side == TL)
    analysisToken = (*tlAnalysisTokens)[pos];

  if (part == WHOLE)
    {
      result = analysisToken;
    }
  else if (part == LEM)
    {
      result.push_back (analysisToken[0]);
    }
  else if (part == LEMH || part == LEMQ)
    {
      string lem = analysisToken[0];
      size_t spaceInd = lem.find (' ');
      if (spaceInd == string::npos)
	{
	  if (part == LEMH)
	    result.push_back (lem);
	  else
	    result.push_back ("");
	}
      else
	{
	  string lemh = lem.substr (0, spaceInd);
	  string lemq = lem.substr (spaceInd);

	  if (part == LEMH)
	    result.push_back (lemh);
	  else
	    result.push_back (lemq);
	}
    }
  // part == "attr"
  else
    {
      result = RuleExecution::findAttrPart (analysisToken, attrs[part]);
    }

  return result;
}

vector<string>
RuleExecution::concat (xml_node concat, vector<vector<string> >* slAnalysisTokens,
		       vector<vector<string> >* tlAnalysisTokens,
		       map<string, vector<vector<string> > > attrs,
		       map<string, string>* vars, vector<string> spaces, unsigned firPat,
		       string localeId, map<unsigned, unsigned> paramToPattern)
{

  vector<string> concatResult;

  for (xml_node node = concat.first_child (); node; node = node.next_sibling ())
    {
      vector<string> result;

      string nodeName = node.name ();
      if (nodeName == CLIP)
	{
	  result = clip (node, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces,
			 firPat, localeId, paramToPattern);
	}
      else if (nodeName == LIT_TAG)
	{
	  result = litTag (node);
	}
      else if (nodeName == LIT)
	{
	  result.push_back (lit (node));
	}
      else if (nodeName == GET_CASE_FROM)
	{
	  result.push_back (
	      getCaseFrom (node, slAnalysisTokens, tlAnalysisTokens, attrs, vars, spaces,
			   firPat, localeId, paramToPattern));
	}
      else if (nodeName == CASE_OF)
	{
	  result.push_back (
	      caseOf (node, slAnalysisTokens, tlAnalysisTokens, localeId,
		      paramToPattern));
	}
      else if (nodeName == B)
	{
	  result.push_back (b (node, spaces, firPat, localeId, paramToPattern));
	}
      else if (nodeName == VAR)
	{
	  result.push_back (var (node, vars));
	}

      concatResult.insert (concatResult.end (), result.begin (), result.end ());
    }

  return concatResult;
}

void
RuleExecution::append (xml_node append, vector<vector<string> >* slAnalysisTokens,
		       vector<vector<string> >* tlAnalysisTokens,
		       map<string, vector<vector<string> > > attrs,
		       map<string, string>* vars, vector<string> spaces, unsigned firPat,
		       string localeId, map<unsigned, unsigned> paramToPattern)
{
  string varName = append.attribute (NAME).value ();

  vector<string> result;

  for (xml_node child = append.first_child (); child; child = child.next_sibling ())
    {
      string childName = child.name ();
      if (childName == CLIP)
	{
	  vector<string> clipResult = clip (child, slAnalysisTokens, tlAnalysisTokens,
					    attrs, vars, spaces, firPat, localeId,
					    paramToPattern);
	  result.insert (result.end (), clipResult.begin (), clipResult.end ());
	}
      else if (childName == LIT_TAG)
	{
	  vector<string> litTagResult = litTag (child);
	  result.insert (result.end (), litTagResult.begin (), litTagResult.end ());
	}
      else if (childName == LIT)
	{
	  string litResult = lit (child);
	  result.push_back (litResult);
	}
      else if (childName == VAR)
	{
	  string varResult = var (child, vars);
	  result.push_back (varResult);
	}
      else if (childName == CONCAT)
	{
	  vector<string> concatResult = concat (child, slAnalysisTokens, tlAnalysisTokens,
						attrs, vars, spaces, firPat, localeId,
						paramToPattern);
	  result.insert (result.end (), concatResult.begin (), concatResult.end ());
	}
      else if (childName == B)
	{
	  string bResult = b (child, spaces, firPat, localeId, paramToPattern);
	  result.push_back (bResult);
	}
      else if (childName == GET_CASE_FROM)
	{
	  string getCaseFromResult = getCaseFrom (child, slAnalysisTokens,
						  tlAnalysisTokens, attrs, vars, spaces,
						  firPat, localeId, paramToPattern);
	  result.push_back (getCaseFromResult);
	}
      else if (childName == CASE_OF)
	{
	  string caseOfResult = caseOf (child, slAnalysisTokens, tlAnalysisTokens,
					localeId, paramToPattern);
	  result.push_back (caseOfResult);
	}

    }

  string newVarValue = (*vars)[varName];
  for (unsigned i = 0; i < result.size (); i++)
    {
      newVarValue += result[i];
    }
  (*vars)[varName] = newVarValue;
}

string
RuleExecution::b (xml_node b, vector<string> spaces, unsigned firPat, string localeId,
		  map<unsigned, unsigned> paramToPattern)
{
  string blank;
  xml_attribute posAtt = b.attribute (POS);
  if (string (posAtt.name ()) == POS)
    {
      unsigned pos = posAtt.as_uint ();
      if (paramToPattern.size ())
	pos = paramToPattern[pos];
      pos--;

      unsigned spacePos = firPat + (pos);
      blank = spaces[spacePos];
    }
  else
    {
      blank = " ";
    }
  return blank;
}

string
RuleExecution::caseOf (xml_node caseOf, vector<vector<string> >* slAnalysisTokens,
		       vector<vector<string> >* tlAnalysisTokens, string localeId,
		       map<unsigned, unsigned> paramToPattern)
{
  string Case;

  unsigned pos = caseOf.attribute (POS).as_uint ();
  if (paramToPattern.size ())
    pos = paramToPattern[pos];
  pos--;

  string part = caseOf.attribute (PART).value ();

  if (part == LEM)
    {
      string side = caseOf.attribute (SIDE).value ();

      string token;
      if (side == SL)
	token = (*slAnalysisTokens)[pos][0];
      else
	token = (*tlAnalysisTokens)[pos][0];

      if (token == CLExec::toLowerCase (token, localeId))
	Case = aa;
      else if (token == CLExec::toUpperCase (token, localeId))
	Case = AA;
      else
	Case = Aa;
    }

  return Case;
}

string
RuleExecution::getCaseFrom (xml_node getCaseFrom,
			    vector<vector<string> >* slAnalysisTokens,
			    vector<vector<string> >* tlAnalysisTokens,
			    map<string, vector<vector<string> > > attrs,
			    map<string, string>* vars, vector<string> spaces,
			    unsigned firPat, string localeId,
			    map<unsigned, unsigned> paramToPattern)
{
  string result;

  unsigned pos = getCaseFrom.attribute (POS).as_uint ();
  if (paramToPattern.size ())
    pos = paramToPattern[pos];
  pos--;

  xml_node child = getCaseFrom.first_child ();
  string childName = child.name ();

  if (childName == LIT)
    {
      result = lit (child);
    }
  else if (childName == VAR)
    {
      result = var (child, vars);
    }
  else if (childName == CLIP)
    {
      vector<string> clipResult = clip (child, slAnalysisTokens, tlAnalysisTokens, attrs,
					vars, spaces, firPat, localeId, paramToPattern);

      for (unsigned i = 0; i < clipResult.size (); i++)
	{
	  result += clipResult[i];
	}
    }

  string slToken = (*slAnalysisTokens)[pos][0];

  if (slToken == CLExec::toLowerCase (slToken, localeId))
    result = CLExec::toLowerCase (result, localeId);
  else if (slToken == CLExec::toUpperCase (slToken, localeId))
    result = CLExec::toUpperCase (result, localeId);
  else
    result = CLExec::FirLetUpperCase (result, localeId);

  return result;
}

void
RuleExecution::modifyCase (xml_node modifyCase, vector<vector<string> >* slAnalysisTokens,
			   vector<vector<string> >* tlAnalysisTokens,
			   map<string, vector<vector<string> > > attrs,
			   map<string, string>* vars, vector<string> spaces,
			   unsigned firPat, string localeId,
			   map<unsigned, unsigned> paramToPattern)
{

  xml_node firstChild = modifyCase.first_child ();
  xml_node secondChild = modifyCase.next_sibling ();

  string childName = secondChild.name ();

  string Case;
  if (childName == LIT)
    {
      Case = lit (secondChild);
    }
  else if (childName == VAR)
    {
      Case = var (secondChild, vars);
    }

  childName = firstChild.name ();
  if (childName == VAR)
    {
      string varName = firstChild.attribute (N).value ();

      if (Case == aa)
	(*vars)[varName] = CLExec::toLowerCase ((*vars)[varName], localeId);
      else if (Case == AA)
	(*vars)[varName] = CLExec::toUpperCase ((*vars)[varName], localeId);
      else if (Case == Aa)
	(*vars)[varName] = CLExec::FirLetUpperCase ((*vars)[varName], localeId);

    }
  else if (childName == CLIP)
    {
      unsigned pos = firstChild.attribute (POS).as_uint ();
      if (paramToPattern.size ())
	pos = paramToPattern[pos];
      pos--;

      string side = firstChild.attribute (SIDE).value ();

      vector<vector<string> >* analysisTokens;
      if (side == SL)
	analysisTokens = slAnalysisTokens;
      else
	analysisTokens = tlAnalysisTokens;

      string part = firstChild.attribute (PART).value ();

      if (part == LEM)
	{
	  if (Case == aa)
	    (*analysisTokens)[pos][0] = CLExec::toLowerCase ((*analysisTokens)[pos][0],
							     localeId);
	  else if (Case == AA)
	    (*analysisTokens)[pos][0] = CLExec::toUpperCase ((*analysisTokens)[pos][0],
							     localeId);
	  else if (Case == Aa)
	    (*analysisTokens)[pos][0] = CLExec::FirLetUpperCase (
		(*analysisTokens)[pos][0], localeId);
	}
      else if (part == LEMH || part == LEMQ)
	{
	  string lem = (*analysisTokens)[pos][0];
	  size_t spaceInd = lem.find (' ');
	  if (spaceInd == string::npos)
	    {
	      if (Case == aa)
		lem = CLExec::toLowerCase (lem, localeId);
	      else if (Case == AA)
		lem = CLExec::toUpperCase (lem, localeId);
	      else if (Case == Aa)
		lem = CLExec::FirLetUpperCase (lem, localeId);
	    }
	  else
	    {
	      string lemh = lem.substr (0, spaceInd);
	      string lemq = lem.substr (spaceInd);

	      if (part == LEMH)
		{
		  if (Case == aa)
		    lemh = CLExec::toLowerCase (lemh, localeId);
		  else if (Case == AA)
		    lemh = CLExec::toUpperCase (lemh, localeId);
		  else if (Case == Aa)
		    lemh = CLExec::FirLetUpperCase (lemh, localeId);
		}
	      else
		{
		  if (Case == aa)
		    lemq = CLExec::toLowerCase (lemq, localeId);
		  else if (Case == AA)
		    lemq = CLExec::toUpperCase (lemq, localeId);
		  else if (Case == Aa)
		    lemq = CLExec::FirLetUpperCase (lemq, localeId);
		}

	      lem = lemh + lemq;
	    }
	}

    }

}
