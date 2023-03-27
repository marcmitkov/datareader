#include <commonv2/topfxmodel.h>
#include <commonv2/instrumentlist.h>
#include <commonv2/instrumentv2.h>
#include <commonv2/prettylog.h>
#include <ibook/IBook.h>
#include <framework/IModelContext.h>
#include "../Base/IAdapterOut.h"
#include "../Base/Utilities.h"

extern IAdapterOut *pTrader;

void TopFxModel::initialize() {
    this->initializeTradingUserKeys();
    if (!this->m_config.getVelioConfig()->m_backtest) {
        IVelioSession *ivs = IVelioSession::getInstance();
        HC::ecn_t ecnKey = ivs->getEcnKey("ZODIAC");
        ivs->registerForInstrumentInfoUpdate((short) ecnKey, *((IInstrumentInfoSubscriber *) this));
    }

    TopTradeModel::initialize();
}

void TopFxModel::futureTickerUpdate(HC::source_t source, FUTURES_TICKER_ENTRY *ticker) {
    (void) source;
    (void) ticker;
    //std::cout << "futures ticker received"  << std::endl;
}

uint32_t TopFxModel::getInstrumentEnabledForPilotCount() {
    // Note: If Futures are also enabled for trading and should pilot, this result won't be correct.
    // However most FX models don't trade futures and use them only for indication.
    // By default TopFxModel doesn't have the code to create futures orders.
    // During pilot, order will be sent (number of enabled instrument * number of pilots each)
    return this->m_instrumentList.getFxEnabledForTradingCount();
}

extern ptime gAsofDate;

void TopFxModel::onConflationDone() {
    const IBook *book = getNextUpdateIBook();
    while (nullptr != book) {
        handleMarketData(book);
        book = getNextUpdateIBook();
    }
    clearUpdatedIBooks();
}

extern bool gDebug;

void TopFxModel::sourceStatus(SOURCE_LEVEL_STATUS_STRUCT *status) {
    if (gDebug) {
        cout << "sourceStatus received fx @" << to_simple_string(gAsofDate) << endl;
        cout << status->m_sl_status << endl;
        cout << status->m_sl_connection_type << endl;
    }
}

#include <Base/IAdapterOut.h>
#include <Base/Utilities.h>

extern IAdapterOut *pTrader;
bool runOnce = true;
extern ptime gAsofDate;
extern bool gDebug;

using namespace boost::gregorian;


void OnTimer(TopTradeModel *model) {
    (void) model;
    cout << CurrentThreadId("OnTimer") << endl;
}

void DoRoll() {
    if (gTomorrow.is_not_a_date_time() == false && gAsofDate.is_not_a_date_time() == false) {  // both dates are set
        //  next time date crosses over
        if (gAsofDate >= gTomorrow) {
            if (gEODHours > 0 || gEODMins > 0)
                gTomorrow = AdvanceTime(gAsofDate.date(), 24 + gEODHours, gEODMins, 0);
            else
                gTomorrow = AdvanceTime(gAsofDate.date(), 48, 0, 0);
            cout << "tomorrow is changed to " << to_simple_string(gTomorrow)
                 << endl;  // TODO:  incorporate holidays/weekends etc
            pTrader->EOD();
            /*
            if (gBacktest == false)
                gNewSignal = true; // restart processing signals
            */
        }
        if (gAsofDate >= gRoll) {
            if (gBacktest)
                gCancelKludge = true;  // will supress the fills of cancelled orders and autofill offset
            pTrader->Expire();
            gNewSignal = false;   // no new signals will be processed for the remainder of the day
            if (gRollHours > 0 || gRollMins > 0)
                gRoll = AdvanceTime(gAsofDate.date(), 24 + gRollHours, gRollMins, 0);
            else
                gRoll = AdvanceTime(gAsofDate.date(), 47, 30, 0);
            // regular roll: UTC = EST + 5, CST  + 6 , SGT = UTC + 8 (DST - Spring/Summer  UTC = EST + 4)
            cout << "roll is changed to " << to_simple_string(gRoll)
                 << endl;  // TODO:  incorporate holidays/weekends etc

        }
        if (gAsofDate.date().day_of_week() == Friday) {
            if (gAsofDate >= gTomorrow) {
                if (gEODHours > 0 || gEODMins > 0)
                    gTomorrow = AdvanceTime(gAsofDate.date(), 24 + gEODHours, gEODMins, 0);
                else
                    gTomorrow = AdvanceTime(gAsofDate.date(), 48, 0, 0);
                cout << "tomorrow is changed to " << to_simple_string(gTomorrow)
                     << endl;  // TODO:  incorporate holidays/weekends etc
                cout << "Friday EOD @" << to_simple_string(gAsofDate) << endl;
                pTrader->EOD();
            }
            if (gAsofDate >= gRoll) {
                cout << "Friday Roll @" << to_simple_string(gAsofDate) << endl;
                pTrader->Expire();
                gNewSignal = false;
                if (gRollHours > 0 || gRollMins > 0)
                    gRoll = AdvanceTime(gAsofDate.date(), 24 + gRollHours, gRollMins, 0);
                else
                    gRoll = AdvanceTime(gAsofDate.date(), 47, 30, 0);
                // regular roll: UTC = EST + 5, CST  + 6 , SGT = UTC + 8 (DST - Spring/Summer  UTC = EST + 4)
                cout << "roll is changed to " << to_simple_string(gRoll)
                     << endl;  // TODO:  incorporate holidays/weekends etc
            }

        }
    }
}


