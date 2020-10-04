#include "core.h"

// dataflow between its child functions
void guidedfilter(	my_data_fifo&  I,
										my_data_fifo&  I_COPY,
										my_int r,
										my_float eps,
										my_data_fifo& I_2,
										my_data_fifo& ONES,
										GRAY_IMAGE& I_enhanced
								 )
{
	static my_data_fifo mean_I, mean_II, N, a, b, mean_A, mean_B, N_COPY;
	#pragma HLS STREAM variable=mean_I  DEPTH = 1  dim=1
	#pragma HLS STREAM variable=mean_II  DEPTH = 1  dim=1
	#pragma HLS STREAM variable=N  DEPTH = 1  dim=1
	#pragma HLS STREAM variable=a  DEPTH = 1  dim=1
	#pragma HLS STREAM variable=b  DEPTH = 1  dim=1
	#pragma HLS STREAM variable=mean_A  DEPTH = 1  dim=1
	#pragma HLS STREAM variable=mean_B  DEPTH = 1  dim=1
	#pragma HLS STREAM variable=N_COPY  DEPTH = 1  dim=1

	#pragma HLS DATAFLOW

	boxfilter (ONES, r, N);
	boxfilter (I, r, mean_I);
	boxfilter (I_2, r, mean_II);

	compute_a_b (N, mean_I, mean_II, eps, a, b, N_COPY);

	 boxfilter (a, r, mean_A);
	 boxfilter (b, r, mean_B);

	 compute_I_enhanced (N_COPY, mean_A, mean_B, I_enhanced, I_COPY);

}

// compute output image of guided filter
void compute_I_enhanced (my_data_fifo& n, my_data_fifo& mean_A, my_data_fifo& mean_B,
		GRAY_IMAGE& I_enhanced, my_data_fifo& I)
{
	my_float mean_a, mean_b, N, i, q;
	GRAY_PIXEL pixel;

	for (my_int x=0; x<MAX_HEIGHT; x++)
  {	for (my_int y=0; y<MAX_WIDTH; y++)
		{
				#pragma HLS PIPELINE
				mean_A>>mean_a;
				mean_B>>mean_b;
				n>>N;		I>>i;

				mean_a = mean_a / N;
				mean_b = mean_b / N;

				q = (mean_a * i + mean_b);
				q = (i - q) * 10 + q*3;

				// due to the fact that values of q range at [-1, 10], then i add 3 to them and then multiply with 10 to
				// take advantage of the ready functions of erode and dilate by vivado hls library that involve 8 bit
				// values [0, 255]. Note that erode and dilate just consider to apply the smallest or the largest value accordingly
				// inside the window, so all we need is just to move the values in range [0, 255] without changing the order between them.
				// Example: (0.21375 + 3)*10 = 3.21375 * 10 = 32.1375 and 32 will be the value for erode
				q = (q + 3) * 10;

				pixel.val[0] = q;
				I_enhanced<<pixel;
	   }
	 }
}

// compute parameters a and b
void compute_a_b (my_data_fifo& n, my_data_fifo& mean_i, my_data_fifo& mean_ii, my_float eps,
								  my_data_fifo& a, my_data_fifo& b, my_data_fifo&  N_COPY)
{
	my_float mean_I, mean_II, var_I, my_a, my_b, N ;

	 for (my_int x=0; x<MAX_HEIGHT; x++)
	 {	for (my_int y=0; y<MAX_WIDTH; y++)
    	{
			#pragma HLS PIPELINE
    		n>>N;
    		N_COPY<<N;
    		mean_i >> mean_I;
    		mean_ii >> mean_II;

    		mean_I = mean_I / N;
    		mean_II =mean_II / N;

    		var_I = mean_II - mean_I * mean_I;

    		my_a = var_I / (var_I + eps);
    	  a << my_a;
    	  my_b = mean_I - my_a * mean_I;
    	  b << my_b;
	     }
	  }
}


// cumulative sum over Y axis at the least possible time
void boxfilter(my_data_fifo& imSrc, my_int r, my_data_fifo& imDst)
{
	 my_float temp, temp1, temp2, temp3, temp4, temp5,temp6;
	 BUFFER imCum;
	 int myx = 1, myx1 = 0, x, y;


	for (x=0; x<MAX_HEIGHT+r; x++)
	{	for (y=0; y<MAX_WIDTH; y++)
		{
			#pragma HLS PIPELINE
			if (x==0)	// first row
			{
				imSrc>>temp6;
				imCum.val[0][y] = temp6;
			}
			else	// all other rows
			{
				temp4 = imCum.val[myx1][y];	// take the value at previous row and same column compared to the position that new value will be stored

				if (x<MAX_HEIGHT) // for rows 1 until MAX_HEIGHT - r of new image
				{
					if (x>=2*r+1)	// save the previous value of the position where new value will be stored
						temp = imCum.val[myx][y];

					imSrc>>temp2;	// get new value
					temp3 = temp2 + temp4;	// add to new value, the value at previous row and same column compared to the position that new value will be stored
					imCum.val[myx][y]  = temp3;	// store the sum into the position of new value
				}else	// for rows MAX_HEIGHT - r until MAX_HEIGHT of new image
					temp = imCum.val[myx][y];	// get the value at row MAX_HEIGHT and same column compared to incoming value

				if (x>=r && x<=2*r)	{
					imDst<<temp3;
				}
				else if (x>=2*r+1 && x<MAX_HEIGHT) {
					temp6 = temp3 - temp;
					imDst<<temp6;
				}
				else if (x>=MAX_HEIGHT) {
					temp1 = temp4 - temp ;
					imDst<<temp1;
				}


				if ( (x<MAX_HEIGHT)&&(y==MAX_WIDTH-1) ){
					if (myx1==2*r)
						myx1 = 0;
					else
						myx1++;
				}

				if (y==MAX_WIDTH-1){
					if (myx==2*r)
						myx = 0;
					else
						myx++;
				}
			}

		}
	}

 }
