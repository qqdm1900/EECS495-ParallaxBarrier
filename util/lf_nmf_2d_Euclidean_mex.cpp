
//-------------------------------------------------------------------------
// LF_NMF_2D_EUCLIDEAN_MEX
//    Factorizes 4D light fields for display on dual-stacked LCDs using
//    NMF for a frustrum of rays nearly perpendicular to the display . 
//    Accepts as input a (relative) two-plane light field parameterization.
//
//-------------------------------------------------------------------------

// Define included files.
#include <math.h>
#include <cstring>
#include "mex.h"

// Define pointers to input/output arguments.
#define LF_IN       prhs[0] // (input) 4D light fiel
#define W_IN        prhs[1] // (input) initial rear masks
#define H_IN        prhs[2] // (input) initial front masks
#define NITER_IN    prhs[3] // (input) number of iterations
#define FIX_H_IN    prhs[4] // (input) flag to disable front mask update
#define MIN_PSNR_IN prhs[5] // (input) minimum PSNR (stop if exceeded)
#define W_OUT       plhs[0] // (output) optimized rear mask pairs
#define H_OUT       plhs[1] // (output) optimized front mask pairs
#define E_OUT       plhs[2] // (output) PSNR as a function of iteration index

// Define macros for element-wise minimum/maximum operations.
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)>(b)?(b):(a))

// Define macro to determine if input is NaN.
#ifndef isnan
   #define isnan(x) ((x)!=(x))
#endif

// Declare structure for storing range of mask indices.
typedef struct {
   unsigned int start;
   unsigned int finish;
   unsigned int length;
} MaskIndices;

// Declare auxiliary functions.
unsigned long mxArrayReadScalar(const mxArray*);
inline unsigned int su_idx(unsigned int, unsigned int);
inline unsigned int tv_idx(unsigned int, unsigned int);
inline MaskIndices SU_MaskIndices(unsigned int, unsigned int*, unsigned int);
inline MaskIndices TV_MaskIndices(unsigned int, unsigned int*, unsigned int);
inline unsigned int num_indices(MaskIndices*, MaskIndices*);
inline unsigned int curr_idx(unsigned int, unsigned int, unsigned int, 
        MaskIndices*, MaskIndices*, unsigned int*, unsigned int);
inline int ab_idx(unsigned int, unsigned int, unsigned int, unsigned int);

