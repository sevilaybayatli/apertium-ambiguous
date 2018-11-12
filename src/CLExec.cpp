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

#include "CLExec.h"
#include "TranElemLiterals.h"
#include "pugixml.hpp"

using namespace std;
using namespace pugi;
using namespace elem;

string
exec (string cmd)
{
  string data;
  FILE * stream;
  const int max_buffer = 256;
  char buffer[max_buffer];

  stream = popen (cmd.c_str (), "r");
  if (stream)
    {
      while (!feof (stream))
	if (fgets (buffer, max_buffer, stream) != NULL)
	  data.append (buffer);
      pclose (stream);
    }
  return data;
}



void
CLExec::biltrans (string inFilePath, string outFilePath)
{
  // clear file before writing again
  ofstream ofs;
  ofs.open (outFilePath.c_str (), ofstream::out | ofstream::trunc);
  exec (
      string ("apertium -d $HOME/apertium-kaz-tur kaz-tur-biltrans ") + inFilePath
	  + string (" ") + outFilePath);
}

void
CLExec::lextor (string inFilePath, string outFilePath)
{
  // clear file before writing again
  ofstream ofs;
  ofs.open (outFilePath.c_str (), ofstream::out | ofstream::trunc);
  exec (
      string ("lrx-proc -m $HOME/apertium-kaz-tur/kaz-tur.autolex.bin ") + inFilePath
	  + string (" >") + outFilePath);
}

void
CLExec::interchunk (string inFilePath, string outFilePath)
{
  exec (
      string ("apertium-interchunk")
	  + string (" $HOME/apertium-kaz-tur/apertium-kaz-tur.kaz-tur.t2x")
	  + string (" $HOME/apertium-kaz-tur/kaz-tur.t2x.bin ") + inFilePath
	  + string (" ") + outFilePath);
}

void
CLExec::postchunk (string inFilePath, string outFilePath)
{
  exec (
      string ("apertium-postchunk")
	  + string (" $HOME/apertium-kaz-tur/apertium-kaz-tur.kaz-tur.t3x")
	  + string (" $HOME/apertium-kaz-tur/kaz-tur.t3x.bin ") + inFilePath
	  + string (" ") + outFilePath);
}

void
CLExec::transfer (string inFilePath, string outFilePath)
{
  exec (
      string ("apertium-transfer -n")
	  + string (" $HOME/apertium-kaz-tur/apertium-kaz-tur.kaz-tur.t4x")
	  + string (" $HOME/apertium-kaz-tur/kaz-tur.t4x.bin ") + inFilePath
	  + string (" | lt-proc -g $HOME/apertium-kaz-tur/kaz-tur.autogen.bin")
	  + string (" | lt-proc -p $HOME/apertium-kaz-tur/kaz-tur.autopgen.bin")
	  + string (" >") + outFilePath);
}


vector<string>
CLExec::getFilesInDir (string dir)
{
  vector<string> files;

  DIR *pDIR;
  struct dirent *entry;
  if ((pDIR = opendir ((string ("./") + dir).c_str ())))
    {
      while ((entry = readdir (pDIR)))
	{
	  if (strcmp (entry->d_name, ".") != 0 && strcmp (entry->d_name, "..") != 0)
	    {
	      files.push_back (entry->d_name);
	    }
	}
      closedir (pDIR);
    }

  return files;
}

void
CLExec::runYasmet ()
{
  vector<string> datasets = getFilesInDir (DATASETS);

  for (unsigned i = 0; i < datasets.size (); i++)
    {
      string dataset = datasets[i];

      exec (
	  (string ("./yasmet <") + DATASETS + string ("/") + dataset + string (">")
	      + MODELS + string ("/") + dataset + string (".model")).c_str ());
    }
}

map<string, map<string, vector<float> > >
CLExec::loadYasmetModels ()
{
  // mpa with key yasmet model name and the value is
  // another map with key word name and the value is
  // vector of weights in order
  map<string, map<string, vector<float> > > classWeights;

  vector<string> models = CLExec::getFilesInDir (MODELS);

  for (unsigned i = 0; i < models.size (); i++)
    {
      string model = models[i];
      ifstream modelFile ((MODELS + string ("/") + model).c_str ());

      if (modelFile.is_open ())
	{
	  string line;

	  // ignore first line
	  getline (modelFile, line);

	  while (getline (modelFile, line))
	    {
	      // split by ':' but there is a case with ':' as feature !!
	      // the solution is splitting by ':[0:9]'
	      // there is another case , without ':' at all
	      // (ие 1) , We should put it in a vector with all weights equal to 1

	      // 0=>word , 1=>rule_num & 2=>wieght
	      // if word = ':' , 0=>rule_num & 1=>wieght
	      vector<string> splits;

	      char lineChar[line.size ()];
	      strcpy (lineChar, line.c_str ());

	      char * split;
	      split = strtok (lineChar, ": ");

	      while (split != NULL)
		{
		  splits.push_back (split);
		  split = strtok (NULL, ": ");
		}

	      // 0=>word , 1=>rule_num & 2=>wieght
	      // if word = ':' , 0=>rule_num & 1=>wieght
	      // we don't need rule number , because
	      // the weights are already sorted

	      float weight = strtof (splits[splits.size () - 1].c_str (), NULL);

	      string word = splits[0];
	      unsigned size = 0;
	      if (splits.size () == 2)
		{
		  if (splits[0].size () == 1)
		    word = ":";
		  else
		    size = classWeights[model].begin ()->second.size ();
		}

	      if (size)
		{
		  vector<float> weights (size, weight);
		  classWeights[model][word] = weights;
		}
	      else
		classWeights[model][word].push_back (weight);
	    }
	}
      else
	{
	  cout << "error in opening model file " << model << endl;
	}
    }
  return classWeights;
}

