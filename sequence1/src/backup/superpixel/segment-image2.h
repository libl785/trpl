
/*
  Modified by Haifen Gong 2010.
*/



/*
  Copyright (C) 2006 Pedro Felzenszwalb

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#ifndef SEGMENT_IMAGE
#define SEGMENT_IMAGE

#include <cstdlib>
#include "image.h"
#include "misc.h"
#include "filter.h"
#include "segment-graph.h"

#include <boost/numeric/ublas/matrix.hpp>
#include "cvpr_array_traits.hpp"

// random color
rgb random_rgb(){ 
    rgb c;
    double r;
  
    c.r = (uchar)rand();
    c.g = (uchar)rand();
    c.b = (uchar)rand();

    return c;
}

// dissimilarity measure between pixels
static inline float diff(image<float> *r, image<float> *g, image<float> *b,
			 int x1, int y1, int x2, int y2) {
    return sqrt(square(imRef(r, x1, y1)-imRef(r, x2, y2)) +
		square(imRef(g, x1, y1)-imRef(g, x2, y2)) +
		square(imRef(b, x1, y1)-imRef(b, x2, y2)));
}

/*
 * Segment an image
 *
 * Returns a color image representing the segmentation.
 *
 * im: image to segment.
 * sigma: to smooth the image.
 * c: constant for treshold function.
 * min_size: minimum component size (enforced by post-processing stage).
 * num_ccs: number of connected components in the segmentation.
 */
template <class Mat1, class Mat2>
int segment_image(Mat1 const &im, float sigma, float c, int min_size,
		  boost::numeric::ublas::matrix<int>& seg,
		  Mat2& vis ) 
{
    //int width = im->width();
    //int height = im->height();
    typedef cvpr::array3d_traits<Mat1> tr1;
    typedef cvpr::array3d_traits<Mat2> tr2;

    //assert tr1::size3(im)==3 
    int width = tr1::size3(im);
    int height = tr1::size2(im);

    image<float> *r = new image<float>(width, height);
    image<float> *g = new image<float>(width, height);
    image<float> *b = new image<float>(width, height);

    // smooth each color channel  
    for (int y = 0; y < height; y++) {
	for (int x = 0; x < width; x++) {
	    //imRef(r, x, y) = imRef(im, x, y).r;
	    //imRef(g, x, y) = imRef(im, x, y).g;
	    //imRef(b, x, y) = imRef(im, x, y).b;

	    //imRef(r, x, y) = im(y, x*3+0);
	    //imRef(g, x, y) = im(y, x*3+1);
	    //imRef(b, x, y) = im(y, x*3+2);

	    imRef(r, x, y) = tr1::ref(im, 0, y, x);
	    imRef(g, x, y) = tr1::ref(im, 1, y, x);
	    imRef(b, x, y) = tr1::ref(im, 2, y, x);
	}
    }
    image<float> *smooth_r = smooth(r, sigma);
    image<float> *smooth_g = smooth(g, sigma);
    image<float> *smooth_b = smooth(b, sigma);
    delete r;
    delete g;
    delete b;
 
    // build graph
    edge *edges = new edge[width*height*4];
    int num = 0;
    for (int y = 0; y < height; y++) {
	for (int x = 0; x < width; x++) {
	    if (x < width-1) {
		edges[num].a = y * width + x;
		edges[num].b = y * width + (x+1);
		edges[num].w = diff(smooth_r, smooth_g, smooth_b, x, y, x+1, y);
		num++;
	    }

	    if (y < height-1) {
		edges[num].a = y * width + x;
		edges[num].b = (y+1) * width + x;
		edges[num].w = diff(smooth_r, smooth_g, smooth_b, x, y, x, y+1);
		num++;
	    }

	    if ((x < width-1) && (y < height-1)) {
		edges[num].a = y * width + x;
		edges[num].b = (y+1) * width + (x+1);
		edges[num].w = diff(smooth_r, smooth_g, smooth_b, x, y, x+1, y+1);
		num++;
	    }

	    if ((x < width-1) && (y > 0)) {
		edges[num].a = y * width + x;
		edges[num].b = (y-1) * width + (x+1);
		edges[num].w = diff(smooth_r, smooth_g, smooth_b, x, y, x+1, y-1);
		num++;
	    }
	}
    }
    delete smooth_r;
    delete smooth_g;
    delete smooth_b;

    // segment
    universe *u = segment_graph(width*height, num, edges, c);
  
    // post process small components
    for (int i = 0; i < num; i++) {
	int a = u->find(edges[i].a);
	int b = u->find(edges[i].b);
	if ((a != b) && ((u->size(a) < min_size) || (u->size(b) < min_size)))
	    u->join(a, b);
    }
    delete [] edges;
    int num_ccs = u->num_sets();


    tr2::change_size(vis, 3, height, width);

    // pick random colors for each component
    rgb *colors = new rgb[width*height];
    for (int i = 0; i < width*height; i++)
	colors[i] = random_rgb();
  
    for (int y = 0; y < height; y++) {
	for (int x = 0; x < width; x++) {
	    int comp = u->find(y * width + x);
	    //imRef(output, x, y) = colors[comp];
	    //vis(y, x*3+0) = colors[comp].r;
	    //vis(y, x*3+1) = colors[comp].g;
	    //vis(y, x*3+2) = colors[comp].b;
	    tr2::ref(vis, 0, y, x) = colors[comp].r;
	    tr2::ref(vis, 1, y, x) = colors[comp].g;
	    tr2::ref(vis, 2, y, x) = colors[comp].b;
	}
    }  

    delete [] colors;  


    seg = boost::numeric::ublas::matrix<int>(height, width);
    for (int y = 0; y < height; y++) {
	for (int x = 0; x < width; x++) {
	    int comp = u->find(y * width + x);
	    seg(y, x) = comp;
	}
    }  


    delete u;

    //return output;
    return num_ccs;
}

#endif