/* 
Beam Delivery Simulation (BDSIM) Copyright (C) Royal Holloway, 
University of London 2001 - 2021.

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
#include "BDSAcceleratorComponent.hh"
#include "BDSColours.hh"
#include "BDSColourFromMaterial.hh"
#include "BDSDebug.hh"
#include "BDSException.hh"
#include "BDSExtent.hh"
#include "BDSGlobalConstants.hh"
#include "BDSMaterials.hh"
#include "BDSMuonCooler.hh"

#include "G4Colour.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4String.hh"
#include "G4ThreeVector.hh"
#include "G4Tubs.hh"
#include "G4Types.hh"
#include "G4VisAttributes.hh"

#include "CLHEP/Units/SystemOfUnits.h"

#include <map>
#include <set>
#include <vector>

class G4Material;

BDSMuonCooler::BDSMuonCooler(const G4String& nameIn,
			     G4double        lengthIn,
			     G4double        containerRadiusIn,
			     const std::vector<BDS::MuonCoolerCoilInfo>&   coilInfosIn,
			     const std::vector<BDS::MuonCoolerCavityInfo>& cavityInfosIn,
			     const BDS::MuonCoolerAbsorberInfo&            absorberInfoIn,
			     BDSFieldInfo*   outerFieldRecipeIn):
  BDSAcceleratorComponent(nameIn, lengthIn, 0, "muoncooler", nullptr),
  containerRadius(containerRadiusIn),
  coilInfos(coilInfosIn),
  cavityInfos(cavityInfosIn),
  absorberInfo(absorberInfoIn),
  outerFieldRecipe(outerFieldRecipeIn)
{;}

BDSMuonCooler::~BDSMuonCooler()
{;}

void BDSMuonCooler::Build()
{
  BuildContainerLogicalVolume();
  BuildCoils();
  BuildAbsorber();
  AttachOuterBField();
  BuildCavities();
}

void BDSMuonCooler::BuildContainerLogicalVolume()
{
  containerSolid = new G4Tubs(name + "_container_solid",
			      0,
			      containerRadius,
			      0.5*chordLength,
			      0,
			      CLHEP::twopi);

  G4Material* airMaterial = BDSMaterials::Instance()->GetMaterial("G4_AIR");
  containerLogicalVolume = new G4LogicalVolume(containerSolid,
					       airMaterial,
					       name + "_container_lv");

  BDSExtent ext(containerRadius, containerRadius, chordLength * 0.5);
  SetExtent(ext);
  
  BuildUserLimits();
  containerLogicalVolume->SetUserLimits(userLimits);

  auto containerVis = new G4VisAttributes(*BDSColours::Instance()->GetColour("opaquebox"));
  containerVis->SetVisibility(true);
  containerVis->SetForceLineSegmentsPerCircle(BDSGlobalConstants::Instance()->NSegmentsPerCircle());
  containerLogicalVolume->SetVisAttributes(containerVis);
  RegisterVisAttributes(containerVis);
}

void BDSMuonCooler::BuildCoils()
{
  // make a unique set of coil visualisations - only what we need
  std::set<G4Material*> coilMaterials;
  for (const auto& info : coilInfos)
    {coilMaterials.insert(info.material);}
  std::map<G4Material*, G4VisAttributes*> coilVises;
  for (const auto& material : coilMaterials)
    {
      auto coilVis = new G4VisAttributes(*BDSColourFromMaterial::Instance()->GetColour(material));
      coilVis->SetVisibility(true);
      coilVis->SetForceLineSegmentsPerCircle(BDSGlobalConstants::Instance()->NSegmentsPerCircle());
      RegisterVisAttributes(coilVis);
      coilVises[material] = coilVis;
    }

  // loop over coils and build and place them
  G4int i = 0;
  for (const auto& info : coilInfos)
    {
      G4String iStr = std::to_string(i);
      G4String baseName = name + "_coil_" + iStr;
      auto coilSolid = new G4Tubs(baseName + "_solid",
				  info.innerRadius,
				  info.innerRadius + info.radialThickness,
				  0.5*info.lengthZ,
				  0,
				  CLHEP::twopi);
      RegisterSolid(coilSolid);
      auto coilLV = new G4LogicalVolume(coilSolid, info.material, baseName + "_lv");
      RegisterLogicalVolume(coilLV);
      coilLV->SetVisAttributes(coilVises[info.material]);
      
      auto coilPV = new G4PVPlacement(nullptr,
				      G4ThreeVector(0,0,info.offsetZ),
				      coilLV,
				      baseName + "_pv",
				      containerLogicalVolume,
				      false,
				      0,
				      checkOverlaps);
      RegisterPhysicalVolume(coilPV);
      i++;
    }
}

void BDSMuonCooler::BuildAbsorber()
{;}

void BDSMuonCooler::AttachOuterBField()
{;}

void BDSMuonCooler::BuildCavities()
{;}
