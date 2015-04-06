
#include "Globals.h"
#include "TargaImage.h"
#include "libtarga.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>
#include <time.h>
using namespace std;
using namespace stdext;

// constants
const int           RED             = 0;                // red channel
const int           GREEN           = 1;                // green channel
const int           BLUE            = 2;                // blue channel
const unsigned char BACKGROUND[3]   = { 0, 0, 0 };      // background color

struct RGB {
   float r;
   float g;
   float b;
   bool operator <(const RGB &A) const
   { return this->r < A.r; }
};

// Computes n choose s, efficiently
double Binomial(int n, int s)
{
    double        res;

    res = 1;
    for (int i = 1 ; i <= s ; i++)
        res = (n - i + 1) * res / i ;

    return res;
}// Binomial

void convolve(unsigned char* in, unsigned char* out, int dataSizeX, int dataSizeY,
                    float* kernel, int kernelSizeX, int kernelSizeY)
{
    int i, j, m, n, mm, nn;
    int kCenterX, kCenterY;                         // center index of kernel
    float sum;                                      // temp accumulation buffer
    int rowIndex, colIndex;

    // find center position of kernel (half of kernel size)
    kCenterX = kernelSizeX / 2;
    kCenterY = kernelSizeY / 2;

    for(i=0; i < dataSizeY; ++i)                // rows
    {
        for(j=0; j < dataSizeX; ++j)            // columns
        {
            sum = 0;                            // init to 0 before sum
            for(m=0; m < kernelSizeY; ++m)      // kernel rows
            {
                mm = kernelSizeY - 1 - m;       // row index of flipped kernel

                for(n=0; n < kernelSizeX; ++n)  // kernel columns
                {
                    nn = kernelSizeX - 1 - n;   // column index of flipped kernel

                    // index of input signal, used for checking boundary
                    rowIndex = i + m - kCenterY;
                    colIndex = j + n - kCenterX;

                    // ignore input samples which are out of bound
                    if(rowIndex >= 0 && rowIndex < dataSizeY && colIndex >= 0 && colIndex < dataSizeX)
                        sum += in[dataSizeX * rowIndex + colIndex] * kernel[kernelSizeX * mm + nn];
                }
            }
            out[dataSizeX * i + j] = (unsigned char)((float)fabs(sum) + 0.5f);
        }
    }
}

map< int, RGB > converse_map( const map< RGB, int >& o )
{
  map< int, RGB > result;
  for ( map< RGB, int >::const_iterator begin( o.begin() );begin != o.end(); ++begin )
     result.insert( make_pair( begin->second, begin->first ) );
  return result;
}
///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage() : width(0), height(0), data(NULL)
{}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h) : width(w), height(h)
{
   data = new unsigned char[width * height * 4];
   ClearToBlack();
}// TargaImage



///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables to values given.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h, unsigned char *d)
{
    int i;

    width = w;
    height = h;
    data = new unsigned char[width * height * 4];

    for (i = 0; i < width * height * 4; i++)
	    data[i] = d[i];
}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Copy Constructor.  Initialize member to that of input
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(const TargaImage& image) 
{
   width = image.width;
   height = image.height;
   data = NULL; 
   if (image.data != NULL) {
      data = new unsigned char[width * height * 4];
      memcpy(data, image.data, sizeof(unsigned char) * width * height * 4);
   }
}


