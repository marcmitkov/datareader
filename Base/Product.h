#pragma once
#include<vector>
#include<string>
#include <boost/date_time/posix_time/posix_time.hpp>
//#include <boost/date_time/posix_time/posix_time_io.hpp>
#include "../Base/decimal.h"

using namespace std;

using namespace boost::posix_time;

class Product {

public:

	Product(string exchange, string externalKey);
	Decimal TickSize;
	string Exchange;
	string Key;
	string ExternalKey;

};