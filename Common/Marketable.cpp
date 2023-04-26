#include "Marketable.h"
bool Marketable::InSession(ptime ts)
{

	bool result = _sessionStart <= ts && _sessionEnd >= ts;
	return result;
}

void Marketable::CandleUpdate(int id, Candle*, int)
{
(void) id;

}