// Define MEX-file gateway routine.
void mexFunction(
    int nlhs, mxArray* plhs[],
    int nrhs, const mxArray* prhs[]){
   
   // Verify number of input arguments.
   if(nrhs < 4 || nrhs > 6 )
      mexErrMsgTxt("Incorrect number of input arguments (i.e., expected four, five, or six).");
   
   // Verify first input argument (i.e., a 4D light field matrix).
   double* lf = mxGetPr(LF_IN);
   if(lf == NULL)
      mexErrMsgTxt("Input light field is invalid.");
   if(mxGetNumberOfDimensions(LF_IN) != 4)
      mexErrMsgTxt("Input light field must be four-dimensional.");
   if(!mxIsDouble(LF_IN))
      mexErrMsgTxt("Input light field must be of type double.");
   const mwSize* mw_lf_dim = mxGetDimensions(LF_IN);
   unsigned int lf_dim[4];
   for(int i=0; i<4; i++)
      lf_dim[i] = mw_lf_dim[i];
   unsigned long N = lf_dim[0]*lf_dim[1];
      
   // Verify second and third input arguments (i.e., initial mask pairs).
   if(mxGetNumberOfDimensions(W_IN) != 2)
      mexErrMsgTxt("Input rear masks W must be two-dimensional.");
   if(mxGetNumberOfDimensions(H_IN) != 2)
      mexErrMsgTxt("Input front masks H must be two-dimensional.");
   if(!mxIsDouble(W_IN))
      mexErrMsgTxt("Input rear masks W must be of type double.");
   if(!mxIsDouble(H_IN))
      mexErrMsgTxt("Input front masks H must be of type double.");
   if(mxGetN(W_IN) != mxGetM(H_IN))
      mexErrMsgTxt("Number of columns in W must equal number of rows in H.");
   unsigned long R = mxGetN(W_IN);  
   if(mxGetM(W_IN) != N || mxGetN(W_IN) != R){
      char msg[1024];
      sprintf(msg,"Input rear masks W must have dimensions %dx%d.", N, R);
      mexErrMsgTxt(msg);
   }  
   if(mxGetM(H_IN) != R || mxGetN(H_IN) != N){
      char msg[1024];
      sprintf(msg,"Input front masks H must have dimensions %dx%d.", R, N);
      mexErrMsgTxt(msg);
   }
   
   // Verify forth input argument (i.e., number of iterations).
   if (!mxIsNumeric(NITER_IN))
      mexErrMsgTxt("Input number of iterations must be a numerical value.");
   if (mxGetNumberOfElements(NITER_IN) != 1)
      mexErrMsgTxt("Input number of iterations must be scalar.");
   unsigned long niter = mxArrayReadScalar(NITER_IN);
   
   // Verify fifth input argument (i.e., flag to enable/disable fixed front masks).
   bool fix_H = false;
   if(nrhs == 5){
      if (!mxIsLogical(FIX_H_IN))
         mexErrMsgTxt("Input flag to disable front mask update must be Boolean.");
      if (mxGetNumberOfElements(FIX_H_IN) != 1)
         mexErrMsgTxt("Input flag to disable front mask update must be scalar.");
      fix_H = *(bool*) mxGetData(FIX_H_IN);
   }
   
   // Verify sixth input argument (i.e., minimum PSNR).
   double min_PSNR = 1000.0;
   if(nrhs == 6){
      if(!mxIsNumeric(MIN_PSNR_IN))
         mexErrMsgTxt("Minimum PSNR (stopping criterion) be a numerical value.");
      if(mxGetNumberOfElements(NITER_IN) != 1)
         mexErrMsgTxt("Minimum PSNR (stopping criterion) must be scalar.");
      min_PSNR = mxArrayReadScalar(MIN_PSNR_IN);   
   }
   
   // Evaluate intermediate variables (e.g., angles and half-angles).
   unsigned int nAngles[2];
   nAngles[0] = lf_dim[2];
   nAngles[1] = lf_dim[3];
   unsigned int nHalfAngles[2];
   nHalfAngles[0] = (nAngles[0]-1)/2;
   nHalfAngles[1] = (nAngles[1]-1)/2;
   
   // Initialze the front/rear mask pairs (for each temporally-multiplexed frame).
   mxArray* W = mxCreateNumericMatrix(mxGetM(W_IN), mxGetN(W_IN), mxDOUBLE_CLASS, mxREAL);
   mxArray* H = mxCreateNumericMatrix(mxGetM(H_IN), mxGetN(H_IN), mxDOUBLE_CLASS, mxREAL);
   memcpy((double*)mxGetData(W), 
          (double*)mxGetData(W_IN), 
          sizeof(double)*mxGetM(W)*mxGetN(W));
   memcpy((double*)mxGetData(H), 
          (double*)mxGetData(H_IN), 
          sizeof(double)*mxGetM(H)*mxGetN(H));
   double* W_data  = mxGetPr(W);
   double* H_data  = mxGetPr(H);
      
   // Allocate intermediate variables for evaluating the update rule.
   mxArray* W0 = mxCreateNumericMatrix(mxGetM(W), mxGetN(W), mxDOUBLE_CLASS, mxREAL);
   mxArray* H0 = mxCreateNumericMatrix(mxGetM(H), mxGetN(H), mxDOUBLE_CLASS, mxREAL);
   double* W0_data = mxGetPr(W0);
   double* H0_data = mxGetPr(H0);
   
   // Allocate PSNR array (if necessary).
   bool evaluate_PSNR = false;
   mxArray* E = NULL;
   double* E_data = NULL;
   if(nlhs > 2 || nrhs > 5){
      evaluate_PSNR = true;
      E = mxCreateNumericMatrix(niter, 1, mxDOUBLE_CLASS, mxREAL);
      E_data = mxGetPr(E);
   }
   
   // Apply the weighted multiplicative update rule.
   for(unsigned int iter=0; iter<niter; iter++) {
         
      // Evaluate PSNR of light field approximation (if necessary).
      if(evaluate_PSNR){
         double MSE = 0;
         double max_elem = 0;
         double num_elem = 0;
         for(unsigned int b=0; b<lf_dim[2]; b++){
            for(unsigned int a=0; a<lf_dim[3]; a++){
               for(unsigned int v=0; v<lf_dim[0]; v++){
                  for(unsigned int u=0; u<lf_dim[1]; u++){
                     unsigned int s = u+(a-nHalfAngles[1]);
                     unsigned int t = v+(b-nHalfAngles[0]);
                     if(s>=0 && s<lf_dim[1] && t>=0 && t<lf_dim[0]){    
                        double lf_approx = 0;
                        for(unsigned int r=0; r<R; r++){
                           unsigned int i = mw_lf_dim[1]*v+u;
                           unsigned int j = mw_lf_dim[1]*t+s;
                           lf_approx += W_data[r*N+i]*H_data[j*R+r];            
                        }
                        MSE += pow(lf[mw_lf_dim[0]*(mw_lf_dim[1]*
                                     (mw_lf_dim[2]*a+b)+u)+v] - lf_approx, 2);
                        max_elem = MAX(max_elem, 
                           lf[mw_lf_dim[0]*(mw_lf_dim[1]*(mw_lf_dim[2]*a+b)+u)+v]);
                        num_elem++;
                     }
                  }
               }
            }
         }
         MSE /= num_elem;
         E_data[iter] = (double)(10.0*log10(pow(max_elem, 2)/MSE));
         if(E_data[iter] > min_PSNR){
            mexPrintf("  + Stopping at iteration #%03d (PSNR = %4.1f dB > %4.1f dB)...\n",
                      iter+1, E_data[iter], min_PSNR);
            for(unsigned int i=iter+1; i<niter; i++)
               E_data[i] = E_data[iter];
            if(nlhs > 0)
               W_OUT = W;
            if(nlhs > 1)
               H_OUT = H;
            if(nlhs > 2)
               E_OUT = E;
            return;
         }
         if((iter%10)==0){
            mexPrintf("  + Updating for iteration #%03d (initial PSNR = %4.1f dB)...\n", 
                      iter+1, E_data[iter]);
         }
      }
      else{
         if((iter%10)==0)
            mexPrintf("  + Updating for iteration #%d...\n", iter+1);
      }
      mexEvalString("drawnow");
      
      // Initialize factorization using previous result.
      memcpy(W0_data, W_data, sizeof(double)*mxGetM(W)*mxGetN(W));
      memcpy(H0_data, H_data, sizeof(double)*mxGetM(H)*mxGetN(H));
         
      // Update the front mask pairs (i.e., the "H" matrix).
      if(!fix_H){
         memset(H_data, 0, sizeof(double)*mxGetM(H)*mxGetN(H));
         for(unsigned int j=0; j<N; j++) {
            unsigned int s = su_idx(j, lf_dim[1]);
            unsigned int t = tv_idx(j, lf_dim[1]);
            MaskIndices S = SU_MaskIndices(s, nHalfAngles, lf_dim[1]);
            MaskIndices T = TV_MaskIndices(t, nHalfAngles, lf_dim[0]);
            for(unsigned int r=0; r<R; r++){
               double num = 0;
               double den = 0;
               unsigned int I = num_indices(&S, &T);
               for(unsigned int i=0; i<I; i++){
                  unsigned int ii = curr_idx(i, s, t, &S, &T, nHalfAngles, lf_dim[1]);
                  unsigned int u = su_idx(ii, lf_dim[1]);
                  unsigned int v = tv_idx(ii, lf_dim[1]);
                  int a = (nAngles[1]-1)-ab_idx(s, u, nHalfAngles[1], nAngles[1]);
                  int b = (nAngles[0]-1)-ab_idx(t, v, nHalfAngles[0], nAngles[0]);
                  double dotp = 0;
                  for(unsigned int dp=0; dp<R; dp++)
                     dotp += W0_data[dp*N+ii]*H0_data[j*R+dp];
                  num += W0_data[ii+r*N]*
                     lf[mw_lf_dim[0]*(mw_lf_dim[1]*(mw_lf_dim[2]*a+b)+u)+v];
                  den += W0_data[r*N+ii]*dotp;
               }
               H_data[j*R+r] = H0_data[j*R+r]*(num/den);
            }
         }
         for(unsigned int i=0; i<mxGetM(H)*mxGetN(H); i++){
            H_data[i] = MIN(H_data[i], 1);
            if(isnan(H_data[i])) 
               H_data[i] = 1.0;
         }
      }

      // Update the rear mask pairs (i.e., the "W" matrix).
      memcpy(H0_data, H_data, sizeof(double)*mxGetM(H)*mxGetN(H));
      memset(W_data, 0, sizeof(double)*mxGetM(W)*mxGetN(W));
      for(unsigned int i=0; i<N; i++){
         unsigned int u = su_idx(i, lf_dim[1]);
         unsigned int v = tv_idx(i, lf_dim[1]);
         MaskIndices U = SU_MaskIndices(u, nHalfAngles, lf_dim[1]);
         MaskIndices V = TV_MaskIndices(v, nHalfAngles, lf_dim[0]);
         for(unsigned int r=0; r<R; r++){
            double num = 0;
            double den = 0;
            unsigned int J = num_indices(&U, &V);
            for(unsigned int j=0; j<J; j++){
               unsigned int jj = curr_idx(j, u, v, &U, &V, nHalfAngles, lf_dim[1]);
               unsigned int s = su_idx(jj, lf_dim[1]);
               unsigned int t = tv_idx(jj, lf_dim[1]);
               int a = (nAngles[1]-1)-ab_idx(s, u, nHalfAngles[1], nAngles[1]);
               int b = (nAngles[0]-1)-ab_idx(t, v, nHalfAngles[0], nAngles[0]);
               double dotp = 0;
               for(unsigned int dp=0; dp<R; dp++)
                  dotp += W0_data[dp*N+i]*H0_data[jj*R+dp];
               num += H0_data[jj*R+r]*
                  lf[mw_lf_dim[0]*(mw_lf_dim[1]*(mw_lf_dim[2]*a+b)+u)+v];
               den += H0_data[jj*R+r]*dotp;
            }
            W_data[r*N+i] = W0_data[r*N+i]*(num/den);
         }
      }
      for(unsigned int i=0; i<mxGetM(W)*mxGetN(W); i++){
         W_data[i] = MIN(W_data[i], 1);
         if(isnan(W_data[i]))
            W_data[i] = 1.0;
      }
      
   }
   
   // Return optimized front/rear mask pairs.
   if(nlhs > 0)
      W_OUT = W;
   if(nlhs > 1)
      H_OUT = H;
   if(nlhs > 2)
      E_OUT = E;

   // Return without errors.
   return;
}

