#include <gtest/gtest.h>
#include <ftk/numeric/inverse_linear_interpolation_solver.hh>
#include <ftk/numeric/linear_interpolation.hh>
#include <ftk/numeric/rand.hh>

class inverse_interpolation_test : public testing::Test {
public:
  const int nruns = 1; // 00000;
  const double epsilon = 1e-9;
};

TEST_F(inverse_interpolation_test, inverse_linear_interpolation_2simplex_vector2) {
  double V[3][2], mu[3], v[2];
  for (int run = 0; run < nruns; run ++) {
    ftk::rand3x2(V);
    ftk::inverse_lerp_s2v2(V, mu);
    ftk::lerp_s2v2(V, mu, v);

    EXPECT_NEAR(0.0, v[0], epsilon);
    EXPECT_NEAR(0.0, v[1], epsilon);
  }
}

TEST_F(inverse_interpolation_test, inverse_linear_interpolation_3simplex_vector3) {
  double V[4][3], mu[4], v[3];
  for (int run = 0; run < nruns; run ++) {
    ftk::rand4x3(V);
    ftk::inverse_lerp_s3v3(V, mu);
    ftk::lerp_s3v3(V, mu, v);

    EXPECT_NEAR(0.0, v[0], epsilon);
    EXPECT_NEAR(0.0, v[1], epsilon);
    EXPECT_NEAR(0.0, v[2], epsilon);
  }
}
