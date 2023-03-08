#include "Trader.h"
#include <commonv2/modeltypes.h>
#include "Instrument.h"
#include "../Base/Utilities.h"
#include "../Base/NamedParamsReader.h"
#include "Synthetic.h"
#include "../Base/IAdapterIn.h"
#include "../Base/Position.h"
#include <algorithm>
//#include "../commonv2/AdapterIn.h"
#include "StrategyA.h"
#include "../Base/Utilities.h"

#if TWS
#include "../TwsSocketClient/EDecoder.h" //TICK_PRICE codes
#include "../TwsSocketClient/EWrapper.h" //BID, ASK codes
#endif
// HCTech:  active version
extern map <string, string> _strategyMap;
extern map<hcteam::book_feed_type_t, long> bookFeedMap;

Trader::Trader(IAdapterIn *adapter) {
    cout << "Chanel: creating trader" << endl;
    m_pAdapter = adapter;
#if TWS
    m_params = *NamedParamsReader::ReadNamedParams("./");
#endif
}

void Trader::PositionUpdate(uint64_t id,  double pos)
{
    if (_hcMap.count(id) > 0) {
        SymbolInfo si = _hcMap[id];
        cout << "got position update for "  << si.m_ekey << ","  << si.m_exchange << ","  << si.m_symbol  << "," << to_string(pos) << endl;
    }
    /*
    if (_router.count(id) > 0) {
        auto v = _router[id];
        for (auto it = v.begin(); it != v.end(); ++it) {
            Strategy *strat = *it;
            //  strat-> PositonUpdate()  maybe implement in the future
        }
    }
    */
    string symbol =  GetSymbol(id);
    _positionMap[symbol]=pos;
}


#if ( __HC_EVLIB__ )
void onTraderTimerCallback( int, void * data )
{
    const std::lock_guard<std::mutex> lock(gPositionsMutex);
    auto d= static_cast<CBData *>(data);
    cout << "sending order using timer callback "  << d->od->ToString()  << " @ " << to_simple_string(gAsofDate) << endl;
    uint64_t orderid = d->trader->SendOrder(d->od, d->strat);
    cout <<  to_string(orderid)  << " timer order sent successfully."  << endl;
    delete d;
    cout <<  "deleted cbdata"  << endl;
}
#endif

uint64_t Trader::SendOrderEx(OrderDetails *od, Strategy *strat) {
    const std::lock_guard<std::mutex> lock(gPositionsMutex);
    uint64_t orderid = 0;
#if (__HC_EVLIB__)
    const bool recurTimer = false; // Timer happens multiple times on the interval?
    //const int timerDelayIntervalMicros = 1 * 60000000; // 1 minute in microseconds, millisecond accuracy.
    const int timerDelayIntervalMicros = 1 * 30000; // 30 millisecond accuracy.
    auto cbdata = new CBData();
    cbdata->od=od;
    cbdata->strat=strat;
    cbdata->trader=this;
    cout << "will send order "  << od->ToString()  <<  " 30 ms later " << " @ " << to_simple_string(gAsofDate) << endl;
    m_pAdapter->registerCallback(&onTraderTimerCallback, cbdata, timerDelayIntervalMicros, false);
#endif
    return orderid;
}

uint64_t Trader::SendOrder(OrderDetails *od, Strategy *strat) {
    //const std::lock_guard<std::mutex> lock(gPositionsMutex);

    if (_hcMap.count(od->symbolId) > 0 && gDebug) {
        cout << " sending order for " << _hcMap[od->symbolId].m_symbol << endl;
        cout << od->ToString() << endl;  // Maya pls ensure this is correct
    }

    uint64_t orderid = 0;
    if (od->tif == HC::FIX_TIME_IN_FORCE::IOC) {
        cout << "sending ioc" << endl;
        orderid = m_pAdapter->SendIOC(*od);
    } else {
        orderid = m_pAdapter->SendOrder(*od);
    }
    if (orderid > 0) {
        _orderMap[orderid] = strat;
        if (strat != NULL)
            strat->PutOrder(orderid, od);
    }
    return orderid;
}

