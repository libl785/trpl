template <class Mat, class Float>
void preapre_affinity_matrix(umf_sparse_matrix& W,
			     const Mat& mat, int nc,
			     Float sig, int KN)
{
    typedef require_same_type<typename Mat::value_type, Float>
	Mat_value_type_should_be_the_same_as_Float;

    int nrow = mat.size1();
    int ncol = mat.size2()/nc;
    int np = nrow*ncol;
    int Nbr = (KN*KN+KN)*4+(KN==0)*4;

    W = umf_sparse_matrix(np, np, (Nbr+1)*np);
    int ndi[Nbr];
    int ndj[Nbr];
    int bb=0;
    if(KN==0) {
	ndi[0]= 1;  ndj[0]=0;
	ndi[1]= 0;  ndj[1]=1;
	ndi[2]= -1; ndj[2]=0;
	ndi[3]= 0;  ndj[3]=-1;
    }
    else
    {
	for(int di=-KN; di <= KN; ++di)
	{
	    for(int dj = -KN; dj <= KN; ++dj)
	    {
		if(di==0 &&  dj==0) continue;
		ndi[bb] = di;
		ndj[bb] = dj;
		bb++;
	    }
	}
    }

    for(int ii=0; ii<nrow; ++ii)
    {
	for(int jj=0; jj<ncol; ++jj)
	{
	    int iii = ii*ncol+jj;
	    W(iii, iii) = 1e-10; //keep room for diagonal D
	    for(int nn=0; nn<Nbr; ++nn)
	    {
		int i2 = ii+ndi[nn];
		int j2 = jj+ndj[nn];
		if(i2<0) continue;
		if(j2<0) continue;
		if(i2>=nrow) continue;
		if(j2>=ncol) continue;

		int jjj = i2*ncol+j2;
		if(jjj>=iii) continue;
		Float dI=0;
		for(int cc=0; cc<nc; ++cc)
		{
		    Float dpix = mat(ii, jj*nc+cc)-mat(i2, j2*nc+cc);
		    dI += dpix*dpix;
		}
		Float sdI = std::sqrt(dI)/sig;
		W(iii, jjj) = std::exp(-dI*dI/2);
		W(jjj, iii) = W(iii, jjj);
	    }
	}
    }
    //std::cout<<"\t\t\ttime to build W is "<<timer.elapsed()<<std::endl;

}


template <typename Float, typename Int>
matrix<Float> extract_rect_image(matrix<Float> const & mat, Int nc,
				 array<Int, 4> const & ext_ri)
{
    range ry(ext_ri[1], ext_ri[3]+1);
    range rx(ext_ri[0]*nc, ext_ri[2]*nc+1*nc);
    return project(mat, ry, rx);
}


template <typename Float, typename Int>
void prepare_ext_box(const vector<Float>& box,
		     Int s1, Int s2,
		     boost::array<Int, 4>& ext_ri,
		     boost::array<Int, 4>& in_ri)
{
    Float rect_cent_x = (box(0)+box(2))/2;
    Float rect_cent_y = (box(1)+box(3))/2;

    Float rect_w = box(2)-box(0);
    Float rect_h = box(3)-box(1);

    vector<Float> ext_rect(4);
    ext_rect(0) = rect_cent_x-rect_w/2*2.0;
    ext_rect(1) = rect_cent_y-rect_h/2*1.2;
    ext_rect(2) = rect_cent_x+rect_w/2*2.0;
    ext_rect(3) = rect_cent_y+rect_h/2*1.2;
   

    std::transform(ext_rect.begin(), ext_rect.end(), ext_ri.begin(), 
		   ll_static_cast<Int>(_1+0.5f));
    std::transform(box.begin(), box.end(), in_ri.begin(), 
		   ll_static_cast<Int>(_1+0.5f));


    if(ext_ri[0]<0)    {  ext_ri[0] = 0;   }
    if(ext_ri[1]<0)    {  ext_ri[1] = 0;   }
    if(ext_ri[2]>=s2)    {  ext_ri[2] = s2-1;   }
    if(ext_ri[3]>=s1)    {  ext_ri[3] = s1-1;   }

}

