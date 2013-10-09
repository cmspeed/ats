/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

// -----------------------------------------------------------------------------
// ATS
//
// License: see $ATS_DIR/COPYRIGHT
// Author: Ethan Coon (ecoon@lanl.gov)
//
// Scheme for taking coefficients for div-grad operators from cells to
// faces.
// -----------------------------------------------------------------------------

#ifndef AMANZI_UPWINDING_TOTALFLUX_SCHEME_
#define AMANZI_UPWINDING_TOTALFLUX_SCHEME_

#include "upwinding.hh"

namespace Amanzi {

class State;
class CompositeVector;

namespace Operators {

class UpwindTotalFlux : public Upwinding {

public:

  UpwindTotalFlux(std::string pkname,
                  std::string cell_coef,
                  std::string face_coef,
                  std::string flux);

  void Update(const Teuchos::Ptr<State>& S);

  void CalculateCoefficientsOnFaces(
        const CompositeVector& cell_coef,
        const CompositeVector& flux,
        const Teuchos::Ptr<CompositeVector>& face_coef);

private:

  std::string pkname_;
  std::string cell_coef_;
  std::string face_coef_;
  std::string flux_;
};

} // namespace
} // namespace

#endif
