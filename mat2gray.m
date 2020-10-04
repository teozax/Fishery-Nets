function I = mat2gray (M, scale)

  if (nargin < 1|| nargin > 2)
    usage ("mat2gray(...) number of arguments must be 1 or 2");
  end

  [x,y,k] = size(M);
  
  if (k ~= 1)
    usage ("mat2gray(M,...) M must be a matrix");
  end

  if (nargin == 1)
    Mmin = min (min (M));
    Mmax = max (max (M));
  else 
  	if (is_vector (scale))
      Mmin = min (scale (1), scale (2));
      Mmax = max (scale (1), scale (2));
    end
  end

  I = (M < Mmin) .* 0;
  I = I + (M >= Mmin & M < Mmax) .* (1 / (Mmax - Mmin) * (M - Mmin));
  I = I + (M >= Mmax);

end 

