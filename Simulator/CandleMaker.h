#pragma once
#include "Candle.h"
#include "Product.h"
//#include <boost/log/trivial.hpp>
class ICandleMaker{
public:
    virtual Candle* HandleQuote(Decimal bid, long bidSize, Decimal offer, long offerSize)=0;
    virtual Candle* HandleTrade(Decimal tradePx, long tradeSz, int side)=0;
    virtual Candle* GetCurrent()=0;
    vector<Candle*>& Candles() {
        return _candles;
    };
    static string getSideString(int val) ;
protected:
    vector<Candle*> _candles;
    Candle* _current=0;
    Product *_pProduct=0;
    int _lastSide=0; // last known side 0 NA, 1 PAID, 2 GIVEN
    int _unclassified=0;
};
class CandleMaker : public ICandleMaker{
public:
	enum { m1, m5, m15, h1, h4, d1 };
	Candle* HandleTrade(unsigned char side, Decimal rate, int64_t size);
	Candle* HandleQuotePx(unsigned char side, Decimal rate);
	ptime roundedToMinute(const ptime& time);
	CandleMaker(int freq, Product *p); // in minutes
	vector<Candle*>& Candles();
	CandleMaker() {};
    Candle* GetCurrent() override;
    ofstream* candleFile;
	Candle* HandleQuote(Decimal bid, long bidSize, Decimal offer, long offerSize) override;
    Candle* HandleTrade(Decimal tradePx, long tradeSz, int side) override;



private:
	int _frequency; // in minutes
	Product *_pProduct;
	ptime _lastThreshold;

	string PrintCandle(Candle*);

};