ptime ConvertHCTime(IModelContext *modelcontext) {
    HC::timestamp_t hctime = modelcontext->getTime(CURRENT_TIME);
    // HC::timestamp_t hctime = book->getLastSendingTime( )  (this is blank for ICE for some reason)
    HC::timestamp_t nano = 1000000000;
    ptime t1 = from_time_t(hctime / nano);  // get the secs
    t1 += millisec((hctime % nano) / 1000000); // get the millisecs
    string s = to_simple_string(t1);
    if (gDebug)
        cout << s << " about to set timestamp" << endl;
    return t1;
}

/*
#include <backtest/EngineFactory.h>
extern boost::posix_time::ptime parse_time_object(const std::string &time, const std::string &format);
ptime GetBacktestDate(IModelContext *modelcontext)
{
    BACKTEST_PROPERTIES bProperties;
    bProperties.propertyFile = modelcontext->getProperty("velio.model.backTestFile");
    EngineFactory::readProperties(&bProperties);

    //printf("%s\n",bProperties.startDate.c_str());
    //printf("%s\n",bProperties.endDate.c_str());

    cout << to_simple_string(ConvertHCTime(modelcontext)) << endl;
    cout <<bProperties.endDate.c_str() << endl;
    ptime retval = parse_time_object(bProperties.endDate.c_str(),"%Y%m%d");
    return retval;
}
*/
ptime cmeHalt;
ptime cmeResume;

extern void RunOnceStrategy();

