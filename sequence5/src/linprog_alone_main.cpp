#include <boost/mpi.hpp>

#define USE_MPI 1

#include "tracking_data_package.hpp"
#include "tracking_detail.hpp"
#include "misc_utils.hpp"

#include <boost/math/constants/constants.hpp>

#include "lp.hpp"
#include "linprog_app.hpp"
#include "linprog_alone.hpp"

#include "planning.hpp"

#include "finalize_trajectory.hpp"

float app_match_thr = -2;

#include "planning.hpp"

int main(int argc, char* argv[])
{

    mpi::environment env(argc, argv);
    mpi::communicator world;
    std::cout << "I am process " << world.rank() << " of " << world.size()
	      << "." << std::endl;

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
	std::string name = ds.workspace+"Aff_alone.txt";
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

    vector<array<float, 4> > model;

    load_part_model(model,
		    P.head_wid_ratio, P.head_hi_ratio, P.torso_hi_ratio);

    //matrix<float> Ocff;
    //matrix<object_trj_t> gap_trlet_list;


    //prepare_obstacles();
    matrix<matrix<float> > cars;
    load_carboxes(ds, seq, cars);

    matrix<vector<matrix<float> > > car_obsz;
    prepare_car_obs(cars, gi.img2grd, car_obsz);

    vector<vector<matrix<float> > > car_obs;
    combine_car_obs(car_obsz, car_obs);

    matrix<matrix<float> > ped_obs;
    prepare_ped_obs(good_trlet_list, T, ped_obs);


    //plan_all();

    real_timer_t timer;
    float plff_thr = 6.0f;


    matrix<float> Alnff;
    matrix<object_trj_t> gap_trlet_list;
    matrix<int> gap_rind;
    matrix<matrix<int> > gap_paths;
    matrix<vector<matrix<int> > > reduced_paths;


    prepare_alone_affinity(world, seq, gi, P, 0, plff_thr,
			  good_trlet_list,  model, Tff,
			  Alnff, gap_trlet_list,
			  gap_rind, gap_paths, ds);




    if(0!=world.rank()) return 0;

    {
	std::string name = ds.workspace+"gap_trlet_list_alone.xml";
	std::ofstream fout(name.c_str());
	boost::archive::xml_oarchive oa(fout);
	oa << BOOST_SERIALIZATION_NVP(gap_trlet_list);
    }
    {
	std::string name = ds.workspace+"Alnff.txt";
	std::ofstream fout(name.c_str());
	fout<<Alnff;
	fout.close();
    }

    std::cout<<"prepare alone affinity time: "<<timer.elapsed()/1000.0f<<std::endl;

    real_timer_t timer2;

    matrix<int> LMat;
    matrix<int> links;

    matrix<float> Aff2(Aff);

    using namespace boost::lambda;
    solve_linprog(Tff, Aff2+Alnff*0.2, LMat, links);

    std::cout<<"LP time: "<<timer2.elapsed()/1000.0f<<std::endl;

    std::cout<<"Lv="<<links<<std::endl;
    {
	std::string name = ds.workspace+"links_alone.txt";
	std::ofstream fout(name.c_str());
	fout << links;
	fout.close();
    }


    vector<object_trj_t> final_trj_list;
    vector<vector<int> > final_trj_index;
    matrix<int> final_state_list;

    finalize_trajectory(Ncam, T, links, good_trlet_list, gap_trlet_list,
			final_trj_list, final_trj_index, final_state_list);

    //finalize_trajectory(Ncam, T, links, good_trlet_list, gap_trlet_list,
//			final_trj_list, final_trj_index, final_state_list);
    {
	std::string name = ds.workspace+"final_trj_list_alone.xml";
	std::ofstream fout(name.c_str());
	boost::archive::xml_oarchive oa(fout);
	oa << BOOST_SERIALIZATION_NVP(final_trj_list);
    }
    {
	std::string name = ds.workspace+"final_state_list_alone.txt";
	std::ofstream fout(name.c_str());
	fout<<final_state_list;
	fout.close();
    }
    {
	std::string name = ds.workspace+"final_trj_index_alone.txt";
	std::ofstream fout(name.c_str());
	fout << final_trj_index;
	fout.close();
    }

    return 0;

}

#include "linprog_app_impl.hpp"
#include "linprog_alone_impl.hpp"
#include "finalize_trajectory_impl.hpp"
#include "planning_impl.hpp"
#include "lp_impl.hpp"