// Define inline function to return row index, given linear index.
// Note: Assumes linear index into mask, wrapped in "row-major" order.
inline unsigned int su_idx(unsigned int i, unsigned int N){
   return i%(unsigned int)N;
}

// Define inline function to return column index, given linear index.
// Note: Assumes linear index into mask, wrapped in "row-major" order.
inline unsigned int tv_idx(unsigned int i, unsigned int N){
   return i/(unsigned int)N;
}

// Define inline function to evaluate range of mask column indices.
inline MaskIndices SU_MaskIndices(
        unsigned int s, unsigned int* nHalfAngles, unsigned int N){
   MaskIndices mskIdx;
   mskIdx.start  = MAX(0, (int)s-(int)nHalfAngles[1]);
   mskIdx.finish = MIN(N-1, s+nHalfAngles[1]);
   mskIdx.length = mskIdx.finish-mskIdx.start+1;
   return mskIdx;
}

// Define inline function to evaluate range of mask row indices.
inline MaskIndices TV_MaskIndices(
        unsigned int t, unsigned int* nHalfAngles, unsigned int N){
   MaskIndices mskIdx;
   mskIdx.start  = MAX(0, (int)t-(int)nHalfAngles[0]);
   mskIdx.finish = MIN(N-1, t+nHalfAngles[0]);
   mskIdx.length = mskIdx.finish-mskIdx.start+1;
   return mskIdx;
}

