function b = zeroshift(a,p)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%ZEROSHIFT
%   B = ZEROSHIFT(A,SHIFTSIZE) shifts the values in the array A
%   by SHIFTSIZE elements. SHIFTSIZE is a vector of integer scalars where
%   the N-th element specifies the shift amount for the N-th dimension of
%   array A. If an element in SHIFTSIZE is positive, the values of A are
%   shifted down (or to the right). If it is negative, the values of A
%   are shifted up (or to the left).
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Error out if there are not exactly two input arguments.
if nargin < 2
   error('MATLAB:circshift:NoInputs',['No input arguments specified. '...
         'There should be exactly two input arguments.'])
end

% Parse the inputs to reveal the variables necessary for the calculations.
[p, sizeA, numDimsA, msg] = ParseInputs(a,p);

% Error out if ParseInputs discovers an improper SHIFTSIZE input.
if (~isempty(msg))
	error('MATLAB:circshift:InvalidShiftType','%s',msg);
end

% Initialize the cell array of indices
idx1 = cell(1, numDimsA);
idx2 = cell(1, numDimsA);

% Loop through each dimension of the input matrix to calculate shifted indices.
for k = 1:numDimsA
	m       = sizeA(k);
   idx1{k} = mod((0:m-1)-p(k), m)+1;
   idx2{k} = (((1:m)-p(k)) < 1) | (((1:m)-p(k)) > m);
end

% Perform the actual conversion by indexing into the input matrix.
b = a(idx1{:});
b(idx2{1},:) = 0;
b(:,idx2{2}) = 0;

% Parse inputs.
function [p, sizeA, numDimsA, msg] = ParseInputs(a,p)

% Set default values.
sizeA    = size(a);
numDimsA = ndims(a);
msg      = '';

% Make sure that SHIFTSIZE input is a finite, real integer vector.
sh        = p(:);
isFinite  = all(isfinite(sh));
nonSparse = all(~issparse(sh));
isInteger = all(isa(sh,'double') & (imag(sh)==0) & (sh==round(sh)));
isVector  = ((ndims(p) == 2) && ((size(p,1) == 1) || (size(p,2) == 1)));
if ~(isFinite && isInteger && isVector && nonSparse)
    msg = ['Invalid shift type: ' ...
          'must be a finite, nonsparse, real integer vector.'];
    return;
end

% Make sure the shift vector has the same length as numDimsA. 
if (numel(p) < numDimsA)
   p(numDimsA) = 0;
end