void RunOnce(IModelContext *modelcontext) {
    cout << "as of date at the time of runonce " << to_simple_string(gAsofDate.date()) << endl;
    cmeHalt = AdvanceTime(gAsofDate.date(), 21, 59, 29);  // non DST CME halt;
    cmeResume = AdvanceTime(gAsofDate.date(), 23, 00, 01);

    if (IsDST(gAsofDate)) {
        cout << "runonce: we are in DST " << to_simple_string(gAsofDate) << endl;
        cmeHalt = AdvanceTime(gAsofDate.date(), 20, 59, 59);
        cmeResume = AdvanceTime(gAsofDate.date(), 22, 00, 01);
    }

    RunOnceStrategy();

    cout << "initializing products on first quote " << to_simple_string(gAsofDate) << endl;

    if (gAsofDate.time_of_day().hours() >= 23) {
        cerr << "ignoring first quote " << to_simple_string(gAsofDate) << endl;
        return;
    }
    /*
    if(gBacktest)
    {
        ptime temp = GetBacktestDate(modelcontext);
        cout << to_simple_string(temp)  << endl;

        if(temp.date() != gAsofDate.date())
        {
            cerr << "CRITICAL: backtest date "  << to_simple_string(temp)  << " different from time stamp received "  <<  to_simple_string(gAsofDate)  << endl;
            return;
        }
    }*/
    pTrader->InitProducts();
    runOnce = false;
    if (gAsofDate.date().day_of_week() == Friday) {
        cout << "Happy Friday!" << endl;
        if (IsDST(gAsofDate)) {
            cout << "Friday in DST"  << endl;
            gTomorrow = AdvanceTime(gAsofDate.date(), 20, 0, 0);
            gRoll = AdvanceTime(gAsofDate.date(), 19, 00, 0);  // default roll time
        } else {
            gTomorrow = AdvanceTime(gAsofDate.date(), 21, 0, 0);
            gRoll = AdvanceTime(gAsofDate.date(), 20, 00, 0);  // default roll time
        }
        cout << "Friday Tomorrow is " << to_simple_string(gTomorrow) << endl;
        cout << "Friday Roll time is " << to_simple_string(gRoll) << endl;
    } else {
        if (gEODHours > 0 || gEODMins > 0) {
            if (IsDST(gAsofDate))
                gTomorrow = AdvanceTime(gAsofDate.date(), gEODHours -1, gEODMins, 0);
            else
                gTomorrow = AdvanceTime(gAsofDate.date(), gEODHours, gEODMins, 0);
        }
        else
            gTomorrow = AdvanceTime(gAsofDate.date(), 24, 0, 0);  // default roll time


        if (gRollHours > 0 || gRollMins > 0) {
            if (IsDST(gAsofDate))
                gRoll = AdvanceTime(gAsofDate.date(), gRollHours - 1, gRollMins,
                                    0);  // just for holiday  Dec 31, 2020 early close
            else
                gRoll = AdvanceTime(gAsofDate.date(), gRollHours, gRollMins,
                                    0);  // just for holiday  Dec 31, 2020 early close
        } else
            gRoll = AdvanceTime(gAsofDate.date(), 22, 0, 0);  // default roll time
    }

    cout << "tomorrow is " << to_simple_string(gTomorrow) << endl;
    cout << "Roll is " << to_simple_string(gRoll) << endl;
}

map<hcteam::book_feed_type_t, long> bookFeedMap;

bool TopFxModel::IsBookCurrent(const IBook *book) {
    hcteam::book_feed_type_t *bft = (hcteam::book_feed_type_t *) const_cast<IBook *>(book)->getUserData();
    //cout << "current book is "  << hcteam::book_feed_types[*bft]  << endl;
    IPriceLevel *bidLvl = book->getPriceLevel(HC_GEN::SIDE::BID, 0,
                                              OUTRIGHT_AND_IMPLIED_LEVEL); // get TOB (level 0) bid price level
    IPriceLevel *offerLvl = book->getPriceLevel(HC_GEN::SIDE::OFFER, 0,
                                                OUTRIGHT_AND_IMPLIED_LEVEL); // get TOB (level 0) OFFER price level

    if (bidLvl == 0) {
        cout << "bidLvl not set" << endl;
        return false;
    }

    HC::price_t bidPx = bidLvl->getPrice();

    HC::amount_t bidQty = bidLvl->getQuantity(OUTRIGHT_AND_IMPLIED_LEVEL);

    if (offerLvl == 0) {
        cout << "offerLvl not set" << endl;
        return false;
    }
    HC::price_t askPx = offerLvl->getPrice();
    HC::amount_t askQty = offerLvl->getQuantity(OUTRIGHT_AND_IMPLIED_LEVEL);


    auto il = getInstrumentList();
    auto instrument = il->getInstrument(book->getInstrumentKey());
    auto otherbook = instrument->getBook(*bft == hcteam::FXFUT_REALTIME_BOOK ? hcteam::FX_CONFLATED_BOOK
                                                                             : hcteam::FXFUT_REALTIME_BOOK);  // for now get the realtime book

    hcteam::book_feed_type_t *bft2 = (hcteam::book_feed_type_t *) const_cast<IBook *>(otherbook)->getUserData();
    //cout << "other book is "  << hcteam::book_feed_types[*bft2]  << endl;
    IPriceLevel *bidLvl2 = otherbook->getPriceLevel(HC_GEN::SIDE::BID, 0,
                                                    OUTRIGHT_AND_IMPLIED_LEVEL); // get TOB (level 0) bid price level
    IPriceLevel *offerLvl2 = otherbook->getPriceLevel(HC_GEN::SIDE::OFFER, 0,
                                                      OUTRIGHT_AND_IMPLIED_LEVEL); // get TOB (level 0) OFFER price level

    if (bidLvl2 == 0) {
        cout << "bidLvl2 not set" << endl;
    }

    if (offerLvl2 == 0) {
        cout << "offerLvl2 not set" << endl;
    }

    if (bidLvl2 != 0 && offerLvl2 != 0) {
        HC::price_t bidPx2 = bidLvl2->getPrice();
        HC::amount_t bidQty2 = bidLvl2->getQuantity(OUTRIGHT_AND_IMPLIED_LEVEL);
        HC::price_t askPx2 = offerLvl2->getPrice();
        HC::amount_t askQty2 = offerLvl->getQuantity(OUTRIGHT_AND_IMPLIED_LEVEL);
        if ((bidPx2 > bidPx) && (askPx2 < askPx))  // other book has a narrower spread
        {
            if (*bft2 == hcteam::FX_CONFLATED_BOOK)
                cout << "other book has a narrower spread  " << hcteam::book_feed_types[*bft2] << " @ "
                     << to_simple_string(gAsofDate) << endl;

            if (bookFeedMap.count(*bft2) > 0) {
                long t = bookFeedMap[*bft2];
                bookFeedMap[*bft2] = ++t;
            } else {
                bookFeedMap[*bft2] = 1;
            }
            return false;
        }
    }
    if (bookFeedMap.count(*bft) > 0) {
        long t = bookFeedMap[*bft];
        bookFeedMap[*bft] = ++t;
    } else {
        bookFeedMap[*bft] = 1;
    }
    return true;
}

