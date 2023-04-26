//
// Created by lnarayan on 12/8/20.
//
#include "Position.h"
#include "../Base/decimal.h"
#include "../Base/Utilities.h"

int Position::nextId = 0;

const char *entryTypes[EntryCriteria::OFFSET + 1] =
        {"A", "HL", "LH", "HH", "LL", "RHL", "RLH", "RHH", "RLL", "KR", "RKR","CR0","OFFSET"};

string Position::GetHeader() {
    string retval = "Id,TimeStamp,Size,AvgPx,ClosePx,CloseTimeStamp,PnlRealized,PnlUnrealized,State,Remaining,FillPx,FillTimeStamp";
    retval += ",ExitCode,CloseRemaining,CloseFillPx,CloseFillTimeStamp";
    retval += ",";
    retval += EntryCriteria::Headers();
    retval += ",";
    retval += ExitCriteria::Headers();
    return retval;
}

string Position::ToString() const {
    std::string retval = to_string(Id);
    retval += ",";
    retval += to_simple_string(TimeStamp);
    retval += ",";
    retval += to_string(Size);
    retval += ",";
    retval += dec::toString(AvgPx);
    retval += ",";
    retval += dec::toString(ClosePx);
    retval += ",";
    retval += to_simple_string(CloseTimeStamp);
    retval += ",";
    retval += dec::toString(PnlRealized);
    retval += ",";
    retval += dec::toString(PnlUnrealized);
    retval += ",";
    retval += getStateString(mState);
    retval += ",";

    // position  fill related
    retval += to_string(Remaining);
    retval += ",";
    retval += dec::toString(FillPx);
    retval += ",";
    retval += to_simple_string(FillTimeStamp);
    retval += ",";
    retval += getExitCodeString(mExitCode);
    retval += ",";

    //position close fill related
    retval += to_string(CloseRemaining);
    retval += ",";
    retval += dec::toString(CloseFillPx);
    retval += ",";
    retval += to_simple_string(CloseFillTimeStamp);
    retval += ",";

    retval += mEntryCriteria.ToString();
    retval += ",";

    retval += mExitCriteria.ToString();

    return retval;
}

Position::Position(int id, ptime ts, Decimal avgpx, int size) {
    Id = id;
    TimeStamp = ts;
    AvgPx = avgpx;
    Size = size;
    mState = Position::OpenPending;
}

extern ptime gAsofDate;

void Position::ComputeAvgPx(bool first, Decimal closepx) {
    (void) first;
    (void) closepx;
    if (mState == Position::OpenPending || mState == Position::ClosedPending) {

    }

}
bool  Position::IsSignalExpired(vector<Candle* >* cdls, int mins, Decimal mid)
{
    bool retval =false;

    if(Size>0?mid <  AvgPx:mid >  AvgPx)  // price hasn't moved favorably
        retval=true;
    /*
    if(cdls->size() > 0)
    {
        auto current =  cdls->back();
    }
     */
    return retval;
}

Decimal Position::Target() {
    return mExitCriteria._targetlevel;
}

Decimal Position::Stop() {
    return mExitCriteria._stoplevel;
}

void Position::Close(ExitCode ec, Decimal px) {
    if(mState == Open) {
        mExitCode = ec;
        mState = ClosedPending;
        ClosePx = px;
        CloseTimeStamp = gAsofDate;
    }
    else if(mState == OpenPending)  // partially filled position is being closed
    {
        cout <<  "partilly filled position being closed "  << ToString()  << endl;
        mExitCode = ec;
        mState = ClosedPending;
        ClosePx = px;
        CloseTimeStamp = gAsofDate;
    }
    else
    {
        cerr << "errorneously closing " << to_simple_string(gAsofDate)  << endl;
        cerr << ToString() << endl;
    }
}

