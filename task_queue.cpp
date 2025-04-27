#include "task_queue.hpp"

std::vector<int> fd_list;
std::vector<std::string> login_list;

std::mutex event_mutex;
std::condition_variable event_cv;

std::queue<event> event_queue;

void erase_user(const int& fd)
{
    const auto fd_it = std::find(fd_list.begin(), fd_list.end(), fd);
    if (fd_it == fd_list.end())
        return;

    std::string login = get_login_by_fd(fd);

    const auto login_it = std::find(login_list.begin(), login_list.end(), login);
    if (login_it == login_list.end())
        return;

    {
        std::lock_guard lock(server_mutex);
        fd_list.erase(fd_it);
        login_list.erase(login_it);
    }
}

void send_status_to_all()
{
    {
        std::lock_guard lock(event_mutex);
        event_queue.push(event{send_status, to_all, "", ""});
    }
    event_cv.notify_one();
}

void send_status_to_one(const std::string& login)
{
    {
        std::lock_guard lock(event_mutex);
        event_queue.push(event{send_status, to_one, "", login});
    }
    event_cv.notify_one();
}

void client_send_message_list(const std::string& sender, const std::string& receiver)
{
    {
        std::lock_guard lock(event_mutex);
        event_queue.push(event{send_message_list, to_one, sender, receiver});
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

int get_fd_by_login(const std::string& login)
{
    const auto n = std::find(login_list.begin(), login_list.end(), login);
    if (n == login_list.end())
        return -1;

    return fd_list[n - login_list.begin()];
}

void process_event(const event& cur)
{
    switch (cur.type) {
    case send_status: {
        users_map users = get_users_map();
        std::string user_data = "\005";
        for (const auto& [name, status] : users) {
            user_data += name + "\036" + std::to_string(status) + "\036";
        }
        if (cur.subtype == to_all) {
            for (const auto& fd : fd_list) {
                send(fd, user_data.c_str(), user_data.size(), 0);
            }
        } else {
            int receiver_fd = get_fd_by_login(cur.receiver_login);
            send(receiver_fd, user_data.c_str(), user_data.size(), 0);
        }
        break;
    }
    case send_message_list: {
        int sender_fd = get_fd_by_login(cur.sender_login);
        int receiver_fd = get_fd_by_login(cur.receiver_login);
        messages_list list = get_messages_list(cur.sender_login, cur.receiver_login);
        std::string string_list = "\006";
        for (const auto& message : list) {
            string_list += message.time + "\036";
            string_list += message.sender + "\036";
            string_list += message.text + "\036";
            string_list += std::to_string(message.is_file) + '\036';
        }
        string_list[string_list.size() - 1] = '\0';
        if (sender_fd != -1)
            send(sender_fd, string_list.c_str(), string_list.size(), 0);
        if (receiver_fd != -1)
            send(receiver_fd, string_list.c_str(), string_list.size(), 0);
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