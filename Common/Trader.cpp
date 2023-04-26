#include "Trader.h"
#include <commonv2/modeltypes.h>
#include <commonv2/Enum.h>
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
extern map<string, string> _strategyMap;
extern map<hcteam::book_feed_type_t, long> bookFeedMap;

#include <boost/thread.hpp>

void Worker(Trader *trader) {
    if (gFridayEOD)
        return;
    if (!gReducedLogging) {
        cout << "trying Worker@" << to_simple_string(gAsofDate) << endl;
        cout << PrintThreadID("worker thread") << endl;
    }
    const std::lock_guard<std::mutex> lock(gPositionsMutex);
    if (gAsofDate.is_not_a_date_time()) {
        cout << "gAsofDate is not set yet" << endl;
        return;
    }
    if (!gReducedLogging) { cout << "start writing open positions " << trader->getRouter().size() << endl; }
    for (auto &item : trader->getRouter()) {
        if (!gReducedLogging) {
            cout << "getting strategy for  " << item.first << ": " << trader->GetSymbol(item.first) << endl;
        }
        auto v = trader->getRouter()[item.first];
        for (auto it = v.begin(); it != v.end(); ++it) {
            Strategy *strat = *it;
            try {
                strat->WriteOpenPositions(gAsofDate);
                strat->WritePositions();
            }
            catch (const char *msg) {
                cout << "exception in worker " << msg << endl;
            }
        }
    }
    if (!gReducedLogging) {
        cout << "exiting Worker@" << to_simple_string(gAsofDate) << endl;
    }
    return;
}


void onReocurringCallback(int, void *data) {

    if (!gReducedLogging) {
        cout << "trying onReocurringCallback@" << to_simple_string(gAsofDate) << endl;
        cout << PrintThreadID("onReocurringCallback thread") << endl;
    }
    const std::lock_guard<std::mutex> lock(gPositionsMutex);
    auto d = static_cast<CBData *>(data);
    boost::thread worker(Worker, d->trader);
    if (!gReducedLogging) {
        cout << "exiting onReocurringCallback@" << to_simple_string(gAsofDate) << endl;
    }
}

Trader::Trader(IAdapterIn *adapter) {
    cout << "Chanel: creating trader" << endl;
    m_pAdapter = adapter;
#if TWS
    m_params = *NamedParamsReader::ReadNamedParams("./");
#endif
}

void Trader::RunStrategy() {
    Product *prod = new Product("CME", "UBM1");
    Instrument *ins = new Instrument(prod, 0);
    /*
    if (e.second.m_symbol.find("/") != string::npos) {
        ins->_isFX = true;
    }
     */
    auto strat = new StrategyA(ins, "UBM1", "StratA", 5);

    return;
}