void Trader::EOD() {
    cout << "start of EOD process " << _router.size() << endl;
    for (auto &item : _router) {
        cout << "getting strategy for  " << item.first << endl;
        auto v = _router[item.first];
        for (auto it = v.begin(); it != v.end(); ++it) {
            Strategy *strat = *it;

            if (_maxExposureMap.count(item.first) > 0) {
                string s = strat->ComputeStats(_maxExposureMap[item.first]);
                cout << "stats" << endl;
                cout << s << endl;
            }
            /*
            if(_ofsSummary.is_open())
            {
                _ofsSummary << to_simple_string(gAsofDate) << "," <<  to_string(gParamNumber) << "," <<  s << endl;
            }
             */

        }
    }
    cout << "end of EOD process" << endl;
    for (auto &d: crossCountMap) {
        //cout << "Crossed spread stats for " << d.first  << "," << d.second.numCrossed << "," << d.second.meanSpread << "," <<  d.second.maxSpread << "," << to_simple_string(d.second.first)  << "," << to_simple_string(d.second.last)<< endl;

        cout << "Crossed spread stats for " << d.first << "," << d.second.numCrossed << "," << d.second.meanSpread
             << "," << d.second.maxSpread << "," << to_simple_string(d.second.first);
        cout << "," << to_simple_string(d.second.last) << ",(" << d.second.numTooWide << "),"
             << to_simple_string(d.second.firstWide) << ",";
        cout << to_simple_string(d.second.lastWide) << endl;
        //cout << "Wide spread stats for " << d.first  << "," << d.second.numTooWide << ","<< to_simple_string(d.second.firstWide)  << "," << to_simple_string(d.second.lastWide)<< endl;


    }

    for (auto &e: bookFeedMap) {
        cout << hcteam::book_feed_types[e.first] << "=> " << e.second << endl;
    }


}

void Trader::Expire() {
    cout << "start of Expire process; # for symbols is " << _router.size() << endl;
    for (auto &item : _router) {
        cout << "getting strategy for  " << item.first << endl;
        auto v = _router[item.first];
        cout << "# of strategies is   " << v.size() << endl;
        for (auto it = v.begin(); it != v.end(); ++it) {
            Strategy *strat = *it;
            try {
                strat->Expire(true);
            }
            catch (const char *msg) {
                cerr << msg << endl;
            }
            strat->ResetPositionsCounter();
        }
    }
    if (false) {
        CancelAll();
    }
    cout << "end of Expire process" << endl;
}


string Trader::Summary() {
    string retval;
    //retval = "work in progress";
    for (auto &strategies: _orderMap) {
        retval += strategies.first;
        retval += string(",name ") + strategies.second->Name() + "\n";
        for (auto &orderdetails : strategies.second->GetOrderMap()) {
            retval += string(",") + orderdetails.second->ToString() + "\n";
        }
    }
    return retval;
}


void Trader::CancelAll() {
    m_pAdapter->CancelAll();
}

int Trader::CancelOrder(uint64_t orderid) {
    cout << "cancelling " << orderid << endl;
    int retval = m_pAdapter->CancelOrder(orderid);
    return retval;
}


vector<OrderDetails *> Trader::GetOrderDetailsList()  // push only openpending positions for now
{

    if (_ofsOrders.is_open())
        _ofsOrders << "Sending orders to cancel:" << endl;

    vector < OrderDetails * > retval;
    return retval;
}

void Trader::HandleQuote(int symbolid, Decimal bid, long bidSize, Decimal offer, long offerSize) {

    if (_router.count(symbolid) > 0) {
        auto v = _router[symbolid];
        for (auto it = v.begin(); it != v.end(); ++it) {
            Strategy *strat = *it;
            strat->HandleQuote(bid, bidSize, offer, offerSize);
        }
    }
}

