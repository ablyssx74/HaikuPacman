#ifndef STUB_NOTIFICATION_H
#define STUB_NOTIFICATION_H
#include "String.h"
enum notification_type { B_INFORMATION_NOTIFICATION };
class BNotification {
public:
    BNotification(notification_type) {}
    void SetGroup(const char*) {}
    void SetTitle(const char*) {}
    void SetContent(const BString&) {}
    int Send() { return 0; }
};
#endif
