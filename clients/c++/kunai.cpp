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
    conn->send(pack);
    pack.clear();
    conn->recv(pack);
    utils::Pack::deSerialize(pack, reply);
    pool_->release(std::move(conn));
}

client::command_hget_reply
naruto::kunai::Kunai::hget(const std::string &key, std::initializer_list<std::string> fileds) {
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

void naruto::kunai::Kunai::hset(const std::string &key, const std::string &field, const std::string &v) {
    client::command_hset hset;
    hset.set_key(key);
    hset.set_field(field);
    hset.mutable_value()->mutable_bytes_v()->set_value(v);
    client::command_reply reply;
    execute(hset, client::Type::HSET, reply);
    if (reply.errcode() != 0){
        LOG(ERROR) << "Kunai hset error (code:" << reply.errcode() << " errmsg:" << reply.errmsg() << ")";
    }else{
        LOG(INFO) << "Reponse " << reply.DebugString();
    }
}

void naruto::kunai::Kunai::hset(const std::string &key, const std::string &field, int64_t v) {
    client::command_hset hset;
    hset.set_key(key);
    hset.set_field(field);
    hset.mutable_value()->mutable_int64_v()->set_value(v);
    client::command_reply reply;
    execute(hset, client::Type::HSET, reply);
    if (reply.errcode() != 0){
        LOG(ERROR) << "Kunai hset error (code:" << reply.errcode() << " errmsg:" << reply.errmsg() << ")";
    }else{
        LOG(INFO) << "Reponse " << reply.DebugString();
    }
}

void naruto::kunai::Kunai::hset(const std::string &key, const std::string &field, float v) {
    client::command_hset hset;
    hset.set_key(key);
    hset.set_field(field);
    hset.mutable_value()->mutable_float_v()->set_value(v);
    client::command_reply reply;
    execute(hset, client::Type::HSET, reply);
    if (reply.errcode() != 0){
        LOG(ERROR) << "Kunai hset error (code:" << reply.errcode() << " errmsg:" << reply.errmsg() << ")";
    }
}

void naruto::kunai::Kunai::hincr(const std::string &key, const std::string &field, float v) {
    client::command_hincr hincr;
    hincr.set_key(key);
    hincr.set_field(field);
    hincr.mutable_value()->mutable_float_v()->set_value(v);
    client::command_reply reply;
    execute(hincr, client::Type::HINCR, reply);
    if (reply.errcode() != 0){
        LOG(ERROR) << "Kunai hincr error (code:" << reply.errcode() << " errmsg:" << reply.errmsg() << ")";
    }
}

void naruto::kunai::Kunai::hincr(const std::string &key, const std::string &field, int64_t v) {
    client::command_hincr hincr;
    hincr.set_key(key);
    hincr.set_field(field);
    hincr.mutable_value()->mutable_int64_v()->set_value(v);
    client::command_reply reply;
    execute(hincr, client::Type::HINCR, reply);
    if (reply.errcode() != 0){
        LOG(ERROR) << "Kunai hincr error (code:" << reply.errcode() << " errmsg:" << reply.errmsg() << ")";
    }
}
