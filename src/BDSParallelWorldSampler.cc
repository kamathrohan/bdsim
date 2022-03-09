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
#include "BDSAcceleratorModel.hh"
#include "BDSApertureInfo.hh"
#include "BDSBeamline.hh"
#include "BDSBeamlineElement.hh"
#include "BDSBeamPipeInfo.hh"
#include "BDSDebug.hh"
#include "BDSDetectorConstruction.hh"
#include "BDSExtent.hh"
#include "BDSException.hh"
#include "BDSGlobalConstants.hh"
#include "BDSOutput.hh"
#include "BDSParallelWorldSampler.hh"
#include "BDSParser.hh"
#include "BDSSampler.hh"
#include "BDSSamplerCustom.hh"
#include "BDSSamplerCylinder.hh"
#include "BDSSamplerInfo.hh"
#include "BDSSamplerPlane.hh"
#include "BDSSamplerRegistry.hh"
#include "BDSSamplerSphere.hh"
#include "BDSSamplerType.hh"
#include "BDSSDSampler.hh"
#include "BDSUtilities.hh"
#include "BDSWarning.hh"

#include "parser/samplerplacement.h"

#include "globals.hh" // geant4 types / globals
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4Transform3D.hh"
#include "G4VisAttributes.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VUserParallelWorld.hh"

#include <map>
#include <set>
#include <vector>


BDSParallelWorldSampler::BDSParallelWorldSampler(const G4String& name):
  G4VUserParallelWorld("SamplerWorld_" + name),
  suffix(name),
  samplerWorldVis(nullptr),
  generalPlane(nullptr),
  samplerWorldLV(nullptr)
{;}

BDSParallelWorldSampler::~BDSParallelWorldSampler()
{
  // we leak all the placements as g4 is incredibly slow to delete
  // them as it deregisters them - g4 will clean up
  //for (auto placement : placements)
  //  {delete placement;}
  delete samplerWorldVis;
  // samplers are deleted by the sampler registry
}

void BDSParallelWorldSampler::Construct()
{
  BDSGlobalConstants* globals = BDSGlobalConstants::Instance();
  G4VPhysicalVolume* samplerWorld = GetWorld();
  samplerWorldLV = samplerWorld->GetLogicalVolume();

  samplerWorldVis = new G4VisAttributes(*(globals->VisibleDebugVisAttr()));
  samplerWorldVis->SetForceWireframe(true);//just wireframe so we can see inside it
  samplerWorldLV->SetVisAttributes(samplerWorldVis);
  
  const G4double samplerRadius = 0.5*globals->SamplerDiameter();
  const BDSBeamline* beamline  = BDSAcceleratorModel::Instance()->BeamlineMain();
  const G4bool checkOverlaps   = globals->CheckOverlaps();

  // Construct the one sampler typically used for a general sampler.
  generalPlane = new BDSSamplerPlane("Plane_sampler", samplerRadius);
  samplerInstances[-1] = generalPlane;
  // Construct any others we need.
  std::map<int, std::set<int>> filterIDToSet = BDSParser::Instance()->GetSamplerFilterIDToSet();
  for (const auto& kv : filterIDToSet)
    {
      BDSSamplerPlane* samp = new BDSSamplerPlane("Plane_sampler_w_filter_set_" + std::to_string(kv.first),
                                                  samplerRadius,
                                                  (G4int)kv.first);
      samplerInstances[(G4int)kv.first] = samp;
    }

  // For each element in the beamline (if a beamline has been defined) construct and place the
  // appropriate type of sampler if required. Info encoded in BDSBeamlineElement instance
  if (beamline != nullptr)
    {
      for (auto element : *beamline)
	{Place(element, samplerRadius);}
    }
  
  // Now user customised samplers
  std::vector<GMAD::SamplerPlacement> samplerPlacements = BDSParser::Instance()->GetSamplerPlacements();

  for (const auto& samplerPlacement : samplerPlacements)
    {
      G4cout << "User placed sampler: \"" << samplerPlacement.name << "\"" << G4endl;
      
      if (BDSOutput::InvalidSamplerName(samplerPlacement.name))
	{
	  G4cerr << __METHOD_NAME__ << "invalid sampler name \"" << samplerPlacement.name << "\"" << G4endl;
	  BDSOutput::PrintProtectedNames(G4cerr);
	  throw BDSException(__METHOD_NAME__, "");
	}
      
      // use main beamline - in future, multiple beam lines
      G4Transform3D transform = BDSDetectorConstruction::CreatePlacementTransform(samplerPlacement, beamline);
  
      BDSSamplerType st = BDS::DetermineSamplerType(samplerPlacement.samplerType);
      AdjustTransform(transform, st); // for 'forward' samplers we add an extra rotation
      BDSSampler* sampler = BuildSampler(samplerPlacement, st);
      G4String samplerName = G4String(samplerPlacement.name);
      G4int samplerID = BDSSamplerRegistry::Instance()->RegisterSampler(samplerName,
									sampler,
									transform,
                  -1000,
                  nullptr,
                  st);
      
      G4String uniqueName = BDSSamplerRegistry::Instance()->GetNameUnique(samplerID);
      if (uniqueName != samplerName)
	{BDS::Warning("Sampler placement with name \"" + samplerName + "\" will be named \"" + uniqueName + "\" in the output");}

      G4PVPlacement* pl = new G4PVPlacement(transform,
					    sampler->GetContainerLogicalVolume(),
					    samplerName + "_pv",
					    samplerWorldLV,
					    false,
					    samplerID,
					    checkOverlaps);
      placements.push_back(pl);
    } 
}