void SendQuoteToTrade(const IBook *book, IModelContext *modelcontext) {
    gAsofDate = ConvertHCTime(modelcontext);
    if (runOnce) {
        RunOnce(modelcontext);
    }

    int symbolid = book->getInstrumentKey();
    IPriceLevel *bidLvl = book->getPriceLevel(HC_GEN::SIDE::BID, 0,
                                              OUTRIGHT_AND_IMPLIED_LEVEL); // get TOB (level 0) bid price level
    IPriceLevel *offerLvl = book->getPriceLevel(HC_GEN::SIDE::OFFER, 0,
                                                OUTRIGHT_AND_IMPLIED_LEVEL); // get TOB (level 0) OFFER price level

    if (bidLvl == 0) {
        cout << "bidLvl not set" << endl;
        return;
    }

    HC::price_t bidPx = bidLvl->getPrice();

    HC::amount_t bidQty = bidLvl->getQuantity(OUTRIGHT_AND_IMPLIED_LEVEL);

    if (offerLvl == 0) {
        cout << "offerLvl not set" << endl;
        return;
    }
    HC::price_t askPx = offerLvl->getPrice();
    HC::amount_t askQty = offerLvl->getQuantity(OUTRIGHT_AND_IMPLIED_LEVEL);


    if (gDebug) {
        cout << "sending quote to trader" << endl;
        cout << "quote "  <<  bidPx << "/"  << askPx << endl;
    }
    if (askPx / bidPx > 1.5 || askPx / bidPx < 0.5) {
        cerr << "bad tick " << bidPx << "/" << askPx << "@" << to_simple_string(gAsofDate) << endl;
    } else {
        if (bidPx <= askPx || (bidPx > askPx && !gBacktest)) {
            pTrader->HandleQuote(symbolid, Decimal(bidPx), bidQty, Decimal(askPx), askQty);
            DoRoll();  // do this later since we need the last candle  to expire positions off etc.
            if (bidPx > askPx && gDebug) {
                cerr << "juicy opportunity  " << bidPx << "/" << askPx << " @" << to_simple_string(gAsofDate) << endl;
            }
        } else if (gBacktest && bidPx > askPx) {
            if (false)
                cerr << "ignoring crossed quote in backtest mode " << bidPx << "/" << askPx << " @"
                     << to_simple_string(gAsofDate) << endl;
            else {
                pTrader->HandleQuote(symbolid, Decimal(bidPx), bidQty, Decimal(askPx), askQty);
                DoRoll();  // do this later since we need the last candle  to expire positions off etc.
                if (book->getMarketType() == (MARKET_TYPE) gMarketType && gMarketHalted == false) {
                    if (crossCountMap.count(gAsofDate.date()) > 0) {
                        double spread = bidPx - askPx;
                        int num = crossCountMap[gAsofDate.date()].numCrossed;
                        double mean = crossCountMap[gAsofDate.date()].meanSpread;
                        crossCountMap[gAsofDate.date()].meanSpread = (mean * num + spread) / (num + 1);
                        crossCountMap[gAsofDate.date()].numCrossed = ++num;
                        crossCountMap[gAsofDate.date()].last = gAsofDate;
                        if (crossCountMap[gAsofDate.date()].maxSpread < spread)
                            crossCountMap[gAsofDate.date()].maxSpread = spread;
                    } else {
                        double spread = bidPx - askPx;
                        crossCountMap[gAsofDate.date()].numCrossed = 1;
                        crossCountMap[gAsofDate.date()].meanSpread = spread;
                        crossCountMap[gAsofDate.date()].maxSpread = spread;
                        crossCountMap[gAsofDate.date()].first = gAsofDate;
                        crossCountMap[gAsofDate.date()].last = gAsofDate;
                    }
                }
            }
        }
    }
}


