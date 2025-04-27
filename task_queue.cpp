#include "task_queue.hpp"

std::vector<int> fd_list;
std::vector<std::string> login_list;

std::mutex event_mutex;
std::condition_variable event_cv;

std::queue<event> event_queue;

void client_changed_status()
{
    {
        std::lock_guard lock(event_mutex);
        event_queue.push(event{change_status, -1, -1});
    }
    event_cv.notify_one();
}

void client_send_message(int sender_fd, int receiver_fd)
{
    {
        std::lock_guard lock(event_mutex);
        event_queue.push(event{send_message, sender_fd, receiver_fd});
    }
    event_cv.notify_one();
}

std::string get_login_by_fd(const int& fd)
{
    const auto n = std::find(fd_list.begin(), fd_list.end(), fd);
    if (n == fd_list.end())
        return std::string("");

    return std::string(login_list[n - fd_list.begin()]);
}

void process_event(const event& cur)
{
    switch (cur.type) {
    case change_status: {
        users_map users = get_users_map();
        for (const auto& fd : fd_list) {
            std::string user_data;
            for (const auto& [name, id] : users) {
                user_data += std::to_string(id) + ":" + name + "\n";
            }
            send(fd, user_data.c_str(), user_data.size(), 0);
        }
        break;
    }
    case send_message: {
        messages_list list = get_messages_list(get_login_by_fd(cur.sender_fd), get_login_by_fd(cur.receiver_fd));
        std::string string_list;
        for (const auto& message : list) {
            string_list += message.time + '\30';
            string_list += message.sender + '\30';
            string_list += message.text + '\30';
            string_list += std::to_string(message.is_file) + '\30';
        }
        send(cur.sender_fd, string_list.c_str(), string_list.size(), 0);
        send(cur.receiver_fd, string_list.c_str(), string_list.size(), 0);
        break;
    }
    default:
        break;
    }
}

void handler()
{
    for (;;) {
        std::unique_lock lock(event_mutex);

        event_cv.wait(lock, [] { return !event_queue.empty(); });
        event tmp = event_queue.front();
        event_queue.pop();

        lock.unlock();

        process_event(tmp);
    }
}