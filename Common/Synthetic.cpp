#include "Synthetic.h"
#include "Strategy.h"


Synthetic::Synthetic(string name, int id) : Marketable(id)
{
    _name = name;
}
void Synthetic::Add(Instrument *ins)
{
	_instruments.push_back(ins);
}

void Synthetic::HandleQuote(int symbolid, Decimal bid2, long bidSize2, Decimal offer2, long offerSize2)
{
    (void) symbolid;
	for (auto& i : getInstruments())
	{
		i->HandleQuote(bid2, bidSize2, offer2, offerSize2);
	}
	_candleTracker[1] = false;
}

void Synthetic::CandleUpdate(int id, Candle* cdl, int mins)
{
	/*
	if (_candleTracker.count(mins) > 0)
	{
		if(_candleTracker[mins])
	}
	*/

	// force candles for all other instruments
	map<int, Candle*>  candles;
	for (auto& i : getInstruments())
	{
		if (i->Id == id) {
			candles[id] = cdl;
			continue;
		}
		i->CutCandle(mins);
		candles[i->Id] = (i->GetLastCandle(mins));
	}
	_strategy->HandleCandles(candles);
}

//  appropriate for spread products
void Synthetic::HandleQuotePx(unsigned char side, Decimal rate)
{
	Candle *c1;
	if ((c1 = _instruments[0]->CandleMakers()[0].HandleQuotePx(side, rate)) != NULL)
	{
		//Candle* c2 = _instruments[1]->CandleMakers()[0].GetCurrent();
	}
}

void Synthetic::HandleQuoteSz(unsigned char side, int64_t size)
{
    (void) side;
    (void) size;
}
void Synthetic::HandleTrade(unsigned char side)
{
    (void) side;
}

void Synthetic::HandleTrade(unsigned char side, int64_t size)
{
    (void) side;
    (void) size;
}

void Synthetic::AddStrategy(string freq, NamedParams)
{
    (void) freq;
}