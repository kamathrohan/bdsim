/* 
Beam Delivery Simulation (BDSIM) Copyright (C) Royal Holloway, 
University of London 2001 - 2022.

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
#ifndef BDSFIELDEMMUONCOOLER_H
#define BDSFIELDEMMUONCOOLER_H

#include "BDSFieldEM.hh"

#include "G4ThreeVector.hh"
#include "G4Types.hh"

#include <utility>

class BDSFieldEM;
class BDSFieldInfoExtraMuonCooler;
class BDSFieldMag;

/**
 * @brief A composite RF and B field for a muon cooler.
 *
 * @author Laurie Nevay
 */

class BDSFieldEMMuonCooler: public BDSFieldEM
{
public:
  BDSFieldEMMuonCooler() = delete;
  explicit BDSFieldEMMuonCooler(const BDSFieldInfoExtraMuonCooler* info);  
  virtual ~BDSFieldEMMuonCooler();

  /// Accessor to get B and E field.
  virtual std::pair<G4ThreeVector, G4ThreeVector> GetField(const G4ThreeVector& position,
							   const G4double       t) const;
  
private:
  BDSFieldMag* coilField;
  BDSFieldEM*  rfField;
};

#endif
