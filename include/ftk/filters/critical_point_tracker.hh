#ifndef _FTK_CRITICAL_POINT_TRACKER_HH
#define _FTK_CRITICAL_POINT_TRACKER_HH

#include <ftk/ftk_config.hh>
#include <ftk/filters/filter.hh>
#include <ftk/filters/critical_point.hh>
#include <ftk/geometry/points2vtk.hh>
#include <ftk/external/diy/serialization.hpp>
#include <ftk/external/diy-ext/gather.hh>

#if FTK_HAVE_VTK
#include <vtkUnsignedIntArray.h>
#include <vtkVertex.h>
#include <vtkSmartPointer.h>
#endif

namespace ftk {

struct critical_point_tracker : public filter {
  critical_point_tracker() {}

  virtual void update() {}; // TODO
  void reset() {
    field_data_snapshots.clear();
    traced_critical_points.clear();
  }
  
  virtual int cpdims() const = 0;

  void set_type_filter(unsigned int);

  void set_num_scalar_components(int);
  int get_num_scalar_components() const {return num_scalar_components;}

public: // outputs
  const std::vector<std::vector<critical_point_t>>& get_traced_critical_points() {return traced_critical_points;}
  virtual std::vector<critical_point_t> get_critical_points() const = 0;

  void write_traced_critical_points_binary(const std::string& filename) const;
  void write_traced_critical_points_text(std::ostream& os) const;
  void write_traced_critical_points_text(const std::string& filename) const;
  void write_traced_critical_points_vtk(const std::string& filename) const;
#if FTK_HAVE_VTK
  vtkSmartPointer<vtkPolyData> get_traced_critical_points_vtk() const;
#endif

  void write_critical_points_binary(const std::string& filename) const;
  void write_critical_points_text(std::ostream& os) const;
  void write_critical_points_text(const std::string& filename) const;
  void write_critical_points_vtk(const std::string& filename) const;
#if FTK_HAVE_VTK
  vtkSmartPointer<vtkPolyData> get_critical_points_vtk() const;
#endif

public: // inputs
  bool pop_field_data_snapshot();
  virtual void push_field_data_snapshot(
      const ndarray<double> &scalar, 
      const ndarray<double> &vector,
      const ndarray<double> &jacobian);
  virtual void push_scalar_field_snapshot(const ndarray<double> &scalar); // push scalar only

  virtual void push_field_data_spacetime(
      const ndarray<double> &scalars, 
      const ndarray<double> &vectors,
      const ndarray<double> &jacobians);
  void push_scalar_field_spacetime(const ndarray<double>& scalars);

protected:
  struct field_data_snapshot_t {
    ndarray<double> scalar, vector, jacobian;
  };

  std::deque<field_data_snapshot_t> field_data_snapshots;
  
  std::vector<std::vector<critical_point_t>> traced_critical_points;

  // type filter
  bool use_type_filter = false;
  unsigned int type_filter = 0;

