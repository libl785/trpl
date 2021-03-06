#ifndef __TRACKING__DATA__PACKAGE__HPP__INCLUDED__
#define __TRACKING__DATA__PACKAGE__HPP__INCLUDED__

#include <boost/filesystem.hpp>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/assignment.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/lu.hpp>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/utility.hpp>
#include <boost/iterator/counting_iterator.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include <iostream>
#include <fstream>

#include <CImg.h>

#include "real_timer.hpp"
#include "text_file.hpp"
#include "ublas_cimg.hpp"
#include "ublas_random.hpp"

//#include "pretr.hpp"
#include "labelmap.hpp"
#include "cvpr_array_traits.hpp"

#include "mask_utils.hpp"
#include "geometry.hpp"



using namespace cimg_library;

namespace fs = boost::filesystem;
using namespace cvpr;

using namespace boost::numeric::ublas;
using namespace boost;

#include "camera.hpp"

struct parameter_t
{
    float mot_param_sig1; // small
    float mot_param_sig2;  //smooth

    float occl_thr1;
    float occl_thr2;


    int xrange; //% search range x
    int yrange; //% search range y

    int xstep ;
    int ystep ;

    vector<float> scales; //% search range, scale
    vector<float> pvp; // part visbility probability

    //% learning rate
    float fglr;
    float bglr;
    float fglr_sift;
    float bglr_sift;

    //% score threshold

    float thr;
    float thr3;
    float head_wid_ratio;
    float head_hi_ratio;
    float torso_hi_ratio;

    vector<float> sift_scale;

    parameter_t()	{
	using namespace boost::lambda;

	mot_param_sig1 = 100; // small
	mot_param_sig2 = 25;  //smooth

	occl_thr1 = 6;
	occl_thr2 = 4;

	xrange = 50; //% search range x
	yrange = 36; //% search range y

	//xstep = 3;
	//ystep = 2;
	xstep = 4;
	ystep = 3;

	//scales = list_of(1.05)(1)(0.96); //% search range, scale
	scales = vector<float>(3);
	scales <<= 1.05, 1, 0.96;

	pvp = vector<float>(3);
	pvp <<= 0.9, 0.8, 0.7; // head, torso, legs

	//% learning rate
	fglr = 0.04;
	bglr = 0.06;
	fglr_sift = 0.04;
	bglr_sift = 0.06;

	//% score threshold

	thr = 6;
	thr3 = -5;
	//thr = 8;
	//thr3 = -3;
	head_wid_ratio = 0.5;
	head_hi_ratio = 0.2;
	torso_hi_ratio = 0.6;
	sift_scale = vector<float> (5);
	sift_scale(0) = 1;
	for(int ii=1; ii<sift_scale.size(); ++ii)
	{
	    sift_scale(ii) = 1.33*sift_scale(ii-1);
	}
	std::for_each(sift_scale.begin(), sift_scale.end(), _1*=7/7);

	logp1 = vector<float> (pvp.size());
	logp2 = vector<float> (pvp.size());

	float (*flog)(float) = std::log;
	float (*fexp)(float) = std::exp;
	std::transform(pvp.begin(), pvp.end(), logp1.begin(), flog );
	std::transform(pvp.begin(), pvp.end(), logp2.begin(), bind(flog, (1.0f-_1)) );

    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)	{
	ar & mot_param_sig1;
	ar & mot_param_sig2;

	ar & occl_thr1;
	ar & occl_thr2;

	ar & xrange;
	ar & yrange;

	ar & xstep ;
	ar & ystep ;

	ar & scales;
	ar & pvp;
	ar & logp1;
	ar & logp2;

	ar & fglr;
	ar & bglr;
	ar & fglr_sift;
	ar & bglr_sift;

	ar & thr;
	ar & thr3;
	ar & head_wid_ratio;
	ar & head_hi_ratio;
	ar & torso_hi_ratio;

	ar & sift_scale;

    }
//private:
    vector<float> logp1; //log pvp
    vector<float> logp2; //log (1-pvp)

};

