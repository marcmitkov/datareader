#pragma once
#include "decimal.h"
#include "Utilities.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>

//#include <boost/date_time/posix_time/posix_time_io.hpp>
using namespace boost::posix_time;
using namespace std;
using namespace boost::algorithm;

class Quote
{
public:
	ptime date;
	unsigned char side;
	int size;
	Decimal rate;
};
class Candle
{
public:
	Candle() {}
	Candle(ptime);
	Candle(string line);
	ptime timeStamp;
	ptime lastUpdate;
	Decimal LSTB;		// last bid (currency)
	int LSBS;			//last bid size (unit)
	Decimal LSTA;		//last ask (currency)
	int LSAS;			//last ask size (unit)
	Decimal INTF;		// interval first trade price (currency)
	int INTV=0;			//interval volume i.e. sum(shares)) (unit)
	int IVAM=0;			//interval volume over mid quote (bid+ask)/2 (unit)

	time_duration interval;
	string SYBL;
	Decimal LSTP;		//last price (currency)
	int LSTS;			//last size (unit)
	Decimal INTH;		//interval high trade (currency)
	Decimal INTL;		//interval low trade (currency)
	int NTRD=0;			// interval number trades (unit)
	Decimal HBID;       //interval high bid (currency)
	Decimal	LBID;		//interval low  bid (currency)
	Decimal HASK;		//interval high ask (currency)
	Decimal LASK;		//interval low ask (currency)
	int HBSZ;			//interval high bid size(unit)
	int LBSZ;			//interval low bid  size(unit)
	int HASZ;			//interval high ask size(unit)
	int LASZ;			//interval low ask size(unit)
	int HLTS;			//interval high trade size (unit)
	// TODO:  why not have Interval Low Trade side
	string ToString() const;
    string ToStringEx() const;
    static string GetHeader();
    static string GetHeaderEx();
    int _givenCount=0;
    long _givenVolume=0;
    int _paidCount=0;
    long _paidVolume=0;
    Decimal _givenWeightedPrice;
    Decimal _paidWeightedPrice;
protected:

};
