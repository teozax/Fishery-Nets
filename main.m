clear all; close all; clc;

v = VideoReader('my.mp4');  % Open video file
thisFrame = readFrame(v); % get the first frame (this step is necessary for the extraction of frame size)
% time_interval = 0 ;
[x,y,~] = size(thisFrame);  % get height and width dimensions of frame (symbol '~' means we do not care/there is not 3rd dimension)

% while v.CurrentTime < 33
%     thisFrame = readFrame(v);
% end
figure;
resize_value = 4; % determine a scale factor to resize each frame for faster processing
previous_frame = zeros(x/resize_value, y/resize_value); % create a frame to store the previous of the current frame

while v.CurrentTime < 45  % until we reach the end of video
    thisFrame = readFrame(v); % read the next frame of video
    thisFrame = imresize(thisFrame, 1/resize_value); % resize the current frame, based on scale factor

    tic;
      I_enhanced = ex_enhancement(thisFrame);
      [results, Labels] = CCL_windowsAlgorithm(I_enhanced);
    toc;
%   new_frame = results;
    new_frame(:,:,1) = results(:,:,1) .* previous_frame(:,:,1); % red component
    new_frame(:,:,2) = 0; % green component
    new_frame(:,:,3) = results(:,:,3);  % blue component
%   new_frame(:,:,3) = (results(:,:,3) + previous_frame(:,:,3)-new_frame(:,:,2));

    R = new_frame(:,:,1); % get red component
    %1    restore the parts of the holes that got lost during multiplication of red components of results and previous_frame
    Red_labels = unique(R .* Labels); % get uniquely the labels of all detected holes into the list-array Red_labels
    Red_labels = Red_labels(Red_labels ~= 0); % get rid of label 0, because it means nets
    R = Labels .* 0; % initialize red component
    for i = 1:size(Red_labels)
        R = R + (Labels == Red_labels(i));  % remake the red component, but this time with the help of image Labels that contains all the pixels for each label of list Red_labels
    end
    R = R > 0;
    %1
    new_frame(:,:,1) = R; % store the final red component at new frame
    new_frame(:,:,2) = new_frame(:,:,1) .* new_frame(:,:,3);  % find the common pixels between extended holes (after previous processing) and nets
    new_frame(:,:,3) = new_frame(:,:,3) - new_frame(:,:,2); % remove parts of nets where common pixels with red component appear

    previous_frame(:,:,1) = results(:,:,1); % store the red component of new frame to be used for processing of next frame
    %       figure;imshow(previous_frame(:,:,1));
    thisFrame = imresize(thisFrame,resize_value);
    new_frame_r = imresize(new_frame,resize_value);

    %       close all;
    %       figure;
        imshowpair(thisFrame,new_frame_r,'montage');
end
%     imwrite(thisFrame,'C:\Users\theoz\Documents\MATLAB\6.tif');
