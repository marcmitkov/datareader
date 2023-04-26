#include "CandleMaker.h"
#include "Utilities.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <commonv2/Enum.h>

using namespace boost::posix_time;
using namespace boost::gregorian;

// #define _GLIBCXX_USE_CXX11_ABI 1
CandleMaker::CandleMaker(int freq, Product *p) {
    _frequency = freq;
    _pProduct = p;
}

int CandleMaker::GetFrequency()
{
    return _frequency;
}


string CandleMaker::PrintCandle(Candle *cdl) {
    (void) cdl;
    std::string outcandle;
    /* commenting to fix following link error
     /opt/rh/devtoolset-6/root/usr/include/c++/6.3.1/bits/basic_string.h:5472:
     undefined reference to `std::string __gnu_cxx::__to_xstring<std::string, char>(int (*)(char*, unsigned long, char const*, __va_list_tag*), unsigned long, char const*, ...)'
     */
    outcandle = to_simple_string(gAsofDate);
    outcandle += ",";
    outcandle += _pProduct->ExternalKey;
    outcandle += ",";
    outcandle += to_string(_frequency);
    outcandle += ",";
    outcandle += dec::toString(_current->LSTB);
    outcandle += ",";
    outcandle += dec::toString(_current->LSTA);
    outcandle += ",";
    outcandle += dec::toString(_current->INTF);
    outcandle += ",";
    outcandle += to_string(_frequency);
    outcandle += ",";
    outcandle += dec::toString(_current->INTF);
    outcandle += ",";
    outcandle += dec::toString(_current->INTH);
    outcandle += ",";
    outcandle += dec::toString(_current->INTL);
    outcandle += ",";
    outcandle += dec::toString(_current->LSTP);
    string s = to_simple_string(_current->timeStamp);
    outcandle += ",current ts rounded:" + s;
    s = to_simple_string(gAsofDate - _lastThreshold);
    outcandle += ",duration since last threshold:" + s;
    return outcandle;
}


/*  Maya setup an enum for this
 *     namespace SIDE {
		enum SIDE : uint8_t{
			BID = 0,
			OFFER = 1,
			TRADE = 2,
			AGGRESS_BID = 3,
			AGGRESS_OFFER = 4,
			AGGRESS_BID_GTC = 5,  // Only used in REUTERS
			AGGRESS_OFFER_GTC = 6, // Only used in REUTERS
			FX_CANCEL = 7,
			CLEAR = 8,
			UNDEFINED = UINT8_MAX
		};
 */

string ICandleMaker:: getSideString(int val)
{
    const char* retval ="UNDEFINED";
    switch (val)
    {
        case 0:
            retval = "BID";
            break;
        case 1:
            retval = "OFFER";
            break;
        case 2:
            retval = "TRADE";
            break;
        case 3:
            retval = "AGGRESS_BID";
            break;
        case 4:
            retval = "AGGRESS_OFFER";
            break;
        case 5:
            retval = "AGGRESS_BID_GTC";
            break;
        case 6:
            retval = "AGGRESS_OFFER_GTC";
            break;
        case 7:
            retval = "FX_CANCEL";
            break;
        case 8:
            retval = "CLEAR";
            break;
        default:
            retval = "UNDEFINED";
            break;
    }

    return retval;
}

Candle *CandleMaker::HandleTrade(Decimal tradePx, long tradeSz, int side) {
    Candle *retval = NULL;
    if (gAsofDate.time_of_day().total_seconds() == 0) {
        cout << "CandleMaker::HandleTrade: zero time ignored" << endl;
        return retval;
    }
    if (_lastThreshold.is_not_a_date_time() /*_lastThreshold  not set */) {
        int freq = GetFrequency(); // non zero for regular candles
        _lastThreshold = roundedToMinute(gAsofDate, freq);
        _current = new Candle(_lastThreshold);
        _current->interval = time_duration(0, _frequency, 0);
        _current->SYBL = _pProduct->ExternalKey;
        _current->INTF = tradePx;
        _current->lastUpdate = gAsofDate;
    } else if (gAsofDate - _lastThreshold > time_duration(0, _frequency, 0)) {
        //cout << "Cutting candle at: " << to_simple_string(gAsofDate) << endl;
        //ofglobal << "Cutting candle at: " << to_simple_string(gAsofDate) << " _lastThreshold" << to_simple_string(_lastThreshold)<<" Diff "<< to_simple_string(gAsofDate - _lastThreshold)<< endl;
        _candles.push_back(_current);
        //cout << PrintCandle(_current) << endl;
        retval = _current;
        _lastThreshold = roundedToMinute(gAsofDate,  GetFrequency());
        _current = new Candle(_lastThreshold);
        _current->lastUpdate = gAsofDate;
        _current->SYBL = retval->SYBL;
        _current->interval = retval->interval;
        _current->INTF = tradePx;

        // at the minimum set the last bid / ask from the previous candle
        _current->LSTB = retval->LSTB;
        _current->LSTA = retval->LSTA;
    }


    _current->INTV += tradeSz;
    _current->NTRD++;


    _current->LSTP = tradePx;
    _current->LSTS = tradeSz;

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

    return retval;
}

