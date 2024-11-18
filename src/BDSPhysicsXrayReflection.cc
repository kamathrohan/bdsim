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
#include "G4Version.hh"
#if G4VERSION_NUMBER > 1119

#include "BDSDebug.hh"
#include "BDSGlobalConstants.hh"
#include "BDSPhysicsXrayReflection.hh"

#include "G4AutoDelete.hh"
#include "G4Gamma.hh"
#include "G4ParticleDefinition.hh"
#include "G4PhysicsListHelper.hh"
#include "G4XrayReflection.hh"


BDSPhysicsXrayReflection::BDSPhysicsXrayReflection(G4double surfaceRoughnessIn):
  G4VPhysicsConstructor("BDSPhysicsXrayReflection"),
  surfaceRoughness(surfaceRoughnessIn)
{
  G4cout << __METHOD_NAME__ << "surface roughness: " << surfaceRoughness / CLHEP::nm << " nm" << G4endl;
}

BDSPhysicsXrayReflection::~BDSPhysicsXrayReflection()
{;}

void BDSPhysicsXrayReflection::ConstructParticle()
{
  G4Gamma::Gamma();
}

void BDSPhysicsXrayReflection::ConstructProcess()
{
  if (Activated())
    {return;}

  G4XrayReflection* reflection = new G4XrayReflection();
  reflection->SetSurfaceRoughness(surfaceRoughness);
  G4AutoDelete::Register(reflection);

  G4PhysicsListHelper* ph = G4PhysicsListHelper::GetPhysicsListHelper();

  auto aParticleIterator = GetParticleIterator();
  aParticleIterator->reset();

  while( (*aParticleIterator)() )
    {
      G4ParticleDefinition *particle = aParticleIterator->value();
      G4String particleName = particle->GetParticleName();
      if (particleName == "gamma")
        {
          ph->RegisterProcess(reflection, particle);
          break;
        }
    }
  SetActivated();
}

#endif