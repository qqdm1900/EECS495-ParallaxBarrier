
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% GENERATE_MASKS
%    Generates translated pinhole arrays and content-adaptive parallax 
%    barriers, given rendered light fields (i.e., an oblique image set).
%
%    The input/output gamma correction should be measured to ensure
%    the output mask pairs produce linear intensity variation.
%    See: http://perso.telecom-paristech.fr/~brettel/TESTS/Gamma/Gamma.html
%
%    Please read the course notes for additional details.
%       (1) Matthew Hirsch and Douglas Lanman, "Build Your Own 3D Display", 
%           ACM SIGGRAPH 2010 Course Notes, 2010.
%       (2) Douglas Lanman and Matthew Hirsch, "Build Your Own Glasses-Free
%           3D Display", ACM SIGGRAPH 2011 Course Notes, 2011.
%
% Douglas Lanman and Matthew Hirsch
% MIT Media Lab
% July 2010
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Reset Matlab environment and update file path.
clear all; clc; addpath(genpath('./util'));

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Define light field display parameters.

% Define light field display.
display.dSample   = 2;                           % spatial down-sampling factor
display.res       = [1050 1680]/display.dSample; % spatial display resolution [height width] (pixels)
display.nAngles   = [1 3];                       % light field angular resolution [vertical horizontal] (views)
display.fullColor = true;                        % enable/disable full-color display (otherwise luminance-only)
display.inGamma   = 2.2;                         % gamma-correction value for input images (e.g., "Display_Gamma" in POV-Ray)
display.outGamma  = 2.2;                         % gamma-correction value for output images (e.g., should equal display's gamma)

% Set NMF options.
NMF.enable       = true;                        % enable NMF-based mask decomposition
NMF.useMEX       = false;                         % enable precompiled Matlab executable
NMF.numPairs     = prod(display.nAngles);        % decomposition rank (i.e., number of mask pairs)
NMF.initMode     = 2;                            % initialization mode (1: noise, 2: pinholes, 3: pinholes/noise for front/rear)
NMF.numIter      = 10;                           % number of iterations
NMF.gain         = 1.0;                          % light field amplification factor
NMF.fixFrontMask = false;                        % fix the front mask (i.e., do not update)

% Define multi-view skewed orthographic images (i.e, the input light field).
image.frameDir   = './images/teapot2/';          % base directory (e.g., './images/teapot/')
image.frameBase  = 'teapot-0';                   % base file name (e.g., 'teapot-')
image.frameCount = '%0.1d';                      % counter format (e.g., '%0.1d')
image.frameExt   = 'png';                        % file format    (e.g., 'png')

% Define additional options.
options.saveMasks = true; % enable/disable mask output

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Evaluate display parameters.

% Display status.
disp('[Dual-stacked LCD Mask Pair Generator]');
disp('> Parsing input parameters...');

% Evaluate number of half-angles.
display.nHalfAngles = (display.nAngles-1)/2;

% Assign number of color channels.
if display.fullColor
   display.nChannels = 3;
else
   display.nChannels = 1;
end

% Check for input errors.
if(any(rem(display.nHalfAngles,1)>0))
   error('Angular resolutions must be odd!');
end
if(any(rem(display.res./display.nAngles,1)>0))
   error('Angular resolutions must evenly divide screen resolutions!');
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Load light field (i.e., the skewed orthographic image set).

% Display status.
disp('> Loading light field...');

% Load the pre-rendered light field (i.e., the skewed orthographic image set).
% Note: Gamma-correct to convert input images to a linear intensity scale.
if display.fullColor
   LF.dim = [display.res display.nAngles 3];
else
   LF.dim = [display.res display.nAngles];
end
LF.data.ideal = zeros(LF.dim);
k = 0;
for bIdx = 1:LF.dim(3)
   for aIdx = 1:LF.dim(4)
      k = k+1;
      disp(['  + Loading image ',int2str(k),'...']);
      fileIdx = LF.dim(4)*(bIdx-1)+aIdx;
      filename = [image.frameDir,image.frameBase,...
         num2str(fileIdx,image.frameCount),'.',image.frameExt];
      I = im2double(imread(filename));
      I = I.^display.inGamma;
      if display.fullColor
         if size(I,3) == 1
            I = repmat(I,[1 1 3]);
         end
      else
         I = 0.3*I(:,:,1)+0.59*I(:,:,2)+0.11*I(:,:,3);
      end
      I = imresize(I,display.res,'bilinear');
      LF.data.ideal(:,:,LF.dim(3)-bIdx+1,LF.dim(4)-aIdx+1,:) = I;
   end
end 

% Clear temporary variables.
clear I;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Generate pinhole array masks.

% Display status.
disp('> Generating pinhole array mask pairs...');

% Allocate storage for translated pinhole array masks.
for ch = 1:display.nChannels
   LF.data.pinhole_W{ch} = zeros(prod(LF.dim(1:2)),prod(LF.dim(3:4)));
   LF.data.pinhole_H{ch} = zeros(prod(LF.dim(3:4)),prod(LF.dim(1:2)));
end

% Generate pinhole array masks.
k = 0;
for bIdx = 1:display.nAngles(1)
   for aIdx = 1:display.nAngles(2)
      
      % Generate front mask.
      k = k+1;
      H = zeros(display.res);
      H(bIdx:display.nAngles(1):end,aIdx:display.nAngles(2):end) = 1.0;
      for ch = 1:display.nChannels
         LF.data.pinhole_H{ch}(k,:) = reshape(H',[],1);
      end
      
      % Generate rear mask.
      W = zeros([display.res display.nChannels]);
      W = padarray(W,display.nHalfAngles,0,'both');
      for j = 1:display.nAngles(1)
         for i = 1:display.nAngles(2)
            I = squeeze(LF.data.ideal(:,:,display.nAngles(1)-j+1,display.nAngles(2)-i+1,:));
            I = padarray(I,display.nHalfAngles,0,'both');
            for chIdx = 1:3
               W((bIdx+j-2)+(1:display.nAngles(1):display.res(1)),...
                 (aIdx+i-2)+(1:display.nAngles(2):display.res(2)),:) = ...
               I((bIdx+j-2)+(1:display.nAngles(1):display.res(1)),...
                 (aIdx+i-2)+(1:display.nAngles(2):display.res(2)),:);
            end
         end
      end
      W = W((display.nHalfAngles(1)+1):(end-display.nHalfAngles(1)),...
            (display.nHalfAngles(2)+1):(end-display.nHalfAngles(2)),:);
      for ch = 1:display.nChannels       
         LF.data.pinhole_W{ch}(:,k) = reshape(W(:,:,ch)',1,[]);
      end
      
   end
end

% Clear temporary variables.
clear W H I;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Generate content-adaptive parallax barriers.

% Generate content-adaptive parallax barriers.
if NMF.enable

   % Display status.
   disp('> Generating content-adaptive parallax barriers...');

   % Initialize NMF-based decomposition (i.e., mask pairs).
   W = cell(1,display.nChannels);
   H = cell(1,display.nChannels);
   switch NMF.initMode
      
      % Use random subset of pinhole array masks. 
      case 2
         order = randperm(prod(LF.dim(3:4)));
         for ch = 1:display.nChannels
            W{ch} = LF.data.pinhole_W{ch}(:,order(1:NMF.numPairs));
            H{ch} = LF.data.pinhole_H{ch}(order(1:NMF.numPairs),:);
         end
               
      % Use random subset of pinhole arrays (front masks) and random noise (rear masks). 
      case 3
         order = randperm(prod(LF.dim(3:4)));
         for ch = 1:display.nChannels
            if ch == 1
               W{ch} = rand(prod(LF.dim(1:2)),NMF.numPairs);
            else
               W{ch} = W{1};
            end
            H{ch} = LF.data.pinhole_H{ch}(order(1:NMF.numPairs),:);
         end
             
      % Use random noise masks.
      otherwise
         for ch = 1:display.nChannels
            if ch == 1
               W{ch} = rand(prod(LF.dim(1:2)),NMF.numPairs);
               H{ch} = rand(NMF.numPairs,prod(LF.dim(1:2)));     
            else
               W{ch} = W{1};
               H{ch} = H{1};
            end
         end
         
   end
   
   % Evaluate NMF of input light field.
   for ch = 1:display.nChannels
      if display.fullColor
         colorOrder = {'red color','green color','blue color'};
      else
         colorOrder = {'luminance'};
      end
      disp(' '); disp(['  <Processing ',colorOrder{ch},' channel>']);
      if NMF.useMEX 
         [LF.data.NMF_W{ch},LF.data.NMF_H{ch},LF.data.NMF_E{ch}] = ...
            lf_nmf_2d_Euclidean_mex(...
               NMF.gain*LF.data.ideal(:,:,:,:,ch)+1e-9*(LF.data.ideal(:,:,:,:,ch) == 0),...
               W{ch},H{ch},NMF.numIter,NMF.fixFrontMask);
      else
         [LF.data.NMF_W,LF.data.NMF_H] = ...
            lf_nmf_2d_Euclidean(...
               NMF.gain*LF.data.ideal(:,:,:,:,ch)+1e-9*(LF.data.ideal(:,:,:,:,ch) == 0),...
               W{ch},H{ch},NMF.numIter);
      end
   end
   disp(' ');
   
end

% Clear temporary variables.
clear W H;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Evaluate light field reconstructions.

% Display status.
disp('> Evaluating reconstruction accuracy...');

% Evaluate reconstruction using pinhole array mask pairs.
LF.data.pinhole = zeros(LF.dim);
for k = 1:prod(LF.dim(3:4))
   for ch = 1:display.nChannels
      W = reshape(LF.data.pinhole_W{ch}(:,k),LF.dim([2 1]))';
      H = reshape(LF.data.pinhole_H{ch}(k,:),LF.dim([2 1]))';
      for bIdx = 1:LF.dim(3)
         for aIdx = 1:LF.dim(4)
            bShift = bIdx-(LF.dim(3)+1)/2;
            aShift = aIdx-(LF.dim(4)+1)/2;
            LF.data.pinhole(:,:,bIdx,aIdx,ch) = ...
               LF.data.pinhole(:,:,bIdx,aIdx,ch) + W.*zeroshift(H,-[bShift aShift]);
         end
      end
   end
end

% Evaluate reconstruction using content-adaptive parallax barriers.
if 0
   LF.data.NMF = zeros(LF.dim);
   for k = 1:NMF.numPairs
      for ch = 1:display.nChannels
         W = reshape(LF.data.NMF_W{ch}(:,k),LF.dim([2 1]))';
         H = reshape(LF.data.NMF_H{ch}(k,:),LF.dim([2 1]))';
         for bIdx = 1:LF.dim(3)
            for aIdx = 1:LF.dim(4)
               bShift = bIdx-(LF.dim(3)+1)/2;
               aShift = aIdx-(LF.dim(4)+1)/2;
               LF.data.NMF(:,:,bIdx,aIdx,ch) = ...
                  LF.data.NMF(:,:,bIdx,aIdx,ch) + W.*zeroshift(H,-[bShift aShift]);
            end
         end
      end
   end
   LF.data.NMF_MSE = sum((NMF.gain*LF.data.pinhole(:)-LF.data.NMF(:)).^2)/prod(LF.dim);
   LF.data.NMF_PSNR = 10*log10((NMF.gain*max(LF.data.pinhole(:)))^2/LF.data.NMF_MSE);
   disp(['  + NMF achieves PSNR of ',num2str(LF.data.NMF_PSNR,'%0.1f'),' dB']);
   gain = LF.data.NMF(:)./LF.data.pinhole(:);
   gain = gain(isfinite(gain));
   gain = median(gain);
   disp(['  + NMF is ~',num2str(gain,'%0.1f'),'x brighter']);
end

% Clear temporary variables.
clear W H gain;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Write images for each mask (if enabled).

% Write images for each mask (if enabled).
% Note: Gamma-compress depending on measured display gamma value.
if options.saveMasks

   % Display status.
   disp('> Saving mask images...');
   
   % Create output directory.
   outputDir = [image.frameDir,'masks/pinhole/'];
   warning('off','MATLAB:MKDIR:DirectoryExists');
   mkdir(outputDir);
   warning('on','MATLAB:MKDIR:DirectoryExists');
   rmdir(outputDir,'s'); mkdir(outputDir);
   mkdir([outputDir,'W/']); mkdir([outputDir,'H/']);
   fid = fopen([outputDir,'properties.txt'],'w+');
   fprintf(fid,...
      ['type = pinhole\r\n',...
      'rank = %0.2d\r\n',...
      'horizontal views = %0.2d\r\n',...
      'vertical views = %0.2d\r\n'],...
      LF.dim(3)*LF.dim(4),LF.dim(4),LF.dim(3));
   fclose(fid);

   % Write pinhole array mask pairs.
   disp('  - Saving pinhole array mask pairs...');
   for k = 1:prod(LF.dim(3:4))
      W = zeros([display.res display.nChannels]);
      H = zeros([display.res display.nChannels]);
      for ch = 1:display.nChannels
         W(:,:,ch) = reshape(LF.data.pinhole_W{ch}(:,k),LF.dim([2 1]))';
         H(:,:,ch) = reshape(LF.data.pinhole_H{ch}(k,:),LF.dim([2 1]))';
      end
      W = W.^(1/display.outGamma);
      H = H.^(1/display.outGamma);
      imwrite(uint8(255*W),...
         [outputDir,'W/',num2str(k,image.frameCount),'.',image.frameExt]);
      imwrite(uint8(255*H),...
         [outputDir,'H/',num2str(k,image.frameCount),'.',image.frameExt]);
   end

   % Write content-adaptive parallax barriers.
   if NMF.enable
      
      % Create output directory.
      outputDir = [image.frameDir,'masks/NMF/'];
      warning('off','MATLAB:MKDIR:DirectoryExists');
      mkdir(outputDir);
      warning('on','MATLAB:MKDIR:DirectoryExists');
      rmdir(outputDir,'s'); mkdir(outputDir);
      mkdir([outputDir,'W/']); mkdir([outputDir,'H/']);
      fid = fopen([outputDir,'properties.txt'],'w+');
      fprintf(fid,...
         ['type = NMF\r\n',...
         'rank = %0.2d\r\n',...
         'horizontal views = %0.2d\r\n',...
         'vertical views = %0.2d\r\n'],...
         NMF.numPairs,LF.dim(4),LF.dim(3));
      fclose(fid);
      
      % Write content-adaptive parallax barriers.
      disp('  - Saving content-adaptive parallax barriers...');
      delete('./masks/NMF/*.png');
      for k = 1:NMF.numPairs
         W = zeros([display.res display.nChannels]);
         H = zeros([display.res display.nChannels]);
         for ch = 1:display.nChannels
            W(:,:,ch) = reshape(LF.data.NMF_W{ch}(:,k),LF.dim([2 1]))';
            H(:,:,ch) = reshape(LF.data.NMF_H{ch}(k,:),LF.dim([2 1]))';
         end
         W = W.^(1/display.outGamma);
         H = H.^(1/display.outGamma);
         imwrite(uint8(255*W),...
            [outputDir,'W/',num2str(k,image.frameCount),'.',image.frameExt]);
         imwrite(uint8(255*H),...
            [outputDir,'H/',num2str(k,image.frameCount),'.',image.frameExt]);
      end
   end

   % Clear temporary variables.
   clear W H;
   
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Display results.

% Display status.
disp('> Displaying results...');

% Display the pinhole array mask pairs.
disp('  + Displaying pinhole array mask pairs...');
figure(1); clf;
set(gcf,'Name','Pinhole Array Mask Pairs');
for k = 1:prod(LF.dim(3:4))
   W = zeros([display.res display.nChannels]);
   H = zeros([display.res display.nChannels]);
   for ch = 1:display.nChannels
      W(:,:,ch) = reshape(LF.data.pinhole_W{ch}(:,k),LF.dim([2 1]))';
      H(:,:,ch) = reshape(LF.data.pinhole_H{ch}(k,:),LF.dim([2 1]))';
   end
   W = W.^(1/display.outGamma);
   H = H.^(1/display.outGamma);
   imagesc(cat(2,uint8(255*[W H])),[0 255]);
   set(gca,'Pos',[0 0 1 1]);
   axis image off; colormap(gray(256));
   pause(0.2);
end

% Display content-adaptive parallax barriers.
if NMF.enable
   disp('  + Displaying content-adaptive parallax barriers...');
   figure(2); clf;
   set(gcf,'Name','Content-Adaptive Parallax Barriers');
   for k = 1:NMF.numPairs
      W = zeros([display.res display.nChannels]);
      H = zeros([display.res display.nChannels]);
      for ch = 1:display.nChannels
         W(:,:,ch) = reshape(LF.data.NMF_W{ch}(:,k),LF.dim([2 1]))';
         H(:,:,ch) = reshape(LF.data.NMF_H{ch}(k,:),LF.dim([2 1]))';
      end
      W = W.^(1/display.outGamma);
      H = H.^(1/display.outGamma);
      imagesc(cat(2,uint8(255*[W H])),[0 255]);
      set(gca,'Pos',[0 0 1 1]);
      axis image off; colormap(gray(256));
      pause(0.2);
   end
end
  
% Compare reconstruction of skewed orthographic views for input light field.
disp('  + Displaying light field reconstruction results...');
figure(3); clf;
set(gcf,'Name','Light Field Reconstrution Results');
for bIdx = LF.dim(3):-1:1
   for aIdx = LF.dim(4):-1:1
      if NMF.enable
         I = cat(2,[squeeze(LF.data.pinhole(:,:,bIdx,aIdx,:))...
                    squeeze(LF.data.NMF(:,:,bIdx,aIdx,:))]/max(LF.data.NMF(:)));
      else
         I = squeeze(LF.data.pinhole(:,:,bIdx,aIdx,:));
      end
      I = I.^(1/display.outGamma);
      imagesc(uint8(255*I),[0 255]);
      set(gca,'Pos',[0 0 1 1]);
      axis image off; colormap(gray(256));
      pause(0.4);
   end
end

% Plot PSNR of light field approximation (if calculated).
if NMF.enable && NMF.useMEX
   disp('  + Plotting PSNR as a function of iteration index...');
   figure(4); clf;
   set(gcf,'Name','PSNR of Light Field Approximation');
   if display.fullColor
      colorOrder = {'r','g','b'};
   else
      colorOrder{1} = 'b';
   end
   for ch = 1:display.nChannels
      E = [LF.data.NMF_E{:}];
      maxE = max(max(E(isfinite(E))));
      if isempty(maxE)
         maxE = 1000;
      end
      E = LF.data.NMF_E{ch};
      E(~isfinite(E)) = 1.1*maxE;
      plot(1:length(E),E,'-',...
         'LineWidth',3,'Color',colorOrder{ch});
      hold on;
   end
   hold off; axis tight; grid on;
   ylim([0 maxE]);
   xlabel('Iteration Index', 'FontWeight','bold','FontSize',14);
   ylabel('PSNR (dB)', 'FontWeight','bold','FontSize',14);
   set(gca,'LineWidth',2,'FontWeight','bold','FontSize',14);
end

% Clear temporary variables.
clear W H E;