// compute average price of acquiring
int Position::Offset(Decimal closepx) {
    int retval=0;
    if (mState == Position::OpenPending) {
        retval += ComputeNet();
        Decimal avgfill;
        avgfill = (FillPx * (abs(Size) - ComputeRemaining()) +
                   closepx * Decimal(ComputeRemaining())) /
                  abs(Size);    // assume the rest of the position was acquired at closepx

        if (ClosePx == Decimal(0))  // this position was NOT closed by a subsequent position
        {
            cerr << "CRITICAL: zero close price (open pending) position probably expired before getting filled" << to_simple_string(gAsofDate) << endl;
            cerr << ToString()  << endl;
        }
        ClosePx = closepx;
        mState = Position::Closed;
        mExitCode = Position::CONTRA;
        cout << "closed unfilled/partially filled position: " << Remaining
             << " assuming outstanding order was cancelled " << endl;


        Slippage = Size > 0 ? avgfill - AvgPx : AvgPx - avgfill;
        //mEntryCriteria._description = "original px "  + dec::toString(AvgPx);
        FillPx = avgfill;
        CloseFillPx = closepx;
    } else if (mState == Position::ClosedPending) {
        retval += ComputeNet();
        Decimal avgfill;
        avgfill = (FillPx * (abs(Size) - CloseRemaining) +
                   closepx * Decimal(CloseRemaining)) /
                  abs(Size);    // assume the rest of the position was closed at closepx

        if (ClosePx == Decimal(0))  // this position was NOT closed by a subsequent position
        {
            cerr << "CRITICAL: zero close price " << to_simple_string(gAsofDate) << endl;
        }
        ClosePx = closepx;
        mState = Position::Closed;
        mExitCode = Position::CONTRA;
        cout << "closed unfilled/partially filled position: " << Remaining
             << " assuming outstanding order was cancelled " << endl;


        Slippage = Size > 0 ? avgfill - AvgPx : AvgPx - avgfill;
        //mEntryCriteria._description += "original close px "  + dec::toString(ClosePx);
        CloseFillPx = avgfill;
    } else if (mState == Position::Open) {
        retval += Size;
        mState = Position::Closed;
        mExitCode = Position::CONTRA;
        Slippage = Size > 0 ? FillPx - AvgPx : AvgPx - FillPx;
        //mEntryCriteria._description = "original px "  + dec::toString(AvgPx);
        CloseFillPx = ClosePx = closepx;
        CloseTimeStamp = gAsofDate;
    } else {
        if (mState == Position::Closed) {
            if(gDebug) {
                cout << "closed early  " << Id << (mExitCode == MISS ? " miss" : " filled") << endl;
                cout << getExitCodeString(mExitCode) << endl;
            }
        } else
            cerr << "uncovered position state during offset" << endl;
    }
    return retval;
}

Decimal Position::PnL() {
    Decimal retval;
    int filled = Size > 0 ? Size - Remaining : Size + Remaining;
    retval = Decimal(filled) * (CloseFillPx - FillPx);
    return retval;
}

Decimal Position::PnLU(Decimal markpx) {
    Decimal retval=Decimal(0.0);
    if(mState==ClosedPending) {
       retval= Decimal(CloseRemaining)*(markpx - FillPx)*sgn<int>(Size);
    }
    else if(mState==Open)
    {
        retval= Decimal(Size)*(markpx - FillPx);
    }
    else if(mState==OpenPending)
    {
        int filled = Size > 0 ? Size - Remaining : Size + Remaining;
        retval= Decimal(filled)*(markpx - FillPx);
    }
    return retval;
}

int Position::ComputeNet() {
    int retval = 0;
    if (mState == OpenPending)
        retval = Remaining == INT_MAX ? 0 : Size > 0 ? Size - Remaining : Size + Remaining;
    else if (mState == ClosedPending)
        retval = CloseRemaining == INT_MAX ? Size : Size > 0 ? CloseRemaining : -CloseRemaining;
    else if (mState == Open)
        retval = Size;

    return retval;
}

int Position::ComputeRemaining() {  // for position that is not opened
    int retval;
    retval = abs(Remaining == INT_MAX ? Size : Remaining);
    return retval;
}

#include "../Base/Utilities.h"