///////////////////////////////////////////////////////////////////////////////
//
//      Destructor.  Free image memory.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::~TargaImage()
{
    if (data)
        delete[] data;
}// ~TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Converts an image to RGB form, and returns the rgb pixel data - 24 
//  bits per pixel. The returned space should be deleted when no longer 
//  required.
//
///////////////////////////////////////////////////////////////////////////////
unsigned char* TargaImage::To_RGB(void)
{
    unsigned char   *rgb = new unsigned char[width * height * 3];
    int		    i, j;

    if (! data)
	    return NULL;

    // Divide out the alpha
    for (i = 0 ; i < height ; i++)
    {
	    int in_offset = i * width * 4;
	    int out_offset = i * width * 3;

	    for (j = 0 ; j < width ; j++)
        {
	        RGBA_To_RGB(data + (in_offset + j*4), rgb + (out_offset + j*3));
	    }
    }

    return rgb;
}// TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Save the image to a targa file. Returns 1 on success, 0 on failure.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Save_Image(const char *filename)
{
    TargaImage	*out_image = Reverse_Rows();

    if (! out_image)
	    return false;

    if (!tga_write_raw(filename, width, height, out_image->data, TGA_TRUECOLOR_32))
    {
	    cout << "TGA Save Error: %s\n", tga_error_string(tga_get_last_error());
	    return false;
    }

    delete out_image;

    return true;
}// Save_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Load a targa image from a file.  Return a new TargaImage object which 
//  must be deleted by caller.  Return NULL on failure.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Load_Image(char *filename)
{
    unsigned char   *temp_data;
    TargaImage	    *temp_image;
    TargaImage	    *result;
    int		        width, height;

    if (!filename)
    {
        cout << "No filename given." << endl;
        return NULL;
    }// if

    temp_data = (unsigned char*)tga_load(filename, &width, &height, TGA_TRUECOLOR_32);
    if (!temp_data)
    {
        cout << "TGA Error: %s\n", tga_error_string(tga_get_last_error());
	    width = height = 0;
	    return NULL;
    }
    temp_image = new TargaImage(width, height, temp_data);
    free(temp_data);

    result = temp_image->Reverse_Rows();

    delete temp_image;

    return result;
}// Load_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Convert image to grayscale.  Red, green, and blue channels should all 
//  contain grayscale value.  Alpha channel shoould be left unchanged.  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::To_Grayscale()
{
    float grayscale;

	for(int i=0; i<width * height * 4;i += 4){
		grayscale = 0.299 * data[i] + 0.587 * data[i+1] + 0.114 * data[i+2];
		data[i] = grayscale;
		data[i+1] = grayscale;
		data[i+2] = grayscale;
	}

    return true;
}// To_Grayscale


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using uniform quantization.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Uniform()
{
	for(int i=0; i < width * height * 4; i += 4){
	   float r = data[i]/32;
	   float g = data[i+1]/32;
	   float b = data[i+2]/64;
	   data[i] = (int)(r * 32);
	   data[i+1] = (int)(g * 32);
	   data[i+2] = (int)(b * 64);
	}

    return true;
}// Quant_Uniform

///////////////////////////////////////////////////////////////////////////////
//
//      Convert the image to an 8 bit image using populosity quantization.  
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Populosity()
{
    map<RGB,int> histo;
	map<int,RGB> converse_histo;
	map<RGB,int>::iterator iter;
	map<int,RGB>::iterator iter1;
	map<int,RGB> color_256;
    float distance,temp = 0.0, red, green,blue;
	RGB color, closest;
	int sum = 0;
	closest.r = 0;
	closest.g = 0;
	closest.b = 0;
   /* Uniform quantization */
   for(int i=0; i < width * height * 4; i += 4){
	   float r = (float)data[i]/1024;
	   float g = (float)data[i+1]/1024;
	   float b = (float)data[i+2]/1024;
	   float t = (float)r * 1024;
	   data[i] = r * 1024;
	   data[i+1] = g * 1024;
	   data[i+2] = b * 1024;
	}
   /* histogram */
   color.r = data[0]; 
   color.g = data[1]; 
   color.b = data[2];
   histo.insert(map<RGB,int>::value_type(color,1));
   for(int i = 4;i < width * height * 4;i += 4){
	   color.r = data[i]; 
	   color.g = data[i+1];
	   color.b = data[i+2];
	   iter = histo.find(color);
	   if(iter != histo.end())
		   iter->second += 1;
	   else
		   histo.insert(map<RGB,int>::value_type(color,1));
   }
  // converse_histo = converse_map(histo);
 /*  histo.clear();
   for(iter1 = converse_histo.end(); iter1 != converse_histo.begin();iter1--){
	   sum += 1;
	   color_256.insert(make_pair(iter1->first,iter1->second));
	   if(sum >= 256) break;	   
   }
   converse_histo.clear();*/
   for(int i = 0;i < width * height * 4;i += 4){
	   color.r = data[i];
	   color.g = data[i+1];
	   color.b = data[i+2];
	   distance = 500;
	   //iter1 = color_256.find(color);
	   //if(iter1 != color_256.end()){ continue; }
	   for(iter = histo.begin();iter != histo.end();iter++){
		   red = pow((float)(iter->first.r - data[i]),2);
		   green = pow(iter->first.g - data[i+1],2);
		   blue = pow(iter->first.b - data[i+2],2);
		   temp = sqrt(red + green + blue);
		   if(temp == 0){
				closest.r = iter->first.r;
			    closest.g = iter->first.g;
			    closest.b = iter->first.b;
		   }
		   else if(temp < (float)distance){
			   distance = temp;
			   closest.r = iter->first.r;
			   closest.g = iter->first.g;
			   closest.b = iter->first.b;
		   }
	   }	   
	   data[i] = closest.r;
	   data[i+1] = closest.g;
	   data[i+2] = closest.b;
   }

    return true;
}// Quant_Populosity


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image using a threshold of 1/2.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Threshold()
{
	float grayscale;
	for(int i=0; i<width * height * 4;i += 4){
	   grayscale = 0.299 * data[i]/255 + 0.587 * data[i+1]/255 + 0.114 * data[i+2]/255;   // [0,255] change to [0,1] scale
		if( grayscale < 0.5){
		  data[i] = 0;
		  data[i+1] = 0;
		  data[i+2] = 0;
		}
		else{
		  data[i] = 255;
		  data[i+1] = 255;
		  data[i+2] = 255;
		}
	}

    return true;
}// Dither_Threshold


