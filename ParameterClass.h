#ifndef ParameterClass_Included
#define ParameterClass_Included

#include <array>
#include <map>
#include <iostream>


using namespace std;

class ParameterClass{
//=============================================================================
	json_t* read_file_config(){
		int dim;
		size_t flags = 0;
		json_error_t error;
// 		if (rank==0){cout << "-----------------------------------------------------"<<endl;}
		
		//--- loading the JSON file ------------------------------------------
		json_t* file_json = json_load_file("./config.json", flags, &error);
		if(!file_json){
			cerr << "in line " << error.line << ": " << error.text << endl;
			exit(1);
		}
		//--------------------------------------------------------------------
		
		//--- returning the config object ------------------------------------  
		json_t* config_json = json_object_get(file_json, "config");
		if(!config_json){
			cerr << "in line " << error.line << ": " << error.text << endl;
			exit(1);
		}
		//--------------------------------------------------------------------
		return config_json; 
	}
//=============================================================================

//=============================================================================
	int read_integer (int index, const char * name){
		json_t* obj_json = json_array_get(config_json, index);
		if (!obj_json) cout<<"can't catch"<<index<<" member of config="<<name <<endl;
		obj_json = json_object_get(obj_json, name);
		if (!obj_json) cout<<"can't catch "<<name<< " object, name and index does not match" <<endl;
		return json_integer_value(obj_json);
	}
//=============================================================================

//=============================================================================
	double read_double (int index, const char * name){
		json_t* obj_json = json_array_get(config_json, index);
		if (!obj_json) cout<<"can't catch"<<index<<" member of config="<<name <<endl;
		obj_json = json_object_get(obj_json, name);
		if (!obj_json) cout<<"can't catch "<<name<< " object, name and index does not match" <<endl;
		return json_real_value(obj_json);		
	}
//=============================================================================

//=============================================================================
	std::array<double,2> read_orbital_energy (){
		json_t* obj_json = json_array_get(config_json, 2);
		if (!obj_json) cout<<"can't catch 3 member of config" <<endl;
		json_t* obj_json_arr = json_object_get(obj_json, "orbital_energy");
		if (!obj_json) cout<<"can't catch orbital_energy object" <<endl;
		double zero = json_real_value(json_array_get(obj_json_arr, 0));
		double first = json_real_value(json_array_get(obj_json_arr, 1));
		std::array<double,2> arr={zero,first};
		return arr;
	}
//=============================================================================

//=============================================================================
	std::map<double,double> read_ttmo(){
		std::map<double,double> ttmo;
		json_t* obj_json = json_array_get(config_json, 5);
		if (!obj_json) cout<<"can't catch 5 member of config" <<endl;
		const json_t * json_arr = json_object_get(obj_json, "ttmo");
		if (!json_arr) cout<<"can't catch ttmo object" <<endl;
		size_t size = json_array_size(json_arr);
		for (size_t i=0; i<size; i +=2){
			double key   = json_real_value(json_array_get(json_arr, i  ));
			double value = json_real_value(json_array_get(json_arr, i+1));
			ttmo.insert ( std::pair<double,double>(key,value) );
		}

		return ttmo;
	}
//=============================================================================


//=============================================================================
public:
	int dim; //we have two spatial basis functions in STO-3G
	int Nelec; // we have 2 electrons in HeH+
	json_t* config_json;
	std::array<double,2> orbital_energy; // molecular orbital energies
	double ENUC; // nuclear repulsion energy for HeH+ -- constant
	double EN   ; // SCF energy
	std::map<double,double> ttmo ; //  dictionary containing two-electron repulsion integrals

//=============================================================================
	ParameterClass(){ // constructor
		config_json 	= read_file_config();
		dim 			= read_integer(0,"dim");  
		Nelec 			= read_integer(1,"Nelec");
		orbital_energy 	= read_orbital_energy();
		ENUC			= read_double(3,"ENUC");
		EN				= read_double(4,"EN");
		ttmo			= read_ttmo();
	}
//=============================================================================

//========================================================================
	~ParameterClass(void) { //destructor
		delete config_json;
	}
//========================================================================
};


#endif //ParameterClass_Included
