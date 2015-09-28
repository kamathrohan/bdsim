#ifndef BDSDETECTORSOLENOIDMAGFIELD_H
#define BDSDETECTORSOLENOIDMAGFIELD_H

#include "G4Types.hh"
#include "G4MagneticField.hh"

#include "BDSAuxiliaryNavigator.hh"
#include "BDSMagFieldMesh.hh"

/**
 * @brief This class descibes an inner and an outer solenoid field. 
 * Developed from BDSQuadMagField.cc
 */

class BDSDetectorSolenoidMagField: public BDSMagFieldMesh, public BDSAuxiliaryNavigator
{
public: 
  BDSDetectorSolenoidMagField(G4double BIn,
			      G4double Bout,
			      G4double radiusIn,
			      G4double radiusOut,
			      G4double zMin,
			      G4double zMax);
  virtual ~BDSDetectorSolenoidMagField();
  
  virtual void  GetFieldValue( const G4double Point[4],
			       G4double *Bfield ) const;
private:
  G4double itsBIn, itsBOut, itsRadiusIn, itsRadiusOut, itsZMin, itsZMax;
};

#endif
