//
// Created by lnarayan on 10/31/20.
//

#ifndef CHANEL_ADAPTERIN_H
#define CHANEL_ADAPTERIN_H

#include "../Base/IAdapterIn.h"
#include "Base/SymbolInfo.h"
#include <unordered_map>
#include <msg/message_struct.h>
class TopModelAndBookFactory;
class Order;
class AdapterIn : public IAdapterIn
{
public:
    AdapterIn(TopModelAndBookFactory  *);
    int CancelOrder(uint64_t orderid) override;
    virtual void CancelAll(int symbolid) override;
    virtual void CancelAll() override;
    bool ReplaceOrder(OrderDetails& od) override;
    uint64_t SendOrder(OrderDetails& od) override;
    uint64_t SendIOC(OrderDetails& od) override;
    std::map<int, SymbolInfo> getSymbolTable() override;
    std::map<OrderId, OrderDetails> getOrderMap() override;
    const std::unordered_map<HC::orderid_t, Order *>* getActiveOrders() const override;
    void registerCallback(void(*cb)(int, void*), void* clientd, uint64_t delay, bool reoccuring) override;
private:
    TopModelAndBookFactory  *m_pFactory;
};


#endif //CHANEL_ADAPTERIN_H
