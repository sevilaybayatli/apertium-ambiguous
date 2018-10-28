#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <algorithm>

#include "../pugixml/pugixml.hpp"
#include "TranElemLiterals.h"

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
//	  newCurNestedRules.insert (newCurNestedRules.end (), output.begin (), output.end ());
	  newCurNestedRules.push_back (rule);

	  vector<string> newOutput = vector<string> (output);
//	  newOutput.insert (newOutput.end (), output.begin (), output.end ());

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
	      string outRule = vOutRules[i][j][k].first_attribute ().value ();

	      // we want the id only (after the last '-')
	      output.push_back (
		  atoi (outRule.substr (outRule.find_last_of ("-") + 1).c_str ()));
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
//  cout << endl << "nestedoutsRules size : " << nestedOutsRules.size () << endl;

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
//      pair<unsigned, unsigned> p2 = p1.first;
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
//	  for (unsigned m = 0; m < weigInd.size (); m++)
//	    cout << weigInd[m] << " , ";
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
			 vector<string> tlTokens, vector<vector<string> > tags,
			 map<xml_node, vector<vector<int> > > rulesApplied,
			 map<string, vector<vector<string> > > attrs)
{

//  cout << "Sizes  " << slTokenRule.size () << "  " << tlTokenRule.size () << "  "
//      << slTokenTag.size () << "  " << tlTokenTag.size () << endl;

  for (map<xml_node, vector<vector<int> > >::iterator it = rulesApplied.begin ();
      it != rulesApplied.end (); ++it)
    {
      xml_node rule = it->first;
      for (unsigned i = 0; i < rulesApplied[rule].size (); i++)
	{
	  vector<vector<string> > tlAnalysisTokens;

	  // format tokens and their tags into analysisTokens

	  vector<int> tlMatchedTokens = rulesApplied[rule][i];

	  for (unsigned j = 0; j < tlMatchedTokens.size (); j++)
	    {
//	      cout << "HERE1  " << endl;
	      int tlTokInd = tlMatchedTokens[j];

	      vector<string> tlAnalysisToken = RuleExecution::formatTokenTags (
		  tlTokens[tlTokInd], tags[tlTokInd]);

	      tlAnalysisTokens.push_back (tlAnalysisToken);

	      // insert the rule (if not found) then sort the vector
	      pushDistinct (tokenRules, tlTokInd, rule, tlMatchedTokens.size ());
//	      cout << "HERE2  " << endl;
	    }

//	  cout << "HERE3  " << endl;

	  vector<string> output = RuleExecution::ruleExe (rule, &tlAnalysisTokens, attrs);

	  //	  cout << "rule :  " << rule.first_attribute ().value () << endl;
//	  for (unsigned x = 0; x < output.size (); x++)
//	    {
//	      cout << output[x];
//	    }
//	  cout << endl;

	  (*ruleOuts)[rule][tlMatchedTokens[0]] = output;
	}
    }

}

vector<string>
RuleExecution::ruleExe (xml_node rule, vector<vector<string> >* tlAnalysisTokens,
			map<string, vector<vector<string> > > attrs)
{
  vector<string> output;

//  cout << "Before execution" << endl;
//  for (unsigned i = 0; i < tlAnalysisTokens->size (); i++)
//    {
//      for (unsigned j = 0; j < (*tlAnalysisTokens)[i].size (); j++)
//	{
//	  cout << (*tlAnalysisTokens)[i][j] << "  ";
//	}
//      cout << endl;
//    }

  for (xml_node node = rule.child (ACTION).first_child (); node; node =
      node.next_sibling ())
    {

      string nodeName = node.name ();
      if (nodeName == LET)
	{
//	  cout << "HERE4  " << rule.first_attribute ().value () << endl;
	  letAction (node, tlAnalysisTokens, attrs);
//	  cout << "HERE5  " << endl;
	}
      else if (nodeName == CHOOSE)
	{
	  chooseAction (node, tlAnalysisTokens, attrs);
	}
      else if (nodeName == CALL_MACRO)
	{
	  macroAction (node, tlAnalysisTokens, attrs);
	}
      else if (nodeName == OUT)
	{
	  output = outAction (node, tlAnalysisTokens, attrs);
	}

    }

//  cout << "After execution" << endl;
//  for (unsigned i = 0; i < tlAnalysisTokens->size (); i++)
//    {
//      for (unsigned j = 0; j < (*tlAnalysisTokens)[i].size (); j++)
//	{
//	  cout << (*tlAnalysisTokens)[i][j] << "  ";
//	}
//      cout << endl;
//    }

  return output;
}

