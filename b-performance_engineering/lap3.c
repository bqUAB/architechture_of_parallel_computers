#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

float stencil ( float v1, float v2, float v3, float v4)
{
  return (v1 + v2 + v3 + v4) * 0.25f;
}

float max_error ( float prev_error, float old, float new )
{
  float t= fabsf( new - old );
  return t> prev_error? t: prev_error;
}

void laplace_step_line (float *inUp, float *in, float *inDown, float *out, int n)
{
  int i;
  #pragma omp simd
  for ( i=1; i < n-1; i++ )
    out[i]= stencil ( inUp[i], in[i-1], in[i+1], inDown[i] );
}

float laplace_error_line(float error, float *inUp, float *in, float *inDown, float *out, int n)
{
  int i;
  #pragma omp simd reduction(max:error)
  for ( i=1; i < n-1; i++ )
  {
    out[i]=  stencil ( inUp[i], in[i-1], in[i+1], inDown[i] );
    error = max_error( error, out[i], in[i] );
  }
  return error;
}


void laplace_init(float *in, int n)
{
  int i;
  const float pi  = 2.0f * asinf(1.0f);
  memset(in, 0, n*n*sizeof(float));
  for (i=0; i<n; i++) {
    float V = in[i*n] = sinf(pi*i / (n-1));
    in[ i*n+n-1 ] = V*expf(-pi);
  }
}

int main(int argc, char** argv)
{
  int n = 4096;
  int iter_max = 1000;
  float *A, *temp;
    
  const float tol = 1.0e-5f;
  float error= 1.0f;    

  // get runtime arguments 
  if (argc>1) {  n        = atoi(argv[1]); }
  if (argc>2) {  iter_max = atoi(argv[2]); }

  if (iter_max & 1) // n is odd
  { 
    printf("Maximum number of iterations must be even\n" );
    exit(1);
  }

  A    = (float*) malloc( n*n*sizeof(float) );
  temp = (float*) malloc( n*n*sizeof(float) );

  //  set boundary conditions
  laplace_init (A, n);
  laplace_init (temp, n);
  A[(n/128)*n+n/128] = 1.0f; // set singular point

  printf("Jacobi relaxation Calculation: %d x %d mesh,"
         " maximum of %d iterations\n", 
         n, n, iter_max );

  int iter = 0;
  while ( error > tol*tol && iter < iter_max )
  {
    int j;
    error=0.0f;
    iter+=2;
    for ( j=1; j < n-1; j++ ) 
      laplace_step_line         (A+(j-1)*n, A+j*n, A+(j+1)*n, temp+j*n, n);
    for ( j=1; j < n-1; j++ ) 
      error= laplace_error_line (error, temp+(j-1)*n, temp+j*n, temp+(j+1)*n, A+j*n, n);
  }
  error = sqrtf( error );
  printf("Total Iterations: %5d, ERROR: %0.6f, ", iter, error);
  printf("A[%d][%d]= %0.6f\n", n/128, n/128, A[(n/128)*n+n/128]);
}
