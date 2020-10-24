#include "core.h"
#include "ap_int.h"

void net_holes_detection(my_AXI_STREAM& INPUT_STREAM, my_AXI_STREAM& OUTPUT_STREAM)
{
	//Create AXI streaming interfaces for the core
	#pragma HLS INTERFACE s_axilite port=return     bundle=CONTROL_BUS
	#pragma HLS INTERFACE axis port=INPUT_STREAM
	#pragma HLS INTERFACE axis port=OUTPUT_STREAM

	 static my_data_fifo out, ONES, I, I_COPY,I_2, Luminance_img, LI,  help, help1;

	 RGB_IMAGE   img_0 (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  I_enhanced (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  Background (MAX_HEIGHT, MAX_WIDTH) ;
	 GRAY_IMAGE  Background1 (MAX_HEIGHT, MAX_WIDTH) ;
	 GRAY_IMAGE  Background2 (MAX_HEIGHT, MAX_WIDTH) ;
	 GRAY_IMAGE  Background3 (MAX_HEIGHT, MAX_WIDTH) ;
	 GRAY_IMAGE  Background4 (MAX_HEIGHT, MAX_WIDTH) ;
	 GRAY_IMAGE  Background5 (MAX_HEIGHT, MAX_WIDTH) ;
	 GRAY_IMAGE  Background6 (MAX_HEIGHT, MAX_WIDTH) ;
	 GRAY_IMAGE  Background7 (MAX_HEIGHT, MAX_WIDTH) ;
	 GRAY_IMAGE  Background8 (MAX_HEIGHT, MAX_WIDTH) ;
	 GRAY_IMAGE  I_enh1 (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  I_enh2 (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  img_1 (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  img_2 (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  Gabor_Out (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  img_3 (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  img_4 (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  img_5 (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  img_6 (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  img_7 (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  img_8 (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  img_9 (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  img_10 (MAX_HEIGHT, MAX_WIDTH);
	 GRAY_IMAGE  img_11 (MAX_HEIGHT, MAX_WIDTH);
	 RGB_IMAGE   img_12 (MAX_HEIGHT, MAX_WIDTH);

	// thresholds for simple threshold and adaptive_threshold respectively
	my_float thres = 0.05, thres1 = 1.2;
	uint16_t max_value = 0, min_value = 0;
	// useless variables, but necessary parameters for function MinMaxLoc
	hls::Point_<int> min_loc, max_loc;
	// structuring element
	hls::Window<WINDOW_HEIGHT, WINDOW_WIDTH, uint16_t> disk;



	#pragma HLS DATAFLOW
	 hls::AXIvideo2Mat(INPUT_STREAM, img_0);

	 ex_enhancement( img_0, I_enhanced );
	 hls::Duplicate(I_enhanced, I_enh1, I_enh2);

	 strel (&disk);
	 hls::Erode<0,1>(I_enh1, img_1, disk);
	 hls::Dilate<0,1>(img_1, Background, disk);
	 hls::AddWeighted(I_enh2, 1, Background, -1, 0, img_2);

	 hls::Duplicate(img_2, img_3, img_4);
	 hls::Duplicate(img_3, img_8, img_9);
	 hls::Duplicate(img_4, img_10, img_11);

	 hls::MinMaxLoc( img_10, &min_value, &max_value, min_loc, max_loc );
	 mat2gray (img_8, img_5, min_value,  max_value);
	 mean_filter_2D (img_5, img_6);
	 adaptive_threshold (img_9, img_6, help, thres1);

	 threshold (img_11, Luminance_img, thres);
	 CCL(Luminance_img, LI);

	 AXIstream2Mat (LI, help, img_12);

	 hls::Mat2AXIvideo(img_12, OUTPUT_STREAM);

}



void CCL (my_data_fifo& inputImage, my_data_fifo& outputImage)
{
	my_float  even[lab_nums];
	#pragma HLS array_partition variable=even cyclic factor=200
	my_float window_sizes[lab_nums] ;
	#pragma HLS array_partition variable=window_sizes cyclic factor=200
	my_data_fifo ws;	// fifo of window sizes
	#pragma HLS STREAM variable=ws DEPTH = 200 dim=1
	uint16_t local_median, previous, above;
	uint16_t row = 0, pos, Lx, Ly, Lj, New_label = 1,  posx, posy;
	uint16_t r=0, c=0, myrow, mycol, key=0,col;
	my_float P,P1,P2;
	Lj=0;int g=0;
	int i, j,div,mod,div1,mod1;
	int N = 1;
	my_float labels [7000];
	my_float results [MAX_HEIGHT][MAX_WIDTH];

	my_float SI [MAX_HEIGHT][MAX_WIDTH];	// Sizes Image
	my_float sizes [MAX_HEIGHT][MAX_WIDTH];
	#pragma HLS array_partition variable=SI complete

	my_float label = 1;
	labels[0] = 0;
	my_float pixel, temp, LabelUsed;

		inputImage>>pixel;
		// the very first pixel has to be checked alone, because it does not have predecessor pixels that have labels
		if (pixel!=0){	// if it is foreground, store a new label at it
			results[0][0] = label;
			labels[label] = label;
			label++;	// increment the label value, because it was just used and has to take a new value
		}else // if it is background
			results[0][0]=0;

		//first row
		first_row:for(i=1;i<MAX_WIDTH;i++)
		{
		#pragma HLS PIPELINE
			inputImage>>pixel;	// get pixel
			if(pixel==0)	// if it is foreground
			{
				results[0][i] = 0;
			}else	// if it is background
			{
				temp = results[0][i-1];	// get previous pixel's label
				if(temp!=0){	// if previous pixel has a label, then store it to current pixel
					results[0][i] = temp;
				}else{	// if previous pixel does not have a label, then store a new label to current pixel
					results[0][i]=label;
					labels[label] = label;
					label++;	// increment the label value, because it was just used and has to take a new value
				}
			}
		}

		//All the other rows
		for( j=1;j<MAX_HEIGHT;j++)
		{	for( i=0;i<MAX_WIDTH;i++)
			{
			#pragma HLS PIPELINE
				inputImage>>pixel;	// get pixel
				if(pixel!=0) // if it is foreground
				{
					previous = results[j][i-1];
					above = results[j-1][i];
					if( previous!=0 && above!=0)	// if both previous and above pixels have a label
					{
						if(previous>above) // if previous pixel has most recent (newer) label than above pixel (older), then store the label of above to current and previous pixels, such that all three belong to the same object
						{
							labels[previous]=labels[labels[above]];
							results[j][i]=above;
						}
						else // if above pixel has most recent (newer) label than previous pixel (older), then store the label of previous to current and above pixels, such that all three belong to the same object
						{
							labels[above]=labels[labels[previous]];
							results[j][i]=previous;
						}
					}else
					{	// if only one of the previous and above pixels have a tag, then store it to current pixel
						if(previous!=0){
							results[j][i]=previous;
						}else if(above!=0){
							results[j][i]=above;
						}
						else{	// if none of the previous and above pixels have a tag, then store the new tag (label) to current pixel
							results[j][i]=label;
							labels[label]=label;
							label++;	// increment the label value, because it was just used and has to take a new value
						}
					}
				}
				else{	// if it is background
					results[j][i]=0;
				}
			}
		}
		// assign labels on array results
		second_pass_Y:for( j=0;j<MAX_HEIGHT;j++){
			second_pass_X:for( i=0;i<MAX_WIDTH;i++){
				#pragma HLS PIPELINE
				my_float value = labels[results[j][i]];
				results[j][i] = labels[value];
			}
		}
		// init labels to use this array for the size of labels
		init_sizes_table:for( i=0;i<7000;i++)
		{
			#pragma HLS UNROLL factor=10
			labels[i] = 0;
		}
		//count each label's size
		count_label_size_Y:for( j=0;j<MAX_HEIGHT;j++){
			count_label_size_X:for( i=0;i<MAX_WIDTH;i++){
				#pragma HLS PIPELINE
				if (results[j][i]!=0)
					labels[results[j][i]]++;
			}
		}

		for (int i=0; i<=5; i++)	// sizes from 1 to 6 are very small holes and appear at most of the pixels of each window. It was observed that if we take out these sizes then the rest of the sizes
		{
			#pragma HLS UNROLL
			window_sizes[i] = i + 1;
		}

		int lab_size;
		N = 6;
		for(j=0;j<MAX_HEIGHT;j++)
		{	for(i=0;i<MAX_WIDTH;i++)
			{
			#pragma HLS PIPELINE
				sizes[j][i] = 0;
				// take the size of each label and put it into all the places of SI that match the places of results that have that label
				SI[j][i] = labels[results[j][i]];
				lab_size = labels[results[j][i]];
				// also put that size into window_sizes for the check of error
				if (lab_size > 6)
				{	window_sizes [N] = lab_size; N++;	}
			}
		}

		local_sort (window_sizes, even, &N);	// sort the sizes of the whole image
		my_float global_median = window_sizes [N/2];

	  exloop1: for (row=0; (r==0) || (c==0); row+=offset)	// when we both exceed array-image limits means that we have processed all windows
		{
			#pragma HLS LOOP_TRIPCOUNT min=6 max=6
			myrow = row + offset;
	    if (myrow > MAX_HEIGHT){	// if you exceed vertical limit of image then shift the window inside the image, such that its under side fits the last row of the image
				myrow = MAX_HEIGHT;	// 'myrow' defines the under side of windows
	      row = MAX_HEIGHT - offset;	// 'row' defines the upper side of windows
	    	r = 1;	//	notify that you have reached the last set of windows in the image
	    }
	    c = 0;

		  exloop2: for (col = 0; c==0; col+=offset)
			{
				mycol = col + offset;
	      if (mycol > MAX_WIDTH){	// if you exceed horizontal limit of image then shift the window inside the image, such that its right side fits the last column of the image
					mycol = MAX_WIDTH;	// 'mycol' defines the right side of windows
	      	col = MAX_WIDTH - offset;	// 'col' defines the left side of windows
	        c = 1;	// notify that you have reached the last window of the set
	      }
	      int m, n, k;
	      N = 0;
	      ws << 0;	// just pass 0 at the start of fifo to select later a better median size
				// for each window pass the sizes greater than 0 to fifo ws (window sizes). Size 0 means no size - no hole
	      loop4:for (m = row; m < row+50; m++)
	      {
	      	#pragma HLS LOOP_TRIPCOUNT min=50 max=50
	      	loop5:for (n = col ; n < col+50; n++)
	        {
			      #pragma HLS LOOP_TRIPCOUNT min=50 max=50
			      #pragma HLS PIPELINE
						// always be careful with the boundaries of the windows to avoid wrong comparisons.
		      	if ( (m==row) && (n==col) )	// if we are at the very first pixel of the window
	         	{
	          	if (SI[m][n]!=0){	// we check it individually if it has a size, because the previous and above pixels do not belong to the same window or do not exist
								ws <<  SI[m][n];
	              N++;
	         		}
	         	}
	          else if ( (m!=row) && (n==col) )	// pixels at first column of window except of the very first one
	         	{
	          	if ( (SI[m][n]!=0) && (SI[m][n]!=SI[m-1][n]) ){ // if the current pixel has a size and it is different than the above (the previous exceeds limits)
	            	ws <<   SI[m][n];
	              N++;
	         		}
	         	}
	          else if ( (m==row) && (n!=col) ) // pixels at first row of window except of the very first one
	         	{
	          	if ( (SI[m][n]!=0) && (SI[m][n]!=SI[m][n-1]) ){	// if the current pixel has a size and it is different than the previous (the above exceeds limits)
	            	ws <<   SI[m][n];
	              N++;
	         		}
	         	}
	          else if ( (SI[m][n]!=0) &&		// if we are at some pixel rather than these at first row and first column then we can check if it has a size and it is different from both previous and above pixels (no limits exceeded)
	             			(SI[m][n]!=SI[m][n-1]) &&
	         					(SI[m][n]!=SI[m-1][n]) )
	         	{
	          	ws <<   SI[m][n];
	            N++;
	         	}
	        }
	      }
				// Beacuse we always pass 200 labels to fifo ws, each window is going to have a number of sizes that will probably be less than 200. So, we have to set to 0 the values from the position of the last valid size of each window (N) until position 200
	      for (int y = N;y<lab_nums-1;y++)
				{
					#pragma HLS LOOP_TRIPCOUNT min=1 max=200
				  #pragma HLS PIPELINE
	      	ws <<  0;
	      }

	      if (N>0){	// if at least a size with value different than 0 exists in the window, start window processing
		    	windows (ws, global_median, row, col, SI, sizes, results);
	      }
			}
		}
		//pass results to outputImage
		for(int m=0; m<MAX_HEIGHT; m++)
			for(int n=0; n<MAX_WIDTH; n++){
				#pragma HLS PIPELINE
				outputImage<<results[m][n];
			}


}


void local_sort (my_float work_array [lab_nums], my_float even [lab_nums], int *data_size)
{
#pragma HLS INLINE off
	const int N = lab_nums;

	 loop60:for (int i = 0; i<lab_nums; i++)
	 {
		#pragma HLS UNROLL factor=10
		even[i] = 0;
	 }

	 // Sort the data
	 	bool sorting_completed = false;

	 	sort_loop: while (!sorting_completed)
	 	{
	 	#pragma HLS LOOP_TRIPCOUNT min=1 max=100
		#pragma HLS PIPELINE
	 		// even line of comparators
	 		sorting_completed = true;
	 		sort_even: for (int i = 0; i < (N / 2); i++)
	 		{
	 			if (work_array[2 * i] > work_array[2 * i + 1])
	 			{ 	sorting_completed = false;
	 				even[2 * i]= work_array[2 * i + 1];
	 				even[2 * i + 1] = work_array[2 * i];
	 			}
	 			else
	 			{ 	even[2 * i] = work_array[2 * i];
	 				even[2 * i + 1] = work_array[2 * i + 1];
	 			}
	 		}
	 		// odd line of comparators
	 		sort_odd: for (int j = 0; j < (N / 2 - 1); j++)
	 		{
	 			if (even[2 * j + 1] > even[2 * j + 2])
	 			{	sorting_completed = false;
	 				work_array[2 * j + 1] = even[2 * j + 2];
	 				work_array[2 * j + 2] = even[2 * j + 1];
	 			}
	 			else
	 			{ 	work_array[2 * j + 1]= even[2 * j + 1];
	 				work_array[2 * j + 2]= even[2 * j + 2];
	 			}
	 		}
	 		work_array[0] = even[0];
	 		work_array[N-1] = even[N-1];
	 	}

    int prev=0;
    loop9: for (int v = 0;v<N;v++)
    {
		#pragma HLS PIPELINE
    	if (work_array[v] != work_array[prev] )
    	{
    		work_array[++prev] = work_array[v];
    	}
    }

	*data_size = prev+1;
}