void read_sequence_list(const std::string& prefix, vector<std::vector<std::string> >& seq)
{
    seq = vector<std::vector<std::string> >(2);
    std::vector<std::string> seql;
    std::vector<std::string> seqr;

    read_string_list(prefix+"image_list_l.txt", seql);
    read_string_list(prefix+"image_list_r.txt", seqr);

    for(int jj=0; jj<seql.size(); ++jj)
    {
	seql[jj] = prefix+"left_rect/"+seql[jj];
    }

    for(int jj=0; jj<seqr.size(); ++jj)
    {
	seqr[jj] = prefix+"right_rect/"+seqr[jj];
    }

    seq[0] = seql;
    seq[1] = seqr;

}



template <class Float>
void load_part_model(vector<array<Float, 4> >& model,
		     Float head_wid_ratio,
		     Float head_hi_ratio,
		     Float torso_hi_ratio )
{
#if 0
    float part_model_data[3][4]= {
	    {0.25, 0, 0.75, 0.2},
	    {0, 0.2, 1, 0.6},
	    {0, 0.6, 1, 1}
	};
#endif

    float data[3][4]= {
	{0.5-head_wid_ratio/2, 0, 0.5+head_wid_ratio/2, head_hi_ratio},
	{0, head_hi_ratio, 1, torso_hi_ratio},
	{0, torso_hi_ratio, 1, 1}
    };


    model = vector<boost::array<Float, 4> >(3);
    for(int ii=0; ii<3; ++ii)
    {
	for(int jj=0; jj<4; ++jj)
	{
	    model(ii)[jj] = data[ii][jj];
	}
    }

}



struct object_trj_t
{
    int startt;
    int endt;
    int state;

    object_trj_t():startt(-1), endt(-1){};
    /* index order (cam, T, 4) */
    typedef vector<matrix<float> > trj_t;

    /* index order (T, 2)*/
    typedef matrix<float> trj_3d_t;

    trj_t trj;
    trj_3d_t trj_3d;

    /*index order: (cam, T)*/
    matrix<float> scores;

    
    /* index order (cam, part*2, T) */
    vector<matrix<float> > fscores;

    /* index order (cam, part, xxx) */
    /* part: 0 --- body; 1 --- head; 2 --- torso; 3 --- legs*/
    vector<matrix<float> > hist_p;
    vector<matrix<float> > hist_q;
    matrix<matrix<float> > sift_d;
    matrix<matrix<float> > sift_v;
    matrix<matrix<float> > sift_vq;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)	{
	ar & BOOST_SERIALIZATION_NVP(startt);
	ar & BOOST_SERIALIZATION_NVP(endt);
	ar & BOOST_SERIALIZATION_NVP(state);
	ar & BOOST_SERIALIZATION_NVP(trj);
	ar & BOOST_SERIALIZATION_NVP(trj_3d);
	ar & BOOST_SERIALIZATION_NVP(scores);
	ar & BOOST_SERIALIZATION_NVP(fscores);
	ar & BOOST_SERIALIZATION_NVP(hist_p);
	ar & BOOST_SERIALIZATION_NVP(hist_q);
	ar & BOOST_SERIALIZATION_NVP(sift_d);
	ar & BOOST_SERIALIZATION_NVP(sift_v);
	ar & BOOST_SERIALIZATION_NVP(sift_vq);
    }

//save in plain text
#if 0
    void save(std::string const & filename) {
	std::ofstream ofs(filename.c_str());
	boost::archive::text_oarchive oa(ofs);
        oa << *this;
    }
    void load(std::string const & filename) {
	std::ifstream ifs(filename.c_str());
	boost::archive::text_iarchive ia(ifs);
        ia >> *this;
    }
#endif

