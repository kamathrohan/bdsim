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
#include "BDSDebug.hh"
#include "BDSFieldMagDipoleEnge.hh"
#include "BDSMagnetStrength.hh"
#include "BDSMaths.hh"
#include "BDSUtilities.hh"

#include "globals.hh"
#include "G4ThreeVector.hh"
#include "G4Types.hh"

#include "CLHEP/Units/PhysicalConstants.h"
#include "CLHEP/Units/SystemOfUnits.h"

#include <algorithm>
#include <cmath>

//BDSFieldMagDipoleEnge::BDSFieldMagDipoleEnge(BDSMagnetStrength const* strength,
    //                                             G4double apertureIn, G4double engeIn, G4double toleranceIn):
    //      BDSFieldMagDipoleEnge((*strength)["field"], apertureIn, (*strength)["length"], engeIn, toleranceIn)
    //{;}


BDSFieldMagDipoleEnge::BDSFieldMagDipoleEnge(G4double strength,                                             
                                             G4double apertureRadius,
                                             G4double coilLength,
                                             G4double engeCoefficient,
                                             G4double toleranceIn
                                             ):
  D(2*apertureRadius),
  halfLength(0.5*coilLength),
  B0(strength),
  engeCoeff(engeCoefficient),
  spatialLimit(std::min(1e-5*apertureRadius, 1e-5*coilLength)),
  normalisationBY(1.0),
  normalisationBZ(1.0),
  coilTolerance(toleranceIn)
{;}

G4ThreeVector BDSFieldMagDipoleEnge::GetField(const G4ThreeVector& position,
					                          const G4double       /*t*/) const
{
  G4double z = position.z();
  G4double y = position.y();
  G4double rho = position.perp();
   
  // check if close to current source - function not well-behaved at exactly the rho of
  // the current source or at the boundary of +- halfLength
  //if (std::abs(rho - D/2.0) < spatialLimit && (std::abs(z) < halfLength+2*spatialLimit))
  //  {return G4ThreeVector();} // close to radius and inside +- z
  
  //singularities in occur in y at y = +/- l*pi*D. Protect against this.

  G4double zleft = z + halfLength + D;
  G4double zright = z - halfLength - D;
  
  //if (std::abs(OnAxisBz(zp, zm)) < coilTolerance)
  //{ return G4ThreeVector();}

  G4double By = 0;
  G4double Bz = 0;
  
  // check on-axis By and make tolerance cut
  if (rho > D*0.5)
    { return G4ThreeVector();}
  else
    {

      G4double By_left = (1 + std::exp(-zleft*engeCoeff/D) * std::cos(y*engeCoeff/D)) / (1 + 2*std::exp(-zleft*engeCoeff/D) * std::cos(y*engeCoeff/D) + std::exp(-2*zleft*engeCoeff/D));
      G4double By_right = (1 + std::exp(zright*engeCoeff/D) * std::cos(y*engeCoeff/D)) / (1 + 2*std::exp(zright*engeCoeff/D) * std::cos(y*engeCoeff/D) + std::exp(2*zright*engeCoeff/D));

      G4double Bz_left = ( - std::exp(-zleft*engeCoeff/D) * std::sin(y*engeCoeff/D)) / (1 + 2*std::exp(-zleft*engeCoeff/D) * std::cos(y*engeCoeff/D) + std::exp(-2*zleft*engeCoeff/D));
      G4double Bz_right = ( - std::exp(zright*engeCoeff/D) * std::sin(y*engeCoeff/D)) / (1 + 2*std::exp(zright*engeCoeff/D) * std::cos(y*engeCoeff/D) + std::exp(2*zright*engeCoeff/D));
      
      By = B0 * By_left * By_right;
      Bz = B0 * Bz_left * Bz_right;  

      //normalisationBY = By(0,0,0) / B0;
      //normalisationBY = Bz(0,0,0) / B0; //check Bz field!!

      // check for NaNs, not actually necessary TBC
      //if (std::isnan(By))
      //  {By = 0;}
      //if (std::isnan(Bz))
      //  {Bz = B0;}
    }
  // we have to be consistent with the phi we calculated at the beginning,
  // so unit rho is in the x direction.
  G4ThreeVector result = G4ThreeVector(0,By,Bz);
  return result;
}