void
CLExec::handleDatasets ()
{
  string highLetters = "АӘБВГҒДЕЁЖЗИЙКҚЛМНҢОӨПРСТУҰҮФХҺЦЧШЩЪЫІЬЭЮЯ";
  string lowLetters = "аәбвгғдеёжзийкқлмнңоөпрстуұүфхһцчшщъыіьэюя";

  vector<string> datasets = getFilesInDir (DATASETS);

  for (unsigned i = 0; i < datasets.size (); i++)
    {
      string dataset = datasets[i];
      ifstream oldFile ((DATASETS + string ("/") + dataset).c_str ());
      ofstream newFile ((DATASETS + string ("/") + dataset + string ("1")).c_str ());

      if (oldFile.is_open () && newFile.is_open ())
	{
	  string line;

	  // ignore first line
	  getline (oldFile, line);
	  newFile << line << endl;

	  while (getline (oldFile, line))
	    {
	      for (unsigned i = 0; i < highLetters.size () - 1; i += 2)
		{
		  // letter is 2 chars not one
		  string letter = highLetters.substr (i, 2);

		  string::size_type pos = line.find (letter);
		  if (pos != string::npos)
		    {
		      line[pos] = lowLetters[i];
		      line[pos + 1] = lowLetters[i + 1];
		      i++;
		    }
		}

	      string num;
	      for (unsigned i = 0; i < line.size (); i++)
		{
		  if (line[i] == '#')
		    num = "0";
		  else if (line[i] == ':')
		    {
		      line.insert (i, string ("_") + num);
		      if (!num.compare ("0"))
			num = "1";
		      else
			num = "2";
		      i += 2;
		    }
		  else if (line[i] == ' ')
		    {
		      if (line[i + 1] != '#' && line[i - 1] != '#' && line[i - 1] != '0'
			  && line[i - 1] != '1' && line[i - 1] != '2'
			  && line[i - 1] != '3' && line[i - 1] != '4'
			  && line[i - 1] != '5' && line[i + 1] != '$'
			  && line[i - 1] != '$')
			{
			  line.replace (i, 1, "_");
			}
		    }
		}

	      newFile << line << endl;
	    }
	  // remove old file
	  remove ((DATASETS + string ("/") + dataset).c_str ());
	  // rename the new file
	  rename ((DATASETS + string ("/") + dataset + string ("1")).c_str (),
		  (DATASETS + string ("/") + dataset).c_str ());

	  cout << "dataset : " << dataset << "   is finished" << endl;
	}
      else
	{
	  cout << "error in opening model file " << dataset << endl;
	}
    }
}

string
CLExec::toLowerCase (string word)
{
  string highLetters = "АӘБВГҒДЕЁЖЗИЙКҚЛМНҢОӨПРСТУҰҮФХҺЦЧШЩЪЫІЬЭЮЯ";
  string lowLetters = "аәбвгғдеёжзийкқлмнңоөпрстуұүфхһцчшщъыіьэюя";

  for (unsigned i = 0; i < highLetters.size () - 1; i += 2)
    {
      // letter is 2 chars not one
      string letter = highLetters.substr (i, 2);

      string::size_type pos = word.find (letter);
      if (pos != string::npos)
	{
	  word[pos] = lowLetters[i];
	  word[pos + 1] = lowLetters[i + 1];
	  i++;
	}
    }

  return word;
}

// to sort translations from best to worth by their weight
bool
sortParameter (pair<vector<unsigned>, float> a, pair<vector<unsigned>, float> b)
{
  return (a.second > b.second);
}

