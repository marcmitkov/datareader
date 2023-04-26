#ifndef NamedParamsReader_H_
#define NamedParamsReader_H_

#include <fstream>
#include <iostream>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include<vector>
#include"../Base/NamedParams.h"

using namespace std;
class NamedParamsReader {
public:

	static map<string, NamedParams>* ReadNamedParams(string paramsPath, bool translate=false);
};

#endif