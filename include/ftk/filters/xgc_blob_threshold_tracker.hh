#ifndef _FTK_XGC_BLOB_CCL_TRACKER_HH
#define _FTK_XGC_BLOB_CCL_TRACKER_HH

#include <ftk/ftk_config.hh>
#include <ftk/filters/tracker.hh>
#include <ftk/filters/xgc_tracker.hh>
#include <ftk/tracking_graph/tracking_graph.hh>
#include <ftk/basic/duf.hh>

namespace ftk {

struct xgc_blob_threshold_tracker : public xgc_tracker {
  xgc_blob_threshold_tracker(diy::mpi::communicator comm, 
      std::shared_ptr<simplicial_xgc_3d_mesh<>> m3) : xgc_tracker(comm, m3) {}
  virtual ~xgc_blob_threshold_tracker() {}
  
  void set_threshold(double t) { threshold = t; }

  int cpdims() const { return 0; }
  
  void initialize() {}
  void update() {}
  void finalize();

  void update_timestep();

  void push_field_data_snapshot(const ndarray<double> &scalar);

public:
  ndarray<int> get_sliced(int t) const;
  void write_sliced(int t, const std::string& pattern) const;
#if FTK_HAVE_VTK
  vtkSmartPointer<vtkUnstructuredGrid> sliced_to_vtu_slices(int t) const;
  vtkSmartPointer<vtkUnstructuredGrid> sliced_to_vtu_solid(int t) const;
  // vtkSmartPointer<vtkUnstructuredGrid> sliced_to_vtu_partial_solid(int t) const;
#endif

protected:
  double threshold = 0.0;
  duf<int> uf;
};

/////

inline void xgc_blob_threshold_tracker::push_field_data_snapshot(const ndarray<double> &scalar)
{
  ndarray<double> grad, J; // no grad or jacobian needed
  xgc_tracker::push_field_data_snapshot(scalar, grad, J);
}

inline void xgc_blob_threshold_tracker::update_timestep()
{
  if (comm.rank() == 0) fprintf(stderr, "current_timestep=%d\n", current_timestep);

  const int m3n0 = m3->n(0);
  const auto &scalar = field_data_snapshots[0].scalar;
  for (int i = 0; i < m3n0; i ++) {
    if (scalar[i] < threshold) continue;

    for (const auto j : m3->get_vertex_edge_vertex_nextnodes(i)) {
      if (scalar[j] >= threshold) {
        int ei = i + current_timestep * m3n0,
            ej = j + current_timestep * m3n0;
        uf.unite(ei, ej);
      }
    }
  }

  if (field_data_snapshots.size() >= 2) {
    const auto &scalar1 = field_data_snapshots[1].scalar;
    for (int i = 0; i < m3n0; i ++) {
      if (scalar[i] < threshold || scalar1[i] < threshold) continue;
      else
        uf.unite(i + current_timestep * m3n0, i + (current_timestep+1) * m3n0);
    }
  }
  
  fprintf(stderr, "%zu\n", uf.size());
}

inline void xgc_blob_threshold_tracker::finalize()
{
}

void xgc_blob_threshold_tracker::write_sliced(int t, const std::string& pattern) const
{
  const std::string filename = ndarray_writer<double>::filename(pattern, t);

#if FTK_HAVE_VTK
  vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkXMLUnstructuredGridWriter::New();
  writer->SetFileName(filename.c_str());
  // writer->SetInputData( sliced_to_vtu_slices(t) );
  writer->SetInputData( sliced_to_vtu_solid(t) );
  writer->Write();
#else
  fatal("FTK not compiled with VTK.");
#endif
}

ndarray<int> xgc_blob_threshold_tracker::get_sliced(int t) const
{
  ndarray<int> array({m3->n(0)}, -1);
  for (int i = 0; i < m3->n(0); i ++) {
    const int e = i + t * m3->n(0);
    if (uf.exists(e)) 
      array[i] = uf.find(e);
  }
  return array;
}

#if FTK_HAVE_VTK
vtkSmartPointer<vtkUnstructuredGrid> xgc_blob_threshold_tracker::sliced_to_vtu_slices(int t) const
{
  auto grid = m3->to_vtu_slices();
  grid->GetPointData()->AddArray( get_sliced(t).to_vtk_data_array() );
  return grid;
}

vtkSmartPointer<vtkUnstructuredGrid> xgc_blob_threshold_tracker::sliced_to_vtu_solid(int t) const
{
  auto grid = m3->to_vtu_solid();
  grid->GetPointData()->AddArray( get_sliced(t).to_vtk_data_array() );
  return grid;
}

// vtkSmartPointer<vtkUnstructuredGrid> xgc_blob_threshold_tracker::sliced_to_vtu_partial_solid(int t) const
// {
// }
#endif

}

#endif