bool TopFxModel::IsConflated(const IBook *book) {
    bool retval = false;
    int symbolid = book->getInstrumentKey();
    auto il = getInstrumentList();
    auto instrument = il->getInstrument(symbolid);

    hcteam::book_feed_type_t *bft = (hcteam::book_feed_type_t *) const_cast<IBook *>(book)->getUserData();

    // identify  whether it is a conflated source or not
    /* alternative way to identify pricing source (pre hcteam::book_feed_type_t)
    HC::source_t gidReutersMcast = 7;
    HC::source_t gidEBSUltraNYA = 666;
    if (book->getGatewayID() == gidEBSUltraNYA || book->getGatewayID() == gidReutersMcast) {
        //  then we know it is coming from the conflated book of reuters / ebs
    }
    */
    const IBOOK_ENTRY *bookEntryBid = 0, *bookEntryOffer = 0, *bookEntry = 0;
    string bidsrc;
    string offersrc;

    bookEntryBid = instrument->getBestLevelIBookEntry(HC_GEN::SIDE::BID, OUTRIGHT_AND_IMPLIED_LEVEL, *bft);
    bookEntryOffer = instrument->getBestLevelIBookEntry(HC_GEN::SIDE::OFFER, OUTRIGHT_AND_IMPLIED_LEVEL, *bft);
    if (bookEntryBid != 0) {
        bidsrc = getModelContext()->getNameForSourcePrice(bookEntryBid->srcPriceId);
        cout << "bid source " << bidsrc << "  " << bookEntryBid->srcPriceId << endl;
        if (bidsrc == "REUTERS_MCAST" || bidsrc == "EBS_ULTRA_NY_A") {
            return true;
        }
    } else
        cout << "no  bid found" << endl;
    if (bookEntryOffer != 0) {
        offersrc = getModelContext()->getNameForSourcePrice(bookEntryOffer->srcPriceId);
        cout << "offer source " << offersrc << "  " << bookEntryOffer->srcPriceId << endl;
        if (offersrc == "REUTERS_MCAST" || offersrc == "EBS_ULTRA_NY_A") {
            return true;
        }
    } else
        cout << "no offer found" << endl;
    return retval;
}


// end test

void TopFxModel::handleMarketData(const IBook *book) {

    if (!book->isBookSet()) { // Handle empty or 1-sided book.
        return;
    }


    m_ctx->getTime(CURRENT_TIME);
    // Only interact with FX books in this callback.

    Instrumentv2 *instrument = this->m_instrumentList.getFxInstrument(book->getInstrumentKey());

    if (!instrument) { // Handle non-CME Futures Market Data (EUREX / ICE / LIFFE )
        // If you want to change this and also trade futures, then you need to merge
        // TopFtModel::createOneOrder into TopFxModel::createOneOrder and setup order
        // for futures vs. fx depending on market type.
        this->handleFtMarketData(book);
        return;
    }
    //if(IsBookCurrent(book)) {
    SendQuoteToTrade(book, m_ctx);
    //}

    return;
}

void processCommand(TopModelv2 *model, CONSOLE_COMMAND_STRUCT *cmd);

// buy , sell ,  cancel
void TopFxModel::receiveConsoleCommand(CONSOLE_COMMAND_STRUCT *cmd) {
    if (0 == cmd) {
        LOG_ERROR_CUSTOM((&m_logger), "[TopFxModel:receiveConsoleCommand] Received null command.");
        return;
    }
    try {
        processCommand(this, cmd);
    }
    catch (const char *msg) {
        cerr << "EXCEPTION on: " << msg << endl;
    }
}


