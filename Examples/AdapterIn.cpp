//
// Created by lnarayan on 10/30/20.
//
#include <commonv2/AdapterIn.h>
#include <commonv2/topmodelandbookfactory.h>
#include <commonv2/topmodelv2.h>
#include <commonv2/prettylog.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <commonv2/Enum.h>
#include <framework/IModelContext.h>
#include "Base/Position.h"
#include "Base/Utilities.h"
using namespace boost::posix_time;
AdapterIn::AdapterIn(TopModelAndBookFactory  *pFactory)
{
    cout << "Chanel: creating adapter..."  << endl;
    m_pFactory = pFactory;
}
extern ptime gAsofDate;

void AdapterIn::registerCallback(void(*cb)(int, void*), void* clientd, uint64_t delay, bool reoccuring)
{
    auto mctx = m_pFactory->getModel()->getModelContext();
#if( __HC_EVLIB__)
    mctx->addEvTimer(cb, clientd, delay, false /* single timer */ ) ;
#endif
}


int AdapterIn::CancelOrder(uint64_t orderid)
{
    auto om = m_pFactory->getModel()->getOrderManager();
    ORDER_SEND_ERROR cancelStatus = om->sendOrderCancel(orderid);
    if(cancelStatus != SUCCESS)
        cerr <<  "cancel failed for "  << orderid << "  cancel status " <<  Enum::getOrderSendError(cancelStatus) << " @ "  << to_simple_string(gAsofDate) <<endl;
    return cancelStatus;
}
bool AdapterIn::ReplaceOrder(OrderDetails& od)
{
    (void) od;
    return true;
}
extern bool gDebug;
extern bool gSendOrder;
extern ptime gAsofDate;
extern bool gBacktest;

void AdapterIn::CancelAll(int symbolid)
{
    auto model = m_pFactory->getModel();
    auto  il = model->getInstrumentList();
    auto instrument = il->getInstrument(symbolid);
    auto okeys = instrument->getOrderKeys();

    //for(auto & k: okeys)
   // {

   // }
}
/*
const char* EnumRequestTypeString[] = { "BUY", "SELL" };
// return null if string not available for enum val (out of bounds enum val supplied)
string OrderDetails::getRequestTypeString(int val)
{
    if(val >= sizeof(EnumRequestTypeString)/sizeof(char *))
    {
        return 0;
    }
    return EnumRequestTypeString[val];
}
string OrderDetails::getPtypeString(int ptype) {
    string message;

    switch (ptype) {
        case 1:
            message = "MARKET";
            break;
        case 2:
            message = "LIMIT";
            break;
}
}
string OrderDetails::getStatusString(int ptype)
{
    string message;

    switch (ptype) {
        case 0:
            message = "NONE"; // Order was just created.
            break;
        case 1:
            message = "NEW"; // Order was created through createOrder.
            break;
        case 2:
            message = "ADD_PENDING";  // Order was sent, by OrderManager or FPGA
            break;
        case 4:
            message = "REJECTED"; // Order was reject by the ECN.
            break;
        case 8:
            message = "ACCEPTED"; // Order was accepted by the ECN.
            break;
        case 16:
            message = "PENDING_PARTIALLY_FILLED"; // Order was partially filled.
            break;
        case 32:
            message = "PARTIALLY_FILLED"; // Order was partially filled.
            break;
        case 64:
            message = "LEG_FILL";// Order was partially filled.
            break;
        case 128:
            message = "CANCEL_PENDING";// A cancel for this order was sent, by OrderManager or FPGA.
            break;
        case 256:
            message =  "CANCEL_REJECTED"; // Cancel was rejected by the ECN.
            break;
        case 512:
            message = "REPLACE_PENDING";// A replace was sent to the ECN.
            break;
        case 1024:
            message = "REPLACE_REJECTED";// Replace was rejected by the ECN.
            break;
        case 2048:
            message = "EXPIRED"; // Order expired.
            break;
        case 4096:
            message = "CANCELLED"; // Order expired.
            break;
        case 8192:
            message = "HANGING"; // Triggers were cancelled.
            break;
        default:
            break;
    }
    return message;
}
*/

