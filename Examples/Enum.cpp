//
// Created by lnarayan on 1/10/21.
//

#include "Enum.h"
#include <msg/message_enum.h>
#include <msg/protocol.hpp>
#include <framework/BaseOrder.h>

const char *Enum::getSide(int val)
{
    const char * retval;
    switch(val)
    {
        //TODO: What about  BID_OLD = 9, // Bid will be replaced or cancelled
        //   and  OFFER_OLD = 10,
        case HC_GEN::SIDE::BID:
            retval = "BID";
            break;
        case HC_GEN::SIDE::OFFER:
            retval = "OFFER";
            break;
        case HC_GEN::SIDE::TRADE:
            retval = "TRADE";
            break;
        case HC_GEN::SIDE::AGGRESS_BID:
            retval = "AGGRESS_BID";
            break;
        case HC_GEN::SIDE::AGGRESS_OFFER:
            retval = "AGGRESS_OFFER";
            break;
        case HC_GEN::SIDE::AGGRESS_BID_GTC:
            retval = "AGGRESS_BID_GTC";
            break;
        case HC_GEN::SIDE::AGGRESS_OFFER_GTC:
            retval = "AGGRESS_OFFER_GTC";
            break;
        case HC_GEN::SIDE::FX_CANCEL:
            retval = "FX_CANCEL";
            break;
        case HC_GEN::SIDE::CLEAR:
            retval = "CLEAR";
            break;
        default:
            retval = "UNDEFINED";
            break;
    }

    return retval;
}
const char*Enum:: getOrderType(int val)
{
    const char * retval;
    switch(val)
    {
        case HC::FIX_ORDER_TYPE::ICEBERG:
            retval = "ICEBERG";
            break;
        case HC::FIX_ORDER_TYPE::MARKET:
            retval = "MARKET";
            break;
        case HC::FIX_ORDER_TYPE::LIMIT:
            retval = "LIMIT";
            break;
        case HC::FIX_ORDER_TYPE::STOP:
            retval = "STOP";
            break;
        case HC::FIX_ORDER_TYPE::STOP_LIMIT:
            retval = "STOP_LIMIT";
            break;
        case HC::FIX_ORDER_TYPE::LIMIT_LOCAL:
            retval = "LIMIT_LOCAL";
            break;
        case HC::FIX_ORDER_TYPE::KEEP_WARM:
            retval = "KEEP_WARM";
            break;
        case HC::FIX_ORDER_TYPE::SWAP:
            retval = "SWAP";
            break;
        default:
            retval = "UNDEFINED";
            break;
    }

    return retval;

}
const char*Enum:: getTif(int val)
{
    const char * retval;
    switch(val)
    {
        case HC::FIX_TIME_IN_FORCE::DAY:
            retval = "DAY";
            break;
        case HC::FIX_TIME_IN_FORCE::GTC:
            retval = "GTC";
            break;
        case HC::FIX_TIME_IN_FORCE::FOK:
            retval = "FOK";
            break;
        case HC::FIX_TIME_IN_FORCE::IOC:
            retval = "IOC";
            break;
        default:
            retval = "UNDEFINED";
            break;
    }

    return retval;

}

