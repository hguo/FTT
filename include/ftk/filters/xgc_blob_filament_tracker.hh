#ifndef _FTK_XGC_BLOB_FILAMENT_TRACKER_HH
#define _FTK_XGC_BLOB_FILAMENT_TRACKER_HH

#include <ftk/ftk_config.hh>
#include <ftk/algorithms/cca.hh>
#include <ftk/filters/tracker.hh>
#include <ftk/filters/feature_point.hh>
#include <ftk/filters/feature_surface.hh>
#include <ftk/filters/feature_volume.hh>
#include <ftk/geometry/points2vtk.hh>
#include <ftk/geometry/cc2curves.hh>
#include <ftk/external/diy/serialization.hpp>
#include <ftk/external/diy-ext/gather.hh>
#include <ftk/io/tdgl.hh>
#include <iomanip>

namespace ftk {
  
struct xgc_blob_filament_tracker : public virtual tracker {
  xgc_blob_filament_tracker(diy::mpi::communicator comm, 
      std::shared_ptr<simplicial_unstructured_2d_mesh<>> m2, 
      int nphi_, int iphi_);

  int cpdims() const { return 3; }
 
  void set_nphi_iphi(int nphi, int iphi);

  void initialize() {}
  void reset() {
    field_data_snapshots.clear();
  }
  void update() {}
  void finalize();
 
public:
  static std::shared_ptr<xgc_blob_filament_tracker> from_augmented_mesh_file(diy::mpi::communicator comm, const std::string& filename);
  void to_augmented_mesh_file(const std::string& filename);
 
  std::shared_ptr<simplicial_unstructured_2d_mesh<>> get_m2() { return m2; }

protected:
  xgc_blob_filament_tracker(diy::mpi::communicator comm) : tracker(comm) {}

public:
  void update_timestep();
  bool advance_timestep();

public:
  void push_field_data_snapshot(const ndarray<double> &scalar);
  void push_field_data_snapshot(
      const ndarray<double> &scalar, 
      const ndarray<double> &vector,
      const ndarray<double> &jacobian);
  
  bool pop_field_data_snapshot();
 
protected:
  bool check_simplex(int, feature_point_t& cp);
 
  template <int n, typename T>
  void simplex_values(
      const int i,
      int verts[n],
      int t[n],
      int p[n],
      T rzpt[n][4], 
      T f[n], // scalars
      T v[n][2], // vectors
      T j[n][2][2]); // jacobians

protected:
  struct field_data_snapshot_t {
    ndarray<double> scalar, vector, jacobian;
  };
  std::deque<field_data_snapshot_t> field_data_snapshots;

  int nphi, iphi;

  std::shared_ptr<simplicial_unstructured_2d_mesh<>> m2;
  std::shared_ptr<simplicial_unstructured_3d_mesh<>> m3;
  std::shared_ptr<simplicial_unstructured_extruded_3d_mesh<>> m4;

