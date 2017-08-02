/* -*-  mode: c++; indent-tabs-mode: nil -*- */

/*
  An elevation evaluator getting values from the columnar meshes.

  Authors: Ahmad Jan (jana@ornl.gov)
*/
#include <boost/algorithm/string/predicate.hpp>

#include "Mesh.hh"
#include "Mesh_MSTK.hh"
#include "Point.hh"
#include "elevation_evaluator_column.hh"

namespace Amanzi {
namespace Flow {

ElevationEvaluatorColumn::ElevationEvaluatorColumn(Teuchos::ParameterList& plist) :
  ElevationEvaluator(plist) 
{};

ElevationEvaluatorColumn::ElevationEvaluatorColumn(const ElevationEvaluatorColumn& other) :
  ElevationEvaluator(other),
  base_por_key_(other.base_por_key_)
{};

Teuchos::RCP<FieldEvaluator>
ElevationEvaluatorColumn::Clone() const {
  return Teuchos::rcp(new ElevationEvaluatorColumn(*this));
}

void ElevationEvaluatorColumn::EvaluateElevationAndSlope_(const Teuchos::Ptr<State>& S,
        const std::vector<Teuchos::Ptr<CompositeVector> >& results) {

  Teuchos::Ptr<CompositeVector> elev = results[0];
  Teuchos::Ptr<CompositeVector> slope = results[1];
  Epetra_MultiVector& elev_c = *elev->ViewComponent("cell", false);
  Epetra_MultiVector& slope_c = *slope->ViewComponent("cell", false);
 
  
  // Get the elevation and slope values from the domain mesh.
  Key domain = Keys::getDomain(my_keys_[0]);
 
  auto surface_mesh =
    Teuchos::rcp_static_cast<const AmanziMesh::Mesh_MSTK>(S->GetMesh(domain));
  
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  // Set the elevation on cells by getting the corresponding face and its
  // centroid.
  int ncells = elev_c.MyLength();
  std::vector<AmanziGeometry::Point> my_centroid;

  //get elevation on all cells first
  for (int c=0; c !=ncells; c++){
    std::stringstream my_name;
    int id = S->GetMesh("surface_star")->cell_map(false).GID(c);
    my_name << "column_" << id;
    
    int nfaces = S->GetMesh(my_name.str())->num_entities(AmanziMesh::FACE, AmanziMesh::USED);
    
    std::vector<AmanziGeometry::Point> coord; 
    S->GetMesh(my_name.str())->face_get_coordinates(nfaces-1, &coord);
    
    elev_c[0][c] = coord[0][2];
  }

  //Now get slope
  if (domain == "surface_star"){
    Teuchos::RCP<CompositeVector> elev_ngb = S->GetFieldData(my_keys_[0], S->GetField(my_keys_[0])->owner() );
    elev_ngb->ScatterMasterToGhosted("cell");
    const Epetra_MultiVector& elev_ngb_c = *elev_ngb->ViewComponent("cell",true);
    

    //get all cell centroids
    for (int c=0; c!=ncells; ++c) {
      std::stringstream my_name;
      int id = S->GetMesh("surface_star")->cell_map(true).GID(c);
      AmanziGeometry::Point P1 = S->GetMesh("surface_star")->cell_centroid(c);
      P1.set(P1[0], P1[1], elev_ngb_c[0][c]);
      my_centroid.push_back(P1);
    }
    
    //get neighboring cell ids
    for (int c=0; c!= ncells; c++){
      //int id = S->GetMesh("surface_star")->cell_map(false).GID(c);
      AmanziGeometry::Entity_ID_List nadj_cellids;
      S->GetMesh("surface_star")->cell_get_face_adj_cells(c, AmanziMesh::USED, &nadj_cellids);
      int nface_pcell = S->GetMesh("surface_star")->cell_get_num_faces(c);

      int ngb_cells = nadj_cellids.size();
      std::vector<AmanziGeometry::Point> ngb_centroids(ngb_cells);
          
      //get the neighboring cell's centroids
      for(unsigned i=0; i<ngb_cells; i++){
        AmanziGeometry::Point P2 = S->GetMesh("surface_star")->cell_centroid(nadj_cellids[i]);
        ngb_centroids[i].set(P2[0], P2[1], elev_ngb_c[0][nadj_cellids[i]]);
        //std::cout<<"NEIGHB: Centroid: "<<c<<" "<<my_centroid[c]<<" : "<<nadj_cellids[i]<<" "<< ngb_centroids[i]<<"\n";
      }

    
      std::stringstream my_name;
      int id = S->GetMesh("surface_star")->cell_map(false).GID(c);
      my_name << "column_" << id;
      
      std::vector<AmanziGeometry::Point> Normal;
      AmanziGeometry::Point N, PQ, PR, Nor_avg(3);
      
      if (ngb_cells >1){
	for (int i=0; i <ngb_cells-1; i++){
	  PQ = my_centroid[c] - ngb_centroids[i];
	  PR = my_centroid[c] - ngb_centroids[i+1];
	  N = PQ^PR;
          if (N[2] < 0)
            N *= -1.; // all normals upward
	  Normal.push_back(N);
	}
        int nfaces = S->GetMesh(my_name.str())->num_entities(AmanziMesh::FACE, AmanziMesh::USED);
	AmanziGeometry::Point fnor = S->GetMesh(my_name.str())->face_normal(nfaces-1);
	Nor_avg = (nface_pcell - Normal.size()) * fnor; 
	for (int i=0; i <Normal.size(); i++)
	  Nor_avg += Normal[i];
	
        Nor_avg /= nface_pcell;
	slope_c[0][c] = (std::sqrt(std::pow(Nor_avg[0],2) + std::pow(Nor_avg[1],2)))/ std::abs(Nor_avg[2]);
        
      }
      else{
	PQ = my_centroid[c] - ngb_centroids[0];
	slope_c[0][c] = std::abs(PQ[2]) / (std::sqrt(std::pow(PQ[0],2) + std::pow(PQ[1],2))); 
      }
    }
    
  }
  else{
    slope_c[0][0] = 0.0; // if domain is surface_column_*, slope is zero.
  }
    
}


void ElevationEvaluatorColumn::EnsureCompatibility(const Teuchos::Ptr<State>& S){
  
  Key domain = Keys::getDomain(my_keys_[0]);

  int ncells = S->GetMesh("surface_star")->num_entities(AmanziMesh::CELL, AmanziMesh::OWNED);
 
  if (domain == "surface_star") {
    for (int c =0; c < ncells; c++){
      std::stringstream name;
      int id = S->GetMesh("surface_star")->cell_map(false).GID(c);
      name << "column_"<< id;
      base_por_key_ = Keys::readKey(plist_, name.str(), "base porosity", "base_porosity");
      dependencies_.insert(base_por_key_);
    }
    } /*else {
    Errors::Message msg("ElevationEvaluatorColumn: this evaluator should be used for columnar meshes only.");
    Exceptions::amanzi_throw(msg);
    }  */ // keep it for a while!!

  ElevationEvaluator::EnsureCompatibility(S.ptr());
}
} //namespace
} //namespace