/*string OrderDetails::getTifString(int val)

{
    const char* EnumTifString[] = { "DAY","GTC","FOK","IOC"  };
    // return null if string not available for enum val (out of bounds enum val supplied)

    if(val >= sizeof(EnumTifString)/sizeof(char *))
    {
        return 0;
    }
    return EnumTifString[val];

}*/

string OrderDetails::ToString()
{
    string retval = to_string( orderId);
    /*
    string retval = int64_to_string( orderId);
    if(retval.length() > 6)
        retval =  retval.substr(retval.length() - 6);
    */
    retval += string(",") + Enum::getAction(requestType);
    retval += "," + to_string(symbolId);
    retval += "," + name;
    retval += "," + to_string(quantity);
    retval += "," + to_string(price);
    retval += string(",") + Enum::getOrderType(ptype);
    retval += "," + to_string(aggressor);
    retval += string(",") + Enum::getTif(tif);
    return retval;
}


void AdapterIn::CancelAll() {
    auto om = m_pFactory->getModel()->getOrderManager();
    om->cancelAllOpenOrders();
}

uint64_t AdapterIn::SendIOC(OrderDetails& od)
{
    uint64_t retval = -1;
    auto model = m_pFactory->getModel();
    auto  il = model->getInstrumentList();
    auto instrument = il->getInstrument(od.symbolId);
    HC::instrumentkey_t instrumentKey = instrument->getInstrumentKey( ) ;
    enum MARKET_TYPE marketType = instrument->getMarketType( ) ;
    auto book = instrument->getBook(hcteam::FX_IOC_BOOK);

    if(book == 0)
    {
        cout <<  "book not set FX_IOC_BOOK "  << " @ "  << to_simple_string(gAsofDate)  << endl;
        return -1;
    }

    HC::source_t source = model->getModelContext()->getSourcePriceKey(od.srcPrice.c_str());
    auto om = m_pFactory->getModel()->getOrderManager();
    Order * order=0;
    if(marketType == FUTURES)
        order = om->createOrder( marketType, source, instrumentKey ) ;
    else{
        if (!gBacktest) { // Auto trading user if not in backtest mode.
            cout << "using createOrderAutoTradingUser"  << endl;
            order = om->createOrderAutoTradingUser(marketType, source, instrumentKey);
        } else {
            order = om->createOrder(marketType, source, instrumentKey);
        }
    }
    if (order != NULL) {
        enum HC_GEN::ACTION action = (HC_GEN::ACTION) od.requestType;
        auto level = instrument->getBestLevelIBookEntry(action==HC_GEN::BUY?HC_GEN::SIDE::OFFER:HC_GEN::SIDE::BID,OUTRIGHT_AND_IMPLIED_LEVEL,hcteam::FX_IOC_BOOK);
        order->setAction(action);
        if(level->srcPriceId != source)
        {
            cout << "ioc is being sent to "  << od.srcPrice  << " while inside is " << model->getModelContext()->getSourcePrice(level->srcPriceId)->name  << endl;
            cout << "px: " << level->priceEntry.getPrice() << ",maxamt: " <<  level->priceEntry.getMaxAmount() << endl;
        }
        order->setOrderRate( level->priceEntry.getPrice()) ;
        order->setOrderType(static_cast<HC::FIX_ORDER_TYPE>(od.ptype));
        order->setTimeInForce( HC::FIX_TIME_IN_FORCE::IOC ) ;
        order->setAmount( od.quantity ) ;
        // test if it works for all feeds
        // order->setMaxShowAmount(0);
        order->setECNSequenceNumber( model->getLastSeqNum() ) ;

        ORDER_SEND_ERROR sendOrderResult = om->sendOrder( order, model ) ;
        if(!gBacktest)
            PrettyLog::logOrderSend(model->getLogger(), order, sendOrderResult ) ;
        if (gDebug)
            cout << "sent order " << endl;


        switch ( sendOrderResult ) {
            case SUCCESS:
                if (gDebug)
                    cout << "success " << endl;
                // Store the order key so we can reference it later if needed.  Do NOT store a pointer
                // to the order object.  The OrderManager memory pool is managing the objects.
                instrument->addOrderKey(order->getOrderKey());
                od.orderId = order->getOrderKey();
                retval = order->getOrderKey();
                break;
            default:
                if (true)
                    cout << "send IOC order failed " <<  Enum::getOrderSendError(sendOrderResult) << endl;
        }
    }

    return  retval;
}

