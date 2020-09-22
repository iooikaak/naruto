//
// Created by 王振奎 on 2020/9/18.
//

#ifndef NARUTO_REPLICATION_H
#define NARUTO_REPLICATION_H


class Replication {
public:
    // master
    // 全局复制偏移量（一个累计值）
    long long master_repl_offset;
    int repl_ping_slave_period;


};


#endif //NARUTO_REPLICATION_H
