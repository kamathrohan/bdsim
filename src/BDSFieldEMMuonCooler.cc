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
#include "BDSArray2DCoords.hh"
#include "BDSDebug.hh"
#include "BDSException.hh"
#include "BDSFieldEMMuonCooler.hh"
#include "BDSFieldInfoExtra.hh"
#include "BDSFieldMagSolenoidBlock.hh"
#include "BDSFieldMagSolenoidSheet.hh"
#include "BDSFieldMagSolenoidLoop.hh"
#include "BDSFieldMagDipoleEnge.hh"
#include "BDSFieldMagDipoleHardEdgeMuonCooler.hh"
#include "BDSFieldEMRFCavity.hh"
#include "BDSFieldType.hh"
#include "BDSFieldValue.hh"
#include "BDSInterpolatorRoutines.hh"

#include "G4ThreeVector.hh"
#include "G4Types.hh"
#include "CLHEP/Units/SystemOfUnits.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

BDSFieldEMMuonCooler::BDSFieldEMMuonCooler(const BDSFieldInfoExtraMuonCooler* info,
                                           G4double /*brho*/)
{
  BuildMagnets(info);
  BuildDipoles(info);
  BuildRF(info);
  BuildZBins();

  const G4double unset = std::numeric_limits<G4double>::lowest();
  if (info->zPeriodStart != unset && info->zPeriodEnd != unset && info->periodLength != unset)
    {
      BuildPeriods(info);
      periodsSpecified = true;
    }
}

BDSFieldEMMuonCooler::~BDSFieldEMMuonCooler()
{
  for (auto& e : entries)
    {
      delete e.mag;
      delete e.em;
    }
  delete periodicGrid;
}

void BDSFieldEMMuonCooler::BuildZBins()
{
  G4int n = (G4int)entries.size();
  const G4double inf = std::numeric_limits<G4double>::max();

  G4double minExtent  =  inf;
  G4double globalZMin =  inf;
  G4double globalZMax = -inf;
  for (G4int i = 0; i < n; i++)
    {
      G4double ze = entries[i].zHalfExtent;
      if (ze >= inf / 2.0)
        {
          if (entries[i].type == FieldEntry::Type::Solenoid || entries[i].type == FieldEntry::Type::Dipole)
            {alwaysOn.push_back(i);}
          else
            {throw BDSException(__METHOD_NAME__, "EM field entry with infinite z extent is not supported in muon cooler");}
          continue;
        }
      G4double oz = entries[i].offset.z();
      minExtent  = std::min(minExtent, ze);
      globalZMin = std::min(globalZMin, oz - ze);
      globalZMax = std::max(globalZMax, oz + ze);
    }

  if (minExtent >= inf / 2.0)
    {return;} // all fields are always-on; nothing to bin

  binWidth = minExtent / 2.0;
  const G4int maxBins = 100000;
  nBins = std::min((G4int)std::ceil((globalZMax - globalZMin) / binWidth) + 2, maxBins);
  if (nBins == maxBins)
    {binWidth = (globalZMax - globalZMin) / (maxBins - 1);}

  zBinMin = globalZMin;
  zbins.resize(nBins);

  for (G4int i = 0; i < n; i++)
    {
      G4double ze = entries[i].zHalfExtent;
      if (ze >= inf / 2.0)
        {continue;}
      G4double oz  = entries[i].offset.z();
      G4int binLo  = std::max(0,       (G4int)((oz - ze - zBinMin) / binWidth) - 1);
      G4int binHi  = std::min(nBins-1, (G4int)((oz + ze - zBinMin) / binWidth) + 1);
      for (G4int b = binLo; b <= binHi; b++){
        zbins[b].push_back(i);
      }
    }
   
}