vector<string>
RuleExecution::outAction (xml_node out, vector<vector<string> >* tlAnalysisTokens,
			  map<string, vector<vector<string> > > attrs)
{
  vector<string> output;
  output.push_back ("^");

  // there is only one chunk
  xml_node chunk = out.child (CHUNK);
  output.push_back (chunk.attribute (NAME).value ());

  // there is only one tags
  xml_node tags = chunk.child (TAGS);
  for (xml_node tag = tags.child (TAG); tag; tag = tag.next_sibling ())
    {
      vector<string> litTag = litTagAction (tag.first_child ());
      output.insert (output.end (), litTag.begin (), litTag.end ());
    }

  output.push_back ("{");

  if (tags.next_sibling (MLU))
    {
      xml_node mlu = tags.next_sibling (MLU);

      for (xml_node lu = mlu.child (LU); lu; lu = lu.next_sibling ())
	{
	  vector<string> luResult = concat (lu, tlAnalysisTokens, attrs);
	  output.push_back ("^");
	  output.insert (output.end (), luResult.begin (), luResult.end ());
	  output.push_back ("$");

	  if (lu.next_sibling ())
	    output.push_back ("+");
	}
    }

  for (xml_node node = tags.next_sibling (); node; node = node.next_sibling ())
    {
      string nodeName = node.name ();
      if (nodeName == LU)
	{
	  vector<string> luResult = concat (node, tlAnalysisTokens, attrs);
	  output.push_back ("^");
	  output.insert (output.end (), luResult.begin (), luResult.end ());
	  output.push_back ("$");
	}
      else if (nodeName == B)
	{
	  output.push_back (" ");
	}
    }

  output.push_back ("}$");

  return output;
}

void
RuleExecution::macroAction (xml_node callMacro, vector<vector<string> >* tlAnalysisTokens,
			    map<string, vector<vector<string> > > attrs)
{

  string macroName = callMacro.attribute (N).value ();

  map<int, int> paramToPattern;
  int i = 1;
  for (xml_node with_param = callMacro.child (WITH_PARAM); with_param; with_param =
      with_param.next_sibling ())
    {
      paramToPattern[i] = with_param.attribute (POS).as_int ();
    }

  xml_node transfer = callMacro.parent ().parent ().parent ().parent ();

  xml_node macros = transfer.child (SECTION_DEF_MACROS);

  for (xml_node macro = macros.child (DEF_MACRO); macro; macro = macro.next_sibling ())
    {
      if (string (macro.attribute (N).value ()) == macroName)
	{
	  chooseAction (macro.child (CHOOSE), tlAnalysisTokens, attrs, paramToPattern);
	  break;
	}
    }
}

