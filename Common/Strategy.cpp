#include "Strategy.h"
#include "Synthetic.h"
#include "../Base/IAdapterIn.h"



Strategy::Strategy(Marketable *c, string name, string group, int moniker) {
    _pMarketable = c;
    _name = name;
    _group = group;
    _moniker = moniker;
    _pMarketable->SetStrategy(this);
    _paramsLoaded = true;
}


int Strategy::ToFreq(string freq) {
    int retval =1;
    if (freq == "5m")
        retval = 5;
    else if (freq == "15m")
        retval = 15;
    else if (freq == "1H")
        retval = 60;
    else if (freq == "4H")
        retval = 240;
    else if (freq == "1D")
        retval = 1440;
    return retval;
}

void Strategy::HandleCandles(map<int, Candle *> &candleMap) {
    (void)candleMap;
    cout << "handle multiple candles from synthetic" << endl;
}

bool Strategy::IsFilled(uint64_t orderid, int& original, int& remaining)
{
    //const std::lock_guard<std::mutex> lock(gPositionsMutex);
    bool retval=true;
    original=0;
    remaining =0;
    if(_orderMap.count(orderid) >  0)
    {
        auto od = _orderMap[orderid];
        for(int i=0; i < od->positions.size(); i++) {
            auto p = od->positions[i];
            if(p->mState == Position::Open  || p->mState == Position::Closed)
            {
                original += p->Size;
                retval = retval && true;
            }
            else if(p->mState == Position::OpenPending)
            {
                original += p->Size;
                remaining += p->Remaining;
                retval = retval && false;
            }
            else if(p->mState == Position::ClosedPending)
            {
                original += p->Size;
                remaining += p->CloseRemaining;
                retval = retval && false;
            }
        }
    }
    else
    {
        retval=false;
    }
    return retval;
}

void Strategy::HandleCandle(const Candle &candle) {
    if(!_continuous)
        ExecuteExits(candle.LBID, candle.LASK);  // execute exits at candle interval
}

void Strategy::WritePosition(Position * p)
{
   if(_ofsPositions.is_open())
       _ofsPositions <<  p->ToString()  << endl;
}

void Strategy::WriteOpenPosition(Position * p)
{
    if(_ofsOpenPositions.is_open())
        _ofsOpenPositions <<  p->ToString()  << endl;
}

