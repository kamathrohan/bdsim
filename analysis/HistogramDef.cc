/* 
Beam Delivery Simulation (BDSIM) Copyright (C) Royal Holloway, 
University of London 2001 - 2024.

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
#include "HistogramDef.hh"

#include <iostream>
#include <string>

ClassImp(HistogramDef)

HistogramDef::HistogramDef():
  treeName(""),
  histName(""),
  nDimensions(0),
  variable(""),
  selection(""),
  perEntry(false)
{;}

HistogramDef::HistogramDef(const std::string& treeNameIn,
                           const std::string& histNameIn,
                           int                nDimensionsIn,
                           const std::string& variableIn,
                           const std::string& selectionIn,
                           bool               perEntryIn):
  treeName(treeNameIn),
  histName(histNameIn),
  nDimensions(nDimensionsIn),
  variable(variableIn),
  selection(selectionIn),
  perEntry(perEntryIn)
{;}

std::ostream& operator<< (std::ostream& out, const HistogramDef& s)
{
  std::string hs = s.GetHistogramString();
  std::string bs = s.GetBinningString();
  out << hs << " " << s.treeName << " " << s.histName << " " << bs << " " << s.variable << " " << s.selection << "\n";
  return out;
}