void Position::Pnl(bool first, Decimal nextpx  /* next position */, Decimal closepx /* offset */, int positionsize) {
    if (mExitCode == MISS) {
        cout << "no pnl contribution by " << Id << endl;
        return;
    } else if (mState == Closed) {
        PnlRealized = (ClosePx - FillPx) * (abs(Size) - ComputeRemaining()) * sgn(Size);

        cout << "pnl should have been computed at close time" << endl;
    }
    //int filled = Size - ComputeRemaining();
    if (first)  // first position
    {
        PnlRealized = Decimal(Size) * (nextpx - AvgPx);
        PnlUnrealized = Decimal(0.0);
        ClosePx = nextpx;
    } else if (nextpx == Decimal(0.0))  // last position
    {
        PnlRealized = Decimal(Size) * (closepx - AvgPx);
        PnlUnrealized = Decimal(0.0);
        ClosePx = closepx;
    } else {     // TODO: determine Pnl  for other states
        //int positionsize = Size / 2;
        if (positionsize == 0) {
            cerr << "UNEXPECTED position size " << positionsize << " for id " << Id << endl;
        }
        PnlRealized = Decimal(positionsize) * (nextpx - AvgPx);
        PnlUnrealized = Decimal(0.0);
        ClosePx = nextpx;
    }
}


string Position::getExitCodeString(ExitCode val) {
    const char *EnumExitCodeString[] = {"NA", "STOP", "TARGET", "EXPIRE", "EXIT", "HARDSTOP", "MISS", "MISSPARTIAL",
                                        "MISSSIGNAL", "MISSEXPOSURE", "EXITEXPOSURE","MISSCONSECUTIVE", "CONTRA",
                                        "SUPRESSEDSIGNAL", "THROTTLED", "AUTOFILL"};

    // return null if string not available for enum val (out of bounds enum val supplied)

    if (val >= sizeof(EnumExitCodeString) / sizeof(char *)) {
        return 0;
    }
    return EnumExitCodeString[val];

}


string Position::getStateString(State val) {
    const char *EnumStateString[] = {"OpenPending", "Open", "ClosedPending", "Closed", "DeferredClose"};

    // return null if string not available for enum val (out of bounds enum val supplied)

    if (val >= sizeof(EnumStateString) / sizeof(char *)) {
        return 0;
    }
    return EnumStateString[val];

}
template<> char const* enumStrings<Position::State>::data[] = {"OpenPending", "Open", "ClosedPending", "Closed", "DeferredClose"};
template<> char const* enumStrings<Position::ExitCode>::data[] = {"NA", "STOP", "TARGET", "EXPIRE", "EXIT", "HARDSTOP", "MISS", "MISSPARTIAL",
                                                                  "MISSSIGNAL", "MISSEXPOSURE", "EXITEXPOSURE","MISSCONSECUTIVE", "CONTRA",
                                                                  "SUPRESSEDSIGNAL", "THROTTLED", "AUTOFILL"};
template<> char const* enumStrings<EntryCriteria::EntryType>::data[] = {
    "A",
    "HL",
    "LH",
    "HH",
    "LL",
    "RHL",
    "RLH",
    "RHH",
    "RLL",
    "KR",
    "RKR",
    "CR0",
    "OFFSET"
};
template<> char const* enumStrings<EntryCriteria::StrategyName>::data[] = {
        " STRATA",
};