void Trader::PositionUpdate(uint64_t id, double pos) {
    if (_hcMap.count(id) > 0) {
        SymbolInfo si = _hcMap[id];
        _ofsOrders << "got position update for " << si.m_ekey << "," << si.m_exchange << "," << si.m_symbol << ","
                   << to_string(pos) << endl;
    }
    int ce = 0;
    if (_currentExposureMap.count(id) > 0)
        ce = _currentExposureMap[id];

    if (!gBacktest) {
        _ofsOrders << "system says: " << pos << "  current exposure says: " << ce << endl;
    } else {
        _ofsOrders << "current exposure says: " << ce << endl;
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
    string symbol = GetSymbol(id);
    _positionMap[symbol] = pos;
}


#if (__HC_EVLIB__)

void onTraderTimerCallback(int, void *data) {
    if (!gReducedLogging) {
        cout << "trying onTraderTimerCallback@" << to_simple_string(gAsofDate) << endl;
        cout << PrintThreadID("onTraderTimerCallback thread") << endl;
    }
    const std::lock_guard<std::mutex> lock(gPositionsMutex);
    auto d = static_cast<CBData *>(data);
    cout << "sending order using timer callback " << d->od->ToString() << " @ " << to_simple_string(gAsofDate) << endl;
    uint64_t orderid = d->trader->SendOrder(d->od, d->strat);
    cout << to_string(orderid) << " timer order sent successfully." << endl;
    delete d;
    cout << "deleted cbdata" << endl;
    if (!gReducedLogging) {
        cout << "exiting onTraderTimerCallback@" << to_simple_string(gAsofDate) << endl;
    }
}

void onCheckIOCTimerCallback(int, void *data) {
    if (!gReducedLogging) {
        cout << "trying onCheckIOCTimerCallback@" << to_simple_string(gAsofDate) << endl;
        cout << PrintThreadID("onCheckIOCTimerCallback thread") << endl;
    }
    const std::lock_guard<std::mutex> lock(gPositionsMutex);
    auto d = static_cast<CBData *>(data);
    d->trader->CheckIOCFilled(d);
    if (!gReducedLogging) {
        cout << "exitinging onCheckIOCTimerCallback@" << to_simple_string(gAsofDate) << endl;
    }
}

#endif

void Trader::CheckIOCFilled(CBData *data) {

    if (data->od->positions.size() > 0) {
        auto state = data->od->positions[0]->mState;
        auto status = data->od->status;
        // TBD:  if LPs end EXPIRED message  then use that condition to send a regular order
        _ofsOrders << "CheckIOCFilled: order status is " << Enum::getOrderStatus(status) << endl;
        if (state == Position::Open || state == Position::Closed) {  // handle partial open / close
            _ofsOrders << "CheckIOCFilled " << data->od->ToString() << " @ " << to_simple_string(gAsofDate) << endl;
            _ofsOrders << "not sending regular order " << endl;
        } else {
            data->od->tif = HC::FIX_TIME_IN_FORCE::DAY;
            uint64_t orderid = SendOrder(data->od, data->strat);
            _ofsOrders << "sent regular order " << data->od->ToString() << " @ " << to_simple_string(gAsofDate) << endl;
        }
    } else {
        _ofsOrders << "CheckIOCFilled not affialted with a position " << data->od->ToString() << " @ "
                   << to_simple_string(gAsofDate) << endl;
    }
    delete data;
    cout << "deleted cbdata" << endl;
}

uint64_t Trader::SendOrderEx(OrderDetails *od, Strategy *strat) {
    if (!gSendOrder) {
        cout << "ORDERS DISABLED @" << to_simple_string(gAsofDate) << endl;
        return -1;
    }
    cout << "trying SendOrderEx" << endl;
    od->tif = HC::FIX_TIME_IN_FORCE::IOC;
    od->ptype = HC::FIX_ORDER_TYPE::LIMIT;  // most likely this field is ignored
    uint64_t orderid = SendOrder(od, strat);
    _ofsOrders << "sent IOC order " << od->ToString() << " @ " << to_simple_string(gAsofDate) << endl;
    _ofsOrders << "ioc orderid returned " << orderid << endl;
#if (__HC_EVLIB__)
    const bool recurTimer = false; // Timer happens multiple times on the interval?
    //const int timerDelayIntervalMicros = 1 * 60000000; // 1 minute in microseconds, millisecond accuracy.
    const int timerDelayIntervalMicros = gTimerDelayIOC * 1000; // defaults to 30 millisecond accuracy.
    auto cbdata = new CBData();
    cbdata->od = od;
    cbdata->strat = strat;
    cbdata->trader = this;
    _ofsOrders << "will check order " << od->ToString() << " 30 ms later " << " @ " << to_simple_string(gAsofDate)
               << endl;
    m_pAdapter->registerCallback(&onCheckIOCTimerCallback, cbdata, timerDelayIntervalMicros, false);
#endif
    return orderid;
}

uint64_t Trader::SendOrder(OrderDetails *od, Strategy *strat) {

    if (_hcMap.count(od->symbolId) > 0 && gDebug) {
        cout << " sending order for " << _hcMap[od->symbolId].m_symbol << endl;
        cout << od->ToString() << endl;  // Maya pls ensure this is correct
    }

    uint64_t orderid = 0;
    if (od->tif == HC::FIX_TIME_IN_FORCE::IOC) {
        _ofsOrders << "sending ioc" << endl;
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

Strategy *Trader::GetStrategy(int id, int moniker) {
    Strategy *retval = 0;
    auto v = _router[id];
    for (auto it = v.begin(); it != v.end(); it++) {
        if ((*it)->Moniker() == moniker) {
            retval = (*it);
            break;
        }
    }
    return retval;
}

void Trader::EOD(ptime dt) {
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
            strat->WriteOpenPositions(dt);
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

    if (gAsofDate.date().day_of_week() == Friday) {
        gFridayEOD = true;
        m_pAdapter->cancelCallback(gWorkerTimer);
    }
}

void Trader::Expire() {
    _ofsOrders << "start of Expire process; # for symbols is " << _router.size() << endl;
    for (auto &item : _router) {
        _ofsOrders << "getting strategy for  " << item.first << "," << GetSymbol(item.first) << endl;
        auto v = _router[item.first];
        _ofsOrders << "# of strategies is   " << v.size() << endl;
        int maxexposure = 0;
        vector<Position *> offsetPositions;
        for (auto it = v.begin(); it != v.end(); ++it) {
            Strategy *strat = *it;
            _ofsOrders << strat->Name() << "," << strat->Moniker() << endl;
            try {
                // change on June 26
                // strat->Expire(true);
                auto p = strat->ExpireEx();
                if (p != NULL && strat->IsTradeable()) {
                    _ofsOrders << strat->Name() << "_" << strat->Moniker() << endl;
                    _ofsOrders << "offsetting size " << p->Size << endl;
                    offsetPositions.push_back(p);
                    _ofsOrders << "offset pos is " << p->ToString() << endl;
                } else if (strat->IsTradeable() == false) {
                    _ofsOrders << "not tradeable so no offset created " << strat->Name() << "," << strat->Moniker()
                               << endl;
                } else {
                    if (p == NULL) {
                        _ofsOrders << "no positions expiring " << strat->Name() << "," << strat->Moniker()
                                   << endl;
                    } else {
                        _ofsOrders << "CRITICAL at trader expiry " << strat->Name() << "," << strat->Moniker()
                                   << endl;
                    }
                }
                int temp = std::atoi(strat->GetParam("maxExposure").c_str());
                if (temp > maxexposure) {
                    maxexposure = temp;
                }
            }
            catch (const char *msg) {
                cerr << msg << endl;
            }
            strat->ResetPositionsCounter();
        }
        if (offsetPositions.size() > 0) {
            int net = 0;
            Decimal bid, offer;
            for (auto &p : offsetPositions) {
                net += p->Size;
                if (p->Size > 0) {
                    bid = p->AvgPx;
                } else {
                    offer = p->AvgPx;
                }
                _ofsOrders << "aggregating offset @ expiry " << p->ToString() << endl;
            }
            if (net != 0) {
                _ofsOrders << GetSymbol(item.first) << "may be sending net out " << net << endl;
                Decimal px = (net > 0) ? bid : offer;
                auto pos = new Position(Position::nextId, gAsofDate,
                                        px,
                                        net);

                Position::nextId++;
                pos->mEntryCriteria._type = EntryCriteria::EntryType::OFFSET;
                pos->mExitCriteria._expiry = ptime(date(2030, Jan, 1),
                                                   time_duration(hours(0)));  //  essentially manage manually

                vector<Position *> positions;
                positions.push_back(pos);
                vector<int> expectedfills;
                expectedfills.push_back(-net);
                vector<bool> entryflags;
                entryflags.push_back(true);  //since this is an entry


                // create the order
                OrderDetails *od = new OrderDetails(ofglobal);
                od->symbolId = item.first;
                SymbolInfo si;
                if (_hcMap.count(item.first) > 0) {
                    si = _hcMap[item.first];
                }
                od->name = si.m_symbol;
                od->aggressor = true;
                od->requestType = net > 0 ? 0 : 1;  //    BUY = 0, SELL = 1
                od->ptype = 2; // MARKET = 1,     LIMIT = 2
                od->quantity = abs(pos->Size);
                od->tif = 0;
                od->price = pos->ClosePx.getAsDouble();
                for (size_t i = 0; i < expectedfills.size(); i++)
                    od->fills.push_back(expectedfills[i]);
                for (size_t i = 0; i < positions.size(); i++)
                    od->positions.push_back(positions[i]);
                for (size_t i = 0; i < entryflags.size(); i++)
                    od->entryflags.push_back(entryflags[i]);
                // TODO:Sep 19,2021 may need to clear expired positions prior to this
                int currentExposure = GetCurrentExposure(item.first, net);

                if (abs(currentExposure) > maxexposure) {
                    _ofsOrders << "current exposure exceeds maxexposure by " << (abs(currentExposure) - maxexposure)
                               << " hence replacing maxexposure" << endl;
                    maxexposure = abs(currentExposure);
                }
                _ofsOrders << "Current exposure @Expiry before expiring due positions is: " << currentExposure << " @ "
                           << to_simple_string(gAsofDate)
                           << endl;
                _ofsOrders << "Net @Expiry is: " << net << " @ " << to_simple_string(gAsofDate) << endl;
                currentExposure += net;

                if (abs(currentExposure) > maxexposure && sgn(pos->Size) == sgn(currentExposure)) {
                    _ofsOrders << "Netting: abs(potential current exposure) if expiry executed will be "
                               << currentExposure << " exceeds " << maxexposure
                               << endl;
                    int reduceby = abs(currentExposure) - maxexposure;
                    _ofsOrders << "will reduce net by " << sgn<int>(net) * reduceby << endl;
                    pos->Size = net - sgn<int>(net) * reduceby;
                    _ofsOrders << "new position size is " << pos->Size << endl;
                    od->quantity = abs(pos->Size);
                    /* no need since all we can do is not send any expiry orders  */
                    auto v1 = _router[item.first];
                    _ofsOrders << "will try to reduce open positions for # of strategies " << v.size() << endl;
                    int numreduced = 0;
                    int numremaining = sgn<int>(net) * reduceby;
                    vector<Position *> rps;
                    for (auto it = v1.begin(); it != v1.end(); ++it) {
                        Strategy *strat = *it;
                        _ofsOrders << "reducing for " << strat->Name() << "," << strat->Moniker() << endl;
                        /*
                        int amt = strat->ReduceOpenPositions(numremaining);
                        _ofsOrders << "position amt collapsed " << amt << endl;
                        numreduced += amt;
                        numremaining -= amt;
                        if (numremaining == 0) {
                            break;
                        }
                        */
                        auto temp = strat->ReduceOpenPositionsEx(numremaining);
                        _ofsOrders << "reduced " << temp.size() << endl;
                        rps.insert(rps.end(), temp.begin(), temp.end());
                        int amt = 0;
                        for (int j = 0; j < temp.size(); j++)
                            amt += temp[j]->Size;
                        numreduced += amt;
                        numremaining -= amt;
                        if (numremaining == 0) {
                            break;
                        }
                    }

                    int totalcollapsed = 0;
                    // now collapse these with the positions expiring today
                    for (auto it = v1.begin(); it != v1.end(); ++it) {
                        Strategy *strat = *it;
                        _ofsOrders << "collapsing  " << rps.size() << " positions with expired positions for "
                                   << strat->Name() << "," << strat->Moniker() << endl;
                        int numcollapsed = strat->CollapseExpired(rps);
                        totalcollapsed += abs(numcollapsed);

                        if (abs(numcollapsed) != 0) {
                            if (numcollapsed > rps.size()) {
                                _ofsOrders << "CRITICAL: unexpected numcollapsed " << numcollapsed << " >  rps size  "
                                           << rps.size() << endl;
                            }

                            // remove from beginning
                            for (int k = 0; k < abs(numcollapsed); k++) {
                                _ofsOrders << "Erasing " << (*rps.begin())->ToString() << endl;
                                _ofsOrders << "Size is " << rps.size() << endl;
                                rps.erase(rps.begin());
                                _ofsOrders << "Size after erase is " << rps.size() << endl;
                            }
                        }
                    }


                    // TODO: Sep 19,2021 if maxexposure was exceeded prior to expiry this may be true as well
                    if (numremaining > 0) {
                        _ofsOrders << "SUPER CRITICAL could not find collapse candidates for " << GetSymbol(item.first)
                                   << "  @" << to_simple_string(gAsofDate) << endl;
                    }

                }
                if (od->quantity != 0) {
                    string message = "Trader Netting out position:" + to_string(pos->Size);
                    _ofsOrders << message << "  " << od->ToString() << endl;
                    _ofsOrders << "@" << to_simple_string(gAsofDate) << endl;

                    uint64_t oid = 0;
                    if (gIOC && !gBacktest && false)  // cannot do IOC until strategy isidentified else duplicate
                        oid = SendOrderEx(od, 0);  // may need to identify strategy
                    else {
                        try {
                            if (od->quantity > 100 && false) {
                                cerr << "too large order on " << to_simple_string(gAsofDate) << endl;
                            } else {
                                oid = SendOrder(od, 0);
                            }
                        }
                        catch (const char *m) {
                            cerr << "CRITICAL exception " << string(m) << endl;
                        }
                    }

                    if (positions.size() > 0 && entryflags.size() > 0) {
                        if (oid > 0) {
                            cout << (entryflags[0] ? "ENTRY " : "EXIT ") << oid << " sent successfully" << endl;
                            if (entryflags[0])
                                positions[0]->OrderIdEntry = oid;
                            else
                                positions[0]->OrderIdExit = oid;
                        } else {
                            cerr << (entryflags[0] ? "ENTRY " : "EXIT ") << "problem sending order for "
                                 << positions[0]->Id
                                 << endl;

                            if (positions[0]->mEntryCriteria._type == EntryCriteria::EntryType::OFFSET)
                                cerr << "POSSIBLY NEED TO SPLIT ORDER" << endl;
                        }

                    }
                } else {
                    _ofsOrders << "not sending netting order due to  max exposure constraint " << endl;
                }
            } else {
                _ofsOrders << "nothing to net @ " << to_simple_string(gAsofDate) << endl;
            }

            _ofsOrders << "end of Expire process" << endl;
        }
    }
    if (false) {
        CancelAll();
    }
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

    vector<OrderDetails *> retval;
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
        SymbolInfo si;
        if (_hcMap.count(symbolid) > 0) {
            si = _hcMap[symbolid];
        }
        if (!gReducedLogging) {
            cout << "received ticker update for " << si.m_symbol << "@ " << to_simple_string(gAsofDate) << ","
                 << dec::toString(tradePx) << "," << to_string(tradeSz) << endl;
        }
        auto v = _router[symbolid];
        for (auto it = v.begin(); it != v.end(); ++it) {
            Strategy *strat = *it;
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

extern map<string, map<string, string>> stratAparamMap;

// HCTech:  at this stage _hcMap should be populated with descriptive data about subscriptions
void Trader::InitProducts() {

    if (gWorkerThread) {
        //init the timer
        cout << PrintThreadID("main thread") << endl;
        const int timerDelayIntervalMicros = gTimerMins * 1000 * 60000; // call worket thread every 1 minute
        cout << "Reocuuring callback will be called " << gTimerMins << endl;
        auto cbdata = new CBData();
        cbdata->od = 0;
        cbdata->strat = 0;
        cbdata->trader = this;
        gWorkerTimer = m_pAdapter->registerCallback(&onReocurringCallback, cbdata, timerDelayIntervalMicros, true);
    }

    string outdir = gOutputDirectory + "/";

    struct stat info;
    if (stat(outdir.c_str(), &info) != 0) {
        printf("cannot access %s\n", outdir.c_str());
    } else {
        string yyyymmdd = PosixTimeToStringFormat(gAsofDate, "%Y%m%d");
        string fn = outdir + "Orders-" + PosixTimeToStringFormat(gAsofDate, "%Y%m%d") + ".txt";
        cout << "all order related messages in " << fn << endl;
        if (true || !gBacktest || gIOC) {
            _ofsOrders.open(fn, std::ofstream::out);
        }

    }

    for (auto &s : _strategyMap) {
        if (s.first.substr(0, 3) == "VAR") {  // case of Vector Auto Regressive
            vector<string> varSymbols;  // such as VAR1 => ESZ9,NQZ9,EUR/USD
            boost::char_separator<char> sep{",", "", boost::keep_empty_tokens};
            auto f = s.second;
            if (!f.empty()) {
                boost::tokenizer<boost::char_separator<char>> tok(f, sep);
                for (const auto &t : tok)
                    cout << t << endl;
                varSymbols.assign(tok.begin(), tok.end());
            }
            vector<string> exchanges;
            vector<string> symbols;
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
                    vector<string> frequencies;  // EUR/USD_Freq = > 1, 5
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

                            if (_maxExposureMap.count(e.first) == 0) {
                                _maxExposureMap[e.first] = 0;
                                _currentExposureMap[e.first] = 0;
                            }
                            auto strat = new StrategyA(ins, e.second.m_symbol, "StratA", stoi(freq));

                            if (strat->IsInitialized()) {
                                vector<Strategy *> v;
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
    cout << "current exposure map @" << to_simple_string(gAsofDate) << endl;
    _ofsOrders << "current exposure map @" << to_simple_string(gAsofDate) << endl;
    for (auto &ce: _currentExposureMap) {
        cout << _hcMap[ce.first].m_symbol << "=>" << ce.second << endl;
        _ofsOrders << _hcMap[ce.first].m_symbol << "=>" << ce.second << endl;
    }
    cout << "max exposure map" << endl;
    _ofsOrders << "max exposure map" << endl;
    for (auto &me: _maxExposureMap) {
        cout << _hcMap[me.first].m_symbol << "=>" << me.second << endl;
        _ofsOrders << _hcMap[me.first].m_symbol << "=>" << me.second << endl;
    }
}

void Trader::AddExposure(int id, int exp) {
    if (_currentExposureMap.count(id) > 0) {

        _currentExposureMap[id] += exp;
        /*
        if (abs(_currentExposureMap[id]) > abs(_maxExposureMap[id]))
            _maxExposureMap[id] = _currentExposureMap[id];
        */
    } else
        cerr << "current exposure not available for " << GetSymbol(id) << endl;
}

int Trader::GetCurrentExposure(int id, int adjustment /* used during expiry */) {
    int retval = 0;
    int internalexposure = 0;// exposure according to open positions

    auto v = _router[id];
    _ofsOrders << "# of strategies is   " << v.size() << " for " << GetSymbol(id) << endl;
    for (auto it = v.begin(); it != v.end(); ++it) {
        int stratexp = (*it)->Net();
        internalexposure += stratexp;
        (*it)->GetLog() << "strategy exposure for  " << (*it)->FrequencyString() << "  is " << stratexp << " @"
                        << to_simple_string(gAsofDate) << endl;
    }
    _ofsOrders << "internal exposure  is " << internalexposure << " @" << to_simple_string(gAsofDate) << endl;


    if (_currentExposureMap.count(id) > 0) {
        retval = _currentExposureMap[id];
        _ofsOrders << "current exposure  is " << retval << " @" << to_simple_string(gAsofDate) << endl;
    } else
        cerr << "current exposure not available for " << GetSymbol(id) << endl;

    if (internalexposure != retval + adjustment) {
        _ofsOrders << "CRITICAL:  internal exposure " << internalexposure <<
                   " is not equal to current exposure " << retval << " @" << to_simple_string(gAsofDate) << endl;
        for (auto it = v.begin(); it != v.end(); ++it) {
            int stratexp = (*it)->Net(true);
        }
    }
    return retval;
}

void Trader::AddProduct(int symbolid, SymbolInfo si) {
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
    string retval = "NA";
    if (_hcMap.count(id) > 0) {
        retval = _hcMap[id].m_symbol;
    }
    return retval;
}

void Trader::SetMaxExposureAll(int me) {

    for(auto&  item : _router)
    {
        SetMaxExposure(item.first, me);
    }
}

void Trader::SetMaxExposure(int symbolid,int me) {
    cout << "Searching in router " << _router.size() << endl;
    if (_router.count(symbolid) > 0) {
        cout << "setmaxexposure: getting strategy for  " << GetSymbol(symbolid)  <<  " => " << symbolid << endl;
        auto v = _router[symbolid];
        for (auto it = v.begin(); it != v.end(); ++it) {
            Strategy *strat = *it;
            strat->SetMaxExposure(me);
        }
    }
}

void Trader::LiquidateAll(float ratio) {

    for(auto&  item : _router)
    {
        Liquidate(item.first, ratio);
    }
}

void Trader::Liquidate(int symbolid, float ratio) {
    cout << "Searching in router " << _router.size() << endl;

    if (_router.count(symbolid) > 0) {
        cout << "adjust: getting strategy for  " << symbolid << endl;
        auto v = _router[symbolid];
        vector<Position *> offsetPositions;
        for (auto it = v.begin(); it != v.end(); ++it) {
            Strategy *strat = *it;
            if (strat != 0) {
                _ofsOrders << "getting net outstanding for " << strat->Name() << "  " << strat->FrequencyString()
                           << endl;
                int qnum= strat->Net();
                _ofsOrders <<  qnum << endl;
                qnum =  qnum*ratio;
                _ofsOrders <<  "rounded based on ratio to:  "  <<  qnum << endl;
                cout <<  "rounded based on ratio to:  "  <<  qnum << endl;

                Position *p = strat->Liquidate(qnum);

                if (p != NULL && strat->IsTradeable()) {
                    _ofsOrders << p->Size << " positions liquidated to be" << endl;
                    _ofsOrders << strat->Name() << "_" << strat->Moniker() << endl;
                    _ofsOrders << "offsetting size " << p->Size << endl;
                    offsetPositions.push_back(p);
                    _ofsOrders << "offset pos is " << p->ToString() << endl;
                } else if (strat->IsTradeable() == false) {
                    _ofsOrders << "not tradeable so no offset created " << strat->Name() << "," << strat->Moniker()
                               << endl;
                } else {
                    if (p == NULL) {
                        _ofsOrders << "no positions to be liquidated for  " << strat->Name() << "," << strat->Moniker()
                                   << endl;
                    } else {
                        _ofsOrders << "CRITICAL at trader liquidation " << strat->Name() << "," << strat->Moniker()
                                   << endl;
                    }
                }
            }
        }
        if (offsetPositions.size() > 0) {
            int net = 0;
            Decimal bid, offer;
            for (auto &p : offsetPositions) {
                net += p->Size;
                if (p->Size > 0) {
                    bid = p->AvgPx;
                } else {
                    offer = p->AvgPx;
                }
                _ofsOrders << "aggregating offset @ expiry " << p->ToString() << endl;
            }
            if (net != 0) {
                _ofsOrders << GetSymbol(symbolid) << "WILL be sending net out " << net << endl;
                Decimal px = (net > 0) ? bid : offer;
                auto pos = new Position(Position::nextId, gAsofDate,
                                        px,
                                        net);

                Position::nextId++;
                pos->mEntryCriteria._type = EntryCriteria::EntryType::OFFSET;
                pos->mExitCriteria._expiry = ptime(date(2030, Jan, 1),
                                                   time_duration(hours(0)));  //  essentially manage manually

                vector<Position *> positions;
                positions.push_back(pos);
                vector<int> expectedfills;
                expectedfills.push_back(-net);
                vector<bool> entryflags;
                entryflags.push_back(true);  //since this is an entry


                // create the order
                OrderDetails *od = new OrderDetails(ofglobal);
                od->symbolId = symbolid;
                SymbolInfo si;
                if (_hcMap.count(symbolid) > 0) {
                    si = _hcMap[symbolid];
                }
                od->name = si.m_symbol;
                od->aggressor = true;
                od->requestType = net > 0 ? 0 : 1;  //    BUY = 0, SELL = 1
                od->ptype = 2; // MARKET = 1,     LIMIT = 2
                od->quantity = abs(pos->Size);
                od->tif = 0;
                od->price = pos->ClosePx.getAsDouble();
                for (size_t i = 0; i < expectedfills.size(); i++)
                    od->fills.push_back(expectedfills[i]);
                for (size_t i = 0; i < positions.size(); i++)
                    od->positions.push_back(positions[i]);
                for (size_t i = 0; i < entryflags.size(); i++)
                    od->entryflags.push_back(entryflags[i]);


                string message = "Trader liquidating position :" + to_string(pos->Size);
                _ofsOrders << message << "  " << od->ToString() << endl;
                _ofsOrders << "@" << to_simple_string(gAsofDate) << endl;

                uint64_t oid = 0;
                if (gIOC && !gBacktest && false)  // cannot do IOC until strategy isidentified else duplicate
                    oid = SendOrderEx(od, 0);  // may need to identify strategy
                else {
                    try {
                        if (od->quantity > 100 && false) {
                            cerr << "too large order on " << to_simple_string(gAsofDate) << endl;
                        } else {
                            oid = SendOrder(od, 0);
                        }
                    }
                    catch (const char *m) {
                        cerr << "CRITICAL exception " << string(m) << endl;
                    }
                }

                if (positions.size() > 0 && entryflags.size() > 0) {
                    if (oid > 0) {
                        cout << (entryflags[0] ? "ENTRY " : "EXIT ") << oid << " sent successfully" << endl;
                        if (entryflags[0])
                            positions[0]->OrderIdEntry = oid;
                        else
                            positions[0]->OrderIdExit = oid;
                    } else {
                        cerr << (entryflags[0] ? "ENTRY " : "EXIT ") << "problem sending order for "
                             << positions[0]->Id
                             << endl;

                        if (positions[0]->mEntryCriteria._type == EntryCriteria::EntryType::OFFSET)
                            cerr << "POSSIBLY NEED TO SPLIT ORDER" << endl;
                    }

                }

            }
        }
    } else {
        cout << "liquidate:  symbolid  not found " << symbolid << endl;
    }
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

void Trader::AddProducts(string name, vector<int> ids, vector<string> p, vector<string> exchange) {
    return;  // pending initiate VAR strategy
    auto synthetic = new Synthetic(name);
    vector<Product *> products;

    //UN Debugging
    vector<Marketable *> v = _map[exchange[0] + "_" + p[0]];
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
        vector<Strategy *> v1;
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
    cout << "trying to lock HandleOrderEvent" << endl;
    const std::lock_guard<std::mutex> lock(gPositionsMutex);
    if (gCancelKludge && gBacktest) {
        return;
    }

    if (_ofsOrders.is_open()) {
        _ofsOrders << "====> " << GetSymbol(insid) << "  orderevent for inst received @ "
                   << to_simple_string(gAsofDate)
                   << endl;;
        _ofsOrders << "insid " << insid << ",orderid " << orderid << ",eventtype "
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
        case 2048:
            cout << orderid << "-EXPIRED @" << to_simple_string(gAsofDate) << endl;
            break;
        default:
            cerr << orderid << "-EVENTTYPE: " << OrderStateToString((ORDER_STATUS) eventtype) << " @"
                 << to_simple_string(gAsofDate) << endl;
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
    cout << "trying HandleFillEvent" << endl;
    const std::lock_guard<std::mutex> lock(gPositionsMutex);

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
        _ofsOrders << "====> " << GetSymbol(insid) << " fill received @" << to_simple_string(gAsofDate) << endl;
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
    if (strat != 0 && (gDebug || !gBacktest)) {
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
                    strat->GetLog() << "for position " << od->positions[j]->Id << "  @ "
                                    << to_simple_string(gAsofDate)
                                    << " ";
                }

                if (od->positions[j]->mState ==
                    Position::OpenPending)  //initiating position (second clause to cover partial fill
                {

                    if (sgn<int>(od->positions[j]->Size) != (action == 0 ? 1 : -1)) {
                        cerr << "a partial fill received for " << od->positions[j]->Id << endl;
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
                    if (sgn<int>(od->positions[j]->Size) == (action == 0 ? 1 : -1)) {
                        cerr << "a partial fill received for ClosedPending " << od->positions[j]->Id << endl;
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
                            /*
                            if(!gBacktest)
                                strat->WritePosition(od->positions[j]);
                            */
                        }

                        auto it = strat->OpenPositions().begin();
                        for (; it != strat->OpenPositions().end();) {
                            if ((*it)->Id == od->positions[j]->Id) {
                                strat->GetLog() << "excluding from open positions " << (*it)->Id << endl;
                                it = strat->OpenPositions().erase(it);
                            } else {
                                //strat->WriteOpenPosition(*it);
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
                    cerr << "unexpected position state " << Position::getStateString(od->positions[j]->mState)
                         << "  Exit Code "
                         << Position::getExitCodeString(od->positions[j]->mExitCode) << endl;
                    cerr << od->positions[j]->ToString() << endl;
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

    if (ekey.empty()) {
        return;
    }
    if (_map.count(ekey) > 0) {
        vector<Marketable *> v;
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