/* 
Beam Delivery Simulation (BDSIM) Copyright (C) Royal Holloway, 
University of London 2001 - 2020.

This file is part of BDSIM.

BDSIM is free software: you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published 
by the Free Software Foundation version 3 of the License.

BDSIM is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with BDSIM.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "BinGeneration.hh"
#include "BinLoader.hh"
#include "BinSpecification.hh"
#include "Config.hh"
#include "HistogramDef1D.hh"
#include "HistogramDef2D.hh"
#include "HistogramDef3D.hh"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

ClassImp(Config)

Config* Config::instance = nullptr;

std::vector<std::string> Config::treeNames = {"Beam.", "Options.", "Model.", "Run.", "Event."};

Config::Config(const std::string& inputFilePathIn,
	       const std::string& outputFileNameIn):
  allBranchesActivated(false)
{
  InitialiseOptions("");

  optionsString["inputfilepath"]  = inputFilePathIn;
  optionsString["outputfilename"] = outputFileNameIn;

  // turn on merging only
  branches["Event."].push_back("Histos");
  branches["Run."].push_back("Histos");
  optionsBool["perentryevent"] = true;
}

Config::Config(const std::string& fileNameIn,
	       const std::string& inputFilePathIn,
	       const std::string& outputFileNameIn):
  allBranchesActivated(false)
{
  InitialiseOptions(fileNameIn);  
  ParseInputFile();

  if (!inputFilePathIn.empty())
    {optionsString["inputfilepath"] = inputFilePathIn;}
  if (!outputFileNameIn.empty())
    {optionsString["outputfilename"] = outputFileNameIn;}
}

Config::~Config()
{
  instance = nullptr;

  for (auto& nameDefs : histoDefs)
    {
      for (auto& histoDef : nameDefs.second)
	{delete histoDef;}
    }
}

void Config::InitialiseOptions(std::string analysisFile)
{
  optionsString["analysisfile"] = analysisFile;
  
  // for backwards compatability / verbose names
  alternateKeys["calculateopticalfunctions"]         = "calculateoptics";
  alternateKeys["calculateopticalfunctionsfilename"] = "opticsfilename";
  
  optionsBool["debug"]             = false;
  optionsBool["processsamplers"]   = false;
  optionsBool["calculateoptics"]   = false;
  optionsBool["mergehistograms"]   = true;
  optionsBool["emittanceonthefly"] = false;
  optionsBool["perentrybeam"]      = false;
  optionsBool["perentryevent"]     = false;
  optionsBool["perentryrun"]       = false;
  optionsBool["perentryoption"]    = false;
  optionsBool["perentrymodel"]     = false;
  optionsBool["backwardscompatible"] = false; // ignore file types for old data

  optionsString["inputfilepath"]  = "./output.root";
  optionsString["outputfilename"] = "./output_ana.root";
  optionsString["opticsfilename"] = "./output_optics.dat";
  optionsString["gdmlfilename"]   = "./model.gdml";

  optionsNumber["printmodulofraction"] = 0.01;
  optionsNumber["eventstart"]          = 0;
  optionsNumber["eventend"]            = -1;

  // ensure keys exist for all trees.
  for (auto name : treeNames)
    {
      histoDefs[name]         = std::vector<HistogramDef*>();
      histoDefsSimple[name]   = std::vector<HistogramDef*>();
      histoDefsPerEntry[name] = std::vector<HistogramDef*>();
      branches[name]          = std::vector<std::string>();
    }
}

Config* Config::Instance(const std::string& fileName,
			 const std::string& inputFilePath,
			 const std::string& outputFileName)
{
  if(!instance && !fileName.empty())
    {instance = new Config(fileName, inputFilePath, outputFileName);}
  else if(instance && !fileName.empty())
    {
      std::cout << "Config::Instance> Instance present, delete and construct" << std::endl;
      delete instance;
      instance = new Config(fileName, inputFilePath, outputFileName);
    }
  else if (!instance && !fileName.empty())
    {instance = new Config(inputFilePath, outputFileName);}
  // else return current instance (can be nullptr!)
  return instance;
}

void Config::ParseInputFile()
{ 
  std::ifstream f(optionsString.at("analysisfile").c_str());

  if(!f)
    {throw std::string("Config::ParseInputFile> could not open file");}

  lineCounter = 0;
  std::string line;

  // unique patterns to match
  // match a line starting with #
  std::regex comment("^\\#.*");
  // match a line starting with 'histogram', ignoring case
  std::regex histogram("(?:simple)*histogram.*", std::regex_constants::icase);

  while(std::getline(f, line))
    {
      lineCounter++;
      if (std::all_of(line.begin(), line.end(), isspace))
	{continue;} // skip empty lines
      else if (std::regex_search(line, comment))
	{continue;} // skip lines starting with '#'
      else if (std::regex_search(line, histogram))
	{ParseHistogramLine(line);} // any histogram - must be before settings
      else
	{ParseSetting(line);} // any setting
    }
  
  f.close();

  // set flags etc based on what options have been set
  if (optionsBool.at("calculateoptics"))
    {
      allBranchesActivated = true;
      optionsBool["processsamplers"] = true;
      optionsBool["perentryevent"]   = true;
    }
  if (optionsBool.at("mergehistograms"))
    {
      branches["Event."].push_back("Histos");
      branches["Run."].push_back("Histos");
      optionsBool["perentryevent"]   = true;
    }

  // checks on event numbers
  double eS = optionsNumber.at("eventstart");
  double eE = optionsNumber.at("eventend");
  if (eE < 0) // default -1
    {eE = std::numeric_limits<double>::max();}
  if (eS < 0 || eS > eE)
    {
      std::cerr << "Invalid starting event number " << eS << std::endl;
      exit(1);
    }
}

void Config::ParseHistogramLine(const std::string& line)
{
  // Settings with histogram in name can be misidentified - check here.
  // This is the easiest way to do it for now.
  std::string copyLine = line;
  std::transform(copyLine.begin(), copyLine.end(), copyLine.begin(), ::tolower); // convert to lower case
  if (copyLine.find("mergehistograms") != std::string::npos)
    {
      ParseSetting(line);
      return;
    }
  
  // we know the line starts with 'histogram'
  // extract number after it as 1st match and rest of line as 2nd match
  std::regex histNDim("^\\s*(?:Simple)*Histogram([1-3])D[a-zA-Z]*\\s+(.*)", std::regex_constants::icase);
  //std::regex histNDim("^Histogram([1-3])D[a-zA-Z]*\\s+(.*)", std::regex_constants::icase);
  std::smatch match;

  try
    {
      if (std::regex_search(line, match, histNDim))
	{
	  int nDim = std::stoi(match[1]);
	  ParseHistogram(line, nDim);
	}
      else
	{throw std::invalid_argument("");}
    }
  catch (std::invalid_argument& e)
    {
      std::string errString = "\nInvalid histogram type on line #" + std::to_string(lineCounter)
	+ ": \n\"" + line + "\"\n";
      throw std::invalid_argument(e.what() + errString);
    }
}

void Config::ParseHistogram(const std::string& line, const int nDim)
{  
  // split line on white space
  // doesn't inspect words themselves
  // checks number of words, ie number of columns is correct
  std::vector<std::string> results;
  std::regex wspace("\\s+"); // any whitepsace
  // -1 here makes it point to the suffix, ie the word rather than the wspace
  std::sregex_token_iterator iter(line.begin(), line.end(), wspace, -1);
  std::sregex_token_iterator end;
  for (; iter != end; ++iter)
    {
      std::string res = (*iter).str();
      results.push_back(res);
    }
  
  if (results.size() < 7)
    {// ensure enough columns
      std::string errString = "Invalid line #" + std::to_string(lineCounter)
	+ " - invalid number of columns (too few)";
      throw std::string(errString);
    }
  if (results.size() > 7)
    {// ensure not too many columns
      std::string errString = "Invalid line #" + std::to_string(lineCounter)
      + " - too many columns - check no extra whitespace";
      throw std::string(errString);
    }

  bool xLog = false;
  bool yLog = false;
  bool zLog = false;
  ParseLog(results[0], xLog, yLog, zLog);

  bool perEntry = true;
  ParsePerEntry(results[0], perEntry);
  
  std::string treeName  = results[1];
  CheckValidTreeName(treeName);

  if (perEntry)
    {
      std::string treeNameWithoutPoint = treeName; // make copy to modify
      treeNameWithoutPoint.pop_back();             // delete last character
      treeNameWithoutPoint = LowerCase(treeNameWithoutPoint);
      optionsBool["perentry"+treeNameWithoutPoint] = true;
    }
  
  std::string histName  = results[2];
  std::string bins      = results[3];
  std::string binning   = results[4];
  std::string variable  = results[5];
  std::string selection = results[6];

  BinSpecification xBinning;
  BinSpecification yBinning;
  BinSpecification zBinning;
  ParseBins(bins, nDim,xBinning, yBinning, zBinning);
  ParseBinning(binning, nDim, xBinning, yBinning, zBinning, xLog, yLog, zLog);

  // make a check that the number of variables supplied in ttree with y:x syntax doesn't
  // exceed the number of declared dimensions - this would result in a segfault from ROOT
  // a single (not 2) colon with at least one character on either side
  std::regex singleColon("\\w+(:{1})\\w+");
  // count the number of matches by distance of match iterator from beginning
  int nColons = static_cast<int>(std::distance(std::sregex_iterator(variable.begin(),
								    variable.end(),
								    singleColon),
					       std::sregex_iterator()));
  if (nColons > nDim - 1)
    {
      std::cerr << "Error: Histogram \"" << histName << "\" variable includes too many single\n"
		<< "colons specifying more dimensions than the number of specified dimensions."
		<< std::endl;
      std::cerr << "Declared dimensions: " << nDim << std::endl;
      std::cerr << "Number of dimensions in variables " << nColons + 1 << std::endl;
      exit(1);
    }

  HistogramDef1D* result = nullptr;
  switch (nDim)
    {
    case 1:
      {
	result = new HistogramDef1D(treeName, histName,
				    xBinning,
				    variable, selection, perEntry);
	break;
      }
    case 2:
      {
	result = new HistogramDef2D(treeName, histName,
				    xBinning, yBinning,
				    variable, selection, perEntry);
	break;
      }
    case 3:
      {
	result = new HistogramDef3D(treeName, histName,
				    xBinning, yBinning, zBinning,
				    variable, selection, perEntry);
	break;
      }
    default:
      {break;}
    }

  if (result)
    {
      histoDefs[treeName].push_back(result);
      if (perEntry)
	{histoDefsPerEntry[treeName].push_back(result);}
      else
	{histoDefsSimple[treeName].push_back(result);}
      UpdateRequiredBranches(result);
    }
}

void Config::ParsePerEntry(const std::string& name, bool& perEntry) const
{
  std::string res = name;
  std::transform(res.begin(), res.end(), res.begin(), ::tolower); // convert to lower case
  perEntry = res.find("simple") == std::string::npos;
}

void Config::ParseLog(const std::string& definition,
		      bool& xLog,
		      bool& yLog,
		      bool& zLog) const
{
  // capture any lin log definitions after Histogram[1-3]D
  std::regex linLogWords("(Lin)|(Log)", std::regex_constants::icase);
  std::sregex_token_iterator iter(definition.begin(), definition.end(), linLogWords);
  std::sregex_token_iterator end;
  std::vector<bool*> results = {&xLog, &yLog, &zLog};
  int index = 0;
  for (; iter != end; ++iter, ++index)
    {
      std::string res = (*iter).str();
      std::transform(res.begin(), res.end(), res.begin(), ::tolower); // convert to lower case
      *(results[index]) = res == "log";
    }
}

void Config::UpdateRequiredBranches(const HistogramDef* def)
{
  UpdateRequiredBranches(def->treeName, def->variable);
  UpdateRequiredBranches(def->treeName, def->selection);
}

void Config::UpdateRequiredBranches(const std::string& treeName,
				    const std::string& var)
{
  // This won't work properly for the options Tree that has "::" in the class
  // as well as double splitting. C++ regex does not support lookahead / behind
  // which makes it nigh on impossible to correctly identify the single : with
  // regex. For now, only the Options tree has this and we turn it all on, so it
  // it shouldn't be a problem (it only ever has one entry).
  // match word; '.'; word -> here we match the token rather than the bits inbetween
  std::regex branchLeaf("(\\w+)\\.(\\w+)");
  auto words_begin = std::sregex_iterator(var.begin(), var.end(), branchLeaf);
  auto words_end   = std::sregex_iterator();
  for (std::sregex_iterator i = words_begin; i != words_end; ++i)
    {
      std::string targetBranch = (*i)[1];
      SetBranchToBeActivated(treeName, targetBranch);
    }
}

void Config::SetBranchToBeActivated(const std::string& treeName,
				    const std::string& branchName)
{
  auto& v = branches.at(treeName);
  if (std::find(v.begin(), v.end(), branchName) == v.end())
    {v.push_back(branchName);}
}

void Config::CheckValidTreeName(std::string& treeName) const
{
  // check it has a point at the end (simple mistake)
  if (strcmp(&treeName.back(), ".") != 0)
    {treeName += ".";}
  
  if (InvalidTreeName(treeName))
    {
      std::string err = "Invalid tree name \"" + treeName + "\"\n";
      err += "Tree names are one of: ";
      for (const auto& n : treeNames)
	{err += "\"" + n + "\" ";}
      throw(err);
    }
}

bool Config::InvalidTreeName(const std::string& treeName) const
{
  return std::find(treeNames.begin(), treeNames.end(), treeName) == treeNames.end();
}

void Config::ParseBins(const std::string& bins,
		       int nDim,
		       BinSpecification& xBinning,
		       BinSpecification& yBinning,
		       BinSpecification& zBinning) const
{
  // match a number including +ve exponentials (should be +ve int)
  std::regex number("([0-9e\\+]+)+", std::regex_constants::icase);
  int* binValues[3] = {&xBinning.n, &yBinning.n, &zBinning.n};
  auto words_begin = std::sregex_iterator(bins.begin(), bins.end(), number);
  auto words_end   = std::sregex_iterator();
  int counter = 0;
  for (std::sregex_iterator i = words_begin; i != words_end; ++i, ++counter)
    {(*binValues[counter]) = std::stoi((*i).str());}
  if (counter < nDim-1)
    {throw std::string("Invalid bin specification on line #" + std::to_string(lineCounter));}
}

void Config::ParseBinning(const std::string& binning,
			  int nDim,
			  BinSpecification& xBinning,
			  BinSpecification& yBinning,
			  BinSpecification& zBinning,
			  bool xLog,
			  bool yLog,
			  bool zLog) const
{
  // erase braces
  std::string binningL = binning;
  binningL.erase(std::remove(binningL.begin(), binningL.end(), '{'), binningL.end());
  binningL.erase(std::remove(binningL.begin(), binningL.end(), '}'), binningL.end());
  // simple match - let stod throw exception if wrong
  std::regex oneDim("([0-9eE.+-]+):([0-9eE.+-]+)");
  
  std::regex commas(",");
  auto wordsBegin = std::sregex_token_iterator(binningL.begin(), binningL.end(), commas, -1);
  auto wordsEnd   = std::sregex_token_iterator();
  int counter = 0;
  std::vector<BinSpecification*> values = {&xBinning, &yBinning, &zBinning};
  std::vector<bool> isLog = {xLog, yLog, zLog};
  for (auto i = wordsBegin; i != wordsEnd; ++i, ++counter)
    {
      std::string matchS = *i;
      if (matchS.find(".txt") != std::string::npos)
	{// file name for variable binning
	  std::vector<double>* binEdges = RBDS::LoadBins(matchS);
	  (*values[counter]).edges = binEdges;
	  (*values[counter]).n     = (int)binEdges->size() - 1;
	  (*values[counter]).low   = binEdges->at(0);
	  (*values[counter]).high  = binEdges->back();
	}
      else
	{// try to match ranges
	  auto rangeBegin = std::sregex_iterator(matchS.begin(), matchS.end(), oneDim);
	  auto rangeEnd   = std::sregex_iterator();
	  int counterRange = 0;
	  for (auto j = rangeBegin; j != rangeEnd; ++j, ++counterRange)
	    {// iterate over all matches and pull out first and second number
	      std::smatch matchR = *j;
	      try
		{
		  (*values[counter]).low  = std::stod(matchR[1]);
		  (*values[counter]).high = std::stod(matchR[2]);
		}
	      catch (std::invalid_argument&) // if stod can't convert number to double
		{throw std::string("Invalid binning number: \"" + matchS + "\" on line #" + std::to_string(lineCounter));}
	      if ((*values[counter]).high <= (*values[counter]).low)
		{throw std::invalid_argument("high bin edge is <= low bin edge \"" + binning + "\"");}
	      if (isLog[counter])
		{
		  std::vector<double> binEdges = RBDS::LogSpace((*values[counter]).low, (*values[counter]).high, (*values[counter]).n);
		  (*values[counter]).edges = new std::vector<double>(binEdges);
		}
	    }
	}    
    }
  
  if (counter == 0)
    {throw std::string("Invalid binning specification: \"" + binning + "\" on line #" + std::to_string(lineCounter));}
  else if (counter < nDim)
    {
      std::string errString = "Insufficient number of binning dimensions on line #"
	+ std::to_string(lineCounter) + "\n"
	+ std::to_string(nDim) + " dimension histogram, but the following was specified:\n"
	+ binning + "\nDimension defined by \"low:high\" and comma separated";
      throw std::string(errString);
    }
}

std::string Config::LowerCase(const std::string& st) const
{
  std::string res = st;
  std::transform(res.begin(), res.end(), res.begin(), ::tolower);
  return res;
}
  
void Config::ParseSetting(const std::string& line)
{
  // match a word; at least one space; and a word including '.' and _ and / and -
  std::regex setting("(\\w+)\\s+([\\.\\/_\\-\\w\\*]+)", std::regex_constants::icase);
  std::smatch match;
  if (std::regex_search(line, match, setting))
    {
      std::string key   = LowerCase(match[1]);
      std::string value = match[2];

      if (alternateKeys.find(key) != alternateKeys.end())
	{key = alternateKeys[key];} // reassign value to correct key
      
      if (optionsBool.find(key) != optionsBool.end())
	{
	  // match optional space; true or false; optional space - ignore case
	  std::regex tfRegex("\\s*(true|false)\\s*", std::regex_constants::icase);
	  std::smatch tfMatch;
	  if (std::regex_search(value, tfMatch, tfRegex))
	    {
	      std::string flag = tfMatch[1];
	      std::smatch tMatch;
	      std::regex tRegex("true", std::regex_constants::icase);
	      bool result = std::regex_search(flag, tMatch, tRegex);
	      optionsBool[key] = result;
	    }
	  else
	    {optionsBool[key] = (bool)std::stoi(value);}
	}
      else if (optionsString.find(key) != optionsString.end())
	{optionsString[key] = value;}
      else if (optionsNumber.find(key) != optionsNumber.end())
	{optionsNumber[key] = std::stod(value);}
      else
      {throw std::string("Invalid option \"" + key + "\"");}
    }
  else
    {throw std::string("Invalid line #" + std::to_string(lineCounter) + "\n" + line);}
}