uint64_t AdapterIn::SendOrder(OrderDetails& od)
{
    if(!gSendOrder){
        cout << "ORDERS DISABLED @"  << to_simple_string(gAsofDate) << endl;
        return -1;
    }

    auto model = m_pFactory->getModel();
    auto  il = model->getInstrumentList();
    auto instrument = il->getInstrument(od.symbolId);
    auto book = instrument->getBook();  // for now get the realtime book
    hcteam::book_feed_type_t * bft = ( hcteam::book_feed_type_t * ) const_cast<IBook *>(book)->getUserData( ) ;

    // this should be  hcteam::FXFUT_REALTIME_BOOK
    if(*bft != hcteam::FXFUT_REALTIME_BOOK  && gEnableMultipleBooks == false)
    {
        cerr << "CRITICAL:  executions with Reuters and EBS not enabled"  << endl;
        return -1;
    }

    IPriceLevel *bidLvl = book->getPriceLevel(HC_GEN::SIDE::BID, 0,
                                              OUTRIGHT_AND_IMPLIED_LEVEL); // get TOB (level 0) bid price level
    IPriceLevel *offerLvl = book->getPriceLevel(HC_GEN::SIDE::OFFER, 0,
                                                OUTRIGHT_AND_IMPLIED_LEVEL); // get TOB (level 0) OFFER price level

    if(bidLvl==NULL || offerLvl==NULL )
    {
        cerr << "CRITICAL: no price at the time of sending order " << to_simple_string(gAsofDate) << endl; cerr << od.ToString() << endl;
        return -1;
    }
    IPriceLevel *pxLvl=0;

    const IBOOK_ENTRY * bookEntryBid = 0, * bookEntryOffer = 0, *bookEntry = 0;
    enum HC_GEN::ACTION action;
    /* not necessary for limit orders */
    action = (HC_GEN::ACTION)od.requestType;
    string bidsrc;
    string offersrc;
    if(true) {
        bookEntryBid = instrument->getBestLevelIBookEntry(HC_GEN::SIDE::BID);
        bookEntryOffer = instrument->getBestLevelIBookEntry(HC_GEN::SIDE::OFFER);
        bidsrc = model->getModelContext()->getNameForSourcePrice(bookEntryBid->srcPriceId);
        offersrc = model->getModelContext()->getNameForSourcePrice(bookEntryOffer->srcPriceId);
    }

    if (bidLvl->getPrice() == 0 || offerLvl->getPrice()  ==0)
    {
        cerr <<  "CRITICAL:"  << to_simple_string(gAsofDate)  << "  bid/offer level is zero "  << endl;
        return -1;
    }

    string name = instrument->m_instrumentName;


    double payup = od.payup;
    double spread = offerLvl->getPrice() - bidLvl->getPrice();
    bool aggressor = false;
    if(spread < 0)
    {
        cerr << "crossed quotes @"  << to_simple_string(gAsofDate)  << endl;
        cerr << bidLvl->getPrice() << "/" << offerLvl->getPrice() << endl;
    }
    bool goactivecriteria = abs(spread) < payup;
    if(bidLvl->getPrice() == offerLvl->getPrice() )
    {
        cout << "ZERO SPREAD " << bidLvl->getPrice() << "/" << offerLvl->getPrice() << endl;

        if ( HC_GEN::ACTION::BUY == action ) {
            pxLvl = offerLvl;
        } else if ( HC_GEN::ACTION::SELL == action ) {
            pxLvl = bidLvl;
        }
        payup=0;
        aggressor= true;
    }
    else if (od.usemid)
    {
        if (HC_GEN::ACTION::BUY == action) {
            pxLvl = od.aggressor?offerLvl:bidLvl ;
        } else if (HC_GEN::ACTION::SELL == action) {
            pxLvl = od.aggressor?bidLvl:offerLvl;
        }
        aggressor = od.aggressor;
        payup=0;
    }
    else if(goactivecriteria)  // goative criteria
    {
        cout << "WIDE SPREAD " << bidLvl->getPrice() << "/" << offerLvl->getPrice() << endl;

        if ( HC_GEN::ACTION::BUY == action ) {
            pxLvl = offerLvl;
        } else if ( HC_GEN::ACTION::SELL == action ) {
            pxLvl = bidLvl;
        }
        payup=0;
        aggressor= true;
    }
    else {
        if (HC_GEN::ACTION::BUY == action) {
            pxLvl = od.aggressor ? offerLvl : bidLvl;
        } else if (HC_GEN::ACTION::SELL == action) {
            pxLvl = od.aggressor ? bidLvl : offerLvl;
        }
        aggressor = od.aggressor;
        payup=od.payup;
    }
    std::vector<const IBOOK_ENTRY *> entries ;
    pxLvl->getBookEntries(entries);
    bookEntry = entries[0];


    HC::instrumentkey_t instrumentKey = instrument->getInstrumentKey( ) ;
    enum MARKET_TYPE marketType = instrument->getMarketType( ) ;
    HC::source_t source = (marketType == FUTURES)?book->getGatewayID( ) : bookEntry->srcPriceId ;
    auto om = m_pFactory->getModel()->getOrderManager();
    Order * order=0;
    if(marketType == FUTURES)
         order = om->createOrder( marketType, source, instrumentKey ) ;
    else{
        if (!gBacktest) { // Auto trading user if not in backtest mode.
            cout << "using createOrderAutoTradingUser"  << endl;
            order = om->createOrderAutoTradingUser(marketType, source, instrumentKey);
        } else {
            order = om->createOrder(marketType, source, instrumentKey);
        }
    }

    HC::price_t price = pxLvl->getPrice();
    if(od.price != 0  && aggressor == false) {
        price = od.price;
        cout << to_string(od.price)  << " order price will override " << to_string(price) << endl;
    }

    if (order != NULL)
    {

        enum HC_GEN::ACTION action2 = (HC_GEN::ACTION)od.requestType;
        order->setAction( action2 ) ;

        if(od.usemid && od.aggressor == false)  //  what if aggressor?
        {
            price = (action2 == HC_GEN::ACTION::BUY?(price + (spread/2.0)):(price - (spread/2.0)));
        }

        cout << "adapter will " << (action2?"sell":"buy") << endl;
        /*
        if(abs(od.price - price) > tolerance)
        {
            cerr << "why are the prices different " << od.price << "<=>"  << price << " @" << to_simple_string(gAsofDate) << endl;
            cerr << "probably because stop /target level was set based on spread prevailing at time of entry"  << endl;
        }
        */
        if(payup > 0) {
            cout << "paying up? "  <<  payup << endl;
            if (action2 == HC_GEN::ACTION::BUY) {
                price = price + payup;
            } else {
                price = price - payup;
            }
        }

        order->setOrderRate( price ) ;
        cout << "adapter price is " << od.price << endl;
        cout << "Set order type " << Enum::getOrderType(od.ptype)<<endl;
        order->setOrderType(static_cast<HC::FIX_ORDER_TYPE>(od.ptype));
        if (gDebug)
            cout << "default a order type: " <<  model->getConfig( ).getTradingConfig( )->m_defaultOrderType << endl;
        order->setTimeInForce( model->getConfig( ).getTradingConfig( )->m_defaultOrderTif ) ;
        if (true||gDebug)
            cout << "default a order tif: " <<  model->getConfig( ).getTradingConfig( )->m_defaultOrderTif << endl;
        int multiplier =1;
        order->setAmount( od.quantity*multiplier ) ;
        order->setECNSequenceNumber( model->getLastSeqNum() ) ;
    }
    else
    {
        //Instrumentv2 * instrument = m_pModel->getInstrumentList()->getFxInstrument( book->getInstrumentKey() ) ;
    }
    ORDER_SEND_ERROR sendOrderResult = om->sendOrder( order, model ) ;
    if(!gBacktest)
        PrettyLog::logOrderSend(model->getLogger(), order, sendOrderResult ) ;
    if (gDebug)
        cout << "sent order " << endl;
    uint64_t retval = -1;

    if(    offerLvl->getPrice()  -  bidLvl->getPrice() > od.spreadthreshold)
    {
        cout << "CRITICAL : BAD MARKET?  " << bidLvl->getPrice() <<  "/" <<  offerLvl->getPrice() << endl;
    }

    switch ( sendOrderResult ) {
        case SUCCESS:
            if (gDebug)
                cout << "success " << endl;
            // Store the order key so we can reference it later if needed.  Do NOT store a pointer
            // to the order object.  The OrderManager memory pool is managing the objects.
            instrument->addOrderKey(order->getOrderKey());
            od.orderId = order->getOrderKey();
            retval = order->getOrderKey();
            if(od._rofs.is_open()) {
                string oi = to_string(od.orderId);
                if(oi.length() > 7)
                    oi=oi.substr(oi.length() - 7);
                od._rofs << (action?"sell,":"buy,")  << oi << "," << (od.positions.size()==0?"":to_string(od.positions[0]->Id))  << ","
                << formatDouble(spread,6)  << "," << formatDouble(price,6) << ","  << bidLvl->getPrice()  << ","  << offerLvl->getPrice() << "," << formatDouble(payup,6)
                        << "," << (aggressor?"active":"passive") << "," << bidsrc <<"," << offersrc << "," << to_simple_string(gAsofDate)
                        << "," << (od.usemid?formatDouble(price,6):"") << "," << formatDouble(od.spreadthreshold,6) << endl;
            }
            break;
        default:
            cerr << "order failed. was not sent to the exchange for reason: " << Enum::getOrderSendError(sendOrderResult) << endl;
            if(od._rofs.is_open()) {
                od._rofs <<  (action?"sell,":"buy,") << Enum::getOrderSendError(sendOrderResult)  << ", " << (od.positions.size()==0?"":to_string(od.positions[0]->Id))  << ", "
                         << formatDouble(spread,6)  << "," << formatDouble(price,6) << "," << bidLvl->getPrice()  << ","  << offerLvl->getPrice() << "," << formatDouble(od.payup, 6)
                        << "," <<(aggressor?"active":"passive") << "," << bidsrc <<"," << offersrc << "," << to_simple_string(gAsofDate)
                        << "," << (od.usemid?formatDouble(price,6):"") << "," << formatDouble(od.spreadthreshold,6) << endl;
            }
            break;

    }
    return retval;
}



const std::unordered_map<HC::orderid_t, Order *>* AdapterIn::getActiveOrders() const{

    auto om = m_pFactory->getModel()->getOrderManager();
    auto retval = om->getActiveOrders();
    return retval;
}

std::map<int, SymbolInfo> AdapterIn::getSymbolTable()
{
    auto model = m_pFactory->getModel();
    std::map<int, SymbolInfo> retval;
    auto  p = model->getInstrumentList( );
    for(auto itr = p->getFtInstruments(); itr != p->getFtInstrumentsEnd();itr++)
    {
        SymbolInfo si;
        si.m_symbol = itr->second->m_instrumentName;
        si.m_symbolRoot = itr->second->m_instrumentKey;
        retval[itr->first] = si;
    }

    for(auto itr = p->getFxInstruments(); itr != p->getFxInstrumentsEnd();itr++)
    {
        SymbolInfo si;
        si.m_symbol = itr->second->m_instrumentName;
        si.m_symbolRoot = itr->second->m_instrumentKey;
        retval[itr->first] = si;
    }
    return retval;
}

std::map<OrderId, OrderDetails> AdapterIn::getOrderMap()
{
    std::map<OrderId, OrderDetails> retval;
    return retval;
}

