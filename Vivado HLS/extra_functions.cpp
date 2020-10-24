#include "core.h"

// perform 2D filtering
void mean_filter_2D(GRAY_IMAGE& img_in, GRAY_IMAGE& img_out)
{
	hls::Point_<int> anchor;
	anchor.x = -1;
	anchor.y = -1;

	const uint16_t coef_v[FILTER_HEIGHT][FILTER_WIDTH]= {
	  {1,1,1,1,1,1,1,1,1},
	  {1,1,1,1,1,1,1,1,1},
	  {1,1,1,1,1,1,1,1,1},
	  {1,1,1,1,1,1,1,1,1},
	  {1,1,1,1,1,1,1,1,1},
	  {1,1,1,1,1,1,1,1,1},
	  {1,1,1,1,1,1,1,1,1},
	  {1,1,1,1,1,1,1,1,1},
	  {1,1,1,1,1,1,1,1,1}
	};

	hls::Window<FILTER_HEIGHT, FILTER_WIDTH, uint16_t> mask;
	for (int r=0; r<FILTER_HEIGHT; r++)
		for (int c=0; c<FILTER_WIDTH; c++)
		{
			#pragma HLS PIPELINE
			mask.val[r][c] = coef_v[r][c];
		}

	hls::Filter2D <hls::BORDER_CONSTANT> (img_in, img_out, mask, anchor);

}

// make the structuring element
void strel (hls::Window<WINDOW_HEIGHT, WINDOW_WIDTH, uint16_t> *disk)
{

	const uint16_t coef_v[WINDOW_HEIGHT][WINDOW_WIDTH]= {
	  {0,0,1,1,1,1,1,0,0},
	  {0,1,1,1,1,1,1,1,0},
	  {1,1,1,1,1,1,1,1,1},
	  {1,1,1,1,1,1,1,1,1},
	  {1,1,1,1,1,1,1,1,1},
	  {1,1,1,1,1,1,1,1,1},
	  {1,1,1,1,1,1,1,1,1},
	  {0,1,1,1,1,1,1,1,0},
	  {0,0,1,1,1,1,1,0,0}
	};


	for (int r=0; r<WINDOW_HEIGHT; r++)
		for (int c=0; c<WINDOW_WIDTH; c++)
		{
			#pragma HLS PIPELINE
			disk->val[r][c] = coef_v[r][c];
		}

}

// take the nets and put them into blue component, and holes into red component
void AXIstream2Mat (my_data_fifo&  my_fifo, my_data_fifo&  my_fifo1, RGB_IMAGE& my_image)
{
	my_float f, f1;
	RGB_PIXEL pixel;
	for (int i = 0; i < MAX_HEIGHT; i++)
		for (int j = 0; j < MAX_WIDTH; j++)
		{
			#pragma HLS PIPELINE
				my_fifo>>f;
				my_fifo1>>f1;
				// blue component
				pixel.val[0] = (f1==1) ? 255 : 0;
				// green component
				pixel.val[1] = 0;
				// red component
				pixel.val[2] = (f==-1) ? 255 : 0;
				my_image<<pixel;
		}
}


// this function is needed before 2D filtering according to Matlab code
void mat2gray (GRAY_IMAGE& M,  GRAY_IMAGE& I, uint16_t min_value, uint16_t max_value)
{
	my_float i, max, min, m ;
	GRAY_PIXEL pixel, pixel1;

	 max=max_value;
	 min=min_value;

	for (my_int x=0; x<MAX_HEIGHT; x++)
	{	for (my_int y=0; y<MAX_WIDTH; y++)
    	{
				#pragma HLS PIPELINE
				M>>pixel;
				m = pixel.val[0];
				if ( (m >= min) && (m < max) )
					i = ( ((m - min) / (max - min))  );
				else
					i = 1;
				i = i * 100;
				pixel1.val[0] = i;
				I<<pixel1;
	     }
	}
}

// This is the respective function of Matlab
void adaptive_threshold (GRAY_IMAGE& IN, GRAY_IMAGE& IN1, my_data_fifo& Luminance_img, my_float C)
{
	my_float pix, pix1, pix2;
	GRAY_PIXEL pixel, pixel1;
	for (my_int x=0; x<MAX_HEIGHT; x++)
	{	for (my_int y=0; y<MAX_WIDTH; y++)
    {
				#pragma HLS PIPELINE
	    		IN>>pixel;
	    		IN1>>pixel1;
	    		pix = pixel.val[0];
					// reverse this value because they have been multiplicated with 10 at the end of guided filter (compute_I_enhanced)
	    		pix = pix / 10;
	    		pix1 = pixel1.val[0];
					// reverse this value because it was multiplicted with 10 at compute_I_enhanced and also with 100 at mat2gray
	    		pix1 = pix1 / 1000;
	    		pix2 = pix1 - pix - C;
	    		pix2 = (pix2<=0) ? 1 : 0;
	    		Luminance_img<<pix2;
    }
	}
}

