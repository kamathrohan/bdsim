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
#ifndef BDSBEAMLINEINTEGRAL_H
#define BDSBEAMLINEINTEGRAL_H

#include "BDSParticleDefinition.hh"

#include "G4Types.hh"

namespace GMAD
{
  struct Element;
}

/**
 * @brief A class that holds the current integrated quantities along a beam line.
 *
 * Used to hold accumulated quantities like the current total arc length, synchronous
 * global time and current particle rigidity / momentum along the beam line.
 * 
 * @author Laurie Nevay
 */

class BDSBeamlineIntegral
{
public:
  BDSBeamlineIntegral() = delete;
  explicit BDSBeamlineIntegral(const BDSParticleDefinition& incomingParticle,
			       G4double T0In = 0,
			       G4double integratedArcLength = 0);

  ~BDSBeamlineIntegral();

  /// Increment all quantities in this class by adding this element
  /// onto the beam line.
  void Integrate(const GMAD::Element& componentAsDefined);
  
  G4double synchronousTAtEnd;
  G4double synchronousTAtMiddleOfLastElement;
  G4double arcLength;
  BDSParticleDefinition designParticle;
};

#endif
