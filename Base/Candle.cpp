
#include "Candle.h"

Candle::Candle(ptime ts) {
    timeStamp = ts;
    INTH = Decimal(0);
    INTL = Decimal(INT_MAX);

    HBID = Decimal(0);
    LBID = Decimal(INT_MAX);

    HASK = Decimal(0);
    LASK = Decimal(INT_MAX);

}

Candle::Candle(string line) {
    //Maya please implement

    boost::char_separator<char> sep{ ",", "", boost::keep_empty_tokens };
    boost::tokenizer<boost::char_separator<char>> tok(line, sep);
    vector<std::string> result(tok.begin(), tok.end());


    //boost::split(result,line,boost::is_any_of(","));

    timeStamp = time_from_string(result[0]);

    interval = duration_from_string(result[1]);
    SYBL = (std::string) result[2];
    LSTP = Decimal(result[3]);
    LSTS = stoi(result[4]);
    INTF = Decimal(result[5]);
    INTH = Decimal(result[6]);
    INTL = Decimal(result[7]);
    INTV = stoi(result[8]);
    NTRD = stoi(result[9]);
    LSTB = Decimal(result[10]);
    LSBS = stoi(result[11]);
    LSTA = Decimal(result[12]);
    LSAS = stoi(result[13]);
    HBID = Decimal(result[14]);
    LBID = Decimal(result[15]);
    HASK = Decimal(result[16]);
    LASK = Decimal(result[17]);
    HBSZ = stoi(result[18]);
    LBSZ = stoi(result[19]);
    HASZ = stoi(result[20]);
    LASZ = stoi(result[21]);
    HLTS = stoi(result[22]);
    IVAM = stoi(result[23]);

}

string Candle::GetHeader() {
    string retval = "D,T,SYBL,LSTP,LSTS,INTF,INTH,INTL,INTV,NTRD,LSTB,LSBS,LSTA,LSAS,HBID,LBID,HASK,LASK,HBSZ,LBSZ,HASZ,LASZ,HLTS,IVAM";
    return retval;
}

string Candle::GetHeaderEx() {
    //
    string retval = "timestamp,open,high,low,close,amount_G,count_G,weightedPrice_G,amount_P,count_P,weightedPrice_P,lastUpdate";
    return retval;
}
// useful to print in case of quantum candles
string Candle::ToStringEx() const {
    string retval = PosixTimeToStringFormat(timeStamp, "%Y-%m-%d %H:%M:%S");
    retval += string(",") + dec::toString(INTF);
    retval += string(",") + dec::toString(INTH);
    retval += string(",") + dec::toString(INTL);
    retval += string(",") + dec::toString(LSTP);
    retval += string(",") + to_string(_givenVolume);
    retval += string(",") + to_string(_givenCount);
    retval += string(",") + dec::toString(_givenWeightedPrice);
    retval += string(",") + to_string(_paidVolume);
    retval += string(",") + to_string(_paidCount);
    retval += string(",") + dec::toString(_paidWeightedPrice);
    retval += string(",") + to_simple_string(lastUpdate);
    return retval;
}

string Candle::ToString() const {
    string retval = PosixTimeToStringFormat(timeStamp, "%Y-%m-%d %H:%M:%S");
    // string retval = to_simple_string(timeStamp);  //  this produces fractional secs and useful for quantum candles
    retval += string(",") + to_simple_string(interval);
    retval += string(",") + SYBL;
    retval += string(",") + dec::toString(LSTP);
    retval += string(",") + to_string(LSTS);
    retval += string(",") + dec::toString(INTF);
    retval += string(",") + dec::toString(INTH);
    retval += string(",") + dec::toString(INTL);
    retval += string(",") + to_string(INTV);
    retval += string(",") + to_string(NTRD);
    retval += string(",") + dec::toString(LSTB);
    retval += string(",") + to_string(LSBS);
    retval += string(",") + dec::toString(LSTA);
    retval += string(",") + to_string(LSAS);
    retval += string(",") + dec::toString(HBID);
    retval += string(",") + dec::toString(LBID);
    retval += string(",") + dec::toString(HASK);
    retval += string(",") + dec::toString(LASK);
    retval += string(",") + to_string(HBSZ);
    retval += string(",") + to_string(LBSZ);
    retval += string(",") + to_string(HASZ);
    retval += string(",") + to_string(LASZ);
    retval += string(",") + to_string(HLTS);
    retval += string(",") + to_string(IVAM);

    return retval;
}