void TopFxModel::handleFtMarketData(const IBook *book) {
    FtInstrument *instrument = (FtInstrument *) this->m_instrumentList.getFtInstrument(book->getInstrumentKey());

    if (instrument) {
        if (gDebug) {
            cout << "handle futures quote" << endl;
            PrettyLog::logQuote(m_logger, book);
            cout << " for instrument id " << instrument->getInstrumentKey() << endl;
        }
        SendQuoteToTrade(book, m_ctx);
    }
}

int tempctr = 0;

void TopFxModel::tradeTickerUpdate(TRADE_TICKER_STRUCT *tradeTicker, bool isMine) {
    (void) tradeTicker;
    (void) isMine;
    gAsofDate = ConvertHCTime(m_ctx);
    if (runOnce) {
        RunOnce(m_ctx);
    }
    /*
    if(tempctr < 1001) {
        cout << "ecn " << tradeTicker->m_ecn << ",gateway " << tradeTicker->m_gateway << " @ " << to_simple_string(gAsofDate) << endl;
        tempctr++;
    }
    */
    pTrader->HandleTicker(tradeTicker->m_instrument_key, Decimal(tradeTicker->m_rate), tradeTicker->m_amount,
                          tradeTicker->m_side);
}

Order *TopFxModel::createOneOrder(const IBook *book, Instrumentv2 *instrument) {
    (void) book;
    const IBOOK_ENTRY *bookEntry = 0;
    enum HC_GEN::ACTION action = instrument->getNextAction();
    if (HC_GEN::ACTION::BUY == action) {
        bookEntry = instrument->getBestLevelIBookEntry(HC_GEN::SIDE::OFFER);
    } else if (HC_GEN::ACTION::SELL == action) {
        bookEntry = instrument->getBestLevelIBookEntry(HC_GEN::SIDE::BID);
    }

    HC::instrumentkey_t instrumentKey = instrument->getInstrumentKey();

    if (!bookEntry) {
        LOG_ERROR_CUSTOM((&m_logger),
                         "[TopFxModel:createOneOrder] Unable to create instrument %ld %s order. No bestLevelIBookEntry on that side.",
                         instrumentKey,
                         (HC_GEN::ACTION::SELL == action ? "SELL" : "BUY"));
        return 0;
    }

    HC::source_t source = bookEntry->srcPriceId;
    HC::price_t price = bookEntry->priceEntry.m_price;
    enum MARKET_TYPE marketType = instrument->getMarketType(); // Must be FX
    static bool isBacktest = this->m_config.getVelioConfig()->m_backtest;
    Order *order = 0;

    if (!isBacktest) { // Auto trading user if not in backtest mode.
        order = m_orderManager->createOrderAutoTradingUser(marketType, source, instrumentKey);
    } else {
        order = m_orderManager->createOrder(marketType, source, instrumentKey);
    }

    if (!order) {
        LOG_ERROR_CUSTOM((&m_logger),
                         "[TopFxModel:createOneOrder] Order not created. m_orderManager returned NULL on createOrderAutoTradingUser.");
        return 0;
    }

    order->setAction(action);
    order->setOrderRate(price);
    order->setOrderType(this->getConfig().getTradingConfig()->m_defaultOrderType);
    order->setTimeInForce(this->getConfig().getTradingConfig()->m_defaultOrderTif);
    order->setAmount(this->getConfig().getTradingConfig()->m_defaultAmount);
    order->setECNSequenceNumber(m_ctx->getLastSeqNum());

    if (HC::FIX_ORDER_TYPE::ICEBERG == this->getConfig().getTradingConfig()->m_defaultOrderType) {
        order->setMaxShowAmount(this->getConfig().getTradingConfig()->m_defaultMaxShowAmount);
    }

    return order;
}


void TopFxModel::globalPositionUpdate(POSITION_DATA_STRUCT* pPos) {
    //const std::lock_guard<std::mutex> lock(gPositionsMutex);
    string symbol =  pTrader->GetSymbol(pPos->instrumentKey);
    cout << "FX position update for " << symbol  <<  ","  <<  to_string(pPos->position) << ","  <<  to_string(pPos->modelKey)  <<  " @ "
         << to_simple_string(gAsofDate) << endl;
    pTrader->PositionUpdate(pPos->instrumentKey, pPos->position);

}