  // scalar components
  int num_scalar_components = 1;
};

///////

inline void critical_point_tracker::push_field_data_snapshot(
    const ndarray<double>& scalar,
    const ndarray<double>& vector,
    const ndarray<double>& jacobian)
{
  field_data_snapshot_t snapshot;
  snapshot.scalar = scalar;
  snapshot.vector = vector;
  snapshot.jacobian = jacobian;

  field_data_snapshots.emplace_back(snapshot);
}

inline void critical_point_tracker::push_scalar_field_snapshot(const ndarray<double>& scalar)
{
  field_data_snapshot_t snapshot;
  snapshot.scalar = scalar;

  field_data_snapshots.emplace_back(snapshot);
}

inline void critical_point_tracker::push_field_data_spacetime(
    const ndarray<double>& scalars,
    const ndarray<double>& vectors,
    const ndarray<double>& jacobians)
{
  for (size_t t = 0; t < scalars.shape(scalars.nd()-1); t ++) {
    auto scalar = scalars.slice_time(t);
    auto vector = vectors.slice_time(t);
    auto jacobian = jacobians.slice_time(t);

    push_field_data_snapshot(scalar, vector, jacobian);
  }
}

inline void critical_point_tracker::push_scalar_field_spacetime(const ndarray<double>& scalars)
{
  for (size_t t = 0; t < scalars.shape(scalars.nd()-1); t ++)
    push_scalar_field_snapshot( scalars.slice_time(t) );
}


inline bool critical_point_tracker::pop_field_data_snapshot()
{
  if (field_data_snapshots.size() > 0) {
    field_data_snapshots.pop_front();
    return true;
  } else return false;
}

//////
#if FTK_HAVE_VTK
inline void critical_point_tracker::write_traced_critical_points_vtk(const std::string& filename) const
{
  if (comm.rank() == get_root_proc()) {
    auto poly = get_traced_critical_points_vtk();
    write_vtp(filename, poly);
  }
}

inline void critical_point_tracker::write_critical_points_vtk(const std::string& filename) const
{
  if (comm.rank() == get_root_proc()) {
    auto poly = get_critical_points_vtk();
    write_vtp(filename, poly);
  }
}

inline vtkSmartPointer<vtkPolyData> critical_point_tracker::get_critical_points_vtk() const
{
  vtkSmartPointer<vtkPolyData> polyData = vtkPolyData::New();
  vtkSmartPointer<vtkPoints> points = vtkPoints::New();
  vtkSmartPointer<vtkCellArray> vertices = vtkCellArray::New();
  
  vtkIdType pid[1];
  
  const auto critical_points = get_critical_points();
  for (const auto &cp : get_critical_points()) {
    double p[3] = {cp.x[0], cp.x[1], cp.x[2]};
    pid[0] = points->InsertNextPoint(p);
    vertices->InsertNextCell(1, pid);
  }

  polyData->SetPoints(points);
  polyData->SetVerts(vertices);

#if 0 // TODO
  // point data for types
  vtkSmartPointer<vtkDoubleArray> types = vtkSmartPointer<vtkDoubleArray>::New();
  types->SetNumberOfValues(results.size());
  for (auto i = 0; i < results.size(); i ++) {
    types->SetValue(i, static_cast<double>(results[i].type));
  }
  types->SetName("type");
  polyData->GetPointData()->AddArray(types);
  
  // point data for scalars
  if (has_scalar_field) {
    vtkSmartPointer<vtkDoubleArray> scalars = vtkSmartPointer<vtkDoubleArray>::New();
    scalars->SetNumberOfValues(results.size());
    for (auto i = 0; i < results.size(); i ++) {
      scalars->SetValue(i, static_cast<double>(results[i].scalar));
    }
    scalars->SetName("scalar");
    polyData->GetPointData()->AddArray(scalars);
  }
#endif
  return polyData;
}

inline vtkSmartPointer<vtkPolyData> critical_point_tracker::get_traced_critical_points_vtk() const
{
  vtkSmartPointer<vtkPolyData> polyData = vtkPolyData::New();
  vtkSmartPointer<vtkPoints> points = vtkPoints::New();
  vtkSmartPointer<vtkCellArray> lines = vtkCellArray::New();
  vtkSmartPointer<vtkCellArray> verts = vtkCellArray::New();

  for (const auto &curve : traced_critical_points)
    for (auto i = 0; i < curve.size(); i ++) {
      double p[3] = {curve[i][0], curve[i][1], curve[i][2]};
      points->InsertNextPoint(p);
    }

  size_t nv = 0;
  for (const auto &curve : traced_critical_points) {
    if (curve.size() < 2) { // isolated vertex
      vtkSmartPointer<vtkVertex> obj = vtkVertex::New();
      obj->GetPointIds()->SetNumberOfIds(curve.size());
      for (int i = 0; i < curve.size(); i ++)
        obj->GetPointIds()->SetId(i, i+nv);
      verts->InsertNextCell(obj);
    } else { // lines
      vtkSmartPointer<vtkPolyLine> obj = vtkPolyLine::New();
      obj->GetPointIds()->SetNumberOfIds(curve.size());
      for (int i = 0; i < curve.size(); i ++)
        obj->GetPointIds()->SetId(i, i+nv);
      lines->InsertNextCell(obj);
    }
    nv += curve.size();
  }
 
  polyData->SetPoints(points);
  polyData->SetLines(lines);
  polyData->SetVerts(verts);

  // point data for types
  if (1) { // if (type_filter) {
    vtkSmartPointer<vtkUnsignedIntArray> types = vtkSmartPointer<vtkUnsignedIntArray>::New();
    types->SetNumberOfValues(nv);
    size_t i = 0;
    for (const auto &curve : traced_critical_points) {
      for (auto j = 0; j < curve.size(); j ++)
        types->SetValue(i ++, curve[j].type);
    }
    types->SetName("type");
    polyData->GetPointData()->AddArray(types);
  }

  if (1) { // ids
    vtkSmartPointer<vtkUnsignedIntArray> ids = vtkSmartPointer<vtkUnsignedIntArray>::New();
    ids->SetNumberOfValues(nv);
    size_t i = 0;
    for (auto k = 0; k < traced_critical_points.size(); k ++)
      for (auto j = 0; j < traced_critical_points[k].size(); j ++)
        ids->SetValue(i ++, k);
    ids->SetName("id");
    polyData->GetPointData()->AddArray(ids);

  }

  // point data for scalars
  // if (has_scalar_field) {
  if (1) { // scalar is 0 if no scalar field available
    vtkSmartPointer<vtkDoubleArray> scalars = vtkSmartPointer<vtkDoubleArray>::New();
    scalars->SetNumberOfValues(nv);
    size_t i = 0;
    for (const auto &curve : traced_critical_points) {
      for (auto j = 0; j < curve.size(); j ++)
        scalars->SetValue(i ++, curve[j].scalar[0]);
    }
    scalars->SetName("scalar");
    polyData->GetPointData()->AddArray(scalars);
  }

  return polyData;
}
#else
inline void critical_point_tracker::write_traced_critical_points_vtk(const std::string& filename) const
{
  if (is_root_proc())
    fprintf(stderr, "[FTK] fatal: FTK not compiled with VTK.\n");
}

inline void critical_point_tracker::write_critical_points_vtk(const std::string& filename) const
{
  if (is_root_proc())
    fprintf(stderr, "[FTK] fatal: FTK not compiled with VTK.\n");
}
#endif

inline void critical_point_tracker::write_traced_critical_points_binary(const std::string& filename) const
{
  if (is_root_proc())
  	diy::serializeToFile(traced_critical_points, filename);
}

inline void critical_point_tracker::write_traced_critical_points_text(const std::string& filename) const
{
  if (is_root_proc()) {
    std::ofstream out(filename);
    write_traced_critical_points_text(out);
    out.close();
  }
}

inline void critical_point_tracker::write_traced_critical_points_text(std::ostream& os) const
{
  os << "#trajectories=" << traced_critical_points.size() << std::endl;
  for (int i = 0; i < traced_critical_points.size(); i ++) {
    os << "--trajectory " << i << std::endl;
    const auto &curve = traced_critical_points[i];
    for (int k = 0; k < curve.size(); k ++) {
      const auto &cp = curve[k];
      if (cpdims() == 2) {
        os << "---x=(" << cp[0] << ", " << cp[1] << "), "
           << "t=" << cp[2] << ", ";
      } else {
        os << "---x=(" << cp[0] << ", " << cp[1] << ", " << cp[2] << "), " 
           << "t=" << cp[3] << ", ";
      }
      os << "scalar=" << cp.scalar[0] << ", "
         << "type=" << cp.type << std::endl;
    }
  }
}

inline void critical_point_tracker::write_critical_points_text(const std::string& filename) const
{
  if (is_root_proc()) {
    std::ofstream out(filename);
    write_critical_points_text(out);
    out.close();
  }
}

inline void critical_point_tracker::write_critical_points_text(std::ostream& os) const
{
  for (const auto &cp : get_critical_points()) {
    if (cpdims() == 2) {
      os << "---x=(" << cp[0] << ", " << cp[1] << "), "
         << "t=" << cp[2] << ", ";
    } else {
      os << "---x=(" << cp[0] << ", " << cp[1] << ", " << cp[2] << "), " 
         << "t=" << cp[3] << ", ";
    }
    os << "scalar=" << cp.scalar[0] << ", "
       << "type=" << cp.type << std::endl;
  }
}

inline void critical_point_tracker::set_type_filter(unsigned int f)
{
  use_type_filter = true;
  type_filter = f;
}

inline void critical_point_tracker::set_num_scalar_components(int n)
{
  assert(n <= FTK_CP_MAX_NUM_VARS);
  num_scalar_components = n;
}

}

#endif