/*
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
void Trader::HandleTicker(int symbolid, Decimal tradePx, long tradeSz, int side) {
    if (_router.count(symbolid) > 0) {
        auto v = _router[symbolid];
        for (auto it = v.begin(); it != v.end(); ++it) {
            Strategy *strat = *it;
            if (_hcMap.count(symbolid) > 0) {

                SymbolInfo si = _hcMap[symbolid];


            }
            strat->HandleTicker(tradePx, tradeSz, side);
        }
    }
}


void Trader::HandleQuoteCrossAsset(int symbolid, Decimal bid, long bidSize, Decimal offer, long offerSize) {
    //pending implement when futures issues fixed
    (void) symbolid;
    (void) bid;
    (void) bidSize;
    (void) offer;
    (void) offerSize;

}

void Trader::HandleTickerCrossAsset(int symbolid, Decimal tradePx, long tradeSz, int side) {
    //pending implement when futures issues fixed
    (void) symbolid;
    (void) tradePx;
    (void) tradeSz;
    (void) side;

}

extern map <string, map<string, string>> stratAparamMap;

// HCTech:  at this stage _hcMap should be populated with descriptive data about subscriptions
void Trader::InitProducts() {

    string outdir = gOutputDirectory + "/";

    struct stat info;
    if (stat(outdir.c_str(), &info) != 0) {
        printf("cannot access %s\n", outdir.c_str());
    } else {
        string yyyymmdd = PosixTimeToStringFormat(gAsofDate, "%Y%m%d");
        string fn = outdir + "Orders-" + PosixTimeToStringFormat(gAsofDate, "%Y%m%d") + ".txt";

        if (!gBacktest)
            _ofsOrders.open(fn, std::ofstream::out);

    }

    for (auto &s : _strategyMap) {
        if (s.first.substr(0, 3) == "VAR") {  // case of Vector Auto Regressive
            vector <string> varSymbols;  // such as VAR1 => ESZ9,NQZ9,EUR/USD
            boost::char_separator<char> sep{",", "", boost::keep_empty_tokens};
            auto f = s.second;
            if (!f.empty()) {
                boost::tokenizer<boost::char_separator<char>> tok(f, sep);
                for (const auto &t : tok)
                    cout << t << endl;
                varSymbols.assign(tok.begin(), tok.end());
            }
            vector <string> exchanges;
            vector <string> symbols;
            vector<int> ids;
            for (auto &e : _hcMap) {
                auto it = find(varSymbols.begin(), varSymbols.end(), e.second.m_symbol);
                if (it != varSymbols.end()) {
                    cout << "adding symbol:" << e.second.m_symbol << " for id " << e.first << endl;
                    exchanges.push_back(e.second.m_exchange);
                    symbols.push_back(e.second.m_symbol);
                    ids.push_back(e.first);
                }
            }
            AddProducts(s.first, ids, symbols, exchanges);
        } else
            for (auto &e : _hcMap) {
                if (e.second.m_symbol == s.first)  //create strategies for this instrument and frequencies
                {
                    vector <string> frequencies;  // EUR/USD_Freq = > 1, 5
                    boost::char_separator<char> sep{",", "", boost::keep_empty_tokens};

                    auto f = s.second;
                    boost::tokenizer<boost::char_separator<char>> tok(f, sep);
                    for (const auto &t : tok)
                        cout << t << endl;

                    if (!f.empty()) {
                        frequencies.assign(tok.begin(), tok.end());
                        for (auto &freq : frequencies) {
                            Product *prod = new Product(e.second.m_exchange, e.second.m_symbol);
                            Instrument *ins = new Instrument(prod, e.first);

                            if (e.second.m_symbol.find("/") != string::npos) {
                                ins->_isFX = true;
                            }
                            auto strat = new StrategyA(ins, e.second.m_symbol, "StratA", stoi(freq));
                            if (_maxExposureMap.count(e.first) == 0) {
                                _maxExposureMap[e.first] = 0;
                                _currentExposureMap[e.first] = 0;
                            }

                            if (strat->IsInitialized()) {
                                vector < Strategy * > v;
                                if (_router.count(e.first) > 0) {
                                    v = _router[e.first];
                                    v.push_back(strat);
                                } else {
                                    v.push_back(strat);
                                }
                                _router[e.first] = v;
                            } else {
                                delete prod;

                                delete ins;
                                delete strat;
                            }
                        }
                    }
                }
            }
    }
}

int Trader::GetCurrentExposure(int id)
{
    int retval=0;
    if(_currentExposureMap.count(id) > 0)
        retval=_currentExposureMap[id];
    else
        cerr << "current exposure not avaialble for "  <<  GetSymbol(id)  << endl;
    return retval;
}

void Trader::AddProduct(int symbolid, SymbolInfo si) {
    ofglobal << "Chanel: AddProduct:" << symbolid << "," << si.m_symbol << endl;
    cout << "Chanel: AddProduct:" << symbolid << "," << si.m_symbol << endl;
    _hcMap[symbolid] = si;
}

int Trader::GetSymbolId(string esh1) {
    int retval = -1;
    for (auto &si : _hcMap) {
        if (si.second.m_symbol == esh1) {
            retval = si.first;
            break;
        }
    }
    return retval;
}

string Trader::GetSymbol(int id) {
    string retval="NA";
    if(_hcMap.count(id) >0)
    {
        retval =  _hcMap[id].m_symbol;
    }
    return retval;
}

void Trader::Mute(int symbolid, int frequency) {
    cout << "Searching in router " << _router.size() << endl;

    if (_router.count(symbolid) > 0) {
        cout << "mute: getting strategy for  " << symbolid << endl;
        auto v = _router[symbolid];
        for (auto it = v.begin(); it != v.end(); ++it) {
            Strategy *strat = *it;
            if (strat != 0) {
                cout << "disabling signal for " << strat->Name() << "  " << strat->FrequencyString() << endl;
                strat->DisableSignal();
            }
        }
    } else {
        cout << "mute:  symbolid  not found " << symbolid << endl;
    }

}

void Trader::AddProducts(string name, vector<int> ids, vector <string> p, vector <string> exchange) {
    return;  // pending initiate VAR strategy
    auto synthetic = new Synthetic(name);
    vector < Product * > products;

    //UN Debugging
    vector < Marketable * > v = _map[exchange[0] + "_" + p[0]];
    //

    for (size_t i = 0; i < p.size(); i++) {
        Product *prod = new Product(exchange[i], p[i]);
        products.push_back(prod);
        auto ins = new Instrument(prod, ids[i], synthetic);  // for now using 1 min VAR
        ins->AddCandleMaker(1);
        synthetic->Add(ins);
    }

    // auto strat = new Strategy(synthetic, name, "VAR", 1);
    Strategy *strat = 0;
    for (auto &i : ids) {
        //_router[i] =
        vector < Strategy * > v1;
        if (_router.count(i) > 0) {
            v1 = _router[i];
            v1.push_back(strat);
        } else {
            v1.push_back(strat);
        }
        _router[i] = v1;
    }
}

// end
void Trader::HandleQuotePx(int symbolid, unsigned char side, Decimal rate) {
    //Write tick data to file
    if (gWriteTicks) {
        ptime now = second_clock::local_time();
        string t = PosixTimeToStringFormat(now, "%Y-%m-%d %H:%M:%S");
        /*
        if(side == 0x0)
            *(m_pAdapter->getSymbolTable()[symbolid].mp_tickfile) <<t<< ","<< TICK_PRICE << "," << BID << "," << rate << endl;
        else
            *(m_pAdapter->getSymbolTable()[symbolid].mp_tickfile) << t << "," << TICK_PRICE << "," << ASK << "," << rate << endl;
         */
    }
    string sym = m_pAdapter->getSymbolTable()[symbolid].m_symbol;
    string exchange = m_pAdapter->getSymbolTable()[symbolid].m_exchange;
    //ptime ticks = gAsofDate;

    auto ekey = m_pAdapter->getSymbolTable()[symbolid].m_ekey;
    if (_map.count(ekey) > 0) {
        auto v = _map[ekey];
        for (auto it = v.begin(); it != v.end(); ++it) {
            Marketable *ins = *it;
            //cout << "sending price to " << ins->Name() << endl;
            ins->HandleQuotePx(side, rate);

        }
    }
}

