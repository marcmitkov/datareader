#pragma once
//#include <inttypes.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include<map>

//#include "./CandleMaker.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include <boost/math/tools/precision.hpp>

#include "../Base/Quote.h"
#include "../Base/Macros.h"
#include "../Base/CandleMaker.h"
#include "../Trading/Product.h"
#include "../Base/MacroTrader.h"

/*
#ifndef _NOMODEL
#include "../Common/MacroTraderEx.h"
using namespace Common;
#endif
*/

using namespace std;

using namespace boost::posix_time;

namespace Anon {

	class Simulator
	{
		//void MakeQuote(int64_t ticks, unsigned char side, Decimal rate, int64_t size, int factor, Quote&);
		//void MakeTrade(int64_t ticks, unsigned char side, Decimal rate, int64_t size, int factor, Quote&);

		CandleMaker _candleMaker;
		vector<string> _externalKeyVector;
		static void PriceBookUpdated(string, MarketDepthChangeEventArgs* e);
		void RaiseQuote(Product * p, Quote q, bool tickData = true);
		void RaiseTrade(Product * p, Quote q, bool tickData = true);

	public:
		Simulator();
		int ReadData();
		void ProcessMultipleTick();
		//bool eof(vector<ifstream*>);
		void ProcessMultipleCandles();
		void ValidateTimestamp(string ts, int numFields);

		void MakeQuote(int64_t ticks, unsigned char side, Decimal rate, int64_t size, int factor, Quote&);
		void MakeTrade(int64_t ticks, unsigned char side, Decimal rate, int64_t size, int factor, Quote&);

	};
}