BDSSampler* BDSParallelWorldSampler::BuildSampler(const GMAD::SamplerPlacement& samplerPlacement,
                                                  BDSSamplerType st) const
{
  BDSSampler* result = nullptr;
  G4String samplerName = G4String(samplerPlacement.name);
  
  switch (st.underlying())
    {
    case BDSSamplerType::plane:
      {
	BDSApertureInfo* shape;
	if (samplerPlacement.apertureModel.empty())
	  {
	    shape = new BDSApertureInfo(samplerPlacement.shape,
					samplerPlacement.aper1 * CLHEP::m,
					samplerPlacement.aper2 * CLHEP::m,
					samplerPlacement.aper3 * CLHEP::m,
					samplerPlacement.aper4 * CLHEP::m,
					samplerPlacement.name);
	  }
	else
	  {shape = BDSAcceleratorModel::Instance()->Aperture(samplerPlacement.apertureModel);}
	
	result = new BDSSamplerCustom(samplerName, *shape, samplerPlacement.partIDSetID);
	break;
      }
    case BDSSamplerType::cylinder:
      {
        G4double startAnglePhi = samplerPlacement.startAnglePhi * CLHEP::rad;
        G4double sweepAnglePhi = samplerPlacement.sweepAnglePhi * CLHEP::rad;
        if (sweepAnglePhi <= 0) // default in parser is -1 to flag we should use 2pi now we have units
          {sweepAnglePhi = CLHEP::twopi;}
        else if (sweepAnglePhi > CLHEP::twopi + 1e-6)
	  {throw BDSException(__METHOD_NAME__, "\"sweepAnglePhi\" must be in range (0 to pi] in samplerplacement \"" + samplerPlacement.name + "\"");}
	result = new BDSSamplerCylinder(samplerName,
					samplerPlacement.aper1 * CLHEP::m,
					2 * samplerPlacement.aper2 * CLHEP::m,
					startAnglePhi,
					sweepAnglePhi,
					samplerPlacement.partIDSetID);
	break;
      }
    case BDSSamplerType::cylinderforward:
      {
        if (BDS::IsFinite(samplerPlacement.startAnglePhi))
          {BDS::Warning("\"startAnglePhi\" in samplerplacement \""+samplerPlacement.name+"\" != 0 -> this has no effect for a cylinderforward sampler");}
        
        G4double sweepAnglePhi = samplerPlacement.sweepAnglePhi * CLHEP::rad;
        if (sweepAnglePhi <= 0) // default in parser is -1 to flag we should use 2pi now we have units
          {sweepAnglePhi = CLHEP::twopi;}
        else if (sweepAnglePhi > CLHEP::twopi + 1e-6)
          {throw BDSException(__METHOD_NAME__, "\"sweepAnglePhi\" must be in range (0 to pi] in samplerplacement \"" + samplerPlacement.name + "\"");}
        G4double startAnglePhi = -0.5*sweepAnglePhi;
        result = new BDSSamplerCylinder(samplerName,
                                        samplerPlacement.aper1 * CLHEP::m,
                                        2 * samplerPlacement.aper2 * CLHEP::m,
                                        startAnglePhi,
                                        sweepAnglePhi,
                                        samplerPlacement.partIDSetID);
        break;
      }
    case BDSSamplerType::sphere:
      {
        G4double startAnglePhi = samplerPlacement.startAnglePhi * CLHEP::rad;
        G4double sweepAnglePhi = samplerPlacement.sweepAnglePhi * CLHEP::rad;
        if (sweepAnglePhi <= 0) // default in parser is -1 to flag we should use 2pi now we have units
          {sweepAnglePhi = CLHEP::twopi;}
        else if (sweepAnglePhi > CLHEP::twopi + 1e-6)
          {throw BDSException(__METHOD_NAME__, "\"sweepAnglePhi\" must be in range (0 to 2 pi] in samplerplacement \"" + samplerPlacement.name + "\"");}
        G4double startAngleTheta = samplerPlacement.startAngleTheta * CLHEP::rad;
        G4double sweepAngleTheta = samplerPlacement.sweepAngleTheta * CLHEP::rad;
        if (sweepAngleTheta <= 0) // default in parser is -1 to flag we should use 2pi now we have units
          {sweepAngleTheta = CLHEP::pi;}
        else if (sweepAnglePhi > CLHEP::pi + 1e-6)
          {throw BDSException(__METHOD_NAME__, "\"sweepAngleTheta\" must be in range (0 to pi] in samplerplacement \"" + samplerPlacement.name + "\"");}
        
        result = new BDSSamplerSphere(samplerName,
				      samplerPlacement.aper1 * CLHEP::m,
				      startAnglePhi,
				      sweepAnglePhi,
				      startAngleTheta,
				      sweepAngleTheta,
				      samplerPlacement.partIDSetID);
	break;
      }
    case BDSSamplerType::sphereforward:
      {
        if (BDS::IsFinite(samplerPlacement.startAnglePhi))
          {BDS::Warning("\"startAnglePhi\" in samplerplacement \""+samplerPlacement.name+"\" != 0 -> this has no effect for a sphereforward sampler");}
        if (BDS::IsFinite(samplerPlacement.startAngleTheta))
          {BDS::Warning("\"startAngleTheta\" in samplerplacement \""+samplerPlacement.name+"\" != 0 -> this has no effect for a sphereforward sampler");}
        
        G4double sweepAnglePhi = samplerPlacement.sweepAnglePhi * CLHEP::rad;
        if (sweepAnglePhi <= 0) // default in parser is -1 to flag we should use 2pi now we have units
          {sweepAnglePhi = CLHEP::twopi;}
        else if (sweepAnglePhi > CLHEP::twopi + 1e-6)
          {throw BDSException(__METHOD_NAME__, "\"sweepAnglePhi\" must be in range (0 to 2 pi] in samplerplacement \"" + samplerPlacement.name + "\"");}
        G4double sweepAngleTheta = samplerPlacement.sweepAngleTheta * CLHEP::rad;
        if (sweepAngleTheta <= 0) // default in parser is -1 to flag we should use 2pi now we have units
          {sweepAngleTheta = CLHEP::pi;}
        else if (sweepAngleTheta > CLHEP::pi + 1e-6)
          {throw BDSException(__METHOD_NAME__, "\"sweepAngleTheta\" must be in range (0 to pi] in samplerplacement \"" + samplerPlacement.name + "\"");}
  
        G4double startAngleTheta = CLHEP::halfpi -0.5*sweepAngleTheta;
        G4double startAnglePhi   = -0.5*sweepAnglePhi;
        
        result = new BDSSamplerSphere(samplerName,
                                      samplerPlacement.aper1 * CLHEP::m,
                                      startAnglePhi,
                                      sweepAnglePhi,
                                      startAngleTheta,
                                      sweepAngleTheta,
                                      samplerPlacement.partIDSetID);
        break;
      }
    }
  return result;
}