//save in xml format
    void save(std::string const & filename) {
	std::ofstream ofs(filename.c_str());
	boost::archive::xml_oarchive oa(ofs);
        oa << boost::serialization::make_nvp("tracklet", *this);

    }
    void load(std::string const & filename) {
	std::ifstream ifs(filename.c_str());
	boost::archive::xml_iarchive ia(ifs);
        ia >> boost::serialization::make_nvp("tracklet", *this);
    }

};

struct pmodel_t
{
    float bw, bh;
    float hpre;
};

struct smodel_t
{

};


struct directory_structure_t
{
    directory_structure_t() {
	prefix = "../images/";
	output = "../output2/";
	workspace = "../workspace/";
	figures = "../figures/";
    }
    void make_dir(){
	if ( !fs::exists( output ) )
	{
	    fs::create_directory( output );
	}
    
	if ( !fs::exists( figures ) )
	{
	    fs::create_directory( figures );
	}
    }


    std::string prefix;
    std::string output;
    std::string workspace;
    std::string figures;
};



struct geometric_info_t
{
    matrix<matrix<double> > img2grd;
    matrix<matrix<double> > grd2img;

    matrix<matrix<double> > goals_im;
    matrix<matrix<double> > polys_im;

    matrix<double> goal_ground;
    matrix<double> poly_ground;

    ground_lim_t ground_lim;
    camera_param_t<double> cam_param;

    float horiz_mean;
    float horiz_sig;

    matrix<matrix<unsigned char> > poly_mask;

    void load(directory_structure_t const& ds,
	array<std::size_t, 2> const& img_size ) {
	std::map<std::string, std::string> horiz_map;
	read_keyvalue_pairs(ds.workspace+"init_input/horiz.txt", horiz_map);
	horiz_mean = lexical_cast<float>(horiz_map["horiz_mean"]);
	horiz_sig = lexical_cast<float>(horiz_map["horiz_sig"]);
	std::cout<< horiz_mean <<","<<horiz_sig<<std::endl;

	load_camera_param(ds.workspace+"init_input/", cam_param);
	cam_param.print();

	matrix<double> goal2d, poly2d;
	read_text_array2d(ds.workspace+"init_input/goal2d.txt", goal2d);
	read_text_array2d(ds.workspace+"init_input/poly2d.txt", poly2d);
	//std::cout<<goal2d<<std::endl;
	//std::cout<<poly2d<<std::endl;

	{
	    std::string name = ds.workspace+"init_input/img2grd.txt";
	    std::ifstream fin(name.c_str());
	    fin>>img2grd;
	    fin.close();
	}

	{
	    std::string name = ds.workspace+"init_input/grd2img.txt";
	    std::ifstream fin(name.c_str());
	    fin>>grd2img;
	    fin.close();
	}

	compute_binocular_transform(cam_param, goal2d, poly2d, img_size, poly_mask,
				    goal_ground, poly_ground,
				    goals_im, polys_im, img2grd, grd2img, ground_lim);

	//std::cout<<"img2grd="<<img2grd<<std::endl;

    }

};



struct object_info_t
{

    object_info_t(int Ncam, int nobj, parameter_t const& P) {
	curr_num_obj = 0;
	trlet_list = vector<object_trj_t>(nobj);
	
	load_part_model(model, P.head_wid_ratio, P.head_hi_ratio, P.torso_hi_ratio);

	pmodel_list = matrix<pmodel_t>(Ncam, nobj);
	smodel_list = matrix<smodel_t>(Ncam, nobj);
    }

    int curr_num_obj;
    vector<object_trj_t> trlet_list;
    vector<array<float, 4> > model;
    matrix<pmodel_t> pmodel_list;
    matrix<smodel_t> smodel_list;

};

namespace boost {
    namespace serialization {

	template<class Archive, class T>
	void serialize(Archive & ar, CImg<unsigned char> & im, const unsigned int version)
	{
	    //ar & ;
	    //ar & ;
	    //ar & ;
	}

    } // namespace serialization
} // namespace boost


#endif
