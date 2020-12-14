//
// Created by kwins on 2020/12/8.
//

#ifndef NARUTO_REPLICA_STATE_H
#define NARUTO_REPLICA_STATE_H

#include <string>
#include <iostream>
#include <fstream>
#include <glog/logging.h>
#include <nlohmann/json.hpp>

namespace naruto::replica {

enum class replState {
    NONE = 0,          // 未开始复制
    CONNECT,           // 准备和 master 建立连接
    CONNECTING,        // 已经和 master 建立连接，未进行通信确认
    RECEIVE_PONG,      // 发送 Ping 到 master ，准备接受 master Pong 确认
    WAIT_BGSAVE,       // 等待bgsave完成
    TRANSFOR,          // 已经和master建立连接，并且 PING-PONG 确认连接有效,开始全量同步
    CONNECTED,         // 首次全量同步已经完成，和master连接正常连接，开始正常增量同步，一开始会有增量延迟情况
};


struct replConf {
    bool is_master = false;
    std::string run_id;             // 服务器id
    std::string db_dump_aof_name;   // DB文件 Dump 时的aof文件名
    int64_t db_dump_aof_off = 0;    // DB文件 Dump 时aof文件位置
    std::string master_ip;
    int master_port = 0;
    std::string master_run_id;      // 如果是slave则代表master id
    std::string master_aof_name;    // 如果是slave则代表已同步完成的aof文件名
    int64_t master_aof_off = 0;     // 如果是slave则代表已同步完成的aof文件位置

    void to_json(nlohmann::json& j){
        j["master"] = is_master;
        j["run_id"] = run_id;
        j["db_dump_aof_name"] = db_dump_aof_name;
        j["db_dump_aof_off"] = db_dump_aof_off;
        j["master_ip"] = master_ip;
        j["master_port"] = master_port;
        j["master_run_id"] = master_run_id;
        j["master_aof_name"] = master_aof_name;
        j["master_aof_off"] = master_aof_off;
    }

    void toJson(const std::string& filename){
        std::string tmpfile = filename + ".tmp";
        std::ofstream out(tmpfile, std::ios::out| std::ios::trunc);
        if (!out.is_open()) {
            LOG(ERROR) << "Replica conf toJson " << strerror(errno);
            return;
        }
        nlohmann::json j;
        to_json(j);
        out << j.dump(4);
        out.close();
        if (::rename(tmpfile.c_str(), filename.c_str()) == -1){
            LOG(INFO) << "Repl conf to json " << strerror(errno);
        }
    }

    void from_json(const nlohmann::json& j){
        j.at("master").get_to(is_master);
        j.at("run_id").get_to(run_id);
        j.at("db_dump_aof_name").get_to(db_dump_aof_name);
        j.at("db_dump_aof_off").get_to(db_dump_aof_off);
        j.at("master_run_id").get_to(master_run_id);
        j.at("master_ip").get_to(master_ip);
        j.at("master_port").get_to(master_port);
        j.at("master_aof_name").get_to(master_aof_name);
        j.at("master_aof_off").get_to(master_aof_off);
    }

    void fromJson(const std::string& filename){
        std::ifstream in(filename, std::ios::in);
        if (!in.is_open()) {
            LOG(WARNING) << "Replica parse conf file " << strerror(errno);
            return;
        }
        nlohmann::json j;
        in >> j;
        from_json(j);
        in.close();
    }

    friend std::ostream& operator<<(std::ostream& out, const replConf& repl_conf){
        out << "master:" << repl_conf.is_master << "\n"
            << "run_id:" << repl_conf.run_id << "\n"
            << "db_dump_aof_name:" << repl_conf.db_dump_aof_name << "\n"
            << "db_dump_aof_off:" << repl_conf.db_dump_aof_off << "\n"
            << "master_ip:" << repl_conf.master_ip << "\n"
            << "master_port:" << repl_conf.master_port << "\n"
            << "master_run_id:" << repl_conf.master_run_id << "\n"
            << "master_aof_name:" << repl_conf.master_aof_name << "\n"
            << "master_aof_off:" << repl_conf.master_aof_off << "\n"
                ;
        return out;
    }
};

}

#endif //NARUTO_REPLICA_STATE_H
