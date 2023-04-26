#pragma once
#include "Strategy.h"
#include "../Alglib/interpolation.h"
#include "../Base/IAdapterOut.h"
#include<fstream>
using namespace alglib;
class StrategyA : public Strategy
{
    enum  TrendMonikerType { CURRENT=0, HIGHER=1, OTHER=2};
    Decimal _ticksize;
	int _minlen;
	float _rho;
	int _optimizeFit=0;
	bool _graphSpline=true;
	Decimal _cushion;
	double _payup=0;
    double _spreadthreshold=0;
    int _throttleCount=0;
    int _currentIntervalVolume=0;
    Decimal _toowide=Decimal(3);
    Decimal _stopsize;
    Decimal _targetsize;
    float _h2l2stopx=0.5;
    bool _tradingHalt=false;
    bool _usemid;
    bool _clearscaling=false;
    int _scalefactor=4;
	int _mode;
	int _modeKR=0;
    bool _stopKR=false;
    int _lookbackKR =60;
    int _multiplierKRstop=1;
    int _multiplierKRtarget=1;
    int _trendMonikerKR;
	Decimal _riskFactor;
	vector<double> X, Y;
	vector<int> _changeIndex;
	vector<bool> _vbs;
	int _contracts;
	bool _test; //  pilot
	bool _testBS;
    int _holdingPeriod=0;   // in days
    int _signalExpiry=0;   // in days
    bool _aggressorin=0; //0:passive  1:active
    bool _aggressorout=0; //0:passive  1:active
    bool _skipmissedsignal=true;
    bool _mr=false;
    bool _useRank1=false;
    bool _useRank2=false;
    bool _useSR=false;
    void InitDate(string name);
    void SwitchDate(ptime newdate);
    void OpenLogs();
    string _suffixFutures;
    ptime _currentDate;
    ptime _lastDataLog;
    int _intv=0;
    float _intvMultiplier=10;
    int _netAdjust=0; //  amount by which actual exposure is understated
    int NetExpiry();  // how much is expiring at roll
    Position * TransmitOrder(int qty, double px, bool aggressor, vector<Position *> positions, vector<int> expectedfills,  vector<bool> entryflags, string message);
    int ExecuteExits(Decimal bid, Decimal ask) override;
    Decimal GetLevel(bool bs, int nth, bool passive, bool print=false);
    Decimal GetLevel(const vector<int>&  changeIndex,  const vector<bool>&  vbs, bool bs, int nth, bool passive, bool print=false);
    int ReadCandles(string file);
    int ReadOpenPositions(string file);
    //ofstream _ofsOrders;
    map<EntryCriteria::EntryType,bool> _supressedSignalsMap;
    int _countHL=0;
    int _countLH=0;
    bool IsQualifiedTracker(Position* pos);
    bool IsTradingHalted();
    void ExecuteKR(const vector<bool>& vbs,  const Candle& candle, const vector<Candle *>* cdls, ptime endtime, int mode, int volume);
    void ExecuteKR2(const vector<bool>& vbs,  const Candle& candle, const vector<Candle *>* cdls, ptime endtime, int mode, int volume);
    void StopKR(bool krsignal, const vector<Candle *> *cdls);
    void SendOrderKR(bool krsignal, Decimal tp, const Candle &candle, const vector<Candle *> *cdls, ptime endtime,
                     int mode, int volume);
    void ExecuteCircadian( const Candle& candle, const vector<Candle *>* cdls, ptime endtime);
    void ScalePositions(Position* mainpos, int scalefactor);
    void ExecuteScalePositions(Decimal bid, Decimal ask);

    int  ExpireInterDay(Position* pos);
    void LoopCandles();
    bool IsSupressed(EntryCriteria::EntryType value);
    void PrintLevels(int start, vector<int>&  changeIndex,  vector<bool>&  vbs, vector<Candle *> *cdls, vector<double>& vs);

    vector<vector<double>> _previousSplines;
    vector<ptime> _previousDates;
    vector<Candle *> _krCandles;

    bool _classifyTP=false;
    bool _computeTP=false;

    int GetTrendMoniker(TrendMonikerType trend);
    void GenerateChangePoints(int start, ptime endTime, vector<Candle *>* cdls,  int boundaryAdjustment,  vector<bool>& vbs, vector<int>& currentChangeIndex, vector<double>& vs, vector<double>& vds, vector<double>& vd2s);
    void ClassifyTPs(ptime endTime, const vector<Candle *>* cdls,  const vector<int>& currentChangeIndex, const vector<bool>& vbs);

public:
	StrategyA(Marketable*, string name, string group, int freq);
	virtual ~StrategyA() {}
    virtual bool IsTradeable() override;
    virtual Position* Liquidate(int size=0) override;
    virtual Position* ExpireEx() override;
    virtual void RunCandles() override;
	virtual void HandleCandle(const Candle& candle);
	int GetZerosEx(ptime cdlts, real_1d_array x, real_1d_array y, int start, vector<int>& vindex,
		vector<bool>& vbs, vector<double>& vs,  vector<double>& vds,  vector<double>& vd2s);
	int Derivatives(real_1d_array x, real_1d_array y, double rho, ae_int_t m, vector<double>& vs,
		vector<double>& vds, vector<double>& vd2s);
	int findChangePointsEx(vector<double> v, vector<double> vd, int start, vector<int>& vindex, vector<bool>& vbs);
	bool UpdateChangePointsV7(const vector<bool> vbs, const vector<int> currentChangeIndex);
    void GraphEx(ptime date,  const vector<double>& vs);
    void Test();
    virtual void NotifyExposure(Position *, int, int) override ;
    bool initTradingHalt(string n);
    ptime ComputeExpiry();
    virtual int GetLastLevel(bool mr) override;
    virtual string GetParam(string) override;
    virtual int ReduceOpenPositions(int numop) override;
    vector<Position *> ReduceOpenPositionsEx(int numop) override;
    int CollapseExpired(vector<Position *>& collapsewith) override;
    //virtual int ReduceOpenPositionsEx(int numop) override;
    int Net(bool mismatch=false) override;  // get net exposue
    int CloseOpenPositions() override;
};