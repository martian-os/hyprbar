#include "hyprbar/core/event_loop.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>

namespace hyprbar {

EventLoop::EventLoop()
    : epoll_fd_(-1)
    , next_timer_id_(1)
    , shutdown_requested_(false)
{
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ < 0) {
        throw std::runtime_error("Failed to create epoll instance");
    }
}

EventLoop::~EventLoop() {
    if (epoll_fd_ >= 0) {
        close(epoll_fd_);
    }
}

bool EventLoop::add_fd(int fd, uint32_t events, EventHandler handler) {
    if (fd < 0 || !handler) {
        return false;
    }

    struct epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "Failed to add fd " << fd << " to epoll" << std::endl;
        return false;
    }

    handlers_.push_back({fd, events, handler});
    return true;
}

void EventLoop::remove_fd(int fd) {
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    
    auto it = std::find_if(handlers_.begin(), handlers_.end(),
        [fd](const FdHandler& h) { return h.fd == fd; });
    
    if (it != handlers_.end()) {
        handlers_.erase(it);
    }
}

int EventLoop::add_timer(Duration interval, TimerCallback callback) {
    if (!callback) {
        return -1;
    }

    int id = next_timer_id_++;
    auto now = std::chrono::steady_clock::now();
    
    timers_.push_back({
        id,
        now + interval,
        interval,
        callback,
        true,  // repeating
        false  // not cancelled
    });

    return id;
}

int EventLoop::add_timer_once(Duration delay, TimerCallback callback) {
    if (!callback) {
        return -1;
    }

    int id = next_timer_id_++;
    auto now = std::chrono::steady_clock::now();
    
    timers_.push_back({
        id,
        now + delay,
        Duration(0),
        callback,
        false,  // one-shot
        false   // not cancelled
    });

    return id;
}

void EventLoop::cancel_timer(int timer_id) {
    for (auto& timer : timers_) {
        if (timer.id == timer_id) {
            timer.cancelled = true;
            break;
        }
    }
}

void EventLoop::process_timers() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto& timer : timers_) {
        if (timer.cancelled) {
            continue;
        }

        if (now >= timer.next_expiry) {
            handle_expired_timer(timer, now);
        }
    }

    remove_cancelled_timers();
}

void EventLoop::handle_expired_timer(Timer& timer, TimePoint now) {
    timer.callback();
    
    if (timer.repeating) {
        timer.next_expiry = now + timer.interval;
    } else {
        timer.cancelled = true;
    }
}

void EventLoop::remove_cancelled_timers() {
    timers_.erase(
        std::remove_if(timers_.begin(), timers_.end(),
            [](const Timer& t) { return t.cancelled; }),
        timers_.end()
    );
}

int EventLoop::get_next_timer_timeout() const {
    if (timers_.empty()) {
        return -1;  // No timers, wait indefinitely
    }

    auto now = std::chrono::steady_clock::now();
    auto next_expiry = timers_[0].next_expiry;

    for (const auto& timer : timers_) {
        if (!timer.cancelled && timer.next_expiry < next_expiry) {
            next_expiry = timer.next_expiry;
        }
    }

    auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
        next_expiry - now
    );

    return std::max(0, static_cast<int>(timeout.count()));
}

bool EventLoop::dispatch(int timeout_ms) {
    if (shutdown_requested_) {
        return false;
    }

    // Process expired timers first
    process_timers();

    // Calculate timeout considering next timer
    int actual_timeout = timeout_ms;
    if (!timers_.empty()) {
        int timer_timeout = get_next_timer_timeout();
        if (timeout_ms < 0) {
            actual_timeout = timer_timeout;
        } else if (timer_timeout >= 0) {
            actual_timeout = std::min(timeout_ms, timer_timeout);
        }
    }

    // Wait for events
    const int max_events = 32;
    struct epoll_event events[max_events];
    
    int n = epoll_wait(epoll_fd_, events, max_events, actual_timeout);
    
    if (n < 0) {
        if (errno == EINTR) {
            return !shutdown_requested_;
        }
        std::cerr << "epoll_wait failed: " << errno << std::endl;
        return false;
    }

    // Process ready file descriptors
    for (int i = 0; i < n; ++i) {
        int fd = events[i].data.fd;
        uint32_t revents = events[i].events;

        auto it = std::find_if(handlers_.begin(), handlers_.end(),
            [fd](const FdHandler& h) { return h.fd == fd; });
        
        if (it != handlers_.end()) {
            it->handler(fd, revents);
        }
    }

    // Process timers again after handling events
    process_timers();

    return !shutdown_requested_;
}

void EventLoop::shutdown() {
    shutdown_requested_ = true;
}

}  // namespace hyprbar
