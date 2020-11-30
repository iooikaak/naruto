//
// Created by kwins on 2020/11/27.
//

#include "kunai.h"
#include "utils/pack.h"
#include "protocol/replication.pb.h"
#include "protocol/command_types.pb.h"

naruto::kunai::Kunai::Kunai(const naruto::kunai::Options &opts) {
    pool_ = std::make_shared<connection::ConnectionPool>(opts.poolops, opts.connops);
}

void naruto::kunai::Kunai::execute(google::protobuf::Message &msg,
                              int type, google::protobuf::Message &reply) {
    auto conn = pool_->fetch();
    utils::Bytes pack;
    utils::Pack::serialize(msg, type, pack);
    conn.send(pack);

    pack.clear();

    conn.recv(pack);
    utils::Pack::deSerialize(pack, reply);
    pool_->release(std::move(conn));
}

client::command_hget_reply
naruto::kunai::Kunai::hget(const std::string &key, std::initializer_list<std::string> fileds) {
    auto conn = pool_->fetch();
    client::command_hget hget;
    hget.set_key(key);
    for(const auto& v : fileds){
        hget.add_fields(v);
    }
    client::command_hget_reply reply;
    execute(hget, client::Type::HGET, reply);
    return reply;
}

client::command_reply naruto::kunai::Kunai::slaveof(const std::string &ip, int port) {
    replication::command_slaveof sof;
    sof.set_ip(ip);
    sof.set_port(port);

    client::command_reply reply;
    execute(sof, client::Type::SLAVEOF, reply);
    return reply;
}
