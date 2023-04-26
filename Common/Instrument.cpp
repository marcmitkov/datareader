#include "Instrument.h"
#include "../Base/Utilities.h"
#include "Strategy.h"
#include "../Base/CandleMakerEx.h"
extern Strategy*  CreateStrategy(Marketable * c, string s, string name, string group, int freq);
Instrument::Instrument(Product *p, int id, Marketable *synthetic) : Marketable(id)
{
	_product = p;
	_name = p->Key;  // this is used to look up the params
	_parent = synthetic;

}

void Instrument::AddCandleMaker(int freqminORquantum, bool quantum)
{
    ICandleMaker *cm1=0;
    if(!quantum)
        cm1 = new CandleMaker(freqminORquantum, _product);
    else
        cm1 = new CandleMakerEx(freqminORquantum, _product);

    _candleMakersMap[freqminORquantum]= cm1;
}

void Instrument::HandleTrade(Decimal tradePx, long tradeSz, int side)
{
    for (auto& cm : _candleMakersMap)
    {
        Candle* cdl = cm.second->HandleTrade(tradePx, tradeSz, side);
        if (cdl != NULL)
        {
            if (_parent != 0)
                _parent->CandleUpdate(Id, cdl, cm.first);
            else {
                _strategy->HandleCandle(*cdl);
            }
        }
    }
}


void Instrument::HandleQuote(Decimal bid, long bidSize, Decimal offer, long offerSize)
{
	for (auto& cm : _candleMakersMap)
	{
		Candle* cdl = cm.second->HandleQuote(bid, bidSize, offer, offerSize);
		if (cdl != NULL)
		{
			if (_parent != 0)
				_parent->CandleUpdate(Id, cdl, cm.first);
			else {
                _strategy->HandleCandle(*cdl);
            }
		}
	}
}

Candle *Instrument::CutCandle(int freqmin)
{
	Candle* retval=NULL;
	if (_candleMakersMap.count(freqmin) > 0)
	{
		auto cm = _candleMakersMap[freqmin];
		retval = cm->GetCurrent();
	}
	return retval;
}

Candle* Instrument::GetLastCandle(int freqmin)
{
	Candle* retval = NULL;
	if (_candleMakersMap.count(freqmin) > 0)
	{
		auto cm = _candleMakersMap[freqmin];
		int cnt = cm->Candles().size();
		if(cnt > 1)
			retval = cm->Candles()[cnt-2];
	}
	return retval;
}

vector<Candle*>* Instrument::GetCandles(int freqminORquantum)
{
    vector<Candle*>* retval = 0;
	if (_candleMakersMap.count(freqminORquantum) > 0)
	{
		auto cm = _candleMakersMap[freqminORquantum];
		retval =  &cm->Candles();
	}
	else{
	    cout << "candlemaker not found for frequency OR quantum " << freqminORquantum << endl;
	}
	return retval;
}




//  0:	livesim/live
//  1:	backtest reading candles
//  2:	backtest reading ticks
//  3:  generate candles from live date
/*
void Instrument::AddStrategy(string freq, NamedParams np)
{
	_namedParams[freq] = np;

	if (gMode != 3)
 		_strategies.push_back(CreateStrategy(this, "A", _product->ExternalKey, "Momentum", stoi(freq)));

	if (gMode != 1)
	{
		if (freq == "1m")
		{
			CandleMaker *cm1 = new CandleMaker(1, _product);
			_candleMakers.push_back(*cm1);
		}
		else if (freq == "5m") {
			CandleMaker *cm5 = new CandleMaker(5, _product);
			_candleMakers.push_back(*cm5);
		}
		else if (freq == "15m") {
			CandleMaker *cm15 = new CandleMaker(15, _product);
			_candleMakers.push_back(*cm15);
		}
		else if (freq == "1H") {
			CandleMaker *cm60 = new CandleMaker(60, _product);
			_candleMakers.push_back(*cm60);
		}
		else if (freq == "4H") {
			CandleMaker *cm240 = new CandleMaker(240, _product);
			_candleMakers.push_back(*cm240);
		}
		else if (freq == "1D") {
			// daily candles will probably need to be generated EOD
			CandleMaker *cm1440 = new CandleMaker(1440, _product);
			_candleMakers.push_back(*cm1440);
		}
	}
}
*/
void Instrument::HandleQuotePx(unsigned char side, Decimal rate)
{
	switch (side)
	{
	case 0x0:
		BestBid.Price = Decimal(rate);
		break;
	case 0x1:
		BestAsk.Price = Decimal(rate);
		break;
	default:
		break;
	}

	for (size_t j = 0; j < _candleMakers.size(); j++)
	{
		Candle * c;
		if ((c = _candleMakers[j].HandleQuotePx(side, rate)) != NULL)
		{
			WriteCandle(*c, *(_candleMakers[j].candleFile));
			HandleCandle(*c);
		}
	}

}


void Instrument::HandleCandle(const Candle& rCandle)
{
	for (vector<Strategy *>::iterator it = _strategies.begin(); it != _strategies.end(); ++it)
	{
	
		time_t t = rCandle.interval.total_seconds() / 60;
		if (gDebug)
		{
			cout << "Candle Interval:" << to_simple_string(rCandle.interval) << endl;
			cout << "Candle Interval in secs:" << rCandle.interval.total_seconds() << endl;
			cout << "time_t: " << t << "Strategy Freq: " << (*it)->Frequency() << endl;
		}
	}
}

void Instrument::HandleQuoteSz(unsigned char side, int64_t size)
{
	switch (side)
	{
	case 0x0:
		BestBid.Quantity = size;
		break;
	case 0x1:
		BestAsk.Quantity = size;
		break;
	default:
		break;
	}
}


void Instrument::HandleTrade(unsigned char side)
{
	for (size_t j = 0; j <  _candleMakers.size(); j++)
	{
		Candle * c;
		if ((c = _candleMakers[j].HandleTrade(side, LastTradePx, LastTradeSz)) != NULL)
		{
			HandleCandle(*c);
		}
	}
}

//MR NewCmake

void  Instrument::HandleTrade(unsigned char side, int64_t size)
{
	for (size_t j = 0; j < _candleMakers.size(); j++)
	{
		Candle * c;
		if ((c = _candleMakers[j].HandleTrade(side, LastTradePx, size)) != NULL)
		{
			HandleCandle(*c);
		}
	}
}

void Instrument::WriteCandle(const Candle& rCandle, ofstream& of)
{
	of << rCandle.timeStamp << "," << to_simple_string(rCandle.timeStamp.time_of_day()) << "," << rCandle.SYBL
		<< "," << rCandle.LSTP << "," << rCandle.LSTS << "," << rCandle.INTF << "," << rCandle.INTH
		<< "," << rCandle.INTL << "," << rCandle.INTV << "," << rCandle.NTRD << "," << rCandle.LSTB
		<< "," << rCandle.LSBS << "," << rCandle.LSTA << "," << rCandle.LSAS << "," << rCandle.HBID
		<< "," << rCandle.LBID << "," << rCandle.HASK << "," << rCandle.LASK << "," << rCandle.HBSZ
		<< "," << rCandle.LBSZ << "," << rCandle.HASZ << "," << rCandle.LASZ << "," << rCandle.HLTS
		<< "," << rCandle.IVAM << endl;

	
}


