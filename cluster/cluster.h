//
// Created by kwins on 2020/12/8.
//

#ifndef NARUTO_CLUSTER_H
#define NARUTO_CLUSTER_H

#include "cluster_view.h"

namespace naruto::cluster {

class Cluster {
public:
    void initializer();
    void onClusterCron(ev::timer& watcher, int event);
    clusterView view;
    int iteration{};
};

extern std::shared_ptr<Cluster> clsptr;

}

#endif //NARUTO_CLUSTER_H
