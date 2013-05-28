/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

/*
  Evaluates the porosity, given a small compressibility of rock.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#include "compressible_porosity_evaluator.hh"
#include "compressible_porosity_model.hh"

namespace Amanzi {
namespace Flow {
namespace FlowRelations {

// registry of method
Utils::RegisteredFactory<FieldEvaluator,CompressiblePorosityEvaluator> CompressiblePorosityEvaluator::fac_("compressible porosity");


CompressiblePorosityEvaluator::CompressiblePorosityEvaluator(Teuchos::ParameterList& plist) :
    SecondaryVariableFieldEvaluator(plist) {

  pres_key_ = plist_.get<std::string>("pressure key", "pressure");
  dependencies_.insert(pres_key_);
  poro_key_ = plist_.get<std::string>("base porosity key", "base_porosity");
  dependencies_.insert(poro_key_);

  if (my_key_ == std::string("")) {
    my_key_ = plist_.get<std::string>("compressed porosity key",
            "porosity");
  }
  setLinePrefix(my_key_+std::string(" evaluator"));

  ASSERT(plist_.isSublist("compressible porosity model parameters"));
  model_ = Teuchos::rcp(new CompressiblePorosityModel(
      plist_.sublist("compressible porosity model parameter")));

}


CompressiblePorosityEvaluator::CompressiblePorosityEvaluator(const CompressiblePorosityEvaluator& other) :
    SecondaryVariableFieldEvaluator(other),
    pres_key_(other.pres_key_),
    poro_key_(other.poro_key_),
    model_(other.model_) {}

Teuchos::RCP<FieldEvaluator>
CompressiblePorosityEvaluator::Clone() const {
  return Teuchos::rcp(new CompressiblePorosityEvaluator(*this));
}


// Required methods from SecondaryVariableFieldEvaluator
void CompressiblePorosityEvaluator::EvaluateField_(const Teuchos::Ptr<State>& S,
        const Teuchos::Ptr<CompositeVector>& result) {
  Teuchos::RCP<const CompositeVector> pres = S->GetFieldData(pres_key_);
  Teuchos::RCP<const CompositeVector> poro = S->GetFieldData(poro_key_);
  const double& patm = *S->GetScalarData("atmospheric_pressure");

  // evaluate the model
  for (CompositeVector::name_iterator comp=result->begin();
       comp!=result->end(); ++comp) {
    const Epetra_MultiVector& pres_v = *(pres->ViewComponent(*comp,false));
    const Epetra_MultiVector& poro_v = *(poro->ViewComponent(*comp,false));
    Epetra_MultiVector& result_v = *(result->ViewComponent(*comp,false));

    int count = result->size(*comp);
    for (int id=0; id!=count; ++id) {
      result_v[0][id] = model_->Porosity(poro_v[0][id], pres_v[0][id], patm);
    }
  }


}


void CompressiblePorosityEvaluator::EvaluateFieldPartialDerivative_(
    const Teuchos::Ptr<State>& S,
    Key wrt_key, const Teuchos::Ptr<CompositeVector>& result) {

  Teuchos::RCP<const CompositeVector> pres = S->GetFieldData(pres_key_);
  Teuchos::RCP<const CompositeVector> poro = S->GetFieldData(poro_key_);
  const double& patm = *S->GetScalarData("atmospheric_pressure");

  if (wrt_key == pres_key_) {
    // evaluate the model
    for (CompositeVector::name_iterator comp=result->begin();
         comp!=result->end(); ++comp) {
      const Epetra_MultiVector& pres_v = *(pres->ViewComponent(*comp,false));
      const Epetra_MultiVector& poro_v = *(poro->ViewComponent(*comp,false));
      Epetra_MultiVector& result_v = *(result->ViewComponent(*comp,false));

      int count = result->size(*comp);
      for (int id=0; id!=count; ++id) {
        result_v[0][id] = model_->DPorosityDPressure(poro_v[0][id], pres_v[0][id], patm);
      }
  }

  } else if (wrt_key == poro_key_) {
    // evaluate the model
    for (CompositeVector::name_iterator comp=result->begin();
         comp!=result->end(); ++comp) {
      const Epetra_MultiVector& pres_v = *(pres->ViewComponent(*comp,false));
      const Epetra_MultiVector& poro_v = *(poro->ViewComponent(*comp,false));
      Epetra_MultiVector& result_v = *(result->ViewComponent(*comp,false));

      int count = result->size(*comp);
      for (int id=0; id!=count; ++id) {
        result_v[0][id] = model_->DPorosityDBasePorosity(poro_v[0][id], pres_v[0][id], patm);
      }
    }

  } else {
    ASSERT(0);
  }
}



} //namespace
} //namespace
} //namespace