void Trader::HandleVolume(int symbolid, int64_t vol) {
    /*if (gWriteTicks) {
        ptime now = second_clock::local_time();
        string t = PosixTimeToStringFormat(now, "%Y%m%d %H:%M:%S");
        *(m_pAdapter->getSymbolTable()[symbolid].mp_tickfile) <<t<<","<< TICK_SIZE << "," << VOLUME << "," << vol << endl;
    }*/
    auto ekey = m_pAdapter->getSymbolTable()[symbolid].m_ekey;
    if (_map.count(ekey) > 0) {
        auto v = _map[ekey];
        for (auto it = v.begin(); it != v.end(); ++it) {
            Marketable *ins = *it;
            //cout << "Trade Size: "  << vol - ins->Volume << endl;

            unsigned char side;
            if (ins->LastTradePx == ins->BestBid.Price)
                side = 0x0; // passive BUY / Active SELL
            else if (ins->LastTradePx == ins->BestAsk.Price)
                side = 0x1; // passive SELL / Active BUY
            else
                side = 0x2;

            if (ins->Volume > 0)  // first message should be skipped
                ins->HandleTrade(side, vol - ins->Volume);
            ins->Volume = vol;
        }
    }
}

void Trader::HandleQuoteSz(int symbolid, unsigned char side, int64_t size) {
    (void) side;
    (void) size;
    if (gWriteTicks) {
        ptime now = second_clock::local_time();
        string t = PosixTimeToStringFormat(now, "%Y-%m-%d %H:%M:%S");
#if TWS
        if (side == 0x0)
            *(m_pAdapter->getSymbolTable()[symbolid].mp_tickfile) << t << "," << TICK_SIZE << "," << BID_SIZE << "," << size << endl;
        else
            *(m_pAdapter->getSymbolTable()[symbolid].mp_tickfile) << t << "," << TICK_SIZE << "," << ASK_SIZE << "," << size << endl;
#endif
    }

    string sym = m_pAdapter->getSymbolTable()[symbolid].m_symbol;
    string exchange = m_pAdapter->getSymbolTable()[symbolid].m_exchange;
    //ptime ticks = gAsofDate;
    //HandleQuote( sym , exchange, ptime ticks, unsigned char side, Decimal rate, int64_t size, int factor)


}

