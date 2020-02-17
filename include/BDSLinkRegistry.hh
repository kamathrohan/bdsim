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
#ifndef BDSLINKREGISTRY_H
#define BDSLINKREGISTRY_H

#include <map>
#include <string>

#include "G4Transform3D.hh"
#include "G4Types.hh"

class BDSLinkOpaqueBox;

/**
 * @brief Registry / map of components for tracker linkage.
 *  
 * @author Laurie Nevay
 */

class BDSLinkRegistry
{
public:
  BDSLinkRegistry();
  ~BDSLinkRegistry();

  void Register(BDSLinkOpaqueBox*    componentIn,
		const G4Transform3D& globalToInputIn);

  G4Transform3D Transform(const std::string& name) const;
  G4Transform3D Transform(G4int ID) const;

private:

  struct LinkEntry
  {
    BDSLinkOpaqueBox* component;
    G4Transform3D     transform;
    G4int             id;
  };

  std::map<std::string, LinkEntry> byName;
  std::map<G4int,       LinkEntry> byID;
};

#endif
