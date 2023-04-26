#pragma once
#include "Marketable.h"
#include "Instrument.h"
class Synthetic : public Marketable
{
public:
	Synthetic(string name, int id=0);
	void Add(Instrument *);
	virtual void HandleQuotePx(unsigned char side, Decimal rate);
	virtual void HandleQuoteSz(unsigned char side, int64_t size);
	virtual void HandleTrade(unsigned char side);
	virtual void HandleTrade(unsigned char side, int64_t size);
	void AddStrategy(string freq, NamedParams);
	virtual bool  IsSynthetic() { return true; };
	// enhanced adapter
    virtual void HandleQuote(int symbolid, Decimal bid, long bidSize, Decimal offer, long offerSize);
    void HandleTrade(Decimal tradePx, long tradeSz, int side) override { (void) tradePx;(void) tradeSz;(void) side;};
    vector<Instrument*>& getInstruments() { return _instruments;}
	void  CandleUpdate(int id, Candle*, int mins);
protected:
	vector<Instrument*> _instruments;
	map<int, bool> _candleTracker;
};