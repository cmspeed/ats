/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

/*
  ATS

  EOS for an ideal gas air with a molar fraction of water vapor.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#ifndef _FLOWRELATIONS_EOS_VAPOR_IN_GAS_HH_
#define _FLOWRELATIONS_EOS_VAPOR_IN_GAS_HH_

#include "Teuchos_ParameterList.hpp"
#include "eos.hh"
#include "vapor_pressure_model.hh"

namespace Amanzi {
namespace Flow {
namespace FlowRelations {

// Equation of State model
class EOSVaporInGas {

public:
  explicit EOSVaporInGas(Teuchos::ParameterList& eos_plist);

  virtual double MassDensity(double T, double p, double mol_frac_vapor);
  virtual double DMassDensityDT(double T, double p, double mol_frac_vapor);
  virtual double DMassDensityDp(double T, double p, double mol_frac_vapor);
  virtual double DMassDensityDmol_frac(double T, double p, double mol_frac_vapor);

  virtual double molar_mass(double mol_frac_vapor) {
    return mol_frac_vapor*Mv_ + (1.0-mol_frac_vapor)*gas_eos_->molar_mass(); }

  virtual double MolarDensity(double T, double p) { return gas_eos_->MolarDensity(T,p); }
  virtual double DMolarDensityDT(double T, double p) { return gas_eos_->DMolarDensityDT(T,p); }
  virtual double DMolarDensityDp(double T, double p) { return gas_eos_->DMolarDensityDp(T,p); }

  virtual double SaturatedVaporPressure(double T);

protected:
  virtual void InitializeFromPlist_();

  Teuchos::ParameterList eos_plist_;

  Teuchos::RCP<EOS> gas_eos_;
  Teuchos::RCP<VaporPressureModel> sat_vapor_model_;
  double Mv_;
};

} // namespace
} // namespace
} // namespace

#endif