void Position::Read(std::string iss)
{
    try {
        //cout << "Line :" << iss << endl;
        boost::char_separator<char> sep{ ",", "", boost::keep_empty_tokens };
        boost::tokenizer<boost::char_separator<char>> tok(iss, sep);
        /*
        for (const auto &t : tok)
            cout << t << endl;
        */
        vector<string> vec;
        vec.assign(tok.begin(), tok.end());
        /*
        cout << "Vector size is " << vec.size() << endl;
        for (int i = 0; i < vec.size(); i++)
            cout << "vec[" << i << "]: " << vec[i] << endl;
        */
        if (vec.size() != 23)
            throw new exception();
        /* ignore this id
        if (vec[0].length() > 0)
            Id = stoi(vec[0]);
        */
        if (vec[1] != "not-a-date-time")
            TimeStamp = time_from_string(vec[1]);
        cout << to_simple_string(TimeStamp) << endl;
        if (vec[2].length() > 0)
            Size = stoi(vec[2]);
        if (vec[3].length() > 0)
            AvgPx = Decimal(vec[3]);
        if (vec[4].length() > 0)
            ClosePx = Decimal(vec[4]);
        if (vec[5] != "not-a-date-time")
            CloseTimeStamp = time_from_string(vec[5]);
        if (vec[6].length() > 0)
            PnlRealized = Decimal(vec[4]);
        if (vec[7].length() > 0)
            PnlUnrealized = Decimal(vec[5]);
        std::stringstream ssState(vec[8]);
        if (vec[8].length() > 0)
            ssState >> enumFromString( mState);
        if (vec[9].length() > 0)
            Remaining = stoi(vec[9]);
        if (vec[10].length() > 0)
            FillPx = Decimal(vec[10]);
        if (vec[11].length() > 0  &&  vec[11] != "not-a-date-time")
            FillTimeStamp =  time_from_string(vec[11]);
        if (vec[12].length() > 0) {
            std::stringstream ss(vec[12]);
            ss >>  enumFromString(mExitCode);
        }
        if (vec[13].length() > 0)
            CloseRemaining = stoi(vec[13]);
        if (vec[14].length() > 0)
            CloseFillPx = Decimal(vec[14]);
        if (vec[15].length() > 0  &&  vec[15] != "not-a-date-time")
            CloseFillTimeStamp =  time_from_string(vec[15]);

        if (vec[16].length() > 0) {
            std::stringstream ss(vec[16]);
            ss >>  enumFromString(mEntryCriteria._strategyName );
        }
        if (vec[17].length() > 0) {
            std::stringstream ss(vec[17]);
            ss >>  enumFromString(mEntryCriteria._type );
        }
        if (vec[18].length() > 0)
            mEntryCriteria._value = stoi(vec[18]);

        if (vec[19].length() > 0)
            mEntryCriteria._description = vec[19];

        if (vec[20].length() > 0)
            mExitCriteria._stoplevel = Decimal(vec[20]);

        if (vec[21].length() > 0)
            mExitCriteria._targetlevel = Decimal(vec[21]);

        if (vec[22].length() > 0  &&  vec[22] != "not-a-date-time")
            mExitCriteria._expiry =  time_from_string(vec[22]);
    }
    catch (exception& e) {
        cout << "Exception in Position Read : " << e.what() << endl;
    }
    return;
}



/*
EntryCriteria::EntryCriteria(Types type, Decimal value, string description)
{
    _type = type;
    _value = value;
    _description = description;
}
*/
string EntryCriteria::ToString() const {
    const string delim = string(",");
    string retval = getStrategyName(_strategyName);
    retval += delim + getEntryType(_type);
    retval += delim + dec::toString(_value);
    retval += delim + _description;
    return retval;
}

const char *EntryCriteria::getStrategyName(int val) {
    const char *retval;
    switch (val) {
        case STRATA:
            retval = "STRATA";
            break;
        default:
            retval = "UNDEFINED";
            break;

    }

    return retval;

}

const char *EntryCriteria::getEntryType(int val) {

    const char *retval;
    switch (val) {
        case A:
            retval = "A";
            break;
        case OFFSET:
            retval = "OFFSET";
            break;
        case HL:
            retval = "HL";
            break;
        case LH:
            retval = "LH";
            break;
        case HH:
            retval = "HH";
            break;
        case LL:
            retval = "LL";
            break;
        case RHL:
            retval = "RHL";
            break;
        case RLH:
            retval = "RLH";
            break;
        case RHH:
            retval = "RHH";
            break;
        case RLL:
            retval = "RLL";
            break;
        case KR:
            retval = "KR";
            break;
        case RKR:
            retval = "RKR";
            break;
        case CR0:
            retval = "CR0";
            break;
        default:
            retval = "UNDEFINED";
            break;

    }

    return retval;

}

string EntryCriteria::Headers() {
    return "StrategyName,EntryType,Value,Description";
}

ExitCriteria::ExitCriteria() {
    _stoplevel = Decimal(0.0);
    _targetlevel = Decimal(0.0);
}

ExitCriteria::ExitCriteria(ExitCriteria *ec) {
    _stoplevel = ec->_stoplevel;
    _targetlevel = ec->_targetlevel;
    _expiry = ec->_expiry;
}

string ExitCriteria::ToString() const {
    const string delim = string(",");
    string str = dec::toString(_stoplevel);
    str += delim + dec::toString(_targetlevel);
    str += delim + to_simple_string(_expiry);
    return str;
}

string ExitCriteria::Headers() {
    return "StopSize,TargetSize,Expiry";
}
