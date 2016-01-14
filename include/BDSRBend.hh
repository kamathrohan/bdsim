#ifndef BDSRBEND_H
#define BDSRBEND_H 

#include "globals.hh"

#include "BDSMagnet.hh"

class  BDSBeamPipeInfo;
struct BDSMagnetOuterInfo;

class BDSBeamPipe;

class BDSRBend: public BDSMagnet
{
public:
  BDSRBend(G4String            name,
	   G4double            length,
	   G4double            bField,
	   G4double            bGrad,
	   G4double            angle,
	   BDSBeamPipeInfo*    beamPipeInfo,
	   BDSMagnetOuterInfo* magnetOuterInfo);

private:
  G4double bField;
  G4double bGrad;
  G4double magFieldLength;

  /// chord length of straight section (along main chord) [m]
  G4double straightSectionChord;

  /// length of little straight sections on either side of dipole [m]
  G4double straightSectionLength;

  /// x shift for magnet and beampipe from chord
  G4double magnetXShift;

  /// radius of magnet body
  G4double outerRadius;

  /// Extra start and finish piece of beam pipe
  BDSBeamPipe* bpFirstBit;
  BDSBeamPipe* bpLastBit;

  /// Override method from BDSAcceleratorComponent to detail construction process.
  virtual void Build();

  /// Override method so we can build several bits of beam pipe
  virtual void BuildBeampipe();

  /// Override BDSMagnet::BuildOuter() so we can get a different length of central section
  /// and magnet container.
  virtual void BuildOuter();

  /// Override BDSMagnet::BuildContainerLogicalVolume() so we can conditionally build the
  /// correct container volume if no outer magnet geometry is built. In the case of an RBend
  /// we can't simply use the central beam pipe segment as the container volume as it'll be too
  /// small.
  virtual void BuildContainerLogicalVolume();

  /// Place the beam pipe and outer geometry in the overall container. If there's no outer
  /// geometry, then we don't need to place either as the beam pipe becomes the container.
  virtual void PlaceComponents();

  /// temporary function while old constructor still exists - used to avoid duplicating
  /// code in the mean time
  void CalculateLengths(G4double aLength);
};

#endif
