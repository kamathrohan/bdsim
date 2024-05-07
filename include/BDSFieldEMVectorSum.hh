/* 
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
#ifndef BDSFIELDEMVECTORSUM_H
#define BDSFIELDEMVECTORSUM_H

#include "BDSFieldEM.hh"

#include "G4ThreeVector.hh"
#include "G4Types.hh"

#include <vector>

/**
 * @brief A vector sum of multiple displaced EM fields.
 * 
 * @author Chris Rogers
 */

class BDSFieldEMVectorSum: public BDSFieldEM
{
public:
  /** Construct a BDSFieldEMVectorSum with no child fields */
  BDSFieldEMVectorSum();

  /** Construct a BDSFieldEMVectorSum with a list of fields and offsets
   * 
   *  fieldsIn: vector of input fields. BDSFieldEMVectorSum takes ownership of
   *  the memory allocated to fieldsIn
   *  fieldOffsetsIn: vector of translations.
   *  timeOffsetsIn: vector of time offsets.
   *  zLengthsIn: vector of lengths in z.
   */
  explicit BDSFieldEMVectorSum(const std::vector<BDSFieldEM*>& fieldsIn,
                               const std::vector<G4ThreeVector>& fieldOffsetsIn,
                               const std::vector<double>& timeOffsetsIn,
                               const std::vector<double>& zLengthsIn);

  /** Deletes fieldsIn */
  virtual ~BDSFieldEMVectorSum();

  /** Append a child field to the BDSFieldEMVectorSum
   *  
   *  positionOffset: offset of the child field from the BDSFieldEMVectorSum centre
   *  timeOffset: offset of the child field from the parent geometry
   *  zLength: zLength of the child field (see GetField)
   *  emfield: pointer to the BDSFieldEM to be added; BDSFieldEMVectorSum takes
   *           ownership of the emfield memory 
   */
  virtual void PushBackField(const G4ThreeVector& positionOffset,
                             double timeOffset,
                             double zLength,
                             BDSFieldEM* emfield);

  /** Calculate the field value.
   *  
   *  position: position relative to the centre of the BDSFieldEMVectorSum at which
   *  the field is calculated
   *  time: time relative to the BDSFieldEMVectorSum
   *  
   *  Loops over members of fields, translates to local coordinates. If the local z 
   *  position is within zLength/2, return the field calculated using local coordinates
   * 
   *  Returns a pair of three vectors, <bfield, efield> in the coordinate system
   *  relative to the BDSFieldEMVectorSum  
   */ 
  virtual std::pair<G4ThreeVector, G4ThreeVector> GetField(const G4ThreeVector& position,
                                                           const G4double       t = 0) const;

private:
  std::vector<BDSFieldEM*> fields;
  std::vector<G4ThreeVector> fieldOffsets;
  std::vector<double> timeOffsets;
  std::vector<double> zLengthsOver2; // store half lengths to save a flop at lookup time
};

#endif