void
CLExec::beamSearch (
    vector<pair<vector<unsigned>, float> > *beamTree,
    unsigned beam,
    vector<string> slTokens,
    vector<pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<xml_node> > > > > ambigInfo,
    map<string, map<string, vector<float> > > classesWeights)
{
  // initilaization
  (*beamTree).push_back (pair<vector<unsigned>, float> ());

  for (unsigned i = 0; i < ambigInfo.size (); i++)
    {
      pair<pair<unsigned, unsigned>, pair<unsigned, vector<vector<xml_node> > > > p =
	  ambigInfo[i];
      pair<unsigned, unsigned> wordInd = p.first;
      vector<vector<xml_node> > ambigRules = p.second.second;
      unsigned ambigRulesSize = ambigRules.size ();

      // name of the file is the concatenation of rules ids
      string rulesNums;
      for (unsigned x = 0; x < ambigRules.size (); x++)
	{
	  for (unsigned y = 0; y < ambigRules[x].size (); y++)
	    {
	      xml_node rule = ambigRules[x][y];
	      string ruleCmnt = rule.first_attribute ().value ();

	      rulesNums += ruleCmnt.substr (ruleCmnt.find_last_of ("-") + 1);
	      rulesNums += "_";

	    }
	  rulesNums += "+";
	}

      map<string, vector<float> > classWeights = classesWeights[(rulesNums + ".model")];

      // build new tree for the new words
      vector<pair<vector<unsigned>, float> > newTree;

      // initialize the new tree
      for (unsigned x = 0; x < ambigRulesSize; x++)
	{
	  newTree.push_back (pair<vector<unsigned>, float> (vector<unsigned> (), 0));
	}
      // put rules
      for (unsigned z = 0; z < ambigRulesSize; z++)
	{
	  for (unsigned y = 0; y < ambigRules[z].size (); y++)
	    {
	      string ruleCmnt = ambigRules[z][y].first_attribute ().value ();
	      newTree[z].first.push_back (
		  atoi (ruleCmnt.substr (ruleCmnt.find_last_of ("-") + 1).c_str ()));

	    }
	}

      for (unsigned x = wordInd.first; x < wordInd.first + wordInd.second; x++)
	{
	  // word key is the word and it's order in the rule
	  string num;
	  switch (x - wordInd.first)
	    {
	    case 0:
	      num = "_0";
	      break;
	    case 1:
	      num = "_1";
	      break;
	    default:
	      num = "_2";
	    }

	  // handle the case of two lemmas separated by a space
	  for (unsigned t = 0; t < slTokens[x].size (); t++)
	    if (slTokens[x][t] == ' ')
	      slTokens[x].replace (t, 1, "_");

	  string word = toLowerCase (slTokens[x]) + num;
	  vector<float> wordWeights = classWeights[word];

	  // put weights
	  if (wordWeights.empty ())
	    {
	      for (unsigned z = 0; z < ambigRulesSize; z++)
		newTree[z].second += 1;
	      cout << "word : " << word << "  is not found in dataset : " << rulesNums
		  << endl;
	    }

	  else
	    for (unsigned z = 0; z < ambigRulesSize; z++)
	      newTree[z].second += wordWeights[z];

	}

      // expand beamTree
      unsigned initSize = beamTree->size ();
      for (unsigned z = 0; z < ambigRulesSize - 1; z++)
	{
	  for (unsigned x = 0; x < initSize; x++)
	    {
	      beamTree->push_back (pair<vector<unsigned>, float> ((*beamTree)[x]));
	    }
	}

      // merge the two trees
      for (unsigned z = 0; z < ambigRulesSize; z++)
	{
	  for (unsigned x = initSize * z; x < initSize * (z + 1); x++)
	    {
	      // put the new rules with the old
	      (*beamTree)[x].first.insert ((*beamTree)[x].first.end (),
					   newTree[z].first.begin (),
					   newTree[z].first.end ());

	      // add their wiehgts
	      (*beamTree)[x].second += newTree[z].second;
	    }
	}

      // sort beam tree
      sort (beamTree->begin (), beamTree->end (), sortParameter);

      // remove elements more than (beam)
      if (beamTree->size () > beam)
	beamTree->erase (beamTree->begin () + beam, beamTree->end ());
    }
}

void
CLExec::getTransInds (vector<pair<unsigned, float> > *transInds,
		      vector<pair<vector<unsigned>, float> > beamTree,
		      vector<vector<unsigned> > rulesIds)
{
  for (unsigned i = 0; i < beamTree.size (); i++)
    {
      vector<unsigned> transInd = beamTree[i].first;
      for (unsigned j = 0; j < rulesIds.size (); j++)
	{
	  vector<unsigned> weigInd = rulesIds[j];

	  unsigned count = 0;
	  for (unsigned x = 0; x < weigInd.size () && count < transInd.size (); x++)
	    {
	      if (transInd[count] == weigInd[x])
		count++;
	    }

	  if (count == transInd.size ())
	    {
	      transInds->push_back (pair<unsigned, float> (j, beamTree[i].second));
	      break;
	    }
	}
    }
}
