//
// Created by 王振奎 on 2020/9/18.
//

#include "replication.h"
#include <chrono>

naruto::Replication::Replication(bool master,
        int back_log_size, int merge_threads_size, int repl_timeout_sec) {
    _master = master;
    _master_host = "";
    _master_port = 0;
    _repl_timeout = repl_timeout_sec;
    _repl_incr_id.store(0);
    _pos.store(0);
    _repl.reserve(merge_threads_size);
    _back_log.reserve(0);
    _master_repl_offset = 0;
    _repl_ping_slave_period = 0;
    _repl_back_size  = back_log_size;
    _repl_backlog_histlen = 0;
    _repl_backlog_idx = 0;
    _repl_backlog_off = 0;
    _repl_state = REPL_STATE_NONE;
    _repl_transfer_size = 0;
    _repl_transfer_read = 0;
    _repl_transfer_last_fsync_off = 0;
    _repl_transfer_s = 0;
    _repl_transfer_fd = 0;
    _repl_transfer_tmpfile = "";
    _repl_transfer_lastio = std::chrono::steady_clock::now();
    _repl_slave_ro = false;
    _repl_down_since = 0;
    _repl_master_runid = "";
    _repl_master_initial_offset = 0;
}

void naruto::Replication::onReplCron(ev::timer &, int) {
    auto now = std::chrono::steady_clock::now();
    auto distens = now - _repl_transfer_lastio;
    if (_master_host.size() > 0 && (_repl_state == REPL_STATE_CONNECTING||
    _repl_state == REPL_STATE_RECEIVE_PONG) && )
}