void Trader::HandleTradePx(int symbolid, Decimal rate) {
    if (gWriteTicks) {
        ptime now = second_clock::local_time();
        string t = PosixTimeToStringFormat(now, "%Y-%m-%d %H:%M:%S");
#if TWS
        * (m_pAdapter->getSymbolTable()[symbolid].mp_tickfile) << t << "," << TICK_PRICE << "," << LAST << "," << rate << endl;
#endif
    }
    string sym = m_pAdapter->getSymbolTable()[symbolid].m_symbol;
    string exchange = m_pAdapter->getSymbolTable()[symbolid].m_exchange;
    //ptime ticks = gAsofDate;

    auto ekey = m_pAdapter->getSymbolTable()[symbolid].m_ekey;
    if (_map.count(ekey) > 0) {
        auto v = _map[ekey];
        for (auto it = v.begin(); it != v.end(); ++it) {
            Marketable *ins = *it;
            ins->LastTradePx = rate;
            ins->LastTradeSz = 0;  // wait for size
        }
    }
}

void Trader::HandleTradeSz(int symbolid, int64_t size) {
    if (gWriteTicks) {
        ptime now = second_clock::local_time();
        string t = PosixTimeToStringFormat(now, "%Y-%m-%d %H:%M:%S");
#if TWS
        * (m_pAdapter->getSymbolTable()[symbolid].mp_tickfile) << t << "," << TICK_SIZE << "," << LAST_SIZE << "," << size << endl;
#endif
    }
    string sym = m_pAdapter->getSymbolTable()[symbolid].m_symbol;
    string exchange = m_pAdapter->getSymbolTable()[symbolid].m_exchange;
    //ptime ticks = gAsofDate;

    auto ekey = m_pAdapter->getSymbolTable()[symbolid].m_ekey;
    if (_map.count(ekey) > 0) {
        auto v = _map[ekey];
        for (auto it = v.begin(); it != v.end(); ++it) {
            Marketable *ins = *it;
            ins->LastTradeSz = size;

            unsigned char side;
            if (ins->LastTradePx == ins->BestBid.Price)
                side = 0x0; // passive BUY / Active SELL
            else if (ins->LastTradePx == ins->BestAsk.Price)
                side = 0x1; // passive SELL / Active BUY
            else
                side = 0x2;

            ins->HandleTrade(side);
        }
    }
}

