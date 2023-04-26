#pragma once
#include "Marketable.h"
#include "../Base/Position.h"
using namespace std;
class OrderDetails;
class Strategy
{
public:
	Strategy() {}
	Strategy(Marketable *, string name, string group, int freq);
	int Frequency() { return _freq; }
	string Name()   {return _name;}
    virtual void HandleQuote(Decimal bid, long bidSize, Decimal offer, long offerSize);
    void HandleTicker(Decimal tradePx, long tradeSz, int side);
	virtual void HandleCandles(map<int, Candle*>& candleMap);
	virtual void HandleCandle(const Candle& candle);
	bool IsInitialized() {  return _paramsLoaded;}
    string FrequencyString();
	OrderDetails* GetOrder(uint64_t orderid);
	void PutOrder(uint64_t, OrderDetails*);
	string ComputeStats(int maxexposure);
    virtual Position* Liquidate(int size=0)=0;
    virtual Position* ExpireEx()=0;
	void ResetPositionsCounter() { _numPositions =0; }
    map< uint64_t, OrderDetails*>& GetOrderMap() { return _orderMap; }
    bool  InSession(ptime);
    static int ToFreq(string freq);
    ofstream& GetLog() { return _logA;}
    vector<Position *>& OpenPositions() { return _openpositions; }
    virtual void NotifyExposure(Position *, int, int) {};
    bool IsFilled(uint64_t orderid, int& original, int& remaining);
    void DisableSignal() {_disableSignal =true; }
    void WritePosition(Position * p);
    void WriteOpenPosition(Position * p);
    static ptime sHalt1;  // non DST CME halt1  (applies only to equity indices)
    static ptime sResume1;
    void WriteOpenPositions(ptime);
    void WritePositions();
    virtual void RunCandles()=0;
    virtual int GetLastLevel(bool mr)=0;
    int Moniker() { return _moniker;};
    virtual bool IsTradeable() =0;
    virtual string GetParam(string)=0;
    virtual int ReduceOpenPositions(int numop)=0;
    virtual vector<Position *> ReduceOpenPositionsEx(int numop)=0;
    virtual  int CollapseExpired(vector<Position *>& collapsewith)=0;
    virtual int Net(bool mismatch=false)=0;
    virtual int CloseOpenPositions()=0;
    void SetMaxExposure(int e) { _maxExposure=e;}

protected:
	string _name;
	string _group;
	int _moniker;
    int _freq;
	int _scale=0;
    int _scalePositions=0;
    Decimal _lastHL=Decimal(0);
    Decimal _lastLH=Decimal(0);
    Decimal _highestHL=Decimal(0);
    Decimal _lowestLH=Decimal(INT_MAX);
    int _scaleMultiplier=1;
	Marketable *_pMarketable;
    bool _paramsLoaded;
    int _numPositions=0;
	map< uint64_t, OrderDetails*>  _orderMap;
    ofstream _ofsCandles;
    ofstream _ofsPositions;
    ofstream _ofsOpenPositions;
    ofstream _ofsExecutions;
    string _currentDir;
    vector<Position *> _positions;
    vector<Position *> _openpositions;  // structure purpose of executing stops / target efficiently
    vector<Position *> _scalepositionslmt;  // limit entry
    vector<Position *> _scalepositionsstp;  // stop entry
    ptime _sessionStart;
    ptime _sessionEnd;
    int _unfilled=0;
    ofstream _logA;
    bool _continuous=true;
    bool _tradeable=false;
    bool _aggressoradd=false;
    bool _aggressorfade=false;
    int _candleQuantum=0;
     //virtual int ExecuteExits(Decimal bid, Decimal ask) { return 0;}
    virtual int ExecuteExits(Decimal , Decimal ) { return 0;}

    ofstream _ofsSummary;
    ofstream _ofsPnL;
    ofstream _ofsSpline;
    map<int, int> _maxExposureStrategyMap;
    map<int, int> _currentExposureStrategyMap;
    bool _disableSignal=false;

    int NetExposure(bool longside);

    bool _circadianStart=false;
    bool _circadianEnd=false;
	bool _circadianRun=false;
    int _maxExposure=INT_MAX;

};
