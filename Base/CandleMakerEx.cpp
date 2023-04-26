//
// Created by lnarayan on 2/8/21.
//
#include "CandleMakerEx.h"
CandleMakerEx::CandleMakerEx(int quantum, Product *p) {
    _quantum = quantum;
    _pProduct =  p;
}

int CandleMakerEx::GetFrequency()
{
    return 0;
}

Candle *CandleMakerEx::HandleTrade(Decimal tradePx, long tradeSz, int side) {
    // Maya please implement
    Candle *retval=0;
    if(_current == 0)
    {
        _current = new Candle(gAsofDate);
        _current->SYBL = _pProduct->ExternalKey;
        _current->lastUpdate = gAsofDate;
        _current->interval = time_duration(0,0,0);
        _current->INTF = tradePx;
        /* fix bug to make sure no 0 price candles  */
        _current->LSTB =   tradePx;
        _current->LSTA =   tradePx;
    }
    _current->lastUpdate = gAsofDate;

    _current->LSTP = tradePx;
    _current->LSTS = tradeSz;

    _current->INTV += tradeSz;
    _current->NTRD++;

    if (_current->INTH < tradePx)  // make sure this is initialized to 0
    {
        _current->INTH = tradePx;   // the highest trade PRICE during the interval
        _current->HLTS = tradeSz;
    }

    if (_current->INTL > tradePx)  // make sure this is initialized to Decimal(INT_MAX)
    {
        _current->INTL = tradePx;   // the LOWest trade PRICE during the interval
    }

    Decimal mid = (_current->LSTB + _current->LASK) / Decimal(2.0);
    if (tradePx > mid) {
        _current->IVAM += tradeSz;
    }

    if (tradePx == _current->LSTB)  //paid trade
    {
        Decimal temp = (_current->_paidWeightedPrice*_current->_paidVolume + tradePx*tradeSz);
        _current->_paidVolume += tradeSz;
        if(_current->_paidVolume !=0)
            _current->_paidWeightedPrice =  temp/_current->_paidVolume;
        _current->_paidCount++;
        _current->_paidVolume += tradeSz;
        _lastSide = 1;
        if(side !=  4)
        {
            if(gDebug)
                cout <<  " not paid according to HCTech? possibly trade update arrived before quote: rare"  << endl;
        }
    } else if (tradePx == _current->LSTA)  //given trade
    {
        Decimal temp = (_current->_givenWeightedPrice*_current->_givenVolume + tradePx*tradeSz);
        _current->_givenVolume += tradeSz;
        if(_current->_givenVolume !=0)
            _current->_givenWeightedPrice = temp/_current->_givenVolume;

        _current->_givenCount++;
        _current->_givenVolume += tradeSz;
        _lastSide = 2;

        if(side !=  3)
        {
            if(gDebug)
                cout <<  " not given according to HCTech? possibly trade update arrived before quote: rare"  << endl;
        }
    } else {
        if(gDebug)
            cout << "unable to classify trade pick given/paid from last trade?  side is " << side << endl;

        if (_lastSide == 1 && side == 4)  //paid trade  in agreement with  HCTech
        {
            _current->_paidCount++;
            _current->_paidVolume += tradeSz;
        } else if (_lastSide == 2 && side == 3)  //given trade  in agreement with  HCTech
        {
            _current->_givenCount++;
            _current->_givenVolume +=tradeSz;
        }
        else if (side == 3)  // given trade according HCTech  in DISAGREEMENT with last trade
        {
            _current->_givenCount++;
            _current->_givenVolume +=tradeSz;
        }
        else if (side == 4)  // paid trade according HCTech in DISAGREEMENT with last trade
        {
            _current->_paidCount++;
            _current->_paidVolume += tradeSz;
        }
        else{
            _unclassified++;
            if(gDebug)
                cout <<  "side specified for HCTech trade for unclassified: VERY RARE " <<  getSideString(side) << endl;
        }
    }
    // for paid trades side corresponds to 4 AGGRESS_BID ,  for given 3  AGGRESS_OFFER
    if (side != 3  && side != 4) {
        if(gDebug) {
            cerr << "trade was not classified by HCTech? VERY RARE" << getSideString(side) << endl;
            // Maya try this new design pattern: this should solve the zodiac sync issue
            //cerr << "trade was not classified by HCTech? VERY RARE" << Enum::getSide(side) << endl;
        }
    }



    // time to cut new candle
    if(_current->INTV >= _quantum)
    {
        _candles.push_back(_current);
        retval = _current;
        _current = new Candle(gAsofDate);
        _current->lastUpdate = gAsofDate;
        _current->SYBL = retval->SYBL;
        _current->interval = gAsofDate - _current->timeStamp;
        _current->INTF = tradePx;
        /* this is to cover case when only 1 trade large enough to croses quantum and no quotes yet  */
        _current->LSTB =   tradePx;
        _current->LSTA =   tradePx;
        _current->LBID =   tradePx;
        _current->LASK =   tradePx;
        _current->LSAS =    0;
        _current->LSBS =    0;
        _current->HBID =   tradePx;
        _current->HASK =   tradePx;
    }

    return retval;
}


Candle* CandleMakerEx::HandleQuote(Decimal bid, long bidSize, Decimal offer, long offerSize)
{
    if(_current != NULL) {
        _current->LSTB = bid;
        _current->LSBS = bidSize;
        if (bid < _current->LBID) {
            _current->LBID = bid;
            _current->LBSZ = bidSize;
        }
        if (bid > _current->HBID) {
            _current->HBID = bid;
            _current->HBSZ = bidSize;
        }

        _current->LSTA = offer;
        _current->LSAS = offerSize;

        if (offer < _current->LASK) {
            _current->LASK = offer;
            _current->LASZ = offerSize;
        }
        if (offer > _current->HASK) {
            _current->HASK = offer;
            _current->HASZ = offerSize;
        }
    }
    return NULL;
}

Candle *CandleMakerEx::GetCurrent()
{
    return _current;
}