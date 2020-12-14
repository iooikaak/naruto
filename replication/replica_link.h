//
// Created by kwins on 2020/12/8.
//

#ifndef NARUTO_REPLICA_LINK_H
#define NARUTO_REPLICA_LINK_H

#include "link/client_link.h"
#include "replica_state.h"

namespace naruto::replica{

class replicaLink : public link::clientLink {

public:
    enum class flags : unsigned{
        NONE = 0,
        SLAVE = 1 << 1,
        MASTER = 1 << 2
    };

public:
    explicit replicaLink(connection::ConnectOptions opts);
    explicit replicaLink(int sd);

    void onRead(ev::io& watcher, int events) override;
    void onWrite(ev::io& watcher, int events) override;
    void close() const override;

    void onSendBulkToSlave(ev::io&, int);
    void onSendIncrToSlave(ev::timer&, int);

    std::shared_ptr<ev::io> repl_rio; // slave 接收 master 增量复制io
    std::shared_ptr<ev::timer> repl_tio; // master 定时发送增量数据

    // 主服务器首次同步DB数据给从服务器时使用
    off_t repl_dboff;
    off_t repl_dbsize;
    std::shared_ptr<std::ifstream> repl_db_f;

    // 主服务器记录的从服务器同步状态
    // 从服务器状态
    replState repl_state;

    // 主服务器 run_id
    std::string repl_run_id;

    // 同步aof文件名
    std::string repl_aof_file_name;

    // 同步aof位置
    int64_t repl_aof_off;
};

}



#endif //NARUTO_REPLICA_LINK_H