void BDSParallelWorldSampler::AdjustTransform(G4Transform3D& trans,
                                              BDSSamplerType st) const
{
  switch (st.underlying())
    {
    case BDSSamplerType::cylinderforward:
      {
	auto rmExisting = trans.getRotation();
	G4RotationMatrix rm;
	rm.rotate(-CLHEP::halfpi, {0,0,1});
	rm.rotate(-CLHEP::halfpi, {1,0,0});
	rmExisting *= rm;
	trans = G4Transform3D(rmExisting, trans.getTranslation());
	break;
      }
    case BDSSamplerType::sphereforward:
      {
	auto rmExisting = trans.getRotation();
	G4RotationMatrix rm;
	rm.rotate(-CLHEP::halfpi, {0,1,0});
	rmExisting *= rm;
	trans = G4Transform3D(rmExisting, trans.getTranslation());
	break;
      }
    default:
      {break;}
    }
}

void BDSParallelWorldSampler::Place(const BDSBeamlineElement* element,
				    G4double samplerRadius)
{
  BDSSamplerInfo* samplerInfo = element->GetSamplerInfo();
  if (!samplerInfo)
    {return;}
  BDSSamplerType samplerType = samplerInfo->samplerType;
  if (samplerType == BDSSamplerType::none)
    {return;}
  // else must be a valid sampler
  
  G4String name = samplerInfo->name;
  G4double sEnd = element->GetSPositionEnd();
  
  BDSSampler* sampler = nullptr;
  switch (samplerType.underlying())
    {
    case BDSSamplerType::plane:
      {sampler = samplerInstances[samplerInfo->pdgSetID]; break;}
    case BDSSamplerType::cylinder:
      {// these are built uniquely for each instance so the length matches exactly
        // TBC - could include angled faces
	G4double length = element->GetAcceleratorComponent()->GetChordLength();
  
	// for a cylindrical sampler we make it 'fit' the component
	G4double boundingRadius = element->GetExtent().TransverseBoundingRadius();
	if (boundingRadius > 0) // protect against bad extent, i.e. 0
	  {samplerRadius = boundingRadius;}
	sampler = new BDSSamplerCylinder(name,
					 samplerRadius,
					 length,
					 0, CLHEP::twopi,
					 samplerInfo->pdgSetID);
	break;
      }
    default: // no spherical samplers for beam line samplers
      {break;} // leave as nullptr - shouldn't occur due to if at top
    }
  
  if (sampler)
    {
      G4Transform3D* pt = new G4Transform3D(*element->GetSamplerPlacementTransform());
      
      G4int samplerID = BDSSamplerRegistry::Instance()->RegisterSampler(name,
									sampler,
									*pt,
									sEnd,
									element,
									samplerType);

      G4VPhysicalVolume* samplerWorld   = GetWorld();
      samplerWorldLV = samplerWorld->GetLogicalVolume();
      const G4bool checkOverlaps = BDSGlobalConstants::Instance()->CheckOverlaps();
      // record placements for cleaning up at destruction.
      G4PVPlacement* pl = new G4PVPlacement(*pt,              // placement transform
					    sampler->GetContainerLogicalVolume(), // logical volume
					    name + "_pv",     // name of placement
					    samplerWorldLV,   // mother volume
					    false,            // no boolean operation
					    samplerID,        // copy number
					    checkOverlaps);

      placements.push_back(pl);
    }
}
