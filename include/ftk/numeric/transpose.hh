#ifndef _FTK_TRANSPOSE_H
#define _FTK_TRANSPOSE_H

#include <algorithm>
#include <utility>

namespace ftk {

template <typename T>
void transpose2x2(const T A[2][2], T B[2][2])
{
  B[0][0] = A[0][0];
  B[0][1] = A[1][0];
  B[1][0] = A[0][1];
  B[1][1] = A[1][1];
}

template <typename T>
void transpose2x2(T m[4])
{
  std::swap(m[1], m[2]);
}

template <typename T>
void transpose3x3(T m[9]) 
{
  std::swap(m[1], m[3]);
  std::swap(m[2], m[6]);
  std::swap(m[5], m[7]);
}

template <typename T>
void transpose3x3(const T a[9], T b[9]) 
{
  b[0] = a[0];
  b[1] = a[3];
  b[2] = a[6];
  b[3] = a[1];
  b[4] = a[4];
  b[5] = a[7];
  b[6] = a[2];
  b[7] = a[5];
  b[8] = a[8];
}

template <typename T>
void transpose3x3(T A[3][3])
{
  std::swap(A[0][1], A[1][0]);
  std::swap(A[0][2], A[2][0]);
  std::swap(A[1][2], A[2][1]);
}

template <typename T>
void transpose3x3(const T A[3][3], T B[3][3])
{
  B[0][0] = A[0][0];
  B[0][1] = A[1][0];
  B[0][2] = A[2][0];
  B[1][0] = A[0][1];
  B[1][1] = A[1][1];
  B[1][2] = A[2][1];
  B[2][0] = A[0][2];
  B[2][1] = A[1][2];
  B[2][2] = A[2][2];
}

template <typename T>
void transpose4x4(T m[16]) 
{
  std::swap(m[1], m[4]);
  std::swap(m[2], m[8]);
  std::swap(m[3], m[12]);
  std::swap(m[6], m[9]);
  std::swap(m[7], m[13]);
  std::swap(m[11], m[14]);
}

template <typename T>
void transpose3x2(const T A[3][2], T B[2][3])
{
  B[0][0] = A[0][0]; 
  B[0][1] = A[1][0]; 
  B[0][2] = A[2][0];
  B[1][0] = A[0][1];
  B[1][1] = A[1][1];
  B[1][2] = A[2][1];
}

template <typename T>
void transpose2x3(const T A[2][3], T B[3][2])
{
  B[0][0] = A[0][0]; 
  B[0][1] = A[1][0];
  B[1][0] = A[0][1];
  B[1][1] = A[1][1];
  B[2][0] = A[0][2];
  B[2][1] = A[1][2];
}

}

#endif
