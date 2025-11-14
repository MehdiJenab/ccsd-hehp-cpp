#ifndef ParameterClass_Included
#define ParameterClass_Included



class ParameterClass{
public:
	int dim = 2; //we have two spatial basis functions in STO-3G
	int Nelec = 2; // we have 2 electrons in HeH+
	const std::array<double,2> orbital_energy = {-1.52378656, -0.26763148}; // molecular orbital energies
//  dictionary containing two-electron repulsion integrals

	const double ENUC = 1.1386276671; // nuclear repulsion energy for HeH+ -- constant
	const double EN   = -3.99300007772; // SCF energy
	
	const std::map<double,double> ttmo = {
	{ 5.0,  0.94542695583037617}, 
	{12.0,  0.17535895381500544}, 
	{14.0,  0.12682234020148653}, 
	{17.0,  0.59855327701641903}, 
	{19.0, -0.056821143621433257}, 
	{20.0,  0.74715464784363106}
};


};


#endif //ParameterClass_Included