///////////////////////////////////////////////////////////////////////////////
//
//      Dither image using random dithering.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Random()
{
    float grayscale;
	for(int i=0; i< width * height * 4;i += 4){
	   grayscale = 0.299 * data[i]/255 + 0.587 * data[i+1]/255 + 0.114 * data[i+2]/255;
	   grayscale += ((float)rand())/RAND_MAX * 0.4 - 0.2;
		if( grayscale < 0.5){
		  data[i] = 0;
		  data[i+1] = 0;
		  data[i+2] = 0;
		}
		else{
		  data[i] = 255;
		  data[i+1] = 255;
		  data[i+2] = 255;
		}
	}

    return true;
}// Dither_Random


///////////////////////////////////////////////////////////////////////////////
//
//      Perform Floyd-Steinberg dithering on the image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_FS()
{
    ClearToBlack();
    return false;
}// Dither_FS


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image while conserving the average brightness.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Bright()
{
	int count = 0,i;
        float intensity_avg = 0, threshold = 0;
	float *grayscale = new float[width * height];

	/* calculate the average intensity over image */
	for(int i=0; i< width * height * 4;i += 4){
	   grayscale[i/4] = 0.299 * data[i] / 255 + 0.587 * data[i+1] / 255 + 0.114 * data[i+2] / 255;
//	   data[i] = grayscale[i/4];
///       data[i + 1] = grayscale[i/4];
//	   data[i + 2] = grayscale[i/4];
           intensity_avg += grayscale[i/4];
	}
	intensity_avg = intensity_avg / (width * height);

	sort(grayscale,grayscale + width * height);

   /* find a threshold */
    
	for(i=0; i< width * height * 4;i += 4){
	   int index = (1-intensity_avg) * width * height;
	   float tmp = 0.299 * data[i] / 255 + 0.587 * data[i+1] / 255 + 0.114 * data[i+2] / 255;
	   if( tmp < grayscale[index])
	   {
		   data[i] = 0;
		   data[i + 1] = 0;
		   data[i + 2] = 0;
	   }
	   else{
		   data[i] = 255;
		   data[i + 1] = 255;
		   data[i + 2] = 255;
	   }
	}
    return true;
}// Dither_Bright


///////////////////////////////////////////////////////////////////////////////
//
//      Perform clustered differing of the image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Cluster()
{
	float mask[4][4] = { 0.7500, 0.3750, 0.6250, 0.2500, 
	                     0.0625, 1.0000, 0.8750, 0.4375,
	                     0.5000, 0.8125, 0.9375, 0.1250,
	                     0.1875, 0.5625, 0.3125, 0.6875 };
    float matrix[400][400];
	float grayscale;
    int x, y;
	/* calculate the gray value for each pixel and store it into matrix */
	for(y = 0;y < height;y++)
		for(x = 0;x < width;x++)
		{
		   matrix[x][y] = 0.299 * data[(y * width + x) * 4] / 255 + 0.587 * data[(y * width + x) * 4 + 1] / 255 + 0.114 * data[(y * width + x) * 4 + 2] / 255; 
		}
	/* Dither matrix */
    for(y = 0;y < height;y++)
		for(x = 0;x < width;x++)
		{
			if(matrix[x][y] >= mask[x%4][y%4])
				matrix[x][y] = 255;
			else matrix[x][y] = 0;
		}
	/* Store the new matrix into image data */
	for(y = 0;y < height;y++)
		for(x = 0;x < width;x++)
		{
			data[(y * width + x) * 4] = matrix[x][y];
            data[(y * width + x) * 4 + 1] = matrix[x][y];
			data[(y * width + x) * 4 + 2] = matrix[x][y];
		}
 
    return true;
}// Dither_Cluster


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using Floyd-Steinberg dithering over
//  a uniform quantization - the same quantization as in Quant_Uniform.
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Color()
{
    ClearToBlack();
    return false;
}// Dither_Color


