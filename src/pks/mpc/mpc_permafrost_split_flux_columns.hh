/* -*-  mode: c++; indent-tabs-mode: nil -*- */
/* -------------------------------------------------------------------------
ATS

License: see $ATS_DIR/COPYRIGHT
Author: Ethan Coon

An operator-split permafrost coupler, based on flux.

solve:

(dTheta_s / dt)^* = div k_s grad (z+h)

then solve:

dTheta_s / dt = (dTheta_s / dt)^* + Q_ext + q_ss
dTheta / dt = div k (grad p + rho*g*\hat{z})
k  (grad p + rho*g*\hat{z}) |_s = q_ss

This effectively does an operator splitting on the surface flow equation, but
instead of the typical strateegy of passing pressure, passes the divergence of
lateral fluxes as a fixed source term.

This is the permafrost analog, so deals with energy as well in a similar
strategy.  In this case advection and diffusion of energy are handled in the
first solve:

(dE_s / dt)^* = div (  Kappa_s grad T + hq )

then:

dE_s / dt = (dE_s / dt)^* + QE_ext + h * Q_ext + qE_ss + h * q_ss
dE / dt = div (  Kappa grad T) + hq )
Kappa grad T |_s = qE_ss


------------------------------------------------------------------------- */

#ifndef PKS_MPC_PERMAFROST_SPLIT_FLUX_COLUMNS_HH_
#define PKS_MPC_PERMAFROST_SPLIT_FLUX_COLUMNS_HH_

#include "PK.hh"
#include "mpc.hh"
#include "primary_variable_field_evaluator.hh"

namespace Amanzi {

class MPCPermafrostSplitFluxColumns : public MPC<PK> {

 public:

  MPCPermafrostSplitFluxColumns(Teuchos::ParameterList& FElist,
          const Teuchos::RCP<Teuchos::ParameterList>& plist,
          const Teuchos::RCP<State>& S,
          const Teuchos::RCP<TreeVector>& solution);

  // Virtual destructor
  virtual ~MPCPermafrostSplitFluxColumns() = default;

  // PK methods
  // -- dt is the minimum of the sub pks
  virtual double get_dt();

  // -- initialize in reverse order
  virtual void Initialize(const Teuchos::Ptr<State>& S);
  virtual void Setup(const Teuchos::Ptr<State>& S);
  
  // -- advance each sub pk dt.
  virtual bool AdvanceStep(double t_old, double t_new, bool reinit);

  virtual bool ValidStep();
  
  virtual void set_dt(double dt);

  virtual void CommitStep(double t_old, double t_new,
                          const Teuchos::RCP<State>& S);
  
  virtual void CopyPrimaryToStar(const Teuchos::Ptr<const State>& S,
          const Teuchos::Ptr<State>& S_star);
  virtual void CopyStarToPrimary(double dt);

 protected:

  void init_(const Teuchos::RCP<State>& S);

  virtual void CopyStarToPrimaryPressure_(double dt);
  virtual void CopyStarToPrimaryFlux_(double dt);
  virtual void CopyStarToPrimaryHybrid_(double dt);
  
 protected:
  
  Key p_primary_variable_suffix_;
  Key p_primary_variable_star_;
  Key p_conserved_variable_star_;
  Key p_lateral_flow_source_suffix_;

  Key T_primary_variable_suffix_;
  Key T_primary_variable_star_;
  Key T_conserved_variable_star_;
  Key T_lateral_flow_source_suffix_;
  
  Key cv_key_;
  std::vector<Teuchos::RCP<PrimaryVariableFieldEvaluator> > p_eval_pvfes_;
  std::vector<Teuchos::RCP<PrimaryVariableFieldEvaluator> > T_eval_pvfes_;
  std::vector<std::string> col_domains_;

  std::string coupling_;
  
 private:
  // factory registration
  static RegisteredPKFactory<MPCPermafrostSplitFluxColumns> reg_;


};
} // close namespace Amanzi

#endif