template <typename Float, typename Int>
void prepare_ext_box(gaussian_t<Float> const& g,
		     Int s1, Int s2,
		     array<Int, 4>& inbb,
		     array<Int, 4>& extbb)
{

    Float rect_cent_x = g.mean(0);
    Float rect_cent_y = g.mean(1);

    //new size sometimes does not work because of really bad segment.
    //Float rect_w = 3*std::sqrt(g.var(0, 0))+1;
    //Float rect_h = 3*std::sqrt(g.var(1, 1))+1;

    //temporally use the old size
    Float rect_w = (inbb[2]-inbb[0])/2.0f;
    Float rect_h = (inbb[3]-inbb[1])/2.0f;

    //vector<Float> ext_rect(4);
    inbb[0] = static_cast<int>(rect_cent_x-rect_w+0.5f);
    inbb[1] = static_cast<int>(rect_cent_y-rect_h+0.5f);
    inbb[2] = static_cast<int>(rect_cent_x+rect_w+0.5f);
    inbb[3] = static_cast<int>(rect_cent_y+rect_h+0.5f);   

    extbb[0] = static_cast<int>(rect_cent_x-rect_w*2.4+0.5f);
    extbb[1] = static_cast<int>(rect_cent_y-rect_h*1.2+0.5f);
    extbb[2] = static_cast<int>(rect_cent_x+rect_w*2.4+0.5f);
    extbb[3] = static_cast<int>(rect_cent_y+rect_h*1.2+0.5f);   

    if(extbb[0]<0)    {  extbb[0] = 0;   }
    if(extbb[1]<0)    {  extbb[1] = 0;   }
    if(extbb[2]>=s2)    {  extbb[2] = s2-1;   }
    if(extbb[3]>=s1)    {  extbb[3] = s1-1;   }

    if(inbb[0]<0)    {  inbb[0] = 0;   }
    if(inbb[1]<0)    {  inbb[1] = 0;   }
    if(inbb[2]>=s2)    {  inbb[2] = s2-1;   }
    if(inbb[3]>=s1)    {  inbb[3] = s1-1;   }

}

template <typename Float, typename Int>
void prepare_shape_map(const boost::array<Int, 4>& ext_ri,
		       const boost::array<Int, 4>& in_ri,
		       matrix<Float>& smap)
{
    Int s2 = ext_ri[2]-ext_ri[0]+1;
    Int s1 = ext_ri[3]-ext_ri[1]+1;

    Int w = in_ri[2]-in_ri[0]+1;
    Int h = in_ri[3]-in_ri[1]+1;

    Float centx = (in_ri[2]+in_ri[0])/2.0f;
    Float centy = (in_ri[3]+in_ri[1])/2.0f;

    Float dcx = centx-ext_ri[0];
    Float dcy = centy-ext_ri[1];

    Float sc = 3.0f;
    Float rate = std::sqrt((1-1/sc/sc)/(2*std::log(sc)));
    Float sigw = w/2.0f*rate;
    Float sigh = h/2.0f*rate;

    //the shape map is a DOG, which zero level set enclosed by the BB
    //the relative scale of the two Gaussians are sc=3.0
    //rate is used to align the zero level set of the DOG to the BB

    smap = matrix<Float>(s1, s2);

    for(Int yy=0; yy<s1; ++yy)
    {
	Float dy = yy-dcy;
	dy /= sigh;
	for(Int xx=0; xx<s2; ++xx)
	{
	    Float dx = xx-dcx;
	    dx /= sigw;
	    Float d2 = dx*dx+dy*dy;
	    Float score = -0.5*(1-1/sc/sc)*d2+std::log(sc);
	    smap(yy, xx) = score;
	}
    }
}

template <typename Float, typename Int>
void prepare_shape_map(array<Int, 4> const& ext_ri,
		       array<gaussian_t<Float>, 2> const& pos_models, 
		       matrix<Float>& smap)
{
    int s1 = ext_ri[3]-ext_ri[1]+1;
    int s2 = ext_ri[2]-ext_ri[0]+1;
    int np = s1*s2;
    matrix<Float> data(2, np);
    int pp=0;
    for(int yy=0; yy<s1; ++yy)
    {
	for(int xx=0; xx<s2; ++xx)
	{
	    data(0, pp) = xx;
	    data(1, pp) = yy;
	    ++pp;
	}
    }
    array<vector<Float>, 2> loglike;
    for(int ii=0; ii<2; ++ii)
    {
	gaussian_loglike(pos_models[ii], data, loglike[ii]);
    }
    vector<Float> dll = loglike[0]-loglike[1];
    smap = matrix<Float>(s1, s2);
    std::copy(dll.begin(), dll.end(), smap.data().begin());
}