///////////////////////////////////////////////////////////////////////////////
//
//      Composite the current image over the given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Over(TargaImage* pImage)
{
	float alpha;
	if (width != pImage->width || height != pImage->height)
    {
        cout <<  "Comp_Over: Images not the same size\n";
        return false;
    }
    for(int i = 0;i < height * width * 4;i += 4)
	{
	    alpha = (float)data[i+3] / 255;

		data[i] = data[i] + (1 - alpha) * pImage->data[i];
		data[i+1] = data[i+1] + (1 - alpha) * pImage->data[i+1];
		data[i+2] = data[i+2] + (1 - alpha) * pImage->data[i+2];
		data[i+3] = data[i+3] + (1 - alpha) * pImage->data[i+3];
	}
    
    return true;
}// Comp_Over


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "in" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_In(TargaImage* pImage)
{
    //unsigned char        rgb1[3];

	if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_In: Images not the same size\n";
        return false;
    }
	for(int i = 0;i < height * width * 4;i += 4){
		
		float alpha;
		if((float)pImage->data[i+3] / 255 == 0){
			data[i] = pImage->data[i];
			data[i+1] = pImage->data[i+1];
			data[i+2] = pImage->data[i+2];
			data[i+3] = pImage->data[i+3];
		}
		alpha = (float)pImage->data[i+3] / 255;
		if(alpha != 0 && alpha != 1){
			
			data[i] = alpha * data[i];
			data[i+1] = alpha * data[i+1];
			data[i+2] = alpha * data[i+2];
			data[i+3] = alpha * data[i+3];
		}
     /*   RGBA_To_RGB(data + i, rgb1);
			data[i] = rgb1[0];
			data[i+1] = rgb1[1];
			data[i+2] = rgb1[2];*/
	}
    return true;
}// Comp_In


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "out" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Out(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Out: Images not the same size\n";
        return false;
    }
    for(int i = 0;i < height * width * 4;i += 4){
		
		float alpha;
		if((float)pImage->data[i+3] / 255 == 1){
			data[i] = 0;
			data[i+1] = 0;
			data[i+2] = 0;
			data[i+3] = 0;
		}
		alpha = (float)pImage->data[i+3] / 255;
		if(alpha != 0 && alpha != 1){
			
			data[i] = (1-alpha) * data[i];
			data[i+1] = (1-alpha) * data[i+1];
			data[i+2] = (1-alpha) * data[i+2];
			data[i+3] = (1-alpha) * data[i+3];
		}
	}
    return true;
}// Comp_Out


///////////////////////////////////////////////////////////////////////////////
//
//      Composite current image "atop" given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Atop(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Atop: Images not the same size\n";
        return false;
    }
     for(int i = 0;i < height * width * 4;i += 4){
		
		float alpha_g = 0.0,alpha_f =0.0;
		alpha_g = (float)pImage->data[i+3] / 255;

		alpha_f = (float)data[i+3] / 255;
		if(alpha_g == 0){
			if(alpha_f != 1){

			//alpha_f = (float) data[i+3] / 255;
			  data[i] = (1 - alpha_f) * pImage->data[i];
			  data[i+1] = (1 - alpha_f) * pImage->data[i+1];
			  data[i+2] = (1 - alpha_f) * pImage->data[i+2];
			  data[i+3] = (1 - alpha_f) * pImage->data[i+3];
			}
			else{
			  data[i] = 0;
			  data[i+1] = 0;
			  data[i+2] = 0;
			  data[i+3] = 0;
			}
		}
		else{
			if(alpha_f == 1){
              data[i] = alpha_g * data[i];
			  data[i+1] = alpha_g * data[i+1];
			  data[i+2] = alpha_g * data[i+2];
			  data[i+3] = alpha_g * data[i+3];
			}
			else{
			  data[i] = alpha_g * data[i] + (1 - alpha_f) * pImage->data[i];
			  data[i+1] = alpha_g * data[i+1] + (1 - alpha_f) * pImage->data[i+1];
			  data[i+2] = alpha_g * data[i+2] + (1 - alpha_f) * pImage->data[i+2];
			  data[i+3] = alpha_g * data[i+3] + (1 - alpha_f) * pImage->data[i+3];
			}
		}
		
	}
   
    return true;
}// Comp_Atop


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image with given image using exclusive or (XOR).  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Xor(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Xor: Images not the same size\n";
        return false;
    }
     for(int i = 0;i < height * width * 4;i += 4){
		
		float alpha_g = 0.0,alpha_f =0.0;
		alpha_g = (float)pImage->data[i+3] / 255;

		alpha_f = (float)data[i+3] / 255;
		
		data[i] = (1 - alpha_g) * data[i] + (1 - alpha_f) * pImage->data[i];
	    data[i+1] = (1 - alpha_g) * data[i+1] + (1 - alpha_f) * pImage->data[i+1];
        data[i+2] = (1 - alpha_g) * data[i+2] + (1 - alpha_f) * pImage->data[i+2];
        data[i+3] = (1 - alpha_g) * data[i+3] + (1 - alpha_f) * pImage->data[i+3];
	 }
    return true;
}// Comp_Xor


