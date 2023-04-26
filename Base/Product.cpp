#include "Product.h"
Product::Product(string exchange, string key)
{
	//cout << "Product Construct" << endl;
	Exchange = exchange;
	Key = key; // TYU8
	ExternalKey = exchange + "_" + key;  // example 230:SSIU8 to 1010:TYU8

	//bool isfx = key.size() == 7 || key.find('/') != std::string::npos;


	// external symbol matches up with strategy name in Instrument.cpp
	/*
	ExternalSymbol = ExternalKey;
	std::replace(ExternalSymbol.begin(), ExternalSymbol.end(), ':', '_');
	std::replace(ExternalSymbol.begin(), ExternalSymbol.end(), '/', '_');
	*/
	//if(!isfx) // currently assume US futures
	//ExternalSymbol = ExternalSymbol.substr(0, 2);
}

