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
#ifndef BDSMUONCOOLER_H
#define BDSMUONCOOLER_H
#include "BDSAcceleratorComponent.hh"

#include "G4String.hh"
#include "G4ThreeVector.hh"
#include "G4Types.hh"

#include <vector>

class BSDFieldInfo;
class G4Material;

namespace BDS
{
  struct MuonCoolerCoilInfo
  {
    G4double innerRadius;
    G4double radialThickness;
    G4double lengthZ;
    G4double zOffset;
  };

  struct MuonCoolerCavityInfo
  {
    G4double zOffset;
    G4double length;
    G4double windowThickness;
    G4Material* windowMaterial;
    BDSFieldInfo* fieldRecipe;
  };

  struct MuonCoolerAbsorberInfo
  {
    G4String absorberType;
    G4double cylinderLength;
    G4double cylinderRAdius;
    G4double wedgeOpeningAngle;
    G4double wedgeRotationAngle;
    G4ThreeVector wedgePlacement;
    G4double wedgeApexToBase;
  };
}

/**
 * @brief A muon cooling module.
 *
 * @author Laurie Nevay
 */

class BDSMuonCooler: public BDSAcceleratorComponent
{
public:
  BDSMuonCooler(const G4String& nameIn,
		G4double        lengthIn,
		G4double        containerRadiusIn,
		const std::vector<BDS::MuonCoolerCoilInfo>&   coilInfosIn,
		const std::vector<BDS::MuonCoolerCavityInfo>& cavityInfosIn,
		const BDS::MuonCoolerAbsorberInfo&            absorberInfoIn,
		BDSFieldInfo*   outerFieldRecipeIn);
  virtual ~BDSMuonCooler();

protected:
  /// Construct geometry.
  virtual void Build();

private:  
  /// Void function to fulfill BDSAcceleratorComponent requirements.
  virtual void BuildContainerLogicalVolume(){;};

  G4double containerRadius;
  std::vector<BDS::MuonCoolerCoilInfo> coilInfos;
  std::vector<BDS::MuonCoolerCavityInfo> cavityInfos;
  BDS::MuonCoolerAbsorberInfo absorberInfo;
  BDSFieldInfo* outerFieldRecipe;
};

#endif