  // const simplicial_unstructured_2d_mesh<>& m2;
  // const simplicial_unstructured_3d_mesh<> m3;
  // const simplicial_unstructured_extruded_3d_mesh<> m4;
 
public:
  void write_critical_points_vtp(const std::string& filename) const;

#if FTK_HAVE_VTK
  vtkSmartPointer<vtkPolyData> get_critical_points_vtp() const;
#endif

protected:
  std::map<int, feature_point_t> discrete_critical_points;
};

/////
  
xgc_blob_filament_tracker::xgc_blob_filament_tracker(
    diy::mpi::communicator comm, 
    std::shared_ptr<simplicial_unstructured_2d_mesh<>> m2_, 
    int nphi_, int iphi_) :
  tracker(comm),
  m2(m2_), nphi(nphi_), iphi(iphi_),
  m3(ftk::simplicial_unstructured_3d_mesh<>::from_xgc_mesh(*m2_, nphi_, iphi_)),
  m4(new ftk::simplicial_unstructured_extruded_3d_mesh<>(*m3))
{

}

inline bool xgc_blob_filament_tracker::advance_timestep()
{
  update_timestep();
  pop_field_data_snapshot();

  current_timestep ++;
  return field_data_snapshots.size() > 0;
}

inline bool xgc_blob_filament_tracker::pop_field_data_snapshot()
{
  if (field_data_snapshots.size() > 0) {
    field_data_snapshots.pop_front();
    return true;
  } else return false;
}

inline void xgc_blob_filament_tracker::push_field_data_snapshot(
      const ndarray<double> &scalar)
{
  ndarray<double> F, G, J;

  F.reshape(scalar);
  G.reshape(2, scalar.dim(0), scalar.dim(1));
  J.reshape(2, 2, scalar.dim(0), scalar.dim(1));
  for (size_t i = 0; i < nphi; i ++) {
    // fprintf(stderr, "smoothing slice %zu\n", i);
    ftk::ndarray<double> f, grad, j;
    auto slice = scalar.slice_time(i);
    m2->smooth_scalar_gradient_jacobian(slice, 0.03/*FIXME*/, f, grad, j);
    for (size_t k = 0; k < m2->n(0); k ++) {
      F(k, i) = f(k);
      G(0, k, i) = grad(0, k);
      G(1, k, i) = grad(1, k);
      J(0, 0, k, i) = j(0, 0, k);
      J(1, 0, k, i) = j(1, 0, k);
      J(1, 1, k, i) = j(1, 1, k);
      J(0, 1, k, i) = j(0, 1, k);
    }
  }

  push_field_data_snapshot(F, G, J);
}
  
inline void xgc_blob_filament_tracker::push_field_data_snapshot(
      const ndarray<double> &scalar, 
      const ndarray<double> &vector,
      const ndarray<double> &jacobian)
{
  field_data_snapshot_t snapshot;
  snapshot.scalar = scalar;
  snapshot.vector = vector;
  snapshot.jacobian = jacobian;

  field_data_snapshots.emplace_back(snapshot);
}

inline void xgc_blob_filament_tracker::update_timestep()
{
  if (comm.rank() == 0) fprintf(stderr, "current_timestep=%d\n", current_timestep);

  auto func = [&](int i) {
    feature_point_t cp;
    if (check_simplex(i, cp)) {
      std::lock_guard<std::mutex> guard(mutex);
        discrete_critical_points[i] = cp;
    }
  };

  m4->element_for_ordinal(2, current_timestep, func, xl, nthreads, enable_set_affinity);
  if (field_data_snapshots.size() >= 2)
    m4->element_for_interval(2, current_timestep, func, xl, nthreads, enable_set_affinity);
}

inline bool xgc_blob_filament_tracker::check_simplex(int i, feature_point_t& cp)
{
  int tri[3], t[3], p[3];
  double rzpt[3][4], f[3], v[3][2], j[3][2][2];
  long long vf[3][2];
  const long long factor = 1 << 15; // WIP

  simplex_values<3, double>(i, tri, t, p, rzpt, f, v, j);
  // print3x2("rz", rz);
  // print3x2("v", v);

  for (int k = 0; k < 3; k ++) 
    for (int l = 0; l < 2; l ++) 
      vf[k][l] = factor * v[k][l];

  bool succ = ftk::robust_critical_point_in_simplex2(vf, tri);
  if (!succ) return false;

  double mu[3], x[4];
  bool succ2 = ftk::inverse_lerp_s2v2(v, mu);
  ftk::clamp_barycentric<3>(mu);

  ftk::lerp_s2v4(rzpt, mu, x);
  for (int k = 0; k < 3; k ++)
    cp.x[k] = x[k];
  cp.t = x[3];

  cp.scalar[0] = f[0] * mu[0] + f[1] * mu[1] + f[2] * mu[2];

  double h[2][2];
  ftk::lerp_s2m2x2(j, mu, h);
  cp.type = ftk::critical_point_type_2d(h, true);

  cp.tag = i;
  cp.ordinal = m4->is_ordinal(2, i);
  cp.timestep = current_timestep;

  // fprintf(stderr, "succ, mu=%f, %f, %f, x=%f, %f, %f, %f, timestep=%d, type=%d, t=%d, %d, %d\n", 
  //     mu[0], mu[1], mu[2], 
  //     x[0], x[1], x[2], x[3], cp.timestep, cp.type, 
  //     t[0], t[1], t[2]);
  
  return true;
}

void xgc_blob_filament_tracker::finalize()
{
  if (is_root_proc()) {
    fprintf(stderr, "ncps=%zu\n", discrete_critical_points.size());
  }
}

template<int n, typename T>
void xgc_blob_filament_tracker::simplex_values(
    const int i, 
    int verts[n],
    int t[n], // time
    int p[n], // poloidal
    T rzpt[n][4], 
    T f[n],
    T v[n][2],
    T j[n][2][2])
{
  m4->get_simplex(n-1, i, verts);
  for (int k = 0; k < n; k ++) {
    t[k] = verts[k] / m4->n(0); // m4->flat_vertex_time(verts[k]);
    const int v3 = verts[k] % m3->n(0);
    const int v2 = v3 % m2->n(0);
    p[k] = v3 / m2->n(0); // poloidal plane

    m2->get_coords(v2, rzpt[k]);
    rzpt[k][2] = p[k];
    rzpt[k][3] = t[k];

    const int iv = (t[k] == current_timestep) ? 0 : 1; 
    const auto &data = field_data_snapshots[iv];
    
    f[k] = data.scalar[v3];

    v[k][0] = data.vector[v3*2];
    v[k][1] = data.vector[v3*2+1];

    j[k][0][0] = data.jacobian[v3*4];
    j[k][0][1] = data.jacobian[v3*4+1];
    j[k][1][0] = data.jacobian[v3*4+2];
    j[k][1][1] = data.jacobian[v3*4+3];
  }

  bool b0 = false, b1 = false;
  for (int k = 0; k < n; k ++) {
    if (p[k] == 0)
      b0 = true;
    else if (p[k] == nphi-1)
      b1 = true;
  }
  if (b0 && b1) { // periodical
    // fprintf(stderr, "periodical.\n");
    for (int k = 0; k < n; k ++)
      if (p[k] == 0)
        rzpt[k][2] += nphi;
  }
}

inline /*static*/ std::shared_ptr<xgc_blob_filament_tracker>
xgc_blob_filament_tracker::from_augmented_mesh_file(
    diy::mpi::communicator comm, const std::string &filename)
{
  FILE *fp = fopen(filename.c_str(), "rb");
  assert(fp);
  diy::detail::FileBuffer bb(fp);

  std::shared_ptr<xgc_blob_filament_tracker> tracker(
      new xgc_blob_filament_tracker(comm));

  diy::load(bb, tracker->nphi);
  diy::load(bb, tracker->iphi);

  tracker->m2.reset(new simplicial_unstructured_2d_mesh<>);
  diy::load(bb, *tracker->m2);

  tracker->m3.reset(new simplicial_unstructured_3d_mesh<>);
  diy::load(bb, *tracker->m3);

  tracker->m4.reset(new simplicial_unstructured_extruded_3d_mesh<>(*tracker->m3));

  fclose(fp);
  return tracker;
}

inline void xgc_blob_filament_tracker::to_augmented_mesh_file(const std::string& filename)
{
  if (!is_root_proc()) return;

  FILE *fp = fopen(filename.c_str(), "wb");
  assert(fp);
  diy::detail::FileBuffer bb(fp);

  diy::save(bb, nphi);
  diy::save(bb, iphi);
  diy::save(bb, *m2);
  diy::save(bb, *m3);

  fclose(fp);
}

inline void xgc_blob_filament_tracker::write_critical_points_vtp(const std::string& filename) const
{
#if FTK_HAVE_VTK
  if (comm.rank() == get_root_proc()) {
    auto poly = get_critical_points_vtp();
    write_polydata(filename, poly);
  }
#else
  fatal("FTK not compiled with VTK.");
#endif
}

#if FTK_HAVE_VTK
inline vtkSmartPointer<vtkPolyData> xgc_blob_filament_tracker::get_critical_points_vtp() const
{
  vtkSmartPointer<vtkPolyData> poly = vtkPolyData::New();
  vtkSmartPointer<vtkPoints> points = vtkPoints::New();
  vtkSmartPointer<vtkCellArray> vertices = vtkCellArray::New();
  
  vtkIdType pid[1];
  
  // const auto critical_points = get_critical_points();
  for (const auto &kv : discrete_critical_points) {
    const auto &cp = kv.second;
    double p[3] = {cp.x[0], cp.x[1], cp.x[2]}; // TODO: time
    pid[0] = points->InsertNextPoint(p);
    vertices->InsertNextCell(1, pid);
  }

  poly->SetPoints(points);
  poly->SetVerts(vertices);

  return poly;
}
#endif

} // namespace ftk

#endif
