/* -*-  mode: c++; indent-tabs-mode: nil -*- */
//! A base class with default implementations of methods for a PK that can be implicitly integrated in time.

/*
  ATS is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/


/*!

``PKBDFBase`` is a base class from which PKs that want to use the ``BDF``
series of implicit time integrators must derive.  It specifies both the
``BDFFnBase`` interface and implements some basic functionality for ``BDF``
PKs.

* `"initial time step`" ``[double]`` **1.**

  The initial timestep size for the PK, this ensures that the initial timestep
  will not be **larger** than this value.

* `"assemble preconditioner`" ``[bool]`` **true** 

  A flag for the PK to not assemble its preconditioner if it is not needed by
  a controlling PK.  This is usually set by the MPC, not by the user.

In the top-most (in the PK tree) PK that is meant to be integrated implicitly,
several additional specs are included.  For instance, in a strongly coupled
flow and energy problem, these specs are included in the ``StrongMPC`` that
couples the flow and energy PKs, not to the flow or energy PK itself.
  
* `"time integrator`" ``[implicit-time-integrator-typed-spec]`` **optional**
  A TimeIntegrator_.  Note that this is only provided if this PK is not
  strongly coupled to other PKs.

* `"preconditioner`" ``[preconditioner-typed-spec]`` **optional** is a Preconditioner_ spec.
  Note that this is only used if this PK is not strongly coupled to other PKs.

  This spec describes how to form the (approximate) inverse of the preconditioner.
  
NOTE: ``PKBDFBase  (v)-->`` PKDefaultBase_

*/


#ifndef ATS_PK_BDF_BASE_HH_
#define ATS_PK_BDF_BASE_HH_

#include "Teuchos_TimeMonitor.hpp"

#include "BDFFnBase.hh"
#include "BDF1_TI.hh"
#include "PK_BDF.hh"



namespace Amanzi {

class PK_BDF_Default : public PK_BDF {

 public:


  PK_BDF_Default(Teuchos::ParameterList& pk_tree,
                 const Teuchos::RCP<Teuchos::ParameterList>& glist,
                 const Teuchos::RCP<State>& S,
                 const Teuchos::RCP<TreeVector>& solution) :
    PK_BDF(pk_tree, glist, S, solution),
    PK(pk_tree, glist, S, solution) {}
  
  // Virtual destructor
  virtual ~PK_BDF_Default() {}

  // Default implementations of PK methods.
  // -- Setup
  virtual void Setup(const Teuchos::Ptr<State>& S);

  // -- Initialize
  virtual void Initialize(const Teuchos::Ptr<State>& S);

  // -- Choose a time step compatible with physics.
  virtual double get_dt();

  virtual void set_dt(double dt_);

  // -- Advance from state S0 to state S1 at time S0.time + dt.
  virtual bool AdvanceStep(double t_old, double t_new, bool reinit);

  // -- Commit any secondary (dependent) variables.
  virtual void CommitStep(double t_old, double t_new, const Teuchos::RCP<State>& S);

  // virtual void Solution_to_State(const TreeVector& soln,
  //                                const Teuchos::RCP<State>& S);
  // virtual void Solution_to_State(TreeVector& soln,
  //                                const Teuchos::RCP<State>& S);

  virtual void set_states(const Teuchos::RCP<const State>& S,
                        const Teuchos::RCP<State>& S_inter,
                        const Teuchos::RCP<State>& S_next);

  // update the continuation parameter
  virtual void UpdateContinuationParameter(double lambda);

  // -- Check the admissibility of a solution.
  virtual bool IsAdmissible(Teuchos::RCP<const TreeVector> up) { return true; }

  // -- Possibly modify the predictor that is going to be used as a
  //    starting value for the nonlinear solve in the time integrator.
  virtual bool ModifyPredictor(double h, Teuchos::RCP<const TreeVector> up,
          Teuchos::RCP<TreeVector> u) { return false; }

  // -- Possibly modify the correction before it is applied
  virtual AmanziSolvers::FnBaseDefs::ModifyCorrectionResult
      ModifyCorrection(double h, Teuchos::RCP<const TreeVector> res,
                       Teuchos::RCP<const TreeVector> u,
                       Teuchos::RCP<TreeVector> du) {
    return AmanziSolvers::FnBaseDefs::CORRECTION_NOT_MODIFIED;
  }


  // experimental approach -- calling this indicates that the time
  // integration scheme is changing the value of the solution in
  // state.
  virtual void ChangedSolution() = 0;
  virtual void ChangedSolution(const Teuchos::Ptr<State>& S) = 0;

 
 protected: // data
  // preconditioner assembly control
  bool assemble_preconditioner_;

  // timestep control
  double dt_;
  Teuchos::RCP<BDF1_TI<TreeVector, TreeVectorSpace> > time_stepper_;

  // timing
  Teuchos::RCP<Teuchos::Time> step_walltime_;

};

} // namespace

#endif
