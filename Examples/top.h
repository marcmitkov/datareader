#ifndef INC_TOP_FX_MODEL_H
#define INC_TOP_FX_MODEL_H

#include <commonv2/toptrademodel.h>

class TopFxModel : public TopTradeModel
{
public:
    TopFxModel( HC::Logger & logger, ConfigKeeper & config ) : TopTradeModel( logger, config ) { } ;

    virtual void initialize( ) override ;
    virtual uint32_t getInstrumentEnabledForPilotCount( ) ;
    virtual void onConflationDone( ) override ;
    virtual void tradeTickerUpdate(TRADE_TICKER_STRUCT * tradeTicker, bool isMine ) override ;
    virtual void sourceStatus(SOURCE_LEVEL_STATUS_STRUCT* /*status */) override;
protected:
    inline void handleMarketData( const IBook *book) ;
    inline void handleFtMarketData( const IBook * book ) ;
    virtual Order * createOneOrder( const IBook * book, Instrumentv2 * instrument ) override ;
    void futureTickerUpdate( HC::source_t source, FUTURES_TICKER_ENTRY* ticker ) override;
    virtual void receiveConsoleCommand( CONSOLE_COMMAND_STRUCT *cmd ) override ;
    bool IsConflated(const IBook *book);
    bool IsBookCurrent(const IBook *book);
    void globalPositionUpdate(POSITION_DATA_STRUCT* pPos);
} ;

#endif /* INC_TOP_FX_MODEL_H */

