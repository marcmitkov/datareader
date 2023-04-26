#pragma once
#include "Marketable.h"
#include "../Base/Product.h"
#include "Instrument.h"

class Synthetic;


class Instrument : public Marketable
{
public:
	Instrument(Product *pm, int id, Marketable* synthetic =0);
	void AddCandleMaker(int freqmin, bool quantum=false);
	void HandleQuotePx(unsigned char side, Decimal rate);
	void HandleQuoteSz(unsigned char side, int64_t size);
	virtual void HandleTrade(unsigned char side); // based on last size message
	virtual void HandleTrade(unsigned char side, int64_t size); // based on volume message
	void HandleCandle(const Candle&);
	virtual ~Instrument() {}

private:
	vector<Strategy *> _strategies;
	Product *_product;
	std::vector<PriceInfo> Bids; // { get; internal set; }
	std::vector<PriceInfo> Offers; //{ get; internal set; }
	
	void WriteCandle(const Candle& rCandle, ofstream& of);

public:
	// enhanced adapter
	map<int, ICandleMaker*>_candleMakersMap;
	virtual void HandleQuote(Decimal bid, long bidSize, Decimal offer, long offerSize) override;
    void HandleTrade(Decimal tradePx, long tradeSz, int side) override;
	Candle* CutCandle(int freqmin);
	Candle* GetLastCandle(int freqmin);
	vector<Candle*>* GetCandles(int freqmin);

};