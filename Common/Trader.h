#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include "../Base/Product.h"
#include "../Base/NamedParams.h"
#include "../Base/IAdapterOut.h"
#include "../Base/Candle.h"
class Marketable;
class IAdapterIn;
class Strategy;
class OrderDetails;
class Order;
class CBData;
class Trader : public IAdapterOut
{
public:
	Trader(IAdapterIn *);
	// IAdapterOut interface
	virtual void HandleQuotePx(int symbolid, unsigned char side, Decimal rate);
	virtual void HandleQuoteSz(int symbolid, unsigned char side, int64_t size);
	virtual void HandleTradePx(int symbolid, Decimal rate);
	virtual void HandleTradeSz(int symbolid, int64_t size);
	virtual void HandleVolume(int symbolid, int64_t size);
	virtual	void HandleOrderEvent(int symbolid, uint64_t orderid, int eventtype) override;
	virtual	void HandleFillEvent(int symbolid, uint64_t orderid, int action, int filled, int totalfilled, int remaining, double price) override;	// end
	void HandleCandle(string ekey, Candle cdl);
	vector<Product *> Products();

	void RunStrategy();

    virtual Strategy *GetStrategy(int id, int moniker);
    virtual void AddExposure(int id, int exp) override;

	// enhanced adapter
	virtual void InitProducts();
	void AddProduct(int symbolid, SymbolInfo);
	virtual void HandleQuote(int symbolid, Decimal bid, long bidSize, Decimal offer, long offerSize);
	virtual void HandleTicker(int symbolid, Decimal tradePx, long tradeSz, int side);

	void AddProducts(string name, vector<int> ids, vector<string> p, vector<string> exchange);

    uint64_t SendOrder(OrderDetails* od, Strategy *strat) override;
    const std::unordered_map<uint64_t, Order *>* GetActiveOrders() const;
    void CancelAll() override;
    int CancelOrder(uint64_t orderid) override;

    virtual int GetSymbolId(string esh1) override;
    virtual vector<OrderDetails *> GetOrderDetailsList() override;
    virtual void EOD(ptime) override;
    virtual void Expire() override;
    virtual string Summary() override;

    virtual void Mute(int symbolid, int frequency) override;

    virtual string GetSymbol(int id) override;
    virtual int GetCurrentExposure(int id,  int adjustment=0 /* used during expiry */) override;
    virtual uint64_t SendOrderEx(OrderDetails *od, Strategy *strat) override;

    virtual void PositionUpdate(uint64_t id,  double pos) override;

    map<int, vector<Strategy*>>& getRouter()  { return _router; };
    void CheckIOCFilled(CBData * data);
protected:
	vector<Product *> _products;
	map<string, vector<Marketable *>> _map;
    ptime _asofdate;
	map<string, NamedParams>  m_params;

	// enhanced adapter
	IAdapterIn *m_pAdapter;
	map<int, SymbolInfo> _hcMap;
	map<int, vector<Strategy*>> _router;
	map< uint64_t, Strategy*>  _orderMap;
	map<string, double> _positionMap;
	OrderDetails * GetOrder(uint64_t orderid);
    Strategy *GetStrategy(uint64_t orderid);
    ofstream _ofsOrders;
    map<int, int> _maxExposureMap;
    map<int, int> _currentExposureMap;
    void HandleQuoteCrossAsset(int symbolid, Decimal bid, long bidSize, Decimal offer, long offerSize);
    void HandleTickerCrossAsset(int symbolid, Decimal tradePx, long tradeSz, int side);
    virtual void Liquidate(int symbolid, float ratio ) override;
    virtual void LiquidateAll(float ratio ) override;
    //dynamically change max exposure
    void SetMaxExposureAll(int me) override;
    void SetMaxExposure(int symbolid, int me) override;
};
