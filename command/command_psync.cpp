//
// Created by 王振奎 on 2020/11/1.
//

#include "command_psync.h"

#include "client.h"
#include "utils/pack.h"
#include "utils/file.h"
#include "replication.h"
#include "protocol/replication.pb.h"

DECLARE_string(repl_dir);
DECLARE_string(repl_database_filename);
DECLARE_double(repl_aof_interval);

void naruto::command::CommandPsync::exec(naruto::narutoClient *client) {
    replication::command_psync psync;
    auto type = utils::Pack::deSerialize(client->rbuf, psync);

    const auto& rc = replica->getReplConf();

    replication::command_psync_reply reply;
    reply.set_errcode(0);
    reply.set_run_id(rc.run_id);

    int sync;
    if (psync.run_id() == "?" || psync.repl_aof_file_name().empty()){
        std::string dbname = FLAGS_repl_dir + "/" + FLAGS_repl_database_filename;
        int64_t size = utils::File::size(dbname);
        if (size == -1){
            sync = 1;
        }else{
            sync = 2;
        }
    } else {
        std::string aofname = FLAGS_repl_dir + "/" + psync.repl_aof_file_name();
        int64_t size = utils::File::size(aofname);
        LOG(INFO) <<"aofname=" << aofname << " size--->>" << size;
        if (size == -1 || size < psync.repl_aof_off()){
            sync = 2;
        }else{
            sync = 3;
        }
    }
    LOG(INFO) << "CommandPsync sync=" << sync << "\n" << psync.DebugString();

    switch (sync) {
        case 1: // 还没有生成DB，使用第一个aof文件
        {
            reply.set_psync_type(replication::PARTSYNC);
            client->repl_state = replState::CONNECTED;
            client->repl_aof_off = 0;
            client->repl_aof_file_name = "naruto.aof.0";
            reply.set_repl_aof_off(client->repl_aof_off);
            reply.set_repl_aof_file_name(client->repl_aof_file_name);

            client->repl_tio = std::make_shared<ev::timer>();
            client->repl_tio->set<narutoClient, &narutoClient::onSendIncrToSlave>(client);
            client->repl_tio->set(ev::get_default_loop());
            client->repl_tio->start(FLAGS_repl_aof_interval, FLAGS_repl_aof_interval);
            break;
        }
        case 2: // 全量同步
        {
            if (rc.db_dump_aof_name.empty()){
                reply.set_psync_type(replication::TRYSYNC);
                client->repl_state = replState::WAIT_BGSAVE;
                // 触发一次bgsave
                replica->bgsave();
            }else{
                std::string dbname = FLAGS_repl_dir + "/" + FLAGS_repl_database_filename;
                int64_t size = utils::File::size(dbname);
                reply.set_psync_type(replication::FULLSYNC);
                reply.set_repl_database_size(size);
                reply.set_repl_aof_file_name(rc.db_dump_aof_name);
                reply.set_repl_aof_off(rc.db_dump_aof_off);
                client->repl_state = replState::TRANSFOR;
                client->repl_dbsize = size;
                client->repl_dboff = 0;
                client->repl_db_f = std::make_shared<std::ifstream>(dbname, std::ios::in);
                client->repl_aof_file_name = rc.db_dump_aof_name;
                client->repl_aof_off = rc.db_dump_aof_off;

                auto* w = new ev::io;
                w->set<narutoClient, &narutoClient::onSendBulkToSlave>(client);
                w->set(ev::get_default_loop());
                w->start(client->connect->fd(), ev::WRITE);
            }
            break;
        }
        case 3: // 部分同步
        {
            reply.set_psync_type(replication::PARTSYNC);
            client->repl_state = replState::CONNECTED;
            client->repl_aof_file_name = psync.repl_aof_file_name();
            client->repl_aof_off = psync.repl_aof_off();
            reply.set_repl_aof_file_name(client->repl_aof_file_name);
            reply.set_repl_aof_off(client->repl_aof_off);

            client->repl_tio = std::make_shared<ev::timer>();
            client->repl_tio->set<narutoClient, &narutoClient::onSendIncrToSlave>(client);
            client->repl_tio->set(ev::get_default_loop());
            client->repl_tio->start(FLAGS_repl_aof_interval, FLAGS_repl_aof_interval);
            break;
        }
        default:
        {
            LOG(WARNING) << "Unsupport sync type " << type;
        }
    }

    client->flag |= (unsigned) narutoClient::flags::SLAVE;
    client->write(reply, type);
}
