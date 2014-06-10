function [W,H] = lf_nmf_2d_Euclidean(LF,W,H,niter)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% LF_NMF_2D_EUCLIDEAN
%    Factorizes 4D light fields for display on dual-stacked LCDs using
%    NMF for a frustrum of rays nearly perpendicular to the display. 
%    Accepts as input a (relative) two-plane light field parameterization.
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Extract factorization rank.
if size(W,2) ~= size(H,1)
   error('Input mask matrices {W,H} do not have matching dimensions!');
else
   R = size(W,2);
end

% Extract light field dimensions.
dim         = size(LF);
nAngles     = dim(3:4);
nHalfAngles = (nAngles-1)/2;
N           = prod(dim(1:2));

% Use multiplicative update rule to minimize Euclidean norm in "central band".
% Note: Masks are wrapped in "row-major" order.
for iter = 1:niter
   
   % Display iteration.
   disp(['  + Updating for iteration ',int2str(iter),'...']);
%    pause(0);
   
   % Initialize decomposition (using last iteration result).
   W0 = W; H0 = H;
   
   % Update H matrix.
   H = zeros(R,N,'single');
   for r = 1:R
      for j = 1:N
         s = mod(j-1,dim(2))+1;
         t = floor((j-1)/dim(2))+1;
         NUM = 0; DEN = 0;
         S = max(1,s-nHalfAngles(2)):min(dim(2),s+nHalfAngles(2));
         T = max(1,t-nHalfAngles(1)):min(dim(1),t+nHalfAngles(1));
         I = repmat(S,[length(T) 1])'+dim(2)*repmat(T'-1,[1 length(S)])';
         for i = I(:)'
            u = mod(i-1,dim(2))+1;
            v = floor((i-1)/dim(2))+1;
            a = nAngles(2)-mod((u-s)+nHalfAngles(2),nAngles(2));
            b = nAngles(1)-mod((v-t)+nHalfAngles(1),nAngles(1));
            NUM = NUM + W0(i,r)*LF(v,u,b,a);
            for r2 = 1:R
               DEN = DEN + W0(i,r)*(W0(i,r2)*H0(r2,j));
            end
         end
         H(r,j) = H0(r,j)*(NUM/(DEN+eps(NUM)));
      end
   end
   H = min(1,H);
   
   % Update W matrix.
   H0 = H;
   W = zeros(N,R,'single');
   for i = 1:N
      for r = 1:R
         u = mod(i-1,dim(2))+1;
         v = floor((i-1)/dim(2))+1;
         NUM = 0; DEN = 0;
         U = max(1,u-nHalfAngles(2)):min(dim(2),u+nHalfAngles(2));
         V = max(1,v-nHalfAngles(1)):min(dim(1),v+nHalfAngles(1));
         J = repmat(U,[length(V) 1])'+dim(2)*repmat(V'-1,[1 length(U)])';
         for j = J(:)'
            s = mod(j-1,dim(2))+1;
            t = floor((j-1)/dim(2))+1;
            a = nAngles(2)-mod((u-s)+nHalfAngles(2),nAngles(2));
            b = nAngles(1)-mod((v-t)+nHalfAngles(1),nAngles(1));
            NUM = NUM + H0(r,j)*LF(v,u,b,a);
            for r2 = 1:R
               DEN = DEN + H0(r,j)*(W0(i,r2)*H0(r2,j));             
            end
         end
         W(i,r) = W0(i,r)*(NUM/(DEN+eps(NUM)));
      end
   end
   W = min(1,W);
   
end
