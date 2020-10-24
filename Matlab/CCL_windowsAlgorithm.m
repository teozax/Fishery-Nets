function [results, Labels] = CCL_windowsAlgorithm(frame)
    img = frame(:,:,1); % get input frame

    [rows,columns,~] = size(img); % get height and width dimensions of frame (symbol '~' means we do not care/there is not 3rd dimension)
    results = zeros(rows,columns,3);

    se = strel('disk', 5); % create structuring element as a disk with radius 5 pixels around its center
    background = imopen(img,se);  % perform image opening with the contribution of structuring element

    img = img - background; % subtract image opening result from image to clear the view of environment

    mean_val = 0.05;
    AdaptThresIm = adaptivethreshold(img,9,mean_val,0);
    Nets = AdaptThresIm;

    Luminance = 0.03;
    Luminance_img = img <= Luminance; %  + imcomplement(AdaptThresIm)
    Holes = Luminance_img;
    Holes = Holes > 0;

%     Holes = imcomplement(imfill(imcomplement(Holes),'holes'));
%     Nets = imcomplement(imfill(imcomplement(Nets),'holes'));
%     figure;imshow(Holes);

    connectivity = 4; % set connectivity parameter for the way that connected component labeling is going to be executed
    [L, n] = bwlabel(Luminance_img, connectivity); % this is CCL (connected component labeling)
    [L1] = L;
%     RGB_label = label2rgb(L1, @copper, 'c', 'shuffle');
%     figure;imshow(RGB_label);

    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    labels = unique(L); % store once each label of image L which contains all labels
    labels = labels(labels ~= 0); % get rid of label 0, because it means nets and NOT holes
    labels = [labels, zeros(n,1)];  % extend array's 'labels' columns to 2 for the purpose of storing the size of each label next to it
    % count lebels' size
    for i=1:rows
        for j=1:columns
            if (L(i,j) ~= 0)
               labels(L(i,j),2) = labels(L(i,j),2) + 1;
%                if (Nets(i,j) == 1)
%                     labels(L(i,j),3) = labels(L(i,j),3) + 1;
%                end
            end
        end
    end
    % q = labels(:,2);
    % maxq = max(q);
 if (n > 0) % if there were at least one labels found from CCL then perform windows algorithm
    global_median = unique(labels(:,2));  % unique function will remove the repetitions of the same sizes and it is also going to sort them
    global_median = median(global_median(1:ceil(size(global_median,1)))); % extract the middle value of the sorted sizes
    sizes = zeros(rows,columns);  % create a new 2D image
    % store each label's size to image 'sizes' with the help of array 'labels'
    for i=1:rows
        for j=1:columns
            if (L(i,j) ~= 0)
                sizes(i,j) = labels(L(i,j),2);
            end
        end
    end
    results = zeros(rows,columns);  % initialize image results
    pos = 0;
    [x,y] = size(Luminance_img);
    % offset determines the size of each window (here 50 x 50)
    offset = 50;     row = 1 - offset;  r = 0;  c = 0;

    while (r == 0) || (c == 0)  % when we both exceed array-image limits means that we have processed all windows
        row = row + offset; % 'row' defines the upper side of windows
        myrow = row + offset; % 'myrow' defines the under side of windows
        if(myrow > x) % if you exceed vertical limit of image then shift the window inside the image, such that its under side fits the last row of the image
           myrow = x;
           row = x - offset;
           r = 1; % notify that you have reached the last set of windows in the image
        end
        col = 1 - offset;
        c = 0;
        while c == 0  % when we exceed horizontal image limit means that we have to move to the windows that are the ones we just processed
            col = col + offset; % 'col' defines the left side of windows
            mycol = col + offset; % 'mycol' defines the right side of windows
            if (mycol > y)  % if you exceed horizontal limit of image then shift the window inside the image, such that its right side fits the last column of the image
                mycol = y;
                col = y - offset;
                c = 1;  % notify that you have reached the last window of the set
            end
            window = sizes(row:myrow, col:mycol);
            window_sizes = unique(window(window~=0));
            window_median = median(window_sizes,'omitnan');
            % [medrow, medcol] = find(window_sizes == local_median);
            x1 = size(window_sizes, 1);
            i = 1;

            if (x1 > 0) % if at least one label is found perform windows algorithm
                while (i <= x1) && (window_sizes(i) < window_median/2)  % overtake the very small sizes that will affect comparisons
                    i = i + 1;
                end
                error = 0;
                start = i;
                while (i <= x1) && (error == 0)
                    local_median = window_sizes(ceil((start+i)/2));
%                     if (local_median >= global_median/2)
                        if ((window_sizes(i) > local_median * 2) || (window_sizes(i) > global_median * 4))
                            pos = i;
                            error = 1;
                        elseif (i < x1)
                            if (window_sizes(i) + local_median < window_sizes(i+1))
                                pos = i + 1;
                                error = 1;
                            end
                        end
%                     end
                    if (error == 1) % in case of error mark at image 'results' the pixels of the current window that have the same size with the sizes inside array 'window_sizes' from the position of error until the end
                        for j = pos:x1  % from position of error until the end of 'window_sizes'
                            q = window_sizes(j);  % take the faulty size
                            sizes2 = sizes .* 0;  % initialize the help array 'sizes2'
                            sizes2(row:myrow, col:mycol) = sizes(row:myrow, col:mycol); % at the pixels of 'sizes2' where the current window corresponds, store the values/sizes of that window
                            results = results + (sizes2 == window_sizes(j)) .* (results ~= 1);  % update with value '1' the pixels at image 'results' that current window corresponds, and have the same size with the currently selected faulty size (sizes2 == window_sizes(j)), and haven't been already updated (results ~= 1)
                        end
                    end
                    i = i + 1;
                end
            end
        end
    end

    results(:,:,1) = results;
    results(:,:,2) = 0;
    results(:,:,3) = Nets;

%     results(1:30,:,2)=1;
%     results(rows-30:rows,:,2)=1;
%     results(:,1:30,2)=1;
%     results(:,columns-30:columns,2)=1;
%     figure, imshow(results);
    Labels = L;
 else % if no holes are detected then mark only the nets at final image 'results'
    results(:,:,1) = 1;
    results(:,:,2) = 1;
    results(:,:,3) = Nets;
    Labels = zeros(1,1);  % store only label 0 (nets)
 end
end
