
	#include "hls_video.h"
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdint.h>
	#include <ap_fixed.h>
	#include <iostream>
	#include <math.h>
	#include <ap_int.h>
	#include <hls_stream.h>

// Image dimensions -> 270 * 480
	#define  MAX_HEIGHT 270
	#define  MAX_WIDTH 480

//	#define  X_WINDOWS 6
//	#define  Y_WINDOWS 10

// overall elements in the input image 270 * 480 = 129.600
	#define  elements 129600
// this offset defines the size of windows which check if there is any hole inside them at CCL. Each window is transformed to a square of size offset * offset
	#define  offset 50

	#define  FILTER_HEIGHT 9
	#define FILTER_WIDTH 9

// These are the sizes of windows which are used for erode and dilate
	#define  WINDOW_HEIGHT 9
	#define WINDOW_WIDTH 9
// size of array for labels of each window
	#define lab_nums  200

	typedef ap_fixed<32, 16, AP_RND_CONV > my_float;	// define a float type with 32 bits overall with 16 of them forming the integer part
	typedef ap_fixed<8, 8, AP_RND_CONV > my_float1;
	typedef ap_int<30> my_int;	// define an integer with 30 bits

	typedef hls::stream<ap_axiu<24,1,1,1>> my_AXI_STREAM;	// define an axi streaming interface that has 3 channels (bgr:{rgb in reverse}) of 8 bits each (3*8 = 24)
	typedef hls::stream<my_float> my_data_fifo;	// define a fifo type that each of its places has the size of my_float

	typedef hls::LineBuffer<33, MAX_WIDTH, my_float>  BUFFER;	// Buffer with 33 rows and MAX_WIDTH columns
	typedef hls::LineBuffer<200, 1, my_float>  BUFFER1;
	typedef hls::LineBuffer<MAX_HEIGHT, MAX_WIDTH, my_float>  BUFFER2;

	typedef hls::Scalar<3, uint8_t> RGB_PIXEL;	// pixel that has 3 channels of values, each one taking over 8 bits that represent an unsigned integer (uint)
	typedef hls::Scalar<1, uint16_t> GRAY_PIXEL;	// pixel that has 1 channel of value, taking over 16 bits that represent an unsigned integer (uint)
	typedef hls::Scalar<1, uint16_t> GRAY_PIXEL1;

	typedef hls::Mat<MAX_HEIGHT,   MAX_WIDTH,   HLS_8UC3> RGB_IMAGE;	// MATrix with the form of a fifo that stores values of axi streaming interface 'my_AXI_STREAM'. Every place has 3 channels of 8 bits each and type unsigned character (UC) - identical to unsigned integer
	typedef hls::Mat<MAX_HEIGHT,   MAX_WIDTH,   HLS_16UC1  > GRAY_IMAGE;	// MATrix with the form of a fifo that has one channel and stores values of 16 bits length and type unsigned character (UC) - identical to unsigned integer
	typedef hls::Mat<MAX_HEIGHT,   MAX_WIDTH,   HLS_16UC1   > GRAY_IMAGE1;

	void mat2gray  (GRAY_IMAGE& M,  GRAY_IMAGE& I, uint16_t min_value, uint16_t max_value);
	void adaptive_threshold (GRAY_IMAGE& IN, GRAY_IMAGE& IN1, my_data_fifo& Luminance_img, my_float C);
	void threshold (GRAY_IMAGE& IN, my_data_fifo& Luminance_img, my_float thres);
	void net_holes_detection(my_AXI_STREAM& INPUT_STREAM, my_AXI_STREAM& OUTPUT_STREAM);
	void CCL (my_data_fifo& inputImage,
			my_data_fifo& outputImage
			);
	float rounding(float value);
	void strel (hls::Window<WINDOW_HEIGHT, WINDOW_WIDTH, uint16_t> *disk);
	void mean_filter_2D(GRAY_IMAGE& img_in, GRAY_IMAGE& img_out);
	void ex_enhancement(RGB_IMAGE& inputImage, GRAY_IMAGE&  I_enhanced);
	void guidedfilter(			my_data_fifo& I,
													my_data_fifo&  I_COPY,
													my_int r,
													my_float eps,
													my_data_fifo& I_2,
													my_data_fifo& ONES,
													GRAY_IMAGE& I_enhanced);
	void boxfilter(my_data_fifo& imSrc, my_int r, my_data_fifo& imDst);
	void histeq (GRAY_IMAGE& Img_In, GRAY_IMAGE& Img_Out);


	void compute_a_b (my_data_fifo& n, my_data_fifo& mean_i, my_data_fifo& mean_ii, my_float eps,
									  my_data_fifo& a, my_data_fifo& b, my_data_fifo&  N_COPY);
	void compute_I_enhanced (my_data_fifo& n, my_data_fifo& mean_A, my_data_fifo& mean_B,
			GRAY_IMAGE& I_enhanced, my_data_fifo& I);
	void read_data1(RGB_IMAGE& inputImage,
									my_data_fifo& I,
									my_data_fifo& I_COPY,
									my_data_fifo& I_2,
									my_data_fifo& ONES);
	void read_data2 ( my_data_fifo& I,
									my_data_fifo& I_2);

	void AXIstream2Mat (my_data_fifo&  my_fifo, my_data_fifo&  my_fifo1, RGB_IMAGE& my_image);

	void local_sort (my_float work_array [lab_nums], my_float even [lab_nums], int *data_size);

	void imOpen (	 GRAY_IMAGE&  Background ,
								 GRAY_IMAGE&  I_enh1 ,
								 GRAY_IMAGE& I_enh2 ,
								 GRAY_IMAGE&  img_1,
								 GRAY_IMAGE&  img_2  ,
								 hls::Window<WINDOW_HEIGHT, WINDOW_WIDTH, uint16_t> *disk);

	void windows (my_data_fifo& window,
//			my_float window_sizes [lab_nums],
//			my_float  even[lab_nums],
			my_float global_median,
			uint16_t row,
			uint16_t col,
			my_float SI[MAX_HEIGHT][MAX_WIDTH],
			my_float sizes[MAX_HEIGHT][MAX_WIDTH],
			my_float holes [MAX_HEIGHT][MAX_WIDTH]
									   );


	void gabor (my_data_fifo elem[200],
			my_float SI[MAX_HEIGHT][MAX_WIDTH],
			my_float sizes [MAX_HEIGHT][MAX_WIDTH],
			uint16_t row,
			uint16_t col,
			my_data_fifo & window_sizes,
			my_data_fifo& outputImage);

	void myfunc (my_data_fifo& input ,
			my_data_fifo& window_sizes,
			int row,
			int col,
			my_float SI[MAX_HEIGHT][MAX_WIDTH],
			my_float sizes[MAX_HEIGHT][MAX_WIDTH],
			my_data_fifo& outputImage);
