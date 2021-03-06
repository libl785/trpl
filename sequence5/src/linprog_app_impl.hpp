#ifndef __LINPROG__APP__IMPL__HPP__INCLUDED__
#define __LINPROG__APP__IMPL__HPP__INCLUDED__


void prepare_app_affinity(matrix<int> const& Tff,
			  vector<object_trj_t> const& trlet_list, float thr,
			  matrix<float>& Aff)
{
    int ng = Tff.size1();
    Aff = scalar_matrix<float>(ng, ng, 0);

    for(int ii=0; ii<ng; ++ii)
    {
	int nn1 = ii; //good_trlet_index(ii);
	object_trj_t const& trj1 = trlet_list(nn1);
	for(int jj=0; jj<ng; ++jj)
	{
	    int nn2 = jj; //good_trlet_index(jj);
	    object_trj_t const& trj2 = trlet_list(nn2);
	    if(!Tff(ii, jj)) continue;
	    float match = appmodel_match(trj1.hist_p, trj1.hist_q,
					 trj2.hist_p, trj2.hist_q);
	    Aff(ii, jj) = match - thr;
	}
    }
}


template <class Float>
Float appmodel_match_one(Float hp1, Float hq1, Float hp2, Float hq2, Float ep)
{
    Float match = 0;
    match += hp1*std::log( (hp2+ep)/(hq2+ep) );
    match += hp2*std::log( (hp1+ep)/(hq1+ep) );
    return match;
}

template <class Float>
Float appmodel_match(vector<matrix<Float> > const& hp1,
		     vector<matrix<Float> > const& hq1,
		     vector<matrix<Float> > const& hp2,
		     vector<matrix<Float> > const& hq2)
{
    using namespace boost::lambda;

    Float match = 0;
    Float ep = 1e-6;
    //Float (*flog)(Float) = std::log;

    for(int cam=0; cam<hp1.size(); ++cam)
    {
	for(int pp=0; pp<hp1(cam).size1(); ++pp)
	{
	    for(int bb=0; bb<hp1(cam).size2(); ++bb)
	    {
		match += appmodel_match_one(hp1(cam)(pp, bb), hq1(cam)(pp, bb),
					    hp2(cam)(pp, bb), hq2(cam)(pp, bb), ep);
	    }
	}
    }
    return match;
}


