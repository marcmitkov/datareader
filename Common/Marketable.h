#pragma once
#include<vector>
#include<string>
#include <boost/date_time/posix_time/posix_time.hpp>
//#include <boost/date_time/posix_time/posix_time_io.hpp>
#include "../Base/decimal.h"
#include "../Base/NamedParams.h"
#include "../Base/CandleMaker.h"
#include "../Base/PriceInfo.h"
using namespace std;
using namespace boost::posix_time;
class Strategy;
class Marketable
{
public:
    Marketable(int id) {Id=id;}
	virtual bool  IsSynthetic() { return false; };
	void SetStrategy(Strategy* s) { _strategy = s; };
	Decimal _bid;
	Decimal _ask;
	void SetSessionStart(ptime t) {
		_sessionStart = t;
	}
	void SetSessionEnd(ptime t) {
		_sessionEnd = t;
	}
	map<string, NamedParams> _namedParams;
	string Name() { return _name; }
	virtual void HandleQuotePx(unsigned char side, Decimal rate)=0;
	virtual void HandleQuoteSz(unsigned char side, int64_t size)=0;
	virtual void HandleTrade(unsigned char side) = 0;
	virtual void HandleTrade(unsigned char side, int64_t size) = 0;
	PriceInfo BestBid;
	PriceInfo BestAsk;
	Decimal LastTradePx;
	int64_t LastTradeSz;
	int64_t	Volume;
	vector<CandleMaker>&  CandleMakers() { return _candleMakers; }
	//HCTech
	virtual void CandleUpdate(int id, Candle*, int);

    //virtual void HandleQuote(Decimal bid, long bidSize, Decimal offer, long offerSize) {}
    virtual void HandleQuote(Decimal , long , Decimal , long ) {}

    virtual void HandleTrade(Decimal tradePx, long tradeSz, int side)=0;
    int Id;
    bool IsFX() { return _isFX;}
    bool _isFX=false;
protected:
	ptime _sessionStart;
	ptime _sessionEnd;
	vector<CandleMaker> _candleMakers;
	virtual bool InSession(ptime ts);
	string _name;
	Marketable* _parent;
	Strategy* _strategy;

};