///////////////////////////////////////////////////////////////////////////////
//
//      Calculate the difference bewteen this imag and the given one.  Image 
//  dimensions must be equal.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Difference(TargaImage* pImage)
{
    if (!pImage)
        return false;

    if (width != pImage->width || height != pImage->height)
    {
        cout << "Difference: Images not the same size\n";
        return false;
    }// if

    for (int i = 0 ; i < width * height * 4 ; i += 4)
    {
        unsigned char        rgb1[3];
        unsigned char        rgb2[3];

        RGBA_To_RGB(data + i, rgb1);
        RGBA_To_RGB(pImage->data + i, rgb2);

        data[i] = abs(rgb1[0] - rgb2[0]);
        data[i+1] = abs(rgb1[1] - rgb2[1]);
        data[i+2] = abs(rgb1[2] - rgb2[2]);
        data[i+3] = 255;
    }

    return true;
}// Difference


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 box filter on this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////

bool TargaImage::Filter_Box()
{   
	float *filter_box;
	unsigned char *R,*G,*B,*convolR,*convolG,*convolB;
	filter_box = new float[25];
    R = new unsigned char[width * height];
    G = new unsigned char[width * height];
	B = new unsigned char[width * height];
	convolR = new unsigned char[width * height];
	convolG = new unsigned char[width * height];
	convolB = new unsigned char[width * height];

	for(int i = 0;i < 25;i++)
		filter_box[i] = (float)1/25;
	/* store (r,g,b) to R,G,B */
	for(int i = 0;i < width * height * 4;i += 4){
		R[i/4] = data[i];
		G[i/4] = data[i+1];
		B[i/4] = data[i+2];
	}
    convolve(R,convolR,400,400,filter_box,5,5);
	convolve(G,convolG,400,400,filter_box,5,5);
	convolve(B,convolB,400,400,filter_box,5,5);
	for(int i = 0;i < width * height * 4;i += 4)
	{
		data[i] = convolR[i/4];
		data[i+1] = convolG[i/4];
		data[i+2] = convolB[i/4];
	}
    return true;
}// Filter_Box


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Bartlett filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Bartlett()
{
    float *filter_bart;
	float tmp[25] = {1,2,3,2,1,2,4,6,4,2,3,6,9,6,3,2,4,6,4,2,1,2,3,2,1};
	unsigned char *R,*G,*B,*convolR,*convolG,*convolB;
	filter_bart = new float[25];
    R = new unsigned char[width * height];
    G = new unsigned char[width * height];
	B = new unsigned char[width * height];
	convolR = new unsigned char[width * height];
	convolG = new unsigned char[width * height];
	convolB = new unsigned char[width * height];

	for(int i = 0;i < 25;i++)
		printf("%f\n",tmp[i]);
	memcpy(filter_bart,tmp,sizeof(tmp));
	for(int i = 0;i < 25;i++)
		printf("%f\n",filter_bart[i]);

	for(int i = 0;i < 25;i++)
		filter_bart[i] = (float)filter_bart[i]/81;
    for(int i = 0;i < 25;i++)
		printf("%f\n",filter_bart[i]);

	/* store (r,g,b) to R,G,B */
	for(int i = 0;i < width * height * 4;i += 4){
		R[i/4] = data[i];
		G[i/4] = data[i+1];
		B[i/4] = data[i+2];
	}
    convolve(R,convolR,400,400,filter_bart,5,5);
	convolve(G,convolG,400,400,filter_bart,5,5);
	convolve(B,convolB,400,400,filter_bart,5,5);
	
	for(int i = 0;i < width * height * 4;i += 4)
	{
		data[i] = convolR[i/4];
		data[i+1] = convolG[i/4];
		data[i+2] = convolB[i/4];
	}
    return true;
}// Filter_Bartlett


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Gaussian()
{
    ClearToBlack();
    return false;
}// Filter_Gaussian

///////////////////////////////////////////////////////////////////////////////
//
//      Perform NxN Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////

