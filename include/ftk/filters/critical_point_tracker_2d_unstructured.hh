#ifndef _FTK_CRITICAL_POINT_TRACKER_2D_UNSTRUCTURED_HH
#define _FTK_CRITICAL_POINT_TRACKER_2D_UNSTRUCTURED_HH

#include <ftk/ftk_config.hh>
#include <ftk/numeric/print.hh>
#include <ftk/numeric/cross_product.hh>
#include <ftk/numeric/vector_norm.hh>
#include <ftk/numeric/linear_interpolation.hh>
#include <ftk/numeric/bilinear_interpolation.hh>
#include <ftk/numeric/inverse_linear_interpolation_solver.hh>
#include <ftk/numeric/inverse_bilinear_interpolation_solver.hh>
#include <ftk/numeric/gradient.hh>
#include <ftk/numeric/adjugate.hh>
#include <ftk/numeric/symmetric_matrix.hh>
#include <ftk/numeric/critical_point_type.hh>
#include <ftk/numeric/critical_point_test.hh>
#include <ftk/numeric/fixed_point.hh>
#include <ftk/geometry/cc2curves.hh>
#include <ftk/geometry/curve2tube.hh>
#include <ftk/geometry/curve2vtk.hh>
#include <ftk/filters/critical_point_tracker_regular.hh>
#include <ftk/ndarray.hh>
#include <ftk/ndarray/grad.hh>
#include <ftk/mesh/regular_simplex_mesh.hh>
#include <ftk/external/diy/serialization.hpp>

#if FTK_HAVE_GMP
#include <gmpxx.h>
#endif

#if FTK_HAVE_VTK
#include <vtkUnsignedIntArray.h>
#include <vtkVertex.h>
#endif

namespace ftk {

typedef critical_point_t<3, double> critical_point_2dt_t;

struct critical_point_tracker_2d_unstructured : public critical_point_tracker_regular
{
  critical_point_tracker_2d_unstructured(const simplicial_unstructured_extruded_2d_mesh<>& m) : m1(m) {}

  virtual ~critical_point_tracker_2d_unstructured();

  void initialize();
  void finalize();
  void reset();

  void update_timestep();

protected:
  bool check_simplex(int, critical_point_2dt_t& cp);

protected:
  const simplicial_unstructured_extruded_2d_mesh& m1;
  
  std::map<int, critical_point_2dt_t> discrete_critical_points;
};

////////////////////////

template <typename T>
inline void critical_point_tracker_2d_unstructured::simplex_vectors(
    int n, int verts[], T v[][2])
{
  for (int i = 0; i < n; i ++) {
    const int iv = m1.flat_vertex_time(vert[i]) == current_timestep ? 0 : 1;
    const int k = m1.flat_vertex_id(vert[i]);
    for (int j = 0; j < 2; j ++) {
      v[i][j] = field_data_snapshots[iv].vector(j, vert[k]);
    }
  }
}

template <typename T>
inline void critical_point_tracker_2d_unstructured::simplex_coordinates(
    int n, int verts[], T x[][3])
{
  for (int i = 0; i < n; i ++)
    m1.get_coords(i, x[i]);
}

inline bool check_simplex(int, critical_point_2dt_t& cp)
{
#if FTK_HAVE_GMP
  typedef mpf_class fp_t;
#else
  typedef ftk::fixed_point<> fp_t;
#endif
  
  int tri[3];
  m1.get_simplex(2, i, tri); 

  double V[3][2], X[3][3];
  simplex_vectors<double>(3, tri, V);
  simplex_coordinates(3, tri, X);

  fp_t Vf[3][2];
  for (int k = 0; k < 3; k ++) 
    for (int j = 0; j < 2; j ++) {
      Vf[k][j] = V[k][j];

  bool succ = ftk::robust_critical_point_in_simplex2(Vf, tri);
  if (!succ) return;

  // ftk::print3x2("V", V);
  double mu[3], x[3];
  bool succ2 = ftk::inverse_lerp_s2v2(V, mu);
  // if (!succ2) return;
  ftk::lerp_s2v3(X, mu, x);

#if 0
  double Js[3][2][2], H[2][2];
  for (int k = 0; k < 3; k ++) {
    int t = tri[k] >= m.n(0) ? 1 : 0;
    int v = tri[k] % m.n(0);
    for (int j = 0; j < 2; j ++)
      for (int i = 0; i < 2; i ++)
        Js[k][j][i] = J[t](i, j, v); 
  }
  ftk::lerp_s2m2x2(Js, mu, H);
  // ftk::print2x2("H", H);
  const int type = ftk::critical_point_type_2d(H, true);
#else
  const int type = 0;
#endif

  fprintf(stderr, "mu=%f, %f, %f, x=%f, %f, %f, type=%d\n", 
      mu[0], mu[1], mu[2], x[0], x[1], x[2], type);
  // cps.push_back(x[0]);
  // cps.push_back(x[1]);
  // cps.push_back(x[2]);
  // types.push_back(type);
}

inline void critical_point_tracker_2d_unstructured::update_timestep()
{
  if (comm.rank() == 0) fprintf(stderr, "current_timestep=%d\n", current_timestep);

  auto func = [&](int i) {
    critical_point_2dt_t cp;
    if (check_simplex(i, cp)) {
      std::lock_guard<std::mutex> guard(mutex);
      discrete_critical_points[i] = cp;
    }
  };

  m.element_for_ordinal(2, current_timestep, func);
  if (field_data_snapshots.size() >= 2)
    m.element_for_interval(2, current_timestep, func);
}

}

#endif
