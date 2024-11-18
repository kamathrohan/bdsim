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
#include "BDSBeamlineIntegral.hh"
#include "BDSCavityFieldType.hh"
#include "BDSComponentFactory.hh"
#include "BDSDebug.hh"
#include "BDSException.hh"
#include "BDSParticleDefinition.hh"
#include "BDSUtilities.hh"

#include "G4Types.hh"

#include "parser/element.h"
#include "parser/elementtype.h"

#include "CLHEP/Units/PhysicalConstants.h"
#include "CLHEP/Units/SystemOfUnits.h"

#include <cmath>


BDSBeamlineIntegral::BDSBeamlineIntegral(const BDSParticleDefinition& incomingParticle,
                                         G4double T0In,
                                         G4double integratedArcLength):
  synchronousTAtEnd(T0In),
  synchronousTAtMiddleOfLastElement(T0In),
  arcLength(integratedArcLength),
  designParticle(incomingParticle)
{;}

BDSBeamlineIntegral::~BDSBeamlineIntegral()
{;}

void BDSBeamlineIntegral::Integrate(const GMAD::Element& componentAsDefined)
{
  // length
  G4double thisComponentArcLength = componentAsDefined.l*CLHEP::m;
  arcLength += thisComponentArcLength;
  
  G4double v0 = designParticle.Velocity(); // velocity at entrance of element

  // calculate change in velocity and kinetic energy
  G4double dEk = 0;
  
  G4double particleCharge = designParticle.Charge();
  G4double phase = componentAsDefined.phase * CLHEP::rad;
  G4double cosPhase = std::cos(phase);
  G4double frequency = componentAsDefined.frequency * CLHEP::hertz;
  
  switch (componentAsDefined.type)
    {
    case GMAD::ElementType::_RF:
      {
        // field model
        BDSCavityFieldType tp = BDS::DetermineCavityFieldType(componentAsDefined.cavityFieldType);
        G4double eField = BDSComponentFactory::EFieldFromElement(&componentAsDefined, thisComponentArcLength);
        switch (tp.underlying())
          {
          case BDSCavityFieldType::constantinz:
            {
              dEk = particleCharge * eField * thisComponentArcLength * cosPhase;
              break;
            }
          case BDSCavityFieldType::pillbox:
            {
              if (!BDS::IsFinite(frequency)) // protect against zero division
                {throw BDSException(__METHOD_NAME__, "for a pillbox cavity field, the frequency must be non-zero");}
              G4double rfWavelength = CLHEP::c_light / frequency;
              G4double piGOverBetaLambda = CLHEP::pi * thisComponentArcLength / designParticle.Beta() * rfWavelength;
              G4double transitTimeFactor = std::sin(piGOverBetaLambda) / piGOverBetaLambda;
              dEk = particleCharge * eField * thisComponentArcLength * transitTimeFactor * cosPhase;
              break;
            }
          default:
            {break;}
          }
        break;
      }
    default:
      {break;} // no action
    }
  
  // momentum and therefore BRho
  designParticle.ApplyChangeInKineticEnergy(dEk);

  G4double v1 = designParticle.Velocity(); // velocity at the end
  G4double vMean = (v1 + v0) / 2.0;
  G4double dT = thisComponentArcLength / vMean;

  G4double dTMiddle = thisComponentArcLength / ( (0.5*(v1-v0) + v0 ) / 2.0);
  synchronousTAtMiddleOfLastElement = synchronousTAtEnd + dTMiddle;
  
  // time - now at the start of the next component / end of this component
  synchronousTAtEnd += dT;
}