const char* Enum::getAction(int val)
{
    const char * retval;
    switch(val) {
        case HC_GEN::ACTION::BUY:
            retval = "BUY";
            break;
        case HC_GEN::ACTION::SELL:
            retval = "SELL";
            break;
        default:
            retval = "UNDEFINED";
            break;

    }

    return retval;


}
const char* Enum::getOrderSendError(int val)
{
    const char * retval;
    switch(val)
    {
        case ORDER_SEND_ERROR::SUCCESS:
            retval = "SUCCESS";
            break;
        case ORDER_SEND_ERROR::GLOBAL_TRADING_DISABLED:
            retval = "GLOBAL_TRADING_DISABLED";
            break;
        case ORDER_SEND_ERROR::NODE_DISABLED:
            retval = "NODE_DISABLED";
            break;
        case ORDER_SEND_ERROR::INSTRUMENT_DISABLED:
            retval = "INSTRUMENT_DISABLED";
            break;
        case ORDER_SEND_ERROR::INSTRUMENT_FOR_MODEL_DISABLED:
            retval = "INSTRUMENT_FOR_MODEL_DISABLED";
            break;
        case ORDER_SEND_ERROR::ECN_DISABLED:
            retval = "ECN_DISABLED";
            break;
        case ORDER_SEND_ERROR::SELL_SIDE_DISABLED:
            retval = "SELL_SIDE_DISABLED";
            break;
        case ORDER_SEND_ERROR::BUY_SIDE_DISABLED:
            retval = "BUY_SIDE_DISABLED";
            break;
        case ORDER_SEND_ERROR::MODEL_KEY_DISABLED:
            retval = "MODEL_KEY_DISABLED";
            break;
        case ORDER_SEND_ERROR::MODEL_NOT_REGISTERED_FOR_ITEM:
            retval = "MODEL_NOT_REGISTERED_FOR_ITEM";
            break;
        case ORDER_SEND_ERROR::ORDER_SIZE_TOO_LARGE:
            retval = "ORDER_SIZE_TOO_LARGE";
            break;
        case ORDER_SEND_ERROR::OVER_ORDER_LIMIT:
            retval = "OVER_ORDER_LIMIT";
            break;
        case ORDER_SEND_ERROR::OVER_MAX_ORDER_LIMIT:
            retval = "OVER_MAX_ORDER_LIMIT";
            break;
        case ORDER_SEND_ERROR::SELL_MAX_POSITION_DISABLED:
            retval = "SELL_MAX_POSITION_DISABLED";
            break;
        case ORDER_SEND_ERROR::BUY_MAX_POSITION_DISABLED:
            retval = "BUY_MAX_POSITION_DISABLED";
            break;

        case ORDER_SEND_ERROR::ALREADY_PENDING_CANCEL :
            retval = "ALREADY_PENDING_CANCEL";
            break;
        case ORDER_SEND_ERROR::OTHER:
            retval = "OTHER";
            break;
        case ORDER_SEND_ERROR::UNKNOWN_ERROR:
            retval = "UNKNOWN_ERROR";
            break;
        case ORDER_SEND_ERROR::NO_AVAILABLE_ECN:
            retval = "NO_AVAILABLE_ECN";
            break;
        case ORDER_SEND_ERROR::ORDER_DOES_NOT_EXIST:
            retval = "ORDER_DOES_NOT_EXIST";
            break;
        case ORDER_SEND_ERROR::NOT_INITIALIZED:
            retval = "NOT_INITIALIZED";
            break;
        case ORDER_SEND_ERROR::NO_FREE_TRIGGERS:
            retval = "NO_FREE_TRIGGERS";
            break;
        case ORDER_SEND_ERROR::NOT_VALID_STATE:
            retval = "NOT_VALID_STATE";
            break;
        default:
            retval = "UNDEFINED";
            break;
    }

    return retval;
}

const char* Enum::getOrderStatus(int val){

    const char * retval;
    switch(val)
    {
        case ORDER_STATUS::NONE:
            retval = "NONE";
            break;
        case ORDER_STATUS::NEW:
            retval = "NEW";
            break;
        case ORDER_STATUS::ADD_PENDING:
            retval = "ADD_PENDING";
            break;
        case ORDER_STATUS::REJECTED:
            retval = "REJECTED";
            break;
        case ORDER_STATUS::ACCEPTED:
            retval = "ACCEPTED";
            break;
        case ORDER_STATUS::PENDING_PARTIALLY_FILLED:
            retval = "PENDING_PARTIALLY_FILLED";
            break;
        case ORDER_STATUS::PARTIALLY_FILLED:
            retval = "PARTIALLY_FILLED";
            break;
        case ORDER_STATUS::LEG_FILL:
            retval = "LEG_FILL";
            break;
        case ORDER_STATUS::CANCEL_PENDING:
            retval = "CANCEL_PENDING";
            break;
        case ORDER_STATUS::CANCEL_REJECTED:
            retval = "CANCEL_REJECTED";
            break;
        case ORDER_STATUS::REPLACE_PENDING:
            retval = "REPLACE_PENDING";
            break;
        case ORDER_STATUS::REPLACE_REJECTED:
            retval = "REPLACE_REJECTED";
            break;
        case ORDER_STATUS::EXPIRED:
            retval = "EXPIRED";
            break;
        case ORDER_STATUS::CANCELLED:
            retval = "CANCELLED";
            break;
        case ORDER_STATUS::HANGING:
            retval = "HANGING";
            break;

        default:
            retval = "UNDEFINED";
            break;
    }

    return retval;

}

/*const char* EnumStringOrderSendResult[] = { "SUCCESS","GLOBAL_TRADING_DISABLED","NODE_DISABLED","INSTRUMENT_DISABLED","INSTRUMENT_FOR_MODEL_DISABLED","ECN_DISABLED","SELL_SIDE_DISABLED","BUY_SIDE_DISABLED",
                                            "MODEL_KEY_DISABLED","MODEL_NOT_REGISTERED_FOR_ITEM", "ORDER_SIZE_TOO_LARGE","OVER_ORDER_LIMIT","OVER_MAX_ORDER_LIMIT","SELL_MAX_POSITION_DISABLED",
                                            "BUY_MAX_POSITION_DISABLED","ALREADY_PENDING_CANCEL","OTHER", "UNKNOWN_ERROR","NO_AVAILABLE_ECN","ORDER_DOES_NOT_EXIST","NOT_INITIALIZED","NO_FREE_TRIGGERS",
                                            "NOT_VALID_STATE" };
// return null if string not available for enum val (out of bounds enum val supplied)
const char* Enum::getOrderSendError(int val)
{
    if(val >= sizeof(EnumStringOrderSendResult)/sizeof(char *))
    {
        return 0;
    }
    return EnumStringOrderSendResult[val];
}*/