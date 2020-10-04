function I_enhanced = ex_enhancement(frame)
% example: detail enhancement
I = frame(:,:,1);
% imwrite(frame,'C:\Users\theoz\Documents\MATLAB\1.tif');
I = double(I) / 255;

r = 16;
eps = 0.01;
p = I;

q = zeros(size(I));

q(:, :, 1) = guidedfilter(I(:, :, 1), p(:, :, 1), r, eps);  % only at red component we detect the differences between water and nets, so only that needs to be processed
% q(:, :, 2) = guidedfilter(I(:, :, 2), p(:, :, 2), r, eps);
% q(:, :, 3) = guidedfilter(I(:, :, 3), p(:, :, 3), r, eps);

I_enhanced = (I - q) * 10 + q*3;

Mmin = min (min (I_enhanced));
Mmax = max (max (I_enhanced));

%     img(:,:) = (img(:,:)>=255) .* 255 + (img(:,:)<255) .* img(:,:) .*  (img(:,:)>=0);


% figure();
% imshow(I_enhanced(:,:,1));

end