// remove small segments from a binary segment map
template <class Int>
void remove_small_connected_components(matrix<Int>& seg, Int thr)
{
    matrix<Int> cmpmap(seg.size1(), seg.size2());
    int ncomp = labelmap_connected_components(seg, cmpmap);
    if(ncomp>2)     //remove small regions
    {
	vector<Int> cmpcount=scalar_vector<int>(ncomp, 0);
	labelmap_count_components(cmpmap, ncomp, cmpcount);
	vector<bool> small_flag(ncomp);
	std::transform(cmpcount.begin(), cmpcount.end(), small_flag.begin(), _1<=thr);
	matrix<Int> seg2 = seg;
	tile_panel_t tp(1, 3, seg.size1(), seg.size2());
	tp.add_image_gray(0, 0, seg2);
	for(int ii=0; ii<seg.size1(); ++ii)
	{
	    for(int jj=0; jj<seg.size2(); ++jj)
	    {
		int cpid = cmpmap(ii, jj);
		if(small_flag(cpid)) seg(ii, jj) = 1-seg(ii, jj);
	    }
	}
	tp.add_image_gray(0, 1, seg);
	matrix<int> diff=seg-seg2;
	tp.add_image_gray(0, 2, diff);

    }

}

//threshold a score map into a binary segment map
// prevent collapsing into a single segment
template <class Mat1, class Mat2>
void score_to_segment(const Mat1& X, Mat2& seg, float fgrl, float fgru)
{
    using namespace boost::lambda;
    std::transform(X.data().begin(), X.data().end(), seg.data().begin(), ll_static_cast<int>(_1>0.0f));
    int fg_count = std::count_if(seg.data().begin(), seg.data().end(), _1>0);
    int np = X.data().size();
    int fgl = static_cast<int>(np*fgrl);
    int fgu = static_cast<int>(np*fgru);
    if(fg_count >= fgl&& fg_count <= fgu)  return;
    if(fg_count < fgl) fg_count = fgl; //prevent degeneration when low contrast
    if(fg_count > fgu) fg_count = fgu; //prevent degeneration when low contrast

    typedef typename Mat1::value_type Float;
    typedef std::pair<Float, int> Pair;

    vector<Pair> x2(X.data().size());
    std::transform(X.data().begin(), X.data().end(), boost::counting_iterator<int>(0),
		  x2.begin(), bind(std::make_pair<Float, int>, _1, _2));
    std::sort(x2.begin(), x2.end(), bind(&Pair::first, _1)< bind(&Pair::first, _2));

    for(int ii=0; ii<x2.size(); ++ii)
    {
	int jj = x2(ii).second;
	seg.data()[jj] = (ii>=np-fg_count);
    }

}

template <class Float>
void do_em_gmm(int K, gaussian_mixture_t<Float>& model, const matrix<Float>& datax,
	       int stride, int it)
{
    int np = datax.size2();
    int ndim = datax.size1();
    if(np<=1000 || it>0)
    {
	EM_subsamp_opt<Float> opt(stride, it!=0); //self init (random)
	EM_gaussian_mixture(datax, K, model, opt);
    }
    else
    {
	int s = np/500;
	slice current_slice(0, s, (np-1)/s+1);
	slice all_dim(0, 1, ndim);

	matrix<Float> dataz = project(datax, all_dim, current_slice);

	EM_plain_opt<Float> opt; //self init (random)
	EM_gaussian_mixture(dataz, K, model, opt);
	EM_subsamp_opt<Float> opt2(stride, true); 
	EM_gaussian_mixture(datax, K, model, opt2);

    }
}


template <typename Float>
void spatial_gaussian_from_seg(matrix<int> const& seg,
			       array<gaussian_t<Float>, 2>& pos_models)
{
    array<vector<Float>, 2> pm={scalar_vector<Float>(2, 0), scalar_vector<Float>(2, 0)};
    array<matrix<Float>, 2> pv={scalar_matrix<Float>(2, 2, 0), scalar_matrix<Float>(2, 2, 0)};

    int fc[2]={0, 0};
    for(int yy=0; yy<seg.size1(); ++yy)
    {
	for(int xx=0; xx<seg.size2(); ++xx)
	{
	    int ii;
	    if(seg(yy, xx)>0) ii=0; 
	    else ii = 1;

	    pm[ii](0) += xx;
	    pm[ii](1) += yy;
	    fc[ii]++;
	}
    }
    for(int ii=0; ii<2; ++ii)
    {
	pm[ii] /= fc[ii];
    }
    for(int yy=0; yy<seg.size1(); ++yy)
    {
	for(int xx=0; xx<seg.size2(); ++xx)
	{
	    vector<Float> v(2);
	    v(0) = xx; v(1) = yy;
	    int ii;
	    if(seg(yy, xx)>0) ii=0;
	    else ii = 1;

	    vector<Float> dv = v-pm[ii];
	    pv[ii] += outer_prod(dv, dv);
	}
    }
    for(int ii=0; ii<2; ++ii)
    {
	pv[ii] /= fc[ii];
    }

    for(int ii=0; ii<2; ++ii)
    {
	pos_models[ii].mean = pm[ii];
	pos_models[ii].var = pv[ii];
    }

}