void threshold (GRAY_IMAGE& IN, my_data_fifo& Luminance_img, my_float thres)
{
	my_float pix;
	GRAY_PIXEL pixel;
	for (my_int x=0; x<MAX_HEIGHT; x++)
	{	for (my_int y=0; y<MAX_WIDTH; y++)
    	{
			#pragma HLS PIPELINE
    		IN>>pixel;
    		pix = pixel.val[0];
				// reverse this value because they have been multiplicated with 10 at the end of guided filter (compute_I_enhanced)
    		pix = pix / 10;
    		pix = (pix<=thres) ? 1 : 0;
    		Luminance_img<<pix;
    	}
	}

}



void windows (my_data_fifo& window,
		my_float global_median,
		uint16_t row,
		uint16_t col,
		my_float SI [MAX_HEIGHT][MAX_WIDTH],
		my_float sizes [MAX_HEIGHT][MAX_WIDTH],
		my_float holes [MAX_HEIGHT][MAX_WIDTH]
		)
{
	#pragma HLS INLINE OFF
	my_float  even[lab_nums];
	my_float window_sizes[lab_nums] ;
	#pragma HLS array_partition variable=even cyclic factor=200
	#pragma HLS array_partition variable=window_sizes cyclic factor=200


	int N;
	int s, s1, s2;

//	 	       			for(int j=0;j<MAX_HEIGHT;j++)
//	 	       			{
//	 	       				for(int i=0;i<MAX_WIDTH;i++)
//	 	       				{
//	 	       				#pragma HLS UNROLL
//	 	       					sizes[j][i] = 0;
//	 	       				}
//	 	       			}

	// take the sizes for each window
	loop1: for (int y = 0;y<lab_nums;y++)
	{
		#pragma HLS PIPELINE
		window >>  window_sizes[y];
	}
	// sort the data
	local_sort (window_sizes, even, &N);

	my_float window_median, local_median;
	int e = 0, pos = 0;
	window_median = window_sizes[N/2];
	// perform the same code as in Matlab
	loop2:while ( (e < N) && ( ( window_sizes[e]< window_median/2 ) || (window_sizes[e]<=2) ) )
	{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=200
		e++;
	}
	int error = 0; int start = e;

	loop3:while ( (e < N) && (error == 0) )
	{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=200
		local_median = window_sizes[(start+e)/2];
		// check for faulty values inside window_sizes
	 	if ( (window_sizes[e] > local_median * 2) || (window_sizes[e] > global_median* 4) )
	 	{
			pos = e;
		 	error = 1;
	 	}else if (e < N-1)
	 	{
			if ( window_sizes[e] + local_median < window_sizes[e+1] )
	 		{
				pos = e + 1;
	 	    error = 1;
	 	  }
	 	}
	 	e++;
	}

	if (error == 1)	// for all faulty values
	{
		loop4:for (int t = pos; t<N; t++)
	 	{
	 		#pragma HLS LOOP_TRIPCOUNT min=1 max=200
			#pragma HLS PIPELINE
		 	s = window_sizes[t];	// for each faulty size, s, calculate a unique division number and a unique modulo number, so that they form a unique position at array sizes
		 	s1 = s/MAX_WIDTH;	// s1 will not exceed MAX_HEIGHT
		 	s2 = s%MAX_WIDTH;	// s2 will not exceed MAX_WIDTH
		 	sizes[s1][s2] = -1;	// mark that position with -1 to declare a faulty value. Note that if two different windows have a same faulty size they will indicate the same position at sizes, so we do not even need to initialize array sizes more than once
	 	}
		// take the size, s, for every position of the selected window and mark the respective position on array holes with the value of sizes at position [s1][s2]
	 	loop5:for(int m=0; m<50; m++){
		 	loop6:for(int n=0; n<50; n++){
				#pragma HLS PIPELINE
				s = SI[row+m][col+n];
				s1 = s/MAX_WIDTH;
				s2 = s%MAX_WIDTH;
				holes[row+m][col+n] = sizes[s1][s2];	// row and col are the start variables of x and y coordinates respectively, so by moving for 50 places on x and y we take the exact locations of the selected window
	 	  }
		}
	}

}