string Strategy::ComputeStats(int maxexposure) {
    //const std::lock_guard<std::mutex> lock(gPositionsMutex);
    map<EntryCriteria::EntryType, Decimal> pnlByEntryType;
    string retval;
    cout << Name() <<  "," << FrequencyString()  << " found positions # " << _positions.size() << endl;
    retval = Name() ;
    retval +=  ","  + FrequencyString();
    retval += ","  +  to_string(_positions.size());
    int total=0, winners=0, losers=0, missed=0,missedpartial=0,notclosed=0;
    if (_ofsPositions.is_open()) {
        Decimal pnlrealized;

        Decimal slippage;
        for (auto &p : _positions) {
            /*
            if(p->mExitCriteria._expiry.date() < gRoll.date())  // if expired positions were read but roll has happended
            {
                cout << "skipping writing position for Roll date " << to_simple_string(gRoll)  << endl;
                cout << p->ToString() << endl;
                continue;
            }
            */
            if(p->mState == Position::Closed && p->mExitCode != Position::SUPRESSEDSIGNAL) {
                _ofsPositions << p->ToString() << endl;
                pnlrealized += p->PnlRealized;
                slippage += p->Slippage;
                total++;
                (p->PnlRealized >Decimal(0.0))? winners++:losers++;
                if(p->mExitCode == Position::MISS)
                    missed++;
                else if(p->mExitCode == Position::MISSPARTIAL)
                    missedpartial++;


                if( pnlByEntryType.count(p->mEntryCriteria._type) > 0)
                {
                    pnlByEntryType[p->mEntryCriteria._type] += p->PnlRealized;
                }
                else
                {
                    pnlByEntryType[p->mEntryCriteria._type] = p->PnlRealized;
                }
            }
            else if(p->mExitCode == Position::SUPRESSEDSIGNAL)
            {
                // skip
            }
            else if(p->mEntryCriteria._type !=EntryCriteria::OFFSET  && (p->mExitCriteria._expiry.date() <= gRoll.date()))
            {
                notclosed++;
                cout << "not closed?"  << EntryCriteria::getEntryType(p->mEntryCriteria._type) << endl;
                cout << p->ToString() << endl;
            }
        }
        cout << "Realized Pnl  " <<  pnlrealized  << "@ "  << gAsofDate.date()  << endl;
        cout << "winners: " << winners << ", losers: " << losers << ", total: " << total << endl;
        cout << "unfilled: " << _unfilled << ", slippage: " <<  slippage  <<endl;
        cout << "missed:"  << missed << endl;
        cout << "missed partial:"  << missedpartial << endl;
        if(notclosed > 0)
            cerr << "CRITICAL:  what is this? why  not closed"  << "@ "  << gAsofDate.date()  << endl;
    } else {
        cout << "unable to open positions file " << endl;
    }
    retval += "," + to_string(total); //closed
    retval += "," + to_string(winners);
    retval += "," + to_string(losers);
    retval += "," + to_string(missed);
    retval += "," + to_string(missedpartial);
    retval += "," + to_string(notclosed);
    if (_ofsOpenPositions.is_open()) {
        Decimal pnlunrealized;
        int num1=0, num2=0, num3=0;
        for (auto &p : _positions) {
            if(p->mState != Position::Closed) {
                _ofsOpenPositions << p->ToString() << endl;
                pnlunrealized+=p->PnlUnrealized;
                num1++;
                (p->PnlUnrealized >Decimal(0.0))? num2++:num3++;
            }
        }
        cout << "Unrealized Pnl  " <<  pnlunrealized  << "@ "  << gAsofDate.date()  << endl;
        cout << "winners: " << num2 << ", losers: " << num3 << ", total: " << num1 << endl;
    } else {
        cout << "unable to open openpositions file " << endl;
    }
    cout << "********"  << endl;
    retval += ","  +  to_simple_string(gAsofDate);
    retval += ","  + to_string(gParamNumber);
    retval += ","  + to_string(maxexposure);

    Instrument *ins = static_cast<Instrument *>(_pMarketable);
    vector<Candle *> *cdls = ins->GetCandles(_candleQuantum>0?_candleQuantum:_freq);
    long dailyVolume=0;
    for(auto &c:  *cdls)
    {
        dailyVolume+=c->INTV;
    }
    retval += ","  + to_string(dailyVolume);


    for(auto& i: _maxExposureStrategyMap)
    {
        retval += "\n";
        retval +=  EntryCriteria::getEntryType(i.first) +  string(",") +  to_string(i.second);
    }
    if(_ofsSummary.is_open())
        _ofsSummary <<  retval  <<  endl;

    cout << "PnL Map" << endl;
    for(auto& entry : pnlByEntryType)
    {
        cout << EntryCriteria::getEntryType(entry.first) << " => "  << dec::toString(entry.second) << endl;
    }
    // introduce a header
    retval =  "Symbol,Frequency,PosSize,Total,Winners,Losers,Missed,MissPartial,NotClosed,TimeStamp,Param,MaxExp,Volume\n" +  retval;
    return retval;
}

OrderDetails *Strategy::GetOrder(uint64_t orderid) {
    OrderDetails *retval = 0;
    if (_orderMap.count(orderid) > 0)
        retval = _orderMap[orderid];
    return retval;
}


int Strategy::NetExposure(bool longside) {
    //const std::lock_guard<std::mutex> lock(gPositionsMutex);
    int retval = 0;
    for (auto &pos : _positions) {
        if((pos->Size > 0)  != longside)
            continue;  // position  doesn't qualify
        if (!pos->mExitCriteria._expiry.is_not_a_date_time() && pos->mExitCriteria._expiry >= gRoll) {
            if (pos->mState == Position::OpenPending) {
                if (pos->Remaining == INT_MAX)
                    retval += 0; // no fills were received so position did not open
                else
                    retval += (pos->Size > 0) ? pos->Size - pos->Remaining : pos->Size + pos->Remaining;
            } else if (pos->mState == Position::Open || pos->mState == Position::ClosedPending) {
                if (pos->CloseRemaining == INT_MAX)  // no fills received yet
                    retval += pos->Size;
                else
                    retval += pos->Size > 0 ? pos->CloseRemaining : -pos->CloseRemaining;
            } else if (pos->mState == Position::Closed) {
                // not net remaining
            } else {
                cerr << "unusual state for position id " << pos->Id << endl;
            }
        }
    }
    return retval;
}

