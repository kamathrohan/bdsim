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
#include "BDSComponentConstructor.hh"
#include "BDSComponentFactoryUser.hh"
#include "BDSParticleDefinition.hh"

#include "globals.hh"

#include <map>
#include <string>

BDSComponentFactoryUser::BDSComponentFactoryUser():
  designParticle(nullptr),
  brho(0),
  beta0(0)
{;}

BDSComponentFactoryUser::~BDSComponentFactoryUser()
{
  for (auto& constructor : userFunctions)
    {delete constructor.second;}
}

void BDSComponentFactoryUser::SetDesignParticle(const BDSParticleDefinition* designParticleIn)
{
  designParticle = designParticleIn;
  brho  = designParticle->BRho();
  beta0 = designParticle->Beta();
}

void BDSComponentFactoryUser::RegisterComponent(const G4String& componentTypeName,
						BDSComponentConstructor* componentConstructor)
{
  userFunctions[componentTypeName] = componentConstructor;
}

G4bool BDSComponentFactoryUser::CanConstructComponentByName(const G4String& componentTypeName) const
{
  return userFunctions.find(componentTypeName) != userFunctions.end();
}

BDSAcceleratorComponent* BDSComponentFactoryUser::ConstructComponent(const G4String& componentTypeName,
								     GMAD::Element const* elementIn,
								     GMAD::Element const* prevElementIn,
								     GMAD::Element const* nextElementIn,
								     G4double currentArcLengthIn)
{
  return userFunctions[componentTypeName]->Construct(elementIn,
						     prevElementIn,
						     nextElementIn,
						     currentArcLengthIn,
						     brho,
						     beta0);
}
