/* -*-  mode: c++; indent-tabs-mode: nil -*- */

// -----------------------------------------------------------------------------
// ATS
//
// License: see $ATS_DIR/COPYRIGHT
// Author: Ethan Coon (ecoon@lanl.gov)
//
// Scheme for taking coefficients for div-grad operators from cells to
// faces.
// -----------------------------------------------------------------------------

#include "Mesh.hh"
#include "CompositeVector.hh"
#include "State.hh"
#include "Debugger.hh"
#include "VerboseObject.hh"
#include "upwind_total_flux.hh"
#include "Epetra_IntVector.h"

namespace Amanzi {
namespace Operators {

UpwindTotalFlux::UpwindTotalFlux(std::string pkname,
                                 std::string cell_coef,
                                 std::string face_coef,
                                 std::string flux,
                                 double flux_eps) :
    pkname_(pkname),
    cell_coef_(cell_coef),
    face_coef_(face_coef),
    flux_(flux),
    flux_eps_(flux_eps) {};


void UpwindTotalFlux::Update(const Teuchos::Ptr<State>& S,
                             const Teuchos::Ptr<Debugger>& db) {

  Teuchos::RCP<const CompositeVector> cell = S->GetFieldData(cell_coef_);
  Teuchos::RCP<const CompositeVector> flux = S->GetFieldData(flux_);
  Teuchos::RCP<CompositeVector> face = S->GetFieldData(face_coef_, pkname_);
  CalculateCoefficientsOnFaces(*cell, *flux, face.ptr(), db);
};


void UpwindTotalFlux::CalculateCoefficientsOnFaces(
        const CompositeVector& cell_coef,
        const CompositeVector& flux,
        const Teuchos::Ptr<CompositeVector>& face_coef,
        const Teuchos::Ptr<Debugger>& db) {
  Teuchos::RCP<const AmanziMesh::Mesh> mesh = face_coef->Mesh();

  // initialize the face coefficients
  if (face_coef->HasComponent("cell")) {
    face_coef->ViewComponent("cell",true)->PutScalar(1.0);
  }

  // communicate needed ghost values
  cell_coef.ScatterMasterToGhosted("cell");

  // pull out vectors
  const Epetra_MultiVector& flux_v = *flux.ViewComponent("face",false);
  Epetra_MultiVector& coef_faces = *face_coef->ViewComponent("face",false);
  const Epetra_MultiVector& coef_cells = *cell_coef.ViewComponent("cell",true);

  // Identify upwind/downwind cells for each local face.  Note upwind/downwind
  // may be a ghost cell.
  Epetra_IntVector upwind_cell(*face_coef->ComponentMap("face",true));
  upwind_cell.PutValue(-1);
  Epetra_IntVector downwind_cell(*face_coef->ComponentMap("face",true));
  downwind_cell.PutValue(-1);

  AmanziMesh::Entity_ID_List faces;
  std::vector<int> fdirs;
  int nfaces_local = flux.size("face",false);

  int ncells = cell_coef.size("cell",true);
  for (int c=0; c!=ncells; ++c) {
    mesh->cell_get_faces_and_dirs(c, &faces, &fdirs);   

    for (unsigned int n=0; n!=faces.size(); ++n) {
      int f = faces[n];

      if  (face_coef->HasComponent("cell")) {        
        (*face_coef->ViewComponent("cell",true))[0][c] = coef_cells[0][c];
      }

      if (f < nfaces_local) {
        if (flux_v[0][f] * fdirs[n] > 0) {
          upwind_cell[f] = c;
        } else if (flux_v[0][f] * fdirs[n] < 0) {
          downwind_cell[f] = c;
        } else {
          // We don't care, but we have to get one into upwind and the other
          // into downwind.
          if (upwind_cell[f] == -1) {
            upwind_cell[f] = c;
          } else {
            downwind_cell[f] = c;
          }
        }
      }
    }
  }

  // Determine the face coefficient of local faces.
  // These parameters may be key to a smooth convergence rate near zero flux.
  //  double flow_eps_factor = 1.;
  //  double min_flow_eps = 1.e-8;
  double coefs[2];

  int nfaces = face_coef->size("face",false);
  for (int f=0; f!=nfaces; ++f) {
    int uw = upwind_cell[f];
    int dw = downwind_cell[f];
    AMANZI_ASSERT(!((uw == -1) && (dw == -1)));

    // Teuchos::RCP<VerboseObject> dcvo_dw = Teuchos::null;
    // Teuchos::RCP<VerboseObject> dcvo_uw = Teuchos::null;

    // if (uw >= 0)
    //   dcvo_uw = db->GetVerboseObject(uw, face_coef->Mesh()->get_comm()->MyPID());
    // if (dw >= 0)
    //   dcvo_dw = db->GetVerboseObject(dw, face_coef->Mesh()->get_comm()->MyPID());

    // uw coef
    if (uw == -1) {
      coefs[0] = coef_faces[0][f];
    } else {
      coefs[0] = coef_cells[0][uw];
    }

    // dw coef
    if (dw == -1) {
      coefs[1] = coef_faces[0][f];
    } else {
      coefs[1] = coef_cells[0][dw];
    }

    // Determine the size of the overlap region, a smooth transition region
    // near zero flux
    // double flow_eps = std::max(( 1.0 - std::abs(coefs[0] - coefs[1]) )
    //         * std::sqrt(coefs[0] * coefs[1]) * flow_eps_factor,
    //         min_flow_eps);
    double flow_eps = flux_eps_;

    // Determine the coefficient
    if (std::abs(flux_v[0][f]) >= flow_eps) {
      coef_faces[0][f] = coefs[0];
    } else {
      // Parameterization of a linear scaling between upwind and downwind.
      double param = std::abs(flux_v[0][f]) / (2*flow_eps) + 0.5;
      if (!(param >= 0.5) || !(param <= 1.0)) {
        std::cout << "BAD FLUX! on face " << f << std::endl;
        std::cout << "  flux = " << flux_v[0][f] << std::endl;
        std::cout << "  param = " << param << std::endl;
        std::cout << "  flow_eps = " << flow_eps << std::endl;
      }

      // if (dcvo_uw != Teuchos::null)
      //   *dcvo_uw->os() << "  AVG param = " << param << std::endl;
      // if (dcvo_dw != Teuchos::null)
      //   *dcvo_dw->os() << "  AVG param = " << param << std::endl;

      AMANZI_ASSERT(param >= 0.5);
      AMANZI_ASSERT(param <= 1.0);

      coef_faces[0][f] = coefs[0] * param + coefs[1] * (1. - param);
    }
  }
};


void
UpwindTotalFlux::UpdateDerivatives(const Teuchos::Ptr<State>& S,
                                        std::string potential_key, 
                                        const CompositeVector& dconductivity,
                                        const std::vector<int>& bc_markers,
                                        const std::vector<double>& bc_values,
                                        std::vector<Teuchos::RCP<Teuchos::SerialDenseMatrix<int, double> > >* Jpp_faces) const {
  // Grab derivatives
  dconductivity.ScatterMasterToGhosted("cell");
  const Epetra_MultiVector& dcell_v = *dconductivity.ViewComponent("cell",true);

  // Grab potential
  Teuchos::RCP<const CompositeVector> pres = S->GetFieldData(potential_key);
  pres->ScatterMasterToGhosted("cell");
  const Epetra_MultiVector& pres_v = *pres->ViewComponent("cell",true);

  // Grab flux direction
  const Epetra_MultiVector& flux_v = *S->GetFieldData(flux_)->ViewComponent("face",false);

  // Grab mesh and allocate space
  Teuchos::RCP<const AmanziMesh::Mesh> mesh = dconductivity.Mesh();
  unsigned int nfaces_owned = mesh->num_entities(AmanziMesh::FACE,AmanziMesh::Parallel_type::OWNED);
  Jpp_faces->resize(nfaces_owned);

  // workspace
  double dK_dp[2];
  double p[2];
  

  // Identify upwind/downwind cells for each local face.  Note upwind/downwind
  // may be a ghost cell.
  Epetra_IntVector upwind_cell(mesh->face_map(true));
  upwind_cell.PutValue(-1);
  Epetra_IntVector downwind_cell(mesh->face_map(true));
  downwind_cell.PutValue(-1);

  AmanziMesh::Entity_ID_List faces;
  std::vector<int> fdirs;

  int ncells = dcell_v.MyLength();
  for (int c=0; c!=ncells; ++c) {
    mesh->cell_get_faces_and_dirs(c, &faces, &fdirs);

    for (unsigned int n=0; n!=faces.size(); ++n) {
      int f = faces[n];

      if (f < nfaces_owned) {
        if (flux_v[0][f] * fdirs[n] > 0) {
          upwind_cell[f] = c;
        } else if (flux_v[0][f] * fdirs[n] < 0) {
          downwind_cell[f] = c;
        } else {
          // We don't care, but we have to get one into upwind and the other
          // into downwind.
          if (upwind_cell[f] == -1) {
            upwind_cell[f] = c;
          } else {
            downwind_cell[f] = c;
          }
        }
      }
    }
  }


  for (unsigned int f=0; f!=nfaces_owned; ++f) {
    int uw = upwind_cell[f];
    int dw = downwind_cell[f];
    AMANZI_ASSERT(!((uw == -1) && (dw == -1)));

    AmanziMesh::Entity_ID_List cells;
    mesh->face_get_cells(f, AmanziMesh::Parallel_type::ALL, &cells);
    int mcells = cells.size();

    // uw coef
    if (uw == -1) {
      // boundary, upwind is the boundary
      if (std::abs(flux_v[0][f]) >= flux_eps_) {
        // flux coming from boundary, derivs are zero
        dK_dp[0] = 0.;
      } else {
        // Parameterization of a linear scaling between upwind and downwind.
        double param = std::abs(flux_v[0][f]) / (2*flux_eps_) + 0.5;
        
        // ignoring dparam_dp... not sure how we would include that
        dK_dp[0] = (1.-param) * dcell_v[0][dw];
      }

    } else if (dw == -1) {
      // boundary, upwind is the cell
      if (std::abs(flux_v[0][f]) >= flux_eps_) {
        dK_dp[0] = dcell_v[0][uw];
      } else {
        double param = std::abs(flux_v[0][f]) / (2*flux_eps_) + 0.5;
        dK_dp[0] = param * dcell_v[0][uw];
      }

    } else {
      // non-boundary
      if (std::abs(flux_v[0][f]) >= flux_eps_) {
        if (uw == cells[0]) {
          dK_dp[0] = dcell_v[0][uw];
          dK_dp[1] = 0.;
        } else {
          dK_dp[1] = dcell_v[0][uw];
          dK_dp[0] = 0.;
        }
      } else {
        double param = std::abs(flux_v[0][f]) / (2*flux_eps_) + 0.5;
        if (uw == cells[0]) {
          dK_dp[0] = param * dcell_v[0][uw];
          dK_dp[1] = (1-param) * dcell_v[0][dw];
        } else {
          dK_dp[1] = param * dcell_v[0][uw];
          dK_dp[0] = (1-param) * dcell_v[0][dw];
        }
      }
    }

    // create the local matrix
    Teuchos::RCP<Teuchos::SerialDenseMatrix<int, double> > Jpp =
      Teuchos::rcp(new Teuchos::SerialDenseMatrix<int, double>(mcells, mcells));
    (*Jpp_faces)[f] = Jpp;

    if (mcells == 1) {
      if (bc_markers[f] == Operators::OPERATOR_BC_DIRICHLET) {
        // determine flux
        p[0] = pres_v[0][cells[0]];
        p[1] = bc_values[f];
        double dp = p[0] - p[1];

        (*Jpp)(0,0) = dp * mesh->face_area(f) * dK_dp[0];
      } else {
        (*Jpp)(0,0) = 0.;
      }

    } else {
      p[0] = pres_v[0][cells[0]];
      p[1] = pres_v[0][cells[1]];

      (*Jpp)(0,0) = (p[0] - p[1]) * mesh->face_area(f) * dK_dp[0];
      (*Jpp)(0,1) = (p[0] - p[1]) * mesh->face_area(f) * dK_dp[1];
      (*Jpp)(1,0) = -(*Jpp)(0,0);
      (*Jpp)(1,1) = -(*Jpp)(0,1);
    }
  }
}
} //namespace
} //namespace