void BDSFieldEMMuonCooler::BuildPeriods(const BDSFieldInfoExtraMuonCooler* info)
{
  G4double zStart = info->zPeriodStart;
  G4double zEnd   = info->zPeriodEnd;
  G4double pLen   = info->periodLength;

  G4int nP = (G4int)std::floor((zEnd - zStart) / pLen);
  periodicZStart  = zEnd - nP * pLen;
  periodicZEnd    = zEnd;
  periodLength    = pLen;

  // x/y extent from the minimum coil inner radius
  G4double minInner = std::numeric_limits<G4double>::max();
  for (const auto& ci : info->coilInfos)
    minInner = std::min(minInner, ci.innerRadius);
  periodicXYMax = (minInner < std::numeric_limits<G4double>::max()) ? minInner : 300*CLHEP::mm;

  // granularity from gridPointsPerMm (points per mm, CLHEP native units)
  periodicGridPointsPerMm = info->coilInfos.empty() ? 1.0 : info->coilInfos.front().gridPointsPerMm;
}

void BDSFieldEMMuonCooler::BuildPeriodicMap() const
{
  std::cout<< __METHOD_NAME__ << ": Building periodic grid " << std::endl;
  G4double rhoMax = periodicXYMax;
  G4int Nrho = std::max(2, (G4int)std::ceil(rhoMax / CLHEP::mm * periodicGridPointsPerMm) + 1);
  G4int Nz   = std::max(2, (G4int)std::ceil(periodLength / CLHEP::mm * periodicGridPointsPerMm) + 1);
  G4double drho = rhoMax / (Nrho - 1);
  G4double dz   = periodLength / (Nz - 1);

  periodicGrid = new BDSArray2DCoords(Nrho, Nz, 0.0, rhoMax, 0.0, periodLength);

  for (G4int irho = 0; irho < Nrho; irho++)
    for (G4int iz = 0; iz < Nz; iz++)
      {
        G4double rho    = irho * drho;
        G4double zWorld = periodicZStart + iz * dz;
        G4ThreeVector pos(rho, 0, zWorld);

        G4double Brho = 0.0;
        G4double Bz   = 0.0;

        for (G4int i : alwaysOn)
          {
            if (entries[i].type != FieldEntry::Type::Solenoid)
              {continue;}
            G4ThreeVector B = entries[i].mag->GetField(pos - entries[i].offset, 0);
            Brho += B.x();
            Bz   += B.z();
          }

        if (nBins > 0)
          {
            G4int bin = (G4int)((zWorld - zBinMin) / binWidth);
            if (bin >= 0 && bin < nBins)
              {
                for (G4int i : zbins[bin])
                  {
                    if (entries[i].type != FieldEntry::Type::Solenoid)
                      {continue;}
                    G4ThreeVector B = entries[i].mag->GetField(pos - entries[i].offset, 0);
                    Brho += B.x();
                    Bz   += B.z();
                  }
              }
          }
        else
          {
            for (G4int i = 0; i < (G4int)entries.size(); i++)
              {
                if (entries[i].type != FieldEntry::Type::Solenoid)
                  {continue;}
                G4ThreeVector B = entries[i].mag->GetField(pos - entries[i].offset, 0);
                Brho += B.x();
                Bz   += B.z();
              }
          }

        (*periodicGrid)(irho, iz) = BDSFieldValue(Brho, 0.0, Bz);
      }
}

