//
// Created by kwins on 2020/12/8.
//

#include "replica_link.h"

#include <utility>
#include "connect_worker.h"
#include "replication.h"
#include "utils/file.h"
#include "protocol/client.pb.h"
#include "protocol/command_types.pb.h"

DECLARE_double(repl_aof_interval);
DECLARE_string(repl_dir);

naruto::replica::replicaLink::replicaLink(
        naruto::connection::ConnectOptions opts): link::clientLink(std::move(opts),worker_num) {
    repl_aof_off = 0;
    repl_aof_file_name = "";
    repl_db_f = nullptr;
    repl_dbsize = 0;
    repl_dboff = 0;
    repl_run_id = "";
    repl_state = replState::NONE;
}

naruto::replica::replicaLink::replicaLink(int sd) : link::clientLink(worker_num, sd) {
    repl_aof_off = 0;
    repl_aof_file_name = "";
    repl_db_f = nullptr;
    repl_dbsize = 0;
    repl_dboff = 0;
    repl_run_id = "";
    repl_state = replState::NONE;
}

void naruto::replica::replicaLink::onRead(ev::io &watcher, int events) {
    clientLink::onRead(watcher, events);
}

void naruto::replica::replicaLink::onWrite(ev::io &watcher, int events) {
    clientLink::onWrite(watcher, events);
}

void naruto::replica::replicaLink::close() const {
    clientLink::close();
    if (repl_rio) repl_rio->stop();
    if (repl_tio) repl_tio->stop();
    if (repl_db_f) repl_db_f->close();
}

void naruto::replica::replicaLink::onSendBulkToSlave(ev::io &watcher, int event) {
    int transfer_len = 1024*16;
    if (!repl_db_f) return;
    LOG(INFO) << "Master send bulk to slave...";
    char buf[transfer_len];
    ssize_t nwritten, buflen;
    repl_db_f->seekg(repl_dboff, std::ios::beg);
    repl_db_f->read(buf, transfer_len);
    if ((buflen = repl_db_f->gcount()) <= 0){
        LOG(ERROR) << "Read error sending DB to slave: "
            << ((buflen == 0) ? "premature EOF" : strerror(errno));
        close();
        return;
    }
    if ((nwritten = ::write(connect->fd(), buf, buflen)) == -1){
        if (errno != EAGAIN){
            LOG(WARNING) << "Write error sending DB to slave: " << strerror(errno);
            close();
        }
        return;
    }

    repl_dboff += nwritten;
    if (repl_dboff == repl_dbsize){
        watcher.stop();
        delete &watcher;
        repl_db_f->close();
        repl_db_f = nullptr;
        repl_state = replState::CONNECTED;

        repl_tio = std::make_shared<ev::timer>();
        repl_tio ->set<replicaLink, &replicaLink::onSendIncrToSlave>(this);
        repl_tio->set(ev::get_default_loop());
        repl_tio->start(FLAGS_repl_aof_interval,FLAGS_repl_aof_interval);
        replptr->addSlave(this);
        LOG(INFO) << "Synchronization db with slave succeeded";
    }
}

void naruto::replica::replicaLink::onSendIncrToSlave(ev::timer &, int) {
    if (repl_aof_file_name.empty()) return;
    int transfer_len = 1024*16;
    LOG(INFO) << "onSendIncrToSlave----->>2-->>"
              << " repl_aof_file_name " << repl_aof_file_name
              << " repl_aof_off " << repl_aof_off;

    std::string aofname = FLAGS_repl_dir + "/" + repl_aof_file_name;
    std::ifstream in(aofname, std::ios::in|std::ios::binary);
    if (!in.is_open()) return;
    if (in.peek() == EOF){ // 文件为空
        if (utils::File::hasNextAof(FLAGS_repl_dir, repl_aof_file_name)){
            repl_aof_off = 0;
        }
        in.close();
        return;
    }

    in.seekg(repl_aof_off, std::ios::beg);
    in.peek();
    int64_t readed = 0, pack_nums = 0;
    client::command_aof multi;
    char temp[4096];
    utils::Bytes buffer;

    while (!in.eof()){
        in.read(temp, sizeof(temp));
        buffer.putBytes((uint8_t*)temp, in.gcount());
        readed += (int64_t)in.gcount();

        while (buffer.bytesRemaining() > PACK_SIZE_LEN){
            uint32_t pack_size = buffer.getInt();
            if (buffer.bytesRemaining() < pack_size - sizeof(pack_size)){
                break; // 读取不够一个包
            }
            uint16_t fg = buffer.getShort();
            uint16_t type = buffer.getShort();

            unsigned body_size = pack_size - PACK_HEAD_LEN;
            char msg[body_size + 1];
            buffer.getBytes((uint8_t*)msg, body_size);
            msg[body_size] = '\0';
            multi.mutable_types()->Add(type);
            multi.mutable_commands()->Add(msg);
            pack_nums++;
        }
        if (readed >= transfer_len) break;
    }

    if (in.eof() && utils::File::hasNextAof(FLAGS_repl_dir,repl_aof_file_name)){
        repl_aof_off = 0;
    }else{
        repl_aof_off +=  readed;
    }

    in.close();
    if (buffer.size() == 0) return;

    multi.set_aof_name(repl_aof_file_name);
    multi.set_aof_off(repl_aof_off);
    sendMsg(multi, cmdtype::REPL_MULTI);
    LOG(INFO) << "onSendIncrToSlave----->>10-->>send size=" << readed << " pack_nums=" << pack_nums;
}