void Strategy::HandleQuote(Decimal bid, long bidSize, Decimal offer, long offerSize) {

    if(_continuous)  // execute exits at ticks as opposed to at handle candle
        ExecuteExits(bid, offer);
    if (_pMarketable->IsSynthetic()) {
        auto synth = static_cast<Synthetic *>(_pMarketable);
        for (auto &ins: synth->getInstruments()) {
            ins->HandleQuote(bid, bidSize, offer, offerSize);
        }
    } else {
        _pMarketable->HandleQuote(bid, bidSize, offer, offerSize);
    }
}

void Strategy:: WriteOpenPositions(ptime dt){
    string fn = _currentDir + "/openpositions.txt";
    if(!gReducedLogging) {
        cout << "trying to write " << fn << "  " << _openpositions.size() << " OPEN positions " << " @" << to_simple_string(gAsofDate) << endl;
    }
    if (_ofsOpenPositions.is_open())
        _ofsOpenPositions.close();
    _ofsOpenPositions.open(fn, std::ofstream::out);
    for( auto &p : _openpositions)
    {
        if( dt.date() <= p->mExitCriteria._expiry.date()  && p->mState !=  Position::Closed) {
            //TODO make a note of openpending and closed pending positions
            WriteOpenPosition(p);
        }
        else if(p->mState ==  Position::Closed)
        {
            // pending move to closed positions
        }
        else {
            _logA <<  " did not close an expired position? "  << endl;
            _logA << p->ToString() << endl;
        }
    }
    _ofsOpenPositions.close();
}

void Strategy:: WritePositions(){
    string fn = _currentDir + "/positions.txt";
    if(!gReducedLogging) {
        cout << "trying to write " << fn << "  " << _positions.size() << " positions " << " @" << to_simple_string(gAsofDate) << endl;
    }
    if (_ofsPositions.is_open())
        _ofsPositions.close();
    _ofsPositions.open(fn, std::ofstream::out);
    Decimal realized=Decimal(0.0);
    Decimal unrealized=Decimal(0.0);
    for( auto &p : _positions)
    {
        if(p->mExitCode == Position::SUPRESSEDSIGNAL ||  p->mExitCode == Position::MISSEXPOSURE)
            continue;  // maybe just delete these
        if(p->mState == Position::Closed) {
            p->PnlRealized = p->PnL();
            p->PnlUnrealized = 0.0;
            WritePosition(p);
            realized += p->PnlRealized;
        }
        else{
            if(_pMarketable->BestBid.Price > Decimal(0) && _pMarketable->BestAsk.Price > Decimal(0)) {
                p->PnlUnrealized = p->PnLU(p->Size >0?_pMarketable->BestAsk.Price:_pMarketable->BestBid.Price);
                unrealized += p->PnlUnrealized;
            }
        }
    }

    fn = _currentDir + "/pnl.txt";
    if(_ofsPnL.is_open())
        _ofsPnL.close();
    _ofsPnL.open(fn, std::ofstream::app);
    {
        _ofsPnL  <<  PosixTimeToStringFormat(gAsofDate,"%H:%M:%S");
        _ofsPnL  <<  ","  <<  dec::toString(realized);
        _ofsPnL  <<  ","  <<  dec::toString(unrealized)  << endl;
    }
    _ofsPositions.close();
}

void Strategy::HandleTicker(Decimal tradePx, long tradeSz, int side) {

    if (_pMarketable->IsSynthetic()) {
        auto synth = static_cast<Synthetic *>(_pMarketable);

        for (auto &ins: synth->getInstruments()) {
            ins->HandleTrade(tradePx, tradeSz,side);
        }
    } else {
        _pMarketable->HandleTrade(tradePx, tradeSz,side);
    }

}

bool  Strategy::InSession(ptime ts)
{
    bool result;
    if (_sessionStart.is_not_a_date_time() || _sessionEnd.is_not_a_date_time()) {
        return true;
    }
    else
    {
         result = _sessionStart <= ts && _sessionEnd > ts;
    }
    return result;
}

void Strategy::PutOrder(uint64_t id, OrderDetails *pOD) {
    _orderMap[id] = pOD;
}

string Strategy::FrequencyString() {
    string retval = "invalid frequency";
    switch (_moniker) {
        case 1:
            retval = "1m";
            break;
        case 5:
            retval = "5m";
            break;
        case 15:
            retval = "15m";
            break;
        case 60:
            retval = "1H";
            break;
        case 240:
            retval = "4H";
            break;
        case 1440:
            retval = "1D";
            break;
        default:
            retval= to_string(_moniker) + "m";
    }
    return retval;
}

