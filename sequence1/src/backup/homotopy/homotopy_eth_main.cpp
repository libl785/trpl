#include <iostream>
#include <algorithm>
#include <vector>
#include <numeric>
#include <limits>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/casts.hpp>
#include <boost/lambda/bind.hpp>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/assignment.hpp>
#include <boost/numeric/ublas/lu.hpp>

#include <boost/numeric/bindings/traits/ublas_vector.hpp>
#include <boost/numeric/bindings/traits/ublas_sparse.hpp>
#include <boost/numeric/bindings/umfpack/umfpack.hpp>

#include <boost/numeric/ublas/io.hpp>

#include <boost/iterator/counting_iterator.hpp>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include <CImg.h>

#include "statistics.hpp"
#include "cvpr_array_traits.hpp"
#include "real_timer.hpp"
#include "text_file.hpp"
#include "misc_utils.hpp"

namespace umf=boost::numeric::bindings::umfpack;
using namespace boost::numeric::ublas;
using namespace boost;
using namespace cvpr;
using namespace cimg_library;

namespace fs = boost::filesystem;


typedef cvpr::sparse_matrix_t<double>::type umf_sparse_matrix;

struct directory_structure_t
{
    directory_structure_t() {
	prefix = "../training_data/seq_eth/";
	output = "../test/output2/";
	workspace = "../test/workspace/";
	figures = "../test/figures/";
    }
    void make_dir(){
	if ( !fs::exists( prefix ) )
	{
	    fs::create_directory( prefix );
	}
	if ( !fs::exists( workspace ) )
	{
	    fs::create_directory( workspace );
	}
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

#include "homotopy.hpp"

void read_jpeg_list(std::string const& prefix,
		    std::vector<std::string>& jpg_names)
{
    using namespace boost::numeric::ublas;
    using namespace boost::lambda;
    using namespace boost;

    fs::path full_path(prefix);
    std::vector<fs::path> files;
    typedef fs::directory_iterator diter;
    bool (*is_file)(fs::path const&) = &fs::is_regular_file;
    std::copy_if(diter(full_path), diter(), std::back_inserter(files), is_file);
    vector<std::string> file_names(files.size());
    std::transform(files.begin(), files.end(), file_names.begin(),
		   bind(&fs::path::string, _1));

    bool (*check_end)(std::string const&, std::string const&) = &boost::ends_with;
    bool (*check_start)(std::string const&, std::string const&) =&boost::starts_with;

    function<bool (std::string const&)> jpg_filt = bind(check_end, _1, ".jpeg");
    
    std::copy_if(file_names.begin(), file_names.end(), std::back_inserter(jpg_names),
		 jpg_filt);
    std::for_each(jpg_names.begin(), jpg_names.end(), _1=bind(&fs::path::filename, _1));
    //std::for_each(jpg_names.begin(), jpg_names.end(), std::cout<<_1<<'\n');
}


struct ground_lim_t
{
    double xmin, xmax, ymin, ymax;
};



template <class Float>
void draw_positions(CImg<unsigned char>& image,
		    matrix<Float> const& pos,
		    vector<int> const& obj,
		    int tt,
		    vector<matrix<double> > const& hist_pos,
		    vector<vector<int> >& hist_time)
{
    using namespace boost::lambda;
    unsigned char pcol[3]={255, 0, 0};
    unsigned char lcol[3]={0, 255, 0};
    //image.draw_line(0, 0, 100, 100, pcol);

    std::string num = lexical_cast<std::string>(pos.size1());

    unsigned char tcol[3]={255, 255, 255};
    //image.draw_text(200, 10, num.c_str(), tcol);
    for(int pp=0; pp<pos.size1(); ++pp)
    {
	image.draw_circle(static_cast<int>(pos(pp, 0)+0.5l),
			  static_cast<int>(pos(pp, 1)+0.5l),
			  5, pcol, 1);

	std::string num = lexical_cast<std::string>(obj(pp));
	image.draw_text(static_cast<int>(pos(pp, 0)+0.5l),
			static_cast<int>(pos(pp, 1)+0.5l),
			num.c_str(), tcol);

	for(int ss=0; ss+1<hist_pos(obj(pp)).size1(); ++ss)
	{
	    image.draw_line(static_cast<int>(hist_pos(obj(pp))(ss, 0)+0.5l),
			    static_cast<int>(hist_pos(obj(pp))(ss, 1)+0.5l),
			    static_cast<int>(hist_pos(obj(pp))(ss+1, 0)+0.5l),
			    static_cast<int>(hist_pos(obj(pp))(ss+1, 1)+0.5l),
			    lcol);
	    if(hist_time(obj(pp))(ss+1)>=tt) break;
	}

    }

}

void draw_ground_positions(CImg<float>& gmap,
			   vector<matrix<double> > const& pos,
			   ground_lim_t const& glim,
			   double grid)
{
    using namespace boost::lambda;
    unsigned char pcol[3]={255, 0, 0};
    unsigned char lcol[3]={0, 255, 0};

    std::string num = lexical_cast<std::string>(pos.size());

    unsigned char tcol[3]={255, 255, 255};

    for(int pp=0; pp<pos.size(); ++pp)
    {
	int st = pos(pp).size1();
	if(st == 0) continue;
	double g1 = (pos(pp)(st-1, 0)-glim.xmin)/grid+0.5l;
	double g2 = (pos(pp)(st-1, 1)-glim.ymin)/grid+0.5l;
	gmap.draw_circle(static_cast<int>(g1),
			 static_cast<int>(g2),
			  5, pcol, 1);

	std::string num = lexical_cast<std::string>(pp);
	gmap.draw_text(static_cast<int>(g1),
		       static_cast<int>(g2),
		       num.c_str(), tcol);

	for(int ss=0; ss+1<st; ++ss)
	{
	    double g1 = (pos(pp)(ss, 0)-glim.xmin)/grid+0.5l;
	    double g2 = (pos(pp)(ss, 1)-glim.ymin)/grid+0.5l;
	    double h1 = (pos(pp)(ss+1, 0)-glim.xmin)/grid+0.5l;
	    double h2 = (pos(pp)(ss+1, 1)-glim.ymin)/grid+0.5l;
	    gmap.draw_line(static_cast<int>(g1),
			    static_cast<int>(g2),
			    static_cast<int>(h1),
			    static_cast<int>(h2),
			    lcol);

	}

    }

}


//[frame_number pedestrian_ID pos_x pos_z pos_y v_x v_z v_y ]
//however pos_z and v_z (direction perpendicular to the ground) are not used. 
void extract_frame_pos(matrix<double> const& obsmat, int T,
		       vector<matrix<double> >& frame_pos,
		       vector<vector<int> >& frame_obj)
{
    using namespace boost::lambda;
    matrix_column<matrix<double>const > col(obsmat, 0);
    vector<int> frame_num(col.size());
    std::transform(col.begin(), col.end(), frame_num.begin(),
		   ll_static_cast<int>(_1+0.5l) );    
    int num_rec = frame_num.size();

    frame_pos = vector<matrix<double> >(T);
    frame_obj = vector<vector<int> >(T);
    vector<std::vector<int> > rec_idx(T);

    for(int ii=0; ii<num_rec; ++ii)
    {
	int tt = frame_num(ii)-1;
	if(tt<T) rec_idx(tt).push_back(ii);
    }

    for(int tt=0; tt<T; ++tt)
    {
	frame_pos(tt) = scalar_matrix<double>(rec_idx(tt).size(), 2, 0.0l);
	frame_obj(tt) = scalar_vector<int>(rec_idx(tt).size(), 0);
	for(int kk=0; kk<rec_idx(tt).size(); ++kk)
	{
	    frame_pos(tt)(kk, 0) = obsmat(rec_idx(tt)[kk], 2);
	    frame_pos(tt)(kk, 1) = obsmat(rec_idx(tt)[kk], 4);
	    frame_obj(tt)(kk) = static_cast<int>(obsmat(rec_idx(tt)[kk], 1)+0.5l)-1;
	}
    }

}

void extract_object_pos(matrix<double> const& obsmat,
			vector<matrix<double> >& object_pos,
			vector<vector<int> >& object_time)
{
    using namespace boost::lambda;
    matrix_column<matrix<double>const > col(obsmat, 1);
    vector<int> object_id(col.size());
    std::transform(col.begin(), col.end(), object_id.begin(),
		   ll_static_cast<int>(_1+0.5l) );    

    int num_rec = object_id.size();
    int num_obj = *std::max_element(object_id.begin(), object_id.end());
    object_pos = vector<matrix<double> >(num_obj);
    object_time = vector<vector<int> >(num_obj);
    vector<std::vector<int> > rec_idx(num_obj);

    for(int ii=0; ii<num_rec; ++ii)
    {
	int oo = object_id(ii)-1;
	rec_idx(oo).push_back(ii);
    }

    for(int oo=0; oo<num_obj; ++oo)
    {
	object_pos(oo) = scalar_matrix<double>(rec_idx(oo).size(), 2, 0.0l);
	object_time(oo) = scalar_vector<int>(rec_idx(oo).size(), 0);
	for(int kk=0; kk<rec_idx(oo).size(); ++kk)
	{
	    object_pos(oo)(kk, 0) = obsmat(rec_idx(oo)[kk], 2);
	    object_pos(oo)(kk, 1) = obsmat(rec_idx(oo)[kk], 4);
	    object_time(oo)(kk) = static_cast<int>(obsmat(rec_idx(oo)[kk], 0)+0.5l)-1;
	}
    }

}



void homopgray_apply(matrix<double> const& H,
		     matrix<double> const& prev,
		     matrix<double>& after)
{
    after = matrix<double>(prev.size1(), 2);
    for(int ii=0; ii<prev.size1(); ++ii)
    {
	vector<double> v(3);
	v <<= prev(ii, 0), prev(ii, 1), 1.0l;
	vector<double> vr = prod(H, v);
	vr /= vr(2);
	after(ii, 0) = vr(0);
	after(ii, 1) = vr(1);
    }
}

void interpolate_object_pos(vector<matrix<double> > const& object_pos,
			    vector<vector<int> > const& object_time,
			    vector<matrix<double> > & interp_pos,
			    vector<vector<int> >& interp_time)
{
    int N = object_pos.size();

    interp_pos = vector<matrix<double> >(N);
    interp_time = vector<vector<int> >(N);
    for(int nn=0; nn<N; ++nn)
    {
	int st = object_time(nn).size();
	if(st==0) continue;
	if(st==1)
	{
	    interp_pos(nn) = object_pos(nn);
	    interp_time(nn) = object_time(nn);

	    continue;
	}
	int dt = object_time(nn)(st-1) - object_time(nn)(0) +1;
	interp_pos(nn) = matrix<double>(dt, 2);
	interp_time(nn) = vector<int>(dt);
	int t0 = object_time(nn)(0);
	for(int ss=0; ss+1<st; ++ss)
	{
	    int t1 = object_time(nn)(ss);
	    int t2 = object_time(nn)(ss+1);
	    for(int tt=t1; tt<=t2; ++tt)
	    {
		interp_pos(nn)(tt-t0, 0) = object_pos(nn)(ss, 0)*(t2-tt)/(t2-t1)
		    +object_pos(nn)(ss+1, 0)*(tt-t1)/(t2-t1);
		interp_pos(nn)(tt-t0, 1) = object_pos(nn)(ss, 1)*(t2-tt)/(t2-t1)
		    +object_pos(nn)(ss+1, 1)*(tt-t1)/(t2-t1);

	    }
	}
	std::copy(counting_iterator<int>(t0),
		  counting_iterator<int>(t0+dt),
		  interp_time(nn).begin());
    }
}

void translate_frame_pos(vector<matrix<double> > const& object_pos,
			 vector<vector<int> > &object_time, int T,
			 vector<matrix<double> > & frame_pos,
			 vector<vector<int> > & frame_obj)
{
    using namespace boost::lambda;
    int num_obj = object_pos.size();

    frame_pos = vector<matrix<double> >(T);
    frame_obj = vector<vector<int> >(T);

    vector<std::vector<int> > o_idx(T);
    vector<std::vector<int> > s_idx(T);

    for(int oo=0; oo<num_obj; ++oo)
    {
	for(int ss=0; ss<object_time(oo).size(); ++ss)
	{
	    int tt = object_time(oo)(ss);
	    if(tt<T) 
	    {
		//std::cout<<"tt="<<tt<<std::endl;
		o_idx(tt).push_back(oo);
		s_idx(tt).push_back(ss);
	    }
	}
    }

    for(int tt=0; tt<T; ++tt)
    {
	frame_pos(tt) = scalar_matrix<double>(o_idx(tt).size(), 2, 0.0l);
	frame_obj(tt) = scalar_vector<int>(o_idx(tt).size(), 0);
	for(int kk=0; kk<o_idx(tt).size(); ++kk)
	{
	    int oo = o_idx(tt)[kk];
	    int ss = s_idx(tt)[kk];
	    frame_pos(tt)(kk, 0) = object_pos(oo)(ss, 0);
	    frame_pos(tt)(kk, 1) = object_pos(oo)(ss, 1);
	    frame_obj(tt)(kk) = oo;
	}
    }

}

template <class Mat1, class Mat2>
void compute_ground_map(Mat1 const& map,
			matrix<double> const& H_grd2img,
			ground_lim_t const& glim,
			double grid,
			Mat2 & gmap)
{
    typedef array2d_traits<Mat1> tr1;
    typedef array2d_traits<Mat2> tr2;

    int gsize1 = static_cast<int>((glim.ymax-glim.ymin)/grid)+1;
    int gsize2 = static_cast<int>((glim.xmax-glim.xmin)/grid)+1;

    tr2::change_size(gmap, gsize1, gsize2);

    matrix<double> prev(gsize1*gsize2, 2);
    matrix<int> m(gsize1*gsize2, 2);

    int ii=0; 
    for(int yy=0; yy<gsize1; ++yy)
    {
	for(int xx=0; xx<gsize2; ++xx)
	{
	    prev(ii, 0) = xx*grid+glim.xmin;
	    prev(ii, 1) = yy*grid+glim.ymin;
	    m(ii, 0) = xx;
	    m(ii, 1) = yy;
	    ++ii;
	}
    }

    matrix<double> after;
    homopgray_apply(H_grd2img, prev, after);

    for(int ii=0; ii<gsize1*gsize2; ++ii)
    {
	int yy = m(ii, 1);
	int xx = m(ii, 0);
	double iy = after(ii, 1);
	double ix = after(ii, 0);
	tr2::ref(gmap, yy, xx) = static_cast<typename tr2::value_type>
	    (map.linear_atXY(ix, iy)); //CImg
    }

}

void translate_ground_pos(vector<matrix<double> > const& interp_pos,
			  ground_lim_t const& glim, double grid,
			  vector<matrix<int> >& gmap_pos)
{

    int np = interp_pos.size();
    gmap_pos = vector<matrix<int> >(np);
    for(int pp=0; pp<np; ++pp)	
    {
	int nt = interp_pos(pp).size1();
	gmap_pos(pp) = matrix<int>(nt, 2);
	for(int tt=0; tt<nt; ++tt)
	{
	    gmap_pos(pp)(tt, 0) =
		static_cast<int>((interp_pos(pp)(tt, 0)-glim.xmin)/grid+0.5l);
	    gmap_pos(pp)(tt, 1) =
		static_cast<int>((interp_pos(pp)(tt, 1)-glim.ymin)/grid+0.5l);
	}
    }

}


void fix_object_pos(vector<matrix<int> > const& gmap_pos,
		    vector<vector<int> > const& interp_time,
		    matrix<int> const& obs,
		    vector<matrix<int> > & fixed_pos,
		    vector<vector<int> > & fixed_time)
{
    int np = gmap_pos.size();
    fixed_pos = vector<matrix<int> >(np);
    fixed_time = vector<vector<int> >(np);
    for(int pp=0; pp<np; ++pp)
    {
	std::vector<int> idx;
	for(int tt=0; tt<gmap_pos(pp).size1(); ++tt)
	{
	    int xx = gmap_pos(pp)(tt, 0);
	    int yy = gmap_pos(pp)(tt, 1);
	    if(xx>=0 && xx<obs.size2() && yy>=0 && yy<obs.size1())
	    {
		if(obs(yy, xx)==0) idx.push_back(tt);
	    }
	}
	if(idx.size()==0) continue;

	bool cont_t=true;
	for(int ss=0; ss+1<idx.size(); ++ss)
	{
	    if(idx[ss]+1!=idx[ss+1]) 
	    {
		cont_t = false;
		break;
	    }
	}

	if(!cont_t) continue;

	fixed_pos(pp) = matrix<int>(idx.size(), 2);
	fixed_time(pp) = vector<int>(idx.size());
	for(int ss=0; ss<idx.size(); ++ss)
	{
	    int tt = idx[ss];
	    fixed_pos(pp)(ss, 0) = gmap_pos(pp)(tt, 0);
	    fixed_pos(pp)(ss, 1) = gmap_pos(pp)(tt, 1);
	    fixed_time(pp)(ss) = interp_time(pp)(tt);
	}
    }
}


void test_wind_angle()
{
    int ng = 6;
    int no = 2;
    vector<vector<int> > sg(ng);
    vector<vector<double> > fdist(ng);
    matrix<int> ig2yx(ng, 2);
    matrix<double> obs(no, 2);

    sg(0) = vector<int>(2);
    sg(1) = vector<int>(3);
    sg(2) = vector<int>(2);
    sg(3) = vector<int>(2);
    sg(4) = vector<int>(3);
    sg(5) = vector<int>(2);

    sg(0) <<= 1, 3;
    sg(1) <<= 0, 2, 4;
    sg(2) <<= 1, 5;
    sg(3) <<= 0, 4;
    sg(4) <<= 1, 3, 5;
    sg(5) <<= 2, 4;

    fdist(0) = scalar_vector<double>(2, 1.0l);
    fdist(1) = scalar_vector<double>(3, 1.1l);
    fdist(2) = scalar_vector<double>(2, 1.2l);
    fdist(3) = scalar_vector<double>(2, 1.3l);
    fdist(4) = scalar_vector<double>(3, 1.4l);
    fdist(5) = scalar_vector<double>(2, 1.5l);

    ig2yx <<= 1, -2,
	1, 0,
	1, 2,
	-1, -2,
	-1, 0,
	-1, 2;

    obs <<= 1, 0,
	-1, 0;

    int wnum_l = -1;
    int wnum_u = 0;

    //int wnum_l = -2;
    //int wnum_u = 1;

    vector<vector<int> > result_path;
    vector<double> result_dist;
    vector<vector<int> > result_wind_num;
    wind_angle_planning(sg, fdist, ig2yx, obs, wnum_l, wnum_u, 0, 2,
			result_path, result_dist, result_wind_num);
}

int main(int argc, char* argv[])
{
    //test_dijk();
    //return 0;

    //test_wind_angle();
    //return 0;

    using namespace boost::lambda;
    directory_structure_t ds;
    ds.make_dir();

    std::vector<std::string> seq;
    read_jpeg_list(ds.prefix+"images/", seq);

    matrix<double> obsmat;
    read_text_array2d(ds.prefix+"obsmat.txt", obsmat);
    array2d_print(std::cout, project(obsmat, range(0, 5), range::all()));

    matrix_column<matrix<double> > colx(obsmat, 2), coly(obsmat, 4);

    ground_lim_t glim;
    glim.xmax = *std::max_element(colx.begin(), colx.end())+0.5l;
    glim.xmin = *std::min_element(colx.begin(), colx.end())-0.5l;
    glim.ymax = *std::max_element(coly.begin(), coly.end())+0.5l;;
    glim.ymin = *std::min_element(coly.begin(), coly.end())-0.5l;


    matrix<double> H;  //homography from image to ground
    read_text_array2d(ds.prefix+"H.txt", H);
    std::cout<<"H="<<std::endl;
    array2d_print(std::cout, H);

    std::cout<<"Hinv="<<std::endl;
    matrix<double> Hinv = inverse(H); //homography from ground to image

    array2d_print(std::cout, Hinv);

    int T = seq.size();

    std::string map_name = ds.prefix+"map2.png";
    CImg<unsigned char> map(map_name.c_str()); //map on image plane
    map.permute_axes("yxzc");
    CImg<float> gmap;
    double grid = 0.05;
    compute_ground_map(map, Hinv, glim, grid, gmap);
    {
	std::string name = ds.figures+"gmap.png";
	gmap.save_png(name.c_str());
    }

    vector<matrix<double> > object_pos;
    vector<vector<int> > object_time;
    extract_object_pos(obsmat,  object_pos, object_time);

    vector<matrix<double> > interp_pos;
    vector<vector<int> > interp_time;
    interpolate_object_pos(object_pos, object_time, interp_pos, interp_time);

    matrix<int> obs;
    array2d_transform(gmap, obs, ll_static_cast<int>(_1+0.5f)>=250);


    CImg<float> gmap_vis=gmap;
    gmap_vis.resize(-100, -100, -100, -300);
    draw_ground_positions(gmap_vis, interp_pos, glim, grid);
    {
	std::string name = ds.figures+"gtrj.png";
	gmap_vis.save_png(name.c_str());
    }

    vector<matrix<int> > gmap_pos;
    translate_ground_pos(interp_pos, glim, grid, gmap_pos);

    vector<matrix<int> > fixed_pos; //remove trj hits obstacles
    vector<vector<int> > fixed_time;

    fix_object_pos(gmap_pos, interp_time, obs, fixed_pos, fixed_time);

   
    for(int pp = 1; pp<80; ++pp) 
    {
	//int pp = 7;
	std::cout<<"object "<<pp<<std::endl;
	if(fixed_time(pp).size()==0) continue;
	int tt = fixed_time(pp)(0);
	matrix<int>& path = fixed_pos(pp);
	matrix<int> dyn_obs=scalar_matrix<int>(obs.size1(), obs.size2(), 0);

	std::vector<int> obs_x, obs_y;
	for(int oo=0; oo<fixed_pos.size(); ++oo)
	{
	    if(oo==pp) continue;
	    int nt = fixed_time(oo).size();
	    for(int ss=0; ss<nt; ++ss)
	    {
		if(tt!=fixed_time(oo)(ss)) continue;

		int x0 = fixed_pos(oo)(ss, 0);
		int y0 = fixed_pos(oo)(ss, 1);
		if(std::abs(x0-path(0, 0))<=5 && std::abs(y0-path(0, 1))<=5)
		   break;
		obs_x.push_back(x0);
		obs_y.push_back(y0);
		for(int xx=x0-4; xx<=x0+4; ++xx)
		{
		    if(xx<=0 || xx>=dyn_obs.size2()) continue;
		    for(int yy=y0-4; yy<=y0+4; ++yy)
		    {
			if(yy<=0 || yy>=dyn_obs.size1()) continue;
			dyn_obs(yy, xx) = 1;
		    }
		}
	    }
	}

	if(0 == obs_x.size()) continue;
	vector<matrix<float> > feat;
	generate_feature_maps(obs, dyn_obs, feat);

	for(int ii=0; ii<feat.size(); ++ii)
	{
	    CImg<unsigned char> vis_feat;
	    double maxf = *(std::max_element(feat(ii).data().begin(), feat(ii).data().end()));
	    array2d_transform(feat(ii), vis_feat, ll_static_cast<unsigned char>(_1*254/maxf));
	    std::string name = ds.figures+
		str(format("vis_feat%03d-%03d.png")%pp%ii);
	    vis_feat.save_png(name.c_str());
	}

	vector<vector<int> > sg; //(num_states, num_neighbors)
	matrix<vector<double> > fg; //(num_features, num_states, num_neighbors)
	matrix<int> yx2ig;
	matrix<int> ig2yx;
	std::cout<<"graphing"<<std::endl;
	real_timer_t timerx;
	get_state_graph(obs, dyn_obs, sg, yx2ig, ig2yx);
	get_feature_graph(obs, dyn_obs, feat, sg, ig2yx, fg);
	std::cout<<"graphing time:"<<timerx.elapsed()/1000.0f<<std::endl;

	vector<int> path_ig;
	get_path_gindex(path, yx2ig, path_ig);

	vector<double> wei(feat.size()+1);
	//wei <<= 3.03502, 7.03021, 15.8033, 2;
	wei <<= 1.61642, 7.03021, 15.3362, 1.23757;

	int ng = sg.size();
	vector<vector<double> > fdist(ng);
	for(int gg=0; gg<ng; ++gg)
	{
	    fdist(gg) = scalar_vector<double>(sg(gg).size(), 0.0l);
	    for(int nn=0; nn<sg(gg).size(); ++nn)
	    {
		for(int ff=0; ff<fg.size1(); ++ff)
		{
		    fdist(gg)(nn) += fg(ff, gg)(nn)*wei(ff);
		}
	    }
	}

	int start = path_ig(0);	
	int goal = path_ig(path_ig.size()-1);

	vector<int> spath;
	real_timer_t timerd;
	std::cout<<"dijkstra..."<<std::endl;
	//shortest_path(sg, fdist, start, goal, spath);
	int wnum_l = -1;
	int wnum_u = 0;
	if(obs_x.size()>3) //change to use critical obstacles
	{
	    obs_x.erase(obs_x.begin()+3, obs_x.end());
	    obs_y.erase(obs_y.begin()+3, obs_y.end());
	}
	matrix<double> obs_cent(obs_x.size(), 2);
	{
	    matrix_column<matrix<double> > colx(obs_cent, 0);
	    std::copy(obs_x.begin(), obs_x.end(), colx.begin());
	    matrix_column<matrix<double> > coly(obs_cent, 1);
	    std::copy(obs_y.begin(), obs_y.end(), coly.begin());
	}
	vector<vector<int> > result_path;
	vector<double> result_dist;
	vector<vector<int> > result_wind_num;
	wind_angle_planning(sg, fdist, ig2yx, obs_cent, wnum_l, wnum_u,
			    start, goal,
			    result_path, result_dist, result_wind_num);
	//construct_augmented_graph(sg, fdist, ig2yx, obs_cent, wnum_l, wnum_u);

	std::cout<<"dijkstra time:"<<timerd.elapsed()/1000.f<<std::endl;

#if 0
	std::cout<<"start="<<start<<std::endl;
	std::cout<<"goal="<<goal<<std::endl;
	std::cout<<"sg.size="<<sg.size()<<std::endl;
	std::cout<<"obs(start)="<<obs(path(0, 1), path(0, 0))<<std::endl;
	std::cout<<"obs(goal)="<<obs(path(path.size1()-1, 1),
				     path(path.size1()-1, 0))<<std::endl;
	std::cout<<"start="<<path(0, 1)<<", "<<path(0, 0)<<std::endl;
	std::cout<<"goal="<<path(path.size1()-1, 1)<<", "
		 <<path(path.size1()-1, 0)<<std::endl;
	std::cout<<"obs.size="<<obs.size1()<<", "<<obs.size2()<<std::endl;
#endif

	for(int zz=0; zz<result_path.size(); ++zz)
	{
	    CImg<unsigned char> vis_vmap;
	    array2d_transform(gmap, vis_vmap,
			      ll_static_cast<unsigned char>(_1));
	    unsigned char scol[3]={255, 0, 0};
	    unsigned char gcol[3]={0, 0, 255};
	    unsigned char lcol[3]={0, 255, 0};
	    unsigned char ocol[3]={0, 255, 255};

	    vis_vmap.resize(-100, -100, -100, -300);
	    vis_vmap.draw_circle(path(0, 0), path(0, 1), 5, scol);
	    vis_vmap.draw_circle(path(path.size1()-1, 0),
				 path(path.size1()-1, 1), 5, gcol);

	    for(int tt=0; tt+1<path.size1(); ++tt)
	    {
		vis_vmap.draw_line(path(tt, 0), path(tt, 1),
				   path(tt+1, 0), path(tt+1, 1), lcol);
	    }

	    vector<int>& spath = result_path(zz);
	    for(int ss=0; ss+1<spath.size(); ++ss)
	    {
		int gg = spath(ss);
		int g2 = spath(ss+1);
		vis_vmap.draw_line(ig2yx(gg, 1), ig2yx(gg, 0),
				   ig2yx(g2, 1), ig2yx(g2, 0),
				   gcol);
	    }

	    for(int oo=0; oo<obs_x.size(); ++oo)
	    {
		vis_vmap.draw_circle(obs_x[oo], obs_y[oo], 4, ocol);
	    }

	    std::string name = ds.figures+
		str(format("homotopy%03d-%03d.png")%pp%zz);
	    vis_vmap.save_png(name.c_str());
	}

    }

    return 0;
}
