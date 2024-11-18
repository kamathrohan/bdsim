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
#ifndef BDSFIELDEMRFCAVITY_H
#define BDSFIELDEMRFCAVITY_H

#include "BDSFieldEM.hh"

#include "globals.hh"
#include "G4ThreeVector.hh"

#include <utility>

class BDSCavityInfo;
class BDSMagnetStrength;

/**
 * @brief Pill box cavity electromagnetic field.
 *
 * @author Stuart Walker
 */

class BDSFieldEMRFCavity: public BDSFieldEM
{
public:
  BDSFieldEMRFCavity(BDSMagnetStrength const* strength,
		     G4double                 brho);
  
  BDSFieldEMRFCavity(G4double eFieldAmplitude,
		     G4double frequency,
		     G4double phaseOffset,
		     G4double cavityRadius);
  
  virtual ~BDSFieldEMRFCavity(){;}

  /// Accessor to get B and E field.
  virtual std::pair<G4ThreeVector, G4ThreeVector> GetField(const G4ThreeVector& position,
							   const G4double       t) const;
  
  virtual G4bool TimeVarying() const {return true;}
  
private:
  /// Private constructor to force use of provided one.
  BDSFieldEMRFCavity();  
  
private:
  G4double eFieldMax;    ///< Maximum field in V/m.
  G4double phase;        ///< Phase offset of the oscillator.
  G4double cavityRadius; ///< Radius at maximum extent of cavity.
  G4double wavelength;

  /// X coordinate of first 0 point for bessel J0.
  static const G4double j0FirstZero;

  /// Impedance of free space.
  static const G4double Z0;
  
  /// Pre-calculated normalised calculated radius w.r.t. bessel first 0.
  const G4double normalisedCavityRadius;

  /// Angular frequency calculated from frequency - cached to avoid repeated calculation.
  const G4double angularFrequency;
};

#endif
