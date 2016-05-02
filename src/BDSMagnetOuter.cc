#include "BDSGeometryComponent.hh"
#include "BDSGeometryComponentHollow.hh"
#include "BDSMagnetOuter.hh"

#include <utility>

class G4VSolid;
class G4LogicalVolume;

BDSMagnetOuter::BDSMagnetOuter(G4VSolid*                    containerSolid,
			       G4LogicalVolume*             containerLV,
			       std::pair<G4double,G4double> extentX,
			       std::pair<G4double,G4double> extentY,
			       std::pair<G4double,G4double> extentZ,
			       BDSGeometryComponent*        magnetContainerIn,
			       G4ThreeVector                placementOffset,
			       BDSGeometryComponentHollow*  endPieceBeforeIn,
			       BDSGeometryComponentHollow*  endPieceAfterIn):
  BDSGeometryComponent(containerSolid, containerLV, extentX, extentY, extentZ,
		       placementOffset),
  magnetContainer(magnetContainerIn),
  endPieceBefore(endPieceBeforeIn),
  endPieceAfter(endPieceAfterIn)
{;}

BDSMagnetOuter::BDSMagnetOuter(BDSGeometryComponent* componentIn,
			       BDSGeometryComponent* magnetContainerIn,
			       BDSGeometryComponentHollow* endPieceBeforeIn,
			       BDSGeometryComponentHollow* endPieceAfterIn):
  BDSGeometryComponent(*componentIn),
  magnetContainer(magnetContainerIn),
  endPieceBefore(endPieceBeforeIn),
  endPieceAfter(endPieceAfterIn)
{;}

void BDSMagnetOuter::ClearMagnetContainer()
{
  if (magnetContainer)
    {
      delete magnetContainer;
      magnetContainer = nullptr;
    }
}

void BDSMagnetOuter::ClearEndPieces()
{
  if (endPieceBefore)
    {delete endPieceBefore; endPieceBefore = nullptr;}
  if (endPieceAfter)
    {delete endPieceAfter; endPieceAfter = nullptr;}
}

BDSMagnetOuter::~BDSMagnetOuter()
{
  ClearMagnetContainer();
  ClearEndPieces();
}