vector<string>
RuleExecution::findAttrPart (vector<string> tokenTags, vector<vector<string> > attrTags)
{

  vector<string> matchedTags;
  for (unsigned i = 0; i < tokenTags.size (); i++)
    {
      for (unsigned j = 0; j < attrTags.size (); j++)
	{
//	  cout << tokenTags[i] << "  " << ("<" + attrTags[j][0] + ">") << endl;
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

// equal has only 2 childs
// they are always in this transfer file
// clip and lit-tag only, but we will make it general
bool
RuleExecution::equal (xml_node equal, vector<vector<string> >* tlAnalysisTokens,
		      map<string, vector<vector<string> > > attrs,
		      map<int, int> paramToPattern)
{

  xml_node firstChild = equal.first_child ();
  vector<string> firstResult;

  string firstName = firstChild.name ();
  if (firstName == CLIP)
    {
      firstResult = clipAction (firstChild, tlAnalysisTokens, attrs, paramToPattern);
    }
  else if (firstName == CONCAT)
    {
      firstResult = concat (firstChild, tlAnalysisTokens, attrs, paramToPattern);
    }
  else if (firstName == LIT_TAG)
    {
      firstResult = litTagAction (firstChild);
    }
  else if (firstName == LIT)
    {
      firstResult.push_back (firstChild.attribute (V).value ());
    }

  xml_node secondChild = firstChild.next_sibling ();
  vector<string> secondResult;

  string secondName = secondChild.name ();
  if (secondName == CLIP)
    {
      secondResult = clipAction (secondChild, tlAnalysisTokens, attrs, paramToPattern);
    }
  else if (secondName == CONCAT)
    {
      secondResult = concat (secondChild, tlAnalysisTokens, attrs, paramToPattern);
    }
  else if (secondName == LIT_TAG)
    {
      secondResult = litTagAction (secondChild);
    }
  else if (secondName == LIT)
    {
      secondResult.push_back (secondChild.attribute (V).value ());
    }

  if (firstResult.size () != secondResult.size ())
    return false;

  for (unsigned i = 0; i < firstResult.size (); i++)
    {
      if (firstResult[i] != secondResult[i])
	return false;
    }

  return true;
}

void
RuleExecution::chooseAction (xml_node choose, vector<vector<string> >* tlAnalysisTokens,
			     map<string, vector<vector<string> > > attrs,
			     map<int, int> paramToPattern)
{

  xml_node when = choose.child (WHEN);

  xml_node test = when.child (TEST);

  xml_node node = test.first_child ();
  string nodeName = node.name ();

  bool result = false;

  if (nodeName == EQUAL)
    {
      result = equal (node, tlAnalysisTokens, attrs, paramToPattern);
    }
  else if (nodeName == AND)
    {
      for (xml_node equalNode = node.first_child (); equalNode;
	  equalNode = equalNode.next_sibling ())
	{

	  result = equal (equalNode, tlAnalysisTokens, attrs, paramToPattern);
	  if (!result)
	    break;
	}
    }
  else if (nodeName == OR)
    {
      for (xml_node equalNode = node.first_child (); equalNode;
	  equalNode = equalNode.next_sibling ())
	{

	  result = equal (equalNode, tlAnalysisTokens, attrs, paramToPattern);
	  if (result)
	    break;
	}
    }

  // we assume that let only comes after test
  if (result)
    {
      for (xml_node let = when.child (LET); let; let = let.next_sibling (LET))
	{
	  letAction (let, tlAnalysisTokens, attrs, paramToPattern);
	}
    }
}

vector<string>
RuleExecution::litTagAction (xml_node litTag)
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

void
RuleExecution::letAction (xml_node let, vector<vector<string> >* tlAnalysisTokens,
			  map<string, vector<vector<string> > > attrs,
			  map<int, int> paramToPattern)
{

  // it is always a clip
  xml_node firstChild = let.first_child ();
  vector<string> firstResult = clipAction (firstChild, tlAnalysisTokens, attrs,
					   paramToPattern);

//  cout << "here1  " << firstResult.size () << endl;

  if (firstResult.empty ())
    return;

//  cout << "here2  " << endl;

  xml_node secondChild = firstChild.next_sibling ();

  string secondName = secondChild.name ();
//  cout << "here2" << endl;
  vector<string> secondResult;
//  cout << "here3  " << secondName << endl;

  if (secondName == CLIP)
    {
      secondResult = clipAction (secondChild, tlAnalysisTokens, attrs, paramToPattern);
    }
  else if (secondName == CONCAT)
    {
      secondResult = concat (secondChild, tlAnalysisTokens, attrs, paramToPattern);
    }
  else if (secondName == LIT_TAG)
    {
      secondResult = litTagAction (secondChild);
    }
  else if (secondName == LIT)
    {
      secondResult.push_back (secondChild.attribute (V).value ());
    }

//  cout << "here4" << endl;
  int position = firstChild.attribute (POS).as_int () - 1;
  if (paramToPattern.size ())
    position = paramToPattern[firstChild.attribute (POS).as_int ()] - 1;
//  cout << "here5" << endl;

//  cout << "rule : " << let.parent ().parent ().first_attribute ().value () << " , size : "
//      << firstResult.size () << " , part : " << firstChild.attribute (PART).value ()
//      << " , pos :  " << firstChild.attribute (POS).value () << endl;
//  for (unsigned i = 0; i < (*slAnalysisTokens).size (); i++)
//    {
//      for (unsigned j = 0; j < (*slAnalysisTokens)[i].size (); j++)
//	{
//
//	  cout << (*slAnalysisTokens)[i][j] << "  ";
//	}
//      cout << endl;
//    }
//  cout << endl << endl;

// exchange the first part with the second part
// if the side is TL only
//  cout << (firstChild.attribute (SIDE).value () == TL) << endl;
  if (string (firstChild.attribute (SIDE).value ()) == TL)
    {
//      cout << "HERE6  " << (*tlAnalysisTokens)[position].size () << "  "
//	  << firstResult.size () << "  " << firstChild.attribute (PART).value () << endl;
      for (unsigned i = 0; i < (*tlAnalysisTokens)[position].size (); i++)
	{
	  if ((*tlAnalysisTokens)[position][i] == firstResult[0])
	    {
	      (*tlAnalysisTokens)[position].erase (
		  (*tlAnalysisTokens)[position].begin () + i,
		  (*tlAnalysisTokens)[position].begin () + i + firstResult.size ());
	      (*tlAnalysisTokens)[position].insert (
		  (*tlAnalysisTokens)[position].begin () + i, secondResult.begin (),
		  secondResult.end ());
	      break;
	    }
	}
//      cout << "HERE7  " << endl;
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
RuleExecution::clipAction (xml_node clip, vector<vector<string> >* tlAnalysisTokens,
			   map<string, vector<vector<string> > > attrs,
			   map<int, int> paramToPattern)
{
  vector<string> result;

  int position = clip.attribute (POS).as_int () - 1;
  if (paramToPattern.size ())
    position = paramToPattern[clip.attribute (POS).as_int ()] - 1;

  string langSide = clip.attribute (SIDE).value ();
  string part = clip.attribute (PART).value ();

  vector<string> analysisToken = (*tlAnalysisTokens)[position];

  if (langSide == SL)
    return result;

  string token = analysisToken[0];

  if (part == WHOLE)
    {
      result = analysisToken;
    }
  else if (part == LEM)
    {
      result.push_back (token);
    }
  // part == "attr"
  else
    {
      result = RuleExecution::findAttrPart (analysisToken, attrs[part]);
    }

  return result;
}

vector<string>
RuleExecution::concat (xml_node concat, vector<vector<string> >* tlAnalysisTokens,
		       map<string, vector<vector<string> > > attrs,
		       map<int, int> paramToPattern)
{

  vector<string> result;

  for (xml_node node = concat.first_child (); node; node = node.next_sibling ())
    {

      string nodeName = node.name ();
      if (nodeName == CLIP)
	{
	  vector<string> clipResult = clipAction (node, tlAnalysisTokens, attrs,
						  paramToPattern);
	  result.insert (result.end (), clipResult.begin (), clipResult.end ());
	}
      else if (nodeName == LIT_TAG)
	{
	  vector<string> litTagResult = litTagAction (node);
	  result.insert (result.end (), litTagResult.begin (), litTagResult.end ());
	}
      else if (nodeName == LIT)
	{
	  result.push_back (node.attribute (V).value ());
	}
    }

  return result;
}