#include <commonv2/Enum.h>
#include <framework/BaseOrder.h>
//extern  const std::string OrderStateToString(const int order_status);
void Trader::HandleOrderEvent(int insid, uint64_t orderid, int eventtype) {
    //const std::lock_guard<std::mutex> lock(gPositionsMutex);
    if (gCancelKludge && gBacktest) {
        return;
    }

    if (_ofsOrders.is_open()) {
        _ofsOrders << "====> " <<   GetSymbol(insid) << "  orderevent for inst received @ " << to_simple_string(gAsofDate) << endl;;
        _ofsOrders <<  "insid " << insid << ",orderid " << orderid << ",eventtype "
                      << Enum::getOrderStatus(eventtype) << endl;
    }

    Strategy *strat = GetStrategy(orderid);
    if (strat != 0) {
        strat->GetLog() << "====> orderevent for inst received @ " << to_simple_string(gAsofDate) << endl;
        strat->GetLog() << "insid " << insid << ",orderid " << orderid << ",eventtype "
                        << Enum::getOrderStatus(eventtype) << endl;
    }

    cout << CurrentThreadId("HandleOrderEvent") << endl;

    switch (eventtype) {
        case 2:                    // Order was sent, by OrderManager or FPGA
            cout << orderid << "-ADD_PENDING" << endl;
            break;
        case 4:                       // Order was reject by the ECN.
            cout << orderid << "-REJECTED @" << to_simple_string(gAsofDate) << endl;
            break;
        case 8:
            cout << orderid << "-ACCEPTED @" << to_simple_string(gAsofDate) << endl;
            break;
        case 4096:
            cout << orderid << "-CANCELLED @" << to_simple_string(gAsofDate) << endl;
            break;
        default:
            cerr << orderid << "-EVENTTYPE: " << OrderStateToString((ORDER_STATUS)eventtype) << " @" <<  to_simple_string(gAsofDate) << endl;
    }
    OrderDetails *od = GetOrder(orderid);
    if (od != NULL && od->symbolId == insid) {
        od->status = eventtype;
        if (od->positions.size() != 0) {
            ofstream &log = strat == 0 ? _ofsOrders : strat->GetLog();
            if (log.is_open()) {
                log << "event for position " << od->positions[0]->Id << "  @ " << to_simple_string(gAsofDate)
                    << endl;
            }
        }
    } else if (od != NULL) {
        cout << "odd mismatch of insid" << od->symbolId << "==" << insid << endl;
    } else {
        cerr << "order not found " << orderid << endl;
    }
}

