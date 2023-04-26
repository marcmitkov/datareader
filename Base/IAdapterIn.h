#pragma once
#ifdef TWS
#include "../TwsSocketClient/Contract.h"
#include "../TwsSocketClient/Order.h"
#include "../TwsSocketClient/OrderState.h"
#include "../TwsSocketClient/CommonDefs.h"
#else
#include <unordered_map>
#include <vector>
#endif
#include <map>
#include <string>
#include "../Base/SymbolInfo.h"
using namespace std;
class Position;
#ifdef WIN32
enum ORDER_STATUS {
    NONE = 0,                           // Order was just created.
    NEW = 1,                            // Order was created through createOrder.
    ADD_PENDING = 2,                    // Order was sent, by OrderManager or FPGA
    REJECTED = 4,                       // Order was reject by the ECN.
    ACCEPTED = 8,                       // Order was accepted by the ECN.
    PENDING_PARTIALLY_FILLED = 16,      // Order was partially filled.
    PARTIALLY_FILLED = 32,              // Order was partially filled.
    LEG_FILL = 64, 	                    // Order was partially filled.
    CANCEL_PENDING = 128,               // A cancel for this order was sent, by OrderManager or FPGA.
    CANCEL_REJECTED = 256,              // Cancel was rejected by the ECN.
    REPLACE_PENDING = 512,              // A replace was sent to the ECN.
    REPLACE_REJECTED = 1024,            // Replace was rejected by the ECN.
    EXPIRED = 2048,                     // Order expired.
    CANCELLED = 4096,                   // Order expired.
    HANGING = 8192           // Triggers were cancelled.
};
#endif
struct OrderDetails {
    OrderDetails(ofstream& ofs) : _rofs(ofs){}
	// unified interface to send order
	string srcPrice;
    uint64_t orderId;
    int requestType;  // 0:BUY 1:SELL
    int symbolId;
    string name;
    int quantity;
    double price;
    int ptype; // MARKET = 1,     LIMIT = 2, HC::FIX_ORDER_TYPE
        int tif; //             DAY = 0, GTC = 1, IOC,  HC::FIX_TIME_IN_FORCE::IOC,     IOC = 3            FOK = 2,
	bool aggressor;
	vector<int> fills;
	vector<Position*> positions;
    vector<bool> entryflags;  //entry or exit
    int status;  //     ACCEPTED = 8,    PENDING_PARTIALLY_FILLED = 16,  PARTIALLY_FILLED = 32, EXPIRED = 2048
    double payup;
    double spreadthreshold=20;  //make this a large number
    bool usemid;
    ofstream& _rofs;
#ifdef TWS
	Contract m_contract;  //TBD:  do we need just contId?
	Order m_order;
#else
    string ToString();
#endif
	//OrderState m_orderState;
	static string GetHeader() {  return "Action,OrderId,PositionId,Spread,Original,Bid,Offer,Payup,Aggressor,BidSrc,OfferSrc,Time,Mid,Threshold";}
};
class Strategy;
class Order;
typedef long OrderId;
//messages coming in for example, requesting execution
class IAdapterIn {
public:
#ifdef TWS
	virtual bool CancelOrder(uint64_t orderid) = 0;
	virtual bool ReplaceOrder(OrderDetails& od) = 0;
	virtual bool SendOrder(OrderDetails& od) = 0;
	virtual std::map<int, SymbolInfo> getSymbolTable() = 0;
	virtual std::map<OrderId, OrderDetails> getOrderMap() = 0;
#else
    virtual int CancelOrder(uint64_t orderid)=0;
    virtual void CancelAll(int symbolid)=0;
    virtual void CancelAll()=0;
    //virtual bool ReplaceOrder(OrderDetails& od) { }
    // virtual bool ReplaceOrder(OrderDetails& ) { return true;}
    virtual bool ReplaceOrder(OrderDetails& )=0;
    virtual uint64_t SendOrder(OrderDetails& od) = 0;
    virtual uint64_t SendIOC(OrderDetails& od) = 0;
    virtual const std::unordered_map<uint64_t, Order *>* getActiveOrders() const=0;
    //virtual std::map<int, SymbolInfo> getSymbolTable() { }
    //virtual std::map<int, SymbolInfo> getSymbolTable(){   return std::map<int, SymbolInfo>();}
    //virtual std::map<OrderId, OrderDetails> getOrderMap(){ return std::map<OrderId, OrderDetails>(); }
    virtual std::map<int, SymbolInfo> getSymbolTable()=0;
    virtual std::map<OrderId, OrderDetails> getOrderMap()=0;
    virtual int registerCallback(void(*cb)(int, void*), void* clientd, uint64_t delay, bool reoccuring)=0;
    virtual void cancelCallback(int id)=0;
#endif
};

class Trader;
struct CBData{
    Trader *trader;
    OrderDetails *od;
    Strategy *strat;
};