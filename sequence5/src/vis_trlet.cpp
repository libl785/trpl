
#include "tracking_data_package.hpp"
#include "tracking_detail.hpp"

#include "segment_parts.hpp"


template <class M1, class MV, class M2>
void colorize(M1 const& m1, MV const& cmap, M2& m2);


void load_raw_trlet_list( directory_structure_t & ds, vector<object_trj_t>& trlet_list)
{

    {
	std::string name = ds.workspace+"raw_trlet_list.xml";
	std::ifstream fin(name.c_str());
	boost::archive::xml_iarchive ia(fin);
	ia >> BOOST_SERIALIZATION_NVP(trlet_list);
    }


}



int main(int argc, char* argv[])
{
    using namespace boost::lambda;

    directory_structure_t ds;

    vector<object_trj_t> trlet_list;
    load_raw_trlet_list(ds, trlet_list);
    int num_obj = trlet_list.size();

    vector<matrix<matrix<unsigned char> > >  seg_list;
    {
	std::string name = ds.workspace+"seg_list.txt";
	if(!fs::exists(name)) std::cout<<"warning: seg_list file not found!"<<std::endl;
	load_seg_list(name, seg_list);
    }



    vector<std::vector<std::string> > seq(2);
    read_sequence_list(ds.prefix, seq);
    int T = seq[0].size();
    int Ncam = 2;

    matrix<vector<CImg<unsigned char> > > patches(num_obj, Ncam);
    for(int nn=0; nn<trlet_list.size(); ++nn)
    {
	for(int cam=0; cam<Ncam; ++cam)
	{
	    int dt = trlet_list(nn).endt+1-trlet_list(nn).startt;
	    patches(nn, cam) = vector<CImg<unsigned char> >(dt);
	}
    }

    for(int tt=0; tt<T; tt++)
    {
	//real_timer_t frame_timer;
	vector<CImg<unsigned char> > images(Ncam);
	vector<matrix<float> > grd(Ncam);

	unsigned char ccol[3]={255, 255, 0};

	unsigned char fcol[3]={255, 255, 255};
	unsigned char bcol[3]={0, 0, 0};

	for(int cam=0; cam<Ncam; ++cam)
	{
	    images[cam] = CImg<unsigned char>(seq[cam][tt].c_str());

	    CImg<unsigned char> vis(images[cam]);

	    for(int nn=0; nn<num_obj; ++nn)
	    {
		if(tt<trlet_list(nn).startt) continue;
		if(tt>trlet_list(nn).endt) continue;
		vector<float> b = row(trlet_list(nn).trj(cam), tt);
		boost::array<int, 4> box;
		std::transform(b.begin(), b.end(), box.begin(),
			       ll_static_cast<int>(_1+0.5f));

		vis.draw_line(box[0], box[1], 0, box[0], box[3], 0, ccol, 1);
		vis.draw_line(box[2], box[1], 0, box[2], box[3], 0, ccol, 1);
		vis.draw_line(box[0], box[1], 0, box[2], box[1], 0, ccol, 1);
		vis.draw_line(box[0], box[3], 0, box[2], box[3], 0, ccol, 1);
		patches(nn, cam)(tt-trlet_list(nn).startt) =
		    images[cam].get_crop(box[0], box[1], box[2], box[3]);
		std::string numstr(lexical_cast<std::string>(nn));

		vis.draw_text (box[0], box[1], numstr.c_str(), fcol, bcol);
	    }
	    
	    std::string image_name = fs::basename(fs::path(seq[cam][tt]));

	    fs::path fig_path = fs::path(ds.figures)/(image_name+"_fig.jpg");
	    vis.save_jpeg(fig_path.string().c_str(), 90);

	}
    }
    typedef array3d_traits<CImg<unsigned char> > A;

    vector<CImg<unsigned char> > tile(num_obj);
    for(int nn=0; nn<num_obj; ++nn)
    {
	int ww = patches(nn, 0)(0).width();
	int hh = patches(nn, 0)(0).height();

	for(int cam=0; cam<Ncam; ++cam)
	{
	    for(int tt=0; tt<patches(nn, cam).size(); ++tt)
	    {
		patches(nn, cam)(tt).resize(ww, hh);
	    }
	}

	A::change_size(tile(nn), 3, hh*Ncam, ww*patches(nn, 0).size());

	for(int cam=0; cam<Ncam; ++cam)
	{
	    for(int tt=0; tt<patches(nn, cam).size(); ++tt)
	    {
		add_patch_to_tile(tile(nn), patches(nn, cam)(tt), cam, tt);

	    }
	}

	std::string fig_path=boost::str(boost::format(ds.figures+"patch%04d.jpg")%nn);
	tile(nn).save_jpeg(fig_path.c_str(), 90);
	std::cout<<"Object "<<nn<<" done!"<<std::endl;
    }

    for(int nn=num_obj; nn<20000; ++nn)
    {
	std::string fig_path=boost::str(boost::format(ds.figures+"patch%04d.jpg")%nn);
	if(fs::exists(fig_path))  fs::remove(fig_path);
	else break;
    }
////////////////////////////////////////////////////////////////

    matrix<unsigned char> cmap(3, 3);
    cmap<<= 0, 0, 255, 0, 255, 0, 255, 0, 0;
    matrix<vector<CImg<unsigned char> > > seg_patches(num_obj, Ncam);
    for(int nn=0; nn<trlet_list.size(); ++nn)
    {
	for(int cam=0; cam<Ncam; ++cam)
	{
	    int dt = trlet_list(nn).endt+1-trlet_list(nn).startt;
	    seg_patches(nn, cam) = vector<CImg<unsigned char> >(dt);
	    for(int tt=trlet_list(nn).startt; tt<=trlet_list(nn).endt; ++tt)
	    {
		colorize(seg_list(cam)(nn, tt), cmap, seg_patches(nn, cam)(tt-trlet_list(nn).startt));
	    }
	}
    }

    vector<CImg<unsigned char> > tile2(num_obj);
    for(int nn=0; nn<num_obj; ++nn)
    {
	int ww = patches(nn, 0)(0).width();
	int hh = patches(nn, 0)(0).height();

	for(int cam=0; cam<Ncam; ++cam)
	{
	    for(int tt=0; tt<seg_patches(nn, cam).size(); ++tt)
	    {
		seg_patches(nn, cam)(tt).resize(ww, hh);
	    }
	}

	A::change_size(tile2(nn), 3, hh*Ncam*2, ww*patches(nn, 0).size());

	for(int cam=0; cam<Ncam; ++cam)
	{
	    for(int tt=0; tt<patches(nn, cam).size(); ++tt)
	    {
		add_patch_to_tile(tile2(nn), patches(nn, cam)(tt), cam*2+0, tt);
		add_patch_to_tile(tile2(nn), seg_patches(nn, cam)(tt), cam*2+1, tt);

	    }
	}

	std::string fig_path=boost::str(boost::format(ds.figures+"seg_patch%04d.jpg")%nn);
	tile2(nn).save_jpeg(fig_path.c_str(), 90);
	std::cout<<"Object seg_patch "<<nn<<" done!"<<std::endl;
    }


////////////////////////////////////////////////////////////////////////////////
// Show ground trj

    array<std::size_t, 2> img_size = {768, 1024};
    geometric_info_t gi;
    gi.load(ds, img_size);

    CImg<unsigned char> ground_fig;
    array3d_traits<CImg<unsigned char> >::change_size(ground_fig, 3,
						      gi.ground_lim.ymax, gi.ground_lim.xmax);

    array3d_fill(ground_fig, 255);

    unsigned char pcol[3]={0, 0, 255};
    unsigned char tcol[3]={255, 0, 0};

    CImg<double> poly;
    array2d_copy(gi.poly_ground, poly);

    ground_fig.draw_polygon(poly, pcol);
    for(int nn=0; nn<num_obj; ++nn)
    {
	if(trlet_list(nn).trj.size()==0) continue;
	int t1 = trlet_list(nn).startt;
	int t2 = trlet_list(nn).endt;
	if(t2-t1+1<3) continue;

	object_trj_t::trj_3d_t& trj=trlet_list(nn).trj_3d;

	for(int tt=t1; tt<t2; ++tt)
	{
	    ground_fig.draw_line(static_cast<int>(trj(tt, 0)+0.5f),
				 static_cast<int>(trj(tt, 1)+0.5f), 
				 static_cast<int>(trj(tt+1, 0)+0.5f),
				 static_cast<int>(trj(tt+1, 1)+0.5f), 
				 tcol, 1);
	}
    }

    {
	ground_fig.mirror('y');
	std::string name = ds.figures+"ground_trj_fig.png";
	ground_fig.save_png(name.c_str());
    }

/////////////////////////////////////////////////////////////////////////
//
    int mw = 100;
    int mh = 240;
    matrix<CImg<unsigned char> > model_vis(num_obj, Ncam);
    for(int nn=0; nn<num_obj; ++nn)
    {
	for(int cam=0; cam<Ncam; ++cam)
	{
	    A::change_size(model_vis(nn, cam), 3, mh, mw);
	    array3d_fill(model_vis(nn, cam), 255);
	    matrix<float> const& hist_p = trlet_list(nn).hist_p(cam);
	    int np = hist_p.size1();
	    for(int pp=0; pp<np; ++pp)
	    {
		matrix_row<matrix<float> const> r(hist_p, pp);
		int idx_max = std::max_element(r.begin(), r.end()) - r.begin();
		int ir = idx_max%8;
		int ig = idx_max/8%8;
		int ib = idx_max/8/8;
		unsigned char ccol[3]={ir*255/8+4, ig*255/8+4, ib*255/8+4};
		model_vis(nn, cam).draw_rectangle(0, pp*mh/np, 99, (pp+1)*mh/np-1, ccol, 1); 
	    }
	}
    }

    for(int cam=0; cam<Ncam; ++cam)
	{
	    CImg<unsigned char> tile_model;
	    int nhoriz = static_cast<int>(std::sqrt(static_cast<float>(num_obj+1)));
	    int nvert = nhoriz;
	    A::change_size(tile_model, 3, nvert*240, nhoriz*2*100);
	    int nn=0;
	    for(int yy=0; yy<nvert; ++yy)
	    {
		for(int xx=0; xx<nhoriz; ++xx)
		{
		    if(nn>=model_vis.size1()) break;
		    patches(nn, cam)(0).resize(mw, mh);
		    add_patch_to_tile(tile_model, patches(nn, cam)(0), yy, xx*2+0);
		    add_patch_to_tile(tile_model, model_vis(nn, cam), yy, xx*2+1);
		    ++nn;
		}
	    }
	    {
		std::string name = ds.figures+str(format("model_tile_cam%d.png")%cam);
		tile_model.save_png(name.c_str());
	    }
	}

    return 0;

}


template <class M1, class MV, class M2>
void colorize(M1 const& m1, MV const& cmap, M2& m2)
{
    typedef array2d_traits<M1> A;
    typedef array3d_traits<M2> B;
    typedef array2d_traits<MV> C;

    typedef typename A::value_type Int;

    B::change_size(m2, C::size2(cmap), A::size1(m1), A::size2(m1));

    for(int cc=0; cc<B::size1(m2); ++cc)
    {
	for(int yy=0; yy<B::size2(m2); ++yy)
	{
	    for(int xx=0; xx<B::size3(m2); ++xx)
	    {
		Int v=A::ref(m1, yy, xx);
		if(v<0 || v>cmap.size1()-1)
		    B::ref(m2, cc, yy, xx) = 0;
		else
		    B::ref(m2, cc, yy, xx) = cmap(v, cc);
	    }
	}
    }
}

#include "segment_parts_impl.hpp"