void Trader::HandleFillEvent(int insid, uint64_t orderid, int action, int filled, int totalfilled, int remaining,
                             double price) {
    //const std::lock_guard<std::mutex> lock(gPositionsMutex);

    if (filled > 100000000 || filled < -100000000) {
        cerr << "BAD fill for order " << orderid << ",inst " << insid << ",px " << price << " @ "
             << to_simple_string(gAsofDate);
    }
    if (_maxExposureMap.count(insid) > 0 && _currentExposureMap.count(insid) > 0) {
        int exposure = _currentExposureMap[insid];
        if (action == 0) {

            exposure += filled;

        } else {

            exposure -= filled;
        }
        _currentExposureMap[insid] = exposure;
        if (abs(exposure) > abs(_maxExposureMap[insid]))
            _maxExposureMap[insid] = exposure;

    }
    if (_ofsOrders.is_open()) {
        _ofsOrders << "====> " <<   GetSymbol(insid) << " fill received @" << to_simple_string(gAsofDate) << endl;
        _ofsOrders << (action == 0 ? "BUY" : "SELL") << ",";
        _ofsOrders << insid << ",orderid ";
        _ofsOrders << orderid << ",filled ";
        _ofsOrders << filled << ",remaining ";
        _ofsOrders << remaining << ",price ";
        _ofsOrders << price << endl;
    }
    cout << CurrentThreadId("HandleFillEvent") << endl;
    /*
    if(orderid == 9223372036854775846)
    {
        cout << " fill?"  << endl;
    }
    */
    OrderDetails *od = GetOrder(orderid);
    Strategy *strat = GetStrategy(orderid);
    if (strat != 0) {
        strat->GetLog() << "====> fill received @" << to_simple_string(gAsofDate) << " ";
        strat->GetLog() << (action == 0 ? "BUY" : "SELL") << ",";
        strat->GetLog() << insid << ",";
        strat->GetLog() << orderid << ",";
        strat->GetLog() << filled << ",";
        strat->GetLog() << remaining << ",";
        strat->GetLog() << price << " ";
    }

    if (od != NULL && od->symbolId == insid) {
        if (od->positions.size() != od->fills.size()) {
            cerr << "issue with position/fill mapping" << endl;
            if (strat != 0)
                strat->GetLog() << "issue with position/fill mapping" << endl;
        }
        int unallocated = filled;
        for (size_t j = 0; j < od->positions.size(); j++) {

            if (od->positions[j] != NULL && unallocated > 0) {

                strat->NotifyExposure(od->positions[j], action, filled);

                if (_ofsOrders.is_open())
                    _ofsOrders << "for position " << od->positions[j]->Id << "  @ " << to_simple_string(gAsofDate)
                               << " ";
                if (strat != 0) {
                    strat->GetLog() << "for position " << od->positions[j]->Id << "  @ " << to_simple_string(gAsofDate)
                                    << " ";
                }

                if (od->positions[j]->mState ==
                    Position::OpenPending /*sgn<int>(od->position->Size) == sgn<int>(filled)*/)  //initiating position
                {
                    if (sgn(action - 1) == sgn(od->positions[j]->Size)) {
                        cerr << " there may be a problem action is " << (action == 1 ? "SELL" : "BUY") << " Size is "
                             << od->positions[j]->Size << endl;
                    }

                    if (_ofsOrders.is_open())
                        _ofsOrders << "opening position size " << od->positions[j]->Size << " ";

                    if (strat != 0)
                        strat->GetLog() << "opening position size " << od->positions[j]->Size << " ";
                    od->positions[j]->FillTimeStamp = gAsofDate;
                    Fill aFill;
                    aFill.TimeStamp = gAsofDate;
                    aFill.Price = price;
                    aFill.FillSize = filled;

                    od->positions[j]->Fills.push_back(aFill);
                    od->positions[j]->Remaining =
                            od->positions[j]->Size > 0 ? od->positions[j]->ComputeRemaining() - filled :
                            -od->positions[j]->ComputeRemaining() + filled;

                    if (filled >= abs(od->fills[j])) {
                        unallocated -= abs(od->fills[j]);
                        od->fills[j] = 0;
                    } else {
                        od->fills[j] = od->fills[j] > 0 ? od->fills[j] - filled : od->fills[j] + filled;
                        unallocated = 0;
                    }

                    if (od->positions[j]->ComputeRemaining() == 0) {  // fills come as abs amounts
                        double weightedpx = 0.0;
                        for (auto &f : od->positions[j]->Fills) {
                            weightedpx += f.FillSize * f.Price;
                        }
                        od->positions[j]->FillPx = weightedpx / totalfilled;
                        od->positions[j]->mState = Position::Open;
                        od->positions[j]->CloseRemaining = abs(od->positions[j]->Size);
                        if (_ofsOrders.is_open())
                            _ofsOrders << "final fill for " << od->orderId << ", amount: " << od->positions[j]->Size
                                       << endl;
                        if (strat != 0)
                            strat->GetLog() << "final fill for " << od->positions[j]->Size << endl;

                    } else {  // partial fill
                        double weightedpx = 0.0;
                        int partialamt = 0;
                        for (auto &f : od->positions[j]->Fills) {
                            weightedpx += f.FillSize * f.Price;
                            partialamt += f.FillSize;
                        }
                        od->positions[j]->FillPx = weightedpx / partialamt;
                        od->positions[j]->mState = Position::OpenPending;
                        od->positions[j]->CloseRemaining = abs(
                                od->positions[j]->Size - od->positions[j]->ComputeRemaining());
                        if (_ofsOrders.is_open())
                            _ofsOrders << "partial fill for " << od->orderId << ", amount: " << partialamt << endl;

                        if (strat != 0)
                            strat->GetLog() << "partial fill for " << partialamt << endl;

                    }
                } else if (od->positions[j]->mState ==
                           Position::ClosedPending)  // closing position
                {
                    if ((action ? 1 : -1) != sgn(od->positions[j]->Size)) {
                        cerr << " there may be a problem action is " << (action == 1 ? "SELL" : "BUY") << " Size is "
                             << od->positions[j]->Size << endl;
                    }

                    if (_ofsOrders.is_open())
                        _ofsOrders << "closing position size " << od->positions[j]->Size << " ";

                    if (strat != 0)
                        strat->GetLog() << "closing position size " << od->positions[j]->Size << " ";

                    od->positions[j]->CloseFillTimeStamp = gAsofDate;

                    Fill aFill;
                    aFill.TimeStamp = gAsofDate;
                    aFill.Price = price;
                    aFill.FillSize = filled;
                    od->positions[j]->CloseFills.push_back(aFill);
                    od->positions[j]->CloseRemaining = od->positions[j]->CloseRemaining - filled;
                    if (filled >= abs(od->fills[j])) {
                        unallocated -= abs(od->fills[j]);
                        od->fills[j] = 0;
                    } else {
                        od->fills[j] = od->fills[j] > 0 ? od->fills[j] - filled : od->fills[j] + filled;
                        unallocated = 0;
                    }

                    if (od->positions[j]->CloseRemaining == 0) {

                        double weightedpx = 0;
                        for (auto &f : od->positions[j]->CloseFills) {
                            weightedpx += f.FillSize * f.Price;
                        }
                        od->positions[j]->CloseFillPx = weightedpx / totalfilled;
                        od->positions[j]->mState = Position::Closed;
                        if (_ofsOrders.is_open())
                            _ofsOrders << "final fill at close for " << od->orderId << ", amount: "
                                       << od->positions[j]->Size << endl;
                        if (strat != 0) {
                            strat->GetLog() << "final fill at close for " << od->positions[j]->Size << endl;
                            if(!gBacktest)
                                strat->WritePosition(od->positions[j]);
                        }

                        auto it = strat->OpenPositions().begin();
                        for (; it != strat->OpenPositions().end();) {
                            if ((*it)->Id == od->positions[j]->Id) {
                                strat->GetLog() << "excluding from open positions " << (*it)->Id << endl;
                                it = strat->OpenPositions().erase(it);
                            } else {
                                ++it;
                            }
                        }

                    } else {
                        double weightedpx = 0.0;
                        int partialamt = 0;
                        for (auto &f : od->positions[j]->CloseFills) {
                            weightedpx += f.FillSize * f.Price;
                            partialamt += f.FillSize;
                        }
                        od->positions[j]->CloseFillPx = weightedpx / partialamt;
                        od->positions[j]->mState = Position::ClosedPending;
                        // TODO: FillPx?
                        if (_ofsOrders.is_open())
                            _ofsOrders << "partial fill at close for " << od->orderId << ", amount: " << partialamt
                                       << endl;
                        if (strat != 0)
                            strat->GetLog() << "partial fill at close for " << partialamt << endl;
                    }
                } else {
                    cerr << "unexpected position state " << od->positions[j]->mState << "  Exit Code "
                         << Position::getExitCodeString(od->positions[j]->mExitCode) << endl;
                }
            } else {
                cerr << "no corresponding position set for order " << orderid << endl;
            }
        }
    } else if (od != NULL) {
        cerr << "odd mismatch of insid" << od->symbolId << "==" << insid << endl;
    } else {
        cerr << "order not found " << orderid << endl;
    }
}

void Trader::HandleCandle(string ekey, Candle cdl) {
    if (_map.count(ekey) > 0) {
        vector < Marketable * > v;
        v = _map[ekey];
        if (v.size() == 1) {
            static_cast<Instrument *>(v[0])->HandleCandle(cdl);
        }
    }
}

Strategy *Trader::GetStrategy(uint64_t orderid) {
    Strategy *retval = 0;
    if (_orderMap.count(orderid) > 0) {
        if (_orderMap[orderid] != NULL)
            retval = _orderMap[orderid];
        else
            cout << orderid << " has no strategy affliated with it in GetStartegy" << endl;
    }
    return retval;
}


OrderDetails *Trader::GetOrder(uint64_t orderid) {
    OrderDetails *retval = 0;
    if (_orderMap.count(orderid) > 0) {
        if (_orderMap[orderid] != NULL)
            retval = _orderMap[orderid]->GetOrder(orderid);
        else
            cout << orderid << " has no strategy affliated with it in GetOrder" << endl;
    }
    return retval;
}

vector<Product *> Trader::Products() {
    return _products;
}

const std::unordered_map<uint64_t, Order *> *Trader::GetActiveOrders() const {
    return m_pAdapter->getActiveOrders();
}