Candle *CandleMaker::HandleQuote(Decimal bid, long bidSize, Decimal offer, long offerSize) {
    Candle *retval = NULL;

     if ( gAsofDate.is_not_a_date_time()) {
        cout << "CandleMaker::HandleQuote:not a date time" << endl;
        return retval;
    }

    if (_lastThreshold.is_not_a_date_time() /*_lastThreshold  not set */) {
        //BOOST_LOG_TRIVIAL(info) << "New Candle for interval ";
        _lastThreshold = roundedToMinute(gAsofDate, GetFrequency());
        _current = new Candle(_lastThreshold);
        _current->interval = time_duration(0, _frequency, 0);
        _current->SYBL = _pProduct->ExternalKey;
    } else if (gAsofDate - _lastThreshold > time_duration(0, _frequency, 0)) {
        //cout << "Cutting candle at: " << to_simple_string(gAsofDate) << endl;
        //ofglobal << "Cutting candle at: " << to_simple_string(gAsofDate) << " _lastThreshold" << to_simple_string(_lastThreshold)<<" Diff "<< to_simple_string(gAsofDate - _lastThreshold)<< endl;
        _candles.push_back(_current);
        //cout << PrintCandle(_current) << endl;
        retval = _current;
        _lastThreshold = roundedToMinute(gAsofDate,GetFrequency());
        _current = new Candle(_lastThreshold);
        _current->lastUpdate = gAsofDate;
        _current->SYBL = retval->SYBL;
        _current->interval = retval->interval;
    }

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

    return retval;
}

Candle *CandleMaker::GetCurrent()  // this vesion updates threshold
{
    _candles.push_back(_current);
    //cout << PrintCandle(_current) << endl;
    Candle *retval = _current;
    _lastThreshold = roundedToMinute(gAsofDate,GetFrequency());

    //initialize next candle
    _current = new Candle(_lastThreshold);
    _current->lastUpdate = gAsofDate;
    _current->SYBL = retval->SYBL;
    _current->interval = retval->interval;

    _current->LSTB = retval->LSTB;
    _current->LSBS = retval->LSBS;
    _current->LBID = retval->LSTB;
    _current->HBID = retval->LSTB;


    _current->LSTA = retval->LSTA;
    _current->LSAS = retval->LSAS;
    _current->LASK = retval->LSTA;
    _current->HASK = retval->LSTA;

    return retval;
}


