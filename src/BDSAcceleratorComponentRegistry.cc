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
#include "BDSAcceleratorComponentRegistry.hh"
#include "BDSDebug.hh"
#include "BDSLine.hh"

#include <ostream>
#include <iomanip>
#include <ios>
#include <map>

BDSAcceleratorComponentRegistry* BDSAcceleratorComponentRegistry::instance = nullptr;

BDSAcceleratorComponentRegistry* BDSAcceleratorComponentRegistry::Instance()
{
  if (!instance)
    {instance = new BDSAcceleratorComponentRegistry();}
  return instance;
}

BDSAcceleratorComponentRegistry::BDSAcceleratorComponentRegistry()
{;}

BDSAcceleratorComponentRegistry::~BDSAcceleratorComponentRegistry()
{
#ifdef BDSDEBUG
  G4cout << __METHOD_NAME__ << "size of registry " << registry.size() << G4endl;
#endif
  for (auto& i : registry)
    {delete i.second;}
  for (auto ac : allocatedComponents)
    {delete ac;}
  for (auto ac : curvilinearComponents)
    {delete ac;}
  for (auto ac : tunnelComponents)
    {delete ac;}
  
  instance = nullptr;
}

void BDSAcceleratorComponentRegistry::RegisterComponent(BDSAcceleratorComponent* component,
							bool isModified)
{
  // If the component was modified beyond its original element definition in the parser,
  // ie a drift was modified to match a pole face of a bend, then store if for memory
  // management, but not in the registry
  if (isModified)
    {
      if (IsRegisteredAllocated(component))
	{return;}

      allocatedComponents.insert(component);
      // add to registry for unique components only
      registryForAllocated[component->GetName()] = component;
      if (BDSLine* line = dynamic_cast<BDSLine*>(component))
	{// if line then also add constituents
	  for (const auto element : *line)
	    {RegisterComponent(element, true);}
	}
      return;
    }
  
  if (IsRegistered(component))
    {return;} // don't register something that's already registered

  // in both cases we register the BDSLine* object as it doesn't own its constituents
  registry[component->GetName()] = component;
  // increment counter for each component type
  ++typeCounter[component->GetType()];
  if (BDSLine* line = dynamic_cast<BDSLine*>(component))
    {
      for (const auto element : *line)
	{RegisterComponent(element, false);}
    }
#ifdef BDSDEBUG
  G4cout << __METHOD_NAME__ << "size of registry " << registry.size() << G4endl;
#endif
}

G4bool BDSAcceleratorComponentRegistry::IsRegistered(BDSAcceleratorComponent* component)
{
  return IsRegistered(component->GetName());
}

G4bool BDSAcceleratorComponentRegistry::IsRegisteredAllocated(const BDSAcceleratorComponent* component) const
{
  return std::find(allocatedComponents.begin(), allocatedComponents.end(), component) != allocatedComponents.end();
}

G4bool BDSAcceleratorComponentRegistry::IsRegistered(const G4String& name)
{
  auto search = registry.find(name);
  return !(search == registry.end());
}

BDSAcceleratorComponent* BDSAcceleratorComponentRegistry::GetComponent(const G4String& name)
{
  try
    {return registry.at(name);}
  catch (const std::out_of_range& /*oor*/)
    {
      G4cerr << __METHOD_NAME__ << "unknown component named: \"" << name << "\"" << G4endl;
      return nullptr;
    }
}

void BDSAcceleratorComponentRegistry::RegisterCurvilinearComponent(BDSAcceleratorComponent* component)
{
  curvilinearComponents.insert(component);
}

void BDSAcceleratorComponentRegistry::RegisterTunnelComponent(BDSAcceleratorComponent* component)
{
  tunnelComponents.insert(component);
}

std::map<G4String, BDSAcceleratorComponent*> BDSAcceleratorComponentRegistry::AllComponentsIncludingUnique() const
{
  std::map<G4String, BDSAcceleratorComponent*> result;
  result.insert(registry.begin(), registry.end());
  result.insert(registryForAllocated.begin(), registryForAllocated.end());
  return result;
}

std::ostream& operator<< (std::ostream &out, BDSAcceleratorComponentRegistry const &r)
{
  // save flags since std::left changes the stream
  std::ios_base::fmtflags ff = out.flags();
  out << "Accelerator Component Registry:" << G4endl;
  for (const auto& it : r.registry)
    {out << std::left << std::setw(15) << it.second->GetType() << " \"" << it.first << "\"" << G4endl;}
  out.flags(ff); // reset flags
  return out;
}

void BDSAcceleratorComponentRegistry::PrintNumberOfEachType() const
{
  G4cout << __METHOD_NAME__ << G4endl;
  for (const auto& kv : typeCounter)
    {G4cout << std::setw(20) << kv.first << " : " << kv.second << G4endl;}
}