bool TargaImage::Filter_Gaussian_N( unsigned int N )
{
	
	unsigned char *R,*G,*B,*convolR,*convolG,*convolB;
    R = new unsigned char[width * height];
    G = new unsigned char[width * height];
	B = new unsigned char[width * height];
	convolR = new unsigned char[width * height];
	convolG = new unsigned char[width * height];
	convolB = new unsigned char[width * height];

	/* construct 1d filter */
	float *filter_1d = new float[N];
	float *filter_2d = new float[N*N];
	float sum = 0.0;
	for(int i = 0;i < N;i++)
	{ filter_1d[i] = Binomial(N - 1,i); sum += filter_1d[i]; }
  
	/* construct 2d filter */
	for(int i = 0;i < N;i++)
		for(int j = 0;j < N;j++)
		{
		  filter_2d[i * N + j] = filter_1d[i] * filter_1d[j] / pow(sum,2);
		}
	for(int i = 0;i < N;i++)
	{
		for(int j = 0;j < N;j++)
		{
			printf("%f  ",filter_2d[i * N + j]);
		}
		printf("\n");
	}
	/* store (r,g,b) to R,G,B */
	for(int i = 0;i < width * height * 4;i += 4){
		R[i/4] = data[i];
		G[i/4] = data[i+1];
		B[i/4] = data[i+2];
	}
    convolve(R,convolR,400,400,filter_2d,N,N);
	convolve(G,convolG,400,400,filter_2d,N,N);
	convolve(B,convolB,400,400,filter_2d,N,N);
	for(int i = 0;i < width * height * 4;i += 4)
	{
		data[i] = convolR[i/4];
		data[i+1] = convolG[i/4];
		data[i+2] = convolB[i/4];
	}
    return true;
	
    
   return true;
}// Filter_Gaussian_N


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 edge detect (high pass) filter on this image.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Edge()
{
    float *filter_bart;
	float *f = new float[25];
	float tmp[25] = {1,2,3,2,1,2,4,6,4,2,3,6,9,6,3,2,4,6,4,2,1,2,3,2,1};
	unsigned char *R,*G,*B,*convolR,*convolG,*convolB;
	filter_bart = new float[25];
    R = new unsigned char[width * height];
    G = new unsigned char[width * height];
	B = new unsigned char[width * height];
	convolR = new unsigned char[width * height];
	convolG = new unsigned char[width * height];
	convolB = new unsigned char[width * height];

	for(int i = 0;i < 25;i++){
		f[i] = 0; 
	}
	f[13] = 1;
	for(int i = 0;i < 25;i++)
		printf("%f\n",tmp[i]);
	memcpy(filter_bart,tmp,sizeof(tmp));
	for(int i = 0;i < 25;i++)
		printf("%f\n",filter_bart[i]);

	for(int i = 0;i < 25;i++)
		filter_bart[i] = (float)filter_bart[i]/81;
    for(int i = 0;i < 25;i++)
		printf("%f\n",filter_bart[i]);
	for(int i = 0;i < 25;i++){
		f[i] = f[i] - filter_bart[i];
	}
	/* store (r,g,b) to R,G,B */
	for(int i = 0;i < width * height * 4;i += 4){
		R[i/4] = data[i];
		G[i/4] = data[i+1];
		B[i/4] = data[i+2];
	}
    convolve(R,convolR,400,400,f,5,5);
	convolve(G,convolG,400,400,f,5,5);
	convolve(B,convolB,400,400,f,5,5);
	for(int i = 0;i < width * height * 4;i += 4)
	{
		data[i] = convolR[i/4];
		data[i+1] = convolG[i/4];
		data[i+2] = convolB[i/4];
	}
    return true;
}// Filter_Edge


///////////////////////////////////////////////////////////////////////////////
//
//      Perform a 5x5 enhancement filter to this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Enhance()
{
    ClearToBlack();
    return false;
}// Filter_Enhance


///////////////////////////////////////////////////////////////////////////////
//
//      Run simplified version of Hertzmann's painterly image filter.
//      You probably will want to use the Draw_Stroke funciton and the
//      Stroke class to help.
// Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
void imageDiff(unsigned char *canvas,unsigned char *refImage,unsigned char *difImage){

	double diff;
	for(int i= 0; i< 400 * 400 *4; i += 4)
	{
 	  diff = pow(((double)canvas[i]-(double)refImage[i]), 2)
 			 	+ pow(((double)canvas[i+1]-(double)refImage[i+1]), 2)
 			 	+ pow(((double)canvas[i+2]-(double)refImage[i+2]), 2);
 	  diff = sqrt(diff);
 	  diff = diff > 255 ? 255 : diff;
	  difImage[i] = diff;
	  difImage[i+1] = diff;
	  difImage[i+2] = diff;
	 // difImage[i] = refImage[i];
	 // difImage[i+1] = refImage[i+1];
	 // difImage[i+2] = refImage[i+2];
	}
}

