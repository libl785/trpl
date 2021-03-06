#ifndef __TEXT__FILE__HPP__INCLUDED
#define __TEXT__FILE__HPP__INCLUDED

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/function.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/serialization.hpp>

#include <fstream>
#include "cvpr_stub.hpp"
#include "cvpr_array_traits.hpp"
#include <map>

BEGIN_NAMESPACE_CVPR

template <class T>
void save_text(T const& data, std::string const & filename) {
    std::ofstream ofs(filename.c_str());
    boost::archive::text_oarchive oa(ofs);
    oa << data;
}

template <class T>
void load_text(T & data, std::string const & filename) {
    std::ifstream ifs(filename.c_str());
    boost::archive::text_iarchive ia(ifs);
    ia >> data;
}

template <class T>
void save_xml(T const& data, std::string const & filename, std::string const& tag_name) {
    std::ofstream ofs(filename.c_str());
    boost::archive::xml_oarchive oa(ofs);
    oa << boost::serialization::make_nvp(tag_name.c_str(), data);

}

template <class T>
void load_xml(T& data, std::string const & filename, std::string const& tag_name) {
    std::ifstream ifs(filename.c_str());
    boost::archive::xml_iarchive ia(ifs);
    ia >> boost::serialization::make_nvp(tag_name.c_str(), data);
}

bool read_string_list(const std::string& file,
		      std::vector<std::string>& string_list)
{
    std::ifstream fin(file.c_str());
	
    if(!fin.is_open()) return false;
	
    for(int jj=0; jj<200000; ++jj)
    {
	char line_buf[1024];
	if(fin.eof())	break;
	fin.getline(line_buf, 1024);
	if(line_buf[0]==0)	continue;
	string_list.push_back(line_buf);
    }
    fin.close();
    return true;
}

bool read_keyvalue_pairs(std::string const& fname, 
		       std::map<std::string, std::string>& settings) 
{
    std::vector<std::string> string_list;
    if(!read_string_list(fname, string_list)) return false;

    for(int ii=0; ii<string_list.size(); ++ii)
    {
	std::string& s= string_list[ii];
	boost::trim(s);

	if(s.size()==0) continue;

	std::vector<std::string> kvpair;

	boost::split(kvpair, s, boost::is_any_of("="));

	//std::cout<<kvpair.size()<<std::endl;
	if(kvpair.size()<2) {
	    //settings[boost::trim(key)] = trim(val);
	}
	else {
	    boost::trim(kvpair[0]);
	    boost::trim(kvpair[1]);
	    settings.insert( 
		std::make_pair(kvpair[0],kvpair[1] ) );
	}
    };
    return true;      
}


template <class Mat>
bool read_text_array1d(const std::string& file, Mat& mat)
{
    using namespace boost::lambda;
    typedef array1d_traits<Mat> tr;
    typedef typename tr::value_type T;

    std::vector<std::string> str_list;
    if(!read_string_list(file, str_list)) return false;


    typedef boost::char_separator<char> csept;
    typedef boost::tokenizer<csept >        tokenizer;
    csept sep(", \t");


    std::vector<std::vector<std::string> > all_syms;
    std::size_t s=0;
    for(std::size_t ii=0; ii<str_list.size(); ++ii)
    {
	tokenizer	tokens(str_list[ii], sep);

	std::vector<std::string> syms;
	for (tokenizer::iterator tok_iter = tokens.begin();
	     tok_iter != tokens.end(); ++tok_iter)
	{
	    syms.push_back(*tok_iter);
	    //std::cout << "<" << *tok_iter << "> ";
	}
	if(syms.size()>=1)
	{
	    all_syms.push_back(syms);
	    s+=syms.size();
	}
    }
    if(all_syms.size()>0)
    {
	tr::change_size(mat, s);
	std::size_t kk=0;
	for(int ii=0; ii<all_syms.size(); ++ii)
	{
	    for(int jj=0; jj<all_syms[ii].size(); ++jj)
	    {
		if('\r'== *(all_syms[ii][jj].rbegin()))
		{
		    all_syms[ii][jj] = all_syms[ii][jj].substr(0, all_syms[ii][jj].size()-1);
		}
		tr::ref(mat, kk) = boost::lexical_cast<T>(all_syms[ii][jj]);
		kk++;
	    }
	}
    }

    return true;
}

template <class Mat>
bool read_text_array2d(const std::string& file, Mat& mat)
{

    typedef array2d_traits<Mat> tr;
    typedef typename tr::value_type T;

    std::vector<std::string> str_list;
    if(!read_string_list(file, str_list)) return false;


    typedef boost::char_separator<char> csept;
    typedef boost::tokenizer<csept >        tokenizer;
    csept sep(", \t");


    std::vector<std::vector<std::string> > all_syms;
    for(std::size_t ii=0; ii<str_list.size(); ++ii)
    {
	tokenizer	tokens(str_list[ii], sep);

	std::vector<std::string> syms;
	for (tokenizer::iterator tok_iter = tokens.begin();
	     tok_iter != tokens.end(); ++tok_iter)
	{
	    syms.push_back(*tok_iter);
	    //std::cout << "<" << *tok_iter << "> ";
	}
	if(syms.size()>=1)
	{
	    all_syms.push_back(syms);
	}
    }
    if(all_syms.size()>0)
    {
	//mat = Mat(all_syms.size(), all_syms[0].size());
	tr::change_size(mat, all_syms.size(), all_syms[0].size());
	for(int ii=0; ii<tr::size1(mat); ++ii)
	{
	    for(int jj=0; jj<tr::size2(mat) && jj<all_syms[ii].size(); ++jj)
	    {
		if('\r'== *(all_syms[ii][jj].rbegin()))
		{
		    all_syms[ii][jj] = all_syms[ii][jj].substr(0, all_syms[ii][jj].size()-1);
		}
		tr::ref(mat, ii, jj) = boost::lexical_cast<T>(all_syms[ii][jj]);
	    }
	    for(int jj=all_syms[ii].size(); jj<tr::size2(mat); ++jj)
	    {
		tr::ref(mat, ii, jj) = 0;
	    }
	}
    }

    return true;
}



//for compatibility name
template <class Mat>
bool read_text_matrix(std::string const& file, Mat& mat)
{
    return read_text_array2d(file, mat);
}


template <class Mat>
bool write_matlab_matrix(const std::string& fname, const std::string& varname,
			 const Mat& mat)
{
    typedef array2d_traits<Mat> tr;
    std::ofstream fout(fname.c_str());
    fout<<varname<<"=[";
    for(std::size_t ii=0; ii<tr::size1(mat); ++ii)
    {
	for(std::size_t jj=0; jj<tr::size2(mat); ++jj)
	{
	    fout<<tr::ref(mat, ii, jj)<<"\t";
	}
	if(ii<tr::size1(mat)-1)
	    fout<<std::endl;

    }
    fout<<"];";
    fout.close();
}

template <class Mat>
bool write_matlab_vector(const std::string& fname, const std::string& varname,
			 const Mat& mat)
{
    typedef array1d_traits<Mat> tr;
    std::ofstream fout(fname.c_str());
    fout<<varname<<"=[";
    for(std::size_t ii=0; ii<tr::size(mat); ++ii)
    {
	fout<<tr::ref(mat, ii)<<"\t";
    }
    fout<<"];";
    fout.close();
}


END_NAMESPACE_CVPR

#endif
