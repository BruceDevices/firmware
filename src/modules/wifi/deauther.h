#ifndef WIFI_DEAUTHER_H
#define WIFI_DEAUTHER_H

#include "scan_hosts.h"
#include <vector>

void stationDeauth(Host host);
void deauthAll();
void deauthTargetList(const std::vector<Host>& targets);

#endif