int region(unsigned char *difImage, int& xPos, int& yPos, int radius)
{
	int sum = 0, idx, maxVal = 0, maxX = 0, maxY = 0;

	for(int i = -radius; i <= radius; i++)
	{
		for(int j = -radius; j <= radius; j++)
 		{
 		 	idx = (i+xPos)*3 + 400 * (400-yPos-1+j)*3;
 			if((idx >= 0)&&((i+xPos) >= 0)&&
				((i+xPos) < 400)&&((j+yPos) < 400))
 			{
 				sum += (int)difImage[idx];

 		 		// Test for the max value of the region 
 				if((int)difImage[idx] > maxVal)
				{
					maxVal = (int)difImage[idx];
					maxX = i+xPos;
					maxY = j+yPos;
				}
			}
		}
	}
	xPos = maxX; yPos = maxY;
	return sum;
}

void paintLayer(unsigned char *source,TargaImage *canvas,unsigned char *refImage,int radius){

	unsigned char *difImage = new unsigned char[400*400 * 4];
	imageDiff(canvas->data,refImage,difImage);

	int grid = radius; /* fg * radius, fg set to 1 */
	int error,rad,maxX,maxY,unitNum;

	vector<int> strokePointsX, strokePointsY;
	strokePointsX.begin();
	strokePointsY.begin();

	Stroke stroke;

	for(int x = 0; x < 400;x += grid)
		for(int y = 0;y < 400;y += grid)
		{
			// sum the error
			rad = (grid/2 < 1) ? 1 : grid/2;
 			maxX = x+rad; maxY = y+rad;
 			error = region(difImage, maxX, maxY, rad)/(grid * grid);
			if(error > 25)  // threshold set to 25
			{
			  strokePointsX.push_back(maxX);
			  strokePointsY.push_back(maxY);
			}
		}
	// Randomly pull a point from the container and render it
	//srand(time(NULL));
	while(!strokePointsX.empty() && !strokePointsY.empty())
	{
		unitNum = rand() % strokePointsX.size();
		stroke.radius = radius;
		stroke.x = strokePointsX[unitNum];
		stroke.y = strokePointsY[unitNum];
		stroke.r = refImage[stroke.y * 400 + stroke.x];
		stroke.g = refImage[stroke.y * 400 + stroke.x + 1];
		stroke.b = refImage[stroke.y * 400 + stroke.x + 2];
		stroke.a = refImage[stroke.y * 400 + stroke.x + 3];
		canvas->Paint_Stroke(stroke);
		strokePointsX.erase(strokePointsX.begin()+unitNum);
		strokePointsY.erase(strokePointsY.begin()+unitNum);
	}
	/* copy canvas data to current image */
	for(int i = 0;i < 400 * 400 * 4;i += 4){
		source[i] = canvas->data[i];
		source[i+1] = canvas->data[i+1];
		source[i+2] = canvas->data[i+2];
		source[i+3] = canvas->data[i+3];
	}

	delete[] difImage;
}

void paint(TargaImage *source,int strokeSizes[],int num){

	TargaImage *canvas = new TargaImage(400,400);
	unsigned char *refImage = new unsigned char[400 * 400 * 4];
    /* Set initial canvas to black */
	for(int i = 0;i < 400 * 400 * 4;i += 4){
		canvas->data[i] = 0;
		canvas->data[i+1] = 0;
        canvas->data[i+2] = 0;
		//canvas->data[i+3] = 0;
	}

	for(int i = 0;i < num;i++){
       bool blur = source->Filter_Gaussian_N(2 * strokeSizes[i] + 1);
		/* Gaussian blurred refImage */
	   if(blur == true){
		for(int i = 0;i < 400 * 400 * 4;i += 4){
			refImage[i] = (float)source->data[i];
			refImage[i+1] = (float)source->data[i+1];
			refImage[i+2] = (float)source->data[i+2];
		}
	   }
		paintLayer(source->data,canvas,refImage,strokeSizes[i]);
	}
    //Redraw();


}
bool TargaImage::NPR_Paint()
{   
	/* Not Working !!!!! 
	 The basic idea: I use Gaussian filter N to create a reference image of current image, and a canvas image
	                 to paint, each time stroke color is initialized by the color in the corresponding point 
					 of reference image. I think it is wrong in error computation of paintLayer. I just did not
					 figure out how to do it, and the paper omits some details in this part.*/
	TargaImage *source = new TargaImage(400,400);
	memcpy(source->data,data,sizeof(data));
	int strokeSizes[] = {7,3,1};
	paint(source,strokeSizes,3);
	for(int i = 0;i < width * height * 4;i += 4){
		data[i] = source->data[i];
		data[i+1] = source->data[i+1];
		data[i+2] = source->data[i+2];
		data[i+3] = source->data[i+3];
	}
    return true;
}



///////////////////////////////////////////////////////////////////////////////
//
//      Halve the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Half_Size()
{
    ClearToBlack();
    return false;
}// Half_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Double the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Double_Size()
{
    ClearToBlack();
    return false;
}// Double_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Scale the image dimensions by the given factor.  The given factor is 
//  assumed to be greater than one.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Resize(float scale)
{
    ClearToBlack();
    return false;
}// Resize


