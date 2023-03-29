//
// Created by lnarayan on 1/10/21.
//

#ifndef CHANEL_ENUM_H
#define CHANEL_ENUM_H
#include <string>
using namespace std;
class Enum {
public:
    static  const char* getSide(int);
    static  const char* getOrderSendError(int);
    static const char* getTif(int);
    static const char* getOrderType(int);
    static const char* getAction(int val);
    static const char* getOrderStatus(int);

};


#endif //CHANEL_ENUM_H