Candle *CandleMaker::HandleTrade(unsigned char side, Decimal rate, int64_t size) {
    Candle *retval = NULL;
    /*
    if (gAsofDate.time_of_day().total_seconds() == 0) {
        cout << "zero time ignored" << endl;
        return retval;
    }
    */

    if ( gAsofDate.is_not_a_date_time()) {
        cout << "CandleMaker::HandleTrade:not a date time" << endl;
        return retval;
    }

    if (_lastThreshold.is_not_a_date_time() /*_lastThreshold  not set */) {
        _lastThreshold = roundedToMinute(gAsofDate, GetFrequency());
        _current = new Candle();
        _current->INTF = rate;   // open
        _current->interval = time_duration(0, _frequency, 0);
    } else if (gAsofDate - _lastThreshold > time_duration(0, _frequency, 0)) {
        //BOOST_LOG_TRIVIAL(info) << "Cutting candle at " << to_simple_string(gAsofDate) << " _lastThreshold" << to_simple_string(_lastThreshold) << " Diff " << to_simple_string(gAsofDate - _lastThreshold);

        _candles.push_back(_current);
        retval = _current;
        _lastThreshold = roundedToMinute(gAsofDate,GetFrequency());

        _current = new Candle();
        *_current = *retval;  // to line up candles due to bid/ ask sent in different quotes consequtively?

    }
    _current->lastUpdate = gAsofDate;
    _current->timeStamp = roundedToMinute(gAsofDate,GetFrequency());
    _current->SYBL = _pProduct->ExternalKey;
    _current->LSTP = rate;
    _current->LSTS = size;
    _current->INTV += size;

    if (_current->INTL == Decimal(0.0))
        _current->INTL = rate;

    if (rate < _current->INTL)
        _current->INTL = rate;

    if (rate > _current->INTH)
        _current->INTH = rate;

    _current->LSTP = rate;

    if (side == 0x0) //paid
    {
        _current->INTV += size;
        _current->IVAM += size;
        _current->NTRD++;
    } else if (side == 0x1)  //given
    {
        _current->INTV += size;
        _current->NTRD++;
    } else
        cout << "unknown side " << endl;
    return retval;
}

Candle *CandleMaker::HandleQuotePx(unsigned char side, Decimal rate) {
    Candle *retval = NULL;
    // cout << "CandleMaker ts: " << to_simple_string(ts)  << " for product " << product << endl;


    if (gAsofDate.time_of_day().total_seconds() == 0) {
        cout << "zero time ignored" << endl;
        return retval;
    }
    if (_lastThreshold.is_not_a_date_time() /*_lastThreshold  not set */) {
        //BOOST_LOG_TRIVIAL(info) << "New Candle for interval ";

        _lastThreshold = roundedToMinute(gAsofDate, GetFrequency());
        _current = new Candle();
        _current->INTF = rate;   // open
        _current->interval = time_duration(0, _frequency, 0);
    } else if (gAsofDate - _lastThreshold > time_duration(0, _frequency, 0)) {
        //cout << "Cutting candle at: " << to_simple_string(gAsofDate) << endl;
        //ofglobal << "Cutting candle at: " << to_simple_string(gAsofDate) << " _lastThreshold" << to_simple_string(_lastThreshold)<<" Diff "<< to_simple_string(gAsofDate - _lastThreshold)<< endl;
        _candles.push_back(_current);
        retval = _current;
        _lastThreshold = roundedToMinute(gAsofDate, GetFrequency());
        _current = new Candle();  // TODO: delete this
        *_current = *retval;
    }
    _current->lastUpdate = gAsofDate;
    _current->timeStamp = roundedToMinute(gAsofDate,GetFrequency());
    _current->SYBL = _pProduct->ExternalKey;
    if (side == 0x0) //bid
    {
        _current->LSTB = rate;
        if (_current->LBID == Decimal(0.0))
            _current->LBID = rate;
        if (rate < _current->LBID)
            _current->LBID = rate;
        if (rate > _current->HBID)
            _current->HBID = rate;
    } else {
        _current->LSTA = rate;
        if (_current->LASK == Decimal(0.0))
            _current->LASK = rate;
        if (rate < _current->LASK)
            _current->LASK = rate;
        if (rate > _current->HASK)
            _current->HASK = rate;
    }

    return retval;
}


ptime CandleMaker::roundedToMinute(const ptime &time, int freq)  // floor
{
    // Get time-of-day portion of the time, ignoring fractional seconds
    time_duration tod = seconds(time.time_of_day().total_seconds());
    //cout << "roundedToMinute tod: " << to_simple_string(tod) << endl;

    //
    int rounddown =  tod.minutes();
    if(freq > 0)
        rounddown = ((int)(rounddown/freq))*freq;
    time_duration roundedDownTod(tod.hours(), rounddown, 0, 0);
    // Round time-of-day down to start of the hour
    // time_duration roundedDownTod(tod.hours(), tod.minutes(), 0, 0);

    //cout << "roundedToMinute roundedDownTod: " << to_simple_string(roundedDownTod) << endl;

    // Construct the result with the same date, but with the rounded-down
    // time-of-day.
    ptime result(time.date(), roundedDownTod);

    return result;
}