//////////////////////////////////////////////////////////////////////////////
//
//      Rotate the image clockwise by the given angle.  Do not resize the 
//  image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Rotate(float angleDegrees)
{
    ClearToBlack();
    return false;
}// Rotate


//////////////////////////////////////////////////////////////////////////////
//
//      Given a single RGBA pixel return, via the second argument, the RGB
//      equivalent composited with a black background.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::RGBA_To_RGB(unsigned char *rgba, unsigned char *rgb)
{
    const unsigned char	BACKGROUND[3] = { 0, 0, 0 };

    unsigned char  alpha = rgba[3];

    if (alpha == 0)
    {
        rgb[0] = BACKGROUND[0];
        rgb[1] = BACKGROUND[1];
        rgb[2] = BACKGROUND[2];
    }
    else
    {
	    float	alpha_scale = (float)255 / (float)alpha;
	    int	val;
	    int	i;

	    for (i = 0 ; i < 3 ; i++)
	    {
	        val = (int)floor(rgba[i] * alpha_scale);
	        if (val < 0)
		    rgb[i] = 0;
	        else if (val > 255)
		    rgb[i] = 255;
	        else
		    rgb[i] = val;
	    }
    }
}// RGA_To_RGB


///////////////////////////////////////////////////////////////////////////////
//
//      Copy this into a new image, reversing the rows as it goes. A pointer
//  to the new image is returned.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Reverse_Rows(void)
{
    unsigned char   *dest = new unsigned char[width * height * 4];
    TargaImage	    *result;
    int 	        i, j;

    if (! data)
    	return NULL;

    for (i = 0 ; i < height ; i++)
    {
	    int in_offset = (height - i - 1) * width * 4;
	    int out_offset = i * width * 4;

	    for (j = 0 ; j < width ; j++)
        {
	        dest[out_offset + j * 4] = data[in_offset + j * 4];
	        dest[out_offset + j * 4 + 1] = data[in_offset + j * 4 + 1];
	        dest[out_offset + j * 4 + 2] = data[in_offset + j * 4 + 2];
	        dest[out_offset + j * 4 + 3] = data[in_offset + j * 4 + 3];
        }
    }

    result = new TargaImage(width, height, dest);
    delete[] dest;
    return result;
}// Reverse_Rows


///////////////////////////////////////////////////////////////////////////////
//
//      Clear the image to all black.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::ClearToBlack()
{
    memset(data, 0, width * height * 4);
}// ClearToBlack


///////////////////////////////////////////////////////////////////////////////
//
//      Helper function for the painterly filter; paint a stroke at
// the given location
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::Paint_Stroke(const Stroke& s) {
   int radius_squared = (int)s.radius * (int)s.radius;
   for (int x_off = -((int)s.radius); x_off <= (int)s.radius; x_off++) {
      for (int y_off = -((int)s.radius); y_off <= (int)s.radius; y_off++) {
         int x_loc = (int)s.x + x_off;
         int y_loc = (int)s.y + y_off;
         // are we inside the circle, and inside the image?
         if ((x_loc >= 0 && x_loc < width && y_loc >= 0 && y_loc < height)) {
            int dist_squared = x_off * x_off + y_off * y_off;
            if (dist_squared <= radius_squared) {
               data[(y_loc * width + x_loc) * 4 + 0] = s.r;
               data[(y_loc * width + x_loc) * 4 + 1] = s.g;
               data[(y_loc * width + x_loc) * 4 + 2] = s.b;
               data[(y_loc * width + x_loc) * 4 + 3] = s.a;
            } else if (dist_squared == radius_squared + 1) {
               data[(y_loc * width + x_loc) * 4 + 0] = 
                  (data[(y_loc * width + x_loc) * 4 + 0] + s.r) / 2;
               data[(y_loc * width + x_loc) * 4 + 1] = 
                  (data[(y_loc * width + x_loc) * 4 + 1] + s.g) / 2;
               data[(y_loc * width + x_loc) * 4 + 2] = 
                  (data[(y_loc * width + x_loc) * 4 + 2] + s.b) / 2;
               data[(y_loc * width + x_loc) * 4 + 3] = 
                  (data[(y_loc * width + x_loc) * 4 + 3] + s.a) / 2;
            }
         }
      }
   }
}


///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke() {}

///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke(unsigned int iradius, unsigned int ix, unsigned int iy,
               unsigned char ir, unsigned char ig, unsigned char ib, unsigned char ia) :
   radius(iradius),x(ix),y(iy),r(ir),g(ig),b(ib),a(ia)
{
}
