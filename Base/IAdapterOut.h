#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "../Base/decimal.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include "../Base/SymbolInfo.h"
using namespace std;
using namespace boost::posix_time;

class OrderDetails;
class Strategy;
class Order;
//messages going out from feed/execution
class IAdapterOut {
protected:
	// marketdata related
	virtual void HandleQuotePx(int symbolid, unsigned char side, Decimal rate)=0;
	virtual void HandleQuoteSz(int symbolid, unsigned char side, int64_t size) = 0;
	virtual void HandleTradePx(int symbolid, Decimal rate)=0;
	virtual void HandleTradeSz(int symbolid, int64_t size)=0;
	virtual void HandleVolume(int symbolid, int64_t size) = 0;

public:

	// enhanced adapter
	virtual void InitProducts() = 0;
	virtual void AddProduct(int symbolid, SymbolInfo si) = 0;
    virtual void HandleQuote(int symbolid, Decimal bid, long bidSize, Decimal offer, long offerSize)=0;
    virtual void HandleTicker(int symbolid, Decimal tradePx, long tradeSz,  int side)=0;
    virtual OrderDetails * GetOrder(uint64_t orderid)=0;

    // really to forward to IAdapterIn
    virtual uint64_t SendOrder(OrderDetails* od, Strategy *strat)=0;
    virtual uint64_t SendOrderEx(OrderDetails *od, Strategy *strat)=0;
    virtual const std::unordered_map<uint64_t, Order *>* GetActiveOrders() const=0;
    virtual void CancelAll()=0;
    virtual int CancelOrder(uint64_t orderid)=0;
    virtual void PositionUpdate(uint64_t id,  double pos)=0;

    // order related
    virtual	void HandleOrderEvent(int insid, uint64_t orderid, int eventtype)=0;
    virtual	void HandleFillEvent(int insid, uint64_t orderid, int action, int filled, int totalfilled, int remaining, double price)=0;
    virtual int GetSymbolId(string esh1)=0;
    virtual string GetSymbol(int id)=0;
    virtual void EOD(ptime) =0;
    virtual void Expire() =0;
    virtual vector<OrderDetails *> GetOrderDetailsList()=0;
    virtual string Summary()=0;
    virtual void Mute(int symbolid, int frequency)=0;
    virtual int GetCurrentExposure(int id, int adjustment=0 /* used during expiry */)=0;
    virtual void AddExposure(int id, int exp)=0;
    virtual Strategy *GetStrategy(int id, int moniker)=0;
    virtual void Liquidate(int symbolid, float ratio )=0;
    virtual void LiquidateAll(float ratio )=0;
    virtual void SetMaxExposureAll(int me)=0;
    virtual void SetMaxExposure(int symbolid, int me)=0;
};

