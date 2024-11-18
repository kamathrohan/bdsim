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
#include "BDSFieldESinusoid.hh"
#include "BDSMagnetStrength.hh"
#include "BDSUtilities.hh"

#include "G4ThreeVector.hh"
#include "G4Types.hh"

#include "CLHEP/Units/SystemOfUnits.h"

#include <cmath>

BDSFieldESinusoid::BDSFieldESinusoid(BDSMagnetStrength const* strength,
				     G4double                 brho):
  BDSFieldESinusoid((*strength)["efield"],
		    G4ThreeVector( (*strength)["ex"], (*strength)["ey"], (*strength)["ez"]),
		    (*strength)["frequency"],
		    (*strength)["phase"])
{
  G4int sign = BDS::Sign(brho);
  eField *= sign;
}

BDSFieldESinusoid::BDSFieldESinusoid(G4double eFieldAmplitudeIn,
                                     const G4ThreeVector& unitDirectionIn,
				     G4double frequencyIn,
				     G4double phaseOffsetIn):
  eField(eFieldAmplitudeIn),
  unitDirection(unitDirectionIn.unit()),
  angularFrequency(CLHEP::twopi*frequencyIn),
  phase(phaseOffsetIn)
{
  finiteStrength = BDS::IsFinite(eField);
}

G4ThreeVector BDSFieldESinusoid::GetField(const G4ThreeVector& /*position*/,
					  const G4double       t) const
{
  G4double ef = eField * std::cos(angularFrequency*t + phase);
  G4ThreeVector field = unitDirection * ef;
  return field;
}
