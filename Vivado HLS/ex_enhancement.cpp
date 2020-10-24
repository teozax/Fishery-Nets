#include "core.h"

void ex_enhancement( RGB_IMAGE& inputImage, GRAY_IMAGE& I_enhanced )
{
	// radius of guidedfilter				// variable epsilon of guidedfilter
	my_int r = 16;									my_float eps = 0.01;
	static my_data_fifo ONES, I, I_COPY, I_2;

	#pragma HLS DATAFLOW
	read_data1(inputImage, I, I_COPY, I_2, ONES);
	guidedfilter (I, I_COPY, r, eps, I_2, ONES, I_enhanced);
}

void read_data1(RGB_IMAGE& inputImage,
							my_data_fifo& I,
							my_data_fifo& I_COPY,
							my_data_fifo& I_2,
							my_data_fifo& ONES)
{
	 	my_float i, i_2,ones = 1, my_i, i2, i1,mones;
		RGB_PIXEL rgb_pixel;

		for (int x=0; x<MAX_HEIGHT; x++)
	    {	for (int y=0; y<MAX_WIDTH; y++)
	    	{
				#pragma HLS PIPELINE
	    		inputImage>>rgb_pixel;
	    		i = rgb_pixel.val[2];
	    		i = i / 255;
	    		i_2 = i * i;
	    		I<< i;
	    		I_COPY<<i;
	    		I_2 << i_2;
	    		ONES<<ones;
		     }
		 }

}