void BDSFieldEMMuonCooler::BuildMagnets(const BDSFieldInfoExtraMuonCooler* info)
{

  switch (info->magneticFieldType.underlying())
    {
    case BDSFieldType::solenoidblock:
      {
        for (const auto& ci : info->coilInfos)
          {
            G4double gridPts = ci.useGrid ? ci.gridPointsPerMm : 0.0;
            auto* f = new BDSFieldMagSolenoidBlock(ci.current,
                                                   true,
                                                   ci.innerRadius,
                                                   ci.radialThickness,
                                                   ci.fullLengthZ,
                                                   ci.tiltX,
                                                   ci.tiltY,
                                                   ci.tiltZ,
                                                   ci.onAxisTolerance,
                                                   ci.nSheets,
                                                   gridPts,
                                                   ci.interpolator);
            FieldEntry e;
            e.type        = FieldEntry::Type::Solenoid;
            e.mag         = f;
            e.offset      = G4ThreeVector(ci.offsetX, ci.offsetY, ci.offsetZ);
            e.zHalfExtent = f->GetZHalfExtent();
            entries.push_back(std::move(e));
          }
        break;
      }
    case BDSFieldType::solenoidsheet:
      {
        for (const auto& ci : info->coilInfos)
          {
            G4double gridPts = ci.useGrid ? ci.gridPointsPerMm : 0.0;
            auto* f = new BDSFieldMagSolenoidSheet(ci.current,
                                                   true,
                                                   ci.innerRadius + 0.5*ci.radialThickness,
                                                   ci.fullLengthZ,
                                                   ci.tiltX,
                                                   ci.tiltY,
                                                   ci.tiltZ,
                                                   ci.onAxisTolerance,
                                                   gridPts,
                                                   ci.interpolator);
            FieldEntry e;
            e.type        = FieldEntry::Type::Solenoid;
            e.mag         = f;
            e.offset      = G4ThreeVector(ci.offsetX, ci.offsetY, ci.offsetZ);
            e.zHalfExtent = f->GetZHalfExtent();
            entries.push_back(std::move(e));
          }
        break;
      }
    case BDSFieldType::solenoidloop:
      {
        for (const auto& ci : info->coilInfos)
          {
            const G4double inf = std::numeric_limits<G4double>::max();

            FieldEntry e;
            e.type        = FieldEntry::Type::Solenoid;
            e.mag         = new BDSFieldMagSolenoidLoop(ci.current,
                                                        true,
                                                        ci.innerRadius + 0.5*ci.radialThickness);
            e.offset      = G4ThreeVector(0, 0, ci.offsetZ);
            e.zHalfExtent = inf; // no tolerance set; always evaluate
            entries.push_back(std::move(e));
          }
        break;
      }
    default:
      {
        G4String msg = "\"" + info->magneticFieldType.ToString();
        msg += "\" is not a valid field model for a muon cooler B field";
        throw BDSException(__METHOD_NAME__, msg);
      }
    }
}

void BDSFieldEMMuonCooler::BuildDipoles(const BDSFieldInfoExtraMuonCooler* info)
{

  switch (info->dipoleFieldType.underlying())
    {
    case BDSFieldType::dipole:
      {
        for (const auto& di : info->dipoleInfos)
          {
            FieldEntry e;
            e.type        = FieldEntry::Type::Dipole;
            e.mag         = new BDSFieldMagDipoleHardEdgeMuonCooler(di.fieldStrength,
                                                                     di.apertureRadius,
                                                                     di.fullLengthZ);
            e.offset      = G4ThreeVector(0, 0, di.offsetZ);
            e.zHalfExtent = di.fullLengthZ / 2.0;
            entries.push_back(std::move(e));
          }
        break;
      }
    case BDSFieldType::dipoleenge:
      {
        for (const auto& di : info->dipoleInfos)
          {
            FieldEntry e;
            e.type   = FieldEntry::Type::Dipole;
            auto* f  = new BDSFieldMagDipoleEnge(di.fieldStrength,
                                                  di.apertureRadius,
                                                  di.fullLengthZ,
                                                  di.engeCoefficient,
                                                  di.useGrid,
                                                  di.gridPointsPerMm,
                                                  di.interpolator);
            e.mag         = f;
            e.offset      = G4ThreeVector(0, 0, di.offsetZ);
            e.zHalfExtent = f->GetZHalfExtent();
            entries.push_back(std::move(e));
          }
        break;
      }
    default:
      {
        G4String msg = "\"" + info->dipoleFieldType.ToString();
        msg += "\" is not a valid dipole field model for a muon cooler B field";
        throw BDSException(__METHOD_NAME__, msg);
      }
    }
}

