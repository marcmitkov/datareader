/*
 * Priceinfo.h
 *
 */

#ifndef PRICEINFO_H_
#define PRICEINFO_H_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include "decimal.h"
using namespace boost::posix_time;


	
class PriceInfo{
	public:

	Decimal Price;
	int Quantity;
	int Count;
	ptime Timestamp;

	PriceInfo();

	PriceInfo(Decimal , int , int );
	
};


#endif /* PRICEINFO_H_ */
