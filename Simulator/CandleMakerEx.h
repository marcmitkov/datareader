#pragma once
#include "Candle.h"
#include "Product.h"
#include "CandleMaker.h"

class CandleMakerEx :public ICandleMaker{
public:
    CandleMakerEx(int quantum, Product *p);
    Candle *HandleTrade(Decimal tradePx, long tradeSz, int side) override;  //trade based candles
    Candle* HandleQuote(Decimal bid, long bidSize, Decimal offer, long offerSize) override;
    Candle *GetCurrent() override;
protected:
    long _quantum=1000;
};