void BDSFieldEMMuonCooler::BuildRF(const BDSFieldInfoExtraMuonCooler* info)
{
  for (const auto& ci : info->cavityInfos)
    {
      FieldEntry e;
      e.type        = FieldEntry::Type::EM;
      e.em          = new BDSFieldEMRFCavity(ci.peakEField,
                                             ci.frequency,
                                             ci.phaseOffset,
                                             ci.cavityRadius,
                                             0.0); // tOffset provided globally
      e.offset      = G4ThreeVector(0.0, 0.0, ci.offsetZ);
      e.timeOffset  = ci.globalTimeOffset;
      e.zHalfExtent = ci.lengthZ / 2.0;
      entries.push_back(std::move(e));
    }
}

std::pair<G4ThreeVector, G4ThreeVector> BDSFieldEMMuonCooler::GetField(const G4ThreeVector& position,
                                                                        const G4double       t) const
{
  std::pair<G4ThreeVector, G4ThreeVector> result;
  G4double qz = position.z();
  if (periodsSpecified && qz >= periodicZStart && qz < periodicZEnd)
    {
      if (!periodicGrid) BuildPeriodicMap();  // one-time cost

      G4double rho    = position.perp();
      G4double zLocal = std::fmod(qz - periodicZStart, periodLength);
      if (zLocal < 0) {zLocal += periodLength;}

      BDSFieldValue localData[2][2];
      G4double frho, fz;
      periodicGrid->ExtractSection2x2(rho, zLocal, localData, frho, fz);
      BDSFieldValue r = BDS::Linear2D(localData, frho, fz);

      const G4double eps = 1e-9 * CLHEP::mm;
      G4double Brho = r.x();
      G4double Bx   = (rho > eps) ? Brho * position.x() / rho : 0.0;
      G4double By   = (rho > eps) ? Brho * position.y() / rho : 0.0;
      result.first  = G4ThreeVector(Bx, By, r.z());

      if (nBins > 0)
        {
          G4int bin = (G4int)((qz - zBinMin) / binWidth);
          if (bin >= 0 && bin < nBins)
            {
              for (G4int i : zbins[bin])
                {
                  const FieldEntry& e = entries[i];
                  G4ThreeVector dr = position - e.offset;
                  if (e.type == FieldEntry::Type::Dipole)
                    {
                      result.first += e.mag->GetField(dr, t);
                    }
                  else if (e.type == FieldEntry::Type::EM)
                    {  
                      if (std::fabs(dr.z()) > e.zHalfExtent)
                        {continue;}
                  //std::cout << __METHOD_NAME__ << ": Evaluating EM field at position " << position.z() << " and time " << t - e.timeOffset << "RF at " << e.offset <<  std::endl ;

                      auto fe = e.em->GetField(dr, t - e.timeOffset);
                      result.first  += fe.first;
                      result.second += fe.second;
                    }
                }
            }
        }
      return result;
    }

  for (G4int i : alwaysOn)
    {
      const FieldEntry& e = entries[i];
      G4ThreeVector dr = position - e.offset;
      result.first += e.mag->GetField(dr, t);
    }

  if (nBins > 0)
    {
      G4int bin = (G4int)((position.z() - zBinMin) / binWidth);
      if (bin >= 0 && bin < nBins)
        {
          for (G4int i : zbins[bin])
            {
              const FieldEntry& e = entries[i];
              G4ThreeVector dr = position - e.offset;
              if (e.type == FieldEntry::Type::Solenoid || e.type == FieldEntry::Type::Dipole)
                {result.first += e.mag->GetField(dr, t);}
              else
                {
                  if (std::fabs(dr.z()) > e.zHalfExtent)
                    {continue;}
                  auto fe = e.em->GetField(dr, t - e.timeOffset);
                  result.first  += fe.first;
                  result.second += fe.second;
                }
            }
        }
    }

  return result;
}
