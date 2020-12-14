//
// Created by kwins on 2020/12/8.
//

#ifndef NARUTO_CLUSTER_LINK_H
#define NARUTO_CLUSTER_LINK_H

#include "link/client_link.h"
namespace naruto::cluster{

class clusterLink : public link::clientLink{
public:
    explicit clusterLink(connection::ConnectOptions opts);
    explicit clusterLink(int sd);
    void regReadEvent();
    void onRead(ev::io &watcher, int events) override;
    void onWrite(ev::io &watcher, int events) override;
    void close() const override;
};

}



#endif //NARUTO_CLUSTER_LINK_H
