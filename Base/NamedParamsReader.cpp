#include "NamedParamsReader.h"
#include "Utilities.h"


extern std::string GetCurrentWorkingDir(void);
map<string, NamedParams>* NamedParamsReader::ReadNamedParams(string paramsPath, bool translate)
{
	auto NamedParamsMap = new map<string, NamedParams>();

	try {
		NamedParamsVector npVector;
		std::ifstream file(paramsPath + "params.xml");
		if (paramsPath == "") {
			cout << "reading params from " << GetCurrentWorkingDir() << "\\params.xml" << endl;
		}
		else {
			cout << "reading params from " << paramsPath << "params.xml" << endl;
		}

		boost::archive::xml_iarchive ia(file);
		ia >> BOOST_SERIALIZATION_NVP(npVector);
		for (int i = 0; i < npVector.m_npVector.size(); i++)
		{
			NamedParams obj = npVector.m_npVector[i];
			if (obj.Key.find(":") != string::npos)
			{
				cout << ": in key is not a valid format for xml.  may create problems later" << endl;
			}
			string key = obj.Key + "_" + obj.Frequency;
			(*NamedParamsMap)[key] = obj;
		}
		//file.close();
		return NamedParamsMap;
	}
	catch (exception &e)
	{
		cout << "while reading named params " << e.what() << endl;
		return NamedParamsMap;
	}
}