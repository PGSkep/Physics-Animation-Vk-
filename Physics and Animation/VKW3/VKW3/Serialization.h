#include <windows.h>
#include "Scene.h"
#include <iostream>
#include <fstream>
#include <istream>
using namespace std;

void write(const std::string& file_name, Scene& data) // Writes the given Scene data to the given file name.
{
	std::ofstream out;
	out.open(file_name, std::ios::binary);
	out.write(reinterpret_cast<char*>(&data), sizeof(Scene));
	out.close();
};

void read(const std::string& file_name, Scene& data) // Reads the given file and assigns the data to the given Scene.
{
	std::ifstream in;
	in.open(file_name, std::ios::binary);
	in.read(reinterpret_cast<char*>(&data), sizeof(Scene));
	in.close();
};

struct member_details
{
	float	member_length,																				//to store member length
		member_width,																				//to store member width
		member_depth,																				//to store member depth
		member_qk_factored_load,																	//to store unfactored load
		member_gk_factored_load,																	//to store factored load
		member_max_moment,																			//to store moment
		member_shear;																				//to store shear value
	int		member_type,																				//to store member type for quicker search
		member_no;																					//to display record number
	std::string	member_name;																			//to store member name e.g. beam or column
}record[100];

//load member details from file

void member_dimensions_file()
{
	int	i,																						//declare and set int for loop
		no_of_members = 100;																		//declare variable for while loop so that user enters a value less than 15
	string	filePath;																				//declare string for file name
	ifstream  dimensionsInFile;																		//declare input filestream name
																									//cout<<"Enter full path name of file to save to"<
																									//cin>> filePath;																				//user input for file path name
	dimensionsInFile.open("test.dat");
	if (!dimensionsInFile)
	{
		std::cout << "Cannot load file" << endl;
		return;
	}
	else
	{
		for (i = 0; i < no_of_members; i++)																//start of loop
		{	// write struct data from file	
			dimensionsInFile >>
				record[i].member_depth >>
				record[i].member_length >>
				record[i].member_width >>
				record[i].member_qk_factored_load >>
				record[i].member_gk_factored_load >>
				record[i].member_max_moment >>
				record[i].member_shear >>
				record[i].member_type >>
				record[i].member_no;
			std::cout << " Member no " << i << "stored" << endl;
		}
		std::cout << "All members have been successfully loaded" << endl;
		dimensionsInFile.close();
	}
}

void member_details_save()
{
	int	i,																						//declare and set int for loop
		no_of_members = 100;																		//declare variable for while loop so that user enters a value less than 15
	string	filePath;																				//declare string for file name
	ofstream  dimensionsOutfile;																	//declare output filestream name
																									//cout<<"Enter full path name of file to save to"<
																									//cin>> filePath;																					//user input for file path name
	dimensionsOutfile.open("test.dat");
	if (!dimensionsOutfile)
	{
		std::cout << "Cannot load file" << endl;
		return;
	}
	else
	{
		for (i = 0; i < no_of_members; i++)																//start of loop
		{	// write struct data from file	
			dimensionsOutfile <<
				record[i].member_depth <<
				record[i].member_length <<
				record[i].member_width <<
				record[i].member_qk_factored_load <<
				record[i].member_gk_factored_load <<
				record[i].member_max_moment <<
				record[i].member_shear <<
				record[i].member_type <<
				record[i].member_no;
			std::cout << " Member no " << i << "stored" << endl;
		}
		std::cout << "All members have been successfully saved" << endl;
		dimensionsOutfile.close();
	}

}


//template<class T>
//std::ostream &operator<<(std::ostream &output, T const &input) {
//	T::size_type size = input.size();
//
//	output << size << "\n";
//	std::copy(input.begin(), input.end(),
//		std::ostream_iterator<T::value_type>(output, "\n"));
//
//	return output;
//}
//
//template<class T>
//std::istream &operator >> (std::istream &input, T &output) {
//	T::size_type size, i;
//
//	input >> size;
//	output.resize(size);
//	std::copy_n(
//		std::istream_iterator<t::value_type>(input),
//		size,
//		output.begin());
//
//	return input;
//}