void prepare_occl_affinity(vector<std::vector<std::string> > const& seq,
			   geometric_info_t const& gi,
			   parameter_t const& P,
			   vector<object_trj_t> const& trlet_list,
			   vector<array<float, 4> > const& model,
			   matrix<float> const& Tff,
			   matrix<float>& Ocff,
			   matrix<object_trj_t>& gap_trlet_list)
{

    int Ncam = gi.img2grd.size2();
    //int ng = good_trlet_index.size();
    int num_obj = trlet_list.size();
    int ng = num_obj;
    gap_trlet_list = matrix<object_trj_t>(ng, ng);

    int T = seq[0].size();

    //1. Allocate and fill bbs
    for(int ii=0; ii<ng; ++ii)
    {
	int nn1 = ii; //good_trlet_index(ii);
	for(int jj=0; jj<ng; ++jj)
	{
	    int nn2 = jj; //good_trlet_index(jj);
	    if(!Tff(ii, jj)) continue;
	    int t1 = trlet_list(nn1).endt;
	    int t2 = trlet_list(nn2).startt;
	    object_trj_t& gap_trlet=gap_trlet_list(ii, jj);
	    gap_trlet.startt = t1+1;
	    gap_trlet.endt = t2-1;

	    gap_trlet.trj = vector<matrix<float> >(Ncam);
	    gap_trlet.scores = scalar_matrix<float>(Ncam, T, 0);
	    for(int cam=0; cam<Ncam; ++cam)
	    {
		gap_trlet.trj(cam) = scalar_matrix<float>(T, 4, 0);
	    }
	    matrix<float> ww(2, Ncam), hh(2, Ncam);
	    for(int cam=0; cam<Ncam; ++cam)
	    {
		ww(0, cam) = trlet_list(nn1).trj(cam)(t1, 2)-trlet_list(nn1).trj(cam)(t1, 0);
		ww(1, cam) = trlet_list(nn2).trj(cam)(t2, 2)-trlet_list(nn2).trj(cam)(t2, 0);
		hh(0, cam) = trlet_list(nn1).trj(cam)(t1, 3)-trlet_list(nn1).trj(cam)(t1, 1);
		hh(1, cam) = trlet_list(nn2).trj(cam)(t2, 3)-trlet_list(nn2).trj(cam)(t2, 1);
	    }
	    gap_trlet.trj_3d = scalar_matrix<float>(T, 2, 0);

	    vector<float> pos1(row(trlet_list(nn1).trj_3d, t1));
	    vector<float> pos2(row(trlet_list(nn2).trj_3d, t2));

	    for(int tt=t1+1; tt<=t2-1; ++tt)
	    {
		vector<float> pos( (pos1*(t2-tt)+pos2*(tt-t1))/(t2-t1));
		row(gap_trlet.trj_3d, tt) = pos;
		for(int cam=0; cam<Ncam; ++cam)
		{
		    vector<double> gx(1), gy(1), ix, iy;
		    gx <<= pos(0);
		    gy <<= pos(1);
		    apply_homography(gi.grd2img(tt, cam), gx, gy, ix, iy);
		    float wwt = (ww(0, cam)*(t2-tt)+ww(1, cam)*(tt-t1))/(t2-t1);
		    float hht = (hh(0, cam)*(t2-tt)+hh(1, cam)*(tt-t1))/(t2-t1);

		    vector<float> bodyr(4);
		    bodyr <<= (ix(0)-wwt/2), (iy(0)-hht), (ix(0)+wwt/2), iy(0);
		    row(gap_trlet.trj(cam), tt) = bodyr;

		}
	    }

	}
    }

    //2. Refinement and get scores
    vector<float> dx(3), dy(3);
    //dx <<= -4, -2, 0, 2, 4;
    //dy <<= -4, -2, 0, 2, 4;
    dx <<= -4, 0, 4;
    dy <<= -4, 0, 4;

    for(int tt=0; tt<T; ++tt)
    {
	std::vector<int> iis, jjs;
	for(int ii=0; ii<ng; ++ii)
	{
	    for(int jj=0; jj<ng; ++jj)
	    {
		if(gap_trlet_list(ii, jj).trj.size()==0) continue;
		int t1 = gap_trlet_list(ii, jj).startt;
		int t2 = gap_trlet_list(ii, jj).endt;
		if(t1<=tt&&t2>=tt)
		{
		    iis.push_back(ii);
		    jjs.push_back(jj);
		}
	    }
	}
	if(iis.size()<=0) continue;

	vector<CImg<unsigned char> > images(Ncam);
	vector<matrix<float> > grd(Ncam);

	for(int cam=0; cam<Ncam; ++cam)
	{
	    images[cam] = CImg<unsigned char>(seq[cam][tt].c_str());
	}
	//std::cout<<"tt="<<tt<<std::endl;

	for(int kk=0; kk<iis.size(); ++kk)
	{
	    int ii = iis[kk];
	    int jj = jjs[kk];
	    int nn1 = ii; //good_trlet_index(ii);
	    int nn2 = jj; //good_trlet_index(jj);
	    object_trj_t& gap_trlet=gap_trlet_list(ii, jj);

	    for(int cam=0; cam<Ncam; ++cam)
	    {
		vector<float> bodyr(row(gap_trlet.trj(cam), tt));
		matrix<float> cand_rects;
		enumerate_rects_refine(bodyr, dx, dy, cand_rects);
		vector<float> cand_score_sum(scalar_vector<float>(cand_rects.size1(), 0));

		array<int, 2> nns={nn1, nn2};
		for(int ll=0; ll<nns.size(); ++ll)
		{
		    int nn = nns[ll];
		    matrix<float> hist_fscores;
		    vector<float> cand_hist_score;
		    get_cand_hist_score(images(cam), model, P.logp1, P.logp2,
					trlet_list(nn).hist_p(cam),
					trlet_list(nn).hist_q(cam),
					cand_rects,
					cand_hist_score, hist_fscores);
		    for(int bb=0; bb<cand_hist_score.size(); ++bb)
		    {
			if(isnan(cand_hist_score(bb))) cand_hist_score(bb) = -10;
		    }

		    cand_score_sum += cand_hist_score;
		}
		int idx_max = std::max_element(cand_score_sum.begin(),
					       cand_score_sum.end())
		    -cand_score_sum.begin();
		gap_trlet.scores(cam, tt) = cand_score_sum(idx_max)/2;
		row(gap_trlet.trj(cam), tt) = row(cand_rects, idx_max);
	    }
	}
    }


    //3. Compute Ocff
    Ocff = scalar_matrix<float>(ng, ng, 0);

    for(int ii=0; ii<ng; ++ii)
    {
	int nn1 = ii; //good_trlet_index(ii);
	for(int jj=0; jj<ng; ++jj)
	{
	    int nn2 = jj; //good_trlet_index(jj);
	    if(!Tff(ii, jj)) continue;
	    object_trj_t gap_trlet=gap_trlet_list(ii, jj);
	    for(int cam=0; cam<Ncam; ++cam)
	    {
		for(int tt=gap_trlet.startt; tt<=gap_trlet.endt; ++tt)
		{
		    Ocff(ii, jj) += sat2(gap_trlet.scores(cam, tt), 2.0f, 8.0f) -2;
		}
		Ocff(ii, jj) = sat(Ocff(ii, jj), 30.0f);
	    }
	}
    }

}

void enumerate_rects_refine(vector<float> const& rect,
			    vector<float> const& dx, vector<float> const& dy,
			    matrix<float>& cand_rects)
{
    int sx = dx.size();
    int sy = dy.size();
    cand_rects = scalar_matrix<float>(sx*sy, 4, 0);
    int rr=0;
    for(int yy=0; yy<sy; ++yy)
    {
	for(int xx=0; xx<sx; ++xx)
	{
	    cand_rects(rr, 0) = rect(0) + dx(xx);
	    cand_rects(rr, 1) = rect(1) + dy(yy);
	    cand_rects(rr, 2) = rect(2) + dx(xx);
	    cand_rects(rr, 3) = rect(3) + dy(yy);
	    rr++;
	}
    }
}


#endif