// Define inline function to evaluate number of mask indices.
inline unsigned int num_indices(MaskIndices* S, MaskIndices* T){
  return S->length*T->length;
}

// Define inline function to return linear index, given ranges of mask row/colmn indices.
inline unsigned int curr_idx(
        unsigned int i, unsigned int s, unsigned int t, 
        MaskIndices* S, MaskIndices* T, 
        unsigned int* nHalfAngles, unsigned int N){
   return ((S->start)+(i%S->length)+((i/S->length)+T->start)*N);
}

// Define inline function to return linear index, given row/columnr indices.
inline int ab_idx(unsigned int s, unsigned int u, unsigned int nHalfAngles, unsigned int nAngles){
   return ((int)((int)u-(int)s)+nHalfAngles)%nAngles;
}

// Define function to read a 64-bit scalar input argument.
unsigned long mxArrayReadScalar(const mxArray* a){
  
   if(mxIsDouble(a)){
      double* d = (double*) mxGetData(a);
      return (unsigned long) *d;
   }
   if(mxIsSingle(a)) {
      float* f = (float*) mxGetData(a);
      return (unsigned long) *f;
   }
   if(mxIsInt8(a)) {
      char* i8 = (char*) mxGetData(a);
      return (unsigned long) *i8;
   }
   if(mxIsUint8(a)) {
      unsigned char* u8 = (unsigned char*) mxGetData(a);
      return (unsigned long) *u8;
   }
   if(mxIsInt16(a)) {
      short* i16 = (short*)mxGetData(a);
      return (unsigned long) *i16;
   }
   if(mxIsUint16(a)) {
      unsigned short* u16 = (unsigned short*) mxGetData(a);
      return (unsigned long) *u16;
   }
   if(mxIsInt32(a)) {
      int* i32 = (int*) mxGetData(a);
      return (unsigned long) *i32;
   }
   if(mxIsUint32(a)) {
      unsigned int* u32 = (unsigned int*) mxGetData(a);
      return (unsigned long) *u32;
   }
   if(mxIsInt64(a)) {
      long* i64 = (long*) mxGetData(a);
      return (unsigned long) *i64;
   }
   if(mxIsUint64(a)) {
      return *(unsigned int*) mxGetData(a);
   }
   mexErrMsgTxt("Input to mxArrayReadScalar is an unknown type.");
   return 0;
   
}
