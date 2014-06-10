
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% MAKE_LF_NMF_2DCC_EUCLIDEAN
%    Compiles MEX implemetaton of light field factorizaton using NMF with
%    a weighted Euclidean update rule.
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Display compilation details.
clear all; clc;
disp('Compiling lf_nmf_2d_Euclidean_mex...');
eval('mex -largeArrayDims lf_nmf_2d_Euclidean_mex.cpp');

% Test compiled NMF function.
LF.dim  = [15 21 5 3];
LF.data = rand(LF.dim);
R       = 3;
W0      = rand(prod(LF.dim([1 2])),R);
H0      = rand(R,prod(LF.dim([1 2])));
niter   = 100;
tic
   [W H E] = lf_nmf_2d_Euclidean_mex(LF.data,W0,H0,niter,false);
toc
plot(1:niter,E,'.-');
xlabel('iteration index');
ylabel('PSNR');
grid on;