#include "tracking_data_package.hpp"
#include "tracking_detail.hpp"

#include "linprog_app.hpp"
#include "lp.hpp"

#include "finalize_trajectory.hpp"

float app_match_thr = -2;

int main(int argc, char* argv[])
{
    directory_structure_t ds;

    vector<object_trj_t> good_trlet_list;
    {
	std::string name = ds.workspace+"good_trlet_list.xml";
	std::ifstream fin(name.c_str());
	boost::archive::xml_iarchive ia(fin);
	ia >> BOOST_SERIALIZATION_NVP(good_trlet_list);
    }
    //vector<object_trj_t>& trlet_list= good_trlet_list;


    matrix<int> Tff;
    {
	std::string name = ds.workspace+"Tff.txt";
	std::ifstream fin(name.c_str());
	fin>>Tff;
	fin.close();
    }
    //std::cout<<Tff<<std::endl;

    matrix<float> Aff;
    prepare_app_affinity(Tff, good_trlet_list, app_match_thr, Aff);
    {
	std::string name = ds.workspace+"Aff.txt";
	std::ofstream fout(name.c_str());
	fout<<Aff;
	fout.close();
    }

//////////////////////////////////////////////////////////////////////////
    vector<std::vector<std::string> > seq(2);
    read_sequence_list(ds.prefix, seq);
    int T = seq[0].size();
    int Ncam = 2;

    array<std::size_t, 2> img_size = {768, 1024};
    geometric_info_t gi;
    gi.load(ds, img_size);

    parameter_t P;

    //matrix<pmodel_t> pmodel_list;
    vector<array<float, 4> > model;

    load_part_model(model, P.head_wid_ratio, P.head_hi_ratio, P.torso_hi_ratio);

    matrix<float> Ocff;
    matrix<object_trj_t> gap_trlet_list;

    std::cout<<"prepare occl affinity ..."<<std::endl;
    real_timer_t timer;
    prepare_occl_affinity(seq, gi, P, good_trlet_list, model,
			  Tff, Ocff, gap_trlet_list);

    {
	std::string name = ds.workspace+"Ocff.txt";
	std::ofstream fout(name.c_str());
	fout<<Ocff;
	fout.close();
    }
    {
	std::string name = ds.workspace+"gap_trlet_list.xml";
	std::ofstream fout(name.c_str());
	boost::archive::xml_oarchive oa(fout);
	oa << BOOST_SERIALIZATION_NVP(gap_trlet_list);
    }

    std::cout<<"prepare occl affinity time: "<<timer.elapsed()/1000.0f<<std::endl;

    real_timer_t timer2;

    matrix<int> LMat;
    matrix<int> links;

    //matrix<float> Aff2(Aff+Ocff*0.2);
    matrix<float> Aff2(Aff);
    solve_linprog(Tff, Aff2, LMat, links);

    std::cout<<"LP time: "<<timer2.elapsed()/1000.0f<<std::endl;

    std::cout<<"Lv="<<links<<std::endl;
    {
	std::string name = ds.workspace+"links.txt";
	std::ofstream fout(name.c_str());
	fout << links;
	fout.close();
    }


    vector<object_trj_t> final_trj_list;
    vector<vector<int> > final_trj_index;
    matrix<int> final_state_list;

    finalize_trajectory(Ncam, T, links, good_trlet_list, gap_trlet_list,
			final_trj_list, final_trj_index, final_state_list);
    {
	std::string name = ds.workspace+"final_trj_list.xml";
	std::ofstream fout(name.c_str());
	boost::archive::xml_oarchive oa(fout);
	oa << BOOST_SERIALIZATION_NVP(final_trj_list);
    }

    {
	std::string name = ds.workspace+"final_state_list.txt";
	std::ofstream fout(name.c_str());
	fout<<final_state_list;
	fout.close();
    }

    {
	std::string name = ds.workspace+"final_trj_index.txt";
	std::ofstream fout(name.c_str());
	fout << final_trj_index;
	fout.close();
    }


    return 0;

}

#include "lp_impl.hpp"
#include "linprog_app_impl.hpp"
#include "finalize_trajectory_impl.hpp"
